.PHONY: all debug test clean clean_all server client install setup

BUILD_DIR = build
INSTALLER = install.sh

# =======================================================
# Target de Setup (primera instalación)
# =======================================================

# Instala todas las dependencias necesarias del sistema
# Instala todas las dependencias necesarias del sistema
setup:
	@echo "--- Instalando dependencias del sistema ---"
	@sudo apt-get update
	@sudo apt-get install -y cmake build-essential qt6-base-dev libqt6charts6-dev qt6-multimedia-dev libyaml-cpp-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libxmp-dev fluidsynth libfluidsynth-dev libwavpack-dev
	@echo "--- Dependencias instaladas correctamente ---"


# =======================================================
# Targets de Desarrollo Rápido (make debug)
# =======================================================

# Target principal de compilación para desarrollo.
# Usa 'debug' para compilar rápidamente sin instalar.
debug:
	@echo "--- 1. Asegurando permisos sobre $(BUILD_DIR)/ ---"
	@if [ -d "$(BUILD_DIR)" ]; then sudo chown -R $(USER):$(USER) $(BUILD_DIR); fi
	@echo "--- 2. Configuracion de CMake ---"
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	@echo "--- 3. Compilacion ---"
	@cmake --build $(BUILD_DIR)

# Ejecuta el servidor (después de compilar)
server: debug
	@echo "--- Iniciando Servidor ---"
	@./server config/configuracion.yaml

# Ejecuta el cliente (después de compilar)
client: debug
	@echo "--- Iniciando Cliente (Lobby) ---"
	@./client

# Ejecuta todos los tests unitarios.
test: debug
	@echo "--- Ejecutando Tests ---"
	@./taller_tests

# Limpieza ligera: solo elimina ejecutables y archivos compilados (mantiene dependencias)
clean:
	@echo "--- Limpieza de Ejecutables ---"
	@echo "--- 1. Corrigiendo permisos (requiere sudo) ---"
	sudo chown -R $(USER):$(USER) $(BUILD_DIR) 2>/dev/null || echo "No se pudo cambiar permisos, continuando..."
	@echo "--- 2. Limpiando archivos compilados ---"
	@rm -f client server taller_editor taller_tests collision_test
	@if [ -d "$(BUILD_DIR)" ]; then \
		cd $(BUILD_DIR) && make clean 2>/dev/null || true; \
		rm -f CMakeCache.txt; \
		rm -rf CMakeFiles/; \
		rm -rf client_autogen/ server_autogen/ taller_editor_autogen/ taller_tests_autogen/ collision_test_autogen/; \
		rm -rf taller_common_autogen/ taller_lobby_autogen/; \
		rm -f *.a lib/*.a; \
		rm -f bin/*; \
		rm -rf client_src/ server_src/ common_src/ editor/; \
	fi
	@echo "--- Limpieza Completada (dependencias preservadas) ---"

# Limpieza profunda: elimina TODO incluyendo dependencias descargadas
clean_all:
	@echo "--- Limpieza Profunda (incluyendo dependencias) ---"
	@echo "--- 1. Corrigiendo permisos (requiere sudo) ---"
	sudo chown -R $(USER):$(USER) $(BUILD_DIR) 2>/dev/null || echo "No se pudo cambiar permisos, continuando..."
	@echo "--- 2. Eliminando todo el build ---"
	@rm -Rf $(BUILD_DIR)
	@rm -f client server taller_editor taller_tests collision_test
	@echo "--- Limpieza Profunda Completada ---"


# =======================================================
# Target de Instalación (make install)
# =======================================================

# El target 'install' primero instala dependencias y luego ejecuta el script de instalación.
# Para primera instalación: sudo make install
install: setup debug
	@echo "--- 4. Preparando el instalador ---"
	@sudo chmod +x $(INSTALLER)
	@echo "--- 5. Ejecutando instalación final del sistema (requiere sudo) ---"
	@sudo bash ./$(INSTALLER)
	@echo "--- Instalación Completa ---"
