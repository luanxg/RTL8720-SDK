#include <stdio.h>
#include <string.h>
#include "fvad.h"
#include "device_vad.h"

typedef enum _duer_vad_status_enum {
    VAD_WAKEUP,
    VAD_SPEAKING,
    VAD_PLAYING,
} duer_vad_status_enum_t;

int init_fvad(Fvad *vad, int sample_rate)
{
    if (vad == NULL) {
        printf("vad is NULL\n");
        return -1;
    }

    if (fvad_set_mode(vad, 3) < 0) {
        printf(" vad set mode fail\n");
        fvad_free(vad);
        return -1;
    }

    if (fvad_set_sample_rate(vad, sample_rate) < 0) {
        printf("vad set sample rate fail\n");
        fvad_free(vad);
        return -1;
    }

    return 0;
}
int g_voice_cache[VAD_CACHE_SIZE] = {0};

void vad_reset_cache()
{
    g_voice_cache[0] = 0;
    g_voice_cache[1] = 0;
    g_voice_cache[2] = 0;
    g_voice_cache[3] = 0;
}

// for VAD_WAKEUP status
int g_vad_wakeup_total_len = 0;// wakeup len
int g_vad_wakeup_voice_len = 0;// wakeup voice len

// for VAD_SPEAKING status
int g_vad_speaking_silence_len = 0;// speaking silence len

int g_vad_status = VAD_PLAYING;
int g_vad_status_before = VAD_PLAYING;

char *g_vad_status_string[] = {
    "wakeup",
    "speaking",
    "playing",
};

// human finish speak
int g_vad_stop_speak = 0;
int vad_stop_speak()
{
    return g_vad_stop_speak;
}

void vad_stop_speak_done()
{
    g_vad_stop_speak = 0;
}

// get and set vad status
int vad_get_status()
{
    return g_vad_status;
}

void vad_set_status(int status)
{
    g_vad_status_before = g_vad_status;
    g_vad_status = status;
    //printf("%s -> %s\n",g_vad_status_string[g_vad_status_before],g_vad_status_string[g_vad_status]);

    if (VAD_WAKEUP == g_vad_status) {
        g_vad_wakeup_total_len = 0;
        g_vad_wakeup_voice_len = 0;
    } else if (VAD_SPEAKING == g_vad_status) {
        g_vad_speaking_silence_len = 0;
    }

    // human start speak
    if ((VAD_WAKEUP == g_vad_status_before) && (VAD_SPEAKING == g_vad_status)) {
        printf("[VAD] start speak\n");
    }

    // human stop speak
    if ((VAD_SPEAKING == g_vad_status_before) && (VAD_PLAYING == g_vad_status)) {
        printf("[VAD] stop speak\n");
        g_vad_stop_speak = 1;
    }

    // cloud vad work
    if ((VAD_WAKEUP == g_vad_status_before) && (VAD_PLAYING == g_vad_status)) {
        printf("[VAD] cloud vad work\n");
    }
}

void vad_set_playing()
{
    vad_set_status(VAD_PLAYING);
}

// snowboy notify vad that snowboy detect a hot word
void vad_set_wakeup()
{
    printf("[VAD] wakeup\n");
    vad_reset_cache();
    vad_set_status(VAD_WAKEUP);
}

// check voice per 20 ms
void vad_check_voice(int voice)
{
    if (VAD_PLAYING == vad_get_status()) {
        return;
    }

    //printf("%d ", voice);
    static int count = 0;
    int total = 0;
    int voice_time = VAD_INPUT_MS;// 20ms
    int i = 0;

    // update vad cache
    for (i = (VAD_CACHE_SIZE - 1); i > 0; --i) {
        g_voice_cache[i] = g_voice_cache[i - 1];
    }

    g_voice_cache[0] = voice;

    // check voice per 80 ms
    count += voice_time;

    if (count >= VAD_FRAME_MS) {
        for (i = 0; i < VAD_CACHE_SIZE ; ++i) {
            //printf("[%d] ",g_voice_cache[i]);
            total += g_voice_cache[i];
        }

        //printf(" Frame %d\n", total);
        count = 0;

        //vad status change:
        if (VAD_WAKEUP == vad_get_status()) {
            //printf("wakeup %d\t%d,%d,%d,%d\n",total,g_voice_cache[0],g_voice_cache[1],g_voice_cache[2],g_voice_cache[3]);
            g_vad_wakeup_total_len += VAD_FRAME_MS;

            if (g_vad_wakeup_total_len > VAD_IGNORE_HOTWORD_TIME) {
                if (total >= VAD_VOICE_NUM_IN_FRAME) {
                    g_vad_wakeup_voice_len += VAD_FRAME_MS;
                    //printf("* %d ",total);
                } else {
                    g_vad_wakeup_voice_len = 0;
                    //printf("_ %d\n",total);
                }

                if (g_vad_wakeup_voice_len >= VAD_VOICE_VALID_TIME) {
                    //printf("\nwake up -> speaking\n");
                    vad_set_status(VAD_SPEAKING);
                }
            }
        } else if (VAD_SPEAKING == vad_get_status()) {
            if (total >= VAD_VOICE_NUM_IN_FRAME) {
                g_vad_speaking_silence_len = 0;

                // check if have some silence in a speaking frame
                for (i = 0; i < VAD_CACHE_SIZE; ++i) {
                    if (g_voice_cache[i] > 0) {
                        break;
                    }
                }

                int some_silence_time = i * VAD_INPUT_MS;

                if (some_silence_time > 0) {
                    count += some_silence_time;
                    //printf("some_silence_time %d i:%d (%d,%d,%d,%d)\n", some_silence_time, i,
                      //     g_voice_cache[0], g_voice_cache[1], g_voice_cache[2], g_voice_cache[3]);
                }
            } else {
                g_vad_speaking_silence_len += VAD_FRAME_MS;
            }

            if (g_vad_speaking_silence_len >= VAD_SPEAKING_SILENCE_VALID_TIME) {
                //printf("%d speaking -> stop\n", g_vad_speaking_silence_len);
                vad_set_status(VAD_PLAYING);
            }
        }
    }
}
int device_vad(char *buff, size_t length)
{
    // check 100ms voice
    static Fvad *vad = NULL;

    if (NULL == vad) {
        vad = fvad_new();

        if (vad == NULL) {
            printf("create vad fail\n");
            return -1;
        }

        if (init_fvad(vad, SAMPLE_RATE) < 0) {
            printf("init vad fail\n");
            return -2;
        }

        //reset vad cache
        vad_reset_cache();
    }

    if ((length % VAD_20MS_SIZE) != 0) {
        printf("vad input length error : %d\n", length);
        return -4;
    }

    // init vad cache

    int voice = 0;
    int total = 0;
    int ret = 0;

    for (int i = 0; i < length; i += VAD_10MS_SIZE) {
        voice += fvad_process(vad, (int16_t *)buff + i / 2, VAD_10MS_SIZE / 2);
        total ++;

        if (2 == total) {
            // update vad cache
            ret += voice;
            vad_check_voice(voice);
            // reset
            voice = 0;
            total = 0;
        }
    }

    return ret;
}

