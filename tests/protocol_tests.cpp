#include <arpa/inet.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "../client_src/client_protocol.h"
#include "../client_src/lobby/model/lobby_client.h"
#include "../common_src/config.h"
#include "../common_src/dtos.h"
#include "../common_src/lobby_protocol.h"
#include "../server_src/server_protocol.h"

constexpr const char* kHost = "127.0.0.1";
constexpr const char* kPort = "8085";
constexpr int kDelay = 100;

// TESTS DE INTEGRACIÓN: CLIENTE ↔ SERVIDOR REALES

TEST(ServerClientProtocolTest, UsernameSerializationAndReception) {
    std::string username = "Lourdes";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_USERNAME);

        std::string received = server_protocol.read_string();
        EXPECT_EQ(received, username);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        LobbyClient client(protocol);
        client.send_username(username);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, SendAndReceiveWelcomeMessage) {
    std::string welcome_msg = "Bienvenida!";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        server_protocol.send_buffer(LobbyProtocol::serialize_welcome(welcome_msg));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        LobbyClient client(protocol);
        std::string received_msg = client.receive_welcome();

        EXPECT_EQ(received_msg, welcome_msg);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, CreateGameSerializationAndReception) {
    std::string game_name = "Carrera1";
    uint8_t max_players = 4;
    std::vector<std::pair<std::string, std::string>> races = {{"Tokyo", "track1"},
                                                              {"Paris", "track2"}};

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_CREATE_GAME);

        std::string received_game_name = server_protocol.read_string();
        EXPECT_EQ(received_game_name, game_name);

        uint8_t received_max_players = server_protocol.get_uint8_t();
        EXPECT_EQ(received_max_players, max_players);

        uint8_t received_races_count = server_protocol.get_uint8_t();
        EXPECT_EQ(received_races_count, races.size());

        for (size_t i = 0; i < races.size(); ++i) {
            std::string city = server_protocol.read_string();
            std::string track = server_protocol.read_string();
            EXPECT_EQ(city, races[i].first);
            EXPECT_EQ(track, races[i].second);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        LobbyClient client(protocol);
        try {
            client.create_game(game_name, max_players, races);
        } catch (const std::exception& e) {
            std::cerr << "Exception in client.create_game: " << e.what() << std::endl;
            throw;
        }
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, JoinGameSerializationAndReception) {
    uint16_t game_id = 42;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_JOIN_GAME);

        uint16_t received_id = server_protocol.read_uint16();
        EXPECT_EQ(received_id, game_id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        client.join_game(game_id);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, SelectCarSerializationAndReception) {
    std::string car_name = "Ferrari";
    std::string car_type = "Sport";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_SELECT_CAR);

        std::string received_name = server_protocol.read_string();
        std::string received_type = server_protocol.read_string();

        EXPECT_EQ(received_name, car_name);
        EXPECT_EQ(received_type, car_type);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        client.select_car(car_name, car_type);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, LeaveGameSerializationAndReception) {
    uint16_t game_id = 7;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_LEAVE_GAME);

        uint16_t received_id = server_protocol.read_uint16();
        EXPECT_EQ(received_id, game_id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        client.leave_game(game_id);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, ErrorMessageSerialization) {
    LobbyErrorCode error_code = ERR_GAME_FULL;
    std::string error_msg = "Game is full";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        auto buffer = LobbyProtocol::serialize_error(error_code, error_msg);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        uint8_t type = client.peek_message_type();

        EXPECT_EQ(type, MSG_ERROR);

        std::string received_error;
        client.read_error_details(received_error);
        EXPECT_EQ(received_error, error_msg);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, ListMultipleGames) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_LIST_GAMES);

        // Simular respuesta con 2 partidas
        std::vector<GameInfo> games(2);
        games[0].game_id = 1;
        strncpy(games[0].game_name, "Sala 1", sizeof(games[0].game_name));
        games[0].current_players = 2;
        games[0].max_players = 4;
        games[0].is_started = false;

        games[1].game_id = 2;
        strncpy(games[1].game_name, "Sala 2", sizeof(games[1].game_name));
        games[1].current_players = 1;
        games[1].max_players = 8;
        games[1].is_started = false;

        auto buffer = LobbyProtocol::serialize_games_list(games);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        client.request_games_list();

        auto received_games = client.receive_games_list();
        EXPECT_EQ(received_games.size(), 2);
        EXPECT_EQ(received_games[0].game_id, 1);
        EXPECT_EQ(received_games[1].game_id, 2);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, GameCreatedConfirmation) {
    uint16_t expected_game_id = 123;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        auto buffer = LobbyProtocol::serialize_game_created(expected_game_id);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        uint16_t received_id = client.receive_game_created();

        EXPECT_EQ(received_id, expected_game_id);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, GameJoinedConfirmation) {
    uint16_t expected_game_id = 456;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        auto buffer = LobbyProtocol::serialize_game_joined(expected_game_id);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        uint16_t received_id = client.receive_game_joined();

        EXPECT_EQ(received_id, expected_game_id);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, CarSelectedAcknowledgment) {
    std::string car_name = "Mustang";
    std::string car_type = "Muscle";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        auto buffer = LobbyProtocol::serialize_car_selected_ack(car_name, car_type);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        LobbyClient client(protocol);
        std::string confirmed_car = client.receive_car_confirmation();

        EXPECT_EQ(confirmed_car, car_name);
    });

    client_thread.join();
    server_thread.join();
}

// ============================================================
// TESTS DE COMANDOS DEL JUEGO (GAME COMMANDS)
// ============================================================

TEST(GameCommandProtocolTest, AccelerateCommandSimple) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::ACCELERATE);
        EXPECT_FLOAT_EQ(command.speed_boost, 1.0f);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        // Enviar solo el código de comando (1 byte)
        uint8_t cmd = CMD_ACCELERATE;
        client_socket.sendall(&cmd, sizeof(cmd));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, BrakeCommandSimple) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::BRAKE);
        EXPECT_FLOAT_EQ(command.speed_boost, 1.0f);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        uint8_t cmd = CMD_BRAKE;
        client_socket.sendall(&cmd, sizeof(cmd));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, TurnLeftWithIntensity) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::TURN_LEFT);
        EXPECT_FLOAT_EQ(command.turn_intensity, 0.75f);  // 75 / 100.0
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        // Enviar código + intensidad (2 bytes)
        uint8_t cmd = CMD_TURN_LEFT;
        uint8_t intensity = 75;  // 75%

        client_socket.sendall(&cmd, sizeof(cmd));
        client_socket.sendall(&intensity, sizeof(intensity));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, TurnRightWithIntensity) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::TURN_RIGHT);
        EXPECT_FLOAT_EQ(command.turn_intensity, 0.50f);  // 50 / 100.0
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        uint8_t cmd = CMD_TURN_RIGHT;
        uint8_t intensity = 50;  // 50%

        client_socket.sendall(&cmd, sizeof(cmd));
        client_socket.sendall(&intensity, sizeof(intensity));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, UseNitroCommand) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::USE_NITRO);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        uint8_t cmd = CMD_USE_NITRO;
        client_socket.sendall(&cmd, sizeof(cmd));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, CheatTeleportWithCheckpoint) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::CHEAT_TELEPORT_CHECKPOINT);
        EXPECT_EQ(command.checkpoint_id, 5);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        // Enviar código + checkpoint_id (3 bytes)
        uint8_t cmd = CMD_CHEAT_TELEPORT;
        uint16_t checkpoint_id = htons(5);  // Network order

        client_socket.sendall(&cmd, sizeof(cmd));
        client_socket.sendall(&checkpoint_id, sizeof(checkpoint_id));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, UpgradeSpeedCommand) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::UPGRADE_SPEED);
        EXPECT_EQ(command.upgrade_type, UpgradeType::SPEED);
        EXPECT_EQ(command.upgrade_level, 2);
        EXPECT_EQ(command.upgrade_cost_ms, 500);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        // Enviar código + nivel + costo (4 bytes)
        uint8_t cmd = CMD_UPGRADE_SPEED;
        uint8_t level = 2;
        uint16_t cost = htons(500);  // Network order

        client_socket.sendall(&cmd, sizeof(cmd));
        client_socket.sendall(&level, sizeof(level));
        client_socket.sendall(&cost, sizeof(cost));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, DisconnectCommand) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        ComandMatchDTO command;
        bool success = server_protocol.read_command_client(command);

        EXPECT_TRUE(success);
        EXPECT_EQ(command.command, GameCommand::DISCONNECT);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        uint8_t cmd = CMD_DISCONNECT;
        client_socket.sendall(&cmd, sizeof(cmd));
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameCommandProtocolTest, MultipleCommandsSequence) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        // Comando 1: ACCELERATE
        ComandMatchDTO cmd1;
        bool success1 = server_protocol.read_command_client(cmd1);
        EXPECT_TRUE(success1);
        EXPECT_EQ(cmd1.command, GameCommand::ACCELERATE);

        // Comando 2: TURN_LEFT con intensidad
        ComandMatchDTO cmd2;
        bool success2 = server_protocol.read_command_client(cmd2);
        EXPECT_TRUE(success2);
        EXPECT_EQ(cmd2.command, GameCommand::TURN_LEFT);
        EXPECT_FLOAT_EQ(cmd2.turn_intensity, 0.80f);

        // Comando 3: USE_NITRO
        ComandMatchDTO cmd3;
        bool success3 = server_protocol.read_command_client(cmd3);
        EXPECT_TRUE(success3);
        EXPECT_EQ(cmd3.command, GameCommand::USE_NITRO);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        Socket client_socket(kHost, kPort);

        // Enviar 3 comandos consecutivos
        uint8_t cmd1 = CMD_ACCELERATE;
        client_socket.sendall(&cmd1, sizeof(cmd1));

        uint8_t cmd2 = CMD_TURN_LEFT;
        uint8_t intensity = 80;
        client_socket.sendall(&cmd2, sizeof(cmd2));
        client_socket.sendall(&intensity, sizeof(intensity));

        uint8_t cmd3 = CMD_USE_NITRO;
        client_socket.sendall(&cmd3, sizeof(cmd3));
    });

    client_thread.join();
    server_thread.join();
}

// TESTS DE RACE_INFO (INFORMACIÓN INICIAL DE CARRERA)

TEST(RaceInfoProtocolTest, SendAndReceiveRaceInfo) {
    // Datos de prueba
    RaceInfoDTO sent_info;
    std::memset(&sent_info, 0, sizeof(sent_info));
    std::strncpy(sent_info.city_name, "Vice City", sizeof(sent_info.city_name) - 1);
    std::strncpy(sent_info.race_name, "Playa", sizeof(sent_info.race_name) - 1);
    std::strncpy(sent_info.map_file_path, "server_src/city_maps/Vice City/Playa",
                 sizeof(sent_info.map_file_path) - 1);
    sent_info.total_laps = 3;
    sent_info.race_number = 1;
    sent_info.total_races = 3;
    sent_info.total_checkpoints = 15;
    sent_info.max_time_ms = 600000;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        // Enviar RACE_INFO
        bool success = server_protocol.send_race_info(sent_info);
        EXPECT_TRUE(success);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        // Recibir RACE_INFO
        RaceInfoDTO received_info = protocol.receive_race_info();

        // Verificar que los datos sean correctos
        EXPECT_STREQ(received_info.city_name, sent_info.city_name);
        EXPECT_STREQ(received_info.race_name, sent_info.race_name);
        EXPECT_STREQ(received_info.map_file_path, sent_info.map_file_path);
        EXPECT_EQ(received_info.total_laps, sent_info.total_laps);
        EXPECT_EQ(received_info.race_number, sent_info.race_number);
        EXPECT_EQ(received_info.total_races, sent_info.total_races);
        EXPECT_EQ(received_info.total_checkpoints, sent_info.total_checkpoints);
        EXPECT_EQ(received_info.max_time_ms, sent_info.max_time_ms);
    });

    client_thread.join();
    server_thread.join();
}

TEST(RaceInfoProtocolTest, SendRaceInfoWithLongPaths) {
    // Test con rutas más largas
    RaceInfoDTO sent_info;
    std::memset(&sent_info, 0, sizeof(sent_info));
    std::strncpy(sent_info.city_name, "San Andreas", sizeof(sent_info.city_name) - 1);
    std::strncpy(sent_info.race_name, "Desierto del Diablo Extremo",
                 sizeof(sent_info.race_name) - 1);
    std::strncpy(sent_info.map_file_path,
                 "server_src/city_maps/San Andreas/Desierto del Diablo Extremo",
                 sizeof(sent_info.map_file_path) - 1);
    sent_info.total_laps = 5;
    sent_info.race_number = 2;
    sent_info.total_races = 5;
    sent_info.total_checkpoints = 25;
    sent_info.max_time_ms = 900000;  // 15 minutos

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);
        bool success = server_protocol.send_race_info(sent_info);
        EXPECT_TRUE(success);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        RaceInfoDTO received_info = protocol.receive_race_info();

        EXPECT_STREQ(received_info.city_name, "San Andreas");
        EXPECT_STREQ(received_info.race_name, "Desierto del Diablo Extremo");
        EXPECT_EQ(received_info.total_laps, 5);
        EXPECT_EQ(received_info.race_number, 2);
        EXPECT_EQ(received_info.total_races, 5);
        EXPECT_EQ(received_info.total_checkpoints, 25);
        EXPECT_EQ(received_info.max_time_ms, 900000);
    });

    client_thread.join();
    server_thread.join();
}

TEST(RaceInfoProtocolTest, SendRaceInfoFirstRaceOfThree) {
    // Simular la primera carrera de una partida de 3 carreras
    RaceInfoDTO sent_info;
    std::memset(&sent_info, 0, sizeof(sent_info));
    std::strncpy(sent_info.city_name, "Liberty City", sizeof(sent_info.city_name) - 1);
    std::strncpy(sent_info.race_name, "Circuito Centro", sizeof(sent_info.race_name) - 1);
    std::strncpy(sent_info.map_file_path, "server_src/city_maps/Liberty City/Circuito Centro",
                 sizeof(sent_info.map_file_path) - 1);
    sent_info.total_laps = 3;
    sent_info.race_number = 1;  // Primera carrera
    sent_info.total_races = 3;  // De 3 totales
    sent_info.total_checkpoints = 12;
    sent_info.max_time_ms = 600000;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);
        bool success = server_protocol.send_race_info(sent_info);
        EXPECT_TRUE(success);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        RaceInfoDTO received_info = protocol.receive_race_info();

        // Verificar que es la primera carrera de 3
        EXPECT_EQ(received_info.race_number, 1);
        EXPECT_EQ(received_info.total_races, 3);
        EXPECT_STREQ(received_info.city_name, "Liberty City");
    });

    client_thread.join();
    server_thread.join();
}

TEST(RaceInfoProtocolTest, SendRaceInfoWithMinimalData) {
    // Test con datos mínimos
    RaceInfoDTO sent_info;
    std::memset(&sent_info, 0, sizeof(sent_info));
    std::strncpy(sent_info.city_name, "VC", sizeof(sent_info.city_name) - 1);
    std::strncpy(sent_info.race_name, "T1", sizeof(sent_info.race_name) - 1);
    std::strncpy(sent_info.map_file_path, "maps/t1", sizeof(sent_info.map_file_path) - 1);
    sent_info.total_laps = 1;
    sent_info.race_number = 1;
    sent_info.total_races = 1;
    sent_info.total_checkpoints = 5;
    sent_info.max_time_ms = 60000;  // 1 minuto

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);
        bool success = server_protocol.send_race_info(sent_info);
        EXPECT_TRUE(success);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);
        RaceInfoDTO received_info = protocol.receive_race_info();

        EXPECT_STREQ(received_info.city_name, "VC");
        EXPECT_STREQ(received_info.race_name, "T1");
        EXPECT_EQ(received_info.total_laps, 1);
        EXPECT_EQ(received_info.total_checkpoints, 5);
        EXPECT_EQ(received_info.max_time_ms, 60000);
    });

    client_thread.join();
    server_thread.join();
}

TEST(RaceInfoProtocolTest, SendMultipleRaceInfoSequentially) {
    // Simular envío de info de múltiples carreras consecutivas
    std::vector<std::pair<std::string, std::string>> races = {
        {"Vice City", "Playa"},
        {"Liberty City", "Centro"},
        {"San Andreas", "Desierto"}
    };

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        ServerProtocol server_protocol(client_conn);

        for (size_t i = 0; i < races.size(); ++i) {
            RaceInfoDTO info;
            std::memset(&info, 0, sizeof(info));
            std::strncpy(info.city_name, races[i].first.c_str(), sizeof(info.city_name) - 1);
            std::strncpy(info.race_name, races[i].second.c_str(), sizeof(info.race_name) - 1);
            info.total_laps = 3;
            info.race_number = static_cast<uint8_t>(i + 1);
            info.total_races = static_cast<uint8_t>(races.size());
            info.total_checkpoints = 10 + (i * 5);
            info.max_time_ms = 600000;

            bool success = server_protocol.send_race_info(info);
            EXPECT_TRUE(success);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol protocol(kHost, kPort);

        for (size_t i = 0; i < races.size(); ++i) {
            RaceInfoDTO received_info = protocol.receive_race_info();

            EXPECT_STREQ(received_info.city_name, races[i].first.c_str());
            EXPECT_STREQ(received_info.race_name, races[i].second.c_str());
            EXPECT_EQ(received_info.race_number, static_cast<uint8_t>(i + 1));
            EXPECT_EQ(received_info.total_races, static_cast<uint8_t>(races.size()));
            EXPECT_EQ(received_info.total_checkpoints, 10 + (i * 5));
        }
    });

    client_thread.join();
    server_thread.join();
}

TEST(GameSnapshotProtocolTest, SendAndReceiveSnapshotBasic) {
    // === Construir snapshot que enviará el servidor ===
    GameState sent;

    // ---- PLAYERS ----
    InfoPlayer p1;
    p1.player_id = 1;
    p1.username = "Lourdes";
    p1.car_name = "Inferno";
    p1.car_type = "Sport";

    p1.pos_x = 123.4f;
    p1.pos_y = 567.8f;
    p1.angle = 1.57f;
    p1.speed = 42.0f;
    p1.velocity_x = 1.0f;
    p1.velocity_y = 3.0f;

    p1.health = 75.0f;
    p1.nitro_amount = 50.0f;
    p1.nitro_active = true;
    p1.is_drifting = false;
    p1.is_colliding = true;

    p1.completed_laps = 1;
    p1.current_checkpoint = 2;
    p1.position_in_race = 3;
    p1.race_time_ms = 120000;

    p1.race_finished = false;
    p1.is_alive = true;
    p1.disconnected = false;

    sent.players.push_back(p1);


    // ---- CHECKPOINTS ----
    CheckpointInfo c1;
    c1.id = 10;
    c1.pos_x = 400.0f;
    c1.pos_y = 800.0f;
    c1.width = 50.0f;
    c1.angle = 0.5f;
    c1.is_start = true;
    c1.is_finish = false;
    sent.checkpoints.push_back(c1);


    // ---- HINTS ----
    HintInfo h1;
    h1.id = 5;
    h1.pos_x = 900.0f;
    h1.pos_y = 300.0f;
    h1.direction_angle = 0.75f;
    h1.for_checkpoint = 10;
    sent.hints.push_back(h1);


    // ---- NPC ----
    NPCCarInfo npc1;
    npc1.npc_id = 77;
    npc1.pos_x = 50.0f;
    npc1.pos_y = 60.0f;
    npc1.angle = 0.33f;
    npc1.speed = 10.0f;
    npc1.car_model = "truck";
    npc1.is_parked = false;
    sent.npcs.push_back(npc1);


    // ---- RACE CURRENT INFO ----
    sent.race_current_info.city = "Vice City";
    sent.race_current_info.race_name = "Playa";
    sent.race_current_info.total_laps = 3;
    sent.race_current_info.total_checkpoints = 12;


    // ---- RACE INFO ----
    sent.race_info.status = MatchStatus::IN_PROGRESS;
    sent.race_info.race_number = 1;
    sent.race_info.total_races = 3;
    sent.race_info.remaining_time_ms = 300000;
    sent.race_info.players_finished = 0;
    sent.race_info.total_players = 5;
    sent.race_info.winner_name = "";


    // ---- EVENTS ----
    GameEvent e1;
    e1.type = GameEvent::EXPLOSION;
    e1.player_id = 1;
    e1.pos_x = 123.0f;
    e1.pos_y = 321.0f;
    sent.events.push_back(e1);


    // ======================= THREADS ===========================
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();
        ServerProtocol sp(client_conn);

        EXPECT_TRUE(sp.send_snapshot(sent));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        ClientProtocol cp(kHost, kPort);
        GameState received = cp.receive_snapshot();

        // ---- PLAYERS ----
        //ASSERT_EQ(received.players.size(), 1);
        const InfoPlayer& rp = received.players[0];

        EXPECT_EQ(rp.player_id, 1);
        EXPECT_EQ(rp.username, "Lourdes");
        EXPECT_EQ(rp.car_name, "Inferno");
        EXPECT_EQ(rp.car_type, "Sport");

        EXPECT_FLOAT_EQ(rp.pos_x, 123.4f);
        EXPECT_FLOAT_EQ(rp.pos_y, 567.8f);
        EXPECT_FLOAT_EQ(rp.angle, 1.57f);
        EXPECT_FLOAT_EQ(rp.speed, 42.0f);
        EXPECT_FLOAT_EQ(rp.velocity_x, -1.0f);
        EXPECT_FLOAT_EQ(rp.velocity_y, 3.0f);

        EXPECT_FLOAT_EQ(rp.health, 75.0f);
        EXPECT_FLOAT_EQ(rp.nitro_amount, 50.0f);
        EXPECT_TRUE(rp.nitro_active);
        EXPECT_FALSE(rp.is_drifting);
        EXPECT_TRUE(rp.is_colliding);

        EXPECT_EQ(rp.completed_laps, 1);
        EXPECT_EQ(rp.current_checkpoint, 2);
        EXPECT_EQ(rp.position_in_race, 3);
        EXPECT_EQ(rp.race_time_ms, 120000);

        EXPECT_FALSE(rp.race_finished);
        EXPECT_TRUE(rp.is_alive);
        EXPECT_FALSE(rp.disconnected);


        // ---- CHECKPOINT ----
        ASSERT_EQ(received.checkpoints.size(), 1);
        const auto& rc = received.checkpoints[0];
        EXPECT_EQ(rc.id, 10);
        EXPECT_FLOAT_EQ(rc.pos_x, 400.0f);
        EXPECT_FLOAT_EQ(rc.pos_y, 800.0f);
        EXPECT_FLOAT_EQ(rc.width, 50.0f);
        EXPECT_FLOAT_EQ(rc.angle, 0.5f);
        EXPECT_TRUE(rc.is_start);
        EXPECT_FALSE(rc.is_finish);


        // ---- HINT ----
        ASSERT_EQ(received.hints.size(), 1);
        const auto& rh = received.hints[0];
        EXPECT_EQ(rh.id, 5);
        EXPECT_FLOAT_EQ(rh.pos_x, 900.0f);
        EXPECT_FLOAT_EQ(rh.pos_y, 300.0f);
        EXPECT_FLOAT_EQ(rh.direction_angle, 0.75f);
        EXPECT_EQ(rh.for_checkpoint, 10);


        // ---- NPC ----
        ASSERT_EQ(received.npcs.size(), 1);
        const auto& rnpc = received.npcs[0];
        EXPECT_EQ(rnpc.npc_id, 77);
        EXPECT_FLOAT_EQ(rnpc.pos_x, 50.0f);
        EXPECT_FLOAT_EQ(rnpc.pos_y, 60.0f);
        EXPECT_FLOAT_EQ(rnpc.angle, 0.33f);
        EXPECT_FLOAT_EQ(rnpc.speed, 10.0f);
        EXPECT_EQ(rnpc.car_model, "truck");
        EXPECT_FALSE(rnpc.is_parked);


        // ---- RACE CURRENT INFO ----
        EXPECT_EQ(received.race_current_info.city, "Vice City");
        EXPECT_EQ(received.race_current_info.race_name, "Playa");
        EXPECT_EQ(received.race_current_info.total_laps, 3);
        EXPECT_EQ(received.race_current_info.total_checkpoints, 12);


        // ---- RACE INFO ----
        EXPECT_EQ(received.race_info.status, MatchStatus::IN_PROGRESS);
        EXPECT_EQ(received.race_info.race_number, 1);
        EXPECT_EQ(received.race_info.total_races, 3);
        EXPECT_EQ(received.race_info.remaining_time_ms, 300000);
        EXPECT_EQ(received.race_info.players_finished, 0);
        EXPECT_EQ(received.race_info.total_players, 5);
        EXPECT_EQ(received.race_info.winner_name, "");


        // ---- EVENTS ----
        ASSERT_EQ(received.events.size(), 1);
        const auto& re = received.events[0];
        EXPECT_EQ(re.type, GameEvent::EXPLOSION);
        EXPECT_EQ(re.player_id, 1);
        EXPECT_FLOAT_EQ(re.pos_x, 123.0f);
        EXPECT_FLOAT_EQ(re.pos_y, 321.0f);
    });

    client_thread.join();
    server_thread.join();
}
