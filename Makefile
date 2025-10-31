.PHONY: all debug test clean server client install

BUILD_DIR = build
INSTALLER = install.sh

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
	@./server juego.yaml

# Ejecuta el cliente (después de compilar)
client: debug
	@echo "--- Iniciando Cliente (Lobby) ---"
	@./client

# Ejecuta todos los tests unitarios.
test: debug
	@echo "--- Ejecutando Tests ---"
	@./taller_tests

# Limpieza: elimina la carpeta de compilación.
clean:
	@echo "--- Limpieza Profunda ---"
	@rm -Rf $(BUILD_DIR)
	@echo "--- Limpieza Completada ---"

# =======================================================
# Target de Instalación (make install)
# =======================================================

# El target 'install' ejecuta el script de instalación del sistema.
# Depende de 'debug' para asegurar que el proyecto esté compilado.
install: debug
	@echo "--- 4. Preparando el instalador ---"
	# 1. Asegurarse de que el script instalador sea ejecutable
	@sudo chmod +x $(INSTALLER)
	# 2. Ejecutar el script con privilegios de root para la instalación final.
	@echo "--- 5. Ejecutando instalación final del sistema (requiere sudo) ---"
	@sudo bash ./$(INSTALLER)
	@echo "--- Instalación Completa ---"


