#include "../common_src/config.h" // Nuestra clase lectora de YAML
#include <box2d/box2d.h>                      // Motor físico (v3.x)
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

// Para simplificar, la ruta se asume donde el instalador la puso (/etc/NFS-TP/)
#define TIPO_AUTO_PRUEBA "DEPORTIVO"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Faltan argumentos. Uso correcto: ./server <ruta_a_configuracion.yaml>" << std::endl;
        return 1;
    }

    // El archivo de configuración es el primer argumento (argv[1])
    const std::string ruta_config = argv[1];
    try {
        // 1. Cargar Configuración YAML
        Configuracion config(ruta_config);

        // 2. Inicializar Box2D
        b2Vec2 gravedad = {config.obtenerGravedadX(), config.obtenerGravedadY()};

        // Inicialización estricta para evitar errores de B2_SECRET_COOKIE
        b2WorldDef mundoDef = b2DefaultWorldDef();
        mundoDef.gravity = gravedad;
        b2WorldId mundo = b2CreateWorld(&mundoDef);

        int subStepCount = config.obtenerIteracionesVelocidad() + config.obtenerIteracionesPosicion();


        std::cout << "Servidor: Configuración YAML cargada y Box2D (V3) inicializado." << std::endl;
        std::cout << "Gravedad del mundo (X, Y): (" << gravedad.x << ", " << gravedad.y << ")" << std::endl;

        // 3. Crear un Cuerpo Estático (Muro de Prueba)
        b2BodyDef groundBodyDef = b2DefaultBodyDef();
        groundBodyDef.type = b2_staticBody;
        groundBodyDef.position = {0.0f, -10.0f};
        b2BodyId groundBody = b2CreateBody(mundo, &groundBodyDef);

        b2Polygon solidBox = b2MakeBox(50.0f, 10.0f);
        b2ShapeDef groundShapeDef = b2DefaultShapeDef();

        b2ShapeId groundShape = b2CreatePolygonShape(groundBody, &groundShapeDef, &solidBox);
        b2Shape_SetFriction(groundShape, 0.8f); // Aplicar fricción

        std::cout << "Servidor: Muro estático de prueba creado." << std::endl;

        // 4. Crear un Cuerpo Dinámico (Auto de Prueba)
        const AtributosAuto& attrs = config.obtenerAtributosAuto(TIPO_AUTO_PRUEBA);

        b2BodyDef autoBodyDef = b2DefaultBodyDef();
        autoBodyDef.type = b2_dynamicBody;
        autoBodyDef.position = {0.0f, 4.0f};
        autoBodyDef.linearDamping = attrs.control * 0.05f;

        b2BodyId autoBody = b2CreateBody(mundo, &autoBodyDef);

        b2Polygon autoShape = b2MakeBox(attrs.masa * 0.5f, attrs.masa * 0.25f);
        b2ShapeDef autoShapeDef = b2DefaultShapeDef();

        b2ShapeId autoShapeId = b2CreatePolygonShape(autoBody, &autoShapeDef, &autoShape);

        // CORRECCIÓN FINAL: Añadir el argumento 'bool updateBodyMass' (true)
        b2Shape_SetDensity(autoShapeId, attrs.masa, true);
        b2Shape_SetFriction(autoShapeId, 0.8f);

        // Aplicar una fuerza inicial
        b2Vec2 fuerza = {attrs.aceleracion * 10.0f, 0.0f};
        b2Body_ApplyForceToCenter(autoBody, fuerza, true);

        std::cout << "Servidor: Auto '" << TIPO_AUTO_PRUEBA << "' creado y acelerado con Masa: " << attrs.masa << std::endl;


        // 5. Bucle de Simulación de Prueba (1 segundo)
        float tiempo_paso = config.obtenerTiempoPaso();

        int pasos = static_cast<int>(1.0f / tiempo_paso);

        std::cout << "\nSimulando 1 segundo (" << pasos << " pasos):" << std::endl;

        for (int i = 0; i < pasos; ++i) {
            b2World_Step(mundo, tiempo_paso, subStepCount);

            b2Vec2 posicion = b2Body_GetPosition(autoBody);
            b2Rot rotacion = b2Body_GetRotation(autoBody);

            float angulo = b2Rot_GetAngle(rotacion);

            if (i % 30 == 0) {
                std::cout << "  Paso " << i << ": Posicion X=" << posicion.x << ", Angulo=" << angulo << " rad" << std::endl;
            }
        }

        b2DestroyWorld(mundo);

        std::cout << "\nServidor: Simulacion de prueba finalizada. Éxito en la Fase 1." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nFallo fatal del Servidor: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}