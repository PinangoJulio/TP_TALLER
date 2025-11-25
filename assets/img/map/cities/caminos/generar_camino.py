import cv2
import numpy as np
import yaml
import sys

# --- CONFIGURACIÓN ---
IMAGE_PATH = 'camino-1-vice-city.jpg' # Cambia esto por tu mapa
HEX_COLOR = '#ff006f' 
TILE_SIZE = 1          
SAMPLE_RATE = 40       
OUTPUT_FILE = 'recorrido.yaml'

# IMPORTANTE: Define aquí dónde está el inicio de este trazado
# Opciones: 'ABAJO', 'ARRIBA', 'IZQUIERDA', 'DERECHA'
DONDE_EMPIEZA = 'ABAJO' 

SEARCH_RADIUS = 3      

def hex_to_bgr(hex_color):
    hex_color = hex_color.lstrip('#')
    rgb = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    return rgb[::-1]

def procesar_mapa():
    print(f"Procesando {IMAGE_PATH}...")
    img = cv2.imread(IMAGE_PATH)
    if img is None:
        print(f"Error: No se encontró '{IMAGE_PATH}'")
        return

    # 1. Crear máscara
    target_bgr = hex_to_bgr(HEX_COLOR)
    lower = np.array([max(0, c-20) for c in target_bgr])
    upper = np.array([min(255, c+20) for c in target_bgr])
    mask = cv2.inRange(img, lower, upper)

    # 2. Encontrar píxeles (y, x)
    points = np.argwhere(mask > 0)
    if len(points) == 0:
        print("No se encontró el color especificado.")
        return

    print(f"Píxeles encontrados: {len(points)}. Buscando inicio en: {DONDE_EMPIEZA}...")

    # --- LÓGICA DE INICIO FLEXIBLE ---
    # points[:, 0] son las Y (filas)
    # points[:, 1] son las X (columnas)
    
    if DONDE_EMPIEZA == 'ABAJO':
        # Mayor Y es abajo
        start_idx = np.argmax(points[:, 0])
    elif DONDE_EMPIEZA == 'ARRIBA':
        # Menor Y es arriba
        start_idx = np.argmin(points[:, 0])
    elif DONDE_EMPIEZA == 'IZQUIERDA':
        # Menor X es izquierda
        start_idx = np.argmin(points[:, 1])
    elif DONDE_EMPIEZA == 'DERECHA':
        # Mayor X es derecha
        start_idx = np.argmax(points[:, 1])
    else:
        print("Error: Configuración DONDE_EMPIEZA inválida.")
        return

    start_y, start_x = points[start_idx]
    
    # Preparar grilla para búsqueda rápida
    grid = np.pad(mask > 0, pad_width=SEARCH_RADIUS, mode='constant', constant_values=0)
    cy, cx = start_y + SEARCH_RADIUS, start_x + SEARCH_RADIUS
    
    ordered_path = []
    
    # Bucle de recorrido (Pathfinding rápido)
    while True:
        ordered_path.append((cx - SEARCH_RADIUS, cy - SEARCH_RADIUS))
        
        # "Borrar" zona visitada
        grid[cy-1:cy+2, cx-1:cx+2] = False 
        
        found_next = False
        y_min, y_max = cy - SEARCH_RADIUS, cy + SEARCH_RADIUS + 1
        x_min, x_max = cx - SEARCH_RADIUS, cx + SEARCH_RADIUS + 1
        
        window = grid[y_min:y_max, x_min:x_max]
        local_hits = np.argwhere(window)
        
        if len(local_hits) > 0:
            center = np.array([SEARCH_RADIUS, SEARCH_RADIUS])
            dists = np.sum((local_hits - center)**2, axis=1)
            nearest_idx = np.argmin(dists)
            dy, dx = local_hits[nearest_idx]
            cy = y_min + dy
            cx = x_min + dx
            found_next = True
        
        if not found_next:
            break

    # Muestreo final
    final_checkpoints = ordered_path[::SAMPLE_RATE]
    if ordered_path[-1] != final_checkpoints[-1]:
        final_checkpoints.append(ordered_path[-1])

    # Generar YAML
    data = {
        'mapa': IMAGE_PATH, # Guardo referencia de qué imagen se usó
        'checkpoints': []
    }

    for i, (y, x) in enumerate(final_checkpoints):
        data['checkpoints'].append({
            'id': i,
            'x': float(x) / TILE_SIZE, 
            'y': float(y) / TILE_SIZE, 
            'type': 'START' if i == 0 else ('FINISH' if i == len(final_checkpoints)-1 else 'CHECKPOINT')
        })

    with open(OUTPUT_FILE, 'w') as file:
        yaml.dump(data, file, sort_keys=False)

    print(f"¡Listo! {len(final_checkpoints)} checkpoints generados empezando desde {DONDE_EMPIEZA}.")

if __name__ == "__main__":
    procesar_mapa()