#pragma once

#define KEY_VOLUME_UP  11
#define KEY_VOLUME_DOWN  12

#define KEY_MUTE_TOGGLE  13

#define KEY_VOLUME_UP_LONG  21
#define KEY_VOLUME_DOWN_LONG  22

void volume_window_push();
void volume_window_refresh(char *value, char *mute);
