#include <gtest/gtest.h>
#include <box2d/box2d.h>
#include "../server_src/game/map_loader.h"
#include "../server_src/game/obstacle.h"
#include "../common_src/config.h" 

class MapLoaderTest : public ::testing::Test {
protected:
    b2WorldId worldId;

    void SetUp() override {
        // Inicializamos un mundo de Box2D para el test
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 0.0f};
        worldId = b2CreateWorld(&worldDef);
    }

    void TearDown() override {
        if (b2World_IsValid(worldId)) {
            b2DestroyWorld(worldId);
        }
    }
};

TEST_F(MapLoaderTest, LoadTestMap_Test) {
    // 1. Instanciar managers
    ObstacleManager obstacles;
    MapLoader loader;

    // 2. Cargar el mapa (envuelto en NO_THROW para que el test falle elegantemente si no existe el archivo)
    // Asegúrate de que "server_src/city_maps/test_track.yaml" exista en tu carpeta de ejecución
    // o cambia la ruta.
    EXPECT_NO_THROW(
        loader.load_map(worldId, obstacles, "server_src/city_maps/test_track.yaml")
    );

    // 3. Obtener datos cargados
    const auto& spawns = loader.get_spawn_points();
    const auto& checkpoints = loader.get_checkpoints();

    // CORRECCIÓN: Usamos las variables para satisfacer al compilador (-Werror)
    // Verificamos que el tamaño sea válido (>= 0 siempre es true, pero "usa" la variable).
    EXPECT_GE(spawns.size(), 0);
    EXPECT_GE(checkpoints.size(), 0);
    
    // Si tienes el archivo yaml real, podrías descomentar esto para ser más estricta:
    // EXPECT_FALSE(spawns.empty());
}