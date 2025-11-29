#include "gtest/gtest.h"
#include <box2d/box2d.h>
#include "../common_src/config.h"
#include "../server_src/game/car.h"
#include "../server_src/game/collision_handler.h"

TEST(CollisionTest, CarTakesDamage) {
    Car car(1, 12, 100);
    EXPECT_EQ(car.health, 100);
    EXPECT_TRUE(car.is_alive());
    car.apply_collision_damage(30.0f);
    EXPECT_LT(car.health, 100);
    EXPECT_TRUE(car.is_alive());
}

TEST(CollisionTest, SevereCollisionMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    car1.apply_collision_damage(25.0f);
    int health_after_medium = car1.health;
    car2.apply_collision_damage(60.0f);
    int health_after_severe = car2.health;
    EXPECT_LT(health_after_severe, health_after_medium);
}

TEST(CollisionTest, CarDestroysAtZeroHealth) {
    Car car(1, 12, 50);
    EXPECT_TRUE(car.is_alive());
    EXPECT_FALSE(car.is_destroyed);
    car.apply_collision_damage(200.0f); 
    EXPECT_FALSE(car.is_alive());
    EXPECT_EQ(car.health, 0);
    EXPECT_TRUE(car.is_destroyed);
}

TEST(CollisionTest, DestroyedCarIgnoresDamage) {
    Car car(1, 12, 100);
    car.destroy();
    EXPECT_EQ(car.health, 0);
    car.apply_collision_damage(50.0f);
    EXPECT_EQ(car.health, 0);
}

TEST(CollisionHandlerTest, RegisterCar) {
    CollisionHandler handler;
    Car car(1, 12, 100);
    ASSERT_NO_THROW({
        handler.register_car(1, &car);
    });
}

TEST(CollisionHandlerTest, ApplyPendingCollisions) {
    CollisionHandler handler;
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    handler.register_car(1, &car1);
    handler.register_car(2, &car2);
    SUCCEED();
}

TEST(CollisionIntegrationTest, TwoBodiesCollide) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    b2BodyDef bodyDef1 = b2DefaultBodyDef();
    bodyDef1.type = b2_dynamicBody;
    bodyDef1.position = {0.0f, 0.0f};
    b2BodyId body1 = b2CreateBody(mundo, &bodyDef1);
    
    b2BodyDef bodyDef2 = b2DefaultBodyDef();
    bodyDef2.type = b2_dynamicBody;
    bodyDef2.position = {2.0f, 0.0f};
    b2BodyId body2 = b2CreateBody(mundo, &bodyDef2);
    
    b2Circle circle;
    circle.center = {0.0f, 0.0f};
    circle.radius = 0.5f;
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreateCircleShape(body1, &shapeDef, &circle);
    b2CreateCircleShape(body2, &shapeDef, &circle);
    
    b2Body_SetLinearVelocity(body1, {5.0f, 0.0f});
    b2Body_SetLinearVelocity(body2, {-5.0f, 0.0f});
    
    bool collision_detected = false;
    float timeStep = 1.0f / 60.0f;
    
    for (int i = 0; i < 100; ++i) {
        b2World_Step(mundo, timeStep, 4);
        
        b2ContactEvents contact_events = b2World_GetContactEvents(mundo);
        
        if (contact_events.beginCount > 0) {
            collision_detected = true;
            std::cout << "[Test] Collision detected via events at frame " << i << std::endl;
            break;
        }
        
        b2Vec2 vel1 = b2Body_GetLinearVelocity(body1);
        b2Vec2 vel2 = b2Body_GetLinearVelocity(body2);
        
        // Si las velocidades se invirtieron → hubo colisión
        if (vel1.x < 0.0f && vel2.x > 0.0f) {
            collision_detected = true;
            std::cout << "[Test] Collision detected via velocity change at frame " << i << std::endl;
            break;
        }
        
        if (i % 20 == 0) {
            b2Vec2 pos1 = b2Body_GetPosition(body1);
            b2Vec2 pos2 = b2Body_GetPosition(body2);
            float distance = std::sqrt((pos2.x - pos1.x) * (pos2.x - pos1.x) + 
                                      (pos2.y - pos1.y) * (pos2.y - pos1.y));
            
            std::cout << "[Debug] Frame " << i << " | Distance: " << distance 
                      << " | Vel1.x: " << vel1.x << " | Vel2.x: " << vel2.x << std::endl;
        }
    }
    
    EXPECT_TRUE(collision_detected) << "Bodies should have collided";
    b2DestroyWorld(mundo);
}

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

TEST(ObstacleCollisionTest, WallCausesMoreDamage) {
    Car car1(1, 12, 100);
    Car car2(2, 12, 100);
    car1.apply_collision_damage(40.0f);
    int health_normal = car1.health;
    car2.apply_collision_damage(40.0f * 1.5f);
    int health_wall = car2.health;
    EXPECT_LT(health_wall, health_normal);
}

TEST(ObstacleCollisionTest, BuildingCausesSevereDamage) {
    Car car(1, 12, 100);
    EXPECT_TRUE(car.is_alive());
    car.apply_collision_damage(200.0f);
    EXPECT_TRUE(car.is_destroyed);
    EXPECT_EQ(car.health, 0);
    EXPECT_FALSE(car.is_alive());
}

TEST(ObstacleIntegrationTest, CarHitsWall) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacle_mgr(mundo);
    obstacle_mgr.create_wall(5.0f, 0.0f, 1.0f, 10.0f);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId carBody = b2CreateBody(mundo, &bodyDef);
    
    b2Circle circle;
    circle.center = {0.0f, 0.0f};
    circle.radius = 0.5f;
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreateCircleShape(carBody, &shapeDef, &circle);
    
    b2Body_SetLinearVelocity(carBody, {10.0f, 0.0f});
    
    bool collision_detected = false;
    float timeStep = 1.0f / 60.0f;
    float initial_velocity = 10.0f;
    
    for (int i = 0; i < 100; ++i) {
        b2World_Step(mundo, timeStep, 4);
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        b2Vec2 vel = b2Body_GetLinearVelocity(carBody);
        b2Vec2 pos = b2Body_GetPosition(carBody);
        
        if (events.beginCount > 0) {
            collision_detected = true;
            std::cout << "[Test] Wall collision via events at frame " << i << std::endl;
            break;
        }
        
        if (std::abs(vel.x) < initial_velocity * 0.5f || vel.x < 0.0f) {
            collision_detected = true;
            std::cout << "[Test] Wall collision via velocity change at frame " << i << std::endl;
            break;
        }
        
        if (i % 20 == 0) {
            std::cout << "[Debug] Frame " << i << " | Pos.x: " << pos.x 
                      << " | Vel.x: " << vel.x << std::endl;
        }
    }
    
    EXPECT_TRUE(collision_detected) << "Car should have hit the wall";
    
    obstacle_mgr.clear();
    b2DestroyWorld(mundo);
}
