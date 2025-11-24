#!/bin/bash

# Script de verificación de estructura de mapas
# Verifica que cada circuito tenga su YAML y PNG correspondientes

CITY_MAPS_DIR="server_src/city_maps"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "  Verificación de Mapas del Servidor"
echo "========================================="
echo ""

if [ ! -d "$CITY_MAPS_DIR" ]; then
    echo -e "${RED}❌ Error: No existe la carpeta $CITY_MAPS_DIR${NC}"
    exit 1
fi

total_cities=0
total_circuits=0
total_valid=0
total_invalid=0

# Iterar sobre cada ciudad
for city_dir in "$CITY_MAPS_DIR"/*; do
    if [ ! -d "$city_dir" ]; then
        continue
    fi

    city_name=$(basename "$city_dir")

    # Ignorar archivos de ejemplo
    if [[ "$city_name" == "EJEMPLO"* ]]; then
        continue
    fi

    total_cities=$((total_cities + 1))

    echo -e "${YELLOW}📍 Ciudad: $city_name${NC}"

    circuits_found=0
    circuits_valid=0

    # Buscar archivos YAML en la ciudad
    for yaml_file in "$city_dir"/*.yaml; do
        # Verificar si el glob no encontró archivos
        if [ ! -f "$yaml_file" ]; then
            continue
        fi

        total_circuits=$((total_circuits + 1))
        circuits_found=$((circuits_found + 1))

        # Obtener nombre base del archivo (sin extensión)
        base_name=$(basename "$yaml_file" .yaml)

        # Verificar si existe el PNG correspondiente
        png_file="$city_dir/$base_name.png"

        if [ -f "$png_file" ]; then
            echo -e "   ${GREEN}✅ $base_name${NC}"
            echo -e "      YAML: $(basename "$yaml_file")"
            echo -e "      PNG:  $(basename "$png_file")"
            circuits_valid=$((circuits_valid + 1))
            total_valid=$((total_valid + 1))
        else
            echo -e "   ${RED}❌ $base_name${NC}"
            echo -e "      YAML: $(basename "$yaml_file") ✅"
            echo -e "      PNG:  $base_name.png ❌ FALTA"
            total_invalid=$((total_invalid + 1))
        fi
        echo ""
    done

    if [ $circuits_found -eq 0 ]; then
        echo -e "   ${YELLOW}⚠️  No se encontraron circuitos en esta ciudad${NC}"
        echo ""
    else
        echo -e "   📊 Circuitos válidos: $circuits_valid / $circuits_found"
        echo ""
    fi
done

echo "========================================="
echo "  Resumen"
echo "========================================="
echo -e "Ciudades encontradas:     $total_cities"
echo -e "Circuitos totales:        $total_circuits"
echo -e "${GREEN}Circuitos válidos:        $total_valid${NC}"

if [ $total_invalid -gt 0 ]; then
    echo -e "${RED}Circuitos inválidos:      $total_invalid${NC}"
    echo ""
    echo -e "${YELLOW}⚠️  Los circuitos inválidos NO aparecerán en el lobby${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}✅ Todos los circuitos están correctamente configurados${NC}"
    exit 0
fi

