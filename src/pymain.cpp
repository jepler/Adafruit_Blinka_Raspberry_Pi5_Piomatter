#include <iostream>
#include <pybind11/pybind11.h>

#include "piomatter/piomatter.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

namespace {}

PYBIND11_MODULE(adafruit_raspberry_pi5_piomatter, m) {
    m.doc() = R"pbdoc(
        hub75 matrix driver for pi5 using pio
        -------------------------------------

        .. currentmodule:: adafruit_raspberry_pi5_piomatter

        .. autosummary::
           :toctree: _generate

           Piomatter
    )pbdoc";

#if 0
    py::enum_<piomatter::orientation>(m, "Orientation", py::doc("describes the orientation of a panel"))
        .value("Normal", piomatter::orientation::normal)
        .value("R180", piomatter::orientation::r180)
        .value("CCW", piomatter::orientation::ccw)
        .value("CW", piomatter::orientation::cw)
    ;
#endif

    py::class_<piomatter::matrix_geometry>(m, "Geometry"
#if 0
        , py::doc("Describes the geometry of a panel")
#endif
                                           )
        .def(py::init([](size_t width, size_t height, size_t pixels_across,
                         size_t n_addr_lines, bool serpentine, int rotation,
                         size_t n_planes) {
                 switch (rotation) {
                 case 0:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_normal);

                 case 180:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_r180);

                 case 90:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_ccw);

                 case 270:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes, width, height,
                         serpentine, piomatter::orientation_cw);
                 }
                 throw std::runtime_error("invalid orientation");
             })
#if 1
                 ,
             py::arg("width"), py::arg("height"), py::arg("pixels_across"),
             py::arg("n_addr_lines"), py::arg("serpentine") = false,
             py::arg("orientation") = 0u, py::arg("n_planes") = 10u
#endif
             )
        .def_readonly("width", &piomatter::matrix_geometry::width)
        .def_readonly("hieght", &piomatter::matrix_geometry::height);

#if 0
    py::class_<piomatter::piomatter_base>(m, "PiomatterBase", py::doc("Base class of all pinouts"))
        .def("show", &piomatter_base::show)
    ;
#endif

    using AdafruitMatrixBonnetRGB888 =
        piomatter::piomatter<piomatter::adafruit_matrix_bonnet_pinout,
                             piomatter::colorspace_rgb888>;
    py::class_<AdafruitMatrixBonnetRGB888>(
        m, "AdafruitMatrixBonnetRGB888") // .def(py::init<py::buffer,
                                         // matrix_geometry>())
        .def("show", &AdafruitMatrixBonnetRGB888::show);

#if 0
#define pinout(pinoutname, colorspace, pyclassname)                            \
    py::class_<piomatter<pinoutname, colorspace>, piomatter_base>(m,           \
                                                                  pyclassname) \
        .def(py::init<matrix_geometry, py::buffer>())

    pinout(adafruit_matrix_bonnet_pinout, colorspace_rgb888, "AdafruitMatrixBonnetRGB888");
    //pinout(adafruit_matrix_bonnet_pinout, colorspace_rgb888_packed, "AdafruitMatrixBonnetRGB888Packed");
#endif
}
