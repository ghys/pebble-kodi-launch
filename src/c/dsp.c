#include <pebble.h>
#include "dsp.h"


static ActionMenu *s_scene_menu;
static ActionMenuLevel *s_root_level;

/********************************* ActionMenu *********************************/

static char *sound_programs[] = {
  "Hall in Munich",
  "Hall in Vienna",
  "Chamber",
  "Cellar Club",
  "The Roxy Theatre",
  "The Bottom Line",
  "Sports",
  "Action Game",
  "Roleplaying Game",
  "Music Game",
  "Music Video",
  "Standard",
  "Sci-Fi",
  "Adventure",
  "Drama",
  "Mono Movie",
  "Surround Decoder",
  "2ch Stereo",
  "5ch Stereo"
};

#define NB_SOUND_PROGRAMS (sizeof (sound_programs) / sizeof (const char *))

static void change_sound_program(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  char *sound_program = (char *)action_menu_item_get_action_data(action);
  
  DictionaryIterator *iter; 
  app_message_outbox_begin(&iter); 
  dict_write_cstring(iter, KEY_SET_DSP_PROGRAM, sound_program); 
  dict_write_end(iter); 
  app_message_outbox_send();
}

void init_dsp_program_action_menu() {
  
  // Create the root level
  s_root_level = action_menu_level_create(NB_SOUND_PROGRAMS);

  for(unsigned int i = 0; i <= NB_SOUND_PROGRAMS; i++) {
    action_menu_level_add_action(s_root_level, sound_programs[i], change_sound_program, sound_programs[i]);
  }
  
  // Configure the ActionMenu Window about to be shown
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorVividViolet, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignTop
  };

  // Show the ActionMenu
  s_scene_menu = action_menu_open(&config);  
}