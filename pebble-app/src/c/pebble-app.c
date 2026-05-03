#include <pebble.h>

#define APP_VERSION "1.5.0"

#define CMD_THUMBS_UP    1
#define CMD_THUMBS_DOWN  2
#define CMD_NEXT         3
#define CMD_PREVIOUS     4
#define CMD_PLAY_PAUSE   5
#define CMD_REQUEST_INFO 6
#define CMD_VOLUME_UP    7
#define CMD_VOLUME_DOWN  8

static Window *s_main_window;
static TextLayer *s_header_layer;
static TextLayer *s_station_layer;
static TextLayer *s_artist_layer;
static TextLayer *s_song_layer;
static GBitmap *s_icon_thumbs_up;
static GBitmap *s_icon_thumbs_down;
static GBitmap *s_icon_next;
static GBitmap *s_icon_prev;
static GBitmap *s_icon_play;
static GBitmap *s_icon_pause;
static GBitmap *s_icon_ellipsis;
static GBitmap *s_icon_vol_up;
static GBitmap *s_icon_vol_down;
static BitmapLayer *s_bmp_up_layer;
static BitmapLayer *s_bmp_sel_layer;
static BitmapLayer *s_bmp_down_layer;
static Layer *s_btn_bar_layer;

static int s_current_screen = 0;
static bool s_is_playing = false;
static AppTimer *s_screen_timer;

static char s_station_buf[64];
static char s_artist_buf[64];
static char s_song_buf[64];

#define NUDGE_PX 6
#define NUDGE_MS 200

static AppTimer *s_nudge_timer;
static BitmapLayer *s_nudged_layer;

static void nudge_reset_callback(void *data) {
  if (s_nudged_layer) {
    GRect frame = layer_get_frame(bitmap_layer_get_layer(s_nudged_layer));
    frame.origin.x += NUDGE_PX;
    layer_set_frame(bitmap_layer_get_layer(s_nudged_layer), frame);
    s_nudged_layer = NULL;
  }
  s_nudge_timer = NULL;
}

static void nudge_icon(BitmapLayer *layer) {
  if (s_nudge_timer) {
    app_timer_cancel(s_nudge_timer);
    nudge_reset_callback(NULL);
  }
  s_nudged_layer = layer;
  GRect frame = layer_get_frame(bitmap_layer_get_layer(layer));
  frame.origin.x -= NUDGE_PX;
  layer_set_frame(bitmap_layer_get_layer(layer), frame);
  s_nudge_timer = app_timer_register(NUDGE_MS, nudge_reset_callback, NULL);
}

static void send_command(int cmd) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) return;
  dict_write_int32(iter, MESSAGE_KEY_KEY_COMMAND, cmd);
  app_message_outbox_send();
}

static void update_btn_labels(void);
static void show_about(void);

static void screen_timer_callback(void *data) {
  s_screen_timer = NULL;
  if (s_current_screen != 0 && s_main_window) {
    s_current_screen = 0;
    update_btn_labels();
    text_layer_set_text(s_song_layer, s_song_buf);
  }
}

static void reset_screen_timer(void) {
  if (s_screen_timer) {
    app_timer_cancel(s_screen_timer);
  }
  if (s_current_screen != 0) {
    s_screen_timer = app_timer_register(15000, screen_timer_callback, NULL);
  }
}

static void update_btn_labels(void) {
  if (s_current_screen == 0) {
    bitmap_layer_set_bitmap(s_bmp_up_layer, s_icon_next);
    bitmap_layer_set_bitmap(s_bmp_sel_layer, s_icon_ellipsis);
    bitmap_layer_set_bitmap(s_bmp_down_layer, s_is_playing ? s_icon_pause : s_icon_play);
  } else if (s_current_screen == 1) {
    bitmap_layer_set_bitmap(s_bmp_up_layer, s_icon_thumbs_up);
    bitmap_layer_set_bitmap(s_bmp_sel_layer, s_icon_ellipsis);
    bitmap_layer_set_bitmap(s_bmp_down_layer, s_icon_thumbs_down);
  } else {
    bitmap_layer_set_bitmap(s_bmp_up_layer, s_icon_vol_up);
    bitmap_layer_set_bitmap(s_bmp_sel_layer, s_icon_ellipsis);
    bitmap_layer_set_bitmap(s_bmp_down_layer, s_icon_vol_down);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  nudge_icon(s_bmp_up_layer);
  if (s_current_screen == 0) {
    send_command(CMD_NEXT);
  } else if (s_current_screen == 1) {
    send_command(CMD_THUMBS_UP);
    reset_screen_timer();
  } else {
    send_command(CMD_VOLUME_UP);
    reset_screen_timer();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  nudge_icon(s_bmp_down_layer);
  if (s_current_screen == 0) {
    send_command(CMD_PLAY_PAUSE);
    s_is_playing = !s_is_playing;
    bitmap_layer_set_bitmap(s_bmp_down_layer, s_is_playing ? s_icon_pause : s_icon_play);
  } else if (s_current_screen == 1) {
    send_command(CMD_THUMBS_DOWN);
    reset_screen_timer();
  } else {
    send_command(CMD_VOLUME_DOWN);
    reset_screen_timer();
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen == 0) {
    s_current_screen = 1;
    update_btn_labels();
    reset_screen_timer();
  } else if (s_current_screen == 1) {
    s_current_screen = 2;
    update_btn_labels();
    reset_screen_timer();
  } else {
    show_about();
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_screen > 0) {
    s_current_screen--;
    update_btn_labels();
    text_layer_set_text(s_song_layer, s_song_buf);
    reset_screen_timer();
  } else {
    window_stack_pop(true);
  }
}

static void style_text_layer(TextLayer *layer) {
#ifdef PBL_COLOR
  text_layer_set_text_color(layer, GColorBlack);
  text_layer_set_background_color(layer, GColorClear);
#endif
}

static Window *s_about_window;
static TextLayer *s_about_header_layer;
static TextLayer *s_about_title_layer;
static TextLayer *s_about_dev_layer;
static TextLayer *s_about_ver_layer;
static TextLayer *s_about_copyright_layer;
static AppTimer *s_about_timer;

static void about_dismiss(void) {
  s_about_timer = NULL;
  window_stack_pop(true);
}

static void about_timer_callback(void *data) {
  about_dismiss();
}

static void about_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int w = bounds.size.w;
  int h = bounds.size.h;
  int inset = PBL_IF_ROUND_ELSE(20, 2);

#ifdef PBL_COLOR
  window_set_background_color(window, GColorBlueMoon);
#endif

  s_about_header_layer = text_layer_create(GRect(inset, h * 0 / 100, w - inset * 2, 20));
  text_layer_set_font(s_about_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_about_header_layer, GTextAlignmentCenter);
  text_layer_set_text(s_about_header_layer, "PandoraRemote");
  style_text_layer(s_about_header_layer);
  layer_add_child(root, text_layer_get_layer(s_about_header_layer));

  s_about_title_layer = text_layer_create(GRect(inset, h * 18 / 100, w - inset * 2, 36));
  text_layer_set_font(s_about_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_about_title_layer, GTextAlignmentCenter);
  text_layer_set_text(s_about_title_layer, "Pandora Remote");
  style_text_layer(s_about_title_layer);
  layer_add_child(root, text_layer_get_layer(s_about_title_layer));

  s_about_dev_layer = text_layer_create(GRect(inset, h * 39 / 100, w - inset * 2, 40));
  text_layer_set_font(s_about_dev_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_about_dev_layer, GTextAlignmentCenter);
  text_layer_set_text(s_about_dev_layer, "Developed by\nA. Marc Passy");
  style_text_layer(s_about_dev_layer);
  layer_add_child(root, text_layer_get_layer(s_about_dev_layer));

  s_about_ver_layer = text_layer_create(GRect(inset, h * 64 / 100, w - inset * 2, 24));
  text_layer_set_font(s_about_ver_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_about_ver_layer, GTextAlignmentCenter);
  text_layer_set_text(s_about_ver_layer, "v" APP_VERSION);
  style_text_layer(s_about_ver_layer);
  layer_add_child(root, text_layer_get_layer(s_about_ver_layer));

  s_about_copyright_layer = text_layer_create(GRect(inset, h * 79 / 100, w - inset * 2, 36));
  text_layer_set_font(s_about_copyright_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_about_copyright_layer, GTextAlignmentCenter);
  text_layer_set_text(s_about_copyright_layer, "Pandora is a trademark\nof Pandora Media, LLC");
  style_text_layer(s_about_copyright_layer);
  layer_add_child(root, text_layer_get_layer(s_about_copyright_layer));

  s_about_timer = app_timer_register(10000, about_timer_callback, NULL);
}

static void about_window_unload(Window *window) {
  if (s_about_timer) {
    app_timer_cancel(s_about_timer);
    s_about_timer = NULL;
  }
  text_layer_destroy(s_about_header_layer);
  text_layer_destroy(s_about_title_layer);
  text_layer_destroy(s_about_dev_layer);
  text_layer_destroy(s_about_ver_layer);
  text_layer_destroy(s_about_copyright_layer);
  window_destroy(s_about_window);
  s_about_window = NULL;
}

static void show_about(void) {
  s_about_window = window_create();
  window_set_window_handlers(s_about_window, (WindowHandlers) {
    .load = about_window_load,
    .unload = about_window_unload,
  });
  window_stack_push(s_about_window, true);
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
    if (s_current_screen == 0) {
      bitmap_layer_set_bitmap(s_bmp_down_layer, s_is_playing ? s_icon_pause : s_icon_play);
    }
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
}

static void btn_bar_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
#ifdef PBL_ROUND
  GRect frame = layer_get_frame(layer);
  int scr_h = frame.origin.y + bounds.size.h;
  int center_y = scr_h / 2 - frame.origin.y;
  int half_h = scr_h / 2;
  int bar_w = bounds.size.w / 2;
  int max_extend = bounds.size.w - bar_w;
  for (int y = 0; y < bounds.size.h; y++) {
    int dy = y - center_y;
    // Narrowest at center, wider at top/bottom
    int extend = (int)((long)dy * dy * max_extend / ((long)half_h * half_h));
    if (extend > max_extend) extend = max_extend;
    int left = max_extend - extend;
    graphics_fill_rect(ctx, GRect(left, y, bounds.size.w - left, 1), 0, GCornerNone);
  }
#else
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
}

static void relayout(Layer *root) {
  GRect bounds = layer_get_unobstructed_bounds(root);

  int inset_y = PBL_IF_ROUND_ELSE(22, 0);
  int inset_x = PBL_IF_ROUND_ELSE(20, 4);
  int text_w = bounds.size.w - inset_x * 2 - 30;
  int content_h = bounds.size.h - inset_y;

  layer_set_frame(text_layer_get_layer(s_header_layer), GRect(inset_x, inset_y, text_w, 20));
  layer_set_frame(text_layer_get_layer(s_station_layer), GRect(inset_x, inset_y + 20, text_w, 24));
  layer_set_frame(text_layer_get_layer(s_artist_layer), GRect(inset_x, inset_y + 44, text_w, 64));
  layer_set_frame(text_layer_get_layer(s_song_layer), GRect(inset_x, inset_y + 108, text_w, content_h - 108));

  int btn_w = 30;
  int btn_x = bounds.size.w - btn_w;
  int icon_size = 20;
  int btn_mid_y = bounds.size.h / 2 - icon_size / 2;
  int btn_spacing = PBL_IF_ROUND_ELSE(bounds.size.h / 5, bounds.size.h / 3);
  int btn_up_y = btn_mid_y - btn_spacing;
  int btn_down_y = btn_mid_y + btn_spacing;
  int icon_x = btn_x + (btn_w - icon_size) / 2;
#ifdef PBL_ROUND
  int bar_extra = btn_w;
  layer_set_frame(s_btn_bar_layer, GRect(btn_x - bar_extra, 0, btn_w + bar_extra, bounds.size.h));
  icon_x -= 8;
  btn_up_y -= 4;
  btn_mid_y -= 4;
  btn_down_y -= 4;
#else
  layer_set_frame(s_btn_bar_layer, GRect(btn_x, 0, btn_w, bounds.size.h));
#endif

  layer_set_frame(bitmap_layer_get_layer(s_bmp_up_layer), GRect(icon_x, btn_up_y, icon_size, icon_size));
  layer_set_frame(bitmap_layer_get_layer(s_bmp_sel_layer), GRect(icon_x, btn_mid_y, icon_size, icon_size));
  layer_set_frame(bitmap_layer_get_layer(s_bmp_down_layer), GRect(icon_x, btn_down_y, icon_size, icon_size));
}

#ifndef PBL_PLATFORM_APLITE
static void unobstructed_change_handler(AnimationProgress progress, void *context) {
  relayout(window_get_root_layer(s_main_window));
}
#endif

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(root);

#ifdef PBL_COLOR
  window_set_background_color(window, GColorBlueMoon);
#endif

  int inset_y = PBL_IF_ROUND_ELSE(22, 0);
  int inset_x = PBL_IF_ROUND_ELSE(20, 4);
  int text_w = bounds.size.w - inset_x * 2 - 30;

  s_header_layer = text_layer_create(GRect(inset_x, inset_y, text_w, 20));
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_header_layer, GTextAlignmentCenter);
  text_layer_set_text(s_header_layer, "PandoraRemote");
  style_text_layer(s_header_layer);
  layer_add_child(root, text_layer_get_layer(s_header_layer));

  s_station_layer = text_layer_create(GRect(inset_x, inset_y + 20, text_w, 24));
  text_layer_set_font(s_station_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_station_layer, GTextAlignmentCenter);
  text_layer_set_text(s_station_layer, "Connecting...");
  style_text_layer(s_station_layer);
  layer_add_child(root, text_layer_get_layer(s_station_layer));

  s_artist_layer = text_layer_create(GRect(inset_x, inset_y + 44, text_w, 64));
  text_layer_set_font(s_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_artist_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_artist_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(s_artist_layer, "");
  style_text_layer(s_artist_layer);
  layer_add_child(root, text_layer_get_layer(s_artist_layer));

  s_song_layer = text_layer_create(GRect(inset_x, inset_y + 108, text_w, 56));
  text_layer_set_font(s_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_song_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_song_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(s_song_layer, "");
  style_text_layer(s_song_layer);
  layer_add_child(root, text_layer_get_layer(s_song_layer));

  int btn_w = 30;
  s_btn_bar_layer = layer_create(GRect(bounds.size.w - btn_w, 0, btn_w, bounds.size.h));
  layer_set_update_proc(s_btn_bar_layer, btn_bar_draw);
  layer_add_child(root, s_btn_bar_layer);

  s_icon_thumbs_up = gbitmap_create_with_resource(RESOURCE_ID_ICON_THUMBS_UP);
  s_icon_thumbs_down = gbitmap_create_with_resource(RESOURCE_ID_ICON_THUMBS_DOWN);
  s_icon_next = gbitmap_create_with_resource(RESOURCE_ID_ICON_NEXT);
  s_icon_prev = gbitmap_create_with_resource(RESOURCE_ID_ICON_PREV);
  s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ICON_PLAY);
  s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_ICON_PAUSE);
  s_icon_ellipsis = gbitmap_create_with_resource(RESOURCE_ID_ICON_ELLIPSIS);
  s_icon_vol_up = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOL_UP);
  s_icon_vol_down = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOL_DOWN);

  int icon_size = 20;
  s_bmp_up_layer = bitmap_layer_create(GRect(0, 0, icon_size, icon_size));
  bitmap_layer_set_compositing_mode(s_bmp_up_layer, GCompOpSet);
  bitmap_layer_set_background_color(s_bmp_up_layer, GColorClear);
  bitmap_layer_set_alignment(s_bmp_up_layer, GAlignCenter);
  layer_add_child(root, bitmap_layer_get_layer(s_bmp_up_layer));

  s_bmp_sel_layer = bitmap_layer_create(GRect(0, 0, icon_size, icon_size));
  bitmap_layer_set_compositing_mode(s_bmp_sel_layer, GCompOpSet);
  bitmap_layer_set_background_color(s_bmp_sel_layer, GColorClear);
  bitmap_layer_set_alignment(s_bmp_sel_layer, GAlignCenter);
  layer_add_child(root, bitmap_layer_get_layer(s_bmp_sel_layer));

  s_bmp_down_layer = bitmap_layer_create(GRect(0, 0, icon_size, icon_size));
  bitmap_layer_set_compositing_mode(s_bmp_down_layer, GCompOpSet);
  bitmap_layer_set_background_color(s_bmp_down_layer, GColorClear);
  bitmap_layer_set_alignment(s_bmp_down_layer, GAlignCenter);
  layer_add_child(root, bitmap_layer_get_layer(s_bmp_down_layer));

  relayout(root);
  update_btn_labels();

#ifndef PBL_PLATFORM_APLITE
  UnobstructedAreaHandlers ua_handlers = {
    .change = unobstructed_change_handler,
  };
  unobstructed_area_service_subscribe(ua_handlers, NULL);
#endif
}

static void window_unload(Window *window) {
  if (s_screen_timer) {
    app_timer_cancel(s_screen_timer);
    s_screen_timer = NULL;
  }
#ifndef PBL_PLATFORM_APLITE
  unobstructed_area_service_unsubscribe();
#endif
  text_layer_destroy(s_header_layer);
  text_layer_destroy(s_station_layer);
  text_layer_destroy(s_artist_layer);
  text_layer_destroy(s_song_layer);
  layer_destroy(s_btn_bar_layer);
  bitmap_layer_destroy(s_bmp_up_layer);
  bitmap_layer_destroy(s_bmp_sel_layer);
  bitmap_layer_destroy(s_bmp_down_layer);
  gbitmap_destroy(s_icon_thumbs_up);
  gbitmap_destroy(s_icon_thumbs_down);
  gbitmap_destroy(s_icon_next);
  gbitmap_destroy(s_icon_prev);
  gbitmap_destroy(s_icon_play);
  gbitmap_destroy(s_icon_pause);
  gbitmap_destroy(s_icon_ellipsis);
  gbitmap_destroy(s_icon_vol_up);
  gbitmap_destroy(s_icon_vol_down);
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
