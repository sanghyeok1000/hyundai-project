package com.beyondphysics.pleos
import org.junit.Assert.*
import org.junit.Test
class InterruptionTrackerTest {
    @Test fun refreshCannotHideAPreviouslyExpiredChannel() {
        val tracker = InterruptionTracker()
        tracker.observe(true)
        tracker.observeUpdate(false, true)
        assertEquals(1L, tracker.epoch)
    }
    @Test fun transientLossSurvivesAReadySnapshot() {
        val tracker = InterruptionTracker()
        tracker.observe(true); tracker.observe(false); tracker.observe(true)
        assertEquals(1L, tracker.epoch)
        tracker.observe(true)
        assertEquals(1L, tracker.epoch)
    }
    @Test fun initialUnknownDoesNotPreventFirstAutomaticStart() {
        val tracker = InterruptionTracker()
        tracker.observe(false); tracker.observe(false); tracker.observe(true)
        assertEquals(0L, tracker.epoch)
    }
}
