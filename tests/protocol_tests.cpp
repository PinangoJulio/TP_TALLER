#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../common_src/lobby_protocol.h"
#include "../server_src/server_protocol.h"
#include "../client_src/lobby/model/lobby_client.h"
#include "../common_src/dtos.h"
#include "common_src/config.h"

using ::testing::Eq;

constexpr const char* kHost = "127.0.0.1";
constexpr const char* kPort = "8085";
constexpr int kDelay = 100;

// ============================================================
// TESTS DE INTEGRACIÓN: CLIENTE ↔ SERVIDOR REALES
// ============================================================

TEST(ServerClientProtocolTest, UsernameSerializationAndReception) {
    std::string username = "Lourdes";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_USERNAME);

        std::string received = server_protocol.read_string();
        EXPECT_EQ(received, username);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
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

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        server_protocol.send_buffer(LobbyProtocol::serialize_welcome(welcome_msg));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        std::string received_msg = client.receive_welcome();

        EXPECT_EQ(received_msg, welcome_msg);
    });

    client_thread.join();
    server_thread.join();
}

TEST(ServerClientProtocolTest, CreateGameSerializationAndReception) {
    std::string game_name = "Carrera1";
    uint8_t max_players = 4;
    std::vector<std::pair<std::string, std::string>> races = {{"Tokyo","track1"}, {"Paris","track2"}};

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

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
        LobbyClient client(kHost, kPort);
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

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_JOIN_GAME);

        uint16_t received_id = server_protocol.read_uint16();
        EXPECT_EQ(received_id, game_id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        client.join_game(game_id);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 5: Selección de auto (MSG_SELECT_CAR)
TEST(ServerClientProtocolTest, SelectCarSerializationAndReception) {
    std::string car_name = "Ferrari";
    std::string car_type = "Sport";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_SELECT_CAR);

        std::string received_name = server_protocol.read_string();
        std::string received_type = server_protocol.read_string();

        EXPECT_EQ(received_name, car_name);
        EXPECT_EQ(received_type, car_type);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        client.select_car(car_name, car_type);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 6: Leave game
TEST(ServerClientProtocolTest, LeaveGameSerializationAndReception) {
    uint16_t game_id = 7;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_LEAVE_GAME);

        uint16_t received_id = server_protocol.read_uint16();
        EXPECT_EQ(received_id, game_id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        client.leave_game(game_id);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 7: Error message serialization
TEST(ServerClientProtocolTest, ErrorMessageSerialization) {
    LobbyErrorCode error_code = ERR_GAME_FULL;
    std::string error_msg = "Game is full";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        auto buffer = LobbyProtocol::serialize_error(error_code, error_msg);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        uint8_t type = client.peek_message_type();
        
        EXPECT_EQ(type, MSG_ERROR);
        
        std::string received_error;
        client.read_error_details(received_error);
        EXPECT_EQ(received_error, error_msg);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 8: List games con múltiples partidas
TEST(ServerClientProtocolTest, ListMultipleGames) {
    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

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
        LobbyClient client(kHost, kPort);
        client.request_games_list();
        
        auto received_games = client.receive_games_list();
        EXPECT_EQ(received_games.size(), 2);
        EXPECT_EQ(received_games[0].game_id, 1);
        EXPECT_EQ(received_games[1].game_id, 2);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 9: Game created confirmation
TEST(ServerClientProtocolTest, GameCreatedConfirmation) {
    uint16_t expected_game_id = 123;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        auto buffer = LobbyProtocol::serialize_game_created(expected_game_id);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        uint16_t received_id = client.receive_game_created();
        
        EXPECT_EQ(received_id, expected_game_id);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 10: Game joined confirmation
TEST(ServerClientProtocolTest, GameJoinedConfirmation) {
    uint16_t expected_game_id = 456;

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        auto buffer = LobbyProtocol::serialize_game_joined(expected_game_id);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        uint16_t received_id = client.receive_game_joined();
        
        EXPECT_EQ(received_id, expected_game_id);
    });

    client_thread.join();
    server_thread.join();
}

// 🔥 TEST 11: Car selection acknowledgment
TEST(ServerClientProtocolTest, CarSelectedAcknowledgment) {
    std::string car_name = "Mustang";
    std::string car_type = "Muscle";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        auto buffer = LobbyProtocol::serialize_car_selected_ack(car_name, car_type);
        server_protocol.send_buffer(buffer);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        LobbyClient client(kHost, kPort);
        std::string confirmed_car = client.receive_car_confirmation();
        
        EXPECT_EQ(confirmed_car, car_name);
    });

    client_thread.join();
    server_thread.join();
}
