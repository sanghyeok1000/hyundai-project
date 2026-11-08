package com.beyondphysics.pleos

import kotlin.math.abs
import kotlin.math.ceil

internal data class EffectSlot(val id: String, val original: Double, val writable: Boolean)
internal data class OwnedEffect(val group: String, val id: String, val original: Double,
    val expected: Double, val previous: Double? = null)
internal interface EffectJournal {
    fun load(): List<OwnedEffect>
    /** Must be durable before returning. A failure aborts the write. */
    fun save(values: List<OwnedEffect>)
}
internal interface EffectDevice {
    /** All required original values and read/write capability must be known before this returns. */
    fun capture(group: String): List<EffectSlot>
    fun read(id: String): Double
    fun write(id: String, value: Double)
}
internal class EffectUnsupported(message: String) : Exception(message)

/** Serialized by its owner. Records write intent before Binder calls, including ambiguous failures. */
internal class EffectOwnership(private val device: EffectDevice, private val journal: EffectJournal) {
    private val owned = journal.load().toMutableList()
    private val originals = mutableMapOf<String, List<EffectSlot>>()
    private val yielded = mutableSetOf<String>()
    var detail = ""
        private set

    fun apply(group: String, targets: Map<String, Double>, allowed: () -> Boolean): String {
        if (group in yielded) return restore(group)
        try {
            if (!allowed()) return restore(group)
            val slots = originals.getOrPut(group) { device.capture(group) }
            // Check the complete captured group, including read-only prerequisites, for manual changes.
            for (slot in slots) {
                val expected = owned.find { it.id == slot.id }?.expected ?: slot.original
                val observed = device.read(slot.id)
                if (!same(observed, expected)) {
                    yielded.add(group); detail = "$group: USER OVERRIDE ${slot.id} expected=$expected actual=$observed; toggle OFF/ON to resume"
                    return restore(group)
                }
            }
            for ((id, target) in targets) {
                val slot = slots.find { it.id == id && it.writable }
                    ?: throw EffectUnsupported("$id has no verified writable capability")
                require(target.isFinite())
                if (!allowed()) return restore(group)
                val prior = owned.find { it.id == id }
                val current = device.read(id)
                if (!same(current, prior?.expected ?: slot.original)) {
                    yielded.add(group); detail = "$group: USER OVERRIDE $id expected=${prior?.expected ?: slot.original} actual=$current"
                    return restore(group)
                }
                if (same(current, target)) continue
                val entry = OwnedEffect(group, id, prior?.original ?: slot.original, target, current)
                owned.removeAll { it.id == id }; owned.add(entry)
                journal.save(owned) // Never write unless recovery intent is durable.
                if (!allowed()) return restore(group)
                device.write(id, target)
                val actual = device.read(id)
                check(same(actual, target)) { "$id readback mismatch requested=$target actual=$actual original=${slot.original}" }
                owned.remove(entry); owned.add(entry.copy(previous = null)); journal.save(owned)
            }
            return "APPLIED"
        } catch (e: Exception) {
            detail = "$group: ${e.message ?: e.javaClass.simpleName}"
            yielded.add(group)
            val restored = restore(group)
            return if (restored == "RESTORING") restored else if (e is EffectUnsupported) "UNSUPPORTED" else "ERROR"
        }
    }

    fun restore(group: String, reset: Boolean = false): String {
        if (reset) {
            yielded.remove(group); originals.remove(group)
            if (detail.startsWith("$group:")) detail = ""
        }
        var failed = false
        for (entry in owned.filter { it.group == group }.asReversed()) {
            try {
                // Discovery rechecks supported writable area on next-session recovery.
                val supported = device.capture(group).any { it.id == entry.id && it.writable }
                if (!supported) throw EffectUnsupported("restore capability unavailable: ${entry.id}")
                val current = device.read(entry.id)
                if (same(current, entry.expected) || entry.previous?.let { same(current, it) } == true) {
                    device.write(entry.id, entry.original)
                    val restored = device.read(entry.id)
                    check(same(restored, entry.original)) { "restore readback mismatch ${entry.id} requested=${entry.original} actual=$restored" }
                }
                // A different current value belongs to the user, including after process death.
                owned.remove(entry); journal.save(owned)
            } catch (e: Exception) {
                failed = true; detail = "$group RESTORE PENDING: ${e.message}"
            }
        }
        // Recapture at the next activation, while keeping manual-override suppression until OFF.
        if (!failed && owned.none { it.group == group }) originals.remove(group)
        return if (failed) "RESTORING" else "OFF"
    }
    fun original(id: String): Double? = owned.find { it.id == id }?.original
        ?: originals.values.flatten().find { it.id == id }?.original
    private fun same(a: Double, b: Double) = a.isFinite() && b.isFinite() && abs(a - b) < 0.001
}

internal object EffectLimits {
    // SDK2.3.2 sources expose VehicleException.errorCode, but the distributed AAR obfuscates
    // that class to callback.exception.a. Its public message is the exact ErrorCode name.
    // Use only at an OPTIONAL capability query boundary, never for getters or setters.
    fun optionalCapabilityAbsent(error: Exception): Boolean = error !is SecurityException &&
        error !is java.util.concurrent.TimeoutException && error.message == "ERROR_NOT_SUPPORTED_PROPERTY"

    fun cooling(original: Double, offset: Double, min: Double, max: Double, step: Double): Double? {
        if (!listOf(original, offset, min, max, step).all { it.isFinite() } ||
            original !in 20.0..26.0 || step <= 0 || min > max || original !in min..max) return null
        val desired = (original - offset.coerceIn(0.0, 1.0)).coerceAtLeast(20.0).coerceAtLeast(min)
        // Round upward so a supported step never cools farther than requested.
        val result = min + ceil((desired - min) / step - 1e-6) * step
        return result.takeIf { it in 20.0..26.0 && it <= original + 0.001 && it <= max }
    }
}

/** Independent of live AAOS input freshness: Unreal itself must keep sending valid commands. */
internal class EffectHeartbeat {
    private var receivedAt = -1L
    private var suspended = false
    @Volatile var expired = false
        private set
    @Synchronized fun receive(active: Boolean, now: Long): Boolean {
        receivedAt = now
        if (!active) { suspended = false; expired = false }
        return active && !suspended
    }
    @Synchronized fun valid(now: Long): Boolean {
        if (receivedAt < 0 || now < receivedAt || now - receivedAt > 1500) {
            suspended = true; expired = true
        }
        return !suspended
    }
    @Synchronized fun revoke() { suspended = true }
}

/** Select only advertised zones. An empty seat set is global only when advertised by the SDK. */
internal object EffectAreas {
    fun preferred(areas: List<Set<Int>>, driver: Int): Int? = areas.indices
        .filter { areas[it].isEmpty() || driver in areas[it] }
        .minByOrNull { if (areas[it].isEmpty()) 1000 else areas[it].size }
    fun guards(areas: List<Set<Int>>, affected: Set<Int>): List<Int> {
        val global = areas.indexOfFirst { it.isEmpty() }
        if (global >= 0) return listOf(global)
        if (affected.isEmpty()) return emptyList() // global effect requires a verified global prerequisite
        val matching = areas.indices.filter { areas[it].any { seat -> seat in affected } }
        return matching.takeIf { indices -> indices.flatMap { areas[it] }.toSet().containsAll(affected) } ?: emptyList()
    }
}

internal object EffectColor {
    /** Observed Pleos Connect emulator SDK2.3.2 getter uses RGB24 despite its ARGB API. */
    fun emulatorOpaqueArgb(value: Int): Int = value or 0xff000000.toInt()
}
