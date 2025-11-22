# install.sh

set -e

TP_NAME="NFS-TP"
BUILD_DIR="build"
CMAKE_INSTALL_DIR="/opt/cmake" # Debe coincidir con la ubicación de instalación


echo "==================================================="
echo               "TALLER TP - $TP_NAME"
echo "==================================================="

# ----------------------------------------------------
# [1/4] INSTALACIÓN DE DEPENDENCIAS DEL SISTEMA
# ----------------------------------------------------

# --- LÓGICA DE SALTO ---
# Si el ejecutable moderno de CMake ya existe, saltamos la instalación con sudo.
if [ -f "$CMAKE_INSTALL_DIR/bin/cmake" ]; then
    echo "[1/4] Dependencias encontradas. Saltando la instalación del sistema (sudo)."
else
    echo "[1/4] Instalando dependencias base del sistema y actualizando CMake a 3.24+..."
    # Ejecuta el bloque de instalación con sudo si CMake no está en /opt/cmake/bin/

    sudo apt-get install -y \
      g++ qt6-base-dev qt6-multimedia-dev \
      qt6-base-dev-tools build-essential git curl \
      libopus-dev libopusfile-dev libxmp-dev libfluidsynth-dev \
      libwavpack-dev libfreetype-dev

    # --- INSTALACIÓN DE CMAKE (requiere sudo) ---
    CMAKE_VERSION="3.28"

    sudo mkdir -p $CMAKE_INSTALL_DIR

    echo "Descargando CMake v${CMAKE_VERSION}..."
    mkdir -p /tmp/cmake_install
    cd /tmp/cmake_install
    curl -OL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}.0/cmake-${CMAKE_VERSION}.0-linux-x86_64.sh

    echo "Instalando CMake..."
    chmod +x cmake-${CMAKE_VERSION}.0-linux-x86_64.sh
    # Usamos sudo sh -c para la instalación en directorio protegido:
    sudo sh -c "./cmake-${CMAKE_VERSION}.0-linux-x86_64.sh --skip-license --prefix=$CMAKE_INSTALL_DIR"

    echo "Configurando symlink para CMake v${CMAKE_VERSION}..."
    sudo ln -sf $CMAKE_INSTALL_DIR/bin/cmake /usr/local/bin/cmake

    cd - > /dev/null
    rm -rf /tmp/cmake_install
fi

## ----------------------------------------------------
## [2/4] COMPILACIÓN Y TESTS (NO REQUIERE SUDO)
## ----------------------------------------------------

echo "[2/4] Configurando, compilando el proyecto y corriendo tests..."

# Si CMAKE_RUNTIME_OUTPUT_DIRECTORY está configurado en el CMakeLists.txt para ir a la raíz,
# la compilación genera ejecutables en la raíz.
# Estos comandos deben ejecutarse SIN SUDO.

# 2.1 Configuración (Si la compilación se hace con usuario normal)
cmake -B $BUILD_DIR

# 2.2 Compilación
cmake --build $BUILD_DIR

# 2.3 Ejecución de Tests
echo "Corriendo tests unitarios..."
./taller_tests


## ----------------------------------------------------
## [3/4] INSTALACIÓN (COPIA DE ARCHIVOS - REQUIERE SUDO)
## ----------------------------------------------------

echo "[3/4] Instalando archivos en el sistema Linux (/usr/bin, /var, /etc)..."

# Este paso SIEMPRE debe ejecutarse con SUDO si las rutas de destino son protegidas.
# Si solo quieres compilar rápido, puedes comentar las líneas de 'sudo' en esta sección
# y ejecutar sólo los Pasos [1/4] y [2/4] manualmente.

# 3.1 Limpiar y crear directorios de destino
TP_ROOT_DIR="/var/$TP_NAME"
sudo mkdir -p /usr/bin /etc/$TP_NAME $TP_ROOT_DIR/assets $TP_ROOT_DIR/recorridos

# 3.2 Copiar Binarios a /usr/bin
echo "   → Copiando binarios a /usr/bin..."
sudo cp ./client /usr/bin/$TP_NAME-client
sudo cp ./server /usr/bin/$TP_NAME-server
sudo cp ./taller_editor /usr/bin/$TP_NAME-editor

# 3.3 Copiar Archivos de Configuración (YAML) a /etc
echo "   → Copiando archivos de configuración a /etc/$TP_NAME/..."
sudo cp config.yaml /etc/$TP_NAME/

# 3.4 Copiar Assets (Imágenes, Fuentes, Sonidos, Música) a /var
echo "   → Copiando assets a $TP_ROOT_DIR/assets/..."
# Copiar gráficos (si existen)
[ -d "gfx" ] && sudo cp -r gfx/* $TP_ROOT_DIR/assets/ 2>/dev/null || true
# Copiar sonidos (si existen)
[ -d "sfx" ] && sudo cp -r sfx/* $TP_ROOT_DIR/assets/ 2>/dev/null || true
# Copiar assets completos (fuentes, imágenes de lobby, música)
[ -d "assets" ] && sudo cp -r assets/* $TP_ROOT_DIR/assets/ 2>/dev/null || true

# 3.5 Copiar Recorridos/Mapas a /var
echo "   → Copiando mapas de ciudades a $TP_ROOT_DIR/recorridos/..."
sudo cp -r server_src/city_maps/* $TP_ROOT_DIR/recorridos/

## ----------------------------------------------------
## [4/4] Finalización
## ----------------------------------------------------
echo "[4/4] Instalación completada con éxito ✅"
echo ""
echo "==================================================="
echo "           INSTALACIÓN COMPLETADA"
echo "==================================================="
echo ""
echo " BINARIOS instalados en:"
echo "   • /usr/bin/$TP_NAME-client"
echo "   • /usr/bin/$TP_NAME-server"
echo "   • /usr/bin/$TP_NAME-editor"
echo ""
echo "  CONFIGURACIÓN instalada en:"
echo "   • /etc/$TP_NAME/config.yaml"
echo ""
echo " ASSETS (imágenes, música, fuentes) instalados en:"
echo "   • /var/$TP_NAME/assets/"
echo ""
echo "  MAPAS/RECORRIDOS instalados en:"
echo "   • /var/$TP_NAME/recorridos/"
echo ""
