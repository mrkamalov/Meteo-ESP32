import os
import zlib

FIRMWARE_FILE = "firmware.bin"
VERSION = "1.1.0"

def crc32_file(path):
    """Подсчёт CRC32 всего файла"""
    prev = 0
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            prev = zlib.crc32(chunk, prev)
    return format(prev & 0xFFFFFFFF, '08X')

def main():
    if not os.path.exists(FIRMWARE_FILE):
        print(f"Ошибка: файл {FIRMWARE_FILE} не найден")
        return

    # Размер файла
    file_size = os.path.getsize(FIRMWARE_FILE)

    # CRC всего файла
    full_crc = crc32_file(FIRMWARE_FILE)

    # Запись config.txt
    with open("config.txt", "w") as cfg:
        cfg.write(f"{VERSION} {full_crc} {file_size}\n")

    print(f"Версия: {VERSION}")
    print(f"CRC: {full_crc}")
    print(f"Размер: {file_size} байт")
    print("Файл config.txt создан успешно.")

if __name__ == "__main__":
    main()
