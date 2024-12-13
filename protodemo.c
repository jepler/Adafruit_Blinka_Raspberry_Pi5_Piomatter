#include <cstdlib>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <vector>

#include "hardware/pio.h"   
#include "protomatter.pio.h"

#define PIN_R0 (5)
#define PIN_G0 (13)
#define PIN_B0 (6)
#define PIN_R1 (12)
#define PIN_G1 (16)
#define PIN_B1 (23)
#define PIN_ADDR_A (22)
#define PIN_ADDR_B (26)
#define PIN_ADDR_C (27)
#define PIN_ADDR_D (20)
#define PIN_ADDR_E (24)
#define PIN_OE (4)   // /OE: output enable when LOW
#define PIN_CLK (17) // SRCLK: clocks on RISING edge
#define PIN_LAT (21) // RCLK: latches on RISING edge

const uint8_t all_pins[] = {
    PIN_R0,
    PIN_G0,
    PIN_B0,
    PIN_R1,
    PIN_G1,
    PIN_B1,
    PIN_ADDR_A,
    PIN_ADDR_B,
    PIN_ADDR_C,
    // PIN_ADDR_D,
    // PIN_ADDR_E,
    PIN_OE,
    PIN_CLK,
    PIN_LAT,
};
#define DATA_OVERHEAD (6)
#define DELAY_OVERHEAD (4)
#define CLOCKS_PER_DELAY (1)
// so for example a DATA word with a delay of 3 will take DATA_OVERHEAD+3*CLOCKS_PER_DELAY

static const struct pio_program protomatter_program = {
    .instructions = protomatter,
    .length = 32,
    .origin = -1,
};

static inline pio_sm_config protomatter_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + protomatter_wrap_target, offset + protomatter_wrap);
    return c;
}

static inline void protomatter_program_init(PIO pio, int sm, uint offset) {
    for(int i=0; i<sizeof(all_pins); i++) {
        int pin = all_pins[i];
        pio_gpio_init(pio, pin);
        pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    }
    
    pio_sm_config c = protomatter_program_get_default_config(offset);
    sm_config_set_out_shift(&c, /* shift_right= */ false, /* auto_pull = */ true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, 1.0);
    sm_config_set_out_pins(&c, 0, 28);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

}

#define ACROSS (32)
#define DOWN (16)
#define _ (0)
#define r (0xff0000)
#define g (0x00ff00)
#define b (0x0000ff)
#define y (r|g)
#define c (g|b)
#define m (r|b)
#define w (7)

uint32_t pixels[][ACROSS] = {
{_,w,_,_,r,r,_,_,_,g,_,_,b,b,b,_,c,c,_,_,y,_,y,_,m,m,m,_,w,w,w,_}, // 0
{w,_,w,_,r,_,r,_,g,_,g,_,b,_,_,_,c,_,c,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 1
{w,w,w,_,r,_,r,_,g,g,g,_,b,b,_,_,c,c,_,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 2
{w,_,w,_,r,_,r,_,g,_,g,_,b,_,_,_,c,_,c,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 3
{w,_,w,_,r,r,_,_,g,_,g,_,b,_,_,_,c,_,c,_,_,y,_,_,m,m,m,_,_,w,_,_}, // 4
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 5
{_,c,_,_,y,y,_,_,_,m,_,_,r,r,r,_,g,g,_,_,b,_,b,_,w,w,w,_,c,c,c,_}, // 6
{c,_,c,_,y,_,y,_,m,_,m,_,r,_,_,_,g,_,g,_,b,_,b,_,_,w,_,_,_,c,_,_}, // 7
{c,c,c,_,y,_,y,_,m,m,m,_,r,r,_,_,g,g,_,_,b,_,b,_,_,w,_,_,_,c,_,_}, // 8
{c,_,c,_,y,_,y,_,m,_,m,_,r,_,_,_,g,_,g,_,b,_,b,_,_,w,_,_,_,c,_,_}, // 9
{c,_,c,_,y,y,_,_,m,_,m,_,r,_,_,_,g,_,g,_,_,b,_,_,w,w,w,_,_,c,_,_}, // 10
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 11
{r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,g}, // 12
{y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,g,y}, // 13
{g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,g,c,b}, // 14
{c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,y,g,c,b,m,r,g,c,b,m}, // 15
};
#undef r
#undef g
#undef b
#undef c
#undef y
#undef w
#undef _

constexpr int data_delay_shift = 28;
constexpr uint32_t data_delay = 0;
constexpr uint32_t clock_delay = 0;
constexpr uint32_t command_data = 1u <<31;
constexpr uint32_t command_delay = 0;

constexpr uint32_t clk_bit = 1u << PIN_CLK;
constexpr uint32_t lat_bit = 1u << PIN_LAT;
constexpr uint32_t oe_bit = 1u << PIN_OE;
constexpr uint32_t oe_active = 0;
constexpr uint32_t oe_inactive = oe_bit;

constexpr uint32_t pre_latch_delay = 10;
constexpr uint32_t post_latch_delay = 17;
constexpr uint32_t post_oe_delay = 20;

uint32_t rgb(int r, int g, int b) {
    return (r << 16) | (g << 8) | b;
}

uint32_t colorwheel(int i) {
    if(i < 85) {
        return rgb(255 - i * 3, 0, i * 3);
    }
    if(i < 170) {
        i -= 85;
        return rgb(0, i * 3, 255 - i * 3);
    }
    i -= 170;
    return rgb(i * 3, 255 - i * 3, 0);
}

std::vector<uint32_t> test_pattern() {
    std::vector<uint32_t> result;
    uint32_t time=0;

    for(int i=0; i<ACROSS; i++) {
        pixels[12][i] = colorwheel(2*i);
        pixels[13][i] = colorwheel(2*i+64);
        pixels[14][i] = colorwheel(2*i+128);
        pixels[15][i] = colorwheel(2*i+192);
    }

    auto do_delay = [&](uint32_t delay) {
        assert(delay < 1000000);
        time += DELAY_OVERHEAD + delay * CLOCKS_PER_DELAY;
        result.push_back(command_delay | delay);
    };

    auto do_data = [&](uint32_t d, uint32_t delay=0) {
        if(delay < 8) {
            result.push_back(command_data | (delay << data_delay_shift) | d);
            time += DATA_OVERHEAD + delay * CLOCKS_PER_DELAY;
        } else {
            result.push_back(command_data | d);
            time += DATA_OVERHEAD;
            do_delay(delay - DELAY_OVERHEAD / CLOCKS_PER_DELAY);
        }
    };

    auto calc_addr_bits = [](int addr) {
        uint32_t data = 0;
        if(addr & 1) data |= (1 << PIN_ADDR_A);
        if(addr & 2) data |= (1 << PIN_ADDR_B);
        if(addr & 4) data |= (1 << PIN_ADDR_C);
        if(addr & 8) data |= (1 << PIN_ADDR_D);
        if(addr & 16) data |= (1 << PIN_ADDR_E);
        return data;
    };

    auto add_data_word = [&](uint32_t addr_bits, bool r0, bool g0, bool b0, bool r1, bool g1, bool b1) {
        uint32_t data = oe_active | addr_bits;
        if(r0) data |= (1 << PIN_R0);
        if(g0) data |= (1 << PIN_G0);
        if(b0) data |= (1 << PIN_B0);
        if(r1) data |= (1 << PIN_R1);
        if(g1) data |= (1 << PIN_G1);
        if(b1) data |= (1 << PIN_B1);

        do_data(data, data_delay);
        do_data(data | clk_bit, data_delay);
    };

    for(int bit = 7; bit >= 0; bit--) {
        int last_bit = (bit + 1) % 8;
        uint32_t desired_duration = 400 << last_bit; // mumble

        uint32_t r = 1 << (16 + bit);
        uint32_t g = 1 << (8 + bit);
        uint32_t b = 1 << bit;

        for(int addr = 0; addr < 8; addr++) {
            uint32_t t_start = time;
            uint32_t addr_bits = calc_addr_bits((addr + 7) % 8); // address of previous line

            for(int across = 0; across < ACROSS; across++) {
                auto pixel0 = pixels[addr][across];
                auto r0 = pixel0 & r;
                auto g0 = pixel0 & g;
                auto b0 = pixel0 & b;
                auto pixel1 = pixels[addr+8][across];
                auto r1 = pixel1 & r;
                auto g1 = pixel1 & g;
                auto b1 = pixel1 & b;

                add_data_word(addr_bits, r0, g0, b0, r1, g1, b1);
            }

            uint32_t t_end = time;
            uint32_t duration = t_end - t_start;

            printf("line duration %d\n", duration);
            // hold /OE low until desired time has elapsed
            if (duration < desired_duration) {
                printf("additional duration %d\n", desired_duration - duration);
                do_delay(desired_duration - duration);
            }

            do_data(addr_bits | oe_inactive, pre_latch_delay);
            do_data(addr_bits | oe_inactive | lat_bit, post_latch_delay);
            addr_bits = calc_addr_bits(addr);
            do_data(addr_bits | oe_inactive, post_oe_delay);
        }
    }
    printf("Time %d (%.1f us)\n", time, time*1e6/clock_get_hz(clk_sys));

    return result;
}

int main(int argc, char **argv) {
    PIO pio = pio0;
    int sm = pio_claim_unused_sm(pio, true);
    pio_sm_config_xfer(pio, sm, PIO_DIR_TO_SM, 256, 1); 

printf("clock %fMHz\n", clock_get_hz(clk_sys)/1e6);

    uint offset = pio_add_program(pio, &protomatter_program);
    printf("Loaded program at %d, using sm %d\n", offset, sm);

    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_clkdiv(pio, sm, 1.0);

    protomatter_program_init(pio, sm, offset);

    std::vector<uint32_t> data = test_pattern();


std::vector<uint32_t> dc;
int rep = 0;
while(dc.size() + data.size() < UINT16_MAX / sizeof(uint32_t)) {
rep ++;
    std::copy(data.begin(), data.end(), std::back_inserter(dc));
}
std::swap(dc, data);

    uint32_t *databuf = &data[0];
    size_t datasize = data.size() * sizeof(uint32_t);
    assert(datasize < 65536);

printf("%zd data elements (%d repetitions)\n", data.size(), rep);
int n = argc > 1 ? atoi(argv[1]) : 1;
    for(int i=0; i<n; i++) {
       pio_sm_xfer_data(pio, sm, PIO_DIR_TO_SM, datasize, databuf);
    }
}
