
#include "../include/pwnagotchi.h"

/*
Icons from RogueMaster at:
https://github.com/RogueMaster/flipperzero-firmware-wPlugins/commit/8c45f8e9a921f61cda78ecdb2e58a244041d3e05
*/
#include "flipagotchi_icons.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <furi.h>

Pwnagotchi* pwnagotchi_alloc() {
    Pwnagotchi* pwn = malloc(sizeof(Pwnagotchi));
    if (pwn == NULL) {
        FURI_LOG_E("PWN", "Memory allocation failed in pwnagotchi_alloc");
        return NULL;
    }

    // Set numbered values
    pwn->face = Cool;
    pwn->friendFace = NoFace;
    pwn->mode = PwnMode_Manual;

    // Set string values to initial
    pwn->channel = furi_string_alloc_set_str(CHANNEL_DEFAULT_TEXT);
    pwn->apStat = furi_string_alloc_set_str(AP_STAT_DEFAULT_TEXT);
    pwn->uptime = furi_string_alloc_set_str(UPTIME_DEFAULT_TEXT);
    pwn->hostname = furi_string_alloc_set_str(HOSTNAME_DEFAULT_TEXT);
    pwn->message = furi_string_alloc_set_str(MESSAGE_DEFAULT_TEXT);
    pwn->handshakes = furi_string_alloc_set_str(HANDSHAKES_DEFAULT_TEXT);
    pwn->friendStat = furi_string_alloc_set_str(FRIEND_STAT_DEFAULT_TEXT);

    if (pwn->channel == NULL || pwn->apStat == NULL || pwn->uptime == NULL ||
        pwn->hostname == NULL || pwn->message == NULL || pwn->handshakes == NULL ||
        pwn->friendStat == NULL) {
        FURI_LOG_E("PWN", "String allocation failed in pwnagotchi_alloc");
        pwnagotchi_free(pwn);
        return NULL;
    }

    return pwn;
}

void pwnagotchi_draw_face(Pwnagotchi* pwn, Canvas* canvas) {
    if (pwn == NULL || canvas == NULL) {
        FURI_LOG_E("PWN", "Null pointer detected in pwnagotchi_draw_face");
        return;
    }

    const Icon* currentFace;
    bool draw = true;

    switch(pwn->face) {
    case NoFace:
        draw = false;
        break;
    case DefaultFace:
        currentFace = &I_awake_flipagotchi;
        break;
    case Look_r:
        currentFace = &I_look_r_flipagotchi;
        break;
    case Look_l:
        currentFace = &I_look_l_flipagotchi;
        break;
    case Look_r_happy:
        currentFace = &I_look_r_happy_flipagotchi;
        break;
    case Look_l_happy:
        currentFace = &I_look_l_happy_flipagotchi;
        break;
    case Sleep:
        currentFace = &I_sleep_flipagotchi;
        break;
    case Sleep2:
        currentFace = &I_sleep2_flipagotchi;
        break;
    case Awake:
        currentFace = &I_awake_flipagotchi;
        break;
    case Bored:
        currentFace = &I_bored_flipagotchi;
        break;
    case Intense:
        currentFace = &I_intense_flipagotchi;
        break;
    case Cool:
        currentFace = &I_cool_flipagotchi;
        break;
    case Happy:
        currentFace = &I_happy_flipagotchi;
        break;
    case Grateful:
        currentFace = &I_grateful_flipagotchi;
        break;
    case Excited:
        currentFace = &I_excited_flipagotchi;
        break;
    case Motivated:
        currentFace = &I_motivated_flipagotchi;
        break;
    case Demotivated:
        currentFace = &I_demotivated_flipagotchi;
        break;
    case Smart:
        currentFace = &I_smart_flipagotchi;
        break;
    case Lonely:
        currentFace = &I_lonely_flipagotchi;
        break;
    case Sad:
        currentFace = &I_sad_flipagotchi;
        break;
    case Angry:
        currentFace = &I_angry_flipagotchi;
        break;
    case Friend:
        currentFace = &I_friend_flipagotchi;
        break;
    case Broken:
        currentFace = &I_broken_flipagotchi;
        break;
    case Debug:
        currentFace = &I_debug_flipagotchi;
        break;
    case Upload:
        currentFace = &I_upload_flipagotchi;
        break;
    case Upload1:
        currentFace = &I_upload1_flipagotchi;
        break;
    case Upload2:
        currentFace = &I_upload2_flipagotchi;
        break;
    default:
        draw = false;
    }

    if(draw) {
        canvas_draw_icon(canvas, PWNAGOTCHI_FACE_J, PWNAGOTCHI_FACE_I, currentFace);
    }
}

void pwnagotchi_free(Pwnagotchi* pwn) {
    if (pwn == NULL) {
        return;
    }

    furi_string_free(pwn->channel);
    furi_string_free(pwn->apStat);
    furi_string_free(pwn->uptime);
    furi_string_free(pwn->hostname);
    furi_string_free(pwn->message);
    furi_string_free(pwn->handshakes);
    furi_string_free(pwn->friendStat);

    free(pwn);
}

void pwnagotchi_draw_all(Pwnagotchi* pwn, Canvas* canvas) {
    if (pwn == NULL || canvas == NULL) {
        FURI_LOG_E("PWN", "Null pointer detected in pwnagotchi_draw_all");
        return;
    }

    pwnagotchi_draw_face(pwn, canvas);
    pwnagotchi_draw_name(pwn, canvas);
    pwnagotchi_draw_channel(pwn, canvas);
    pwnagotchi_draw_aps(pwn, canvas);
    pwnagotchi_draw_uptime(pwn, canvas);
    pwnagotchi_draw_lines(pwn, canvas);
    pwnagotchi_draw_friend(pwn, canvas);
    pwnagotchi_draw_mode(pwn, canvas);
    pwnagotchi_draw_handshakes(pwn, canvas);
    pwnagotchi_draw_message(pwn, canvas);
}
