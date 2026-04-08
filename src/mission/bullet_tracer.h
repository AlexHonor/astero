#pragma once
#include "raylib.h"
#include <vector>
#include <deque>

enum class TraceEventType { Ricochet, Damage, Blocked, Expired };

struct TraceEvent {
    TraceEventType type;
    Vector2        pos;
    int            damage = 0;
};

struct BulletTrace {
    std::vector<Vector2>    path;
    std::vector<TraceEvent> events;
};

class BulletTracer {
public:
    static constexpr int MAX_TRACES = 10;
    bool enabled = false;

    void FinishTrace(BulletTrace trace);
    void Draw() const;  // call inside BeginMode2D

private:
    std::deque<BulletTrace> traces;
};
