// SPDX-License-Identifier: MIT
#include "pico/stdlib.h"
#include "tusb.h"

enum {
  BLINK_UNMOUNTED_MS = 250,
  BLINK_MOUNTED_MS = 1000,
  BLINK_SUSPENDED_MS = 2500,
};

static uint32_t blink_interval_ms = BLINK_UNMOUNTED_MS;

static void blink_task(void) {
  static uint32_t last_ms;
  static bool led_on;
  uint32_t now = to_ms_since_boot(get_absolute_time());
  if ((uint32_t)(now - last_ms) < blink_interval_ms) return;
  last_ms = now;
  led_on = !led_on;
  gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

void tud_mount_cb(void) { blink_interval_ms = BLINK_MOUNTED_MS; }
void tud_umount_cb(void) { blink_interval_ms = BLINK_UNMOUNTED_MS; }
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED_MS;
}
void tud_resume_cb(void) {
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED_MS : BLINK_UNMOUNTED_MS;
}

int main(void) {
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  tusb_rhport_init_t init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(BOARD_TUD_RHPORT, &init);

  for (;;) {
    tud_task();
    blink_task();
  }
}
