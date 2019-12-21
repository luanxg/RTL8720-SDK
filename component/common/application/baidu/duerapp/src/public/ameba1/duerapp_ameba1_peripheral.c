#include "duerapp.h"
#include "duerapp_config.h"
#include "duerapp_audio.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "timer_api.h"
#include "lightduer_timers.h"
#include "lightduer_key.h"
#include "lightduer_types.h"

#define GPIO_IRQ_PIN            	PC_5
#define GPIO_PLAY_STOP_PIN			PB_4
#define GPIO_PLAY_VOLUME_UP			PD_4
#define GPIO_PLAY_VOLUME_DOWN		PD_5
#define GPIO_PLAY_STOP				PD_6
#define GPIO_LED_WING_LEFT_PIN		PA_4
#define GPIO_LED_WING_RIGHT_PIN		PA_5
#define GPIO_LED_HEAD_PIN			PA_6

#define CODEC_INIT  	alc5680_i2c_init
#define SET_VOLUME		set_alc5680_volume
#define GET_VOLUME		get_alc5680_volume

#define VOLUME_MAX (10)
#define VOLUME_MIX (0)

gpio_irq_t 	duer_play_stop_button;
gpio_irq_t 	duer_play_volume_up_button;
gpio_irq_t 	duer_play_volume_down_button;

gpio_irq_t 	duer_play_stop;

gtimer_t 	duer_gpio_timer;

gpio_t 		duer_gpio_led_wing_left;
gpio_t 		duer_gpio_led_wing_right;
gpio_t 		duer_gpio_led_head;
gpio_t 		duer_gpio_led_voice;

//1010
u8 volume_5680_value[] = {0x00, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82, 0x8c, 0x96, 0xa0, 0xaf};
//u8 volume_5680_value[] = {0x00, 0x0d, 0x1f, 0x31, 0x43, 0x55, 0x67, 0x79, 0x8b, 0x9d, 0xaf};


int current_volume_index = 0;

extern duer_timer_handler g_restart_timer;

static void duerapp_gtimer_fast_timeout_handler(uint32_t id)
{
    gpio_write(&duer_gpio_led_voice, !gpio_read(&duer_gpio_led_voice));
}

static void duerapp_gtimer_slow_timeout_handler(uint32_t id)
{
    gpio_write(&duer_gpio_led_wing_left, !gpio_read(&duer_gpio_led_wing_left));
    gpio_write(&duer_gpio_led_wing_right, !gpio_read(&duer_gpio_led_wing_right));
    gpio_write(&duer_gpio_led_head, !gpio_read(&duer_gpio_led_head));
}

void duerapp_gpio_led_mode(duer_led_mode_t mode)
{
    gtimer_stop(&duer_gpio_timer);
    switch(mode){
        case DUER_LED_ON:
            gpio_write(&duer_gpio_led_wing_left, 1);
            gpio_write(&duer_gpio_led_wing_right, 1);
            gpio_write(&duer_gpio_led_head, 1);
            gpio_write(&duer_gpio_led_voice, 0);
            break;
        case DUER_LED_OFF:
            gpio_write(&duer_gpio_led_wing_left, 0);
            gpio_write(&duer_gpio_led_wing_right, 0);
            gpio_write(&duer_gpio_led_head, 0);
            gpio_write(&duer_gpio_led_voice, 1);
            break;
        case DUER_LED_FAST_TWINKLE_150:
            gtimer_start_periodical(&duer_gpio_timer, 150000,
                    (void*)duerapp_gtimer_fast_timeout_handler, NULL);
            break;
        case DUER_LED_FAST_TWINKLE_300:
            gtimer_start_periodical(&duer_gpio_timer, 300000,
                    (void*)duerapp_gtimer_fast_timeout_handler, NULL);
            break;
        case DUER_LED_SLOW_TWINKLE:
            gtimer_start_periodical(&duer_gpio_timer, 800000,
                    (void*)duerapp_gtimer_slow_timeout_handler, NULL);
            break;
        default:
            DUER_LOGE("Unknown GPIO LED mode!!");
            break;
    }
}

void duerapp_play_mp3_volume_set(int index)
{		
    if(index < VOLUME_MIX)
        index = VOLUME_MIX;
    if(index > VOLUME_MAX)
        index = VOLUME_MAX;

	current_volume_index = index;

    SET_VOLUME(volume_5680_value[current_volume_index], volume_5680_value[current_volume_index]);
    DUER_LOGI("Volume Set, index %d, value %d\r\n", current_volume_index, volume_5680_value[current_volume_index]);
}

u16 duerapp_play_mp3_volume_get()
{
    u16 volume_16 = 0;
    u16 volume_index = 0;

    GET_VOLUME(&volume_16);
    DUER_LOGD("Volume Register Data %x\r\n", volume_16);

    switch(volume_16){
        case 0:
            volume_index = 0;
            break;
        case 20560:
            volume_index = 1;
            break;
        case 23130:
            volume_index = 2;
            break;
        case 25700:
            volume_index = 3;
            break;
        case 28270:
            volume_index = 4;
            break;
        case 30840:
            volume_index = 5;
            break;
		case 33410:
            volume_index = 6;
            break;
        case 35980:
            volume_index = 7;
            break;
        case 38550:  //0x96(150)
            volume_index = 8;
			break;
		case 41120:
            volume_index = 9;
            break;
        case 44975: //0xaf(175)
            volume_index = 10;
            break;
        default:
            volume_index = 0;
            SET_VOLUME(volume_5680_value[0], volume_5680_value[0]);
            break;
    }

    return volume_index;
}

static void dueros_stop_trigger_irq_handler (uint32_t id, gpio_irq_event event)
{
    DUER_LOGI("Reset button trigger\n\r");

    duer_stop();

    duer_timer_start(g_restart_timer, 600);
}

static void play_volume_up_trigger_irq_handler (uint32_t id, gpio_irq_event event)
{
    DUER_LOGI("play volume up button trigger\r\n");
    duerapp_media_volume_up();
}

static void play_volume_down_trigger_irq_handler (uint32_t id, gpio_irq_event event)
{
    DUER_LOGI("play volume down button trigger\r\n");
    duerapp_media_volume_down();
}

static void play_stop_trigger_irq_handler (uint32_t id, gpio_irq_event event)
{
   /* int ret;
    DUER_LOGI("play next button trigger\r\n");
    ret = duer_report_key_event(NEXT_KEY, id);
    if (ret != DUER_OK) {
        // Error handle
    }*/
}

void init_voice_trigger_irq(void (*callback) (uint32_t id, gpio_irq_event event))
{
	gpio_irq_t voice_irq;

	//init voice trigger pin
	gpio_irq_init(&voice_irq, GPIO_IRQ_PIN, callback, NULL);
	gpio_irq_set(&voice_irq, IRQ_RISE, 1);
	gpio_irq_enable(&voice_irq);
}

void initialize_peripheral(void)
{
    DUER_LOGI("Initializing Peripheral....\n");

    gpio_irq_t play_stop_irq;
    gtimer_t 		gpio_timer;
    //init voice trigger pin
    gpio_irq_init(&duer_play_stop_button, GPIO_PLAY_STOP_PIN, dueros_stop_trigger_irq_handler, NULL);
    gpio_irq_set(&duer_play_stop_button, IRQ_FALL, 1);
    gpio_irq_enable(&duer_play_stop_button);

    //init voice trigger pin
    gpio_irq_init(&duer_play_volume_up_button, GPIO_PLAY_VOLUME_UP, play_volume_up_trigger_irq_handler, NULL);
    gpio_irq_set(&duer_play_volume_up_button, IRQ_FALL, 1);
    gpio_irq_enable(&duer_play_volume_up_button);

    //init voice trigger pin
    gpio_irq_init(&duer_play_volume_down_button, GPIO_PLAY_VOLUME_DOWN, play_volume_down_trigger_irq_handler, NULL);
    gpio_irq_set(&duer_play_volume_down_button, IRQ_FALL, 1);
    gpio_irq_enable(&duer_play_volume_down_button);

    gpio_irq_init(&duer_play_stop, GPIO_PLAY_STOP, play_stop_trigger_irq_handler, NULL);
    gpio_irq_set(&duer_play_stop, IRQ_FALL, 1);
    gpio_irq_enable(&duer_play_stop);


    gtimer_init(&duer_gpio_timer, TIMER1);
    //init LED control pin
    gpio_init(&duer_gpio_led_wing_left, GPIO_LED_WING_LEFT_PIN);
    gpio_dir(&duer_gpio_led_wing_left, PIN_OUTPUT);
    gpio_mode(&duer_gpio_led_wing_left, PullNone);
    gpio_write(&duer_gpio_led_wing_left, led_off);

    gpio_init(&duer_gpio_led_wing_right, GPIO_LED_WING_RIGHT_PIN);
    gpio_dir(&duer_gpio_led_wing_right, PIN_OUTPUT);
    gpio_mode(&duer_gpio_led_wing_right, PullNone);
    gpio_write(&duer_gpio_led_wing_right, led_off);

    gpio_init(&duer_gpio_led_head, GPIO_LED_HEAD_PIN);
    gpio_dir(&duer_gpio_led_head, PIN_OUTPUT);
    gpio_mode(&duer_gpio_led_head, PullNone);
    gpio_write(&duer_gpio_led_head, led_off);

    gpio_init(&duer_gpio_led_voice, GPIO_LED_VOICE);
    gpio_dir(&duer_gpio_led_voice, PIN_OUTPUT);
    gpio_mode(&duer_gpio_led_voice, PullNone);
    gpio_write(&duer_gpio_led_voice, led_off);

    duerapp_gpio_led_mode(DUER_LED_FAST_TWINKLE_300);

    //open audio codec clock & function and set initial volume
    PLL0_Set(0, ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
    PLL1_Set(0, ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k
    RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);

    PAD_CMD(_PA_0, DISABLE);
    PAD_CMD(_PA_1, DISABLE);
    PAD_CMD(_PA_2, DISABLE);
    PAD_CMD(_PA_3, DISABLE);
    PAD_CMD(_PA_4, DISABLE);
    PAD_CMD(_PA_5, DISABLE);
    PAD_CMD(_PA_6, DISABLE);

    PAD_CMD(_PB_28, DISABLE);
    PAD_CMD(_PB_29, DISABLE);
    PAD_CMD(_PB_30, DISABLE);
    PAD_CMD(_PB_31, DISABLE);
	
    //init I2C with codec and set initial volume
    CODEC_INIT();
    SET_VOLUME(0x82, 0x82);
		
    current_volume_index = duerapp_play_mp3_volume_get();
    DUER_LOGI("Volume Get, index %d, value %d\r\n", current_volume_index, volume_5680_value[current_volume_index]);

    DUER_LOGI("Peripheral initialized.\n");
}
