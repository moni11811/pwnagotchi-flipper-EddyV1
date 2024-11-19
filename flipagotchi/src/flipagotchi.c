
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
    if (model == NULL || model->queue == NULL) {
        FURI_LOG_E("PWN", "Null pointer dereference prevented in flipagotchi_exec_cmd");
        return false;
    }

    if(message_queue_has_message(model->queue)) {
        PwnCommand cmd;
        message_queue_pop_message(model->queue, &cmd);
        FURI_LOG_D("PWN", "Has message (code: %d), processing...", cmd.parameterCode);

        switch(cmd.parameterCode) {
        case 0x04: {
            int face = cmd.arguments[0] - 4;

            if(face < 0) {
                face = 0;
            }

            model->pwn->face = cmd.arguments[0] - 4;

            break;
        }
        case 0x05: {
            text_message_process(model->pwn->hostname, cmd.arguments, PWNAGOTCHI_MAX_HOSTNAME_LEN);
            break;
        }
        case 0x06: {
            text_message_process(model->pwn->channel, cmd.arguments, PWNAGOTCHI_MAX_CHANNEL_LEN);
            break;
        }
        case 0x07: {
            text_message_process(model->pwn->apStat, cmd.arguments, PWNAGOTCHI_MAX_APS_LEN);
            break;
        }
        case 0x08: {
            text_message_process(model->pwn->uptime, cmd.arguments, PWNAGOTCHI_MAX_UPTIME_LEN);
            break;
        }
        case 0x09: {
            break;
        }
        case 0x0a: {
            enum PwnagotchiMode mode;

            switch(cmd.arguments[0]) {
            case 0x04:
                mode = PwnMode_Manual;
                break;
            case 0x05:
                mode = PwnMode_Auto;
                break;
            case 0x06:
                mode = PwnMode_Ai;
                break;
            default:
                mode = PwnMode_Manual;
                break;
            }
            model->pwn->mode = mode;

            break;
        }
        case 0x0b: {
            text_message_process(
                model->pwn->handshakes, cmd.arguments, PWNAGOTCHI_MAX_HANDSHAKES_LEN);
            break;
        }
        case 0x0c: {
            text_message_process(model->pwn->message, cmd.arguments, PWNAGOTCHI_MAX_MESSAGE_LEN);
            break;
        }
        }
    }

    return false;
}

static void flipagotchi_view_draw_callback(Canvas* canvas, void* _model) {
    PwnDumpModel* model = _model;
    pwnagotchi_draw_all(model->pwn, canvas);
}

static bool flipagotchi_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

static uint32_t flipagotchi_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void flipagotchi_on_irq_cb(FuriHalSerialHandle* serial_handle, FuriHalSerialRxEvent ev, void* context) {
    if (context == NULL) {
        FURI_LOG_E("PWN", "Null context in flipagotchi_on_irq_cb");
        return;
    }

    FlipagotchiApp* app = context;
    uint8_t data = furi_hal_serial_async_rx(serial_handle);

    if(ev & FuriHalSerialRxEventData) {
        furi_stream_buffer_send(app->rx_stream, &data, 1, 0);
        furi_thread_flags_set(furi_thread_get_id(app->worker_thread), WorkerEventRx);
    }
}

static void flipagotchi_push_to_list(PwnDumpModel* model, const char data) {
    if (model == NULL || model->queue == NULL) {
        FURI_LOG_E("PWN", "Null pointer detected in flipagotchi_push_to_list");
        return;
    }
    message_queue_push_byte(model->queue, data);
}

static int32_t flipagotchi_worker(void* context) {
    if (context == NULL) {
        FURI_LOG_E("PWN", "Null context in flipagotchi_worker");
        return -1;
    }

    FlipagotchiApp* app = context;

    while(true) {
        uint32_t events = furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
    }
    return 0;
}
