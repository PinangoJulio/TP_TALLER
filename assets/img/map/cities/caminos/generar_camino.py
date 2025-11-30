import cv2
import numpy as np
import yaml
from collections import deque

# --- CONFIGURACIÓN ---
IMAGE_PATH = 'vice-city/camino-2-vice-city.png' 
OUTPUT_YAML = 'recorrido_final.yaml'
OUTPUT_IMG = 'debug_resultado_v5.jpg'
OUTPUT_MASK = 'debug_mascara_v5.jpg'

TILE_SIZE = 1          
SAMPLE_RATE = 50        # Cada cuantos pasos guardamos un checkpoint
DONDE_EMPIEZA = 'IZQUIERDA' 

def get_start_point(coords, orientation):
    # coords es un array de [y, x]
    if orientation == 'ABAJO':     return coords[np.argmax(coords[:, 0])]
    elif orientation == 'ARRIBA':   return coords[np.argmin(coords[:, 0])]
    elif orientation == 'IZQUIERDA': return coords[np.argmin(coords[:, 1])]
    elif orientation == 'DERECHA':   return coords[np.argmax(coords[:, 1])]
    return coords[0]

def procesar_mapa():
    print(f"--- Iniciando V5 (Inundación) para {IMAGE_PATH} ---")
    
    # 1. Carga y Máscara
    img = cv2.imread(IMAGE_PATH)
    if img is None:
        print("Error: No imagen.")
        return

    # Usamos HSV para detectar el rosa
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    lower_pink = np.array([140, 50, 50])
    upper_pink = np.array([175, 255, 255])
    mask = cv2.inRange(hsv, lower_pink, upper_pink)

    # 2. Asegurar CONEXIÓN (Clave para que no se corte)
    # Dilatamos (engordamos) el camino para cerrar cualquier hueco pequeño
    kernel = np.ones((5,5), np.uint8)
    mask = cv2.dilate(mask, kernel, iterations=2)
    mask = cv2.erode(mask, kernel, iterations=1) # Restauramos un poco el borde
    
    # Quedarse solo con el camino más grande
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours: return
    largest_contour = max(contours, key=cv2.contourArea)
    clean_mask = np.zeros_like(mask)
    cv2.drawContours(clean_mask, [largest_contour], -1, 255, thickness=cv2.FILLED)
    
    cv2.imwrite(OUTPUT_MASK, clean_mask)

    # 3. Algoritmo de INUNDACIÓN (BFS)
    # Convertimos la máscara en puntos navegables
    # y_idxs, x_idxs
    points = np.argwhere(clean_mask > 0)
    start_y, start_x = get_start_point(points, DONDE_EMPIEZA)
    
    print(f"Inicio detectado en: {start_x}, {start_y}")
    print("Calculando distancias (Inundando el mapa)...")

    # Mapa de distancias (-1 significa no visitado)
    h, w = clean_mask.shape
    dist_map = np.full((h, w), -1, dtype=np.int32)
    parent_map = np.zeros((h, w, 2), dtype=np.int32) # Para recordar de dónde vinimos

    queue = deque([(start_y, start_x)])
    dist_map[start_y, start_x] = 0
    
    max_dist = 0
    end_node = (start_y, start_x)

    # Direcciones: Arriba, Abajo, Izq, Der
    neighbors = [(-1,0), (1,0), (0,-1), (0,1)]

    while queue:
        cy, cx = queue.popleft()
        
        # Si encontramos un punto más lejos, actualizamos el final
        if dist_map[cy, cx] > max_dist:
            max_dist = dist_map[cy, cx]
            end_node = (cy, cx)

        for dy, dx in neighbors:
            ny, nx = cy + dy, cx + dx
            
            # Verificar limites y si es parte del camino rosa (clean_mask)
            if 0 <= ny < h and 0 <= nx < w:
                if clean_mask[ny, nx] > 0 and dist_map[ny, nx] == -1:
                    dist_map[ny, nx] = dist_map[cy, cx] + 1
                    parent_map[ny, nx] = [cy, cx]
                    queue.append((ny, nx))

    print(f"Camino inundado. Distancia máxima encontrada: {max_dist} pasos.")

    # 4. BACKTRACKING (Recuperar el camino desde el final hasta el inicio)
    path = []
    curr_y, curr_x = end_node
    
    print("Reconstruyendo ruta óptima...")
    while (curr_y, curr_x) != (start_y, start_x):
        path.append((curr_x, curr_y))
        # Moverse al padre (hacia atrás)
        py, px = parent_map[curr_y, curr_x]
        curr_y, curr_x = py, px
    
    path.append((start_x, start_y))
    path.reverse() # Ahora va de Inicio -> Fin

    # 5. MUESTREO Y SALIDA
    # Al hacer BFS, el camino va pixel a pixel. Muestreamos.
    final_checkpoints = path[::SAMPLE_RATE]
    if path[-1] != final_checkpoints[-1]:
        final_checkpoints.append(path[-1])

    # Visualización
    debug_img = img.copy()
    debug_img = (debug_img * 0.4).astype(np.uint8)

    for i in range(len(final_checkpoints) - 1):
        pt1 = tuple(map(int, final_checkpoints[i]))
        pt2 = tuple(map(int, final_checkpoints[i+1]))
        cv2.line(debug_img, pt1, pt2, (0, 255, 255), 2)
        if i % 2 == 0:
             cv2.arrowedLine(debug_img, pt1, pt2, (255, 255, 255), 2, tipLength=0.5)

    cv2.circle(debug_img, tuple(map(int, final_checkpoints[0])), 8, (0, 255, 0), -1)
    cv2.circle(debug_img, tuple(map(int, final_checkpoints[-1])), 8, (0, 0, 255), -1)

    cv2.imwrite(OUTPUT_IMG, debug_img)

    # YAML
    data = {'mapa': IMAGE_PATH, 'checkpoints': []}
    for i, (x, y) in enumerate(final_checkpoints):
        data['checkpoints'].append({
            'id': i,
            'x': float(x) / TILE_SIZE, 
            'y': float(y) / TILE_SIZE, 
            'type': 'START' if i == 0 else ('FINISH' if i == len(final_checkpoints)-1 else 'CHECKPOINT')
        })

    with open(OUTPUT_YAML, 'w') as file:
        yaml.dump(data, file, sort_keys=False)

    print(f"¡HECHO! Revisa {OUTPUT_IMG}. Debería ser perfecto.")

if __name__ == "__main__":
    procesar_mapa()