#!/usr/bin/python3
"""
Mirror a scaled copy of the framebuffer to 64x32 matrices,

The upper left corner of the framebuffer is displayed until the user hits ctrl-c.

Control scale, matrix size, and orientation with command line arguments.

python fbmirror_scaled.py [scale] [width] [height] [orientation]

    scale int: How many times to scale down the display framebuffer. Default is 3.
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
import PIL.Image as Image

if len(sys.argv) >= 2:
    scale = int(sys.argv[1])
else:
    scale = 3

if len(sys.argv) >= 3:
    width = int(sys.argv[2])
else:
    width = 64

if len(sys.argv) >= 4:
    height = int(sys.argv[3])
else:
    height = 32

if len(sys.argv) >= 5:
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

xoffset = 0
yoffset = 0

geometry = adafruit_raspberry_pi5_piomatter.Geometry(width=width, height=height, n_addr_lines=4, rotation=rotation)
matrix_framebuffer = np.zeros(shape=(geometry.height, geometry.width, 3), dtype=np.uint8)
matrix = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB888Packed(matrix_framebuffer, geometry)

while True:
    tmp = linux_framebuffer[yoffset:yoffset+height*scale, xoffset:xoffset+width*scale]
    # Convert the RGB565 framebuffer into RGB888Packed (so that we can use PIL image operations to rescale it)
    r = (tmp & 0xf800) >> 8
    r = r | (r >> 5)
    r = r.astype(np.uint8)
    g = (tmp & 0x07e0) >> 3
    g = g | (g >> 6)
    g = g.astype(np.uint8)
    b = (tmp & 0x001f) << 3
    b = b | (b >> 5)
    b = b.astype(np.uint8)
    img = Image.fromarray(np.stack([r, g, b], -1))
    img = img.resize((width, height))
    matrix_framebuffer[:,:] = np.asarray(img)
    matrix.show()
