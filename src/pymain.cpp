#include <iostream>
#include <pybind11/pybind11.h>
#include <string>

#include "piomatter/piomatter.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

namespace {
struct PyPiomatter {
    PyPiomatter(py::buffer buffer,
                std::unique_ptr<piomatter::piomatter_base> &&matter)
        : buffer{buffer}, matter{std::move(matter)} {}
    py::buffer buffer;
    std::unique_ptr<piomatter::piomatter_base> matter;

    void show() { matter->show(); }
    double fps() const { return matter->fps; }
};

template <typename pinout, typename colorspace>
std::unique_ptr<PyPiomatter>
make_piomatter_pc(py::buffer buffer,
                  const piomatter::matrix_geometry &geometry) {
    using cls = piomatter::piomatter<pinout, colorspace>;
    using data_type = colorspace::data_type;

    const auto n_pixels = geometry.width * geometry.height;
    const auto data_size_in_bytes = colorspace::data_size_in_bytes(n_pixels);
    const py::buffer_info info = buffer.request();
    const size_t buffer_size_in_bytes = info.size * info.itemsize;

    if (buffer_size_in_bytes != data_size_in_bytes) {
        throw std::runtime_error(
            py::str("Framebuffer size must be {} bytes ({} elements of {} "
                    "bytes each), got a buffer of {} bytes")
                .attr("format")(data_size_in_bytes, n_pixels,
                                colorspace::data_size_in_bytes(1),
                                buffer_size_in_bytes)
                .template cast<std::string>());
    }

    std::span<data_type> framebuffer(reinterpret_cast<data_type *>(info.ptr),
                                     data_size_in_bytes / sizeof(data_type));
    return std::make_unique<PyPiomatter>(
        buffer, std::move(std::make_unique<cls>(framebuffer, geometry)));
}

enum Colorspace { RGB565, RGB888, RGB888Packed };

enum Pinout {
    AdafruitMatrixBonnet,
    AdafruitMatrixBonnetBGR,
};

template <class pinout>
std::unique_ptr<PyPiomatter>
make_piomatter_p(Colorspace c, py::buffer buffer,
                 const piomatter::matrix_geometry &geometry) {
    switch (c) {
    case RGB565:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb565>(
            buffer, geometry);
    case RGB888:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb888>(
            buffer, geometry);
    case RGB888Packed:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb888_packed>(
            buffer, geometry);

    default:
        throw std::runtime_error(py::str("Invalid colorspace {!r}")
                                     .attr("format")(c)
                                     .template cast<std::string>());
    }
}

std::unique_ptr<PyPiomatter>
make_piomatter(Colorspace c, Pinout p, py::buffer buffer,
               const piomatter::matrix_geometry &geometry) {
    switch (p) {
    case AdafruitMatrixBonnet:
        return make_piomatter_p<piomatter::adafruit_matrix_bonnet_pinout>(
            c, buffer, geometry);
    case AdafruitMatrixBonnetBGR:
        return make_piomatter_p<piomatter::adafruit_matrix_bonnet_pinout_bgr>(
            c, buffer, geometry);
    default:
        throw std::runtime_error(py::str("Invalid pinout {!r}")
                                     .attr("format")(p)
                                     .template cast<std::string>());
    }
}
} // namespace

PYBIND11_MODULE(adafruit_raspberry_pi5_piomatter, m) {
    py::options options;
    options.enable_enum_members_docstring();
    options.enable_function_signatures();
    options.enable_user_defined_docstrings();

    m.doc() = R"pbdoc(
        HUB75 matrix driver for Raspberry Pi 5 using PIO
        ------------------------------------------------

        .. currentmodule:: adafruit_raspberry_pi5_piomatter

        .. autosummary::
           :toctree: _generate

           Orientation
           Pinout
           Colorspace
           Geometry
           PioMatter
           AdafruitMatrixBonnetRGB888
           AdafruitMatrixBonnetRGB888Packed
    )pbdoc";

    py::enum_<piomatter::orientation>(
        m, "Orientation", "Describe the orientation of a set of panels")
        .value("Normal", piomatter::orientation::normal, "Normal orientation")
        .value("R180", piomatter::orientation::r180, "Rotated 180 degrees")
        .value("CCW", piomatter::orientation::ccw,
               "Rotated 90 degress counterclockwise")
        .value("CW", piomatter::orientation::cw,
               "Rotated 90 degress clockwise");

    py::enum_<Pinout>(
        m, "Pinout", "Describes the pins used for the connection to the matrix")
        .value("AdafruitMatrixBonnet", Pinout::AdafruitMatrixBonnet,
               "Adafruit Matrix Bonnet or Matrix Hat")
        .value("AdafruitMatrixBonnetBGR", Pinout::AdafruitMatrixBonnetBGR,
               "Adafruit Matrix Bonnet or Matrix Hat with BGR color order")
        .value("AdafruitMatrixHat", Pinout::AdafruitMatrixBonnet,
               "Adafruit Matrix Bonnet or Matrix Hat")
        .value("AdafruitMatrixHatBGR", Pinout::AdafruitMatrixBonnet,
               "Adafruit Matrix Bonnet or Matrix Hat with BGR color order");

    py::enum_<Colorspace>(
        m, "Colorspace",
        "Describes the organization of the graphics data in memory")
        .value("RGB888Packed", Colorspace::RGB888Packed,
               "3 bytes per pixel in RGB order")
        .value("RGB888", Colorspace::RGB888, "4 bytes per pixel in RGB order")
        .value("RGB565", Colorspace::RGB565, "2 bytes per pixel in RGB order");

    py::class_<piomatter::matrix_geometry>(m, "Geometry", R"pbdoc(
Describe the geometry of a set of panels

``width`` and ``height`` give the panel resolution in pixels.

``n_addr_lines`` gives the number of connected address lines.

The number of pixels in the shift register is automatically computed from these values.

``serpentine`` controls the arrangement of multiple panels when they are stacked in rows.
If it is `True`, then each row goes in the opposite direction of the previous row.

``n_planes`` controls the color depth of the panel. This is separate from the framebuffer
layout. Decreasing ``n_planes`` can increase FPS at the cost of reduced color fidelity.
The default, 10, is the maximum value.
)pbdoc")
        .def(py::init([](size_t width, size_t height, size_t n_addr_lines,
                         bool serpentine, piomatter::orientation rotation,
                         size_t n_planes) {
                 size_t n_lines = 2 << n_addr_lines;
                 size_t pixels_across = width * height / n_lines;
                 size_t odd = (width * height) % n_lines;
                 if (odd) {
                     throw std::runtime_error(
                         py::str(
                             "Total pixel count {} must be a multiple of {}, "
                             "the number of distinct row addresses for {}")
                             .attr("format")(width * height, n_lines,
                                             n_addr_lines)
                             .cast<std::string>());
                 }
                 switch (rotation) {
                 case piomatter::orientation::normal:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_normal);

                 case piomatter::orientation::r180:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_r180);

                 case piomatter::orientation::ccw:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_ccw);

                 case piomatter::orientation::cw:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_cw);
                 }
                 throw std::runtime_error("invalid rotation");
             }),
             py::arg("width"), py::arg("height"), py::arg("n_addr_lines"),
             py::arg("serpentine") = true,
             py::arg("rotation") = piomatter::orientation::normal,
             py::arg("n_planes") = 10u)
        .def_readonly("width", &piomatter::matrix_geometry::width)
        .def_readonly("height", &piomatter::matrix_geometry::height);

    py::class_<PyPiomatter>(m, "PioMatter", R"pbdoc(
HUB75 matrix driver for Raspberry Pi 5 using PIO
)pbdoc")
        .def(py::init(&make_piomatter), py::arg("colorspace"),
             py::arg("pinout"), py::arg("framebuffer"), py::arg("geometry"))
        .def("show", &PyPiomatter::show, R"pbdoc(
Update the displayed image

After modifying the content of the framebuffer, call this method to
update the data actually displayed on the panel. Internally, the
data is triple-buffered to prevent tearing.
)pbdoc")
        .def_property_readonly("fps", &PyPiomatter::fps, R"pbdoc(
The approximate number of matrix refreshes per second.
)pbdoc");

    m.def(
        "AdafruitMatrixBonnetRGB565",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB565,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("buffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB565 memory layout (2 bytes per
pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB565, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc");

    m.def(
        "AdafruitMatrixBonnetRGB888",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB888,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("framebuffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB888 memory layout (4 bytes per
pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB888, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc")
        //.doc() =
        ;

    m.def(
        "AdafruitMatrixBonnetRGB888Packed",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB888Packed,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("framebuffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB888 packed memory layout (3
bytes per pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB888Packed, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc");
}
