#include "bullet_tracer.h"
#include <cstdio>

static constexpr Color NEON = {57, 255, 20, 255};

void BulletTracer::FinishTrace(BulletTrace trace) {
    if (trace.path.empty()) return;
    traces.push_back(std::move(trace));
    while ((int)traces.size() > MAX_TRACES)
        traces.pop_front();
}

void BulletTracer::Draw() const {
    if (!enabled || traces.empty()) return;

    int total = (int)traces.size();
    for (int ti = 0; ti < total; ti++) {
        const BulletTrace& bt = traces[ti];

        // Newer traces are brighter
        float t = (total == 1) ? 1.f : (float)ti / (float)(total - 1);
        unsigned char line_a = (unsigned char)(60  + t * 140);
        unsigned char evt_a  = (unsigned char)(100 + t * 155);
        Color line_col = {NEON.r, NEON.g, NEON.b, line_a};
        Color evt_col  = {NEON.r, NEON.g, NEON.b, evt_a};

        // Path
        for (int i = 1; i < (int)bt.path.size(); i++)
            DrawLineV(bt.path[i-1], bt.path[i], line_col);

        // Start dot
        if (!bt.path.empty())
            DrawCircleV(bt.path[0], 3.f, {NEON.r, NEON.g, NEON.b, (unsigned char)(evt_a / 2)});

        // Events
        for (const auto& e : bt.events) {
            switch (e.type) {
                case TraceEventType::Ricochet:
                    DrawCircleV(e.pos, 5.f, evt_col);
                    DrawCircleLinesV(e.pos, 7.f, evt_col);
                    DrawText("RIC", (int)e.pos.x + 8, (int)e.pos.y - 8, 14, evt_col);
                    break;
                case TraceEventType::Damage:
                    DrawCircleV(e.pos, 5.f, evt_col);
                    DrawCircleLinesV(e.pos, 9.f, evt_col);
                    DrawText(TextFormat("-%d", e.damage),
                             (int)e.pos.x + 8, (int)e.pos.y - 8, 14, evt_col);
                    break;
                case TraceEventType::Blocked:
                    DrawCircleV(e.pos, 4.f, evt_col);
                    DrawText("BLKD", (int)e.pos.x + 8, (int)e.pos.y - 8, 14, evt_col);
                    break;
                case TraceEventType::Expired:
                    DrawCircleV(e.pos, 3.f, evt_col);
                    DrawText("EXP", (int)e.pos.x + 8, (int)e.pos.y - 8, 14, evt_col);
                    break;
            }
        }
    }
}
