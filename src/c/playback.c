#include <pebble.h>
#include "playback.h"

static Window *s_main_window = NULL;
static TextLayer *s_title_layer;
static TextLayer *s_mainline_layer = NULL;
static TextLayer *s_subline_layer = NULL;
static TextLayer *s_elapsed_layer = NULL;
//static BitmapLayer *s_icon_layer;
static ActionBarLayer *s_action_bar_layer;

static GBitmap *s_pause_bitmap, *s_play_bitmap, *s_down_bitmap, *s_up_bitmap;

void playback_window_refresh(char *source, char *status, char *mainline, char *subline, char *elapsed) {

  if (!s_main_window || !s_mainline_layer || !s_subline_layer || !s_elapsed_layer) {
    return;
  }
  text_layer_set_text(s_title_layer, source);
  text_layer_set_text(s_mainline_layer, mainline);
  text_layer_set_text(s_subline_layer, subline);
  text_layer_set_text(s_elapsed_layer, elapsed);
  if (!strcmp(status, "Play")) {
    action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_pause_bitmap);
  } else {
    action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_play_bitmap);
  }
}

static void skip_rev() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Skip Rev");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_SKIP_REV, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void skip_fwd() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Skip Fwd");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_SKIP_FWD, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void goto_prev() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Goto prev");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_GOTO_PREV, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void goto_next() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Goto next");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_GOTO_NEXT, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void play_pause() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Play/Pause");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_PAUSE, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void stop() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Stop");
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_STOP, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}

void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, skip_rev);
  window_single_click_subscribe(BUTTON_ID_DOWN, skip_fwd);
  window_single_click_subscribe(BUTTON_ID_SELECT, play_pause);
  window_long_click_subscribe(BUTTON_ID_UP, 0, goto_prev, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, goto_next, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, stop, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONFIRM);

  const GEdgeInsets title_insets = {.top = 3, .right = ACTION_BAR_WIDTH, .bottom = 21, .left = ACTION_BAR_WIDTH / 4};
  //s_icon_layer = bitmap_layer_create(grect_inset(bounds, icon_insets));
  //bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  //bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  //layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
  s_title_layer = text_layer_create(grect_inset(bounds, title_insets));
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  const GEdgeInsets elapsed_insets = {.top = 18, .right = ACTION_BAR_WIDTH, .left = ACTION_BAR_WIDTH / 4};
  s_elapsed_layer = text_layer_create(grect_inset(bounds, elapsed_insets));

  text_layer_set_background_color(s_elapsed_layer, GColorClear);
  text_layer_set_text_alignment(s_elapsed_layer, GTextAlignmentCenter);
  text_layer_set_font(s_elapsed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(s_elapsed_layer));
  
  const GEdgeInsets mainline_insets = {.top = 34, .right = ACTION_BAR_WIDTH, .bottom = 56, .left = ACTION_BAR_WIDTH / 4};
  s_mainline_layer = text_layer_create(grect_inset(bounds, mainline_insets));

  text_layer_set_background_color(s_mainline_layer, GColorClear);
  text_layer_set_text_alignment(s_mainline_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mainline_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mainline_layer));

  const GEdgeInsets subline_insets = {.top = 115, .right = ACTION_BAR_WIDTH, .left = ACTION_BAR_WIDTH / 4};
  s_subline_layer = text_layer_create(grect_inset(bounds, subline_insets));

  text_layer_set_background_color(s_subline_layer, GColorClear);
  text_layer_set_text_alignment(s_subline_layer, GTextAlignmentCenter);
  text_layer_set_font(s_subline_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_subline_layer));

  s_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_SKIP_FWD);
  s_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_SKIP_REV);
  s_pause_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_PAUSE);
  s_play_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_PLAY);

  s_action_bar_layer = action_bar_layer_create();
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, s_up_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, s_down_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_pause_bitmap);
  action_bar_layer_add_to_window(s_action_bar_layer, window);
  action_bar_layer_set_click_config_provider(s_action_bar_layer, (ClickConfigProvider)click_config_provider);
}

static void window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_mainline_layer);
  text_layer_destroy(s_subline_layer);
  text_layer_destroy(s_elapsed_layer);
  action_bar_layer_destroy(s_action_bar_layer);
  //bitmap_layer_destroy(s_icon_layer);

  gbitmap_destroy(s_pause_bitmap);
  gbitmap_destroy(s_play_bitmap);
  gbitmap_destroy(s_up_bitmap);
  gbitmap_destroy(s_down_bitmap);

  window_destroy(window);
  s_main_window = NULL;
}

void playback_window_push(char *source, char *status, char *mainline, char *subline, char *elapsed) {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorMalachite, GColorWhite));
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_main_window, true);
  playback_window_refresh(source, status, mainline, subline, elapsed);
}
