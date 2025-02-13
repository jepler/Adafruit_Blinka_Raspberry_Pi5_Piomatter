#!/usr/bin/python3
# SPDX-FileCopyrightText: 2025 Tim Cocks for Adafruit Industries
#
# SPDX-License-Identifier: MIT
"""
Display a spiral around the display drawn with a rainbow color.

Run like this:

$ python rainbow_spiral.py

"""
import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import numpy as np
import rainbowio
from PIL import Image, ImageDraw

width = 64
height = 32
pen_radius = 1


canvas = Image.new('RGB', (width, height), (0, 0, 0))
draw = ImageDraw.Draw(canvas)

geometry = piomatter.Geometry(width=width, height=height, n_addr_lines=4,
                                                     rotation=piomatter.Orientation.Normal)
framebuffer = np.asarray(canvas) + 0  # Make a mutable copy
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.AdafruitMatrixBonnet,
                             framebuffer=framebuffer,
                             geometry=geometry)

color_index = 0

def update_matrix():
    framebuffer[:] = np.asarray(canvas)
    matrix.show()

def darken_color(hex_color, darkness_factor):
    # Convert hex color number to RGB
    r = (hex_color >> 16) & 0xFF
    g = (hex_color >> 8) & 0xFF
    b = hex_color & 0xFF

    # Apply darkness factor
    r = int(r * (1 - darkness_factor))
    g = int(g * (1 - darkness_factor))
    b = int(b * (1 - darkness_factor))

    # Ensure values are within the valid range
    r = max(0, min(255, r))
    g = max(0, min(255, g))
    b = max(0, min(255, b))

    # Convert RGB back to hex number
    darkened_hex_color = (r << 16) + (g << 8) + b

    return darkened_hex_color

step_count = 4
darkness_factor = 0.5

clearing = False

try:
    # step_down_size = pen_radius * 2 + 2

    while True:
        for step in range(step_count):
            step_down_size = step * (pen_radius* 2) + (2 * step)
            for x in range(pen_radius + step_down_size, width - pen_radius - step_down_size - 1):
                color_index = (color_index + 2) % 256
                color = darken_color(rainbowio.colorwheel(color_index), darkness_factor) if not clearing else 0x000000
                draw.circle((x, pen_radius + step_down_size), pen_radius, color)
                update_matrix()
            for y in range(pen_radius + step_down_size, height - pen_radius - step_down_size - 1):
                color_index = (color_index + 2) % 256
                color = darken_color(rainbowio.colorwheel(color_index), darkness_factor) if not clearing else 0x000000
                draw.circle((width - pen_radius - step_down_size -1, y), pen_radius, color)
                update_matrix()
            for x in range(width - pen_radius - step_down_size - 1, pen_radius + step_down_size, -1):
                color_index = (color_index + 2) % 256
                color = darken_color(rainbowio.colorwheel(color_index), darkness_factor) if not clearing else 0x000000
                draw.circle((x, height - pen_radius - step_down_size - 1), pen_radius, color)
                update_matrix()
            for y in range(height - pen_radius - step_down_size - 1, pen_radius + ((step+1) * (pen_radius* 2) + (2 * (step+1))) -1, -1):
                color_index = (color_index + 2) % 256
                color = darken_color(rainbowio.colorwheel(color_index), darkness_factor) if not clearing else 0x000000
                draw.circle((pen_radius + step_down_size, y), pen_radius, color)
                update_matrix()

            if step != step_count-1:
                # connect to next iter
                for x in range(pen_radius + step_down_size, pen_radius + ((step+1) * (pen_radius* 2) + (2 * (step+1)))):
                    color_index = (color_index + 2) % 256
                    color = darken_color(rainbowio.colorwheel(color_index),
                                         darkness_factor) if not clearing else 0x000000
                    draw.circle((x, pen_radius + ((step+1) * (pen_radius* 2) + (2 * (step+1)))), pen_radius, color)
                    update_matrix()

        clearing = not clearing

except KeyboardInterrupt:
    print("Exiting")
