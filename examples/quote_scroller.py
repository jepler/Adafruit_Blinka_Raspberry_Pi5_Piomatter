import requests
from PIL import ImageFont, ImageDraw, Image
import adafruit_raspberry_pi5_piomatter
import numpy as np
import time

# Load the font
font = ImageFont.truetype("LindenHill-webfont.ttf", 26)  # Replace "arial.ttf" with your desired font file

# Text to measure
#text = "Hello, World!"


quote_resp = requests.get("https://www.adafruit.com/api/quotes.php").json()

text = f"{quote_resp[0]["text"]} - {quote_resp[0]["author"]}"

#print(font.getbbox(text))
x, y, text_width, text_height = font.getbbox(text)

full_txt_img = Image.new("RGB", (int(text_width) + 6, int(text_height) + 6), (0, 0, 0))
draw = ImageDraw.Draw(full_txt_img)
draw.text((3, 3), text, font=font, fill=(255, 0, 255))
#img.save("quote.png")

single_frame_img = Image.new("RGB", (64, 32), (0, 0, 0))

geometry = adafruit_raspberry_pi5_piomatter.Geometry(width=64, height=64, n_addr_lines=4, rotation=adafruit_raspberry_pi5_piomatter.Orientation.R180)
framebuffer = np.asarray(single_frame_img) + 0  # Make a mutable copy

matrix = adafruit_raspberry_pi5_piomatter.AdafruitMatrixBonnetRGB888Packed(framebuffer, geometry)


for x_pixel in text_width:
    single_frame_img = full_txt_img.crop(x_pixel, 0, 64, 32)
    framebuffer[:] = np.asarray(single_frame_img)
    matrix.show()
    time.sleep(0.01)