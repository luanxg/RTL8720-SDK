// Copyright (2017) Baidu Inc. All rights reserveed.
/**
 * File: lightduer_play_event.h
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: Play Event Handle Interface
 */

#ifndef BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_PLAY_EVENT_H
#define BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_PLAY_EVENT_H

enum PlayStatus {
    START_PLAY    = 1,
    STOP_PLAY     = 2,
    END_PLAY      = 3,
    NEXT_PLAY     = 4,
    PREVIOUS_PLAY = 5,
    REPEAT_PLAY   = 6,
};

enum PlaySource {
    PLAY_NET_FILE   = 1,
    PLAY_LOCAL_FILE = 2,
};

struct PlayInfo {
    char *message_id;     // <= 70 chars
    char *audio_item_id;  // <= 30 chars
    int topic_id;
};

/*
 * Init lightduer play event framework
 *
 * @param void: None
 *
 * @return int: Success: DUER_OK
 *              Failed:  DUER_ERR_FAILED.
 */
extern int duer_init_play_event(void);

/*
 * Set play info
 * you have to set this each you send a new speech to Duer successfully
 * Call it in Thread context (unblock)
 *
 * @param void: struct PlayInfo *
 *
 * @return int: Success: DUER_OK
 *              Failed:  DUER_ERR_FAILED.
 */
extern int duer_set_play_info(const struct PlayInfo *play_info);

/*
 * Set the  play source
 * If choose PLAY_LOCAL_FILE, the play event framework will ignore any requests
 * Call it in Thread context (unblock)
 *
 * @param void: enum PlaySource
 *              PLAY_NET_FILE
 *              PLAY_LOCAL_FILE
 *
 * @return int: Success: DUER_OK
 *              Failed:  DUER_ERR_FAILED.
 */
extern int duer_set_play_source(enum PlaySource source);

/*
 * Report play event
 *
 * @param void: enum PlayStatus
 *
 * APP have to report the start play status to the Duer server
 * when APP begin to play the URL which gets from Duer server
 *               START_PLAY
 *               STOP_PLAY
 * APP have to report the end play status to the Duer server
 * when APP play the URL which gets from Duer server end
 *               END_PLAY
 *               NEXT_PLAY
 *               PREVIOUS_PLAY
 *               REPEAT_PLAY
 *
 * @return int: Success: DUER_OK
 *              Failed:  DUER_ERR_FAILED.
 *
 */
extern int duer_report_play_status(enum PlayStatus play_status);

#endif // BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_PLAY_EVENT_H
