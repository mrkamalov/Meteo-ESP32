import os
import zlib

FIRMWARE_FILE = "firmware.bin"
PART_SIZE = 921600  # размер части, например 50 КБ
VERSION = "1.1.0"

def crc32_file(path):
    """Подсчёт CRC32 всего файла"""
    prev = 0
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            prev = zlib.crc32(chunk, prev)
    return format(prev & 0xFFFFFFFF, '08X')

def crc32_data(data):
    """Подсчёт CRC32 блока данных"""
    return format(zlib.crc32(data) & 0xFFFFFFFF, '08X')

def split_file(file_path, part_size):
    """Разделение файла на части и подсчёт CRC каждой части"""
    part_crcs = []
    with open(file_path, "rb") as f:
        index = 0
        while True:
            data = f.read(part_size)
            if not data:
                break
            part_name = f"part{index:03d}.bin"
            with open(part_name, "wb") as part:
                part.write(data)
            crc = crc32_data(data)
            part_crcs.append(crc)
            print(f"Создан {part_name}, CRC={crc}, размер={len(data)} байт")
            index += 1
    return part_crcs, index

def main():
    # CRC всего файла
    full_crc = crc32_file(FIRMWARE_FILE)
    print(f"CRC всего файла: {full_crc}")

    # Разделение на части
    part_crcs, total_parts = split_file(FIRMWARE_FILE, PART_SIZE)

    # Запись config.txt
    with open("config.txt", "w") as cfg:
        # первая строка
        cfg.write(f"{VERSION} {full_crc} {total_parts} {PART_SIZE}\n")
        # CRC каждой части в столбик
        for crc in part_crcs:
            cfg.write(f"{crc}\n")

    print("Файл config.txt создан успешно.")

if __name__ == "__main__":
    main()
