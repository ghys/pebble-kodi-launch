#include <pebble.h>
#include "volume.h"

static Window *s_main_window = NULL;
static TextLayer *s_title_layer;
static TextLayer *s_label_layer = NULL;
//static BitmapLayer *s_icon_layer;
static ActionBarLayer *s_action_bar_layer;

static GBitmap *s_mute_bitmap, *s_down_bitmap, *s_up_bitmap;

void volume_window_refresh(char *value, char *mute) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Refreshing volume: val=%s mute=%s", value, mute);

  if (!s_main_window || !s_label_layer) {
    return;
  }
  if (!strcmp(mute, "On")) {
    text_layer_set_text(s_label_layer, "Muted");
  } else {
    text_layer_set_text(s_label_layer, value);
  }
}

static void volume_down() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Volume down");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_VOLUME_DOWN, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void volume_down_long() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Volume down (long)");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_VOLUME_DOWN_LONG, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void volume_up() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Volume up");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_VOLUME_UP, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void volume_up_long() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Volume up (long)");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_VOLUME_UP_LONG, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void volume_toggle_mute() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Mute toggle");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_MUTE_TOGGLE, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}

void playback_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_DOWN, volume_down);
  window_single_click_subscribe(BUTTON_ID_UP, volume_up);
  window_single_click_subscribe(BUTTON_ID_SELECT, volume_toggle_mute);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, volume_down_long, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 0, volume_up_long, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONFIRM);

  const GEdgeInsets title_insets = {.top = 7, .right = 28, .bottom = 56, .left = 14};
  //s_icon_layer = bitmap_layer_create(grect_inset(bounds, icon_insets));
  //bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  //bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  //layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
  s_title_layer = text_layer_create(grect_inset(bounds, title_insets));
  text_layer_set_text(s_title_layer, "Volume");
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  const GEdgeInsets label_insets = {.top = 112, .right = ACTION_BAR_WIDTH, .left = ACTION_BAR_WIDTH / 2};
  s_label_layer = text_layer_create(grect_inset(bounds, label_insets));

  /*if (!strcmp(s_mute->value->cstring, "On")) {
    text_layer_set_text(s_label_layer, "Muted");
  } else {
    text_layer_set_text(s_label_layer, s_volume->value->cstring);
  }*/

  //volume_window_refresh(); // initial refresh
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  s_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_DOWN);
  s_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_UP);
  s_mute_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_MUTE);

  s_action_bar_layer = action_bar_layer_create();
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, s_up_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, s_down_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_mute_bitmap);
  action_bar_layer_add_to_window(s_action_bar_layer, window);
  action_bar_layer_set_click_config_provider(s_action_bar_layer, (ClickConfigProvider)playback_click_config_provider);
}

static void window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_label_layer);
  action_bar_layer_destroy(s_action_bar_layer);
  //bitmap_layer_destroy(s_icon_layer);

  gbitmap_destroy(s_mute_bitmap);
  gbitmap_destroy(s_up_bitmap);
  gbitmap_destroy(s_down_bitmap);

  window_destroy(window);
  s_main_window = NULL;
}

void volume_window_push(char *value, char *mute) {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorCadetBlue, GColorWhite));
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_main_window, true);
  volume_window_refresh(value, mute);
}
