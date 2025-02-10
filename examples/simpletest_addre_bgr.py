#!/usr/bin/python3
"""
Display a static 64x64 image

This assumes a 64x64 matrix with "BGR" pixel order, such as https://www.adafruit.com/product/5362

Run like this:

$ python simpletest.py

The image is displayed until the user hits enter to exit.
"""

import pathlib

import adafruit_raspberry_pi5_piomatter as piomatter
import numpy as np
import PIL.Image as Image

geometry = piomatter.Geometry(width=64, height=64, n_addr_lines=5, rotation=piomatter.Orientation.Normal, n_planes=8)
framebuffer = np.asarray(Image.open(pathlib.Path(__file__).parent / "blinka64x64.png"))
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed, pinout=piomatter.Pinout.AdafruitMatrixBonnetBGR, framebuffer=framebuffer, geometry=geometry)
matrix.show()

input("Hit enter to exit")
