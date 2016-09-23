#include <pebble.h>
#include "input.h"

static ActionMenu *s_scene_menu;
static ActionMenuLevel *s_root_level, *s_input_level;

/********************************* ActionMenu *********************************/

static char scenebuffer[25][2][20];
static char inputbuffer[25][2][20];

static void set_scene(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  char *scene = (char *)action_menu_item_get_action_data(action);
  
  DictionaryIterator *iter; 
  app_message_outbox_begin(&iter); 
  dict_write_cstring(iter, KEY_SET_SCENE, scene); 
  dict_write_end(iter); 
  app_message_outbox_send();
  
  
}

static void switch_input(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  char *input = (char *)action_menu_item_get_action_data(action);
  DictionaryIterator *iter; 
  app_message_outbox_begin(&iter); 
  dict_write_cstring(iter, KEY_SWITCH_INPUT, input); 
  dict_write_end(iter); 
  app_message_outbox_send(); 
}


void init_input_action_menu(DictionaryIterator *iter) {
  int nb_scenes = dict_find(iter, KEY_SCENE_COUNT)->value->int32;
  int nb_inputs = dict_find(iter, KEY_INPUT_COUNT)->value->int32;
  
  // Create the root level
  s_root_level = action_menu_level_create(nb_scenes + 1);

  for(int i = 1; i <= nb_scenes; i++) {
    strcpy(scenebuffer[i][0], dict_find(iter, KEY_SCENE_COUNT + i)->value->cstring);
    strcpy(scenebuffer[i][1], dict_find(iter, KEY_SCENE_COUNT + 50 + i)->value->cstring);
    action_menu_level_add_action(s_root_level, scenebuffer[i][1], set_scene, scenebuffer[i][0]);
  }
  
  s_input_level = action_menu_level_create(nb_inputs);
  action_menu_level_add_child(s_root_level, s_input_level, "Switch input");

  // Set up the secondary actions
  for(int i = 1; i <= nb_inputs; i++) {
    strcpy(inputbuffer[i][0], dict_find(iter, KEY_INPUT_COUNT + i)->value->cstring);
    strcpy(inputbuffer[i][1], dict_find(iter, KEY_INPUT_COUNT + 50 + i)->value->cstring);
    action_menu_level_add_action(s_input_level, inputbuffer[i][1], switch_input, inputbuffer[i][0]);
  }
  
  // Configure the ActionMenu Window about to be shown
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignCenter
  };

  // Show the ActionMenu
  s_scene_menu = action_menu_open(&config);  
}