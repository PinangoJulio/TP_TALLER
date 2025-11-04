#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../server_src/lobby/lobby_manager.h"

TEST(LobbyManagerTest, CreateGame) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    
    EXPECT_GT(game_id, 0);
    EXPECT_TRUE(manager.is_player_in_game("player1"));
    EXPECT_EQ(manager.get_player_game("player1"), game_id);
}

TEST(LobbyManagerTest, CannotCreateMultipleGames) {
    LobbyManager manager;
    uint16_t game1 = manager.create_game("Game 1", "player1", 4);
    uint16_t game2 = manager.create_game("Game 2", "player1", 4);
    
    EXPECT_GT(game1, 0);
    EXPECT_EQ(game2, 0);  // Debe fallar
}

TEST(LobbyManagerTest, JoinGame) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    
    EXPECT_TRUE(manager.join_game(game_id, "player2"));
    EXPECT_TRUE(manager.is_player_in_game("player2"));
}

TEST(LobbyManagerTest, CannotJoinMultipleGames) {
    LobbyManager manager;
    uint16_t game1 = manager.create_game("Game 1", "player1", 4);
    uint16_t game2 = manager.create_game("Game 2", "player2", 4);
    
    EXPECT_TRUE(manager.join_game(game1, "player3"));
    EXPECT_FALSE(manager.join_game(game2, "player3"));  // Debe fallar
}

TEST(LobbyManagerTest, AutoDestroyGameWithLessThanTwoPlayers) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    manager.join_game(game_id, "player2");
    
    // Ambos jugadores abandonan
    manager.leave_game("player1");
    manager.leave_game("player2");
    
    // La partida debe haber sido destruida
    EXPECT_FALSE(manager.join_game(game_id, "player3"));
}

TEST(LobbyManagerTest, HostCanStartGame) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    manager.join_game(game_id, "player2");
    
    EXPECT_TRUE(manager.is_game_ready(game_id));
    EXPECT_TRUE(manager.start_game(game_id, "player1"));  // Host puede iniciar
}

TEST(LobbyManagerTest, NonHostCannotStartGame) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    manager.join_game(game_id, "player2");
    
    EXPECT_FALSE(manager.start_game(game_id, "player2"));  // No-host no puede
}

TEST(LobbyManagerTest, CannotStartWithLessThanTwoPlayers) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    
    EXPECT_FALSE(manager.is_game_ready(game_id));
    EXPECT_FALSE(manager.start_game(game_id, "player1"));
}

TEST(LobbyManagerTest, HostTransferOnLeave) {
    LobbyManager manager;
    uint16_t game_id = manager.create_game("Test Game", "player1", 4);
    manager.join_game(game_id, "player2");
    manager.join_game(game_id, "player3");  // Agregar un 3er jugador
    
    // player1 (host original) se va
    manager.leave_game("player1");
    
    // player2 debería ser el nuevo host
    // El juego NO se destruye porque quedan 2 jugadores (player2 y player3)
    EXPECT_TRUE(manager.is_player_in_game("player2"));
    EXPECT_TRUE(manager.is_player_in_game("player3"));
    EXPECT_TRUE(manager.is_game_ready(game_id));
    
    // player2 (nuevo host) puede iniciar el juego
    EXPECT_TRUE(manager.start_game(game_id, "player2"));
}

