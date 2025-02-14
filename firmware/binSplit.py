import os

def split_file(file_path, chunk_size=50 * 1024):
    """Split a binary file into smaller parts."""
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return
    
    file_size = os.path.getsize(file_path)
    print(f"File size: {file_size} bytes")
    
    with open(file_path, "rb") as file:
        part = 0
        while chunk := file.read(chunk_size):
            part_name = f"{os.path.splitext(file_path)[0]}_part{part:03d}.bin"
            with open(part_name, "wb") as part_file:
                part_file.write(chunk)
            print(f"Created: {part_name} ({len(chunk)} bytes)")
            part += 1

    print("File splitting complete.")

# Example usage
file_path = "firmware.bin"  # Replace with the path to your file
split_file(file_path)
