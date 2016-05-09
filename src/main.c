/*
This application receives location from companion app (iOS) in order to display
directional arrow and distance from another User.
*/



#include <pebble.h>

//Vector path for directional arrow
static const GPathInfo ARROW_POINTS = {
	3,(GPoint[]){{-25,0},{25,0},{0,-48}}
};

typedef enum {
  AppKeyHeading = 0,  		// Key: 0
  //AppKeyHeadingRad,        // Key: 1
  AppKeyDistance,    // Key: 1
  //AppKeyDistanceRad,      // Key: 3
} AppKeys;

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // A new message has been successfully received
	Tuple *heading_tuple = dict_find(iter, AppKeyHeading);
	Tuple *distance_tuple = dict_find(iter, AppKeyDistance);
	if(heading_tuple){
		rallyup_heading = heading_tuple->value->int32;
	}
	if(distance_tuple){
		rallyup_distance = distance_tuple->value->int32;
	}
}

int32_t rallyup_heading;
//convert to 
int32_t rallyup_distance;
static Window *s_main_window;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_background_bitmap;
static Layer *s_path_layer;
static TextLayer *s_text_layer_calib_state, *s_heading_layer;

// Largest Expected inbox and outbox message sizes
const uint32_t inbox_size = ;
const uint32_t outbox_size = ;

static GPath *s_arrow;

//might need to change heading_data based on what is sent by appMessage
static void arrow_heading_handler(CompassHeadingData heading_data) {
	//rotate arrow
	gpath_rotate_to(s_arrow, DEG_TO_TRIGANGLE(reallyup_heading) + heading_data.true_heading);//add own heading to heading_data.true_heading to get relative heading
	
	//display heading?
	static char s_heading_buf[64];
  snprintf(s_heading_buf, sizeof(s_heading_buf),
    //" %ld°\n%ld.%02ldm",
		" %ld.%02ldm",
    //TRIGANGLE_TO_DEG((long)heading_data.magnetic_heading),
    // display radians, units digit replace w distance
    (TRIGANGLE_TO_DEG((long)heading_data.magnetic_heading) * 2) / 360,
    // radians, digits after decimal
    ((TRIGANGLE_TO_DEG((long)heading_data.magnetic_heading) * 200) / 360) % 100
  );
  text_layer_set_text(s_heading_layer, s_heading_buf);
	
	
	//Modify alert layout depending on calibration state
	GRect bounds = layer_get_frame(window_get_root_layer(s_main_window));
	GRect alert_bounds;
	if(heading_data.compass_status == CompassStatusDataInvalid) {
		//tell user to move arm
		alert_bounds = GRect(0,0, bounds.size.w, bounds.size.h);
		text_layer_set_background_color(s_text_layer_calib_state, GColorBlack);
		text_layer_set_text_color(s_text_layer_calib_state, GColorWhite);
		text_layer_set_font(s_text_layer_calib_state,fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
		text_layer_set_text(s_text_layer_calib_state, "RallyUp is calibrating!\n\nMove your arm to aid calibration.");
	} else if(heading_data.compass_status == CompassStatusCalibrating) {
		//show status
		alert_bounds = GRect(0, -3, bounds.size.w, bounds.size.h / 7);
		text_layer_set_background_color(s_text_layer_calib_state, GColorClear);
    text_layer_set_text_color(s_text_layer_calib_state, GColorBlack);
    text_layer_set_font(s_text_layer_calib_state, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(s_text_layer_calib_state, "Searching...");
	}
	text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentCenter);
	layer_set_frame(text_layer_get_layer(s_text_layer_calib_state), alert_bounds);
	
	//trigger layer for refresh
	layer_mark_dirty(s_path_layer);
}

static void path_layer_update_callback(Layer *path, GContext *ctx) {
	graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
	gpath_draw_filled(ctx,s_arrow);
	
}

static void main_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	// Create the bitmap for the background and put it on the screen
  s_bitmap_layer = bitmap_layer_create(bounds);
  //s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_COMPASS_BACKGROUND);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_background_bitmap);

	//make arrow background transparent
	bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
	
	//Create layer for arrows
	s_path_layer = layer_create(bounds);
	
	//define draw for this layer
	layer_set_update_proc(s_path_layer, path_layer_update_callback);
	layer_add_child(window_layer, s_path_layer);
	
	//Initialize and define path used to draw arrow
	s_arrow = gpath_create(&ARROW_POINTS);
	
	//Move arrow to center of screen.
	GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
	gpath_move_to(s_arrow,center);
	
	// Register to be notified about inbox received events
	app_message_register_inbox_received(inbox_received_callback);

	//place text layers onto screen
	s_heading_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(40, 12),bounds.size.h*4/5,bounds.size.w/2,bounds.size.h/6));
	text_layer_set_text(s_heading_layer, "No Data");
	text_layer_set_font(s_heading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	//text_layer_set_background_color(s_heading_layer, GCompOpSet);
	layer_add_child(window_layer, text_layer_get_layer(s_heading_layer));
	
	s_text_layer_calib_state = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentLeft);
  text_layer_set_background_color(s_text_layer_calib_state, GColorClear);
	
	layer_add_child(window_layer, text_layer_get_layer(s_text_layer_calib_state));
	
	#if defined(PBL_ROUND)
		text_layer_enable_screen_text_flow_and_paging(s_text_layer_calib_state, 5);
	#endif
}

static void main_window_unload(Window *window){
	text_layer_destroy(s_heading_layer);
  text_layer_destroy(s_text_layer_calib_state);
  gpath_destroy(s_arrow);
  layer_destroy(s_path_layer);
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

static void init(){
	//initialize compassservice and set a filter to 2 degrees
	compass_service_set_heading_filter(DEG_TO_TRIGANGLE(2));
  compass_service_subscribe(&arrow_heading_handler);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}


static void deinit() {
  compass_service_unsubscribe();
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}