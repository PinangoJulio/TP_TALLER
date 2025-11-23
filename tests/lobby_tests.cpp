#include "../common_src/queue.h"
#include "../server_src/game/match.h"
#include "../server_src/network/matches_monitor.h"
#include "common_src/config.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Eq;
using ::testing::Gt;

class MatchesMonitorTest : public ::testing::Test {
protected:
    MatchesMonitor monitor;
    Queue<GameState> dummy_queue;

    void SetUp() override {
        // No cargar config.yaml para evitar problemas en tests
        monitor.clear_all_matches();  // Limpiamos antes de cada test
    }
};

// Test 1: crear una partida correctamente
TEST_F(MatchesMonitorTest, CreateMatch) {
    int match_id = monitor.create_match(4, "player1", 1, dummy_queue);
    EXPECT_GT(match_id, 0);
}

// Test 2: unirse a una partida existente
TEST_F(MatchesMonitorTest, JoinMatch) {
    int match_id = monitor.create_match(4, "player1", 1, dummy_queue);
    bool joined = monitor.join_match(match_id, "player2", 2, dummy_queue);
    EXPECT_TRUE(joined);
}

// Test 3: no se puede unir si el match no existe
TEST_F(MatchesMonitorTest, CannotJoinNonexistentMatch) {
    bool joined = monitor.join_match(999, "playerX", 10, dummy_queue);
    EXPECT_FALSE(joined);
}

// Test 4: agregar carreras a una partida existente
TEST_F(MatchesMonitorTest, AddRacesToMatch) {
    int match_id = monitor.create_match(4, "player1", 1, dummy_queue);

    std::vector<ServerRaceConfig> races = {{"Tokyo", "track1"}, {"Paris", "track2"}};

    EXPECT_TRUE(monitor.add_races_to_match(match_id, races));
}

// Test 5: no se pueden agregar carreras si el match no existe
TEST_F(MatchesMonitorTest, AddRacesToInvalidMatch) {
    std::vector<ServerRaceConfig> races = {{"Tokyo", "track1"}};
    EXPECT_FALSE(monitor.add_races_to_match(999, races));
}

// Test 6: listar partidas disponibles devuelve información coherente
TEST_F(MatchesMonitorTest, ListAvailableMatches) {
    int match_id = monitor.create_match(4, "player1", 1, dummy_queue);
    monitor.join_match(match_id, "player2", 2, dummy_queue);

    auto matches = monitor.list_available_matches();
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].game_id, match_id);
    EXPECT_EQ(matches[0].current_players, 2);
    EXPECT_EQ(matches[0].max_players, 4);
}

// Test 7: asignar auto a jugador
TEST_F(MatchesMonitorTest, SetPlayerCar) {
    int match_id = monitor.create_match(4, "host", 1, dummy_queue);
    monitor.join_match(match_id, "guest", 2, dummy_queue);

    EXPECT_TRUE(monitor.set_player_car("guest", "CarA", "TypeX"));
}

// Test 8: eliminar jugador de partida
TEST_F(MatchesMonitorTest, DeletePlayerFromMatch) {
    int match_id = monitor.create_match(4, "host", 1, dummy_queue);
    monitor.join_match(match_id, "guest", 2, dummy_queue);

    monitor.leave_match_by_id(2, match_id);

    auto matches = monitor.list_available_matches();
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].current_players, 1);
}

// Test 9: limpiar todas las partidas
TEST_F(MatchesMonitorTest, ClearAllMatches) {
    monitor.create_match(4, "p1", 1, dummy_queue);
    monitor.create_match(4, "p2", 2, dummy_queue);
    monitor.clear_all_matches();

    auto matches = monitor.list_available_matches();
    EXPECT_TRUE(matches.empty());
}
