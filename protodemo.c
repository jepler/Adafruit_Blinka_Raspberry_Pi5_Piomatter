#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <ctime>

#include "hardware/pio.h"   
#include "protomatter.pio.h"

#include "matrixmap.h"

#define countof(arr) (sizeof((arr)) / sizeof((arr)[0]))

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

typedef uint8_t pin_t;

const pin_t all_pins[] = {
    PIN_R0,
    PIN_G0,
    PIN_B0,
    PIN_R1,
    PIN_G1,
    PIN_B1,
    PIN_OE,
    PIN_CLK,
    PIN_LAT,
    PIN_ADDR_A,
    PIN_ADDR_B,
    PIN_ADDR_C,
    PIN_ADDR_D,
    PIN_ADDR_E,
};
#define DATA_OVERHEAD (3)
#define CLOCKS_PER_DATA (2)
#define DELAY_OVERHEAD (5) // including the y==0 loop iteration and "jmp top"
#define CLOCKS_PER_DELAY (1)

constexpr uint32_t command_data = 1u <<31;
constexpr uint32_t command_delay = 0;

constexpr uint32_t clk_bit = 1u << PIN_CLK;
constexpr uint32_t lat_bit = 1u << PIN_LAT;
constexpr uint32_t oe_bit = 1u << PIN_OE;
constexpr uint32_t oe_active = 0;
constexpr uint32_t oe_inactive = oe_bit;

constexpr uint32_t post_oe_delay = 0;
constexpr uint32_t post_latch_delay = 0;
constexpr uint32_t post_addr_delay = 500;

// so for example sending 8 data words will take DATA_OVERHEAD + CLOCKS_PER_DATA*8 PIO cycles
// and sending a delay of 5 will take DELAY_OVERHEAD + CLOCKS_PER_DELAY * 5

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

static inline void protomatter_pin_init(PIO pio, int sm, int num_addr_pins) {
    for(int i=0; i<countof(all_pins) - 5 + num_addr_pins; i++) {
        int pin = all_pins[i];
        pio_gpio_init(pio, pin);
        pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    }
}

static inline void protomatter_program_init(PIO pio, int sm, uint offset) {

    pio_sm_config c = protomatter_program_get_default_config(offset);
    sm_config_set_out_shift(&c, /* shift_right= */ false, /* auto_pull = */ true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, 1.0);
    sm_config_set_out_pins(&c, 0, 28);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

}

void protomatter_convert(std::vector<uint32_t> &result, const MatrixMap &matrixmap, uint32_t *pixels, int across, int down, int n_planes) {
    result.clear();

    int data_count = 0;

    auto do_delay = [&](uint32_t delay) {
        if (delay == 0) return;
        assert(delay < 1000000);
        assert(!data_count);
        result.push_back(command_delay | (delay ? delay - 1 : 0));
    };

    auto prep_data = [&data_count, &result](uint32_t n) {
        assert(!data_count);
        assert(n < 60000);
        result.push_back(command_data | (n - 1));
        data_count = n;
    };

    auto do_data = [&](uint32_t d) {
        assert(data_count);
        data_count --;
        result.push_back(d);
    };

    auto do_data_delay = [&](uint32_t d, uint32_t delay) {
        prep_data(1);
        do_data(d);
        do_delay(delay);
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

    auto add_pixels = [&do_data, &result](uint32_t addr_bits, bool r0, bool g0, bool b0, bool r1, bool g1, bool b1, bool active) {
        uint32_t data = (active ? oe_active : oe_inactive) | addr_bits;
        if(r0) data |= (1 << PIN_R0);
        if(g0) data |= (1 << PIN_G0);
        if(b0) data |= (1 << PIN_B0);
        if(r1) data |= (1 << PIN_R1);
        if(g1) data |= (1 << PIN_G1);
        if(b1) data |= (1 << PIN_B1);

        do_data(data);
        do_data(data | clk_bit);
    };


    int last_bit = 0;
    int prev_addr = 7;
    // illuminate the right row for data in the shift register (the previous address)
    uint32_t addr_bits = calc_addr_bits(prev_addr);
    assert(matrixmap.size() == down * across);

#define N_BITS 10
#define OFFSET (N_BITS - n_planes)
    for(int addr = 0; addr < down / 2; addr++) {
        for(int bit = n_planes - 1; bit >= 0; bit--) {
            uint32_t r = 1 << (20 + OFFSET + bit);
            uint32_t g = 1 << (10 + OFFSET + bit);
            uint32_t b = 1 << (0 + OFFSET + bit);

            // the shortest /OE we can do is one DATA_OVERHEAD...
            // TODO: should make sure desired duration of MSB is at least `across`
            uint32_t desired_duration = 1 << last_bit;
            last_bit = bit;


            prep_data(2 * across);
            auto mapiter = matrixmap.begin() + 2 * addr * across;
            for(int x = 0; x < across; x++) {
                assert(mapiter != matrixmap.end());
                auto pixel0 = pixels[*mapiter++];
                auto r0 = pixel0 & r;
                auto g0 = pixel0 & g;
                auto b0 = pixel0 & b;
                assert(mapiter != matrixmap.end());
                auto pixel1 = pixels[*mapiter++];
                auto r1 = pixel1 & r;
                auto g1 = pixel1 & g;
                auto b1 = pixel1 & b;

                add_pixels(addr_bits, r0, g0, b0, r1, g1, b1, x < desired_duration);
            }

            // hold /OE low until desired time has elapsed to illuminate the LAST line
            int remain = desired_duration - across;
            if (remain > 0) {
                do_data_delay(addr_bits | oe_active, remain * CLOCKS_PER_DATA - DELAY_OVERHEAD);
            }

            do_data_delay(addr_bits | oe_inactive, post_oe_delay);
            do_data_delay(addr_bits | oe_inactive | lat_bit, post_latch_delay);

            // with oe inactive, set address bits to illuminate THIS line
            if (addr != prev_addr) {
                addr_bits = calc_addr_bits(addr);
                do_data_delay(addr_bits | oe_inactive, post_addr_delay);
                prev_addr = addr;
            }
        }
    }
}


uint16_t gamma_lut[256];
void make_gamma_lut(double exponent) {
    for(int i=0; i<256; i++) {
        auto v = std::max(i, int(round(1023 * pow(i / 255, exponent))));
        gamma_lut[i] = v;
    }
}

uint32_t rgb(unsigned r, unsigned g, unsigned b) {
    assert(r < 256);
    assert(g < 256);
    assert(b < 256);
    return (gamma_lut[r] << 20) | (gamma_lut[g] << 10) | gamma_lut[b];
}


#define ACROSS (128)
#define DOWN (32)
#define N_PLANES (10)
#define _ (0)
#define r (1023 << 20)
#define g (1023 << 10)
#define b (1023)
#define y (r|g)
#define c (g|b)
#define m (r|b)
#define w (r|g|b)

constexpr int width = 64, height = 64;

uint32_t pixels[height][width] = {
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

uint32_t colorwheel(int i) {
    i = i & 0xff;
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

MatrixMap matrixmap;

void test_pattern(std::vector<uint32_t> &result, int offs) {
    for(int i=0; i<width; i++) {
        pixels[height-5][i] = rgb(1+i*4, 1+i*4, 1+i*4);
        pixels[height-4][i] = colorwheel(2*i + offs / 3);
        pixels[height-3][i] = colorwheel(2*i+64 + offs / 5);
        pixels[height-2][i] = colorwheel(2*i+128 + offs / 2);
        pixels[height-1][i] = colorwheel(2*i+192 + offs / 7);
    }
    for(int i=0; i<height; i++) {
        pixels[i][i] = rgb(0xff,0xff,0xff);
    }

    protomatter_convert(result, matrixmap, &pixels[0][0], ACROSS, DOWN, N_PLANES);
}
static uint64_t monotonicns64() {

    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * UINT64_C(1000000000)+ tp.tv_nsec;

}

static void dump_matrixmap() {
    FILE *f = fopen("matrixmap.txt", "w");
    bool first = true;
    fprintf(f, "[\n");
    for(auto i : matrixmap) {
        if (!first) { fprintf(f, ",\n"); }
        first=false;
        fprintf(f, "%u", i);
    }
    fprintf(f, "]\n");
    fclose(f);

}

static void dump_test_pattern() {
    FILE *f = fopen("pattern.txt", "w");
    std::vector<uint32_t> data;
    test_pattern(data, 0);
    printf("%zd data elements\n", data.size());
    bool first = true;
    fprintf(f, "[\n");
    for(auto i : data) {
    if (!first) { fprintf(f, ",\n"); }
    first=false;
    fprintf(f, "%u", i); }
    fprintf(f, "]\n");
    fclose(f);
}

constexpr size_t MAX_XFER = 32768;

void pio_sm_xfer_data_large(PIO pio, int sm, int direction, size_t size, uint32_t *databuf) {
    while(size) {
        size_t xfersize = std::min(size_t{MAX_XFER}, size);
        int r = pio_sm_xfer_data(pio, sm, direction, xfersize, databuf);
        if (r) {
            perror("pio_sm_xfer_data (reboot may be required)");
            abort();
        }
        size -= xfersize;
        databuf += xfersize / sizeof(*databuf);
    }
}

static_assert(!(DOWN & (DOWN-1))); // is a power of two

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 0;

    // matrixmap = make_matrixmap(width, height, 3, false, orientation_r180);
    matrixmap = make_matrixmap(width, height, 4, true, orientation_normal);

    if(n == 0) {
        dump_matrixmap();
        dump_test_pattern();
        exit(0);
    }

    PIO pio = pio0;
    int sm = pio_claim_unused_sm(pio, true);
    int r = pio_sm_config_xfer(pio, sm, PIO_DIR_TO_SM, MAX_XFER, 2); 
    if (r) {
        perror("pio_sm_config_xfer");
        abort();
    }

    make_gamma_lut(2.2);

printf("clock %fMHz\n", clock_get_hz(clk_sys)/1e6);

    uint offset = pio_add_program(pio, &protomatter_program);
    printf("Loaded program at %d, using sm %d\n", offset, sm);

    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_clkdiv(pio, sm, 1.0);

    protomatter_program_init(pio, sm, offset);
    protomatter_pin_init(pio, sm, __builtin_ffs(DOWN)-2);


    std::vector<uint32_t> data;
    uint64_t start = monotonicns64();
    for(int i=0; i<n; i++) {
        test_pattern(data, i);

        uint32_t *databuf = &data[0];
        size_t datasize = data.size() * sizeof(uint32_t);

        pio_sm_xfer_data_large(pio, sm, PIO_DIR_TO_SM, datasize, databuf);
    }
    uint64_t end = monotonicns64();

    uint64_t duration = end - start;
    double fps = n * 1e9 / duration;
    printf("%.1f FPS [%d frames in %fs]\n", fps, n, duration / 1e9);
}
