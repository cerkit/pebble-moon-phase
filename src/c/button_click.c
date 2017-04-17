#include <pebble.h>
#include <pebble_fonts.h>
#include "moonPhaseCalc.c"
#define KEY_LAT 0
#define KEY_LNG 1
  
static Window *window;
static Window *phase_window;
static TextLayer *text_layer;
static TextLayer *title_layer;
static TextLayer *push_button_layer;
static GBitmap *moon_phase_image;
static RotBitmapLayer *moon_phase_bitmap_layer;
static int coord_latitude;
static int coord_longitude;
static char lat_buffer[8];

static const uint32_t MOON_PHASES[] = {
  RESOURCE_ID_IMAGE_PHASE0, //0
  RESOURCE_ID_IMAGE_PHASE1, //1
  RESOURCE_ID_IMAGE_PHASE2, //2
  RESOURCE_ID_IMAGE_PHASE3, //3
  RESOURCE_ID_IMAGE_PHASE4,
  RESOURCE_ID_IMAGE_PHASE5,
  RESOURCE_ID_IMAGE_PHASE6,
  RESOURCE_ID_IMAGE_PHASE7
};


// Phase Names

// English
static char* EN_PHASE_NAMES[] = {
  "New Moon",
  "Waxing Crescent",
  "First Quarter",
  "Waxing Gibbous",
  "Full Moon",
  "Waning Gibbous",
  "Last Quarter",
  "Waning Crescent"  
};

// French
static char* FR_PHASE_NAMES[] = {
  "nouvelle Lune",
  "Lune montante",
  "premier Quart",
  "Lune gibbeuse croissante",
  "pleine Lune",
  "Lune Gibbeuse Décroissante",
  "dernier Trimestre",
  "Lune descendante"  
};

// German
static char* DE_PHASE_NAMES[] = {
  "Neumond",
  "Zunehmender Mond",
  "Erstes Viertel",
  "Zweites Viertel",
  "Vollmond",
  "Drittes Viertel",
  "Letztes Viertel",
  "Abnehmend en Halbmond"  
};

// Spanish
static char* ES_PHASE_NAMES[] = {
  "Luna Nueva",
  "Luna Creciente",
  "primer Cuarto",
  "creciente",
  "Luna Llena",
  "Luna llena",
  "Último Cuarto",
  "Creciente Menguante"  
};

/************************************************
// Chinese
static char* CN_PHASE_NAMES[] = {
  "新月",
  "殘月",
  "第一季度",
  "殘月",
  "滿月",
  "殘月",
  "上個季度",
  "殘月"  
};
************************************************/

static void request_location(void) {  
  // Declare the dictionary's iterator
  DictionaryIterator *out_iter;
  
  // Prepare the outbox buffer for this message
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if(result == APP_MSG_OK) {
    // Add an item to ask for coordinate data
    int value = 1;
    dict_write_int(out_iter, 2, &value, sizeof(int), true);
  
    // Send this message
    result = app_message_outbox_send();
    if(result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  } else {
    // The outbox cannot be used right now
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }
}

static void showMoon(int phase, int latitude){
  char *moonPhase;

  // Use setlocale() to obtain the system locale for translation
  char *sys_locale = setlocale(LC_ALL, "");

  // Set the moonPhase depending on locale
  if (strcmp("fr_FR", sys_locale) == 0) {
    // French
    moonPhase = FR_PHASE_NAMES[phase];
  } else if (strcmp("de_DE", sys_locale) == 0) {
    // German
    moonPhase = DE_PHASE_NAMES[phase];
  } else if(strcmp("es_ES", sys_locale) == 0){
    // Spanish
    moonPhase = ES_PHASE_NAMES[phase];
  } else if(strcmp("zh_CN", sys_locale) == 0){
    // Spanish
    //moonPhase = CN_PHASE_NAMES[phase];
    moonPhase = EN_PHASE_NAMES[phase];
  }else {
    // Fall back to English
    moonPhase = EN_PHASE_NAMES[phase];
  }
  
  phase_window = window_create();
  window_set_background_color(phase_window, GColorBlack);
  
  // set up phase window
  Layer *phase_window_layer = window_get_root_layer(phase_window);
  
  GRect bounds = layer_get_bounds(phase_window_layer);
  
  text_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h - 35 }, .size = { bounds.size.w, 35 } }); 
  text_layer_set_background_color(text_layer, GColorBlack);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  
  uint32_t *phaseImageResourceHandle = resource_get_handle(MOON_PHASES[phase]);
  moon_phase_image = gbitmap_create_with_resource((uint32_t)phaseImageResourceHandle);
  moon_phase_bitmap_layer = rot_bitmap_layer_create(moon_phase_image);
  
  #if defined(PBL_ROUND) || defined(PBL_PLATFORM_EMERY)
    layer_set_bounds((Layer*)moon_phase_bitmap_layer, (GRect) { .origin = { 8, 0 }, .size = { 120, 120 } });
  #else
    layer_set_bounds((Layer*)moon_phase_bitmap_layer, (GRect) { .origin = { -15, -20 }, .size = { 120, 120 } });
  #endif
  
  // see if the watch is in the Southern hemisphere. If so, rotate the moon 180 degrees.
  if(latitude < 0) {
    rot_bitmap_layer_set_angle(moon_phase_bitmap_layer, 0x8000);  
  }
  
  layer_add_child(phase_window_layer, (Layer*)moon_phase_bitmap_layer);
  

  layer_add_child(phase_window_layer, text_layer_get_layer(text_layer));
  text_layer_set_text(text_layer, moonPhase);
  
  window_stack_push(phase_window, false);  
}

static void doPhase(int latitude) {
	// calculate moon phase
	int phase;
  int ip;
  struct tm *t;
  time_t temp;
  temp = time(NULL);
  t = localtime(&temp);
  
  int y = t->tm_year + 1900;
  int m = t->tm_mon + 1;
  int d = t->tm_mday;
  //int h = t->tm_hour;
  
  phase = moon_phase(y, m, d);
	showMoon(phase, latitude);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_LAT:
      coord_latitude = (int)t->value->int32;
      snprintf(lat_buffer, sizeof(lat_buffer), "%d", (int)t->value->int32);
      break;
    case KEY_LNG:
      coord_longitude = (int)t->value->int32;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  doPhase(coord_latitude);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  // Something went wrong retrieving coordinates, default to Northern hemisphere
  doPhase(1);
   
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void button_click_handler(ClickRecognizerRef recognizer, void *context) {
  request_location();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, button_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, button_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, button_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  title_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h / 3 }, .size = { bounds.size.w, 75 } });
  
  text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer));
  
  text_layer_set_text(title_layer, "cerkit.com Moon Phase");
  
  push_button_layer = text_layer_create((GRect) { .origin = { 0, (bounds.size.h / 3) + 71 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text_alignment(push_button_layer, GTextAlignmentCenter);
  text_layer_set_text(push_button_layer, "Push any button");
  layer_add_child(window_layer, text_layer_get_layer(push_button_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(75, 75); 
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const bool animated = false;
  window_stack_push(window, animated);   
}

static void deinit(void) {
  gbitmap_destroy(moon_phase_image);
  rot_bitmap_layer_destroy(moon_phase_bitmap_layer);
  window_destroy(window);
  window_destroy(phase_window);
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}