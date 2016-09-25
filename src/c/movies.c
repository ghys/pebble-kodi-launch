#include <pebble.h>
#include "movies.h"

#define KEY_ERROR     0
#define KEY_DATA_REQUEST  1
#define KEY_NB_MENU_ITEMS    9999
#define KEY_FIRST_ITEM    10000
#define ITEM_SIZE    10
#define KEY_PLAY_MOVIE    1001



static Tuple *s_error;



static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level;

AppMessageInboxReceived s_main_msg_callback;

static uint32_t s_movieids[32];


static char s_appglance[256];

static void set_app_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
    if (limit < 1) return;
    APP_LOG(APP_LOG_LEVEL_INFO, "Setting app glance to: %s", s_appglance);

    const AppGlanceSlice entry = (AppGlanceSlice) {
        .layout = {
            .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
            .subtitle_template_string = s_appglance
        },
            .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION
    };

    const AppGlanceResult result = app_glance_add_slice(session, entry);
    if (result != APP_GLANCE_RESULT_SUCCESS) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "AppGlance Error: %d", result);
    }
}


/* MOVIES LIST */

static Window *s_movies_window;
static TextLayer *s_movies_loading_layer;
static TextLayer *s_no_movies_layer;
static SimpleMenuItem *s_movies_menu_items = NULL;
static SimpleMenuSection s_movies_menu_section;
static SimpleMenuLayer *s_movies_menu_layer;
char s_movies_sortcriteria[32];

static void play_movie(int index, void *context) {
    uint32_t movieid = s_movieids[index];
    
    snprintf(s_appglance, sizeof(s_appglance), "%s", s_movies_menu_items[index].title);
    
    app_glance_reload(set_app_glance, NULL);
    
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint32(iter, KEY_PLAY_MOVIE, movieid);
	dict_write_end(iter);
	app_message_outbox_send();
    
    // then pop all & exit
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_pop_all(true);
}


static void movies_list_received(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "movies app message callback");
  
 	Tuple *nb_items_tuple = dict_find(iter, KEY_NB_MENU_ITEMS);
    if (!nb_items_tuple || nb_items_tuple->value->int32 < 1) {
        text_layer_set_text(s_movies_loading_layer, "");
        text_layer_set_text(s_no_movies_layer, "No movies");
        return;
    }

    int nb_items = nb_items_tuple->value->int32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d", nb_items);

    s_movies_menu_items = malloc(nb_items * sizeof(SimpleMenuItem));

    for (int i = 0; i < nb_items; i++) {
        Tuple *title_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 1);
        Tuple *subtitle_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 2);
        Tuple *id_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 0);
        s_movieids[i] = id_tuple->value->uint32;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "title=%s subtitle=%s", title_tuple->value->cstring, subtitle_tuple->value->cstring);
        s_movies_menu_items[i].title = title_tuple->value->cstring;
        s_movies_menu_items[i].subtitle = subtitle_tuple->value->cstring;
        s_movies_menu_items[i].icon = NULL;
        s_movies_menu_items[i].callback = play_movie;
    }

    s_movies_menu_section = (SimpleMenuSection) {
        "Movies", s_movies_menu_items, nb_items
    };

    Layer *window_layer = window_get_root_layer(s_movies_window);
	GRect bounds = layer_get_bounds(window_layer);
    
    s_movies_menu_layer = simple_menu_layer_create(bounds, s_movies_window, &s_movies_menu_section, 1, NULL);
#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_movies_menu_layer), GColorVividCerulean, GColorWhite);
#endif
    layer_add_child(window_layer, (Layer *)s_movies_menu_layer);
}

static void movies_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_movies_loading_layer = text_layer_create(GRect(0, bounds.size.h - 16, bounds.size.w, 16));
	text_layer_set_text(s_movies_loading_layer, "Loading...");
	text_layer_set_font(s_movies_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_movies_loading_layer, GTextAlignmentCenter);
	text_layer_set_background_color(s_movies_loading_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(s_movies_loading_layer));
	s_no_movies_layer = text_layer_create(GRect(0, 50, bounds.size.w, 80));
	text_layer_set_text(s_no_movies_layer, "");
	text_layer_set_font(s_no_movies_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_no_movies_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_no_movies_layer));
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, s_movies_sortcriteria);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void movies_window_unload(Window *window) {
    text_layer_destroy(s_movies_loading_layer);
    text_layer_destroy(s_no_movies_layer);
    simple_menu_layer_destroy(s_movies_menu_layer);
    window_destroy(s_movies_window);
    s_movies_window = NULL;
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "restoring main appmessage callback");
    app_message_register_inbox_received(s_main_msg_callback);
  
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "getbasicinfo");
	dict_write_end(iter);
	app_message_outbox_send();
}

static void show_all_movies(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
    s_main_msg_callback = app_message_register_inbox_received(movies_list_received);
    char *action_data = (char *)action_menu_item_get_action_data(action);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "sort criteria=%s", (char *)context);
    strncpy(s_movies_sortcriteria, action_data, strlen(action_data));
    s_movies_sortcriteria[strlen(action_data)] = '\0';

    if(!s_movies_window) {
        s_movies_window = window_create();
        //window_set_background_color(s_first_window, GColorBlack);
        window_set_window_handlers(s_movies_window, (WindowHandlers) {
            .load = movies_window_load,
            .unload = movies_window_unload,
        });
    }
    window_stack_push(s_movies_window, true);
}


/*
ACTION MENU
*/


void init_movies_action_menu() {
  
    // Create the root level
    s_root_level = action_menu_level_create(3);

    action_menu_level_add_action(s_root_level, "Recently added movies", show_all_movies, "recentmovies");
    action_menu_level_add_action(s_root_level, "Last played", show_all_movies, "lastplayedmovies");
    action_menu_level_add_action(s_root_level, "Unwatched", show_all_movies, "unwatchedmovies");
    //action_menu_level_add_action(s_root_level, "By title", show_all_movies, "moviesbytitle");

    // Configure the ActionMenu Window about to be shown
    ActionMenuConfig config = (ActionMenuConfig) {
        .root_level = s_root_level,
        .colors = {
            .background = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite),
            .foreground = GColorBlack,
        },
            .align = ActionMenuAlignCenter
    };

    // Show the ActionMenu
    s_action_menu = action_menu_open(&config);  
}
