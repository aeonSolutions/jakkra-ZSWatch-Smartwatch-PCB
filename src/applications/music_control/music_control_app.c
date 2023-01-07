#include <music_control/music_control_ui.h>
#include <application_manager.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <events/ble_data_event.h>

// Functions needed for all applications
static void music_control_app_start(lv_obj_t *root, lv_group_t *group);
static void music_control_app_stop(void);
static void on_close_music(void);

// Functions related to app functionality
static void timer_callback(lv_timer_t *timer);

LV_IMG_DECLARE(music);

static application_t app = {
    .name = "Music",
    .icon = &music,
    .start_func = music_control_app_start,
    .stop_func = music_control_app_stop
};

static lv_timer_t *progress_timer;
static int progress_seconds;
static bool running;
static bool playing;
static int track_duration;

static void music_control_app_start(lv_obj_t *root, lv_group_t *group)
{
    progress_timer = lv_timer_create(timer_callback, 1000,  NULL);
    music_control_ui_show(root, on_close_music);
    running = true;
}

static void music_control_app_stop(void)
{
    lv_timer_del(progress_timer);
    running = false;
    music_control_ui_remove();
}

static void on_close_music(void)
{
    application_manager_app_close_request(&app);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    char buf[5 * MAX_MUSIC_FIELD_LENGTH];
    if (running && is_ble_data_event(aeh)) {
        struct ble_data_event *event = cast_ble_data_event(aeh);
        if (event->data.type == BLE_COMM_DATA_TYPE_MUSTIC_INFO) {
            snprintf(buf, sizeof(buf), "Track: %s",
                     event->data.data.music_info.track_name);
            progress_seconds = 0;
            track_duration = event->data.data.music_info.duration;
            music_control_ui_music_info(event->data.data.music_info.track_name, event->data.data.music_info.artist);
            music_control_ui_set_track_progress(0);
            playing = true;
        }

        if (event->data.type == BLE_COMM_DATA_TYPE_MUSTIC_STATE) {
            music_control_ui_set_music_state(event->data.data.music_state.playing, (((float)event->data.data.music_state.position / (float)track_duration)) * 100, event->data.data.music_state.shuffle);
            progress_seconds = event->data.data.music_state.position;
            playing = event->data.data.music_state.playing;
        }

        return false;
    }

    return false;
}

static void timer_callback(lv_timer_t *timer)
{
    if (playing) {
        progress_seconds++;
        music_control_ui_set_track_progress((((float)progress_seconds / (float)track_duration)) * 100);
    }
}

static int music_control_app_add(const struct device *arg)
{
    application_manager_add_application(&app);
    running = false;

    return 0;
}

SYS_INIT(music_control_app_add, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
APP_EVENT_LISTENER(music_control_app, app_event_handler);
APP_EVENT_SUBSCRIBE(music_control_app, ble_data_event);