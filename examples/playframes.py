#!/usr/bin/python3
"""
Display a series of 64x32 PNG images as fast as possible

Run like this:

$ python playframes.py "/path/to/images/*.png"

The image files are sorted and then played repeatedly until interrupted with ctrl-c.
"""

import glob
import sys
import time

import adafruit_raspberry_pi5_piomatter as piomatter
import numpy as np
import PIL.Image as Image

images = sorted(glob.glob(sys.argv[1]))

geometry = piomatter.Geometry(width=64, height=32, n_addr_lines=4, rotation=piomatter.Orientation.Normal)
framebuffer = np.asarray(Image.open(images[0])) + 0  # Make a mutable copy
nimages = len(images)
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.AdafruitMatrixBonnet,
                             framebuffer=framebuffer,
                             geometry=geometry)

while True:
    t0 = time.monotonic()
    for i in images:
        framebuffer[:] = np.asarray(Image.open(i))
        matrix.show()
    t1 = time.monotonic()
    dt = t1 - t0
    fps = nimages/dt
    print(f"{nimages} frames in {dt}s, {fps}fps [{matrix.fps}]")
