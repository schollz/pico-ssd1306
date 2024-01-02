#include <assert.h>
#include <stdio.h>

#include "data/blink.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "math.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

/*int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}*/

inline void blit_animation_1bpp(uint8_t* dest, size_t dest_width,
                                size_t dest_height, uint16_t dest_x,
                                uint16_t dest_y, const Animation& anim,
                                uint16_t frame_num) {
  assert(frame_num < anim.num_frames);

  const Animation::Frame& frame = anim.frames[frame_num];
  for (uint8_t layer_idx = 0; layer_idx < anim.num_layers; layer_idx++) {
    const Animation::Frame::Layer& layer = frame.layers[layer_idx];

    // draw pixel: dest[x + (y / 8) * width_] |= (1 << (y & 7));
    for (uint16_t y = 0; y < layer.height; y++) {
      for (uint16_t x = 0; x < layer.width; x++) {
        uint16_t in_x = x + layer.x;
        uint16_t in_y = y + layer.y;
        uint16_t in_offset = (in_x / 8) + (in_y * (anim.image_width / 8));
        assert(in_offset < anim.image_width * anim.image_height);
        uint8_t val =
            anim.image_data[(in_x / 8) + (in_y * (anim.image_width / 8))] &
            (1 << (in_x & 7));

        uint16_t out_x = x + layer.dest_offset_x + dest_x;
        uint16_t out_y = y + layer.dest_offset_y + dest_y;
        assert(out_x < dest_width && out_y < dest_height);
        if (val) {
          dest[out_x + (out_y / 8) * dest_width] |= (1 << (out_y & 7));
        }
      }
    }
  }
}

int main() {
  stdio_init_all();

  // set onboard led high to show that we're on
  /*gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);
  gpio_put(25, 1);*/

  SSD1306 display(128, 64, spi0, /*baudrate*/ 8000 * 1000, /*mosi*/ 3,
                  /*cs*/ 5,
                  /*sclk*/ 2, /*reset*/ 6, /*dc*/ 4);
  display.init();

  //   const Animation& anim = animation_blink;
  //   uint16_t frame = 0;
  //   while (true) {
  //     display.clear();
  //     blit_animation_1bpp(display.buffer(), display.width(),
  //     display.height(), 0,
  //                         8, anim, frame);
  //     display.update();
  //     sleep_ms(anim.frames[frame].duration_ms);
  //     frame = (frame + 1) % anim.num_frames;
  //   }

  //   while (true) {
  //     sleep_ms(1000);
  //   }

  // INITIALIZE ADC
  // intialize aray of 128 values
  int16_t arr[2][128];
  uint8_t arr_idx = 0;
  for (uint8_t j = 0; j < 2; j++) {
    for (int16_t i = 0; i < 128; i++) {
      arr[j][i] = 0;
    }
  }

  adc_init();
  adc_gpio_init(26);
  adc_gpio_init(27);
  adc_gpio_init(28);

  while (true) {
    for (uint8_t j = 0; j < 2; j++) {
      // switch to adc channel 0
      adc_select_input(j);
      // get adc value
      uint16_t adc_val = adc_read();
      // move all values in the array over by one
      for (int16_t i = 0; i < 127; i++) {
        arr[j][i] = arr[j][i + 1];
      }
      arr[j][127] = adc_val * 64 / 4096;

      display.clear();
      for (int16_t i = 1; i < 128; i++) {
        int16_t x0 = i - 1;
        int16_t y0 = arr[j][i - 1];
        int16_t x1 = i;
        int16_t y1 = arr[j][i];
        // color all the pixels between two points (x0,y0) and (x1,y1)
        // (inclusive)
        // draw continuous line
        if (x0 == x1) {
          display.draw_pixel(x0, y0, SSD1306_COLOR_ON);
        } else {
          float m = (float)(y1 - y0) / (float)(x1 - x0);
          float b = y0 - m * x0;
          int8_t inc = 1;
          if (y1 < y0) {
            inc = -1;
          }
          for (int16_t y = y0; y <= y1; y += inc) {
            int16_t x = round((float)(y - b) / m);
            display.draw_pixel(x, y, SSD1306_COLOR_ON);
          }
          display.draw_pixel(x1, y1, SSD1306_COLOR_ON);
          display.draw_pixel(x0, y0, SSD1306_COLOR_ON);
        }
      }
    }

    display.update();
    sleep_ms(1);
  }

  while (true) {
    display.clear();

    float t =
        (float)(to_ms_since_boot(get_absolute_time())) / 1000.0f * 2.0f * M_PI;
    int16_t x = 64 + (int16_t)(cos(t) * 60);
    int16_t y = 48;
    // int16_t y = 45 + (int16_t) (sin(t) * 15);

    display.draw_pixel(x, y, SSD1306_COLOR_ON);
    display.draw_pixel(x - 1, y, SSD1306_COLOR_ON);
    display.draw_pixel(x + 1, y, SSD1306_COLOR_ON);
    display.draw_pixel(x, y - 1, SSD1306_COLOR_ON);
    display.draw_pixel(x, y + 1, SSD1306_COLOR_ON);

    display.update();

    sleep_ms(2);
  }

  return 0;
}
