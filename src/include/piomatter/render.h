#pragma once

#include "matrixmap.h"
#include <cassert>
#include <span>
#include <vector>

namespace piomatter {

constexpr int DATA_OVERHEAD = 3;
constexpr int CLOCKS_PER_DATA = 2;
constexpr int DELAY_OVERHEAD = 5;
constexpr int CLOCKS_PER_DELAY = 1;

constexpr uint32_t command_data = 1u << 31;
constexpr uint32_t command_delay = 0;

struct gamma_lut {
    gamma_lut(double exponent = 2.2) {
        for (int i = 0; i < 256; i++) {
            auto v = std::max(i, int(round(1023 * pow(i / 255., exponent))));
            lut[i] = v;
        }
    }

    unsigned convert(unsigned v) {
        if (v >= std::size(lut))
            return 1023;
        return lut[v];
    }

    void convert_rgb888_packed_to_rgb10(std::vector<uint32_t> &result,
                                        std::span<const uint8_t> source) {
        result.resize(source.size() / 3);
        for (size_t i = 0, j = 0; i < source.size(); i += 3) {
            uint32_t r = source[i + 0] & 0xff;
            uint32_t g = source[i + 1] & 0xff;
            uint32_t b = source[i + 2] & 0xff;
            result[j++] = (convert(r) << 20) | (convert(g) << 10) | convert(b);
        }
    }

    void convert_rgb888_to_rgb10(std::vector<uint32_t> &result,
                                 std::span<const uint32_t> source) {
        result.resize(source.size());
        for (size_t i = 0; i < source.size(); i++) {
            uint32_t data = source[i];
            uint32_t r = (data >> 16) & 0xff;
            uint32_t g = (data >> 8) & 0xff;
            uint32_t b = data & 0xff;
            result[i] = (convert(r) << 20) | (convert(g) << 10) | convert(b);
        }
    }

    void convert_rgb565_to_rgb10(std::vector<uint32_t> &result,
                                 std::span<const uint16_t> source) {
        result.resize(source.size());
        for (size_t i = 0; i < source.size(); i++) {
            uint32_t data = source[i];
            unsigned r5 = (data >> 11) & 0x1f;
            unsigned r = (r5 << 3) | (r5 >> 2);
            unsigned g6 = (data >> 5) & 0x3f;
            unsigned g = (g6 << 2) | (g6 >> 4);
            unsigned b5 = (data)&0x1f;
            unsigned b = (b5 << 3) | (b5 >> 2);

            result[i] = (convert(r) << 20) | (convert(g) << 10) | convert(b);
        }
    }

    uint16_t lut[256];
};

struct colorspace_rgb565 {
    using data_type = uint16_t;
    static constexpr size_t data_size_in_bytes(size_t n_pixels) {
        return sizeof(data_type) * n_pixels;
    }

    colorspace_rgb565(float gamma = 2.2) : lut{gamma} {}
    gamma_lut lut;
    const std::span<const uint32_t>
    convert(std::span<const data_type> data_in) {
        lut.convert_rgb565_to_rgb10(rgb10, data_in);
        return rgb10;
    }
    std::vector<uint32_t> rgb10;
};

struct colorspace_rgb888 {
    using data_type = uint32_t;
    static constexpr size_t data_size_in_bytes(size_t n_pixels) {
        return sizeof(data_type) * n_pixels;
    }

    colorspace_rgb888(float gamma = 2.2) : lut{gamma} {}
    gamma_lut lut;
    const std::span<const uint32_t>
    convert(std::span<const data_type> data_in) {
        lut.convert_rgb888_to_rgb10(rgb10, data_in);
        return rgb10;
    }
    std::vector<uint32_t> rgb10;
};

struct colorspace_rgb888_packed {
    using data_type = uint8_t;
    static constexpr size_t data_size_in_bytes(size_t n_pixels) {
        return sizeof(data_type) * n_pixels * 3;
    }

    colorspace_rgb888_packed(float gamma = 2.2) : lut{gamma} {}
    gamma_lut lut;
    const std::span<const uint32_t>
    convert(std::span<const data_type> data_in) {
        lut.convert_rgb888_packed_to_rgb10(rgb10, data_in);
        return rgb10;
    }
    std::vector<uint32_t> rgb10;
};

struct colorspace_rgb10 {
    using data_type = uint32_t;

    const std::span<const uint32_t>
    convert(std::span<const data_type> data_in) {
        return data_in;
    }
};

// Render a buffer in linear RGB10 format into a piomatter stream
template <typename pinout>
void protomatter_render_rgb10(std::vector<uint32_t> &result,
                              const matrix_geometry &matrixmap,
                              const uint32_t *pixels) {
    result.clear();

    int data_count = 0;

    auto do_data_delay = [&](uint32_t data, int32_t delay) {
        delay = std::max((delay / CLOCKS_PER_DELAY) - DELAY_OVERHEAD, 1);
        assert(delay < 1000000);
        assert(!data_count);
        result.push_back(command_delay | (delay ? delay - 1 : 0));
        result.push_back(data);
    };

    auto prep_data = [&data_count, &result](uint32_t n) {
        assert(!data_count);
        assert(n);
        assert(n < 60000);
        result.push_back(command_data | (n - 1));
        data_count = n;
    };

    int32_t active_time;

    auto do_data_clk_active = [&active_time, &data_count, &result](uint32_t d) {
        bool active = active_time > 0;
        active_time--;
        d |= active ? pinout::oe_active : pinout::oe_inactive;
        assert(data_count);
        data_count--;
        result.push_back(d);
    };

    auto calc_addr_bits = [](int addr) {
        uint32_t data = 0;
        if (addr & 1)
            data |= (1 << pinout::PIN_ADDR[0]);
        if (addr & 2)
            data |= (1 << pinout::PIN_ADDR[1]);
        if (addr & 4)
            data |= (1 << pinout::PIN_ADDR[2]);
        if constexpr (std::size(pinout::PIN_ADDR) >= 4) {
            if (addr & 8)
                data |= (1 << pinout::PIN_ADDR[3]);
        }
        if constexpr (std::size(pinout::PIN_ADDR) >= 5) {
            if (addr & 16)
                data |= (1 << pinout::PIN_ADDR[4]);
        }
        return data;
    };

    auto add_pixels = [&do_data_clk_active,
                       &result](uint32_t addr_bits, bool r0, bool g0, bool b0,
                                bool r1, bool g1, bool b1) {
        uint32_t data = addr_bits;
        if (r0)
            data |= (1 << pinout::PIN_RGB[0]);
        if (g0)
            data |= (1 << pinout::PIN_RGB[1]);
        if (b0)
            data |= (1 << pinout::PIN_RGB[2]);
        if (r1)
            data |= (1 << pinout::PIN_RGB[3]);
        if (g1)
            data |= (1 << pinout::PIN_RGB[4]);
        if (b1)
            data |= (1 << pinout::PIN_RGB[5]);

        do_data_clk_active(data);
    };

    int last_bit = 0;
    // illuminate the right row for data in the shift register (the previous
    // address)

    const size_t n_addr = 1u << matrixmap.n_addr_lines;
    const int n_planes = matrixmap.n_planes;
    constexpr size_t n_bits = 10u;
    unsigned offset = n_bits - n_planes;
    const size_t pixels_across = matrixmap.pixels_across;

    size_t prev_addr = n_addr - 1;
    uint32_t addr_bits = calc_addr_bits(prev_addr);

    for (size_t addr = 0; addr < n_addr; addr++) {
        // printf("addr=%zu/%zu\n", addr, n_addr);
        for (int bit = n_planes - 1; bit >= 0; bit--) {
            // printf("bit=%d/%d\n", bit, n_planes);
            uint32_t r = 1 << (20 + offset + bit);
            uint32_t g = 1 << (10 + offset + bit);
            uint32_t b = 1 << (0 + offset + bit);

            // the shortest /OE we can do is one DATA_OVERHEAD...
            // TODO: should make sure desired duration of MSB is at least
            // `pixels_across`
            active_time = 1 << last_bit;
            last_bit = bit;

            prep_data(pixels_across);
            auto mapiter = matrixmap.map.begin() + 2 * addr * pixels_across;
            for (size_t x = 0; x < pixels_across; x++) {
                assert(mapiter != matrixmap.map.end());
                auto pixel0 = pixels[*mapiter++];
                auto r0 = pixel0 & r;
                auto g0 = pixel0 & g;
                auto b0 = pixel0 & b;
                assert(mapiter != matrixmap.map.end());
                auto pixel1 = pixels[*mapiter++];
                auto r1 = pixel1 & r;
                auto g1 = pixel1 & g;
                auto b1 = pixel1 & b;

                add_pixels(addr_bits, r0, g0, b0, r1, g1, b1);
            }

            do_data_delay(addr_bits | pinout::oe_active,
                          active_time * CLOCKS_PER_DATA / CLOCKS_PER_DELAY -
                              DELAY_OVERHEAD);

            do_data_delay(addr_bits | pinout::oe_inactive,
                          pinout::post_oe_delay);
            do_data_delay(addr_bits | pinout::oe_inactive | pinout::lat_bit,
                          pinout::post_latch_delay);

            // with oe inactive, set address bits to illuminate THIS line
            if (addr != prev_addr) {
                addr_bits = calc_addr_bits(addr);
                do_data_delay(addr_bits | pinout::oe_inactive,
                              pinout::post_addr_delay);
                prev_addr = addr;
            }
        }
    }
}

} // namespace piomatter
