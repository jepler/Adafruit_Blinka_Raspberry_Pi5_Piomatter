#!/usr/bin/python3
# SPDX-FileCopyrightText: 2025 Tim Cocks for Adafruit Industries
#
# SPDX-License-Identifier: MIT
"""
Display quote from the Adafruit quotes API as text scrolling across the
matrices.

Requires the requests library to be installed.

Run like this:

$ python quote_scroller.py

"""

import adafruit_raspberry_pi5_piomatter as piomatter
import numpy as np
import requests
from PIL import Image, ImageDraw, ImageFont

# 128px for 2x1 matrices. Change to 64 if you're using a single matrix.
total_width = 128
total_height = 32

bottom_half_shift_compensation = 1

font_color = (0, 128, 128)

# Load the font
font = ImageFont.truetype("LindenHill-webfont.ttf", 26)

quote_resp = requests.get("https://www.adafruit.com/api/quotes.php").json()

text = f'{quote_resp[0]["text"]} - {quote_resp[0]["author"]}'
#text = "Sometimes you just want to use hardcoded strings. - Unknown"

x, y, text_width, text_height = font.getbbox(text)

full_txt_img = Image.new("RGB", (int(text_width) + 6, int(text_height) + 6), (0, 0, 0))
draw = ImageDraw.Draw(full_txt_img)
draw.text((3, 3), text, font=font, fill=font_color)
full_txt_img.save("quote.png")

single_frame_img = Image.new("RGB", (total_width, total_height), (0, 0, 0))

geometry = piomatter.Geometry(width=total_width, height=total_height,
                              n_addr_lines=4, rotation=piomatter.Orientation.Normal)
framebuffer = np.asarray(single_frame_img) + 0  # Make a mutable copy

matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.AdafruitMatrixBonnet,
                             framebuffer=framebuffer,
                             geometry=geometry)

print("Ctrl-C to exit")
while True:
    for x_pixel in range(-total_width-1,full_txt_img.width):
        if bottom_half_shift_compensation == 0:
            # full paste
            single_frame_img.paste(full_txt_img.crop((x_pixel, 0, x_pixel + total_width, total_height)), (0, 0))

        else:
            # top half
            single_frame_img.paste(full_txt_img.crop((x_pixel, 0, x_pixel + total_width, total_height//2)), (0, 0))
            # bottom half shift compensation
            single_frame_img.paste(full_txt_img.crop((x_pixel, total_height//2, x_pixel + total_width, total_height)), (bottom_half_shift_compensation, total_height//2))

        framebuffer[:] = np.asarray(single_frame_img)
        matrix.show()
