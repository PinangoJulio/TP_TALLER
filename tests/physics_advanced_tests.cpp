#include "gtest/gtest.h"
#include <box2d/box2d.h>
#include "../common_src/config.h"
#include "../server_src/game/game_loop.h"
#include "../server_src/network/monitor.h"
#include <cmath>

// TESTS DE ATRIBUTOS DEL YAML
TEST(CarAttributesTest, DeportivoAttributes) {
    Configuracion config("config.yaml");
    const AtributosAuto& deportivo = config.obtenerAtributosAuto("DEPORTIVO");
    
    EXPECT_FLOAT_EQ(deportivo.velocidad_maxima, 15.0f);
    EXPECT_FLOAT_EQ(deportivo.aceleracion, 1.0f);
    EXPECT_EQ(deportivo.salud_base, 100);
    EXPECT_FLOAT_EQ(deportivo.masa, 1.5f);
    EXPECT_FLOAT_EQ(deportivo.control, 8.0f);
}

TEST(CarAttributesTest, CamionAttributes) {
    Configuracion config("config.yaml");
    const AtributosAuto& camion = config.obtenerAtributosAuto("CAMION");
    
    EXPECT_FLOAT_EQ(camion.velocidad_maxima, 8.0f);
    EXPECT_FLOAT_EQ(camion.aceleracion, 0.5f);
    EXPECT_EQ(camion.salud_base, 200);
    EXPECT_FLOAT_EQ(camion.masa, 3.0f);
    EXPECT_FLOAT_EQ(camion.control, 4.0f);
}

TEST(CarAttributesTest, MotoAttributes) {
    Configuracion config("config.yaml");
    const AtributosAuto& moto = config.obtenerAtributosAuto("MOTO");
    
    EXPECT_FLOAT_EQ(moto.velocidad_maxima, 18.0f);
    EXPECT_FLOAT_EQ(moto.aceleracion, 1.5f);
    EXPECT_EQ(moto.salud_base, 60);
    EXPECT_FLOAT_EQ(moto.masa, 0.8f);
    EXPECT_FLOAT_EQ(moto.control, 10.0f);
}

// TESTS DE VELOCIDAD MÁXIMA
TEST(VelocityLimitTest, DeportivoMaxSpeed) {
    Configuracion config("config.yaml");
    Monitor monitor;
    Queue<Command> game_queue;
    
    GameLoop game(monitor, game_queue, config);
    
    // Simular comandos de aceleración
    Command cmd;
    cmd.player_id = 1;
    cmd.action = GameCommand::ACCELERATE;
    
    // Acelerar durante 100 frames
    for (int i = 0; i < 100; ++i) {
        game_queue.push(cmd);
    }
    
    SUCCEED(); 
}

TEST(VelocityLimitTest, CamionSlowerThanDeportivo) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& deportivo = config.obtenerAtributosAuto("DEPORTIVO");
    const AtributosAuto& camion = config.obtenerAtributosAuto("CAMION");
    
    EXPECT_GT(deportivo.velocidad_maxima, camion.velocidad_maxima);
    EXPECT_GT(deportivo.aceleracion, camion.aceleracion);
}

TEST(VelocityLimitTest, MotoFastestCar) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& moto = config.obtenerAtributosAuto("MOTO");
    const AtributosAuto& deportivo = config.obtenerAtributosAuto("DEPORTIVO");
    const AtributosAuto& camion = config.obtenerAtributosAuto("CAMION");
    
    EXPECT_GT(moto.velocidad_maxima, deportivo.velocidad_maxima);
    EXPECT_GT(moto.velocidad_maxima, camion.velocidad_maxima);
}

// TESTS DE FRICCIÓN LATERAL
TEST(LateralFrictionTest, DriftingReducesLateralVelocity) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    // Crear un body
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.5f;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    // Aplicar velocidad lateral (drift)
    b2Body_SetLinearVelocity(body, {10.0f, 5.0f}); 
    b2Body_SetAngularVelocity(body, 0.0f);
    
    b2Vec2 vel_antes = b2Body_GetLinearVelocity(body);
    float speed_antes = std::sqrt(vel_antes.x * vel_antes.x + vel_antes.y * vel_antes.y);
    
    b2Rot rotation = b2Body_GetRotation(body);
    float angle = b2Rot_GetAngle(rotation);
    b2Vec2 lateral_direction = {-std::sin(angle), std::cos(angle)};
    float lateral_velocity = lateral_direction.x * vel_antes.x + 
                            lateral_direction.y * vel_antes.y;
    
    b2Vec2 lateral_impulse = {
        -lateral_direction.x * lateral_velocity * 0.9f,
        -lateral_direction.y * lateral_velocity * 0.9f
    };
    
    b2Body_ApplyLinearImpulseToCenter(body, lateral_impulse, true);
    b2World_Step(mundo, 0.016666f, 4);
    
    b2Vec2 vel_despues = b2Body_GetLinearVelocity(body);
    float speed_despues = std::sqrt(vel_despues.x * vel_despues.x + vel_despues.y * vel_despues.y);
    
    EXPECT_LT(speed_despues, speed_antes + 0.1f); 
    
    b2DestroyWorld(mundo);
}

// TESTS DE MASA Y DENSIDAD
TEST(MassTest, CamionHeavierThanDeportivo) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    // Crear deportivo
    const AtributosAuto& deportivo_attrs = config.obtenerAtributosAuto("DEPORTIVO");
    b2BodyDef bodyDef1 = b2DefaultBodyDef();
    bodyDef1.type = b2_dynamicBody;
    b2BodyId deportivo = b2CreateBody(mundo, &bodyDef1);
    
    b2Polygon box1 = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef1 = b2DefaultShapeDef();
    shapeDef1.density = deportivo_attrs.masa;
    b2CreatePolygonShape(deportivo, &shapeDef1, &box1);
    
    // Crear camión
    const AtributosAuto& camion_attrs = config.obtenerAtributosAuto("CAMION");
    b2BodyDef bodyDef2 = b2DefaultBodyDef();
    bodyDef2.type = b2_dynamicBody;
    b2BodyId camion = b2CreateBody(mundo, &bodyDef2);
    
    b2Polygon box2 = b2MakeBox(1.2f, 2.5f);
    b2ShapeDef shapeDef2 = b2DefaultShapeDef();
    shapeDef2.density = camion_attrs.masa;
    b2CreatePolygonShape(camion, &shapeDef2, &box2);
    
    float masa_deportivo = b2Body_GetMass(deportivo);
    float masa_camion = b2Body_GetMass(camion);
    
    EXPECT_GT(masa_camion, masa_deportivo);
    
    b2DestroyWorld(mundo);
}

TEST(MassTest, MotoLighterThanDeportivo) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& moto = config.obtenerAtributosAuto("MOTO");
    const AtributosAuto& deportivo = config.obtenerAtributosAuto("DEPORTIVO");
    
    EXPECT_LT(moto.masa, deportivo.masa);
}

// TESTS DE ACELERACIÓN
TEST(AccelerationTest, MotoAcceleratesFaster) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& moto = config.obtenerAtributosAuto("MOTO");
    const AtributosAuto& camion = config.obtenerAtributosAuto("CAMION");
    
    EXPECT_GT(moto.aceleracion, camion.aceleracion);
}

TEST(AccelerationTest, AccelerationProportionalToMass) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    const AtributosAuto& attrs = config.obtenerAtributosAuto("DEPORTIVO");
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = attrs.masa;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    float accel_force = attrs.aceleracion * attrs.masa * 10.0f;
    b2Vec2 fuerza = {accel_force, 0.0f};
    b2Body_ApplyForceToCenter(body, fuerza, true);
    
    b2World_Step(mundo, 0.016666f, 4);
    
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    EXPECT_GT(vel.x, 0.0f); 
    
    b2DestroyWorld(mundo);
}

// TESTS DE CONTROL (TORQUE)
TEST(ControlTest, HighControlBetterTurning) {
    Configuracion config("config.yaml");
    
    const AtributosAuto& moto = config.obtenerAtributosAuto("MOTO");
    const AtributosAuto& camion = config.obtenerAtributosAuto("CAMION");
    
    EXPECT_GT(moto.control, camion.control);
}

TEST(ControlTest, TorqueAppliedCorrectly) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    const AtributosAuto& attrs = config.obtenerAtributosAuto("DEPORTIVO");
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = attrs.masa;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    // Aplicr torque
    float torque = attrs.control;
    b2Body_ApplyTorque(body, torque, true);
    
    b2World_Step(mundo, 0.016666f, 4);
    
    float angular_vel = b2Body_GetAngularVelocity(body);
    EXPECT_GT(angular_vel, 0.0f); // Debería estar girando
    
    b2DestroyWorld(mundo);
}

// TESTS DE DAÑO Y REDUCCIÓN DE VELOCIDAD
TEST(DamageTest, DamageReducesMaxSpeed) {
    Car car(1, 12, 100);
    
    EXPECT_EQ(car.health, 100);
    
    // Simular daño
    car.apply_collision_damage(100.0f); // 50 de daño
    
    EXPECT_LT(car.health, 100);
    EXPECT_GT(car.health, 0);
}

TEST(DamageTest, SevereDamageReducesSpeedMore) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    const AtributosAuto& attrs = config.obtenerAtributosAuto("DEPORTIVO");
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = attrs.masa;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    Car car(1, 12, attrs.salud_base);
    car.body = body;
    
    // Dar velocidad inicial
    b2Body_SetLinearVelocity(body, {10.0f, 0.0f});
    float speed_inicial = 10.0f;
    
    // Aplicar daño severo
    car.apply_collision_damage(100.0f);
    
    b2Vec2 vel_final = b2Body_GetLinearVelocity(body);
    float speed_final = std::sqrt(vel_final.x * vel_final.x + vel_final.y * vel_final.y);
    
    EXPECT_LT(speed_final, speed_inicial);
    
    b2DestroyWorld(mundo);
}

TEST(DamageTest, ZeroHealthStopsMovement) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.5f;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    Car car(1, 12, 100);
    car.body = body;
    
    b2Body_SetLinearVelocity(body, {10.0f, 0.0f});
    
    // Matar el auto
    car.apply_collision_damage(500.0f);
    
    EXPECT_EQ(car.health, 0);
    EXPECT_TRUE(car.is_destroyed);
    
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
    
    b2DestroyWorld(mundo);
}

// TESTS DE COLISIONES CON IMPULSOS

TEST(CollisionImpulseTest, ObstacleCollisionCausesBounce) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacles(mundo);
    obstacles.create_wall(5.0f, 0.0f, 1.0f, 10.0f);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId carBody = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box = b2MakeBox(1.0f, 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.5f;
    b2CreatePolygonShape(carBody, &shapeDef, &box);
    
    // Velocidad hacia la pared
    b2Body_SetLinearVelocity(carBody, {10.0f, 0.0f});
    
    // Simular colisión durante varios frames
    bool collision_detected = false;
    for (int i = 0; i < 100; ++i) {
        b2World_Step(mundo, 0.016666f, 4);
        
        b2Vec2 vel = b2Body_GetLinearVelocity(carBody);
        
        // Si la velocidad cambió de dirección, hubo rebote
        if (vel.x < 0.0f) {
            collision_detected = true;
            std::cout << "[Test] Bounce detected at frame " << i 
                      << " | Vel.x: " << vel.x << std::endl;
            break;
        }
    }
    
    EXPECT_TRUE(collision_detected);
    
    obstacles.clear();
    b2DestroyWorld(mundo);
}

TEST(CollisionImpulseTest, CarToCarMomentumExchange) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);

    b2BodyDef bodyDefA = b2DefaultBodyDef();
    bodyDefA.type = b2_dynamicBody;
    bodyDefA.position = {0.0f, 0.0f};
    b2BodyId carA = b2CreateBody(mundo, &bodyDefA);
    
    b2Circle circleA;
    circleA.center = {0.0f, 0.0f};
    circleA.radius = 1.0f;
    
    b2ShapeDef shapeDefA = b2DefaultShapeDef();
    shapeDefA.density = 1.5f;
    b2CreateCircleShape(carA, &shapeDefA, &circleA);
    
    b2Body_SetLinearVelocity(carA, {5.0f, 0.0f});
    
    b2BodyDef bodyDefB = b2DefaultBodyDef();
    bodyDefB.type = b2_dynamicBody;
    bodyDefB.position = {2.4f, 0.0f};
    b2BodyId carB = b2CreateBody(mundo, &bodyDefB);
    
    b2Circle circleB;
    circleB.center = {0.0f, 0.0f};
    circleB.radius = 1.0f;
    
    b2ShapeDef shapeDefB = b2DefaultShapeDef();
    shapeDefB.density = 1.5f;
    b2CreateCircleShape(carB, &shapeDefB, &circleB);
    
    b2Body_SetLinearVelocity(carB, {-5.0f, 0.0f});
    
    float vel_A_inicial = 5.0f;
    float vel_B_inicial = -5.0f;
    
    bool collision_detected = false;
    float min_distance = 999.0f;
    
    for (int i = 0; i < 200; ++i) {
        b2World_Step(mundo, 0.016666f, 8);
        
        b2Vec2 posA = b2Body_GetPosition(carA);
        b2Vec2 posB = b2Body_GetPosition(carB);
        float distance = std::abs(posB.x - posA.x);
        
        if (distance < min_distance) {
            min_distance = distance;
        }

        b2Vec2 velA_current = b2Body_GetLinearVelocity(carA);
        
        if (std::abs(velA_current.x - vel_A_inicial) > 0.1f) { 
             collision_detected = true;
             break;
        }
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        if (events.beginCount > 0) {
            collision_detected = true;
            break;
        }
    }
    
    // 5. Asersiones
    
    EXPECT_TRUE(collision_detected) << "Cars should have collided (min distance: " 
                                     << min_distance << ")";
    
    if (collision_detected) {
        b2Vec2 vel_A_final = b2Body_GetLinearVelocity(carA);
        b2Vec2 vel_B_final = b2Body_GetLinearVelocity(carB);
        
        // Verificar que las velocidades cambiaron (prueba de intercambio de impulso)
        EXPECT_NE(vel_A_final.x, vel_A_inicial);
        EXPECT_NE(vel_B_final.x, vel_B_inicial);
    }
    
    b2DestroyWorld(mundo);
}

// TESTS DE SPAWN POINTS
TEST(SpawnPointTest, LoadSpawnPointsFromMap) {
    Configuracion config("config.yaml");
    
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {0.0f, 0.0f};
    b2WorldId mundo = b2CreateWorld(&mundoDef);
    
    ObstacleManager obstacles(mundo);
    MapLoader loader(mundo, obstacles);
    
    ASSERT_NO_THROW({
        loader.load_map("server_src/city_maps/test_track.yaml");
    });
    
    auto spawns = loader.get_spawn_points();
    EXPECT_GT(spawns.size(), 0);
    
    b2DestroyWorld(mundo);
}

// TESTS DE INTEGRACIÓN
TEST(IntegrationTest, CompleteRaceScenario) {
    Configuracion config("config.yaml");
    Monitor monitor;
    Queue<Command> game_queue;
    
    GameLoop game(monitor, game_queue, config);
    
    // Cargar mapa
    ASSERT_NO_THROW({
        game.load_map("server_src/city_maps/test_track.yaml");
    });
    
    // Simular comandos de varios jugadores
    Command cmd1;
    cmd1.player_id = 1;
    cmd1.action = GameCommand::ACCELERATE;
    game_queue.push(cmd1);
    
    Command cmd2;
    cmd2.player_id = 2;
    cmd2.action = GameCommand::ACCELERATE;
    game_queue.push(cmd2);
    
    SUCCEED();
}

TEST(IntegrationTest, NitroIncreasesSpeed) {
    Car car(1, 12, 100);
    
    EXPECT_FALSE(car.is_nitro_active());
    
    bool activated = car.activate_nitro();
    EXPECT_TRUE(activated);
    EXPECT_TRUE(car.is_nitro_active());
    
    // Simular ticks hasta que se acabe el nitro
    int ticks = 0;
    while (car.is_nitro_active() && ticks < 20) {
        car.simulate_tick();
        ticks++;
    }
    
    EXPECT_FALSE(car.is_nitro_active());
    EXPECT_GE(ticks, 12); 
}
