#include <gtest/gtest.h>
#include <box2d/box2d.h>
#include "../server_src/game/car.h"
#include "../server_src/game/collision_handler.h"
#include "../server_src/game/obstacle.h"
#include "../common_src/config.h" 

b2BodyId create_test_car_body(b2WorldId world, float x, float y) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {x, y};
    
    b2BodyId bodyId = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(2.0f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    
    return bodyId;
}

class CollisionTest : public ::testing::Test {
protected:
    b2WorldId worldId;

    void SetUp() override {
        // Crear mundo sin gravedad para los tests
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

TEST_F(CollisionTest, CarTakesDamage) {
    // 1. Crear cuerpo y auto
    b2BodyId body = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car("TestCar", "Sport", body);
    
    // Cargar stats: max_speed, accel, handling, durability(health), nitro, weight
    car.load_stats(100.0f, 10.0f, 1.0f, 100.0f, 1.5f, 1000.0f);

    EXPECT_EQ(car.getHealth(), 100);

    // 2. Aplicar daño
    car.takeDamage(10);
    EXPECT_EQ(car.getHealth(), 90);
    
    car.takeDamage(5);
    EXPECT_EQ(car.getHealth(), 85);
}

TEST_F(CollisionTest, SevereCollisionMoreDamage) {
    b2BodyId body1 = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car1("Car1", "Sport", body1);
    car1.load_stats(100.0f, 10.0f, 1.0f, 100.0f, 1.5f, 1000.0f);

    b2BodyId body2 = create_test_car_body(worldId, 10.0f, 0.0f);
    Car car2("Car2", "Sport", body2);
    car2.load_stats(100.0f, 10.0f, 1.0f, 100.0f, 1.5f, 1000.0f);

    // Simular daños distintos
    car1.takeDamage(20); // Choque medio
    car2.takeDamage(50); // Choque fuerte

    EXPECT_LT(car2.getHealth(), car1.getHealth());
    EXPECT_EQ(car1.getHealth(), 80);
    EXPECT_EQ(car2.getHealth(), 50);
}

TEST_F(CollisionTest, CarDestroysAtZeroHealth) {
    b2BodyId body = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car("TestCar", "Sport", body);
    car.load_stats(100.0f, 10.0f, 1.0f, 50.0f, 1.5f, 1000.0f); // 50 HP inicial

    EXPECT_FALSE(car.isDestroyed());

    // Daño letal
    car.takeDamage(50);
    
    EXPECT_EQ(car.getHealth(), 0);
    EXPECT_TRUE(car.isDestroyed());
}

TEST_F(CollisionTest, DestroyedCarIgnoresDamage) {
    b2BodyId body = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car("TestCar", "Sport", body);
    car.load_stats(100.0f, 10.0f, 1.0f, 100.0f, 1.5f, 1000.0f);

    // Destruir manualmente (simulando daño masivo)
    car.takeDamage(100);
    EXPECT_TRUE(car.isDestroyed());
    EXPECT_EQ(car.getHealth(), 0);

    // Intentar dañar más
    car.takeDamage(10);
    EXPECT_EQ(car.getHealth(), 0); // No baja de 0
}

TEST_F(CollisionTest, CollisionHandlerRegistersCars) {
    CollisionHandler handler;
    b2BodyId body = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car("TestCar", "Sport", body);

    // Simplemente verificar que no explota al registrar
    EXPECT_NO_THROW(handler.register_car(1, &car));
    EXPECT_NO_THROW(handler.unregister_car(1));
}

TEST_F(CollisionTest, ObstacleCreationTest) {
    ObstacleManager manager;
    
    // Ahora pasamos 'worldId' como primer argumento
    EXPECT_NO_THROW(manager.create_wall(worldId, 0, 0, 10, 2));
    EXPECT_NO_THROW(manager.create_building(worldId, 20, 20, 5, 5));
    EXPECT_NO_THROW(manager.create_barrier(worldId, 30, 30, 2));
}

TEST_F(CollisionTest, ObstacleCollisionDamage) {
    // Configurar Obstacle Manager y Handler
    ObstacleManager obsManager;
    CollisionHandler handler;
    handler.set_obstacle_manager(&obsManager);

    // Crear pared
    obsManager.create_wall(worldId, 5.0f, 0.0f, 1.0f, 10.0f); 

    // Crear auto y registrarlo
    b2BodyId carBody = create_test_car_body(worldId, 0.0f, 0.0f);
    Car car("TestCar", "Sport", carBody);
    car.load_stats(100.0f, 10.0f, 1.0f, 100.0f, 1.5f, 1000.0f);
    
    // Identificar el auto para la colisión (UserData)
    int playerId = 1;
    handler.register_car(playerId, &car);

    car.apply_collision_damage(10.0f * 1.0f); 
    EXPECT_EQ(car.getHealth(), 90);

    car.repair(100); // Reset salud
    car.apply_collision_damage(10.0f * 2.0f); 
    EXPECT_EQ(car.getHealth(), 80);
}
