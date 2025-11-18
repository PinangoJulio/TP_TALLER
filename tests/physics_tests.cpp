#include "gtest/gtest.h"
#include <box2d/box2d.h>
#include "../common_src/config.h"
#include "../server_src/game/game_loop.h"
#include "../server_src/network/monitor.h"

// Verifica que el archivo YAML se lea sin errores
TEST(ConfigTest, LoadConfiguration) {
    ASSERT_NO_THROW({
        Configuracion config("config.yaml");
    });
}

// Verifica que los valores del YAML coincidan con lo esperado
TEST(ConfigTest, ConfigurationValues) {
    Configuracion config("config.yaml");
    
    EXPECT_FLOAT_EQ(config.obtenerGravedadX(), 0.0f);
    EXPECT_FLOAT_EQ(config.obtenerGravedadY(), 0.0f);
    
    EXPECT_NEAR(config.obtenerTiempoPaso(), 0.016666f, 0.0001f);
    
    EXPECT_EQ(config.obtenerIteracionesVelocidad(), 8);
    EXPECT_EQ(config.obtenerIteracionesPosicion(), 3);
}

// Verifica que el auto se cargue correctamente del YAML
TEST(ConfigTest, CarAttributes) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& deportivo = config.obtenerAtributosAuto("DEPORTIVO");
    
    EXPECT_FLOAT_EQ(deportivo.velocidad_maxima, 15.0f);
    EXPECT_FLOAT_EQ(deportivo.aceleracion, 1.0f);
    EXPECT_EQ(deportivo.salud_base, 100);
    EXPECT_FLOAT_EQ(deportivo.masa, 1.5f);
    EXPECT_FLOAT_EQ(deportivo.control, 8.0f);
}

// Verifica que de error si el auto no existe
TEST(ConfigTest, InvalidCarType) {
    Configuracion config("config.yaml");
    
    EXPECT_THROW(
        config.obtenerAtributosAuto("AUTO_IMAGINARIO"),
        std::runtime_error
    );
}

// Verifica que el mundo de Box2D se cree sin errores
TEST(PhysicsTest, Box2DWorldCreation) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {config.obtenerGravedadX(), config.obtenerGravedadY()};
    b2WorldId mundo = b2CreateWorld(&mundoDef);

    EXPECT_TRUE(b2World_IsValid(mundo));
    b2DestroyWorld(mundo);
}

// Verifica que se pueda crear un cuerpo de Box2D
TEST(PhysicsTest, CreateDynamicBody) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {10.0f, 10.0f};
    
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    EXPECT_TRUE(B2_IS_NON_NULL(body));
    
    b2Vec2 pos = b2Body_GetPosition(body);
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 10.0f);
    
    b2DestroyWorld(mundo);
}

// Verifica que aplicar una fuerza cambie la velocidad del cuerpo
TEST(PhysicsTest, ApplyForceMovesBody) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    b2Vec2 fuerza = {100.0f, 0.0f};
    b2Body_ApplyForceToCenter(body, fuerza, true);

    b2World_Step(mundo, 0.016666f, 10);
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    EXPECT_GT(vel.x, 0.0f);
    
    b2DestroyWorld(mundo);
}

// Verifica que el constructor no lance excepciones
TEST(GameLoopTest, GameLoopConstruction) {
    Configuracion config("config.yaml");
    Monitor monitor;
    Queue<struct Command> game_queue;
    
    ASSERT_NO_THROW({
        GameLoop game(monitor, game_queue, config);
    });
}

// Verifica que se puedan crear múltiples cuerpos sin problemas
TEST(PhysicsTest, MultipleBodiesCreation) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    std::vector<b2BodyId> bodies;
    for (int i = 0; i < 5; ++i) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {static_cast<float>(i * 10), 0.0f};
        
        b2BodyId body = b2CreateBody(mundo, &bodyDef);
        bodies.push_back(body);
        
        EXPECT_TRUE(B2_IS_NON_NULL(body));
    }
    
    EXPECT_EQ(bodies.size(), 5);
    b2DestroyWorld(mundo);
}

// Test integración: GameLoop procesa comandos correctamente
TEST(GameLoopTest, ProcessCommands) {
    Configuracion config("config.yaml");
    Monitor monitor;
    Queue<struct Command> game_queue;
    
    GameLoop game(monitor, game_queue, config);
    
    Command cmd;
    cmd.player_id = 1;
    cmd.action = GameCommand::ACCELERATE;
    
    game_queue.push(cmd);
    
    SUCCEED();
}
