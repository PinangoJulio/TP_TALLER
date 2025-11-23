#!/bin/bash
# Script de instalación de pre-commit hooks para Need for Speed 2D

set -e  # Salir si hay error

echo "=================================================="
echo " Instalando Pre-commit Hooks"
echo "=================================================="
echo ""

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# ========================================
# 1. Verificar si estamos en el directorio correcto
# ========================================
if [ ! -f ".pre-commit-config.yaml" ]; then
    echo -e "${RED}❌ Error: .pre-commit-config.yaml no encontrado${NC}"
    echo "   Ejecuta este script desde la raíz del proyecto"
    exit 1
fi

echo -e "${YELLOW} Directorio actual:${NC} $(pwd)"
echo ""

# ========================================
# 2. Instalar pre-commit
# ========================================
echo -e "${YELLOW} Instalando pre-commit...${NC}"
if command -v pre-commit &> /dev/null; then
    echo -e "${GREEN}✓ pre-commit ya está instalado${NC}"
else
    echo "   Instalando con pip..."
    pip install --user pre-commit

    # Agregar al PATH si no está
    if ! command -v pre-commit &> /dev/null; then
        export PATH="$HOME/.local/bin:$PATH"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
        echo -e "${GREEN}✓ pre-commit instalado y agregado al PATH${NC}"
    fi
fi
echo ""

# ========================================
# 3. Instalar clang-format
# ========================================
echo -e "${YELLOW}🔨 Instalando clang-format...${NC}"
if command -v clang-format &> /dev/null; then
    echo -e "${GREEN}✓ clang-format ya está instalado${NC}"
else
    echo "   Instalando clang-format..."
    sudo apt update
    sudo apt install -y clang-format
    echo -e "${GREEN}✓ clang-format instalado${NC}"
fi
echo ""

# ========================================
# 4. Instalar cppcheck
# ========================================
echo -e "${YELLOW}🔍 Instalando cppcheck...${NC}"
if command -v cppcheck &> /dev/null; then
    echo -e "${GREEN}✓ cppcheck ya está instalado${NC}"
else
    echo "   Instalando cppcheck..."
    sudo apt install -y cppcheck
    echo -e "${GREEN}✓ cppcheck instalado${NC}"
fi
echo ""

# ========================================
# 5. Instalar cmake-format (opcional)
# ========================================
echo -e "${YELLOW} Instalando cmake-format (opcional)...${NC}"
if command -v cmake-format &> /dev/null; then
    echo -e "${GREEN}✓ cmake-format ya está instalado${NC}"
else
    echo "   Instalando cmake-format..."
    pip install --user cmake-format
    echo -e "${GREEN}✓ cmake-format instalado${NC}"
fi
echo ""

# ========================================
# 6. Instalar hooks en el repositorio
# ========================================
echo -e "${YELLOW} Instalando hooks en el repositorio...${NC}"
pre-commit install
echo -e "${GREEN}✓ Hooks instalados en .git/hooks/${NC}"
echo ""

# ========================================
# 7. Ejecutar en todos los archivos (opcional)
# ========================================
echo -e "${YELLOW}❓ ¿Deseas ejecutar los hooks en todos los archivos ahora?${NC}"
echo "   Esto puede tardar varios minutos la primera vez."
read -p "   [s/N]: " -n 1 -r
echo ""

if [[ $REPLY =~ ^[SsYy]$ ]]; then
    echo ""
    echo -e "${YELLOW}Ejecutando pre-commit en todos los archivos...${NC}"
    echo "   (Esto puede tardar un rato la primera vez)"
    echo ""

    if pre-commit run --all-files; then
        echo ""
        echo -e "${GREEN}✓ Todos los checks pasaron correctamente${NC}"
    else
        echo ""
        echo -e "${YELLOW}⚠  Algunos archivos fueron modificados automáticamente${NC}"
        echo "   Revisa los cambios con: git diff"
        echo "   Luego haz: git add . && git commit"
    fi
else
    echo ""
    echo -e "${YELLOW}ℹ  Los hooks se ejecutarán automáticamente en tu próximo commit${NC}"
fi

echo ""
echo "=================================================="
echo -e "${GREEN}✅ Instalación completada${NC}"
echo "=================================================="
echo ""
echo " Para más información, lee: PRE_COMMIT_SETUP.md"
echo ""
echo " Comandos útiles:"
echo "   • pre-commit run --all-files    (ejecutar manualmente)"
echo "   • pre-commit autoupdate         (actualizar versiones)"
echo "   • pre-commit uninstall          (desinstalar hooks)"
echo ""

