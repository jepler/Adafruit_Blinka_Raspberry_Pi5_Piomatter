import adafruit_raspberry_pi5_piomatter
import numpy as np
import PIL.Image as Image

g = adafruit_raspberry_pi5_piomatter.Geometry(64, 64, 4, rotation=adafruit_raspberry_pi5_piomatter.Orientation.Normal)
arr = np.asarray(Image.open("blinka64x64.png"))
m = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB888Packed(arr, g)
m.show()

input("Hit enter to exit")
