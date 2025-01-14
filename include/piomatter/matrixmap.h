#pragma once

#include <stdexcept>
#include <vector>

namespace piomatter {

using matrix_map = std::vector<int>;

int orientation_normal(int width, int height, int x, int y) {
    return x + width * y;
}

int orientation_r180(int width, int height, int x, int y) {
    x = width - x - 1;
    y = height - y - 1;
    return orientation_normal(width, height, x, y);
}

int orientation_ccw(int width, int height, int x, int y) {
    return orientation_normal(height, width, y, width - x - 1);
}

int orientation_cw(int width, int height, int x, int y) {
    return orientation_normal(height, width, y - height - 1, x);
}

namespace {
template <typename Cb>
void submap(std::vector<int> &result, int width, int height, int start_x,
            int dx, int count_x_in, int start_y, int dy, int count_y,
            int half_panel_height, const Cb &cb) {

    for (int y = start_y; count_y; count_y -= 2, y += dy) {
        for (int x = start_x, count_x = count_x_in; count_x--; x += dx) {
            result.push_back(cb(width, height, x, y));
            result.push_back(cb(width, height, x, y + dy * half_panel_height));
        }
    }
}
} // namespace

template <typename Cb>
matrix_map make_matrixmap(int width, int height, int n_addr_lines,
                          bool serpentine, const Cb &cb) {

    int panel_height = 2 << n_addr_lines;
    if (height % panel_height != 0) {
        throw std::range_error("Height does not evenly divide panel height");
    }

    int half_panel_height = 1 << n_addr_lines;
    int v_panels = height / panel_height;
    int pixels_across = width * v_panels;
    matrix_map result;
    result.reserve(width * height);

    for (int i = 0; i < half_panel_height; i++) {
        for (int j = 0; j < pixels_across; j++) {
            int panel_no = j / width;
            int panel_idx = j % width;
            int x, y0, y1;

            if (serpentine && panel_no % 2) {
                x = width - panel_idx - 1;
                y0 = (panel_no + 1) * panel_height - i - 1;
                y1 = (panel_no + 1) * panel_height - i - half_panel_height - 1;
            } else {
                x = panel_idx;
                y0 = panel_no * panel_height + i;
                y1 = panel_no * panel_height + i + half_panel_height;
            }
            result.push_back(cb(width, height, x, y0));
            result.push_back(cb(width, height, x, y1));
        }
    }

    return result;
}

struct matrix_geometry {
    template <typename Cb>
    matrix_geometry(int pixels_across, int n_addr_lines, int n_planes,
                    int width, int height, bool serpentine, const Cb &cb)
        : pixels_across(pixels_across), n_addr_lines(n_addr_lines),
          n_planes(n_planes), width(width),
          height(height), map{make_matrixmap(width, height, n_addr_lines,
                                             serpentine, cb)} {
        int pixels_down = 2 << n_addr_lines;
        if (map.size() != pixels_down * pixels_across) {
            throw std::range_error(
                "map size does not match calculated pixel count");
        }
    }
    int pixels_across, n_addr_lines, n_planes, width, height;
    matrix_map map;
};
} // namespace piomatter
