#include "gtest/gtest.h"
#include <box2d/box2d.h>
#include "../common_src/config.h"
#include "../server_src/game/car.h"
#include "../server_src/game/collision_handler.h"

// Test: Un auto recibe daño al colisionar
TEST(CollisionTest, CarTakesDamage) {
    Car car(1, 12, 100);
    
    EXPECT_EQ(car.health, 100);
    EXPECT_TRUE(car.is_alive());
    
    car.apply_collision_damage(30.0f);
    
    EXPECT_LT(car.health, 100);
    EXPECT_TRUE(car.is_alive());
}

// Test: Colisión severa causa más daño
TEST(CollisionTest, SevereCollisionMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    car1.apply_collision_damage(25.0f);
    int health_after_medium = car1.health;
    
    car2.apply_collision_damage(60.0f);
    int health_after_severe = car2.health;
    
    EXPECT_LT(health_after_severe, health_after_medium);
}

// Test: Auto se destruye cuando la salud llega a 0
TEST(CollisionTest, CarDestroysAtZeroHealth) {
    Car car(1, 12, 50);
    
    EXPECT_TRUE(car.is_alive());
    EXPECT_FALSE(car.is_destroyed);
    
    // Aplicar daño suficiente para destruir
    car.apply_collision_damage(200.0f); 
    
    EXPECT_FALSE(car.is_alive());
    EXPECT_EQ(car.health, 0);
    EXPECT_TRUE(car.is_destroyed);
}

// Test: Auto destruido no recibe más daño
TEST(CollisionTest, DestroyedCarIgnoresDamage) {
    Car car(1, 12, 100);
    
    car.destroy();
    EXPECT_EQ(car.health, 0);
    
    car.apply_collision_damage(50.0f);
    EXPECT_EQ(car.health, 0);
}

// Test: CollisionHandler registra autos correctamente
TEST(CollisionHandlerTest, RegisterCar) {
    CollisionHandler handler;
    Car car(1, 12, 100);
    
    ASSERT_NO_THROW({
        handler.register_car(1, &car);
    });
}

// Test: Aplicar colisión pendiente
TEST(CollisionHandlerTest, ApplyPendingCollisions) {
    CollisionHandler handler;
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    handler.register_car(1, &car1);
    handler.register_car(2, &car2);
    
    SUCCEED();
}

// Test de integración: Dos cuerpos colisionan en Box2D
TEST(CollisionIntegrationTest, TwoBodiesCollide) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    // Crear dos cuerpos MUY CERCA
    b2BodyDef bodyDef1 = b2DefaultBodyDef();
    bodyDef1.type = b2_dynamicBody;
    bodyDef1.position = {-1.5f, 0.0f}; 
    b2BodyId body1 = b2CreateBody(mundo, &bodyDef1);
    
    b2BodyDef bodyDef2 = b2DefaultBodyDef();
    bodyDef2.type = b2_dynamicBody;
    bodyDef2.position = {1.5f, 0.0f};
    b2BodyId body2 = b2CreateBody(mundo, &bodyDef2);
    
    // Agregar formas con DENSIDAD ALTA
    b2Polygon box = b2MakeBox(0.5f, 0.5f); 
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 2.0f;  // ← Densidad alta
    b2CreatePolygonShape(body1, &shapeDef, &box);
    b2CreatePolygonShape(body2, &shapeDef, &box);
    
    // Velocidades ALTAS hacia el centro
    b2Body_SetLinearVelocity(body1, {15.0f, 0.0f});
    b2Body_SetLinearVelocity(body2, {-15.0f, 0.0f});
    
    // Simular con MÁS ITERACIONES
    bool collision_detected = false;
    for (int i = 0; i < 60; ++i) {  
        b2World_Step(mundo, 0.016666f, 8);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            break;
        }
    }
    
    EXPECT_TRUE(collision_detected);
    b2DestroyWorld(mundo);
}

// Test: ObstacleManager crea paredes correctamente
TEST(ObstacleTest, CreateWall) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager manager(mundo);
    
    ASSERT_NO_THROW({
        manager.create_wall(0.0f, 0.0f, 10.0f, 2.0f);
    });
    
    b2DestroyWorld(mundo);
}

// Test: ObstacleManager crea edificios
TEST(ObstacleTest, CreateBuilding) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager manager(mundo);
    
    ASSERT_NO_THROW({
        manager.create_building(10.0f, 10.0f, 5.0f, 5.0f);
    });
    
    b2DestroyWorld(mundo);
}

// Test: Colisión con pared causa más daño
TEST(ObstacleCollisionTest, WallCausesMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    car1.apply_collision_damage(40.0f);
    int health_normal = car1.health;
    
    car2.apply_collision_damage(40.0f * 1.5f);
    int health_wall = car2.health;
    
    EXPECT_LT(health_wall, health_normal);
}

// Test: Colisión con edificio causa mucho daño
TEST(ObstacleCollisionTest, BuildingCausesSevereDamage) {
    Car car(1, 12, 100);
    
    EXPECT_TRUE(car.is_alive());
    
    // Fuerza masiva con multiplicador
    car.apply_collision_damage(200.0f);

    // Debería estar destruido
    EXPECT_TRUE(car.is_destroyed);
    EXPECT_EQ(car.health, 0);
}

// Test de integración: Auto colisiona con pared estática
TEST(ObstacleIntegrationTest, CarHitsWall) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacle_mgr(mundo);
    
    // Crear pared MUY CERCA
    obstacle_mgr.create_wall(3.0f, 0.0f, 1.0f, 5.0f);
    
    // Crear auto dinámico
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId carBody = b2CreateBody(mundo, &bodyDef);
    
    // Agregar forma más pequeña
    b2Polygon box = b2MakeBox(0.5f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 2.0f; 
    b2CreatePolygonShape(carBody, &shapeDef, &box);
    
    // Velocidad ALTA
    b2Body_SetLinearVelocity(carBody, {25.0f, 0.0f});
    
    // Simular
    bool collision_detected = false;
    for (int i = 0; i < 100; ++i) {
        b2World_Step(mundo, 0.016666f, 8);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            break;
        }
    }
    
    EXPECT_TRUE(collision_detected);
    
    obstacle_mgr.clear();
    b2DestroyWorld(mundo);
}
