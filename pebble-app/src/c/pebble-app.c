#include <pebble.h>

#define CMD_THUMBS_UP    1
#define CMD_THUMBS_DOWN  2
#define CMD_NEXT         3
#define CMD_PREVIOUS     4
#define CMD_PLAY_PAUSE   5
#define CMD_REQUEST_INFO 6

static Window *s_main_window;
static TextLayer *s_station_layer;
static TextLayer *s_mode_layer;
static TextLayer *s_artist_layer;
static TextLayer *s_song_layer;
static TextLayer *s_hint_layer;

static int s_current_screen = 0;
static bool s_is_playing = false;

static char s_station_buf[64];
static char s_artist_buf[64];
static char s_song_buf[64];

static void send_command(int cmd) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) return;
  dict_write_int32(iter, MESSAGE_KEY_KEY_COMMAND, cmd);
  app_message_outbox_send();
}

static void update_hints(void) {
  if (s_current_screen == 0) {
    text_layer_set_text(s_hint_layer, "UP:Like DN:Dislike SEL:Play");
  } else {
    text_layer_set_text(s_hint_layer, "UP:Next DN:Prev SEL:Pause");
  }
}

static void update_mode(void) {
  if (s_current_screen == 0) {
    text_layer_set_text(s_mode_layer, "RATING");
  } else {
    text_layer_set_text(s_mode_layer, "PLAYBACK");
  }
}

static void show_feedback(const char *msg) {
  text_layer_set_text(s_song_layer, msg);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen == 0) {
    send_command(CMD_THUMBS_UP);
    show_feedback("Thumbs Up!");
  } else {
    send_command(CMD_NEXT);
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen == 0) {
    send_command(CMD_THUMBS_DOWN);
    show_feedback("Thumbs Down!");
  } else {
    send_command(CMD_PREVIOUS);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen == 0) {
    s_current_screen = 1;
    update_mode();
    update_hints();
  } else {
    send_command(CMD_PLAY_PAUSE);
    s_is_playing = !s_is_playing;
    show_feedback(s_is_playing ? "Playing" : "Paused");
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen == 1) {
    s_current_screen = 0;
    update_mode();
    update_hints();
    text_layer_set_text(s_song_layer, s_song_buf);
  } else {
    window_stack_pop(true);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *station_tuple = dict_find(iter, MESSAGE_KEY_KEY_STATION);
  if (station_tuple) {
    strncpy(s_station_buf, station_tuple->value->cstring, sizeof(s_station_buf) - 1);
    s_station_buf[sizeof(s_station_buf) - 1] = '\0';
    text_layer_set_text(s_station_layer, s_station_buf);
  }

  Tuple *artist_tuple = dict_find(iter, MESSAGE_KEY_KEY_ARTIST);
  if (artist_tuple) {
    strncpy(s_artist_buf, artist_tuple->value->cstring, sizeof(s_artist_buf) - 1);
    s_artist_buf[sizeof(s_artist_buf) - 1] = '\0';
    text_layer_set_text(s_artist_layer, s_artist_buf);
  }

  Tuple *song_tuple = dict_find(iter, MESSAGE_KEY_KEY_SONG);
  if (song_tuple) {
    strncpy(s_song_buf, song_tuple->value->cstring, sizeof(s_song_buf) - 1);
    s_song_buf[sizeof(s_song_buf) - 1] = '\0';
    text_layer_set_text(s_song_layer, s_song_buf);
  }

  Tuple *play_tuple = dict_find(iter, MESSAGE_KEY_KEY_PLAY_STATE);
  if (play_tuple) {
    s_is_playing = (play_tuple->value->int32 == 1);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_station_layer = text_layer_create(GRect(0, 0, bounds.size.w, 28));
  text_layer_set_font(s_station_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_station_layer, GTextAlignmentCenter);
  text_layer_set_text(s_station_layer, "Connecting...");
  layer_add_child(root, text_layer_get_layer(s_station_layer));

  s_mode_layer = text_layer_create(GRect(0, 28, bounds.size.w, 22));
  text_layer_set_font(s_mode_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
  text_layer_set_text(s_mode_layer, "RATING");
  layer_add_child(root, text_layer_get_layer(s_mode_layer));

  s_artist_layer = text_layer_create(GRect(4, 56, bounds.size.w - 8, 36));
  text_layer_set_font(s_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_artist_layer, GTextAlignmentCenter);
  text_layer_set_text(s_artist_layer, "");
  layer_add_child(root, text_layer_get_layer(s_artist_layer));

  s_song_layer = text_layer_create(GRect(4, 96, bounds.size.w - 8, 40));
  text_layer_set_font(s_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_song_layer, GTextAlignmentCenter);
  text_layer_set_text(s_song_layer, "");
  layer_add_child(root, text_layer_get_layer(s_song_layer));

  s_hint_layer = text_layer_create(GRect(0, bounds.size.h - 20, bounds.size.w, 20));
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  text_layer_set_text(s_hint_layer, "UP:Like DN:Dislike SEL:Play");
  layer_add_child(root, text_layer_get_layer(s_hint_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_station_layer);
  text_layer_destroy(s_mode_layer);
  text_layer_destroy(s_artist_layer);
  text_layer_destroy(s_song_layer);
  text_layer_destroy(s_hint_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(512, 512);

  send_command(CMD_REQUEST_INFO);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
