/**
 * Copyright (2017) Realsil Inc. All rights reserved.
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_periphral_play.h
 * Auth: Jingjun Wu (jingjun_wu@realsil.com.cn)
 * Desc: codec play files
 */

#ifndef DUERAPP_PERIPHERAL_AUDIO_H
#define DUERAPP_PERIPHERAL_AUDIO_H
#include "duerapp_audio.h"
/*start audio player*/
extern void audio_player_start();

/*stop audio player*/
extern void audio_player_stop(int reason);

/*start play pcm*/
extern void audio_play_pcm(char *buf, int len);

/*initialize audio as player*/
extern int initialize_audio_as_player(player_t *para);

extern void audio_record_thread_create(void);
extern void audio_record_start();
extern void audio_record_stop();
extern void init_voice_trigger_irq(void (*callback) (uint32_t id, gpio_irq_event event));
#endif // DUERAPP_PERIPHERAL_AUDIO_H