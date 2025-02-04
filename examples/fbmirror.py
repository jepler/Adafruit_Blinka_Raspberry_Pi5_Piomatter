#!/usr/bin/python3
"""
Mirror a scaled copy of the framebuffer to a 64x32 matrix

The upper left corner of the framebuffer is displayed until the user hits ctrl-c.

Control matrix size, and orientation with command line arguments.

python fbmirror_scaled.py [width] [height] [orientation]

    width int: Total width of matrices in pixels. Default is 64.
    height int: Total height of matrices in pixels. Default is 32.
    orientation int: Orientation in degrees, must be 0, 90, 180, or 270.
        Default is 0 or Normal orientation.

The `/dev/fb0` special file will exist if a monitor is plugged in at boot time,
or if `/boot/firmware/cmdline.txt` specifies a resolution such as
`...  video=HDMI-A-1:640x480M@60D`.
"""

import sys

import adafruit_raspberry_pi5_piomatter
import numpy as np

width = 64
height = 32

yoffset = 0
xoffset = 0


if len(sys.argv) >= 2:
    width = int(sys.argv[2])
else:
    width = 64

if len(sys.argv) >= 3:
    height = int(sys.argv[3])
else:
    height = 32

if len(sys.argv) >= 4:
    rotation = int(sys.argv[4])
    if rotation == 90:
        rotation = adafruit_raspberry_pi5_piomatter.Orientation.CW
    elif rotation == 180:
        rotation = adafruit_raspberry_pi5_piomatter.Orientation.R180
    elif rotation == 270:
        rotation = adafruit_raspberry_pi5_piomatter.Orientation.CCW
    elif rotation == 0:
        rotation = adafruit_raspberry_pi5_piomatter.Orientation.Normal
    else:
        raise ValueError("Invalid rotation. Must be 0, 90, 180, or 270.")
else:
    rotation = adafruit_raspberry_pi5_piomatter.Orientation.Normal

with open("/sys/class/graphics/fb0/virtual_size") as f:
    screenx, screeny = [int(word) for word in f.read().split(",")]

with open("/sys/class/graphics/fb0/bits_per_pixel") as f:
    bits_per_pixel = int(f.read())

assert bits_per_pixel == 16

bytes_per_pixel = bits_per_pixel // 8
dtype = {2: np.uint16, 4: np.uint32}[bytes_per_pixel]

with open("/sys/class/graphics/fb0/stride") as f:
    stride = int(f.read())

linux_framebuffer = np.memmap('/dev/fb0',mode='r', shape=(screeny, stride // bytes_per_pixel), dtype=dtype)


geometry = adafruit_raspberry_pi5_piomatter.Geometry(width=width, height=height, n_addr_lines=4, rotation=adafruit_raspberry_pi5_piomatter.Orientation.Normal)
matrix_framebuffer = np.zeros(shape=(geometry.height, geometry.width), dtype=dtype)
matrix = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB565(matrix_framebuffer, geometry)

while True:
    matrix_framebuffer[:,:] = linux_framebuffer[yoffset:yoffset+height, xoffset:xoffset+width]
    matrix.show()
