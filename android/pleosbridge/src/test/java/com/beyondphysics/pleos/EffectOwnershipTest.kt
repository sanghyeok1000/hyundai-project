package com.beyondphysics.pleos

import org.junit.Assert.*
import org.junit.Test

class EffectOwnershipTest {
    private class Memory : EffectJournal {
        var entries = listOf<OwnedEffect>()
        override fun load() = entries
        override fun save(values: List<OwnedEffect>) { entries = values.toList() }
    }
    private class Device : EffectDevice {
        val values = mutableMapOf("color" to 12.0, "brightness" to 80.0)
        var failCapture = false
        var writes = 0
        var failWrites = false
        var ignoreWrites = false
        override fun capture(group: String): List<EffectSlot> {
            if (failCapture) error("missing original")
            return values.map { EffectSlot(it.key, it.value, true) }
        }
        override fun read(id: String) = values.getValue(id)
        override fun write(id: String, value: Double) {
            if (failWrites) error("device disconnected")
            writes++; if (!ignoreWrites) values[id] = value
        }
    }
    @Test fun missingOriginalPreventsEveryWrite() {
        val d = Device(); d.failCapture = true
        val o = EffectOwnership(d, Memory())
        assertEquals("ERROR", o.apply("lights", mapOf("color" to 20.0)) { true })
        assertEquals(0, d.writes)
    }
    @Test fun gateLossBetweenCaptureAndWritePreventsOutput() {
        val d = Device(); val o = EffectOwnership(d, Memory()); var checks = 0
        o.apply("lights", mapOf("color" to 20.0)) { ++checks < 2 }
        assertEquals(12.0, d.values["color"]!!, 0.0)
        assertEquals(0, d.writes)
    }
    @Test fun offRestoresOriginalAfterMultipleUpdatesAndDeduplicates() {
        val d = Device(); val j = Memory(); val o = EffectOwnership(d, j)
        assertEquals("APPLIED", o.apply("lights", mapOf("color" to 20.0)) { true })
        o.apply("lights", mapOf("color" to 20.0)) { true }
        assertEquals(1, d.writes)
        o.apply("lights", mapOf("color" to 30.0)) { true }
        assertEquals("OFF", o.restore("lights"))
        assertEquals(12.0, d.values["color"]!!, 0.0)
        assertTrue(j.entries.isEmpty())
    }
    @Test fun manualChangeYieldsGroupAndNeverOverwritesUser() {
        val d = Device(); val j = Memory(); val o = EffectOwnership(d, j)
        o.apply("lights", mapOf("color" to 20.0, "brightness" to 10.0)) { true }
        d.values["color"] = 99.0
        assertEquals("OFF", o.apply("lights", mapOf("color" to 30.0)) { true })
        assertEquals(99.0, d.values["color"]!!, 0.0)
        assertEquals(80.0, d.values["brightness"]!!, 0.0)
        o.apply("lights", mapOf("color" to 30.0)) { true }
        assertEquals(99.0, d.values["color"]!!, 0.0)
        assertTrue(o.detail.contains("USER OVERRIDE"))
        o.restore("lights", reset = true)
        assertEquals("", o.detail)
    }
    @Test fun nextSessionRecoversPersistedOriginalOnlyIfStillOwned() {
        val d = Device(); val j = Memory()
        EffectOwnership(d, j).apply("lights", mapOf("color" to 20.0)) { true }
        assertEquals("OFF", EffectOwnership(d, j).restore("lights"))
        assertEquals(12.0, d.values["color"]!!, 0.0)
    }
    @Test fun gateLossAfterUpdatingWriteIntentRestoresPreviousOwnedValue() {
        val d = Device(); val j = Memory(); val o = EffectOwnership(d, j)
        o.apply("lights", mapOf("color" to 20.0)) { true }
        var checks = 0
        o.apply("lights", mapOf("color" to 30.0)) { ++checks < 3 }
        assertEquals(12.0, d.values["color"]!!, 0.0)
        assertTrue(j.entries.isEmpty())
    }
    @Test fun failedRestoreRetainsRecoveryAndCanRetryNextSession() {
        val d = Device(); val j = Memory(); val o = EffectOwnership(d, j)
        o.apply("lights", mapOf("color" to 20.0)) { true }
        d.failWrites = true
        assertEquals("RESTORING", o.restore("lights"))
        assertEquals(1, j.entries.size)
        d.failWrites = false
        assertEquals("OFF", EffectOwnership(d, j).restore("lights"))
        assertEquals(12.0, d.values["color"]!!, 0.0)
    }
    @Test fun setterAcknowledgementWithoutReadbackNeverClaimsApplied() {
        val d = Device(); d.ignoreWrites = true
        assertEquals("ERROR", EffectOwnership(d, Memory()).apply("lights", mapOf("color" to 20.0)) { true })
        assertEquals(12.0, d.values["color"]!!, 0.0)
    }
    @Test fun yieldedGroupRetriesPendingRestoreBeforeReportingOff() {
        val d = Device(); val j = Memory(); val o = EffectOwnership(d, j)
        o.apply("lights", mapOf("color" to 20.0)) { true }
        d.failWrites = true
        assertEquals("RESTORING", o.apply("lights", mapOf("color" to 30.0)) { true })
        assertEquals("RESTORING", o.apply("lights", mapOf("color" to 30.0)) { true })
        d.failWrites = false
        assertEquals("OFF", o.apply("lights", mapOf("color" to 30.0)) { true })
        assertEquals(12.0, d.values["color"]!!, 0.0)
        assertTrue(j.entries.isEmpty())
    }
    @Test fun gentleCoolingUsesOnlySupportedStepsInsideComfortRange() {
        assertEquals(22.0, EffectLimits.cooling(23.0, 1.0, 16.0, 30.0, 0.5)!!, 0.0)
        assertNull(EffectLimits.cooling(19.0, 1.0, 16.0, 30.0, 0.5))
        assertNull(EffectLimits.cooling(23.0, 1.0, 16.0, 30.0, Double.NaN))
        assertEquals(20.0, EffectLimits.cooling(20.0, 1.0, 16.0, 30.0, 0.5)!!, 0.0)
        assertEquals(22.5, EffectLimits.cooling(23.0, 0.75, 16.0, 30.0, 0.5)!!, 0.0)
    }
    @Test fun expiredHeartbeatLatchesUntilInactiveCommandEvenWhenCommandsResume() {
        val heartbeat = EffectHeartbeat()
        assertTrue(heartbeat.receive(true, 100))
        assertTrue(heartbeat.valid(1600))
        assertFalse(heartbeat.valid(1601))
        assertTrue(heartbeat.expired)
        assertFalse(heartbeat.receive(true, 1700))
        assertFalse(heartbeat.valid(1700))
        assertFalse(heartbeat.receive(false, 1750))
        assertTrue(heartbeat.receive(true, 1800))
        assertTrue(heartbeat.valid(1800))
        assertFalse(heartbeat.valid(1799)) // a backwards clock must also fail closed
    }
    @Test fun areasAreChosenPerPropertyAndLinkedGuardsKeepTheirActualScopes() {
        val driver = 1
        assertEquals(0, EffectAreas.preferred(listOf(setOf(1, 4)), driver)) // linked fan
        assertEquals(1, EffectAreas.preferred(listOf(setOf(4), setOf(1)), driver)) // driver temperature
        assertEquals(0, EffectAreas.preferred(listOf(emptySet()), driver)) // reported global power
        assertNull(EffectAreas.preferred(listOf(setOf(4)), driver)) // never invent driver support
        assertEquals(listOf(0, 1), EffectAreas.guards(listOf(setOf(1), setOf(4), setOf(16)), setOf(1, 4)))
        assertEquals(listOf(0), EffectAreas.guards(listOf(emptySet(), setOf(1)), setOf(1, 4)))
        assertEquals(emptyList<Int>(), EffectAreas.guards(listOf(setOf(1)), setOf(1, 4))) // passenger prerequisite unknown
        assertEquals(emptyList<Int>(), EffectAreas.guards(listOf(setOf(1, 4)), emptySet())) // row cannot prove global coverage
        assertEquals(listOf(0), EffectAreas.guards(listOf(setOf(1, 4, 16)), setOf(1, 4))) // actual ALL covers ROW1
        assertEquals(emptyList<Int>(), EffectAreas.guards(listOf(setOf(1, 4)), setOf(1, 4, 16))) // broader cooling zone
    }
    @Test fun onlyExplicitUnsupportedOptionalCapabilityIsAbsent() {
        assertTrue(EffectLimits.optionalCapabilityAbsent(Exception("ERROR_NOT_SUPPORTED_PROPERTY")))
        assertFalse(EffectLimits.optionalCapabilityAbsent(Exception("ERROR_PERMISSION_DENIED")))
        assertFalse(EffectLimits.optionalCapabilityAbsent(Exception("ERROR_NOT_SUPPORTED_PROPERTY_EXTRA")))
        assertFalse(EffectLimits.optionalCapabilityAbsent(Exception(null as String?)))
        assertFalse(EffectLimits.optionalCapabilityAbsent(SecurityException("ERROR_NOT_SUPPORTED_PROPERTY")))
        assertFalse(EffectLimits.optionalCapabilityAbsent(java.util.concurrent.TimeoutException("ERROR_NOT_SUPPORTED_PROPERTY")))
    }
    @Test fun emulatorRgbReadbackMatchesOpaqueArgbButDifferentRgbRemainsDistinct() {
        assertEquals(0xff339cbf.toInt(), EffectColor.emulatorOpaqueArgb(0x00339cbf))
        assertEquals(0xff339cbf.toInt(), EffectColor.emulatorOpaqueArgb(0xff339cbf.toInt()))
        assertEquals(-1, EffectColor.emulatorOpaqueArgb(0x00ffffff))
        assertNotEquals(EffectColor.emulatorOpaqueArgb(0x00339cbf), EffectColor.emulatorOpaqueArgb(0x00339cbe))
    }
}
