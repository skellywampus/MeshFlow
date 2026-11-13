#pragma once
#include "meshtastic/mesh.pb.h"

struct M9NodeSnapshot {
    bool favorite = false;
    bool hasPosition = false;
    int32_t latitude = 0, longitude = 0;
    uint32_t positionTime = 0;
    bool hasBattery = false;
    uint32_t battery = 0;
    bool hasVoltage = false;
    float voltage = 0;
};
extern "C" bool meshflow_m9_current_position(meshtastic_Position *position);
extern "C" bool meshflow_m9_node_snapshot(uint32_t node, M9NodeSnapshot *snapshot);
extern "C" bool meshflow_m9_is_favorite(uint32_t node);
extern "C" bool meshflow_m9_set_favorite(uint32_t node, bool favorite);
