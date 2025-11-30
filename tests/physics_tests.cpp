#include <gtest/gtest.h>
#include <box2d/box2d.h>
#include <vector>
#include "../common_src/config.h"

// Clase base para tests básicos de física
class PhysicsTest : public ::testing::Test {
protected:
    b2WorldId worldId;

    void SetUp() override {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 0.0f}; // Sin gravedad (vista top-down)
        worldId = b2CreateWorld(&worldDef);
    }

    void TearDown() override {
        if (b2World_IsValid(worldId)) {
            b2DestroyWorld(worldId);
        }
    }
};

// Verifica que el mundo de Box2D se cree y destruya sin errores
TEST_F(PhysicsTest, Box2DWorldIsValid) {
    EXPECT_TRUE(b2World_IsValid(worldId));
}

// Verifica que se pueda crear un cuerpo dinámico
TEST_F(PhysicsTest, CreateDynamicBody) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {10.0f, 10.0f};
    
    b2BodyId body = b2CreateBody(worldId, &bodyDef);
    
    EXPECT_TRUE(B2_IS_NON_NULL(body));
    
    b2Vec2 pos = b2Body_GetPosition(body);
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 10.0f);
}

// Verifica que aplicar una fuerza cambie la velocidad del cuerpo
TEST_F(PhysicsTest, ApplyForceMovesBody) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};
    
    b2BodyId body = b2CreateBody(worldId, &bodyDef);
    
    // Forma (necesaria para masa)
    b2Polygon box = b2MakeBox(1.0f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    // Aplicar fuerza
    b2Vec2 fuerza = {100.0f, 0.0f};
    b2Body_ApplyForceToCenter(body, fuerza, true);

    // Simular un paso
    b2World_Step(worldId, 0.016666f, 4);
    
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    EXPECT_GT(vel.x, 0.0f); // Debería haberse movido a la derecha
}

// Verifica que se puedan crear múltiples cuerpos
TEST_F(PhysicsTest, MultipleBodiesCreation) {
    std::vector<b2BodyId> bodies;
    for (int i = 0; i < 5; ++i) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {static_cast<float>(i * 10), 0.0f};
        
        b2BodyId body = b2CreateBody(worldId, &bodyDef);
        bodies.push_back(body);
        
        EXPECT_TRUE(B2_IS_NON_NULL(body));
    }
    
    EXPECT_EQ(bodies.size(), 5);
}