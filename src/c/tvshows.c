#include <pebble.h>
#include "tvshows.h"

#define KEY_ERROR     0
#define KEY_DATA_REQUEST  1
#define KEY_DATA_REQUEST_ID  100
#define KEY_DATA_REQUEST_ID2    101
#define KEY_PLAY_EPISODE    1000
#define KEY_NB_MENU_ITEMS    9999
#define KEY_FIRST_ITEM    10000
#define ITEM_SIZE    10

#define MENUITEM_BUFFER_SIZE    64


static Tuple *s_error;



static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level;

AppMessageInboxReceived s_main_msg_callback;


static uint32_t s_tvshowids[32];
static uint32_t s_seasons[32];
static uint32_t s_episodeids[32];
uint32_t s_tvshowid;
char s_showtitle[64];
uint32_t s_season;

static SimpleMenuItem *s_shows_menu_items = NULL;


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



/*
EPISODE LIST
*/


static Window *s_episodes_window;
static TextLayer *s_episodes_loading_layer;
static TextLayer *s_no_episodes_layer;
static SimpleMenuItem *s_episodes_menu_items = NULL;
static SimpleMenuSection s_episodes_menu_section;
static SimpleMenuLayer *s_episodes_menu_layer;

static char *s_episodes_titles;
static char *s_episodes_subtitles;

static void play_episode(int index, void *context) {
    uint32_t episodeid = s_episodeids[index];
    
    if (context) {
        snprintf(s_appglance, sizeof(s_appglance), "%s %s", (char *)context,
                 s_episodes_titles + index * MENUITEM_BUFFER_SIZE);
    } else {
        snprintf(s_appglance, sizeof(s_appglance), "%s %s", s_episodes_titles + index * MENUITEM_BUFFER_SIZE,
                s_episodes_subtitles + index * MENUITEM_BUFFER_SIZE);
    }
    
    app_glance_reload(set_app_glance, NULL);
    
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_uint32(iter, KEY_PLAY_EPISODE, episodeid);
	dict_write_end(iter);
	app_message_outbox_send();
    
    // then pop all & exit
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_pop_all(true);
}



static void episodes_list_received(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "episodes app message callback");
  
 	Tuple *nb_items_tuple = dict_find(iter, KEY_NB_MENU_ITEMS);
    if (!nb_items_tuple || nb_items_tuple->value->int32 < 1) {
        text_layer_set_text(s_episodes_loading_layer, "");
        text_layer_set_text(s_no_episodes_layer, "No episodes");
        return;
    }

    int nb_items = nb_items_tuple->value->int32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d", nb_items);

    s_episodes_menu_items = calloc(nb_items, sizeof(SimpleMenuItem));
    s_episodes_titles = calloc(nb_items, MENUITEM_BUFFER_SIZE);
    s_episodes_subtitles = calloc(nb_items, MENUITEM_BUFFER_SIZE);

    for (int i = 0; i < nb_items; i++) {
        Tuple *title_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 1);
        Tuple *subtitle_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 2);
        Tuple *id_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 0);
        s_episodeids[i] = id_tuple->value->uint32;
        snprintf(s_episodes_titles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", title_tuple->value->cstring);
        snprintf(s_episodes_subtitles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", subtitle_tuple->value->cstring);
        s_episodes_menu_items[i].title = s_episodes_titles + i*MENUITEM_BUFFER_SIZE;
        s_episodes_menu_items[i].subtitle = s_episodes_subtitles + i*MENUITEM_BUFFER_SIZE;
        s_episodes_menu_items[i].icon = NULL;
        s_episodes_menu_items[i].callback = play_episode;
    }

    s_episodes_menu_section = (SimpleMenuSection) {
        s_showtitle, s_episodes_menu_items, nb_items
    };

    Layer *window_layer = window_get_root_layer(s_episodes_window);
	GRect bounds = layer_get_bounds(window_layer);
    
    s_episodes_menu_layer = simple_menu_layer_create(bounds, s_episodes_window, &s_episodes_menu_section, 1, s_showtitle);
#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_episodes_menu_layer), GColorVividViolet, GColorWhite);
#endif
    layer_add_child(window_layer, (Layer *)s_episodes_menu_layer);
}

static void episodes_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_episodes_loading_layer = text_layer_create(GRect(0, bounds.size.h - 16, bounds.size.w, 16));
	text_layer_set_text(s_episodes_loading_layer, "Loading...");
	text_layer_set_font(s_episodes_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_episodes_loading_layer, GTextAlignmentCenter);
	text_layer_set_background_color(s_episodes_loading_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(s_episodes_loading_layer));
	s_no_episodes_layer = text_layer_create(GRect(0, 50, bounds.size.w, 80));
	text_layer_set_text(s_no_episodes_layer, "");
	text_layer_set_font(s_no_episodes_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_no_episodes_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_no_episodes_layer));
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "episodes");
	dict_write_uint32(iter, KEY_DATA_REQUEST_ID, s_tvshowid);
	dict_write_uint32(iter, KEY_DATA_REQUEST_ID2, s_season);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void episodes_window_unload(Window *window) {
    text_layer_destroy(s_episodes_loading_layer);
    text_layer_destroy(s_no_episodes_layer);
    simple_menu_layer_destroy(s_episodes_menu_layer);
    free(s_episodes_titles);
    free(s_episodes_subtitles);
    free(s_episodes_menu_items);
    window_destroy(s_episodes_window);
    s_episodes_window = NULL;
}

static void show_episodes(int index, void *context) {
    app_message_register_inbox_received(episodes_list_received);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "sort criteria=%s", (char *)context);
    s_season = s_seasons[index];

    if(!s_episodes_window) {
        s_episodes_window = window_create();
        //window_set_background_color(s_first_window, GColorBlack);
        window_set_window_handlers(s_episodes_window, (WindowHandlers) {
            .load = episodes_window_load,
            .unload = episodes_window_unload,
        });
    }
    window_stack_push(s_episodes_window, true);
}



/*
SEASON LIST
*/



static Window *s_seasons_window;
static TextLayer *s_seasons_loading_layer;
static TextLayer *s_no_seasons_layer;
static SimpleMenuItem *s_seasons_menu_items = NULL;
static SimpleMenuSection s_seasons_menu_section;
static SimpleMenuLayer *s_seasons_menu_layer;

static char *s_seasons_titles;
static char *s_seasons_subtitles;


static void seasons_list_received(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "seasons app message callback");
  
 	Tuple *nb_items_tuple = dict_find(iter, KEY_NB_MENU_ITEMS);
    if (!nb_items_tuple || nb_items_tuple->value->int32 < 1) {
        text_layer_set_text(s_seasons_loading_layer, "");
        text_layer_set_text(s_no_seasons_layer, "No seasons");
        return;
    }

    int nb_items = nb_items_tuple->value->int32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d", nb_items);

    s_seasons_menu_items = calloc(nb_items, sizeof(SimpleMenuItem));
    s_seasons_titles = calloc(nb_items, MENUITEM_BUFFER_SIZE);
    s_seasons_subtitles = calloc(nb_items, MENUITEM_BUFFER_SIZE);

    for (int i = 0; i < nb_items; i++) {
        
        Tuple *title_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 1);
        Tuple *subtitle_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 2);
        Tuple *id_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 0);
        s_seasons[i] = id_tuple->value->uint32;
        snprintf(s_seasons_titles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", title_tuple->value->cstring);
        snprintf(s_seasons_subtitles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", subtitle_tuple->value->cstring);
        s_seasons_menu_items[i].title = s_seasons_titles + i*MENUITEM_BUFFER_SIZE;
        s_seasons_menu_items[i].subtitle = s_seasons_subtitles + i*MENUITEM_BUFFER_SIZE;
        s_seasons_menu_items[i].icon = NULL;
        s_seasons_menu_items[i].callback = show_episodes;
    }

    s_seasons_menu_section = (SimpleMenuSection) {
        s_showtitle, s_seasons_menu_items, nb_items
    };

    Layer *window_layer = window_get_root_layer(s_seasons_window);
	GRect bounds = layer_get_bounds(window_layer);
    
    s_seasons_menu_layer = simple_menu_layer_create(bounds, s_seasons_window, &s_seasons_menu_section, 1, NULL);
#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_seasons_menu_layer), GColorVividViolet, GColorWhite);
#endif
    layer_add_child(window_layer, (Layer *)s_seasons_menu_layer);
}

static void seasons_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_seasons_loading_layer = text_layer_create(GRect(0, bounds.size.h - 16, bounds.size.w, 16));
	text_layer_set_text(s_seasons_loading_layer, "Loading...");
	text_layer_set_font(s_seasons_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_seasons_loading_layer, GTextAlignmentCenter);
	text_layer_set_background_color(s_seasons_loading_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(s_seasons_loading_layer));
	s_no_seasons_layer = text_layer_create(GRect(0, 50, bounds.size.w, 80));
	text_layer_set_text(s_no_seasons_layer, "");
	text_layer_set_font(s_no_seasons_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_no_seasons_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_no_seasons_layer));
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "seasons");
	dict_write_uint32(iter, KEY_DATA_REQUEST_ID, s_tvshowid);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void seasons_window_unload(Window *window) {
    text_layer_destroy(s_seasons_loading_layer);
    text_layer_destroy(s_no_seasons_layer);
    simple_menu_layer_destroy(s_seasons_menu_layer);
    free(s_seasons_titles);
    free(s_seasons_subtitles);
    free(s_seasons_menu_items);
    window_destroy(s_seasons_window);
    s_seasons_window = NULL;
}

static void show_seasons(int index, void *context) {
    app_message_register_inbox_received(seasons_list_received);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "sort criteria=%s", (char *)context);
    snprintf(s_showtitle, sizeof(s_showtitle), "%s", s_shows_menu_items[index].title);
    s_tvshowid = s_tvshowids[index];

    if(!s_seasons_window) {
        s_seasons_window = window_create();
        //window_set_background_color(s_first_window, GColorBlack);
        window_set_window_handlers(s_seasons_window, (WindowHandlers) {
            .load = seasons_window_load,
            .unload = seasons_window_unload,
        });
    }
    window_stack_push(s_seasons_window, true);
}



/*
TV SHOW LIST
*/



static Window *s_shows_window;
static TextLayer *s_shows_loading_layer;
static TextLayer *s_no_shows_layer;
static SimpleMenuSection s_shows_menu_section;
static SimpleMenuLayer *s_shows_menu_layer;
char s_tvshows_sortcriteria[32];
static char *s_shows_titles;
static char *s_shows_subtitles;

static void tvshows_list_received(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "tv shows app message callback");
  
 	Tuple *nb_items_tuple = dict_find(iter, KEY_NB_MENU_ITEMS);
    if (!nb_items_tuple || nb_items_tuple->value->int32 < 1) {
        text_layer_set_text(s_shows_loading_layer, "");
        text_layer_set_text(s_no_shows_layer, "No TV shows");
        return;
    }

    int nb_items = nb_items_tuple->value->int32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d", nb_items);

    s_shows_menu_items = calloc(nb_items, sizeof(SimpleMenuItem));
    s_shows_titles = calloc(nb_items, MENUITEM_BUFFER_SIZE);
    s_shows_subtitles = calloc(nb_items, MENUITEM_BUFFER_SIZE);

    for (int i = 0; i < nb_items; i++) {
        Tuple *title_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 1);
        Tuple *subtitle_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 2);
        Tuple *id_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 0);
        s_tvshowids[i] = id_tuple->value->uint32;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "title=%s subtitle=%s", title_tuple->value->cstring, subtitle_tuple->value->cstring);
        
        snprintf(s_shows_titles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", title_tuple->value->cstring);
        snprintf(s_shows_subtitles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", subtitle_tuple->value->cstring);
        s_shows_menu_items[i].title = s_shows_titles + i*MENUITEM_BUFFER_SIZE;
        s_shows_menu_items[i].subtitle = s_shows_subtitles + i*MENUITEM_BUFFER_SIZE;
        s_shows_menu_items[i].icon = NULL;
        s_shows_menu_items[i].callback = show_seasons;
    }

    s_shows_menu_section = (SimpleMenuSection) {
        "TV shows", s_shows_menu_items, nb_items
    };

    Layer *window_layer = window_get_root_layer(s_shows_window);
	GRect bounds = layer_get_bounds(window_layer);
    
    s_shows_menu_layer = simple_menu_layer_create(bounds, s_shows_window, &s_shows_menu_section, 1, NULL);
#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_shows_menu_layer), GColorVividViolet, GColorWhite);
#endif
    layer_add_child(window_layer, (Layer *)s_shows_menu_layer);
}

static void tvshows_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_shows_loading_layer = text_layer_create(GRect(0, bounds.size.h - 16, bounds.size.w, 16));
	text_layer_set_text(s_shows_loading_layer, "Loading...");
	text_layer_set_font(s_shows_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_shows_loading_layer, GTextAlignmentCenter);
	text_layer_set_background_color(s_shows_loading_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(s_shows_loading_layer));
	s_no_shows_layer = text_layer_create(GRect(0, 50, bounds.size.w, 80));
	text_layer_set_text(s_no_shows_layer, "");
	text_layer_set_font(s_no_shows_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_no_shows_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_no_shows_layer));
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, s_tvshows_sortcriteria);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void tvshows_window_unload(Window *window) {
    text_layer_destroy(s_shows_loading_layer);
    text_layer_destroy(s_no_shows_layer);
    simple_menu_layer_destroy(s_shows_menu_layer);
    free(s_shows_titles);
    free(s_shows_subtitles);
    free(s_shows_menu_items);
    window_destroy(s_shows_window);
    s_shows_window = NULL;
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "restoring main appmessage callback");
    app_message_register_inbox_received(s_main_msg_callback);
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "getbasicinfo");
	dict_write_end(iter);
	app_message_outbox_send();
}

static void show_all_shows(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
    s_main_msg_callback = app_message_register_inbox_received(tvshows_list_received);
    char *action_data = (char *)action_menu_item_get_action_data(action);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "sort criteria=%s", (char *)context);
    strncpy(s_tvshows_sortcriteria, action_data, strlen(action_data));
    s_tvshows_sortcriteria[strlen(action_data)] = '\0';

    if(!s_shows_window) {
        s_shows_window = window_create();
        //window_set_background_color(s_first_window, GColorBlack);
        window_set_window_handlers(s_shows_window, (WindowHandlers) {
            .load = tvshows_window_load,
            .unload = tvshows_window_unload,
        });
    }
    window_stack_push(s_shows_window, true);
}



/*
RECENTLY ADDED
*/

static Window *s_recent_window;
static TextLayer *s_recent_loading_layer;
static TextLayer *s_no_recent_layer;
static SimpleMenuItem *s_recent_menu_items = NULL;
static SimpleMenuSection s_recent_menu_section;
static SimpleMenuLayer *s_recent_menu_layer;

static void recent_episodes_received(DictionaryIterator *iter, void *context) {
    s_error = dict_find(iter, KEY_ERROR);
    
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "recent episodes app message callback");
  
 	Tuple *nb_items_tuple = dict_find(iter, KEY_NB_MENU_ITEMS);
    if (!nb_items_tuple || nb_items_tuple->value->int32 < 1) {
        text_layer_set_text(s_recent_loading_layer, "");
        text_layer_set_text(s_no_recent_layer, "No recent episodes");
        return;
    }

    int nb_items = nb_items_tuple->value->int32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d", nb_items);

    s_recent_menu_items = calloc(nb_items, sizeof(SimpleMenuItem));
    s_episodes_menu_items = calloc(nb_items, sizeof(SimpleMenuItem));
    s_episodes_titles = calloc(nb_items, MENUITEM_BUFFER_SIZE);
    s_episodes_subtitles = calloc(nb_items, MENUITEM_BUFFER_SIZE);


    for (int i = 0; i < nb_items; i++) {
        Tuple *title_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 1);
        Tuple *subtitle_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 2);
        Tuple *id_tuple = dict_find(iter, KEY_FIRST_ITEM + (i * ITEM_SIZE) + 0);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "show=%s episode=%s", title_tuple->value->cstring, subtitle_tuple->value->cstring);
        s_episodeids[i] = id_tuple->value->uint32;
        snprintf(s_episodes_titles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", title_tuple->value->cstring);
        snprintf(s_episodes_subtitles + i*MENUITEM_BUFFER_SIZE, MENUITEM_BUFFER_SIZE, "%s", subtitle_tuple->value->cstring);
        s_recent_menu_items[i].title = s_episodes_titles + i*MENUITEM_BUFFER_SIZE;
        s_recent_menu_items[i].subtitle = s_episodes_subtitles + i*MENUITEM_BUFFER_SIZE;
        s_recent_menu_items[i].icon = NULL;
        s_recent_menu_items[i].callback = play_episode;
    }

    s_recent_menu_section = (SimpleMenuSection) {
        "Recent episodes", s_recent_menu_items, nb_items
    };

    Layer *window_layer = window_get_root_layer(s_recent_window);
	GRect bounds = layer_get_bounds(window_layer);
    
    s_recent_menu_layer = simple_menu_layer_create(bounds, s_recent_window, &s_recent_menu_section, 1, NULL);
#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(s_recent_menu_layer), GColorVividViolet, GColorWhite);
#endif
    layer_add_child(window_layer, (Layer *)s_recent_menu_layer);
}

static void recent_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_recent_loading_layer = text_layer_create(GRect(0, bounds.size.h - 16, bounds.size.w, 16));
	text_layer_set_text(s_recent_loading_layer, "Loading...");
	text_layer_set_font(s_recent_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(s_recent_loading_layer, GTextAlignmentCenter);
	text_layer_set_background_color(s_recent_loading_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(s_recent_loading_layer));
	s_no_recent_layer = text_layer_create(GRect(0, 50, bounds.size.w, 80));
	text_layer_set_text(s_no_recent_layer, "");
	text_layer_set_font(s_no_recent_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_no_recent_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_no_recent_layer));
    
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "recentepisodes");
	dict_write_end(iter);
	app_message_outbox_send();
}

static void recent_window_unload(Window *window) {
    text_layer_destroy(s_recent_loading_layer);
    text_layer_destroy(s_no_recent_layer);
    simple_menu_layer_destroy(s_recent_menu_layer);
    free(s_shows_titles);
    free(s_shows_subtitles);
    free(s_shows_menu_items);
    window_destroy(s_recent_window);
    s_recent_window = NULL;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "restoring main appmessage callback");
    app_message_register_inbox_received(s_main_msg_callback);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_DATA_REQUEST, "getbasicinfo");
	dict_write_end(iter);
	app_message_outbox_send();
}

static void show_recently_added(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
    s_main_msg_callback = app_message_register_inbox_received(recent_episodes_received);

    if(!s_recent_window) {
        s_recent_window = window_create();
        //window_set_background_color(s_first_window, GColorBlack);
        window_set_window_handlers(s_recent_window, (WindowHandlers) {
            .load = recent_window_load,
            .unload = recent_window_unload,
        });
    }
    window_stack_push(s_recent_window, true);
}




/*
ACTION MENU
*/




void init_tvshows_action_menu() {
  
    // Create the root level
    s_root_level = action_menu_level_create(3);

    action_menu_level_add_action(s_root_level, "Recently added episodes", show_recently_added, NULL);
    action_menu_level_add_action(s_root_level, "Last played", show_all_shows, "lastplayedshows");
    action_menu_level_add_action(s_root_level, "Unwatched", show_all_shows, "unwatchedepisodes");
    //action_menu_level_add_action(s_root_level, "By title", show_all_shows, "showsbytitle");

    // Configure the ActionMenu Window about to be shown
    ActionMenuConfig config = (ActionMenuConfig) {
        .root_level = s_root_level,
        .colors = {
            .background = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorWhite),
            .foreground = GColorBlack,
        },
            .align = ActionMenuAlignCenter
    };

    // Show the ActionMenu
    s_action_menu = action_menu_open(&config);  
}
