#!/usr/bin/python3
"""
Mirror a scaled copy of the framebuffer to RGB matrices,

A portion of the framebuffer is displayed until the user hits ctrl-c.

Control scale, matrix size, and orientation with command line arguments.

Usage: fbmirror_scaled.py [OPTIONS]

Options:
  --x-offset INTEGER              The x offset of top left corner of the
                                  region to mirror
  --y-offset INTEGER              The y offset of top left corner of the
                                  region to mirror
  --scale INTEGER                 The scale factor to reduce the display down
                                  by.
  --num-address-lines INTEGER     The number of address lines used by the
                                  panels
  --num-planes INTEGER            The number of bit planes (color depth. Lower
                                  values can improve refresh rate in frames
                                  per second
  --orientation [Normal|R180|CCW|CW]
                                  The overall orientation (rotation) of the
                                  panels
  --pinout [AdafruitMatrixBonnet|AdafruitMatrixBonnetBGR|AdafruitMatrixHat|AdafruitMatrixHatBGR]
                                  The details of the electrical connection to
                                  the panels
  --serpentine / --no-serpentine  The organization of multiple panels
  --height INTEGER                The panel height in pixels
  --width INTEGER                 The panel width in pixels
  --help                          Show this message and exit.


The `/dev/fb0` special file will exist if a monitor is plugged in at boot time,
or if `/boot/firmware/cmdline.txt` specifies a resolution such as
`...  video=HDMI-A-1:640x480M@60D`.
"""

import adafruit_raspberry_pi5_piomatter as piomatter
import click
import numpy as np
import PIL.Image as Image
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
@click.option("--scale", "scale", type=int, help="The scale factor to reduce the display down by.", default=3)
@piomatter_click.standard_options
def main(xoffset, yoffset, scale, width, height, serpentine, rotation, pinout, n_planes, n_addr_lines):
    geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines, rotation=rotation)
    matrix_framebuffer = np.zeros(shape=(geometry.height, geometry.width, 3), dtype=np.uint8)
    matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed, pinout=pinout, framebuffer=matrix_framebuffer, geometry=geometry)

    while True:
        tmp = linux_framebuffer[yoffset:yoffset + height * scale, xoffset:xoffset + width * scale]
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
        matrix_framebuffer[:, :] = np.array(img)
        matrix.show()

if __name__ == '__main__':
    main()
