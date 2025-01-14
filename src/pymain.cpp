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
};

template <typename pinout, typename colorspace>
std::unique_ptr<PyPiomatter>
make_piomatter(py::buffer buffer, const piomatter::matrix_geometry &geometry) {
    using cls = piomatter::piomatter<pinout, colorspace>;
    using data_type = colorspace::data_type;

    const auto n_pixels = geometry.width * geometry.height;
    const auto data_size_in_bytes = colorspace::data_size_in_bytes(n_pixels);
    const py::buffer_info info = buffer.request();
    const size_t buffer_size_in_bytes = info.size * info.itemsize;

    if (buffer_size_in_bytes != data_size_in_bytes) {
        throw std::runtime_error(
            py::str(
                "Framebuffer size must be {} bytes, got a buffer of {} bytes")
                .attr("format")(data_size_in_bytes, buffer_size_in_bytes)
                .template cast<std::string>());
    }

    std::span<data_type> framebuffer(reinterpret_cast<data_type *>(info.ptr),
                                     data_size_in_bytes / sizeof(data_type));
    return std::make_unique<PyPiomatter>(
        buffer, std::move(std::make_unique<cls>(framebuffer, geometry)));
}
} // namespace

PYBIND11_MODULE(adafruit_raspberry_pi5_piomatter, m) {
    m.doc() = R"pbdoc(
        HUB75 matrix driver for Raspberry Pi 5 using PIO
        ------------------------------------------------

        .. currentmodule:: adafruit_raspberry_pi5_piomatter

        .. autosummary::
           :toctree: _generate

           Piomatter
    )pbdoc";

    py::enum_<piomatter::orientation>(m, "Orientation")
        .value("Normal", piomatter::orientation::normal)
        .value("R180", piomatter::orientation::r180)
        .value("CCW", piomatter::orientation::ccw)
        .value("CW", piomatter::orientation::cw)
        //.doc() = "Describes the orientation of a panel"
        ;

    py::class_<piomatter::matrix_geometry>(m, "Geometry")
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
        .def_readonly("hieght", &piomatter::matrix_geometry::height)
        //.doc() = "Describes the geometry of a panel"
        ;

    py::class_<PyPiomatter>(m, "PioMatter").def("show", &PyPiomatter::show)
        //.doc() = "HUB75 matrix driver for Raspberry Pi 5 using PIO"
        ;

    m.def("AdafruitMatrixBonnetRGB888",
          make_piomatter<piomatter::adafruit_matrix_bonnet_pinout,
                         piomatter::colorspace_rgb888>,
          py::arg("buffer"), py::arg("geometry"))
        //.doc() = "Drive panels connected to an Adafruit Matrix Bonnet using
        // the RGB888 memory layout (4 bytes per pixel)"
        ;

    m.def("AdafruitMatrixBonnetRGB888Packed",
          make_piomatter<piomatter::adafruit_matrix_bonnet_pinout,
                         piomatter::colorspace_rgb888_packed>,
          py::arg("buffer"), py::arg("geometry"))
        // .doc() = "Drive panels connected to an Adafruit Matrix Bonnet using
        // the RGB888 packed memory layout (3 bytes per pixel)"
        ;
}
