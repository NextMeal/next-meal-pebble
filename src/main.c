#include <pebble.h>

#define NUM_MENU_SECTIONS 3
#define APP_VERSION 1

Window *window;
MenuLayer *menu_layer;
ScrollLayer *food_item_scroll_layer;
TextLayer *food_item_text_layer;
static GBitmap *icon_bitmap;

// Key values for AppMessage Dictionary
enum {
  MSGTYPE_KEY = 0,
  MESSAGE_KEY = 1,
	SIZE_KEY = 2
};

typedef struct FoodItem {
	char *name;
} FoodItem;

int counter = 0;
int menuSize;
bool readyToUpdate = true;
char *mealTitle;
char *lastUpdated;
FoodItem *food_item_array;
int selectedFoodIndex;

static void in_received_handler(DictionaryIterator *iter, void *context) {
	(void) context;
	Tuple *tuple;

    readyToUpdate = false;

	tuple = dict_find(iter, MSGTYPE_KEY);
	if (tuple != NULL) {

		Tuple *tmp;
        time_t currentTime;
		switch ((int)tuple->value->uint32) {
			case 0: //menu info data
				tmp = dict_find(iter, SIZE_KEY);
				menuSize = (int)tmp->value->uint32;
			 	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 0 with size: %d", menuSize);
                free(mealTitle);
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 0 with text %s", dict_find(iter, MESSAGE_KEY)->value->cstring);
                char* bufferTitle = dict_find(iter, MESSAGE_KEY)->value->cstring;
                mealTitle = calloc(strlen(bufferTitle), sizeof(char));
                snprintf(mealTitle, strlen(bufferTitle) + 1, "%s", dict_find(iter, MESSAGE_KEY)->value->cstring);
                counter++;
				//APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 0 with name: %s", mealTitle);
                if (food_item_array != NULL) { //get rid of the size 1 menu that said loading
                    free(food_item_array);
                    counter = 0;
                }
				food_item_array = calloc(menuSize, sizeof(FoodItem));
                light_enable_interaction();
				break;

			case 1: //food item
				tmp = dict_find(iter, SIZE_KEY);
                char *name = dict_find(iter, MESSAGE_KEY)->value->cstring;
				int foodItemSize = strlen(name);
				//APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 1 with size: %d", foodItemSize);
                if (food_item_array[counter].name != NULL)
                    free(food_item_array[counter].name);
				food_item_array[counter].name = calloc(foodItemSize, sizeof(char));


                bool duplicate = false;

                if (counter == menuSize) {
                    APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 menu full: %s", name);
                    duplicate = true;
                }

                if (counter > 0) {//if duplicate message sent it will overflow food item array
                    if (strcmp(food_item_array[counter - 1].name, name) == 0) {
                        APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 with duplicate name: %s", name);
                        duplicate = true;
                    }
                }

                if (duplicate == false) {
    				APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 with name: %s", name);
    				snprintf(food_item_array[counter].name, foodItemSize + 1, "%s", name);
    				counter++;
                }

				break;

            case 2: //menu done transmitting
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 2 menu done");

                currentTime = time(NULL);
                strftime(lastUpdated, sizeof("Updated @ 00:00"), "Updated @ %H:%M", localtime(&currentTime));

                readyToUpdate = true;
                light_enable_interaction();
                break;

			default:
				break;
		}

        //layer_mark_dirty((Layer*)menu_layer);
        menu_layer_reload_data(menu_layer);
	}

}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "in dropped");
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "out dropped");
}

static uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	switch ((int)section_index) {
        case 0:
            return 1;
		case 1:
            if (counter > 1) {
                return counter;
            }
            return 1;
			break;
		case 2:
			return 0;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

static uint16_t num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "num header");
	return NUM_MENU_SECTIONS;
}

static int16_t cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "header height %d", MENU_CELL_BASIC_HEADER_HEIGHT);
    if (cell_index->section == 0)
        return 40;
    else
        return 32;
}

static int16_t header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "header height %d", MENU_CELL_BASIC_HEADER_HEIGHT);
    if (section_index == 0)
        return 0;
    else
        return  MENU_CELL_BASIC_HEADER_HEIGHT;
}

void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    char* test = "Unknown";
    char* appBanner = "Next Meal";
    switch(cell_index->section) {
        case 0:
            menu_cell_basic_draw(ctx, cell_layer, appBanner, lastUpdated, icon_bitmap);
            break;
        case 1:
            graphics_context_set_text_color(ctx, GColorBlack);
            graphics_draw_text(ctx, food_item_array[cell_index->row].name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), layer_get_bounds(cell_layer), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,NULL );
            //menu_cell_basic_draw(ctx, cell_layer, food_item_array[cell_index->row].name, "", NULL);
            break;
        case 2:
            menu_cell_basic_draw(ctx, cell_layer, test, "", NULL);
            break;
        default:
            menu_cell_basic_draw(ctx, cell_layer, test, "", NULL);
            break;
    }

}

void draw_header_callback(GContext *ctx, Layer *cell_layer, uint16_t section_index, void *callback_context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "draw header index %d", (int)section_index);
    char *appVersion = calloc(9 + APP_VERSION / 10 + 1, sizeof(char));
    switch((int)section_index) {
        case 0:
            break;
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, mealTitle);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "draw header text %s", mealTitle);
            break;
        case 2:
            //menu_cell_basic_header_draw(ctx, cell_layer, lastUpdated);

            snprintf(appVersion, 9 + APP_VERSION / 10 + 2, "Revision %d", APP_VERSION);
            menu_cell_basic_header_draw(ctx, cell_layer, appVersion);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "draw header text %s", appVersion);

            break;
    }


}

void food_item_window_load(Window *window) {
    food_item_scroll_layer = scroll_layer_create(layer_get_bounds(window_get_root_layer(window)));

    food_item_text_layer = text_layer_create(GRect(0, 0, 144, 2000));
    text_layer_set_text(food_item_text_layer, food_item_array[selectedFoodIndex].name);
    text_layer_set_background_color(food_item_text_layer, GColorClear);
    text_layer_set_text_color(food_item_text_layer, GColorBlack);
    text_layer_set_text_alignment(food_item_text_layer, GTextAlignmentLeft);
    text_layer_set_font(food_item_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_overflow_mode(food_item_text_layer, GTextOverflowModeWordWrap);

    layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(food_item_scroll_layer));
    scroll_layer_set_content_size(food_item_scroll_layer, text_layer_get_content_size(food_item_text_layer));

    scroll_layer_add_child(food_item_scroll_layer, text_layer_get_layer(food_item_text_layer));

    scroll_layer_set_click_config_onto_window(food_item_scroll_layer, window);
}

void food_item_window_unload(Window *window) {
	text_layer_destroy(food_item_text_layer);
    scroll_layer_destroy(food_item_scroll_layer);
}

void select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    if (cell_index->section == 0) {

    } else {
        if (strcmp(food_item_array[cell_index->row].name, "N/A          ") == 0) {

        } else {
            selectedFoodIndex = cell_index->row;
            window = window_create();
        	window_set_window_handlers(window, (WindowHandlers) {
        		.load = food_item_window_load,
        		.unload = food_item_window_unload,
        	});
            window_stack_push(window, true);
        }
    }
}

void request_menu() {
    app_message_outbox_send();
    snprintf(lastUpdated, 15, "%s","Updating...    ");
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "request update");
    readyToUpdate = false;
    menu_layer_reload_data(menu_layer);
}

void tick_callback(struct tm *tick_time, TimeUnits units_changed)
{
    //called every minute
    if (readyToUpdate == true) {
        request_menu();
    }
}

void bluetooth_handler(bool connected) {
    if (connected) { //on reconnect, get new menu
        request_menu();
    } else {
        snprintf(lastUpdated, 15, "%s","Disconnected");
        readyToUpdate = false; //disable updates
    }
}

void menu_window_load(Window *window) {

    icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON);

    lastUpdated = calloc(15, sizeof(char));
	snprintf(lastUpdated, 15, "%s","Updating...    ");

    if (bluetooth_connection_service_peek() == false) {
        snprintf(lastUpdated, 15, "%s","Disconnected");
    }

    //make menu of one item saying loading text
    food_item_array = calloc(1, sizeof(FoodItem));
    food_item_array[counter].name = calloc(13, sizeof(char));
    snprintf(food_item_array[0].name, 14, "N/A          ");

	menu_layer = menu_layer_create(GRect(0, 0, 144, 168 - 16));

	menu_layer_set_click_config_onto_window(menu_layer, window);

	MenuLayerCallbacks callbacks = {
		.get_num_sections = (MenuLayerGetNumberOfSectionsCallback) num_sections_callback,
		.get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback) cell_height_callback,
        .get_header_height = (MenuLayerGetHeaderHeightCallback) header_height_callback,
		.draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
		.draw_header = (MenuLayerDrawHeaderCallback) draw_header_callback,
		.select_click = (MenuLayerSelectCallback) select_click_callback
	};
	menu_layer_set_callbacks(menu_layer, NULL, callbacks);

	layer_add_child(window_get_root_layer(window), menu_layer_get_layer(menu_layer));

    bluetooth_connection_service_subscribe(bluetooth_handler);

    app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
  	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    tick_timer_service_subscribe(MINUTE_UNIT, tick_callback);
}

void menu_window_unload(Window *window) {
	menu_layer_destroy(menu_layer);
    free(mealTitle);
    free(lastUpdated);

    bluetooth_connection_service_unsubscribe();
}

void init() {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = menu_window_load,
		.unload = menu_window_unload,
	});
    //window_set_fullscreen(window, true);

    window_stack_push(window, true);
}

void deinit() {
    tick_timer_service_unsubscribe();
    app_message_deregister_callbacks();
	window_destroy(window);

}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
