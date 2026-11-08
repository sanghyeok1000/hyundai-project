package com.beyondphysics.pleos

/** Persistent across SDK sessions: a brief loss cannot vanish between C++ ticks. */
class InterruptionTracker {
    @Volatile var epoch: Long = 0
        private set
    private var ready = false
    @Synchronized fun observeUpdate(previousReady: Boolean, nextReady: Boolean): Long {
        observe(previousReady)
        return observe(nextReady)
    }
    @Synchronized fun observe(nextReady: Boolean): Long {
        if (ready && !nextReady) epoch++
        ready = nextReady
        return epoch
    }
}
