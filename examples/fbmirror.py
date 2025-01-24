#!/usr/bin/python3
"""
Mirror a scaled copy of the framebuffer to a 64x32 matrix

The upper left corner of the framebuffer is displayed until the user hits ctrl-c.

The `/dev/fb0` special file will exist if a monitor is plugged in at boot time,
or if `/boot/firmware/cmdline.txt` specifies a resolution such as
`...  video=HDMI-A-1:640x480M@60D`.
"""


import adafruit_raspberry_pi5_piomatter
import numpy as np

yoffset = 0
xoffset = 0

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

width = 64
height = 32
geometry = adafruit_raspberry_pi5_piomatter.Geometry(width=width, height=height, n_addr_lines=4, rotation=adafruit_raspberry_pi5_piomatter.Orientation.Normal)
matrix_framebuffer = np.zeros(shape=(geometry.height, geometry.width), dtype=dtype)
matrix = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB565(matrix_framebuffer, geometry)

while True:
    matrix_framebuffer[:,:] = linux_framebuffer[yoffset:yoffset+height, xoffset:xoffset+width]
    matrix.show()
