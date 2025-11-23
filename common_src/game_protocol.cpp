#include "game_protocol.h"

#include <netinet/in.h>

namespace GameProtocol {

// Helper: Agregar uint16_t en big-endian
static void push_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

std::vector<uint8_t> serialize_command(const Command& cmd) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAME_COMMAND);
    buffer.push_back(static_cast<uint8_t>(cmd.action));
    push_uint16(buffer, cmd.player_id);
    return buffer;
}

std::vector<uint8_t> serialize_snapshot(const GameSnapshot& snapshot,
                                        const std::vector<CarState>& cars) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAME_SNAPSHOT);

    // Serializar header del snapshot
    uint32_t frame = htonl(snapshot.frame_number);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&frame),
                  reinterpret_cast<uint8_t*>(&frame) + sizeof(frame));

    push_uint16(buffer, cars.size());

    // Serializar cada auto
    for (const auto& car : cars) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&car),
                      reinterpret_cast<const uint8_t*>(&car) + sizeof(CarState));
    }

    return buffer;
}

std::vector<uint8_t> serialize_event(const GameEvent& event) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAME_EVENT);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&event),
                  reinterpret_cast<const uint8_t*>(&event) + sizeof(GameEvent));
    return buffer;
}

std::vector<uint8_t> serialize_race_start() {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_RACE_START);
    return buffer;
}

std::vector<uint8_t> serialize_race_end(const std::vector<uint16_t>& rankings) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_RACE_END);
    push_uint16(buffer, rankings.size());

    for (uint16_t rank : rankings) {
        push_uint16(buffer, rank);
    }

    return buffer;
}

}  // namespace GameProtocol
