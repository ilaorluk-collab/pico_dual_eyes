import numpy as np

def generate_eye_array():
    num_frames = 30
    rows = 240
    cols = 240
    bytes_per_row = cols // 8 # 30 байт
    total_bytes = rows * bytes_per_row # 7200 байт
    
    frames = []
    
    # Позиции века для 30 кадров (от 0 до 240 и обратно)
    # Сначала глаз открыт (веко на 0), потом закрывается до 240, потом открывается
    eyelid_positions = np.linspace(0, 240, 15, dtype=int).tolist() # Закрытие
    eyelid_positions += eyelid_positions[::-1] # Открытие
    
    for frame_idx, eyelid_y in enumerate(eyelid_positions):
        # Создаем пустую матрицу (0 = черный фон)
        matrix = np.zeros((rows, cols), dtype=int)
        
        # Рисуем глаз (белок) - это будут "1" (желтый)
        cy, cx = 120, 120
        radius = 115
        y, x = np.ogrid[:rows, :cols]
        dist_from_center = np.sqrt((x - cx)**2 + (y - cy)**2)
        
        # Основное тело глаза
        matrix[dist_from_center <= radius] = 1
        
        # Зрачок в центре (20x20) - это "0" (черный провал в желтом)
        matrix[110:130, 110:130] = 0
        
        # ВЕРХНЕЕ ВЕКО: всё, что выше eyelid_y, закрашиваем черным (0)
        matrix[:eyelid_y, :] = 0
        
        # Конвертация в байты (MSB First - как в твоем примере)
        frame_bytes = []
        for r in range(rows):
            for c_byte in range(bytes_per_row):
                byte_val = 0
                for bit in range(8):
                    if matrix[r, c_byte * 8 + bit] == 1:
                        byte_val |= (1 << (7 - bit))
                frame_bytes.append(f"0x{byte_val:02X}")
        
        frames.append(frame_bytes)
    
    # Формируем текст для C++
    with open("eye_frames.h", "w") as f:
        f.write(f"const uint8_t eye_frames[30][7200] = {{\n")
        for i, frame in enumerate(frames):
            f.write(f"  // Кадр {i}\n  {{")
            f.write(", ".join(frame))
            f.write("},\n\n")
        f.write("};\n")

generate_eye_array()
print("Готово! Файл eye_frames.h создан.")
