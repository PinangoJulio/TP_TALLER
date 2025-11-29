# CAMBIOS

## CAMBIOS GENERALES
 - Cambié de sitio el collision_manager.h y .cpp a common_src
 - Creé la clase game_renderer para empezar a migrar lo del test a la funcionalidad
 - Actualicé los CMakeList
 - Agregué campo int32_t level = 0; a info player para determinar el nivel en el que se mueve en el mapa
 - Agruegué en server_src/car.h el atributo int level, para saber el nivel
 - Agruegué en server_src/car.h los métodos:
  
    

        int getLevel() const { return level; }

        void setLevel(int l) { level = l; } 
        
 - Agruegué la inicialización del nivel en el constructor de car 
 - Agregué #include "../../common_src/collision_manager.h" en el gameloop
 - Agruegué el atributo CollisionManager pero lo dejé comentado en gameloop.h
 - Tuve que cambiar el CmakeList de la raiz para que me tomara las librerías y cosas de sdl


## CAMBIOS PARA QUE EL GAMELOOP TOME LAS CONFIGURACIONES DE LAS CARRERAS
 - En gamestate puse en server race confing el atributo ```int laps = 1``` porque se supone que cada circuito se corre una sola vez, lo que cambia es el set de la carrera.
 - Cambié el constructor del gameloop para que reciba la ciudad y poder hacer la renderización luego por ciudad, por ende también tuve que cambiar match, race y algunos test para que usaran la nueva forma del constructor 