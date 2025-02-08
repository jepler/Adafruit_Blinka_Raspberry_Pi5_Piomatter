#pragma once

namespace piomatter {

typedef uint8_t pin_t;

struct adafruit_matrix_bonnet_pinout {
    static constexpr pin_t PIN_RGB[] = {5, 13, 6, 12, 16, 23};
    static constexpr pin_t PIN_ADDR[] = {22, 26, 27, 20, 24};
    static constexpr pin_t PIN_OE = 4;   // /OE: output enable when LOW
    static constexpr pin_t PIN_CLK = 17; // SRCLK: clocks on RISING edge
    static constexpr pin_t PIN_LAT = 21; // RCLK: latches on RISING edge

    static constexpr uint32_t clk_bit = 1u << PIN_CLK;
    static constexpr uint32_t lat_bit = 1u << PIN_LAT;
    static constexpr uint32_t oe_bit = 1u << PIN_OE;
    static constexpr uint32_t oe_active = 0;
    static constexpr uint32_t oe_inactive = oe_bit;

    static constexpr uint32_t post_oe_delay = 0;
    static constexpr uint32_t post_latch_delay = 0;
    static constexpr uint32_t post_addr_delay = 500;
};

struct adafruit_matrix_bonnet_pinout_bgr {
    static constexpr pin_t PIN_RGB[] = {6, 13, 5, 23, 16, 12};
    static constexpr pin_t PIN_ADDR[] = {22, 26, 27, 20, 24};
    static constexpr pin_t PIN_OE = 4;   // /OE: output enable when LOW
    static constexpr pin_t PIN_CLK = 17; // SRCLK: clocks on RISING edge
    static constexpr pin_t PIN_LAT = 21; // RCLK: latches on RISING edge

    static constexpr uint32_t clk_bit = 1u << PIN_CLK;
    static constexpr uint32_t lat_bit = 1u << PIN_LAT;
    static constexpr uint32_t oe_bit = 1u << PIN_OE;
    static constexpr uint32_t oe_active = 0;
    static constexpr uint32_t oe_inactive = oe_bit;

    static constexpr uint32_t post_oe_delay = 0;
    static constexpr uint32_t post_latch_delay = 0;
    static constexpr uint32_t post_addr_delay = 500;
};

} // namespace piomatter
