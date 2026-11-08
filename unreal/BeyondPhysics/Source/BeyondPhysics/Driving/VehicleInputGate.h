#pragma once
#include <cstdint>

namespace BPPhysics
{
// A recovered SDK/park signal never resumes an interrupted experiment by itself.
struct VehicleInputGate
{
    bool EverAllowed=false;
    bool NeedsRestart=false;
    std::uint64_t SeenLossEpoch=0;
    bool Update(bool Allowed, std::uint64_t LossEpoch=0)
    {
        if(LossEpoch!=SeenLossEpoch) { NeedsRestart=true; SeenLossEpoch=LossEpoch; }
        if(!Allowed) { if(EverAllowed) NeedsRestart=true; return false; }
        EverAllowed=true;
        return !NeedsRestart;
    }
    void Restart(bool Allowed) { if(Allowed) NeedsRestart=false; }
};
}
