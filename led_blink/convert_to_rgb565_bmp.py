from PIL import Image
import sys
import struct
import math


def convert_to_rgb565_bmp(input_file, output_file):
    img = Image.open(input_file).convert("RGB")
    width, height = img.size

    # Tamaño real de cada fila, alineado a 4 bytes
    row_raw = width * 2
    row_padded = (row_raw + 3) & ~3      # múltiplo de 4
    padding = row_padded - row_raw       # bytes de relleno por fila

    # Tamaño total de la data del BMP
    pixel_data_size = row_padded * height

    # Crear encabezado BMP básico (54 bytes) + bitfields (12 bytes)
    filesize = 54 + 12 + pixel_data_size
    data_offset = 54 + 12

    header = bytearray(54)
    header[0:2]  = b'BM'
    header[2:6]  = struct.pack('<I', filesize)
    header[10:14] = struct.pack('<I', data_offset)
    header[14:18] = struct.pack('<I', 40)      # tamaño header DIB
    header[18:22] = struct.pack('<I', width)
    header[22:26] = struct.pack('<I', height)
    header[26:28] = struct.pack('<H', 1)       # planos
    header[28:30] = struct.pack('<H', 16)      # bits/pixel
    header[30:34] = struct.pack('<I', 3)       # BI_BITFIELDS
    header[34:38] = struct.pack('<I', pixel_data_size)
    header[38:42] = struct.pack('<I', 2835)
    header[42:46] = struct.pack('<I', 2835)
    header[46:50] = struct.pack('<I', 0)
    header[50:54] = struct.pack('<I', 0)

    # Máscaras RGB565
    bitfields = struct.pack('<III', 0xF800, 0x07E0, 0x001F)

    with open(output_file, "wb") as f:
        f.write(header)
        f.write(bitfields)

        # BMP guarda filas bottom-up
        for y in range(height - 1, -1, -1):
            # Escribir píxeles
            for x in range(width):
                r, g, b = img.getpixel((x, y))
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                f.write(struct.pack('<H', rgb565))

            # Escribir padding necesario
            if padding:
                f.write(b'\x00' * padding)

    print(f"✅ BMP RGB565 guardado correctamente: {output_file}")
    print(f"   → {width}x{height}")
    print(f"   → row_raw={row_raw}, row_padded={row_padded}, padding={padding}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python convert_to_rgb565_bmp.py entrada.png salida.bmp")
    else:
        convert_to_rgb565_bmp(sys.argv[1], sys.argv[2])



