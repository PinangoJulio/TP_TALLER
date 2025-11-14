#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../common_src/lobby_protocol.h"
#include "../server_src/server_protocol.h"
#include "../client_src/lobby/model/lobby_client.h"
#include "../common_src/dtos.h"
#include "client_src/lobby/view/garage_window.h"
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
        // Servidor escucha en el puerto
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        // Lee el mensaje de tipo y luego el string
        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_USERNAME);

        std::string received = server_protocol.read_string();
        EXPECT_EQ(received, username);
    });

    // Damos un pequeño delay para que el server arranque primero
    std::this_thread::sleep_for(std::chrono::milliseconds(kDelay));

    std::thread client_thread([&]() {
        // Cliente se conecta al servidor
        LobbyClient client(kHost, kPort);
        client.send_username(username);  // <- Este método usa LobbyProtocol::serialize_username()
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

        // Envía un mensaje de bienvenida
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

    // CORRECTO: Ahora que RaceConfig usa std::string, esto compila.
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

        // Luego, recibe los nombres de las ciudades y pistas
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
/*
TEST(ServerClientProtocolTest, CarChosenSerializationAndReception) {
    std::string car_name = "Ferrari";
    std::string car_type = "Sport";

    std::thread server_thread([&]() {
        Socket server_socket(kPort);
        Socket client_conn = server_socket.accept();

        LobbyManager dummy_manager;
        ServerProtocol server_protocol(client_conn, dummy_manager);

        uint8_t type = server_protocol.read_message_type();
        EXPECT_EQ(type, MSG_CAR_CHOSEN);

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
    */
