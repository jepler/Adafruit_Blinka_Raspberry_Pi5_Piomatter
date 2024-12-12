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
#define DATA_OVERHEAD (5)
#define DELAY_OVERHEAD (3)
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
#define r (1)
#define g (2)
#define b (4)
#define y (r|g)
#define c (g|b)
#define m (r|b)
#define w (7)

uint8_t pixels[DOWN][ACROSS] = {
{_,w,_,_,r,r,_,_,_,g,_,_,b,b,b,_,c,c,_,_,y,_,y,_,m,m,m,_,w,w,w,_}, // 0
{w,_,w,_,r,_,r,_,g,_,g,_,b,_,_,_,c,_,c,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 1
{w,w,w,_,r,_,r,_,g,g,g,_,b,b,_,_,c,c,_,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 2
{w,_,w,_,r,_,r,_,g,_,g,_,b,_,_,_,c,_,c,_,y,_,y,_,_,m,_,_,_,w,_,_}, // 3
{w,_,w,_,r,r,_,_,g,_,g,_,b,_,_,_,c,_,c,_,_,y,_,_,m,m,m,_,_,w,_,_}, // 4
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 5
{r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_,r,_}, // 6
{r,r,_,_,r,r,_,_,r,r,_,_,r,r,_,_,r,r,_,_,r,r,_,_,r,r,_,_,r,r,_,_}, // 7
{g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_,g,_}, // 8
{g,g,_,_,g,g,_,_,g,g,_,_,g,g,_,_,g,g,_,_,g,g,_,_,g,g,_,_,g,g,_,_}, // 9
{b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_,b,_}, // 10
{b,b,_,_,b,b,_,_,b,b,_,_,b,b,_,_,b,b,_,_,b,b,_,_,b,b,_,_,b,b,_,_}, // 11
{w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_}, // 12
{w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_}, // 13
{_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w,_,w}, // 13
{_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_,_,w,w,_}, // 15
};

constexpr int delay_shift = 28;
constexpr uint32_t data_delay = 1;
constexpr uint32_t clock_delay = 3;
constexpr uint32_t data_bit = 1u <<31;
constexpr uint32_t delay_bit = 0;

constexpr uint32_t clk_bit = 1u << PIN_CLK;
constexpr uint32_t lat_bit = 1u << PIN_LAT;
constexpr uint32_t oe_bit = 1u << PIN_LAT;
constexpr uint32_t oe_active = 0;
constexpr uint32_t oe_inactive = oe_bit;

constexpr uint32_t pre_latch_delay = 7;
constexpr uint32_t post_latch_delay = 7;

std::vector<uint32_t> test_pattern() {
    std::vector<uint32_t> result;

    auto do_data = [&](uint32_t d, uint32_t delay=0) {
        assert(delay < 8);
        result.push_back(data_bit | (delay << delay_shift) | d);
    };

    auto do_delay = [&](uint32_t delay) {
        result.push_back(delay_bit | delay);
    };

    auto add_data_word = [&](int addr, bool r0, bool g0, bool b0, bool r1, bool g1, bool b1) {
        uint32_t data = oe_active;
        if(addr & 1) data |= (1 << PIN_ADDR_A);
        if(addr & 2) data |= (1 << PIN_ADDR_B);
        if(addr & 4) data |= (1 << PIN_ADDR_C);
        if(addr & 8) data |= (1 << PIN_ADDR_D);
        if(addr & 16) data |= (1 << PIN_ADDR_E);
        if(r0) data |= (1 << PIN_R0);
        if(g0) data |= (1 << PIN_G0);
        if(b0) data |= (1 << PIN_B0);
        if(r1) data |= (1 << PIN_R1);
        if(g1) data |= (1 << PIN_G1);
        if(b1) data |= (1 << PIN_B1);

        do_data(data, data_delay);
        do_data(data | clk_bit, data_delay);
    };

    for(int addr = 0; addr < 8; addr++) {
        for(int across = 0; across < ACROSS; across++) {
            auto pixel0 = pixels[addr][across];
            auto r0 = pixel0 & r;
            auto g0 = pixel0 & g;
            auto b0 = pixel0 & b;
            auto pixel1 = pixels[addr+8][across];
            auto r1 = pixel1 & r;
            auto g1 = pixel1 & g;
            auto b1 = pixel1 & b;

            add_data_word(addr, r0, g0, b0, r1, g1, b1);
        }

        do_data(oe_inactive, pre_latch_delay);
        do_data(oe_inactive | lat_bit, post_latch_delay);
    }

    return result;
}

int main() {
    PIO pio = pio0;
    int sm = pio_claim_unused_sm(pio, true);
    pio_sm_config_xfer(pio, sm, PIO_DIR_TO_SM, 256, 1); 

    uint offset = pio_add_program(pio, &protomatter_program);
    printf("Loaded program at %d, using sm %d\n", offset, sm);

    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_clkdiv(pio, sm, 1.0);

    protomatter_program_init(pio, sm, offset);

    std::vector<uint32_t> data = test_pattern();

    uint32_t *databuf = &data[0];
    size_t datasize = data.size() * sizeof(uint32_t);

    assert(datasize < 65536);

printf("%zd data elements\n", data.size());
    for(int i=0; i<10000; i++) {
        pio_sm_xfer_data(pio, sm, PIO_DIR_TO_SM, datasize, databuf);
    }
}
