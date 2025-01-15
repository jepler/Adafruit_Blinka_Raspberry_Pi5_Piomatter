#!/usr/bin/python3
"""
Display a series of 64x32 PNG images as fast as possible

Run like this:

$ python playframes.py "/path/to/images/*.png"

The image files are sorted and then played repeatedly until interrupted with ctrl-c.
"""

import glob
import sys

import adafruit_raspberry_pi5_piomatter
import numpy as np
import PIL.Image as Image

images = sorted(glob.glob(sys.argv[1]))

geometry = adafruit_raspberry_pi5_piomatter.Geometry(width=64, height=32, n_addr_lines=4, rotation=adafruit_raspberry_pi5_piomatter.Orientation.Normal)
framebuffer = np.asarray(Image.open(images[0])) + 0  # Make a mutable copy
matrix = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB888Packed(framebuffer, geometry)

while True:
    for i in images:
        print(i)
        framebuffer[:] = np.asarray(Image.open(i))
        matrix.show()
