#!/usr/bin/python3
"""
Display a static 64x64 image

This assumes two 64x32 matrix panels are hooked together in the "serpentine" configuration.

Run like this:

$ python simpletest.py

The image is displayed until the user hits enter to exit.
"""

import pathlib

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import numpy as np
import PIL.Image as Image

geometry = piomatter.Geometry(width=64, height=64, n_addr_lines=4, rotation=piomatter.Orientation.Normal)
framebuffer = np.asarray(Image.open(pathlib.Path(__file__).parent / "blinka64x64.png"))
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.AdafruitMatrixBonnet,
                             framebuffer=framebuffer,
                             geometry=geometry)
matrix.show()

input("Hit enter to exit")
