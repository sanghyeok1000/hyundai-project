package com.beyondphysics.pleos
import org.junit.Assert.*
import org.junit.Test
class InputGateTest {
    @Test fun rejectsValuesOutsideTheCalibratedEmulatorRange() {
        assertFalse(InputGate.allowed(true,false,0,541f,false,5000,4900,4900,4900,true))
        assertFalse(InputGate.allowed(true,false,0,-541f,false,5000,4900,4900,4900,true))
        assertFalse(InputGate.allowed(true,false,0,Float.POSITIVE_INFINITY,false,5000,4900,4900,4900,true))
    }
    @Test fun eachRequiredChannelMustBeFreshAndClockCannotGoBackwards() {
        assertFalse(InputGate.allowed(true,false,0,0f,false,5000,1000,4900,4900,true))
        assertFalse(InputGate.allowed(true,false,0,0f,false,5000,4900,4900,1000,true))
        assertFalse(InputGate.allowed(true,false,0,0f,false,5000,5001,4900,4900,true))
        assertFalse(InputGate.allowed(true,false,1,0f,false,5000,4900,4900,4900,true))
    }
    @Test fun completeFreshParkedSignalsAllowInput() {
        assertTrue(InputGate.allowed(true,false,0,0f,false,5000,4900,4900,4900,true))
    }
    @Test fun staleUnknownDrivingAndBackgroundBlock() {
        assertFalse(InputGate.allowed(true,false,0,0f,false,5000,4900,1000,4900,true))
        assertFalse(InputGate.allowed(null,false,0,0f,false,5000,4900,4900,4900,true))
        assertFalse(InputGate.allowed(false,false,0,0f,false,5000,4900,4900,4900,true))
        assertFalse(InputGate.allowed(true,true,0,0f,false,5000,4900,4900,4900,true))
        assertFalse(InputGate.allowed(true,false,0,0f,false,5000,4900,4900,4900,false))
        assertFalse(InputGate.allowed(true,false,0,Float.NaN,false,5000,4900,4900,4900,true))
    }
    @Test fun calibrationIsExplicitBoundedEmulatorProfile() {
        assertEquals(1f,InputGate.emulatorSteer(540f),0f)
        assertEquals(-1f,InputGate.emulatorSteer(-1080f),0f)
    }
}
