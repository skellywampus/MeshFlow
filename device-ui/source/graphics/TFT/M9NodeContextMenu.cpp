#if defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "graphics/view/TFT/TFTView_320x240.h"
#include "graphics/view/TFT/M9FocusTheme.h"
#include "graphics/common/M9NodeActions.h"
#include "graphics/common/ViewController.h"
#include "graphics/map/MapPanel.h"
#include "input/policy/InputContextState.h"
#include "Arduino.h"
#include "pb_decode.h"
#include "ui.h"
#include "styles.h"
#include "fonts.h"
#include <cstdio>

namespace {
bool validCoordinates(int32_t lat, int32_t lon)
{
    return lat >= -900000000 && lat <= 900000000 && lon >= -1800000000 && lon <= 1800000000;
}
const char *operationNames[] = {"Position packet", "Coordinate message", "Position request", "NodeInfo request"};
}

void TFTView_320x240::closeNodeContext(bool restore)
{
    nodeContextStatusView = false;
    if (powerStatisticsView)
        input_policy::InputContextState::instance().setFocusSemantic(input_policy::FocusSemantic::Unknown);
    powerStatisticsView = false;
    // Move input away before deleting the modal group and its focused objects.
    setInputGroup(defaultPanelGroup);
    if (nodeContextBox) {
        lv_msgbox_close(nodeContextBox);
        nodeContextBox = nodeContextLabel = nullptr;
    }
    if (nodeContextGroup) {
        lv_group_delete(nodeContextGroup);
        nodeContextGroup = nullptr;
    }
    if (restore && nodeContextReturn && nodeRecord(nodeContextReturn)) {
        ui_set_active(objects.nodes_button, objects.nodes_panel, objects.top_nodes_panel);
        ensureVirtualNodeList();
        reconcileVirtualNodeListInputGroup(true);
        virtualNodeList->focus(nodeContextReturn);
        nodeContextReturn = 0;
    }
}

void TFTView_320x240::nodeContextPopup(const char *title, const std::string &text, bool menu)
{
    closeNodeContext(false);
    nodeContextGroup = lv_group_create();
    nodeContextIsMenu = menu;
    nodeContextBox = lv_msgbox_create(nullptr);
    add_style_settings_panel_style(nodeContextBox);
    lv_obj_set_size(nodeContextBox, 292, 232);
    lv_obj_set_style_pad_all(nodeContextBox, 6, 0);
    lv_obj_set_style_pad_row(nodeContextBox, 2, 0);
    lv_obj_set_style_shadow_width(nodeContextBox, 0, 0);
    lv_obj_set_style_text_font(nodeContextBox, &ui_font_montserrat_12, 0);
    lv_msgbox_add_title(nodeContextBox, title);
    auto *header = lv_msgbox_get_header(nodeContextBox);
    auto *content = lv_msgbox_get_content(nodeContextBox);
    for (auto *part : {header, content}) {
        add_style_settings_panel_style(part);
        lv_obj_set_style_bg_opa(part, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(part, 0, 0);
        lv_obj_set_style_pad_all(part, 2, 0);
        lv_obj_set_style_shadow_width(part, 0, 0);
    }
    lv_obj_set_height(header, 20);
    // Leave room for each focused outline, including at the scroll viewport edge.
    lv_obj_set_style_pad_all(content, 3, 0);
    lv_obj_set_style_pad_row(content, 6, 0);
    lv_obj_set_style_text_font(lv_msgbox_get_title(nodeContextBox), &ui_font_montserrat_12, 0);
    nodeContextLabel = lv_msgbox_add_text(nodeContextBox, text.c_str());
    lv_obj_set_style_text_font(nodeContextLabel, &ui_font_montserrat_12, 0);
    lv_obj_t *first = nullptr;
    if (menu) {
        const char *names[] = {"Send My Position", "Request Position", "Request Node Info", "Direct Message",
                               "Trace Route", "Show on Map",
                               nodeRecord(nodeContextTarget)->favorite ? "Unfavorite" : "Favorite", "Node Details"};
        lv_label_set_long_mode(nodeContextLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_height(nodeContextLabel, 14);
        for (unsigned i = 0; i < 8; ++i) {
            auto *button = lv_button_create(content);
            M9FocusTheme::attach(button);
            add_style_settings_button_style(button);
            lv_obj_add_style(button, get_style_node_panel_style_MAIN_FOCUS_KEY(), LV_STATE_FOCUSED);
            // Let translated/action labels wrap and grow rather than spill
            // outside the narrow M9 menu row. The content viewport scrolls if
            // a wrapped label makes the action list taller than the display.
            lv_obj_set_size(button, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_set_style_min_height(button, 22, LV_PART_MAIN);
            lv_obj_set_style_text_font(button, &ui_font_montserrat_12, 0);
            lv_obj_set_style_pad_all(button, 0, 0);
            lv_obj_set_style_pad_hor(button, 4, 0);
            lv_obj_set_style_radius(button, 3, 0);
            lv_obj_set_style_shadow_width(button, 0, 0);
            lv_obj_set_style_outline_opa(button, LV_OPA_COVER, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_width(button, 1, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_pad(button, 0, LV_STATE_FOCUSED);
            auto *label = lv_label_create(button);
            lv_label_set_text(label, names[i]);
            lv_obj_set_width(label, lv_pct(100));
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_center(label);
            lv_group_add_obj(nodeContextGroup, button);
            lv_obj_add_event_cb(button, nodeContextEvent, LV_EVENT_ALL, reinterpret_cast<void *>(uintptr_t(i)));
            if (!first) first = button;
        }
    }
    auto *close = lv_msgbox_add_footer_button(nodeContextBox, "Back");
    M9FocusTheme::attach(close);
    // The footer is created lazily with its first button.
    auto *footer = lv_msgbox_get_footer(nodeContextBox);
    add_style_settings_panel_style(footer);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_height(footer, 22);
    add_style_settings_button_style(close);
    lv_obj_add_style(close, get_style_node_panel_style_MAIN_FOCUS_KEY(), LV_STATE_FOCUSED);
    lv_obj_set_height(close, 20);
    lv_obj_set_style_pad_all(close, 0, 0);
    lv_obj_set_style_shadow_width(close, 0, 0);
    lv_obj_set_style_outline_opa(close, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(close, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(close, 0, LV_STATE_FOCUSED);
    lv_group_add_obj(nodeContextGroup, close);
    lv_obj_add_event_cb(close, nodeContextEvent, LV_EVENT_ALL, reinterpret_cast<void *>(uintptr_t(8)));
    setInputGroup(nodeContextGroup);
    input_policy::InputContextState::instance().setFocusSemantic(input_policy::FocusSemantic::Unknown);
    input_policy::InputContextState::instance().setEditMode(false);
    lv_group_focus_obj(first ? first : close);
}

void TFTView_320x240::openNodeContext(NodeId id)
{
    if (!id || id == UINT32_MAX || !nodeRecord(id) || nodeContextBox)
        return;
    nodeContextTarget = nodeContextReturn = id;
    if (auto *indev = lv_indev_get_act()) lv_indev_wait_release(indev);
    char title[80];
    snprintf(title, sizeof(title), "%s (!%08x)", nodeShortName(id), id);
    nodeContextPopup("Node actions", title, true);
    if (!nodeContextTimer) {
        nodeContextTimer = lv_timer_create([](lv_timer_t *) {
            auto *self = currentInstance();
            if (!self) return;
            for (auto &op : self->nodeContextOps) {
                if (op.pending && millis() - op.started >= 30000U) {
                    op.pending = false;
                    op.status = "Timed out; no confirmed response";
                    if (&op == &self->nodeContextOps[1]) {
                        self->controller->setTextMessageStatus(op.id, LogMessage::eNoResponse, true);
                        self->updateChatStatus(op.to, op.id, LogMessage::eNoResponse);
                        self->automaticTextRetries.erase(op.id);
                    }
                    self->requests.removeRequest(op.id);
                }
            }
            if (self->nodeContextStatusView) self->nodeContextRenderStatus();
        }, 500, nullptr);
    }
}

void TFTView_320x240::nodeContextEvent(lv_event_t *event)
{
    auto *self = currentInstance();
    if (!self || !self->nodeContextBox) return;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_KEY) {
        const auto key = lv_event_get_key(event);
        if ((key == LV_KEY_UP || key == LV_KEY_DOWN) && !self->nodeContextIsMenu)
            lv_obj_scroll_by(lv_msgbox_get_content(self->nodeContextBox), 0, key == LV_KEY_UP ? 40 : -40, LV_ANIM_OFF);
        else if (key == LV_KEY_UP) lv_group_focus_prev(self->nodeContextGroup);
        else if (key == LV_KEY_DOWN) lv_group_focus_next(self->nodeContextGroup);
        else if (key == LV_KEY_ESC || key == LV_KEY_BACKSPACE) self->closeNodeContext(true);
        else return;
        lv_event_stop_processing(event);
    } else if (code == LV_EVENT_CLICKED) {
        const unsigned action = uintptr_t(lv_event_get_user_data(event));
        // Disable before invoking: one deliberate Center press can dispatch only once.
        auto *button = lv_event_get_target_obj(event);
        if (lv_obj_has_state(button, LV_STATE_DISABLED)) return;
        lv_obj_add_state(button, LV_STATE_DISABLED);
        if (action == 8) self->closeNodeContext(true);
        else self->nodeContextAction(action);
    }
}

uint32_t TFTView_320x240::nodeContextBegin(unsigned operation, NodeId id)
{
    auto &op = nodeContextOps[operation];
    op.id = requests.addRequest(id, ResponseHandler::RemoteConfigRequest);
    op.to = id;
    op.started = millis();
    op.pending = true;
    op.status = "Queued; acknowledgement unconfirmed";
    return op.id;
}

void TFTView_320x240::nodeContextRenderStatus()
{
    if (!nodeContextLabel) return;
    std::string text;
    for (unsigned i = 0; i < 4; ++i) {
        const auto &op = nodeContextOps[i];
        if (op.id && op.to == nodeContextTarget)
            text += std::string(operationNames[i]) + ": " + op.status + "\n";
    }
    lv_label_set_text(nodeContextLabel, text.c_str());
}

void TFTView_320x240::nodeContextAction(unsigned action)
{
    const NodeId to = nodeContextTarget;
    const auto *record = nodeRecord(to);
    if (!record || !to || to == UINT32_MAX) return;
    const uint8_t channel = nodeChannel(to), hops = db.config.lora.hop_limit;
    if (action <= 4 && (to == ownNode || ((action == 0 || action == 3) && !nodeIsMessagable(to)))) {
        nodeContextPopup("Node actions", "This node cannot be addressed directly.", false);
        return;
    }
    if (action == 0) {
        if (nodeContextOps[0].pending || nodeContextOps[1].pending) {
            nodeContextPopup("Send My Position", "Position sharing is already pending. Wait for its result or timeout.", false);
            return;
        }
        meshtastic_Position position{};
        if (!messagesRestored || !meshflow_m9_current_position(&position) ||
            !validCoordinates(position.latitude_i, position.longitude_i)) {
            nodeContextPopup("Send My Position", "No valid last known position, or message history is still loading. Nothing sent.", false);
            return;
        }
        // One last-known snapshot, one explicit destination, two independent requests.
        char text[96];
        snprintf(text, sizeof(text), "%.7f, %.7f", position.latitude_i * 1e-7, position.longitude_i * 1e-7);
        closeNodeContext(false);
        showMessages(to);
        const uint32_t positionId = nodeContextBegin(0, to);
        if (!controller->sendNodePosition(to, channel, hops, positionId, position)) {
            nodeContextOps[0].pending = false;
            nodeContextOps[0].status = "Failed to queue";
        }
        const uint32_t textId = nodeContextBegin(1, to);
        requests.removeRequest(textId);
        // Use the normal text request type so ACKs, failures and history use the established path.
        nodeContextOps[1].id = requests.addRequest(to, ResponseHandler::TextMessageRequest,
            reinterpret_cast<void *>(uintptr_t(to)), [this](const ResponseHandler::Request &r, ResponseHandler::EventType e, int32_t v) {
                onTextMessageCallback(r, e, v);
            });
        const uint32_t messageId = nodeContextOps[1].id;
        addMessage(activeMsgContainer, actTime, messageId, text, LogMessage::eNone, to, channel);
        controller->sendTextMessage(to, channel, hops, actTime, messageId, nodeHasKey(to), text);
        return;
    } else if (action == 1 || action == 2) {
        const unsigned operation = action + 1;
        if (nodeContextOps[operation].pending) {
            nodeContextPopup("Node request", "A request is already pending. Wait for its response or timeout.", false);
            return;
        }
        const uint32_t request = nodeContextBegin(operation, to);
        const bool queued = action == 1 ? controller->requestNodePosition(to, channel, hops, request)
                                       : controller->requestNodeIdentity(to, channel, hops, request, db.user);
        if (!queued) {
            nodeContextOps[operation].pending = false;
            nodeContextOps[operation].status = "Failed to queue request";
        }
    } else if (action == 3) {
        closeNodeContext(false);
        if (messagesRestored) showMessages(to);
        else nodeContextPopup("Direct Message", "Message history is still loading.", false);
        return;
    } else if (action == 4) {
        closeNodeContext(false);
        selectNode(to);
        if (spinnerButton) {
            nodeContextPopup("Trace Route", "A traceroute is already running. Wait for it to finish.", false);
            return;
        }
        ui_event_trace_route(nullptr);
        ui_event_trace_route_start(nullptr);
        return;
    } else if (action == 5 || action == 7) {
        M9NodeSnapshot snapshot;
        meshflow_m9_node_snapshot(to, &snapshot);
        char positionText[180];
        if (snapshot.hasPosition && validCoordinates(snapshot.latitude, snapshot.longitude)) {
            if (snapshot.positionTime && actTime >= snapshot.positionTime)
                snprintf(positionText, sizeof(positionText), "Cached position: %.7f, %.7f\nPosition age: %lu seconds",
                    snapshot.latitude * 1e-7, snapshot.longitude * 1e-7, (unsigned long)(actTime - snapshot.positionTime));
            else
                snprintf(positionText, sizeof(positionText), "Cached position: %.7f, %.7f\nPosition age: unknown",
                    snapshot.latitude * 1e-7, snapshot.longitude * 1e-7);
        } else snprintf(positionText, sizeof(positionText), "Position: unavailable");
        if (action == 5) {
            if (!snapshot.hasPosition || !validCoordinates(snapshot.latitude, snapshot.longitude)) {
                nodeContextPopup("Show on Map", "No valid known position for this node.", false);
                return;
            }
            closeNodeContext(false);
            ui_set_active(objects.map_button, objects.map_panel, objects.top_map_panel);
            if (!map) loadMap();
            map->setLocked(false);
            map->setScrolledPosition(snapshot.latitude * 1e-7, snapshot.longitude * 1e-7);
            lv_label_set_text_fmt(objects.top_map_label, "Cached !%08x: %s", to,
                snapshot.positionTime && actTime >= snapshot.positionTime && actTime - snapshot.positionTime <= 3600
                    ? "last known position" : "outdated / age unknown");
            return;
        }
        char details[640], signal[64], battery[64], lastHeard[64], hop[24];
        if (record->signalDisplay == NodeSignalDisplayKind::Rssi)
            snprintf(signal, sizeof(signal), "RSSI: %ld dBm; SNR: %.1f dB", (long)record->rssi, record->snr);
        else snprintf(signal, sizeof(signal), "RF signal: unavailable");
        if (snapshot.hasBattery && snapshot.battery == 101) snprintf(battery, sizeof(battery), "Battery: external power");
        else if (snapshot.hasBattery && snapshot.battery <= 100) snprintf(battery, sizeof(battery), "Battery: %lu%%", (unsigned long)snapshot.battery);
        else snprintf(battery, sizeof(battery), "Battery: unavailable");
        if (snapshot.hasVoltage) {
            char voltage[24]; snprintf(voltage, sizeof(voltage), " (%.2f V)", snapshot.voltage); strncat(battery, voltage, sizeof(battery)-strlen(battery)-1);
        }
        if (record->lastHeard && actTime >= record->lastHeard)
            snprintf(lastHeard, sizeof(lastHeard), "Last heard: %lu seconds ago", (unsigned long)(actTime-record->lastHeard));
        else snprintf(lastHeard, sizeof(lastHeard), "Last heard: unavailable");
        if (record->hopsAway >= 0) snprintf(hop, sizeof(hop), "Hops: %d", record->hopsAway);
        else snprintf(hop, sizeof(hop), "Hops: unknown");
        snprintf(details, sizeof(details), "Long name: %s\nShort name: %s\nNode: !%08x\n%s\n%s\n%s\n%s\n%s",
            record->hasUser ? record->user.long_name : "unavailable", record->hasUser ? record->user.short_name : "unavailable",
            to, lastHeard, signal, hop, battery, positionText);
        nodeContextPopup("Node Details", details, false);
        return;
    } else if (action == 6) {
        const bool favorite = !record->favorite;
        const bool ok = meshflow_m9_set_favorite(to, favorite);
        if (ok) { nodeStore.setFavorite(to, favorite); syncNodeListPresentation(true); }
        nodeContextPopup("Favorite", ok ? (favorite ? "Node marked as favorite." : "Node removed from favorites.")
                                       : "Could not update favorite (node unavailable or protected-node limit reached).", false);
        return;
    }
    nodeContextPopup("Node operation status", "", false);
    nodeContextStatusView = true;
    nodeContextRenderStatus();
}

void TFTView_320x240::nodeContextPacket(const meshtastic_MeshPacket &packet)
{
    if (packet.to != ownNode) return;
    for (unsigned i = 0; i < 4; ++i) {
        auto &op = nodeContextOps[i];
        if (!op.pending || op.id != packet.decoded.request_id) continue;
        if (packet.decoded.portnum == meshtastic_PortNum_ROUTING_APP) {
            meshtastic_Routing routing{};
            if (!pb_decode_from_bytes(packet.decoded.payload.bytes, packet.decoded.payload.size, &meshtastic_Routing_msg, &routing) ||
                routing.which_variant != meshtastic_Routing_error_reason_tag) continue;
            if (routing.error_reason != meshtastic_Routing_Error_NONE) {
                op.status = "Routing failed (error " + std::to_string(routing.error_reason) + ")";
                op.pending = false;
            } else if (packet.from == op.to && i < 2) {
                op.status = "Acknowledged by recipient"; op.pending = false;
            } else op.status = "Mesh accepted; waiting for recipient response";
        } else if (packet.from == op.to && i == 2 && packet.decoded.portnum == meshtastic_PortNum_POSITION_APP) {
            meshtastic_Position pos{};
            if (pb_decode_from_bytes(packet.decoded.payload.bytes, packet.decoded.payload.size, &meshtastic_Position_msg, &pos) &&
                pos.has_latitude_i && pos.has_longitude_i && validCoordinates(pos.latitude_i, pos.longitude_i)) {
                const uint32_t stamp = pos.timestamp ? pos.timestamp : pos.time;
                op.status = stamp && actTime >= stamp && actTime - stamp <= 120
                    ? "New position response received (recent fix)"
                    : "New response received; sender position is outdated or age unknown";
                op.pending = false;
            }
        } else if (packet.from == op.to && i == 3 && packet.decoded.portnum == meshtastic_PortNum_NODEINFO_APP) {
            meshtastic_User user{};
            if (pb_decode_from_bytes(packet.decoded.payload.bytes, packet.decoded.payload.size, &meshtastic_User_msg, &user) &&
                (user.long_name[0] || user.short_name[0])) {
                op.status = "Fresh NodeInfo received; identity updated"; op.pending = false;
            }
        }
        if (!op.pending && i != 1) requests.removeRequest(op.id); // Text ACKs still flow through chat/history handling.
    }
    if (nodeContextStatusView) nodeContextRenderStatus();
}
#endif
