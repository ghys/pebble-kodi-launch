#pragma once

#define KEY_SKIP_REV  900
#define KEY_SKIP_FWD  901
#define KEY_PAUSE     902
#define KEY_STOP      903
#define KEY_GOTO_PREV 904
#define KEY_GOTO_NEXT 905

void playback_window_push(char *source, char *status, char *mainline, char *subline, char *elapsed);
void playback_window_refresh(char *source, char *status, char *mainline, char *subline, char *elapsed);