# Need for Speed 2D - Taller de Programación I

Implementación del clásico Need for Speed en versión 2D multijugador (hasta 8 jugadores) con carreras por las ciudades de Liberty City, San Andreas y Vice City.

##  Requisitos del Sistema

- **Sistema Operativo**: Ubuntu 24.04 / Xubuntu 24.04
- **Compilador**: g++ con soporte C++20
- **CMake**: 3.24 o superior (se instala automáticamente)
- **Dependencias**: SDL2, Qt6, Box2D, GoogleTest (se instalan automáticamente)

##  Instalación Completa

El instalador automatizado cumple con todos los requisitos del TP:

### ✅ Qué hace el instalador:

1. **Descarga e instala todas las dependencias** (SDL2, Qt6, Box2D, etc.)
2. **Compila el proyecto completo**
3. **Ejecuta los tests unitarios** automáticamente
4. **Instala los binarios** en `/usr/bin/`:
   - `NFS-TP-client` - Cliente gráfico del juego
   - `NFS-TP-server` - Servidor del juego
   - `NFS-TP-editor` - Editor gráfico de mapas
5. **Instala la configuración** en `/etc/NFS-TP/`:
   - `config.yaml` - Archivo de configuración principal
6. **Instala los assets** en `/var/NFS-TP/assets/`:
   - Imágenes (autos, mapas, UI)
   - Fuentes (tipografías)
   - Música de fondo
   - Efectos de sonido
7. **Instala los mapas** en `/var/NFS-TP/recorridos/`:
   - Liberty City
   - San Andreas
   - Vice City

### Instalación Paso a Paso

```sh
# 1. Clonar el repositorio
git clone <URL_DEL_REPO>
cd TP_TALLER

# 2. Ejecutar el instalador completo (requiere sudo)
sudo make install
```

**Nota**: El instalador pedirá tu contraseña de sudo para:
- Instalar dependencias del sistema
- Copiar binarios a `/usr/bin`
- Crear directorios y copiar archivos a `/etc` y `/var`

### ✅ Verificar la Instalación

Después de instalar, puedes verificar que todo se instaló correctamente:

```sh
./verify_installation.sh
```

Este script verifica:
- ✅ Binarios en `/usr/bin`
- ✅ Configuración en `/etc/NFS-TP`
- ✅ Assets en `/var/NFS-TP/assets`
- ✅ Mapas en `/var/NFS-TP/recorridos`
- ✅ Permisos de ejecución

## 🎮 Uso

Una vez instalado, puedes ejecutar las aplicaciones desde cualquier terminal:

```sh
# Ejecutar el cliente
NFS-TP-client

# Ejecutar el servidor  
NFS-TP-server

# Ejecutar el editor
NFS-TP-editor
```

## Desarrollo

### Compilación Rápida (sin instalación completa)

Para desarrollo, usa el target `debug` que solo compila sin instalar:

```sh
make debug
```

Esto genera los ejecutables en la raíz del proyecto:
- `./client`
- `./server`
- `./taller_editor`

### Ejecutar Tests

```sh
make test
# O directamente:
./taller_tests
```

### Ejecutar Cliente y Servidor Localmente

```sh
# Terminal 1: Servidor
./server config.yaml

# Terminal 2: Cliente
./client
```

### Limpieza

```sh
# Limpieza ligera (mantiene dependencias descargadas - RÁPIDO)
make clean

# Limpieza profunda (elimina TODO incluyendo dependencias - LENTO)
make clean_all
```

**Recomendación**: Usa `make clean` durante desarrollo para recompilar rápidamente sin volver a descargar dependencias.

## 📂Estructura de Archivos

```
TP_TALLER/
├── client_src/          # Código fuente del cliente
├── server_src/          # Código fuente del servidor
├── common_src/          # Código compartido
├── editor/              # Código del editor
├── tests/               # Tests unitarios
├── assets/              # Assets del juego
│   ├── fonts/          # Fuentes
│   ├── img/            # Imágenes
│   └── music/          # Música
├── config.yaml         # Configuración principal
├── CMakeLists.txt      # Configuración de CMake
├── Makefile            # Makefile principal
└── install.sh          # Script de instalación

```
