import re
import struct
from PIL import Image
from sys import argv


def convert_c_array_to_image(input_path, output_path):
    with open(input_path, 'r') as f:
        content = f.read()

    match = re.search(r'\{(.*?)\}', content, re.DOTALL)
    if not match:
        raise ValueError('Could not find array data within curly braces.')

    raw_strings = match.group(1).split(',')
    signed_chars = [int(x.strip()) for x in raw_strings if x.strip()]

    unsigned_bytes = bytearray([x + 256 if x < 0 else x for x in signed_chars])

    num_ints = len(unsigned_bytes) // 4
    integers = struct.unpack(f'<{num_ints}i', unsigned_bytes)

    width = integers[0]
    height = integers[1]
    pixels = integers[2:]

    if len(pixels) != width * height:
        print(f'Warning: Expected {width * height} pixels, but found {len(pixels)}.')

    img = Image.new('L', (width, height))

    img.putdata(pixels)

    img.save(output_path)
    print(f"Saved reconstructed image to '{output_path}' (Size: {width}x{height})")


if __name__ == '__main__':
    convert_c_array_to_image(argv[1], 'reconstructed_image2.png')
