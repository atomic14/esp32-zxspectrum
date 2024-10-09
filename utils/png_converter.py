import sys
from PIL import Image

def rgba_to_rgb565(r, g, b):
    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # Swap the bytes
    return ((rgb565 & 0x00FF) << 8) | ((rgb565 & 0xFF00) >> 8)

def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <input_png_file>")
        return

    filename = sys.argv[1]
    
    # Load the PNG file using Pillow
    try:
        with Image.open(filename) as img:
            img = img.convert("RGBA")
            width, height = img.size
            image = list(img.getdata())
    except Exception as e:
        print(f"Error loading image: {e}")
        return

    # Create and write to the output file
    output_filename = "image_rgb565.cpp"
    with open(output_filename, "w") as output_file:
        # Write the image dimensions
        output_file.write("#include <cstdint>\n\n")
        output_file.write(f"const uint16_t rainbowImageWidth = {width};\n")
        output_file.write(f"const uint16_t rainbowImageHeight = {height};\n\n")
        output_file.write("const uint16_t rainbowImageData[] = {\n")

        # Convert to RGB565 and write the pixel data
        for y in range(height):
            for x in range(width):
                r, g, b, a = image[y * width + x]  # Get RGBA values
                rgb565 = rgba_to_rgb565(r, g, b)
                output_file.write(f"0x{rgb565:04x}, ")
            output_file.write("\n")

        output_file.write("};\n")
    
    print(f"Conversion completed. Output written to {output_filename}")

if __name__ == "__main__":
    main()