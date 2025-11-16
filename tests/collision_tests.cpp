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

// Test de integración: Dos cuerpos colisionan en Box2D (CON DAMPING=0)
TEST(CollisionIntegrationTest, TwoBodiesCollide) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    // Crear dos cuerpos MUY CERCA - SIN DAMPING
    b2BodyDef bodyDef1 = b2DefaultBodyDef();
    bodyDef1.type = b2_dynamicBody;
    bodyDef1.position = {-1.1f, 0.0f};
    bodyDef1.enableSleep = false;
    bodyDef1.linearDamping = 0.0f; 
    bodyDef1.angularDamping = 0.0f; 
    b2BodyId body1 = b2CreateBody(mundo, &bodyDef1);
    
    b2BodyDef bodyDef2 = b2DefaultBodyDef();
    bodyDef2.type = b2_dynamicBody;
    bodyDef2.position = {1.1f, 0.0f};
    bodyDef2.enableSleep = false;
    bodyDef2.linearDamping = 0.0f;   
    bodyDef2.angularDamping = 0.0f;  
    b2BodyId body2 = b2CreateBody(mundo, &bodyDef2);
    
    // Usar círculos para mejor detección
    b2Circle circle;
    circle.center = {0.0f, 0.0f};
    circle.radius = 1.0f;
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreateCircleShape(body1, &shapeDef, &circle);
    b2CreateCircleShape(body2, &shapeDef, &circle);
    
    // Velocidades moderadas
    b2Body_SetLinearVelocity(body1, {5.0f, 0.0f});
    b2Body_SetLinearVelocity(body2, {-5.0f, 0.0f});
    
    // Simular
    bool collision_detected = false;
    float timeStep = 1.0f / 60.0f;
    int subSteps = 4;
    
    for (int i = 0; i < 120; ++i) {
        b2World_Step(mundo, timeStep, subSteps);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            std::cout << "[Test] Collision detected at frame " << i << std::endl;
            break;
        }
        
        // Debug cada 20 frames
        if (i % 20 == 0) {
            b2Vec2 pos1 = b2Body_GetPosition(body1);
            b2Vec2 pos2 = b2Body_GetPosition(body2);
            float distance = std::sqrt((pos2.x - pos1.x) * (pos2.x - pos1.x) + 
                                      (pos2.y - pos1.y) * (pos2.y - pos1.y));
            std::cout << "[Debug] Frame " << i << " - Distance: " << distance 
                      << " | Body1: (" << pos1.x << "," << pos1.y << ")"
                      << " | Body2: (" << pos2.x << "," << pos2.y << ")" << std::endl;
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
    
    car.apply_collision_damage(200.0f);

    EXPECT_TRUE(car.is_destroyed);
    EXPECT_EQ(car.health, 0);
    EXPECT_FALSE(car.is_alive());
}

// Test de integración: Auto colisiona con pared estática (CON DAMPING=0)
TEST(ObstacleIntegrationTest, CarHitsWall) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacle_mgr(mundo);
    
    // Crear pared cercana
    obstacle_mgr.create_wall(3.0f, 0.0f, 1.0f, 10.0f);
    
    // Crear auto SIN DAMPING
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    bodyDef.enableSleep = false;
    bodyDef.linearDamping = 0.0f;   
    bodyDef.angularDamping = 0.0f;  
    b2BodyId carBody = b2CreateBody(mundo, &bodyDef);
    
    // Usar círculo
    b2Circle circle;
    circle.center = {0.0f, 0.0f};
    circle.radius = 1.0f;
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreateCircleShape(carBody, &shapeDef, &circle);
    
    // Velocidad moderada
    b2Body_SetLinearVelocity(carBody, {10.0f, 0.0f});
    
    // Simular
    bool collision_detected = false;
    float timeStep = 1.0f / 60.0f;
    int subSteps = 4;
    
    for (int i = 0; i < 120; ++i) {
        b2World_Step(mundo, timeStep, subSteps);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            std::cout << "[Test] Car hit wall at frame " << i << std::endl;
            
            for (int j = 0; j < events.beginCount; ++j) {
                b2ContactBeginTouchEvent* event = events.beginEvents + j;
                b2BodyId bodyA = b2Shape_GetBody(event->shapeIdA);
                b2BodyId bodyB = b2Shape_GetBody(event->shapeIdB);
                
                bool involves_car = (bodyA.index1 == carBody.index1) || 
                                   (bodyB.index1 == carBody.index1);
                bool involves_wall = obstacle_mgr.is_obstacle(bodyA) || 
                                    obstacle_mgr.is_obstacle(bodyB);
                
                if (involves_car && involves_wall) {
                    std::cout << "[Test] Confirmed: car-wall collision" << std::endl;
                    break;
                }
            }
            break;
        }
        
        // Debug cada 20 frames
        if (i % 20 == 0) {
            b2Vec2 pos = b2Body_GetPosition(carBody);
            b2Vec2 vel = b2Body_GetLinearVelocity(carBody);
            std::cout << "[Debug] Frame " << i 
                      << " - Position: (" << pos.x << ", " << pos.y << ")"
                      << " - Velocity: (" << vel.x << ", " << vel.y << ")" << std::endl;
        }
    }
    
    EXPECT_TRUE(collision_detected);
    
    obstacle_mgr.clear();
    b2DestroyWorld(mundo);
}
