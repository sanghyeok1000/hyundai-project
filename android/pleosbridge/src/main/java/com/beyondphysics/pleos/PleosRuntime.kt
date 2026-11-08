package com.beyondphysics.pleos

import ai.pleos.playground.vehicle.Vehicle
import ai.pleos.playground.vehicle.api.model.capability.AccessType
import ai.pleos.playground.vehicle.listener.BrakePedalStatusListener
import ai.pleos.playground.vehicle.listener.SteeringWheelAngleListener
import android.app.Activity
import android.car.Car
import android.car.VehicleGear
import android.car.VehiclePropertyIds
import android.car.drivingstate.CarUxRestrictions
import android.car.drivingstate.CarUxRestrictionsManager
import android.car.hardware.CarPropertyValue
import android.car.hardware.property.CarPropertyManager
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import org.json.JSONObject
import java.util.concurrent.Executors

/** SDK bridge. Vehicle input and optional outputs deliberately enable only the Pleos emulator. */
object PleosRuntime {
    private val main = Handler(Looper.getMainLooper())
    @Volatile private var session: Session? = null // mutation on main thread only
    private var retiring: Session? = null // a new Vehicle must not race old output restoration
    @Volatile private var frame = Frame()
    @Volatile private var foreground = false
    private val interruptions = InterruptionTracker()

    private fun beginSession(context: Context) {
        if (!foreground) return
        if (retiring != null) {
            main.postDelayed({ if (session == null) beginSession(context) }, 250)
            return
        }
        try {
            val next = Session(context)
            session = next; next.start()
        } catch (e: Exception) {
            Log.e("PleosBridge", "Start failed", e)
            session?.close(); session = null
            interruptions.observe(false)
            frame = Frame(detail = "SDK START FAILED")
        }
    }

    private data class Frame(
        val foreground: Boolean = false, val emulator: Boolean = false,
        val parked: Boolean? = null, val requiresDo: Boolean? = null, val flags: Int? = null,
        val angle: Float? = null, val brake: Boolean? = null,
        val gearAt: Long = 0, val uxAt: Long = 0, val angleAt: Long = 0, val brakeAt: Long = 0,
        val detail: String = "SDK STARTING"
    ) {
        fun reason(now: Long): String = when {
            !foreground -> "APP IN BACKGROUND"
            !emulator -> "EMULATOR PROFILE ONLY"
            parked == false -> "PARK REQUIRED"
            parked == null || requiresDo == null || flags == null -> "WAITING FOR PARK / UX"
            requiresDo || flags != 0 -> "VEHICLE UX BLOCKED"
            !InputGate.fresh(now, minOf(gearAt, uxAt)) -> "PARK / UX SIGNAL STALE"
            angle == null || !angle.isFinite() || kotlin.math.abs(angle) > 540f -> "STEERING UNAVAILABLE"
            brake == null -> "BRAKE UNAVAILABLE"
            !InputGate.fresh(now, angleAt) || !InputGate.fresh(now, brakeAt) -> "VEHICLE INPUT STALE"
            else -> "READY"
        }
    }

    @JvmStatic fun start(activity: Activity) {
        main.post {
            foreground = true
            if (session == null) {
                beginSession(activity.applicationContext)
            }
        }
    }

    @JvmStatic fun stop() {
        // Invalidate immediately, before the main-thread cleanup or a Binder callback.
        foreground = false
        interruptions.observe(false)
        frame = Frame(detail = "ACTIVITY PAUSED")
        main.post { val old = session; session = null; old?.close() }
    }

    @JvmStatic fun snapshot(): String {
        val published = frame // one immutable publication, safe from Unreal's game thread
        val f = if (foreground) published else published.copy(foreground = false)
        val now = SystemClock.elapsedRealtime()
        val reason = f.reason(now)
        val allowed = reason == "READY" && InputGate.allowed(f.parked, f.requiresDo, f.flags,
            f.angle, f.brake, now, minOf(f.gearAt, f.uxAt), f.angleAt, f.brakeAt, f.foreground)
        val lossEpoch = interruptions.observe(allowed)
        return JSONObject().apply {
            put("schema", 1); put("allowed", allowed); put("reason", reason); put("detail", f.detail)
            put("lossEpoch", lossEpoch)
            put("rawSteering", f.angle?.takeIf { it.isFinite() } ?: JSONObject.NULL)
            put("steering", if (allowed) InputGate.emulatorSteer(f.angle!!) else 0f)
            put("brake", if (allowed && f.brake == true) 1 else 0)
            put("parked", f.parked ?: JSONObject.NULL)
            put("profile", "PLEOS_EMULATOR_RAW_540_POSITIVE_LEFT")
        }.toString()
    }

    /** C++ intent is advisory; the current immutable input publication is checked again per write. */
    @JvmStatic fun feedback(command: String): String = session?.feedback(command) ?: JSONObject().apply {
        put("schema", 1); put("lights", "OFF"); put("climate", "OFF"); put("detail", "SDK SESSION INACTIVE")
    }.toString()

    private fun outputAllowed(): Boolean {
        val f = frame
        val now = SystemClock.elapsedRealtime()
        return foreground && f.emulator && f.reason(now) == "READY" &&
            InputGate.allowed(f.parked, f.requiresDo, f.flags, f.angle, f.brake, now,
                minOf(f.gearAt, f.uxAt), f.angleAt, f.brakeAt, f.foreground)
    }

    private class Session(private val context: Context) {
        private val emulator = Build.HARDWARE == "ranchu" &&
            Build.SUPPORTED_ABIS.contains("x86_64") && Build.MODEL.contains("Connect")
        private val vehicle = Vehicle(context)
        private val steering = vehicle.getSteeringWheel()
        private val brakeApi = vehicle.getBrake()
        private val worker = Executors.newSingleThreadExecutor()
        private var state = Frame(foreground = true, emulator = emulator)
        @Volatile private var active = true
        @Volatile private var effects: PleosEffects? = null
        @Volatile private var effectsError = ""
        private var effectsIdle = true
        private var closing = false
        private var pending = 0
        private var released = false
        private var releaseAttempts = 0
        private val cancellations = mutableSetOf<() -> Unit>()
        private var steerRead = false; private var brakeRead = false
        private var steerCapPending = false; private var brakeCapPending = false
        private var steerPending = false; private var brakePending = false
        private var nextSteerCapability = 0L; private var nextBrakeCapability = 0L
        private var steerRegistered = false; private var brakeRegistered = false
        private var steerVersion = 0; private var brakeVersion = 0; private var gearVersion = 0; private var uxVersion = 0
        private var car: Car? = null
        private var ux: CarUxRestrictionsManager? = null
        private var props: CarPropertyManager? = null
        private var gearListener: CarPropertyManager.CarPropertyEventCallback? = null
        private var carEpoch = 0
        private var carPollPending = false
        private val tick = object : Runnable {
            override fun run() { if (active) { poll(); main.postDelayed(this, 250) } }
        }

        private fun emit(next: Frame) {
            if (active && session === this) {
                val now = SystemClock.elapsedRealtime()
                interruptions.observeUpdate(foreground && state.reason(now) == "READY", foreground && next.reason(now) == "READY")
                frame = next
                if (state.reason(now) == "READY" && next.reason(now) != "READY") effects?.revoke()
            }
            state = next
        }
        private fun post(action: () -> Unit) { main.post { if (active) action() } }
        // Normal cleanup waits for completion. SDK2.3.2 can lose callbacks on Binder death:
        // after5s we logically cancel/retire the entire session and ignore its late results.
        private fun request(settled: () -> Unit, start: (finish: (() -> Unit) -> Unit) -> Unit) {
            pending++
            var done = false
            lateinit var timeout: Runnable
            lateinit var cancel: () -> Unit
            cancel = {
                if (!done) {
                    done = true; pending--; settled()
                    main.removeCallbacks(timeout); cancellations.remove(cancel)
                }
            }
            val finish: (() -> Unit) -> Unit = { action -> main.post {
                if (!done) {
                    cancel()
                    try { if (active) action() } catch (e: Exception) { invalidate(e) }
                    finally { releaseWhenIdle() }
                }
            } }
            timeout = Runnable {
                if (!done) {
                    cancel()
                    if (active) {
                        Log.w("PleosBridge", "SDK request deadline; retiring session")
                        interruptions.observe(false)
                        emit(state.copy(angle = null, brake = null, detail = "SDK REQUEST TIMEOUT"))
                        if (session === this) {
                            session = null; close(abandonPending = true)
                            main.postDelayed({ if (foreground && session == null) beginSession(context) }, 1000)
                        }
                    } else releaseWhenIdle()
                }
            }
            cancellations.add(cancel); main.postDelayed(timeout, 5000)
            try { start(finish) } catch (e: Exception) { finish { invalidate(e) } }
        }
        private fun invalidate(e: Exception) {
            Log.w("PleosBridge", "Vehicle read failed", e)
            emit(state.copy(angle = null, brake = null, angleAt = 0, brakeAt = 0, detail = e.javaClass.simpleName))
        }
        private val steerListener = object : SteeringWheelAngleListener {
            override fun onSteeringWheelAngleUpdated(angle: Float?) {
                val at = SystemClock.elapsedRealtime()
                post { steerVersion++; emit(state.copy(angle = angle, angleAt = at)) }
            }
            override fun onFailed(e: Exception) = post { steerVersion++; invalidate(e) }
        }
        private val brakeListener = object : BrakePedalStatusListener {
            override fun onBrakePedalStatusUpdated(status: Boolean?) {
                val at = SystemClock.elapsedRealtime()
                post { brakeVersion++; emit(state.copy(brake = status, brakeAt = at)) }
            }
            override fun onFailed(e: Exception) = post { brakeVersion++; invalidate(e) }
        }

        fun start() {
            emit(state)
            if (!emulator || Build.VERSION.SDK_INT < 29) return
            vehicle.initialize()
            // A corrupt recovery record or denied output API must not invalidate steering/brake input.
            try {
                effects = PleosEffects(context, vehicle) { active && session === this && outputAllowed() }
                effectsIdle = false
            } catch (e: Exception) {
                effectsError = "OUTPUT INIT FAILED: ${e.javaClass.simpleName}"
                Log.w("PleosEffects", effectsError, e)
            }
            car = Car.createCar(context, main, Car.CAR_WAIT_TIMEOUT_DO_NOT_WAIT) { connected, ready ->
                if (!active) connected.disconnect()
                else {
                    car = connected; carEpoch++
                    if (ready) attachCar(connected, carEpoch)
                    else { ux = null; props = null; emit(state.copy(parked = null, requiresDo = null, flags = null, gearAt = 0, uxAt = 0)) }
                }
            }
            main.post(tick)
        }

        fun feedback(command: String): String = effects?.feedback(command) ?: JSONObject().apply {
            put("schema", 1); put("lights", if (effectsError.isEmpty()) "UNSUPPORTED" else "ERROR")
            put("climate", if (effectsError.isEmpty()) "UNSUPPORTED" else "ERROR")
            put("detail", effectsError.ifEmpty { "PLEOS EMULATOR PROFILE ONLY" })
        }.toString()

        private fun poll() {
            val now = SystemClock.elapsedRealtime()
            if (!steerRead && !steerCapPending && now >= nextSteerCapability) {
                steerCapPending = true; nextSteerCapability = now + 2000
                request({ steerCapPending = false }) { finish -> steering.checkSteeringWheelAngleCapabilities(
                    onComplete = { config, _ -> finish {
                        steerCapPending = false
                        steerRead = config.accessType == AccessType.READ || config.accessType == AccessType.READ_WRITE
                        if (steerRead && !steerRegistered) {
                            steering.registerSteeringWheelAngle(steerListener); steerRegistered = true
                        }
                    } }, onFailed = { e -> finish { steerCapPending = false; invalidate(e) } }) }
            }
            if (!brakeRead && !brakeCapPending && now >= nextBrakeCapability) {
                brakeCapPending = true; nextBrakeCapability = now + 2000
                request({ brakeCapPending = false }) { finish -> brakeApi.checkBrakePedalStatusCapabilities(
                    onComplete = { config -> finish {
                        brakeCapPending = false
                        brakeRead = config.accessType == AccessType.READ || config.accessType == AccessType.READ_WRITE
                        if (brakeRead && !brakeRegistered) {
                            brakeApi.registerBrakePedalStatus(brakeListener); brakeRegistered = true
                        }
                    } }, onFailed = { e -> finish { brakeCapPending = false; invalidate(e) } }) }
            }
            if (steerRead && !steerPending) {
                steerPending = true; val version = steerVersion
                request({ steerPending = false }) { finish -> steering.getCurrentSteeringWheelAngle(
                    onComplete = { angle -> finish {
                        steerPending = false
                        if (version == steerVersion) emit(state.copy(angle = angle, angleAt = now))
                    } }, onFailed = { e -> finish { steerPending = false; invalidate(e) } }) }
            }
            if (brakeRead && !brakePending) {
                brakePending = true; val version = brakeVersion
                request({ brakePending = false }) { finish -> brakeApi.getCurrentBrakePedalStatus(
                    onComplete = { value -> finish {
                        brakePending = false
                        if (version == brakeVersion) emit(state.copy(brake = value, brakeAt = now))
                    } }, onFailed = { e -> finish { brakePending = false; invalidate(e) } }) }
            }
            val p = props; val u = ux; val epoch = carEpoch
            if (p != null && u != null && !carPollPending) {
                carPollPending = true; val gv = gearVersion; val uv = uxVersion
                worker.execute {
                    val gear = runCatching { p.getIntProperty(VehiclePropertyIds.GEAR_SELECTION, 0) }
                    val restrictions = runCatching { u.currentCarUxRestrictions }
                    main.post {
                        carPollPending = false
                        if (active && epoch == carEpoch) {
                            if (gv == gearVersion) applyGear(gear.getOrNull(), now)
                            if (uv == uxVersion) applyUx(restrictions.getOrNull(), now)
                        }
                    }
                }
            }
        }

        @Suppress("DEPRECATION")
        private fun attachCar(connected: Car, epoch: Int) {
            emit(state.copy(parked = null, requiresDo = null, flags = null, gearAt = 0, uxAt = 0))
            try {
                ux = connected.getCarManager(Car.CAR_UX_RESTRICTION_SERVICE) as CarUxRestrictionsManager
                ux!!.registerListener { value ->
                    val at = SystemClock.elapsedRealtime()
                    post { if (epoch == carEpoch) { uxVersion++; applyUx(value, at) } }
                }
                props = connected.getCarManager(Car.PROPERTY_SERVICE) as CarPropertyManager
                val listener = object : CarPropertyManager.CarPropertyEventCallback {
                    override fun onChangeEvent(value: CarPropertyValue<*>) {
                        val at = SystemClock.elapsedRealtime()
                        post {
                        if (epoch == carEpoch && value.propertyId == VehiclePropertyIds.GEAR_SELECTION) {
                            gearVersion++
                            applyGear(if (value.status == CarPropertyValue.STATUS_AVAILABLE) value.value as? Int else null,
                                at)
                        }
                        }
                    }
                    override fun onErrorEvent(id: Int, area: Int) = post {
                        if (epoch == carEpoch && id == VehiclePropertyIds.GEAR_SELECTION) {
                            gearVersion++; applyGear(null, 0)
                        }
                    }
                }
                gearListener = listener
                check(props!!.registerCallback(listener, VehiclePropertyIds.GEAR_SELECTION, CarPropertyManager.SENSOR_RATE_ONCHANGE))
            } catch (e: Exception) {
                ux = null; props = null
                emit(state.copy(parked = null, requiresDo = null, flags = null, detail = "CAR SERVICE: ${e.javaClass.simpleName}"))
                Log.w("PleosBridge", "Car attach failed", e)
            }
        }
        private fun applyGear(value: Int?, at: Long) = emit(state.copy(
            parked = if (value == null || value == VehicleGear.GEAR_UNKNOWN) null else value == VehicleGear.GEAR_PARK, gearAt = at))
        private fun applyUx(value: CarUxRestrictions?, at: Long) = emit(state.copy(
            requiresDo = value?.isRequiresDistractionOptimization, flags = value?.activeRestrictions, uxAt = at))

        @Suppress("DEPRECATION")
        fun close(abandonPending: Boolean = false) {
            if (closing) return
            closing = true; retiring = this
            active = false; carEpoch++; main.removeCallbacks(tick)
            effects?.close { main.post { effectsIdle = true; releaseWhenIdle() } }
            if (steerRegistered) runCatching { steering.unregisterSteeringWheelAngle(steerListener) }
            if (brakeRegistered) runCatching { brakeApi.unregisterBrakePedalStatus(brakeListener) }
            runCatching { ux?.unregisterListener() }
            gearListener?.let { listener -> runCatching { props?.unregisterCallback(listener) } }
            runCatching { car?.disconnect() }; worker.shutdownNow()
            if (abandonPending) cancellations.toList().forEach { it() }
            releaseWhenIdle()
        }
        private fun releaseWhenIdle() {
            if (!active && pending == 0 && effectsIdle && !released && releaseAttempts < 3) {
                releaseAttempts++
                runCatching { vehicle.release() }.onSuccess {
                    released = true
                    if (retiring === this) retiring = null
                }.onFailure {
                    Log.w("PleosBridge", "Release attempt $releaseAttempts failed", it)
                    if (releaseAttempts < 3) main.postDelayed({ releaseWhenIdle() }, 500)
                }
            }
        }
    }
}
