from PIL import Image, ImageEnhance, ImageFilter
import numpy as np
import os

def convert_image_for_m5stack(input_file, output_file="m5stack_image.h", 
                             width=320, height=240, variable_name="img_data"):
    """
    Converts an image for optimal display on M5Stack with advanced color correction
    and generates a ready-to-use header file.
    """
    print(f"Converting {input_file} for M5Stack display...")
    
    # Load and resize image
    img = Image.open(input_file).convert("RGB")
    original_width, original_height = img.size
    
    # Calculate aspect ratio to maintain proportions
    ratio = min(width / original_width, height / original_height)
    new_width = int(original_width * ratio)
    new_height = int(original_height * ratio)
    
    # Resize with high quality
    img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
    
    # Create black canvas of target size
    canvas = Image.new("RGB", (width, height), (0, 0, 0))
    
    # Paste resized image centered on canvas
    offset_x = (width - new_width) // 2
    offset_y = (height - new_height) // 2
    canvas.paste(img, (offset_x, offset_y))
    img = canvas
    
    # Apply image enhancements for better display on LCD
    img = ImageEnhance.Contrast(img).enhance(1.2)      # Increase contrast
    img = ImageEnhance.Brightness(img).enhance(1.1)    # Slightly brighter
    img = ImageEnhance.Color(img).enhance(1.2)         # More saturated
    img = ImageEnhance.Sharpness(img).enhance(1.5)     # Sharper
    
    # Apply gentle noise reduction
    img = img.filter(ImageFilter.GaussianBlur(radius=0.5))
    img = img.filter(ImageFilter.UnsharpMask(radius=1, percent=150, threshold=3))
    
    # Convert to numpy array
    pixels = np.array(img)
    
    # Apply gamma correction for the M5Stack display
    gamma = 1.2  # Adjust if needed (values > 1 brighten midtones)
    pixels = np.power(pixels / 255.0, 1.0 / gamma) * 255.0
    pixels = pixels.astype(np.uint8)
    
    # Convert to RGB565 with optimized color mapping and dithering
    rgb565_array = convert_to_rgb565_optimized(pixels)
    
    # Generate header file
    generate_header_file(rgb565_array, output_file, width, height, variable_name)
    
    print(f"Conversion complete! Header file saved as {output_file}")
    print(f"Image size: {width}x{height} pixels ({width*height} total)")
    return rgb565_array

def convert_to_rgb565_optimized(pixels):
    """
    Converts RGB888 pixels to RGB565 with optimized color mapping for M5Stack display.
    Uses error diffusion dithering for better color representation.
    """
    height, width, _ = pixels.shape
    output = np.zeros((height, width), dtype=np.uint16)
    
    # Create float working copy
    r = pixels[:,:,0].astype(np.float32)
    g = pixels[:,:,1].astype(np.float32)
    b = pixels[:,:,2].astype(np.float32)
    
    # Error diffusion dithering (Floyd-Steinberg)
    for y in range(height):
        for x in range(width):
            # Get current pixel values
            old_r = np.clip(r[y, x], 0, 255)
            old_g = np.clip(g[y, x], 0, 255)
            old_b = np.clip(b[y, x], 0, 255)
            
            # Convert to RGB565 (5-bit R, 6-bit G, 5-bit B)
            new_r = round(old_r / 255.0 * 31) * 8
            new_g = round(old_g / 255.0 * 63) * 4
            new_b = round(old_b / 255.0 * 31) * 8
            
            # Calculate error
            err_r = old_r - new_r
            err_g = old_g - new_g
            err_b = old_b - new_b
            
            # Store converted pixel in output array
            r565 = round(old_r / 255.0 * 31) & 0x1F
            g565 = round(old_g / 255.0 * 63) & 0x3F
            b565 = round(old_b / 255.0 * 31) & 0x1F
            output[y, x] = (r565 << 11) | (g565 << 5) | b565
            
            # Distribute error to neighboring pixels
            if x < width-1:
                r[y, x+1] += err_r * 7/16
                g[y, x+1] += err_g * 7/16
                b[y, x+1] += err_b * 7/16
            
            if y < height-1:
                if x > 0:
                    r[y+1, x-1] += err_r * 3/16
                    g[y+1, x-1] += err_g * 3/16
                    b[y+1, x-1] += err_b * 3/16
                
                r[y+1, x] += err_r * 5/16
                g[y+1, x] += err_g * 5/16
                b[y+1, x] += err_b * 5/16
                
                if x < width-1:
                    r[y+1, x+1] += err_r * 1/16
                    g[y+1, x+1] += err_g * 1/16
                    b[y+1, x+1] += err_b * 1/16
    
    return output

def generate_header_file(rgb565_array, output_file, width, height, variable_name):
    """
    Generates a C header file with the RGB565 image data.
    """
    with open(output_file, "w") as f:
        f.write(f"// M5Stack optimized RGB565 image\n")
        f.write(f"// Image dimensions: {width}x{height} pixels\n")
        f.write(f"// Generated with advanced color correction and dithering\n\n")
        
        f.write(f"#ifndef {os.path.basename(output_file).split('.')[0].upper()}_H\n")
        f.write(f"#define {os.path.basename(output_file).split('.')[0].upper()}_H\n\n")
        
        f.write(f"#include <pgmspace.h>\n\n")
        
        f.write(f"// Image dimensions\n")
        f.write(f"#define {variable_name.upper()}_WIDTH {width}\n")
        f.write(f"#define {variable_name.upper()}_HEIGHT {height}\n\n")
        
        f.write(f"// RGB565 Image data ({width*height} pixels)\n")
        f.write(f"const uint16_t {variable_name}[{width*height}] PROGMEM = {{\n")
        
        # Write image data in rows of 8 values for readability
        values_per_line = 8
        for y in range(height):
            f.write("  ")
            for x in range(0, width, values_per_line):
                chunk = rgb565_array[y, x:min(x+values_per_line, width)]
                hex_values = [f"0x{val:04X}" for val in chunk]
                f.write(", ".join(hex_values))
                
                # Add comma if not last element
                if y < height-1 or x + values_per_line < width:
                    f.write(",")
                    
                # Add newline after every values_per_line elements
                if x + values_per_line < width:
                    f.write("\n  ")
                else:
                    f.write("\n")
        
        f.write("};\n\n")
        f.write("#endif // " + os.path.basename(output_file).split('.')[0].upper() + "_H\n")

if __name__ == "__main__":
    # Example usage
    input_file = "me2.jpg"  # Change to your input file
    output_file = "me2_img.h"
    convert_image_for_m5stack(input_file, output_file)