#pragma once

#include "../../Systems/StateManager.h"

class Tween {
friend class Effects;
public:
    virtual ~Tween() = default;
    void Start() {
        startTime = StateManager::globalTime;
        started = true;
    }

    virtual void Update() = 0;

    bool Finished() const {
        return finished;
    }

    void SetNext(Tween* tween) {
        nextTween = tween;
    }

    void TakeOwnership() {
        ownedByEffectsSystem = false;
    }

    bool IsOwnedByEffectsSystem() const {
        return ownedByEffectsSystem;
    }

    virtual void Stop(const bool naturalStop = false) {
        finished = true;
        if (naturalStop) {
            if (nextTween) nextTween->Start();
        }
    }
protected:
    Tween(const Time a_duration) : started(false), finished(false), duration(a_duration), nextTween(nullptr), ownedByEffectsSystem(true) {}

    bool started;
    bool finished;

    Time startTime;
    Time duration;

    Tween* nextTween;

    bool ownedByEffectsSystem;
};
