package com.beyondphysics.pleos

import ai.pleos.playground.vehicle.Vehicle
import ai.pleos.playground.vehicle.api.model.area.SeatArea
import ai.pleos.playground.vehicle.api.model.capability.AccessType
import ai.pleos.playground.vehicle.api.model.capability.AreaBounds
import ai.pleos.playground.vehicle.api.model.capability.CapabilityConfig
import ai.pleos.playground.vehicle.constant.area.SeatValue
import ai.pleos.playground.vehicle.constant.unit.TemperatureUnit
import android.content.Context
import android.os.SystemClock
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import java.util.concurrent.atomic.AtomicReference

/** All SDK output runs on one worker, at most twice a second, outside the input/read session. */
internal class PleosEffects(context: Context, vehicle: Vehicle, private val allowed: () -> Boolean) {
    private data class Command(val active: Boolean = false, val lights: Boolean = false,
        val climate: Boolean = false, val color: Long = 0xff77ccccL, val brightness: Double = 0.12,
        val fan: Int = 1, val coolOffset: Double = 0.0, val map: Int = 0)
    private val desired = AtomicReference(Command())
    private val worker = Executors.newSingleThreadScheduledExecutor()
    private val device = SdkEffectsDevice(vehicle)
    private val owner = EffectOwnership(device, PreferencesEffectJournal(context))
    @Volatile private var closed = false
    private val heartbeat = EffectHeartbeat()
    @Volatile private var status = response("OFF", "OFF", "Effects disabled by default")
    private var map = -1
    private var recovered = false
    private var lightState = "OFF"
    private var climateState = "OFF"
    private var lightFailure: String? = null
    private var climateFailure: String? = null
    private var lastDetail = ""
    private val groupDetails = mutableMapOf<String, String>()

    init { worker.scheduleWithFixedDelay({ runCycle() }, 0, 500, TimeUnit.MILLISECONDS) }

    fun feedback(command: String): String {
        if (closed) return status
        try {
            val json = JSONObject(command)
            require(json.getInt("schema") == 1)
            val color = json.getLong("color")
            val brightness = json.getDouble("brightness")
            val fan = json.getInt("fan")
            val cooling = json.getDouble("coolOffset")
            val map = json.getInt("map")
            require(color in 0..0xffffffffL && brightness.isFinite() && cooling.isFinite() &&
                brightness in 0.0..1.0 && fan in 1..2 && cooling in 0.0..1.0 && map in 0..2)
            val active = json.getBoolean("active")
            desired.set(Command(heartbeat.receive(active, SystemClock.elapsedRealtime()), json.getBoolean("lights"),
                json.getBoolean("climate"), color, brightness, fan, cooling, map))
        } catch (e: Exception) {
            desired.set(Command())
            status = response("ERROR", "ERROR", "Invalid feedback command")
        }
        return status
    }

    private fun runCycle() {
        if (closed) return
        try {
            if (!recovered) {
                lightState = owner.restore("lights"); climateState = owner.restore("climate")
                recovered = lightState != "RESTORING" && climateState != "RESTORING"
                if (!recovered) { publish(); return }
            }
            val c = desired.get()
            val enabled = c.active && heartbeat.valid(SystemClock.elapsedRealtime()) && allowed()
            if (c.active && !enabled) heartbeat.revoke()
            if (c.map != map) {
                lightState = owner.restore("lights", reset = true)
                climateState = owner.restore("climate", reset = true)
                if (lightState == "RESTORING" || climateState == "RESTORING") { publish(); return }
                map = c.map; lightFailure = null; climateFailure = null
                groupDetails.clear(); lastDetail = ""
            }
            fun valid(group: String): Boolean {
                val next = desired.get()
                val safe = allowed()
                if (!safe && next.active) heartbeat.revoke()
                return !closed && heartbeat.valid(SystemClock.elapsedRealtime()) && safe && next.active && next.map == c.map &&
                    (if (group == "lights") next.lights else next.climate)
            }
            if (!enabled || !c.lights) {
                lightState = owner.restore("lights", reset = !c.lights)
                if (!c.lights) {
                    lightFailure = null
                    if (lightState != "RESTORING") groupDetails.remove("lights")
                }
            } else if (lightFailure == null) {
                lightState = applyLights(c) { valid("lights") }
                remember("lights", lightState)
                if (lightState in listOf("ERROR", "UNSUPPORTED")) lightFailure = lightState
            } else lightState = lightFailure!!
            if (!enabled || !c.climate) {
                climateState = owner.restore("climate", reset = !c.climate)
                if (!c.climate) {
                    climateFailure = null
                    if (climateState != "RESTORING") groupDetails.remove("climate")
                }
            } else if (climateFailure == null) {
                climateState = applyClimate(c) { valid("climate") }
                remember("climate", climateState)
                if (climateState in listOf("ERROR", "UNSUPPORTED")) climateFailure = climateState
            } else climateState = climateFailure!!
            publish()
        } catch (e: Exception) {
            lastDetail = e.message ?: e.javaClass.simpleName
            lightState = owner.restore("lights"); climateState = owner.restore("climate")
            status = response(if (lightState == "RESTORING") lightState else "ERROR",
                if (climateState == "RESTORING") climateState else "ERROR", lastDetail)
            Log.w("PleosEffects", "Output cycle failed; recovery journal retained", e)
        }
    }
    private fun applyLights(c: Command, valid: () -> Boolean): String = try {
        device.prepare("lights")
        val brightness = device.lightBounds!!
        // A nonzero minimum above 20% of the supported span cannot provide a low setting.
        val ceiling = kotlin.math.floor(brightness.maxValue * 0.20).toInt()
        if (brightness.minValue > ceiling) throw EffectUnsupported("No verified low ambient brightness")
        val value = (brightness.maxValue * c.brightness.coerceIn(0.0, 0.20)).toInt()
            .coerceIn(brightness.minValue, ceiling)
        owner.apply("lights", mapOf(device.propertyId("color") to EffectColor.emulatorOpaqueArgb(c.color.toInt()).toDouble(),
            device.propertyId("brightness") to value.toDouble()), valid)
    } catch (e: Exception) { failure("lights", e) }

    private fun applyClimate(c: Command, valid: () -> Boolean): String {
      return try {
        device.prepare("climate")
        val fanBounds = device.fanBounds!!
        val target = c.fan.coerceIn(maxOf(1, fanBounds.minValue), minOf(2, fanBounds.maxValue))
        val fanId = device.propertyId("fan")
        val tempId = device.optionalPropertyId("temperature")
        // Capture once via owner before using the original temperature as the cooling reference.
        val capture = owner.apply("climate", emptyMap(), valid)
        if (capture != "APPLIED") return capture
        val values = linkedMapOf(fanId to target.toDouble())
        val original = tempId?.let { owner.original(it) }
        val bounds = device.temperatureBounds
        if (c.map != 2 && tempId != null && original != null && bounds != null && device.acOn) {
            EffectLimits.cooling(original, c.coolOffset, bounds.minValue.toDouble(),
                bounds.maxValue.toDouble(), device.temperatureStep)?.let { values[tempId] = it }
        }
        owner.apply("climate", values, valid)
      } catch (e: Exception) { failure("climate", e) }
    }

    private fun failure(group: String, e: Exception): String {
        groupDetails[group] = "$group: ${e.message ?: e.javaClass.simpleName}"
        Log.w("PleosEffects", "$group output failed", e)
        val restored = owner.restore(group)
        return if (restored == "RESTORING") restored else if (e is EffectUnsupported) "UNSUPPORTED" else "ERROR"
    }
    private fun remember(group: String, state: String) {
        if (state == "APPLIED") groupDetails.remove(group)
        else if (owner.detail.startsWith(group)) {
            if (groupDetails[group] != owner.detail) Log.w("PleosEffects", owner.detail)
            groupDetails[group] = owner.detail
        }
    }
    private fun publish() {
        val detail = listOf(if (heartbeat.expired) "FEEDBACK COMMAND EXPIRED; inactive/restart required" else "",
            lastDetail, groupDetails["lights"] ?: "", groupDetails["climate"] ?: "",
            owner.detail, device.areaDescription).filter { it.isNotEmpty() }.distinct().joinToString("; ")
        val next = response(lightState, climateState, detail)
        if (next != status) Log.i("PleosEffects", next)
        status = next
    }
    /** In-flight SDK calls finish/timeout before restore; late completions never schedule new effects. */
    fun revoke() { heartbeat.revoke(); desired.updateAndGet { it.copy(active = false) } }

    fun close(done: () -> Unit) {
        closed = true; desired.set(Command())
        worker.execute {
            try {
                lightState = owner.restore("lights"); climateState = owner.restore("climate")
                publish()
            } finally { worker.shutdown(); done() }
        }
    }
    companion object {
        private fun response(lights: String, climate: String, detail: String) = JSONObject().apply {
            put("schema", 1); put("lights", lights); put("climate", climate); put("detail", detail)
        }.toString()
    }
}

/** Private app storage survives orderly shutdown and process death; no synchronous crash guarantee. */
private class PreferencesEffectJournal(context: Context) : EffectJournal {
    private val preferences = context.getSharedPreferences("pleos_effect_restore_v1", Context.MODE_PRIVATE)
    override fun load(): List<OwnedEffect> {
        val data = JSONArray(preferences.getString("owned", "[]"))
        return (0 until data.length()).map { i -> data.getJSONObject(i).let {
            OwnedEffect(it.getString("group"), it.getString("id"), it.getDouble("original"), it.getDouble("expected"),
                if (it.has("previous")) it.getDouble("previous") else null)
        } }
    }
    override fun save(values: List<OwnedEffect>) {
        val data = JSONArray()
        values.forEach { value -> data.put(JSONObject().apply {
            put("group", value.group); put("id", value.id)
            put("original", value.original); put("expected", value.expected)
            value.previous?.let { put("previous", it) }
        }) }
        check(preferences.edit().putString("owned", data.toString()).commit()) { "Recovery journal not durable" }
    }
}

/** SDK2.3.2 exact public signatures. Awaiting is confined to PleosEffects' serial output worker. */
private class SdkEffectsDevice(vehicle: Vehicle) : EffectDevice {
    private val light = vehicle.getLight()
    private val hvac = vehicle.getHVAC()
    private var lightsPrepared = false
    private var climatePrepared = false
    private val propertyAreas = mutableMapOf<String, List<SeatArea>>()
    var lightBounds: AreaBounds<Int>? = null; private set
    var fanBounds: AreaBounds<Int>? = null; private set
    var temperatureBounds: AreaBounds<Float>? = null; private set
    var temperatureStep = Double.NaN; private set
    var acOn = false; private set
    private var temperatureDetail = ""
    private var poisoned = false
    private var colorProfileLogged = false
    private val readers = mutableMapOf<String, () -> Double>()
    private val writers = mutableMapOf<String, (Double) -> Unit>()
    val areaDescription: String get() = propertyAreas.entries.joinToString("; ") { (key, areas) ->
        "$key=${areas.joinToString("|") { describe(it) }}" } +
        (if (climatePrepared && !acOn) "; AC OFF: fan only, cooling skipped" else temperatureDetail)
    private fun describe(area: SeatArea) = if (area.values.isEmpty()) "GLOBAL(0)" else
        area.values.joinToString("+") { "${it.name}(${it.id})" }
    private fun id(key: String, area: SeatArea) = "$key:${area.values.joinToString(",") { it.name }}"
    fun optionalPropertyId(key: String): String? = propertyAreas[key]?.singleOrNull()?.let { id(key, it) }
    fun propertyId(key: String): String = optionalPropertyId(key) ?: throw EffectUnsupported("No selected $key zone")
    private fun writable(config: CapabilityConfig) = config.accessType == AccessType.READ_WRITE
    private fun readable(config: CapabilityConfig) = config.accessType == AccessType.READ || writable(config)
    private fun preferred(areas: Collection<SeatArea>): SeatArea {
        val actual = areas.toList()
        val index = EffectAreas.preferred(actual.map { it.values.map { seat -> seat.id }.toSet() }, SeatValue.ROW_1_LEFT.id)
            ?: throw EffectUnsupported("No advertised driver, linked-driver or global zone")
        return actual[index]
    }
    private fun guards(areas: List<SeatArea>, affected: SeatArea): List<SeatArea> {
        return guards(areas, affected.values.map { it.id }.toSet())
    }
    private fun guards(areas: List<SeatArea>, affected: Set<Int>): List<SeatArea> {
        val indices = EffectAreas.guards(areas.map { it.values.map { seat -> seat.id }.toSet() }, affected)
        if (indices.isEmpty()) throw EffectUnsupported("Advertised prerequisite zones do not fully cover affected fan/temperature zone")
        return indices.map { areas[it] }
    }
    private fun installClimateGuards(power: List<SeatArea>, auto: List<SeatArea>, ac: List<SeatArea>) {
        propertyAreas["power"] = power; propertyAreas["auto"] = auto; propertyAreas["ac"] = ac
        power.forEach { a -> readers[id("power", a)] = {
            if (value<Boolean> { ok, fail -> hvac.getHvacPowerState(a, ok, fail) }) 1.0 else 0.0 } }
        auto.forEach { a -> readers[id("auto", a)] = {
            if (value<Boolean> { ok, fail -> hvac.getHvacAutoState(a, ok, fail) }) 1.0 else 0.0 } }
        ac.forEach { a -> readers[id("ac", a)] = {
            if (value<Boolean> { ok, fail -> hvac.getHvacAcState(a, ok, fail) }) 1.0 else 0.0 } }
    }
    private fun logCapability(key: String, config: CapabilityConfig, areas: Collection<SeatArea>) {
        Log.i("PleosEffects", "CAP $key ${config.accessType}: ${areas.joinToString("|") { describe(it) }}")
    }

    private class Reply<T>(val result: Result<T>)
    private fun <T> await(start: ((T) -> Unit, (Exception) -> Unit) -> Unit): T {
        check(!poisoned) { "SDK output timeout; pending recovery requires next session" }
        val latch = CountDownLatch(1)
        val result = AtomicReference<Reply<T>?>(null)
        fun finish(value: Result<T>) { if (result.compareAndSet(null, Reply(value))) latch.countDown() }
        start({ finish(Result.success(it)) }, { finish(Result.failure(it)) })
        if (!latch.await(5, TimeUnit.SECONDS)) {
            poisoned = true
            throw TimeoutException("SDK output deadline; journal retained")
        }
        return result.get()!!.result.getOrThrow()
    }
    private fun <T : Any> value(start: ((T?) -> Unit, (Exception) -> Unit) -> Unit): T =
        await<T?>(start) ?: throw IllegalStateException("SDK original value unavailable")

    fun prepare(group: String) {
        check(!poisoned) { "SDK output timed out" }
        if (group == "lights" && !lightsPrepared) {
            val colors = await<Pair<CapabilityConfig, List<SeatArea>>> { ok, fail ->
                light.checkAmbientLightBrightnessColorCapabilities({ config, areas -> ok(config to areas) }, fail) }
            val levels = await<Pair<CapabilityConfig, Map<SeatArea, AreaBounds<Int>>>> { ok, fail ->
                light.checkAmbientLightBrightnessCapabilities({ config, areas -> ok(config to areas) }, fail) }
            logCapability("color", colors.first, colors.second); logCapability("brightness", levels.first, levels.second.keys)
            if (!writable(colors.first) || !writable(levels.first)) throw EffectUnsupported("Ambient read/write unavailable")
            val colorArea = preferred(colors.second)
            val brightnessArea = preferred(levels.second.keys)
            val bounds = levels.second.getValue(brightnessArea)
            if (bounds.minValue < 0 || bounds.maxValue <= 0 || bounds.minValue > bounds.maxValue)
                throw EffectUnsupported("Invalid ambient brightness bounds")
            // Auto-by-drive-mode could overwrite effects. Read it when supported; never change it.
            val auto = try {
                await<CapabilityConfig> { ok, fail -> light.checkAmbientColorAutoByDriveModeEnabledCapabilities(ok, fail) }
            } catch (e: Exception) {
                // The actual emulator explicitly reports this optional property as unsupported.
                // Permission, transport, timeout and subsequent supported-property read failures
                // still fail closed; no broad catch turns an unknown mode into an assumed OFF.
                if (!poisoned && EffectLimits.optionalCapabilityAbsent(e)) {
                    Log.i("PleosEffects", "CAP ambientAuto explicitly ERROR_NOT_SUPPORTED_PROPERTY; optional feature absent")
                    CapabilityConfig(AccessType.NONE)
                } else throw e
            }
            Log.i("PleosEffects", "CAP ambientAuto ${auto.accessType}")
            if (readable(auto)) {
                readers["lightAuto"] = { if (value<Boolean> { ok, fail -> light.getAmbientColorAutoByDriveModeEnabled(ok, fail) }) 1.0 else 0.0 }
            }
            readers[id("color", colorArea)] = {
                val raw = value<Int> { ok, fail -> light.getAmbientLightBrightnessColor(colorArea, ok, fail) }
                val opaque = EffectColor.emulatorOpaqueArgb(raw)
                if (!colorProfileLogged) {
                    colorProfileLogged = true
                    Log.i("PleosEffects", "EMULATOR_ONLY color RGB24/ARGB compatibility: raw=$raw normalized=$opaque; restores equivalent physical RGB")
                }
                opaque.toDouble()
            }
            writers[id("color", colorArea)] = { v -> await<Int> { ok, fail -> light.setAmbientLightBrightnessColor(colorArea, v.toInt(), { _, n -> ok(n) }, fail) }; Unit }
            readers[id("brightness", brightnessArea)] = { value<Int> { ok, fail -> light.getAmbientLightBrightness(brightnessArea, ok, fail) }.toDouble() }
            writers[id("brightness", brightnessArea)] = { v -> await<Int> { ok, fail -> light.setAmbientLightBrightness(brightnessArea, v.toInt(), { _, n -> ok(n) }, fail) }; Unit }
            propertyAreas["color"] = listOf(colorArea); propertyAreas["brightness"] = listOf(brightnessArea)
            lightBounds = bounds; lightsPrepared = true
        }
        if (group == "climate" && !climatePrepared) {
            val fans = await<Pair<CapabilityConfig, Map<SeatArea, AreaBounds<Int>>>> { ok, fail ->
                hvac.checkHvacFanSpeedCapabilities({ config, areas -> ok(config to areas) }, fail) }
            val power = await<Pair<CapabilityConfig, List<SeatArea>>> { ok, fail ->
                hvac.checkHvacPowerCapabilities({ config, areas -> ok(config to areas) }, fail) }
            val auto = await<Pair<CapabilityConfig, List<SeatArea>>> { ok, fail ->
                hvac.checkHvacAutoCapabilities({ config, areas -> ok(config to areas) }, fail) }
            val ac = await<Pair<CapabilityConfig, List<SeatArea>>> { ok, fail ->
                hvac.checkHvacAcCapabilities({ config, areas -> ok(config to areas) }, fail) }
            if (!writable(fans.first) || !readable(power.first) || !readable(auto.first) || !readable(ac.first))
                throw EffectUnsupported("HVAC fan and prerequisite read capability unavailable")
            logCapability("fan", fans.first, fans.second.keys); logCapability("power", power.first, power.second)
            logCapability("auto", auto.first, auto.second); logCapability("ac", ac.first, ac.second)
            val area = preferred(fans.second.keys)
            val bounds = fans.second.getValue(area)
            if (maxOf(1, bounds.minValue) > minOf(2, bounds.maxValue)) throw EffectUnsupported("Gentle fan levels unavailable")
            // Each property has its own SDK-advertised zone model. A linked fan can depend on
            // global power, row-level AUTO and seat-level AC; never synthesize/intersect those IDs.
            installClimateGuards(guards(power.second, area), guards(auto.second, area), guards(ac.second, area))
            propertyAreas["fan"] = listOf(area)
            readers[id("fan", area)] = { value<Int> { ok, fail -> hvac.getHvacFanSpeed(area, ok, fail) }.toDouble() }
            writers[id("fan", area)] = { v -> await<Int?> { ok, fail -> hvac.setHvacFanSpeed(area, v.toInt(), { _, n -> ok(n) }, fail) }; Unit }
            // Temperature is optional. Unknown steps or inaccessible temperature never block fan output.
            temperatureDetail = "; temperature bounds/steps unavailable: fan only"
            try {
                val temp = await<Triple<CapabilityConfig, Map<SeatArea, AreaBounds<Float>>, ai.pleos.playground.vehicle.api.model.capability.HvacTemperatureConfigArray?>> { ok, fail ->
                    hvac.checkHvacTemperatureCapabilities({ config, areas, steps -> ok(Triple(config, areas, steps)) }, fail) }
                logCapability("temperature", temp.first, temp.second.keys)
                if (readable(temp.first) && temp.second.isNotEmpty()) {
                    val temperatureArea = preferred(temp.second.keys)
                    propertyAreas["temperature"] = listOf(temperatureArea)
                    readers[id("temperature", temperatureArea)] = { value<Float> { ok, fail -> hvac.getHvacTemperature(temperatureArea, TemperatureUnit.CELSIUS, ok, fail) }.toDouble() }
                    val step = temp.third?.incrementCelsius?.toDouble() ?: Double.NaN
                    if (writable(temp.first) && step.isFinite() && step > 0) {
                        // Cooling may affect more seats than the fan. Extend all prerequisites
                        // to the union before enabling its writer; keep verified fan-only on failure.
                        val affected = if (area.values.isEmpty() || temperatureArea.values.isEmpty()) emptySet()
                            else (area.values.map { it.id } + temperatureArea.values.map { it.id }).toSet()
                        val p = guards(power.second, affected)
                        val a = guards(auto.second, affected)
                        val c = guards(ac.second, affected)
                        installClimateGuards(p, a, c)
                        writers[id("temperature", temperatureArea)] = { v -> await<Float?> { ok, fail -> hvac.setHvacTemperature(temperatureArea, v.toFloat(), TemperatureUnit.CELSIUS, { _, n -> ok(n) }, fail) }; Unit }
                        temperatureBounds = temp.second.getValue(temperatureArea); temperatureStep = step
                        temperatureDetail = ""
                    }
                }
            } catch (e: Exception) { if (poisoned) throw e else Log.w("PleosEffects", "Temperature capability unavailable; fan only", e) }
            climatePrepared = true; fanBounds = bounds
        }
    }
    override fun capture(group: String): List<EffectSlot> {
        prepare(group)
        val keys = if (group == "lights") listOf("color", "brightness") else listOf("power", "ac", "auto", "fan", "temperature")
        val slots = keys.flatMap { key -> (propertyAreas[key] ?: emptyList()).map { area ->
            val id = id(key, area); EffectSlot(id, read(id), id in writers) } }.toMutableList()
        if (group == "lights" && "lightAuto" in readers) {
            val auto = read("lightAuto")
            if (auto != 0.0) throw EffectUnsupported("Ambient drive-mode auto must already be OFF")
            slots.add(EffectSlot("lightAuto", auto, false))
        }
        if (group == "climate") {
            if (slots.any { it.id.startsWith("power:") && it.original != 1.0 } ||
                slots.any { it.id.startsWith("auto:") && it.original != 0.0 })
                throw EffectUnsupported("HVAC power must already be ON and AUTO OFF; no implicit mode changes")
            acOn = slots.filter { it.id.startsWith("ac:") }.all { it.original == 1.0 }
        }
        return slots
    }
    override fun read(id: String): Double = readers[id]?.invoke()?.also { check(it.isFinite()) }
        ?: throw EffectUnsupported("Missing readable area $id")
    override fun write(id: String, value: Double) {
        check(!poisoned) { "SDK output timed out" }
        (writers[id] ?: throw EffectUnsupported("Missing writable area $id"))(value)
    }
}
