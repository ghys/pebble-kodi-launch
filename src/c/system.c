#include <pebble.h>
#include "system.h"

#define KEY_SYSTEM_ACTION    200

static ActionMenu *s_system_menu;
static ActionMenuLevel *s_root_level;

/********************************* ActionMenu *********************************/

static void send_system_action(char *action) {
  DictionaryIterator *iter; 
  app_message_outbox_begin(&iter); 
  dict_write_cstring(iter, KEY_SYSTEM_ACTION, action); 
  dict_write_end(iter); 
  app_message_outbox_send();
}

static void toggle_fullscreen(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("togglefullscreen");
}

static void toggle_partymode(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("toggleinfo");
}

static void update_library(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("libraryscan");
}

static void toggle_info(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("toggleinfo");
}

static void toggle_codec(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("togglecodec");
}

static void suspend_system(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("suspend");
}

static void reboot_system(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("reboot");
}

static void shutdown_system(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  send_system_action("shutdown");
}

void init_system_action_menu() {
  
  // Create the root level
  s_root_level = action_menu_level_create(8);
  
  action_menu_level_add_action(s_root_level, "Toggle Fullscreen", toggle_fullscreen, NULL);
  action_menu_level_add_action(s_root_level, "Toggle Party mode", toggle_partymode, NULL);
  action_menu_level_add_action(s_root_level, "Update library", update_library, NULL);
  action_menu_level_add_action(s_root_level, "Toggle info", toggle_info, NULL);
  action_menu_level_add_action(s_root_level, "Toggle codec", toggle_codec, NULL);
  action_menu_level_add_action(s_root_level, "Suspend", suspend_system, NULL);
  action_menu_level_add_action(s_root_level, "Reboot", reboot_system, NULL);
  action_menu_level_add_action(s_root_level, "Shutdown", shutdown_system, NULL);
  
  // Configure the ActionMenu Window about to be shown
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignTop
  };

  // Show the ActionMenu
  s_system_menu = action_menu_open(&config);  
}