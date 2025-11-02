#ifndef LOBBY_PROTOCOL_H
#define LOBBY_PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>

// Tipos de mensajes del Lobby
enum LobbyMessageType : uint8_t {
    // Cliente → Servidor
    MSG_USERNAME = 0x01,        // Cliente envía su nombre de usuario
    MSG_LIST_GAMES = 0x02,      // Cliente pide lista de partidas
    MSG_CREATE_GAME = 0x03,     // Cliente crea una partida
    MSG_JOIN_GAME = 0x04,       // Cliente se une a una partida
    
    // Servidor → Cliente
    MSG_WELCOME = 0x10,         // Servidor confirma conexión
    MSG_GAMES_LIST = 0x11,      // Servidor envía lista de partidas
    MSG_GAME_CREATED = 0x12,    // Servidor confirma creación de partida
    MSG_GAME_JOINED = 0x13,     // Servidor confirma unión a partida
    MSG_ERROR = 0xFF            // Servidor envía error
};

// Códigos de error
enum LobbyErrorCode : uint8_t {
    ERR_GAME_NOT_FOUND = 0x01,
    ERR_GAME_FULL = 0x02,
    ERR_INVALID_USERNAME = 0x03,
    ERR_GAME_ALREADY_STARTED = 0x04
};

// Estructura para información de una partida
struct GameInfo {
    uint16_t game_id;
    char game_name[32];
    uint8_t current_players;
    uint8_t max_players;
    bool is_started;
} __attribute__((packed));


namespace LobbyProtocol {
    // Funciones de serialización
    std::vector<uint8_t> serialize_username(const std::string& username);
    std::vector<uint8_t> serialize_list_games();
    std::vector<uint8_t> serialize_create_game(const std::string& game_name, uint8_t max_players);
    std::vector<uint8_t> serialize_join_game(uint16_t game_id);
    std::vector<uint8_t> serialize_welcome(const std::string& message);
    std::vector<uint8_t> serialize_games_list(const std::vector<GameInfo>& games);
    std::vector<uint8_t> serialize_game_created(uint16_t game_id);
    std::vector<uint8_t> serialize_game_joined(uint16_t game_id);
    std::vector<uint8_t> serialize_error(LobbyErrorCode error_code, const std::string& message);
}

#endif // LOBBY_PROTOCOL_H
