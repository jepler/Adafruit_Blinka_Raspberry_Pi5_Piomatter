#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include "piomatter/piomatter.h"

#define _ (0)
#define r (255 << 16)
#define g (255 << 8)
#define b (255)
#define y (r | g)
#define c (g | b)
#define m (r | b)
#define w (r | g | b)

constexpr int width = 64, height = 64;

uint32_t pixels[height][width] = {
    {_, w, _, _, r, r, _, _, _, g, _, _, b, b, b, _,
     c, c, _, _, y, _, y, _, m, m, m, _, w, w, w, _}, // 0
    {w, _, w, _, r, _, r, _, g, _, g, _, b, _, _, _,
     c, _, c, _, y, _, y, _, _, m, _, _, _, w, _, _}, // 1
    {w, w, w, _, r, _, r, _, g, g, g, _, b, b, _, _,
     c, c, _, _, y, _, y, _, _, m, _, _, _, w, _, _}, // 2
    {w, _, w, _, r, _, r, _, g, _, g, _, b, _, _, _,
     c, _, c, _, y, _, y, _, _, m, _, _, _, w, _, _}, // 3
    {w, _, w, _, r, r, _, _, g, _, g, _, b, _, _, _,
     c, _, c, _, _, y, _, _, m, m, m, _, _, w, _, _}, // 4
    {_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
     _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _}, // 5
    {_, c, _, _, y, y, _, _, _, m, _, _, r, r, r, _,
     g, g, _, _, b, _, b, _, w, w, w, _, c, c, c, _}, // 6
    {c, _, c, _, y, _, y, _, m, _, m, _, r, _, _, _,
     g, _, g, _, b, _, b, _, _, w, _, _, _, c, _, _}, // 7
    {c, c, c, _, y, _, y, _, m, m, m, _, r, r, _, _,
     g, g, _, _, b, _, b, _, _, w, _, _, _, c, _, _}, // 8
    {c, _, c, _, y, _, y, _, m, _, m, _, r, _, _, _,
     g, _, g, _, b, _, b, _, _, w, _, _, _, c, _, _}, // 9
    {c, _, c, _, y, y, _, _, m, _, m, _, r, _, _, _,
     g, _, g, _, _, b, _, _, w, w, w, _, _, c, _, _}, // 10
    {_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
     _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _}, // 11
    {r, y, g, c, b, m, r, y, g, c, b, m, r, y, g, c,
     b, m, r, y, g, c, b, m, r, y, g, c, b, m, r, g}, // 12
    {y, g, c, b, m, r, y, g, c, b, m, r, y, g, c, b,
     m, r, y, g, c, b, m, r, y, g, c, b, m, r, g, y}, // 13
    {g, c, b, m, r, y, g, c, b, m, r, y, g, c, b, m,
     r, y, g, c, b, m, r, y, g, c, b, m, r, g, c, b}, // 14
    {c, b, m, r, y, g, c, b, m, r, y, g, c, b, m, r,
     y, g, c, b, m, r, y, g, c, b, m, r, g, c, b, m}, // 15
};
#undef r
#undef g
#undef b
#undef c
#undef y
#undef w
#undef _

#define rgb(r, g, b) ((r << 16) | (g << 8) | b)

uint32_t colorwheel(int i) {
    i = i & 0xff;
    if (i < 85) {
        return rgb(255 - i * 3, 0, i * 3);
    }
    if (i < 170) {
        i -= 85;
        return rgb(0, i * 3, 255 - i * 3);
    }
    i -= 170;
    return rgb(i * 3, 255 - i * 3, 0);
}

void test_pattern(int offs) {
    for (int i = 0; i < width; i++) {
        pixels[height - 5][i] = rgb(1 + i * 4, 1 + i * 4, 1 + i * 4);
        pixels[height - 4][i] = colorwheel(2 * i + offs / 3);
        pixels[height - 3][i] = colorwheel(2 * i + 64 + offs / 5);
        pixels[height - 2][i] = colorwheel(2 * i + 128 + offs / 2);
        pixels[height - 1][i] = colorwheel(2 * i + 192 + offs / 7);
    }
    for (int i = 0; i < height; i++) {
        pixels[i][i] = rgb(0xff, 0xff, 0xff);
    }
}

static uint64_t monotonicns64() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * UINT64_C(1000000000) + tp.tv_nsec;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 0;

    piomatter::matrix_geometry geometry(128, 4, 10, 64, 64, true,
                                        piomatter::orientation_normal);
    piomatter::piomatter p(std::span(&pixels[0][0], 64 * 64), geometry);

    uint64_t start = monotonicns64();
    for (int i = 0; i < n; i++) {
        test_pattern(i);
        p.show();
    }
    uint64_t end = monotonicns64();

    uint64_t duration = end - start;
    double fps = n * 1e9 / duration;
    printf("%.1f FPS [%d frames in %fs]\n", fps, n, duration / 1e9);
}
