package com.beyondphysics.pleos

/** Explicit emulator calibration, not a road-wheel angle or production steering ratio. */
object InputGate {
    const val MAX_AGE_MS = 1800L
    fun fresh(now: Long, time: Long) = time > 0 && now >= time && now-time <= MAX_AGE_MS
    fun allowed(parked: Boolean?, requiresDo: Boolean?, restrictions: Int?,
                angle: Float?, brake: Boolean?, now: Long, safetyTime: Long,
                angleTime: Long, brakeTime: Long, foreground: Boolean) =
        foreground && parked == true && requiresDo == false && restrictions == 0 &&
        angle != null && angle.isFinite() && kotlin.math.abs(angle) <= 540f && brake != null &&
        fresh(now,safetyTime) && fresh(now,angleTime) && fresh(now,brakeTime)
    fun emulatorSteer(raw: Float): Float = (raw/540f).coerceIn(-1f,1f)
}
