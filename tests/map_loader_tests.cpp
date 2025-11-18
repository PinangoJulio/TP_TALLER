#include "gtest/gtest.h"
#include "../server_src/game/map_loader.h"
#include "../common_src/config.h"

TEST(MapLoaderTest, LoadTestMap) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacles(mundo);
    MapLoader loader(mundo, obstacles);
    
    ASSERT_NO_THROW({
        loader.load_map("server_src/city_maps/test_track.yaml");
    });
    
    // Verificar spawn points
    auto spawns = loader.get_spawn_points();
    EXPECT_EQ(spawns.size(), 3);
    
    // Verificar checkpoints
    auto checkpoints = loader.get_checkpoints();
    EXPECT_EQ(checkpoints.size(), 4);
    
    b2DestroyWorld(mundo);
}
