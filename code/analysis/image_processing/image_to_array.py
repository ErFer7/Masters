from struct import pack
from PIL import Image
from sys import argv


def convert_image_to_c_array(image_path: str, array_name: str, add_size: bool = False) -> str:
    img = Image.open(image_path).convert('L')
    width, height = img.size

    content = ''

    if add_size:
        content += f'const unsigned int IMG_WIDTH = {width};\n'
        content += f'const unsigned int IMG_HEIGHT = {height};\n\n'

    pixels = list(img.getdata())

    signed_chars = []

    dimensions = pack('<ii', width, height)

    pixel_data = pack(f'<{len(pixels)}i', *pixels)

    for b in dimensions + pixel_data:
        signed_chars.append(b - 256 if b > 127 else b)

    content += f'const signed char {array_name}[] = {{ {signed_chars[0]},\n'
    sliced_signed_chars = signed_chars[1:]
    sliced_signed_chars_len = len(sliced_signed_chars)

    for i, signed_char in enumerate(sliced_signed_chars):
        print(f'Creating array: {i / sliced_signed_chars_len:.2%}')
        content += 29 * ' ' + f'{signed_char}'

        if i < sliced_signed_chars_len - 1:
            content += ',\n'
        else:
            content += ' };\n'

    return content


if __name__ == '__main__':
    content = '#pragma once\n\n'

    content += convert_image_to_c_array(argv[1], 'img1', True)
    content += convert_image_to_c_array(argv[2], 'img2')

    with open(argv[3], 'w') as file:
        file.write(content)
