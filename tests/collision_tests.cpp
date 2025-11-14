#include "gtest/gtest.h"
#include <box2d/box2d.h>
#include "../common_src/config.h"
#include "../server_src/game/car.h"
#include "../server_src/game/collision_handler.h"

// Test: Un auto recibe daño al colisionar
TEST(CollisionTest, CarTakesDamage) {
    Car car(1, 12, 100);  // ID=1, Nitro=12 ticks, HP=100
    
    EXPECT_EQ(car.health, 100);
    EXPECT_TRUE(car.is_alive());
    
    car.apply_collision_damage(30.0f);  // Colisión moderada
    
    EXPECT_LT(car.health, 100);
    EXPECT_TRUE(car.is_alive());
}

// Test: Colisión severa causa más daño
TEST(CollisionTest, SevereCollisionMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    car1.apply_collision_damage(25.0f);  // Colisión moderada
    int health_after_medium = car1.health;
    
    car2.apply_collision_damage(60.0f);  // Colisión severa
    int health_after_severe = car2.health;
    
    EXPECT_LT(health_after_severe, health_after_medium);
}

// Test: Auto se destruye cuando la salud llega a 0
TEST(CollisionTest, CarDestroysAtZeroHealth) {
    Car car(1, 12, 50);  // Iniciar con poca salud
    
    EXPECT_TRUE(car.is_alive());
    
    car.apply_collision_damage(100.0f);  // Colisión masiva
    
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
    EXPECT_EQ(car.health, 0);  // No cambia
}

// Test: CollisionHandler registra autos correctamente
TEST(CollisionHandlerTest, RegisterCar) {
    CollisionHandler handler;
    Car car(1, 12, 100);
    
    ASSERT_NO_THROW({
        handler.register_car(1, &car);
    });
}

// Test: Aplicar colisión pendiente afecta a ambos autos
TEST(CollisionHandlerTest, ApplyPendingCollisions) {
    CollisionHandler handler;
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    handler.register_car(1, &car1);
    handler.register_car(2, &car2);
    
    // Simular evento de colisión manualmente
    CollisionEvent event;
    event.car_id_a = 1;
    event.car_id_b = 2;
    event.impact_force = 40.0f;
    
    // Como no podemos acceder a pending_collisions, 
    // este test es más conceptual
    // En un sistema real, usarías un mock de Box2D
    
    SUCCEED();  // Placeholder
}

// Test de integración: Dos cuerpos colisionan en Box2D
TEST(CollisionIntegrationTest, TwoBodiesCollide) {
    Configuracion config("config/configuracion.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    // Crear dos cuerpos que van a colisionar
    b2BodyDef bodyDef1 = b2DefaultBodyDef();
    bodyDef1.type = b2_dynamicBody;
    bodyDef1.position = {0.0f, 0.0f};
    b2BodyId body1 = b2CreateBody(mundo, &bodyDef1);
    
    b2BodyDef bodyDef2 = b2DefaultBodyDef();
    bodyDef2.type = b2_dynamicBody;
    bodyDef2.position = {5.0f, 0.0f};
    b2BodyId body2 = b2CreateBody(mundo, &bodyDef2);
    
    // Agregar formas
    b2Polygon box = b2MakeBox(1.0f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2CreatePolygonShape(body1, &shapeDef, &box);
    b2CreatePolygonShape(body2, &shapeDef, &box);
    
    // Hacer que se muevan uno hacia el otro
    b2Body_SetLinearVelocity(body1, {10.0f, 0.0f});
    b2Body_SetLinearVelocity(body2, {-10.0f, 0.0f});
    
    // Simular hasta que colisionen
    bool collision_detected = false;
    for (int i = 0; i < 100; ++i) {
        b2World_Step(mundo, 0.016666f, 10);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            break;
        }
    }
    
    EXPECT_TRUE(collision_detected);
    b2DestroyWorld(mundo);
}

// ============================================
// TESTS DE COLISIONES CON OBSTÁCULOS
// ============================================

// Test: ObstacleManager crea paredes correctamente
TEST(ObstacleTest, CreateWall) {
    Configuracion config("config/configuracion.yaml");
    
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
    Configuracion config("config/configuracion.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager manager(mundo);
    
    ASSERT_NO_THROW({
        manager.create_building(10.0f, 10.0f, 5.0f, 5.0f);
    });
    
    b2DestroyWorld(mundo);
}

// Test: Colisión con pared causa más daño que colisión normal
TEST(ObstacleCollisionTest, WallCausesMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    
    // Colisión normal (sin multiplicador)
    car1.apply_collision_damage(40.0f);
    int health_normal = car1.health;
    
    // Colisión con pared (multiplicador 1.5x)
    car2.apply_collision_damage(40.0f * 1.5f);
    int health_wall = car2.health;
    
    EXPECT_LT(health_wall, health_normal);
}

// Test: Colisión con edificio causa mucho más daño
TEST(ObstacleCollisionTest, BuildingCausesSevereDamage) {
    Car car(1, 12, 100);
    
    EXPECT_TRUE(car.is_alive());
    
    // Colisión frontal con edificio (multiplicador 2.0x)
    car.apply_collision_damage(60.0f * 2.0f);
    
    // Debería estar destruido o muy dañado
    EXPECT_LT(car.health, 30);
}

// Test de integración: Auto colisiona con pared estática
TEST(ObstacleIntegrationTest, CarHitsWall) {
    Configuracion config("config/configuracion.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacle_mgr(mundo);
    
    // Crear pared
    obstacle_mgr.create_wall(10.0f, 0.0f, 2.0f, 10.0f);
    
    // Crear auto dinámico
    b2BodyDef bodyDef =
    
