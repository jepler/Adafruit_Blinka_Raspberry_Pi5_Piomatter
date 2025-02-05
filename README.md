Adafruit-Blinka-Raspberry-Pi5-Piomatter
======================================

|      CI              | status |
|----------------------|--------|
| pip builds           | [![Pip Actions Status][actions-pip-badge]][actions-pip-link] |
| [`cibuildwheel`][]   | [![Wheels Actions Status][actions-wheels-badge]][actions-wheels-link] |

[actions-badge]:           https://github.com/adafruit/Adafruit_Blinka_Raspberry_Pi5_Piomatter/workflows/Tests/badge.svg
[actions-pip-link]:        https://github.com/adafruit/Adafruit_Blinka_Raspberry_Pi5_Piomatter/actions?query=workflow%3A%22Pip
[actions-pip-badge]:       https://github.com/adafruit/Adafruit_Blinka_Raspberry_Pi5_Piomatter/workflows/Pip/badge.svg
[actions-wheels-link]:     https://github.com/adafruit/Adafruit_Blinka_Raspberry_Pi5_Piomatter/actions?query=workflow%3AWheels
[actions-wheels-badge]:    https://github.com/adafruit/Adafruit_Blinka_Raspberry_Pi5_Piomatter/workflows/Wheels/badge.svg

Learn Guide
-----------
[RGB Matrix Panels With Raspberry Pi 5](https://learn.adafruit.com/rgb-matrix-panels-with-raspberry-pi-5)

Installation
------------

Installing from source:

 - clone this repository
 - `pip install ./Adafruit_Blinka_Raspberry_Pi5_Piomatter`

Installing from pip:

 - `pip install Adafruit-Blinka-Raspberry-Pi5-Piomatter`

System setup
------------

If `ls -l /dev/pio0` reports that the file is not found, you may need to update your Pi 5 firmware to one with PIO support and make sure that you are running a suitably recent kernel. If `ls -l /dev/pio0` reports that the file is owned by root and group root, you should add the following to /etc/udev/rules.d/99-com.rules:

```
SUBSYSTEM=="*-pio", GROUP="gpio", MODE="0660"
```

Building the documentation
--------------------------

Documentation for the example project is generated using Sphinx. Sphinx has the
ability to automatically inspect the signatures and documentation strings in
the extension module to generate beautiful documentation in a variety formats.
The following command generates HTML-based reference documentation; for other
formats please refer to the Sphinx manual:

 - `cd Adafruit_Blinka_Raspberry_Pi5_Piomatter/docs`
 - `make html`

License
-------

Adafruit\_Blinka\_Raspberry\_Pi5\_Piomatter is provided under the GPL-2-only license that can be found in the LICENSE
file. By using, distributing, or contributing to this project, you agree to the
terms and conditions of this license.

[`cibuildwheel`]:          https://cibuildwheel.readthedocs.io
