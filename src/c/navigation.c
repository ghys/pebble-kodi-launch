#include <pebble.h>
#include "navigation.h"

#include <pebble.h>
#include "volume.h"


#define KEY_NAV_KEYPRESS  30

#define VAL_NAV_LEFT     1
#define VAL_NAV_RIGHT    2
#define VAL_NAV_UP       3
#define VAL_NAV_DOWN     4
#define VAL_NAV_SELECT   5
#define VAL_NAV_BACK     6
#define VAL_NAV_CONTEXT  7
#define VAL_NAV_OSD      8
#define VAL_NAV_CODEC    9


static Window *s_main_window = NULL;
static TextLayer *s_title_layer;
static TextLayer *s_label_layer = NULL;
//static BitmapLayer *s_icon_layer;
//static ActionBarLayer *s_action_bar_layer;

//static GBitmap *s_mute_bitmap, *s_down_bitmap, *s_up_bitmap;

static void send_keypress(uint8_t key) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "Send keypress %d", key);
  DictionaryIterator *iter; 
  uint8_t value = key; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_NAV_KEYPRESS, &key, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}
static void keypress_down() {
    send_keypress(VAL_NAV_DOWN);
}
static void keypress_up() {
    send_keypress(VAL_NAV_UP);
}
static void keypress_left() {
    send_keypress(VAL_NAV_LEFT);
}
static void keypress_right() {
    send_keypress(VAL_NAV_RIGHT);
}
static void keypress_select() {
    send_keypress(VAL_NAV_SELECT);
}
static void keypress_back() {
    send_keypress(VAL_NAV_BACK);
}
static void keypress_context() {
    send_keypress(VAL_NAV_CONTEXT);
}
static void keypress_osd() {
    send_keypress(VAL_NAV_OSD);
}
static void keypress_codec() {
    send_keypress(VAL_NAV_CODEC);
}

void navigation_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_DOWN, keypress_down);
    window_single_click_subscribe(BUTTON_ID_UP, keypress_up);
    window_single_click_subscribe(BUTTON_ID_SELECT, keypress_right);
    window_single_click_subscribe(BUTTON_ID_BACK, keypress_left);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, keypress_select, NULL);
    //window_long_click_subscribe(BUTTON_ID_BACK, 0, keypress_back, NULL);
    window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 0, 0, false, keypress_select);
    window_multi_click_subscribe(BUTTON_ID_BACK, 2, 0, 0, false, keypress_back);
    window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 0, false, keypress_osd);
    //window_multi_click_subscribe(BUTTON_ID_UP, 4, 0, 0, false, keypress_codec);
    window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, false, keypress_context);
    //window_multi_click_subscribe(BUTTON_ID_BACK, 4, 0, 0, false, keypress_select);
//  window_long_click_subscribe(BUTTON_ID_DOWN, 0, volume_down_long, NULL);
//  window_long_click_subscribe(BUTTON_ID_UP, 0, volume_up_long, NULL);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    //APP_LOG(APP_LOG_LEVEL_INFO, "window load");

    //s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONFIRM);

    const GEdgeInsets title_insets = {.top = 7, .right = 0, .bottom = 56, .left = 0};
    //s_icon_layer = bitmap_layer_create(grect_inset(bounds, icon_insets));
    //bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
    //bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
    //layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
    s_title_layer = text_layer_create(grect_inset(bounds, title_insets));
    text_layer_set_text(s_title_layer, "Navigation");
    text_layer_set_background_color(s_title_layer, GColorClear);
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
    text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

    const GEdgeInsets label_insets = {.top = 50, .right = 0, .left = 0};
    s_label_layer = text_layer_create(grect_inset(bounds, label_insets));
    text_layer_set_text(s_label_layer, "Use 4 buttons to move + double clicks, hold Back button to exit");

    text_layer_set_background_color(s_label_layer, GColorClear);
    text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
    text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(s_label_layer));
    
    window_set_click_config_provider(window, (ClickConfigProvider) navigation_click_config_provider);

/*  s_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_DOWN);
  s_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_UP);
  s_mute_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_MUTE);

  s_action_bar_layer = action_bar_layer_create();
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, s_up_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, s_down_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_mute_bitmap);
  action_bar_layer_add_to_window(s_action_bar_layer, window);
  action_bar_layer_set_click_config_provider(s_action_bar_layer, (ClickConfigProvider)playback_click_config_provider); 
*/
}

static void window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_label_layer);
  //action_bar_layer_destroy(s_action_bar_layer);
  //bitmap_layer_destroy(s_icon_layer);

  //gbitmap_destroy(s_mute_bitmap);
  //gbitmap_destroy(s_up_bitmap);
  //gbitmap_destroy(s_down_bitmap);

  window_destroy(window);
  s_main_window = NULL;
}

void navigation_window_push() {
    if(!s_main_window) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Creating window");
        s_main_window = window_create();
        window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorCadetBlue, GColorWhite));
        window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_main_window, true);
}
