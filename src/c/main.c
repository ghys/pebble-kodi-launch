#include <pebble.h>

#include "movies.h"
#include "navigation.h"
#include "volume.h"
#include "playback.h"
#include "tvshows.h"
#include "livetv.h"
#include "system.h"

#define KEY_ERROR     0
#define KEY_DATA_REQUEST  1
#define KEY_POWER  2
#define KEY_VOLUME  3
#define KEY_MUTE  4
#define KEY_INPUT_TITLE  5
#define KEY_INPUT_NAME  6
#define KEY_DSP_PROGRAM  7
#define KEY_PLAYBACK_MAIN  8
#define KEY_PLAYBACK_SUB  9
#define KEY_PLAYBACK_ELAPSED  10
#define KEY_PLAYBACK_STATUS  14

#define KEY_POWER_TOGGLE  999


#define BIG_MENUITEM_HEIGHT 44
#define SMALL_MENUITEM_HEIGHT 28


static Window *s_main_window;
static TextLayer *s_text_layer;

static Window *s_menu_window = NULL;
static MenuLayer *s_menu_layer;

static GBitmap *s_volume_icon;
static GBitmap *s_volumemuted_icon;
static GBitmap *s_power_icon;
static GBitmap *s_live_icon;
static GBitmap *s_tv_icon;
static GBitmap *s_movie_icon;
static GBitmap *s_navigation_icon;
static GBitmap *s_note_icon;

static Tuple *s_error;
static Tuple *s_volume;
static Tuple *s_mute;
static Tuple *s_input_title;
static Tuple *s_input_name;
static Tuple *s_playback_main;
static Tuple *s_playback_sub;
static Tuple *s_playback_elapsed;
static Tuple *s_playback_status;

static char s_error_text[256];
static char s_input_title_text[64];
static char s_input_name_text[64];
static char s_playback_main_text[256];
static char s_playback_sub_text[256];
static char s_mute_text[16];
static char s_volume_text[8];
static char s_volume_text_label[16];
static char s_playback_status_text[32];
static char s_playback_elapsed_text[16];

/***** Main menu *****/


static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 7;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
    {
      if (s_playback_main && s_playback_sub) {
        menu_cell_basic_draw(ctx, cell_layer, s_playback_main_text, s_playback_sub_text, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "Nothing playing", NULL, NULL);
      }
      break;
    }
    case 1:
      menu_cell_basic_draw(ctx, cell_layer, s_volume_text_label, NULL, s_volume_icon);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "Navigation", NULL, s_navigation_icon);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "TV shows", NULL, s_tv_icon);
      break;
    case 4:
      menu_cell_basic_draw(ctx, cell_layer, "Movies", NULL, s_movie_icon);
      break;
    case 5:
      menu_cell_basic_draw(ctx, cell_layer, "Live TV", NULL, s_live_icon);
      break;
    case 6:
      menu_cell_basic_draw(ctx, cell_layer, "System", NULL, s_power_icon);
      break;
    //case 6:
    //  menu_cell_basic_draw(ctx, cell_layer, "Music", NULL, s_dsp_icon);
    //  break;
    default:
      break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return PBL_IF_ROUND_ELSE(
    menu_layer_is_index_selected(menu_layer, cell_index) ?
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
    (cell_index->row != 0) ? SMALL_MENUITEM_HEIGHT : BIG_MENUITEM_HEIGHT);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
    {
      playback_window_push(s_input_title_text, s_playback_status_text, s_playback_main_text, s_playback_sub_text, s_playback_elapsed_text);
      break;
    }
    case 1:
    {
      volume_window_push(s_volume_text, s_mute_text);
      break;
    }
    case 2:
    {
      navigation_window_push();
      break;
    }
    case 3:
    {
      init_tvshows_action_menu();
      break;
    }
    case 4:
    {
      init_movies_action_menu();
      break;
    }
    case 5:
    {
        init_livetv_action_menu();
        break;
    }
    case 6:
    {
        init_system_action_menu();
        break;
    }
    default:
      break;
  }
}

static void menu_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_power_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_POWER);
  s_volume_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME);
  s_volumemuted_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUMEMUTED);
  s_live_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_MIC);
  s_movie_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_MOVIE);
  s_tv_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_TV);
  s_navigation_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_NAVIGATION);
  s_note_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_NOTE);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorRed, GColorWhite);
#else
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu_layer, GColorWhite, GColorBlack);
#endif
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  gbitmap_destroy(s_power_icon);
  gbitmap_destroy(s_volume_icon);
  gbitmap_destroy(s_volumemuted_icon);
  gbitmap_destroy(s_live_icon);
  gbitmap_destroy(s_movie_icon);
  gbitmap_destroy(s_tv_icon);
  gbitmap_destroy(s_navigation_icon);
  gbitmap_destroy(s_note_icon);
}


/* App message callback */

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    s_volume = dict_find(iter, KEY_VOLUME);
    s_mute = dict_find(iter, KEY_MUTE);
    s_input_title = dict_find(iter, KEY_INPUT_TITLE);
    s_input_name = dict_find(iter, KEY_INPUT_NAME);
    s_playback_main = dict_find(iter, KEY_PLAYBACK_MAIN);
    s_playback_sub = dict_find(iter, KEY_PLAYBACK_SUB);
    s_playback_elapsed = dict_find(iter, KEY_PLAYBACK_ELAPSED);
    s_playback_status = dict_find(iter, KEY_PLAYBACK_STATUS);


    if (s_error) {
        strncpy(s_error_text, s_error->value->cstring, strlen(s_error->value->cstring));
        text_layer_set_text(s_text_layer, s_error_text);
    } else {
        snprintf(s_input_title_text, sizeof(s_input_title_text), "%s", s_input_title->value->cstring);
        snprintf(s_playback_main_text, sizeof(s_playback_main_text), "%s", s_playback_main->value->cstring);
        snprintf(s_playback_sub_text, sizeof(s_playback_sub_text), "%s", s_playback_sub->value->cstring);
        snprintf(s_playback_elapsed_text, sizeof(s_playback_elapsed_text), "%s", s_playback_elapsed->value->cstring);
        snprintf(s_playback_status_text, sizeof(s_playback_status_text), "%s", s_playback_status->value->cstring);
        snprintf(s_mute_text, sizeof(s_mute_text), "%s", s_mute->value->cstring);
        snprintf(s_volume_text, sizeof(s_volume_text), "%s %%", s_volume->value->cstring);
        snprintf(s_volume_text_label, sizeof(s_volume_text_label), "Volume: %s", s_volume->value->cstring);

        /*strncpy(s_input_name_text, s_input_name->value->cstring, strlen(s_input_name->value->cstring));
        strncpy(s_input_title_text, s_input_title->value->cstring, strlen(s_input_title->value->cstring));
        strncpy(s_playback_main_text, s_playback_main->value->cstring, strlen(s_playback_main->value->cstring));
        strncpy(s_playback_sub_text, s_playback_sub->value->cstring, strlen(s_playback_sub->value->cstring));
        strncpy(s_playback_elapsed_text, s_playback_elapsed->value->cstring, strlen(s_playback_elapsed->value->cstring));
        strncpy(s_playback_status_text, s_playback_status->value->cstring, strlen(s_playback_status->value->cstring));
        strncpy(s_mute_text, s_mute->value->cstring, strlen(s_mute->value->cstring));
        strncpy(s_volume_text, s_volume->value->cstring, strlen(s_volume->value->cstring));*/

        if (!s_menu_window) {
            // build menu now
            s_menu_window = window_create();
            window_set_window_handlers(s_menu_window, (WindowHandlers) {
                .load = menu_load,
                .unload = menu_unload,
            });
            window_stack_pop_all(false);
            window_stack_push(s_menu_window, true);
        } else {
            if (window_stack_get_top_window() == s_menu_window) {
                menu_layer_reload_data(s_menu_layer);
            } else {
                volume_window_refresh(s_volume_text, s_mute_text);
                playback_window_refresh(s_input_title_text, s_playback_status_text, s_playback_main_text, s_playback_sub_text, s_playback_elapsed_text);
            }
        }
    }
}

static void inbox_error_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "app message inbox failure: %d", reason);
}

/* Splash window */

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 50, bounds.size.w, 100));
  text_layer_set_text(s_text_layer, "Connecting...");
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  
  DictionaryIterator *iter; 
  uint8_t value = 1; 
  app_message_outbox_begin(&iter); 
  dict_write_int(iter, KEY_DATA_REQUEST, &value, 1, true); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}


/* Init */

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

#ifdef PBL_PLATFORM_APLITE
  int inbox_size = 3000;
#else
  int inbox_size = app_message_inbox_size_maximum(); // - 2800;
#endif
  APP_LOG(APP_LOG_LEVEL_INFO, "opening inbox with size=%d", inbox_size);
  app_message_open(inbox_size, 100);
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_error_handler);

  
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
