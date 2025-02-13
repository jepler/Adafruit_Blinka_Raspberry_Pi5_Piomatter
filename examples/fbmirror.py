#!/usr/bin/python3
"""
Mirror a scaled copy of the framebuffer to a matrix

A portion of the framebuffer is displayed until the user hits ctrl-c.

The `/dev/fb0` special file will exist if a monitor is plugged in at boot time,
or if `/boot/firmware/cmdline.txt` specifies a resolution such as
`...  video=HDMI-A-1:640x480M@60D`.

For help with commandline arguments, run `python fbmirror.py --help`
"""


import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import click
import numpy as np
import piomatter_click

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

@click.command
@click.option("--x-offset", "xoffset", type=int, help="The x offset of top left corner of the region to mirror",  default=0)
@click.option("--y-offset", "yoffset", type=int, help="The y offset of top left corner of the region to mirror", default=0)
@piomatter_click.standard_options
def main(xoffset, yoffset, width, height, serpentine, rotation, pinout, n_planes, n_addr_lines):
    geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines, rotation=rotation)
    framebuffer = np.zeros(shape=(geometry.height, geometry.width), dtype=dtype)
    matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB565, pinout=pinout, framebuffer=framebuffer, geometry=geometry)

    while True:
        framebuffer[:,:] = linux_framebuffer[yoffset:yoffset+height, xoffset:xoffset+width]
        matrix.show()

if __name__ == '__main__':
    main()
