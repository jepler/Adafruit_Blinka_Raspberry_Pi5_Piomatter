#!/usr/bin/python3
"""
Display an animated gif

Run like this:

$ python play_gif.py

The animated gif is played repeatedly until interrupted with ctrl-c.
"""

import time

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import numpy as np
import PIL.Image as Image

width = 64
height = 32

gif_file = "nyan.gif"

canvas = Image.new('RGB', (width, height), (0, 0, 0))
geometry = piomatter.Geometry(width=width, height=height,
                              n_addr_lines=4, rotation=piomatter.Orientation.Normal)
framebuffer = np.asarray(canvas) + 0  # Make a mutable copy
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.AdafruitMatrixBonnet,
                             framebuffer=framebuffer,
                             geometry=geometry)

with Image.open(gif_file) as img:
    print(f"frames: {img.n_frames}")
    while True:
        for i in range(img.n_frames):
            img.seek(i)
            canvas.paste(img, (0,0))
            framebuffer[:] = np.asarray(canvas)
            matrix.show()
            time.sleep(0.1)
