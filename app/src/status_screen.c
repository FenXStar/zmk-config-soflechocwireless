/*
 * Custom OLED status screen for the Sofle (128x32 SSD1306).
 * Night-sky scene with a rocket that animates across the display.
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/central.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

LV_IMG_DECLARE(hello_logo);
LV_IMG_DECLARE(anim_frame_0);
LV_IMG_DECLARE(anim_frame_1);
LV_IMG_DECLARE(anim_frame_2);
LV_IMG_DECLARE(anim_frame_3);

static lv_obj_t *sat;   /* status image (battery / layers text) */
static lv_obj_t *img;   /* artwork image */

static const lv_img_dsc_t *frames[] = {
    &hello_logo, &anim_frame_0, &anim_frame_1, &anim_frame_2, &anim_frame_3,
};

static void set_status_text(const char *text) {
  if (text == NULL || strcmp(text, "") == 0) {
    lv_label_set_text(sat, "");
    return;
  }
  lv_label_set_text(sat, text);
}

static void update_timer_cb(lv_timer_handle_t timer) {
  static uint8_t frame = 0;
  frame = (frame + 1) % 5;
  lv_image_set_src(img, frames[frame]);
}

static lv_timer_t *update_timer;

static void split_status_changed_handler(const zmk_event_t *eh) {
  struct zmk_split_peripheral_status_changed *ev =
      as_zmk_split_peripheral_status_changed(eh);
  if (ev == NULL) {
    return;
  }
  char buf[32];
  int connected = 0;
  for (int i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
    if (CONNECTION_STATE_CONNECTED == ev->connections[i].state) {
      connected++;
    }
  }
  snprintf(buf, sizeof(buf), "%d/%d halves", connected,
           CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS);
  set_status_text(buf);
}

static void layer_state_changed_handler(const zmk_event_t *eh) {
  int active = zmk_keymap_highest_layer_active();
  char buf[32];
  const char *name = zmk_keymap_layer_to_key_symbol(active);
  snprintf(buf, sizeof(buf), "Layer %d (%s)", active, name ? name : "?");
  set_status_text(buf);
}

ZMK_EVENT_HANDLER(split_status_changed, split_status_changed_handler, APPLICATION, 99);
ZMK_EVENT_HANDLER(layer_state_changed, layer_state_changed_handler, APPLICATION, 99);

static void status_screen_init(struct k_work *work) {
  lv_obj_t *scr = lv_display_get_default_screen(zmk_display_get_display());
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_text_font(sat, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_obj_set_style_text_color(sat, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(sat, LV_ALIGN_BOTTOM_LEFT, 0, -1);

  lv_obj_set_size(img, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_image_set_src(img, &hello_logo);
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);

  update_timer = lv_timer_create(update_timer_cb, 400, NULL);

  layer_state_changed_handler(NULL);
}

ZMK_DISPLAY_STATUS_SCREEN_CUSTOM(status_screen_init, )
