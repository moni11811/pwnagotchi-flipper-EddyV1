#include <furi.h>
#include <gui/gui.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <gui/elements.h>
#include <furi_hal.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/dialog_ex.h>

#include "../include/pwnagotchi.h"
#include "../include/protocol.h"
#include "../include/constants.h"
#include "../include/message_queue.h"

#define LINES_ON_SCREEN 6
#define COLUMNS_ON_SCREEN 21

typedef struct PwnDumpModel PwnDumpModel;

typedef struct {
    Gui* gui;
    NotificationApp* notification;
    ViewDispatcher* view_dispatcher;
    View* view;
    FuriThread* worker_thread;
    FuriStreamBuffer* rx_stream;
    FuriHalSerialHandle* serial_handle;
} FlipagotchiApp;

typedef struct {
    FuriString* text;
} ListElement;

struct PwnDumpModel {
    MessageQueue* queue;
    Pwnagotchi* pwn;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRx = (1 << 2),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

const NotificationSequence sequence_notification = {
    &message_display_backlight_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};

static void message_queue_push_command(MessageQueue* queue, uint8_t parameterCode, uint8_t* arguments, size_t argument_len) {
    PwnCommand cmd;
    cmd.parameterCode = parameterCode;
    memcpy(cmd.arguments, arguments, argument_len);
    message_queue_push_message(queue, &cmd);
}

static void text_message_process(FuriString* receiver, uint8_t* arguments, const unsigned int max_text_len) {
    static char charStr[2] = "\0";

    furi_string_set_str(receiver, "");

    for(size_t i = 0; i < max_text_len; i++) {
        if(arguments[i] == 0x00) {
            break;
        }

        charStr[0] = arguments[i];
        furi_string_cat_str(receiver, charStr);
    }
}

static bool flipagotchi_exec_cmd(PwnDumpModel* model) {
    if(message_queue_has_message(model->queue)) {
        PwnCommand cmd;
        message_queue_pop_message(model->queue, &cmd);

        switch(cmd.parameterCode) {
        case 0x0a: { // Process mode change
            enum PwnagotchiMode mode;
            switch(cmd.arguments[0]) {
            case 0x04:
                mode = PwnMode_Manual;
                break;
            case 0x05:
                mode = PwnMode_Auto;
                break;
            default:
                mode = PwnMode_Manual;
                break;
            }
            model->pwn->mode = mode;
            break;
        }
        default:
            break;
        }
    }

    return false;
}

static void flipagotchi_view_draw_callback(Canvas* canvas, void* _model) {
    PwnDumpModel* model = _model;
    pwnagotchi_draw_all(model->pwn, canvas);
}

static bool flipagotchi_view_input_callback(InputEvent* event, void* context) {
    PwnDumpModel* model = context;

    if(event->type == InputTypePress) {
        switch(event->key) {
            case InputKeyUp:
                message_queue_push_command(model->queue, 0x0a, (uint8_t[]){0x05}, 1);
                model->pwn->mode = PwnMode_Auto;
                notification_message(model->app->notification, &sequence_notification);
                break;

            case InputKeyDown:
                message_queue_push_command(model->queue, 0x0a, (uint8_t[]){0x04}, 1);
                model->pwn->mode = PwnMode_Manual;
                notification_message(model->app->notification, &sequence_notification);
                break;

            default:
                break;
        }
        return true;
    }
    return false;
}

static uint32_t flipagotchi_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void flipagotchi_on_irq_cb(FuriHalSerialHandle* serial_handle, FuriHalSerialRxEvent ev, void* context) {
    furi_assert(context);
    FlipagotchiApp* app = context;
    uint8_t data = furi_hal_serial_async_rx(serial_handle);

    if(ev & FuriHalSerialRxEventData) {
        furi_stream_buffer_send(app->rx_stream, &data, 1, 0);
        furi_thread_flags_set(furi_thread_get_id(app->worker_thread), WorkerEventRx);
    }
}

static void flipagotchi_push_to_list(PwnDumpModel* model, const char data) {
    message_queue_push_byte(model->queue, data);
}

static int32_t flipagotchi_worker(void* context) {
    furi_assert(context);
    FlipagotchiApp* app = context;

    while(true) {
        uint32_t events = furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        if(events & WorkerEventStop) break;

        if(events & WorkerEventRx) {
            size_t length = 0;
            do {
                uint8_t data[1];
                length = furi_stream_buffer_receive(app->rx_stream, data, 1, 0);
                if(length > 0) {
                    with_view_model(
                        app->view,
                        PwnDumpModel* model,
                        {
                            for(size_t i = 0; i < length; i++) {
                                flipagotchi_push_to_list(model, data[i]);
                            }
                            flipagotchi_exec_cmd(model);
                        },
                        true);
                }
            } while(length > 0);
        }
    }
    return 0;
}

static FlipagotchiApp* flipagotchi_app_alloc() {
    FlipagotchiApp* app = malloc(sizeof(FlipagotchiApp));

    app->rx_stream = furi_stream_buffer_alloc(2048, 1);
    app->gui = furi_record_open(RECORD_GUI);
    app->notification = furi_record_open(RECORD_NOTIFICATION);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->view = view_alloc();
    view_set_draw_callback(app->view, flipagotchi_view_draw_callback);
    view_set_input_callback(app->view, flipagotchi_view_input_callback);
    view_allocate_model(app->view, ViewModelTypeLocking, sizeof(PwnDumpModel));
    with_view_model(
        app->view,
        PwnDumpModel* model,
        {
            model->queue = message_queue_alloc();
            model->pwn = pwnagotchi_alloc();
        },
        true);

    view_set_previous_callback(app->view, flipagotchi_exit);
    view_dispatcher_add_view(app->view_dispatcher, 0, app->view);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    app->serial_handle = furi_hal_serial_control_acquire(PWNAGOTCHI_UART_CHANNEL);
    furi_hal_serial_init(app->serial_handle, PWNAGOTCHI_UART_BAUD);
    furi_hal_serial_async_rx_start(app->serial_handle, flipagotchi_on_irq_cb, app, true);

    app->worker_thread = furi_thread_alloc();
    furi_thread_set_context(app->worker_thread, app);
    furi_thread_set_callback(app->worker_thread, flipagotchi_worker);
    furi_thread_start(app->worker_thread);

    return app;
}

static void flipagotchi_app_free(FlipagotchiApp* app) {
    furi_thread_flags_set(furi_thread_get_id(app->worker_thread), WorkerEventStop);
    furi_thread_join(app->worker_thread);
    furi_thread_free(app->worker_thread);

    furi_hal_serial_deinit(app->serial_handle);
    furi_hal_serial_control_release(app->serial_handle);

    view_dispatcher_remove_view(app->view_dispatcher, 0);
    with_view_model(
        app->view,
        PwnDumpModel* model,
        {
            message_queue_free(model->queue);
            pwnagotchi_free(model->pwn);
        },
        true);
    view_free(app->view);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_stream_buffer_free(app->rx_stream);

    free(app);
}

int32_t flipagotchi_app(void* p) {
    UNUSED(p);
    FlipagotchiApp* app = flipagotchi_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    flipagotchi_app_free(app);
    return 0;
}
