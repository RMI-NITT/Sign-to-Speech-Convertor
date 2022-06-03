// Main code for spresense
// Runs DL model, take serial input from UART2
// Clean exit and autoreset on interrupt trigger

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <nuttx/config.h>
#define TTYS_NAME "/dev/ttyS2"
#include <nuttx/config.h>
#include <tflmrt/runtime.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "loader_tflite.h"
//#include "pnm_util.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <asmp/mpshm.h>
#include <arch/chip/pm.h>
#include <arch/board/board.h>
#include <sys/stat.h>
#include "audio/audio_high_level_api.h"
#include "memutils/simple_fifo/CMN_SimpleFifo.h"
#include "memutils/memory_manager/MemHandle.h"
#include "memutils/message/Message.h"
#include "include/msgq_id.h"
#include "include/mem_layout.h"
#include "include/memory_layout.h"
#include "include/msgq_pool.h"
#include "include/pool_layout.h"
#include "include/fixed_fence.h"
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
#include <audio/utilities/playlist.h>
#endif
#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
#include "userproc_command.h"
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */

/* Section number of memory layout to use */

#define AUDIO_SECTION   SECTION_NO0

using namespace MemMgrLite;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*------------------------------
 * User definable definitions
 *------------------------------
 */

/* Path of playback file. */

#define PLAYBACK_FILE_PATH "/mnt/sd0/AUDIO"

/* Path of DSP image file. */

#define DSPBIN_FILE_PATH   "/mnt/sd0/BIN"

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
/* Path of playlist file. */

#  define PLAYLIST_FILE_PATH "/mnt/sd0/PLAYLIST"

/* PlayList file name. */

#  define PLAYLIST_FILE_NAME "TRACK_DB.CSV"
#endif

/* Default Volume. -20dB */

#define PLAYER_DEF_VOLUME 100

/* Play time(sec) */

#define PLAYER_PLAY_TIME 200

/* Play file number */

#define PLAYER_PLAY_FILE_NUM 5

/* Definition of FIFO info.
 * The FIFO defined here refers to the buffer to pass playback data
 * between application and AudioSubSystem.
 */
 
/* Recommended frame size differs for each codec.
 *    MP3       : 1440 bytes
 *    AAC       : 1024 bytes
 *    WAV 16bit : 2560 bytes
 *    WAV 24bit : 3840 bytes
 * If the codec to be played is fixed, use this size.
 * When playing multiple codecs, select the largest size.
 * There is no problem increasing the size. If it is made smaller,
 * FIFO under is possibly generated, so it is necessary to be careful.
 */

#define FIFO_FRAME_SIZE  3840

/* The number of elements in the FIFO queue used varies depending on
 * the performance of the system and must be sufficient.
 */

#define FIFO_ELEMENT_NUM  10

/* The number of elements pushed to the FIFO Queue at a time depends
 * on the load of the system. If the number of elements is small,
 * we will underflow if application dispatching is delayed.
 * Define a sufficient maximum value.
 */

#define PLAYER_FIFO_PUSH_NUM_MAX  5

/* Definition of content information to be used when not using playlist. */

#define PLAYBACK_FILE_NAME     "westworld.mp3"
#define PLAYBACK_CH_NUM        AS_CHANNEL_MONO
#define PLAYBACK_BIT_LEN       AS_BITLENGTH_16
#define PLAYBACK_SAMPLING_RATE AS_SAMPLINGRATE_24000  
#define PLAYBACK_CODEC_TYPE    AS_CODECTYPE_MP3

/*------------------------------
 * Definition specified by config
 *------------------------------
 */

/* Definition for selection of output device.
 * Select speaker output or I2S output.
 */

#ifdef CONFIG_EXAMPLES_MY_APP_OUTPUT_DEV_SPHP
#  define PLAYER_OUTPUT_DEV AS_SETPLAYER_OUTPUTDEVICE_SPHP
#  define PLAYER_MIXER_OUT  AS_OUT_SP
#else
#  define PLAYER_OUTPUT_DEV AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT
#  define PLAYER_MIXER_OUT  AS_OUT_I2S
#endif

/* Definition depending on player mode. */

#ifdef CONFIG_EXAMPLES_MY_APP_MODE_HIRES
#  define FIFO_FRAME_NUM  4
#else
#  define FIFO_FRAME_NUM  1
#endif

/*------------------------------
 * Definition of example fixed
 *------------------------------
 */

/* FIFO control value. */

#define FIFO_ELEMENT_SIZE  (FIFO_FRAME_SIZE * FIFO_FRAME_NUM)
#define FIFO_QUEUE_SIZE    (FIFO_ELEMENT_SIZE * FIFO_ELEMENT_NUM)

/* Local error code. */

#define FIFO_RESULT_OK  0
#define FIFO_RESULT_ERR 1
#define FIFO_RESULT_EOF 2
#define FIFO_RESULT_FUL 3

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* For FIFO */

struct player_fifo_info_s
{
  CMN_SimpleFifoHandle          handle;
  AsPlayerInputDeviceHdlrForRAM input_device;
  uint32_t fifo_area[FIFO_QUEUE_SIZE/sizeof(uint32_t)];
  uint8_t  read_buf[FIFO_ELEMENT_SIZE];
};

#ifndef CONFIG_AUDIOUTILS_PLAYLIST
struct Track
{
  char title[64];
  uint8_t   channel_number;  /* Channel number. */
  uint8_t   bit_length;      /* Bit length.     */
  uint32_t  sampling_rate;   /* Sampling rate.  */
  uint8_t   codec_type;      /* Codec type.     */
};
#endif

/* For play file */

struct player_file_info_s
{
  Track   track;
  int32_t size;
  DIR    *dirp;
  int     fd;
};

/* Player info */

struct player_info_s
{
  struct player_fifo_info_s   fifo;
  struct player_file_info_s   file;
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  Playlist *playlist_ins = NULL;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* For control player */

static struct player_info_s  s_player_info;

/* For share memory. */

static mpshm_t s_shm;

/* For frequency lock. */

static struct pm_cpu_freqlock_s s_player_lock;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void app_init_freq_lock(void)
{
  s_player_lock.count = 0;
  s_player_lock.info = PM_CPUFREQLOCK_TAG('A', 'P', 0);
  s_player_lock.flag = PM_CPUFREQLOCK_FLAG_HV;
}

static void app_freq_lock(void)
{
  up_pm_acquire_freqlock(&s_player_lock);
}

static void app_freq_release(void)
{
  up_pm_release_freqlock(&s_player_lock);
}

static bool app_open_contents_dir(void)
{
  DIR *dirp;
  const char *name = PLAYBACK_FILE_PATH;
  
  dirp = opendir(name);

  if (!dirp)
    {
      printf("Error: %s directory path error. check the path!\n", name);
      return false;
    }

  s_player_info.file.dirp = dirp;

  return true;
}

static bool app_close_contents_dir(void)
{
  closedir(s_player_info.file.dirp);

  return true;
}

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
static bool app_open_playlist(void)
{
  bool result = false;

  if (s_player_info.playlist_ins != NULL)
    {
      printf("Error: Open playlist failure. Playlist is already open\n");
      return false;
    }

  s_player_info.playlist_ins = new Playlist(PLAYLIST_FILE_NAME);
  
  result = s_player_info.playlist_ins->init(PLAYLIST_FILE_PATH);
  if (!result)
    {
      printf("Error: Playlist::init() failure.\n");
      return false;
    }

  s_player_info.playlist_ins->setPlayMode(Playlist::PlayModeNormal);
  if (!result)
    {
      printf("Error: Playlist::setPlayMode() failure.\n");
      return false;
    }

  s_player_info.playlist_ins->setRepeatMode(Playlist::RepeatModeOn);
  if (!result)
    {
      printf("Error: Playlist::setRepeatMode() failure.\n");
      return false;
    }

  s_player_info.playlist_ins->select(Playlist::ListTypeAllTrack, NULL);
  if (!result)
    {
      printf("Error: Playlist::select() failure.\n");
      return false;
    }

  return true;
}

static bool app_close_playlist(void)
{
  if (s_player_info.playlist_ins == NULL)
    {
      printf("Error: Close playlist failure. Playlist is not open\n");
      return false;
    }

  delete s_player_info.playlist_ins;
  s_player_info.playlist_ins = NULL;

  return true;
}
#endif /* #ifdef CONFIG_AUDIOUTILS_PLAYLIST */

static bool app_get_next_track(Track* track)
{
  bool ret = true;;
/*
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  if (s_player_info.playlist_ins == NULL)
    {
      printf("Error: Get next track failure. Playlist is not open\n");
      return false;
    }
  ret = s_player_info.playlist_ins->getNextTrack(track);
#else
  snprintf(track->title, sizeof(track->title), "%s", PLAYBACK_FILE_NAME);
  track->channel_number = PLAYBACK_CH_NUM;
  track->bit_length     = PLAYBACK_BIT_LEN;
  track->sampling_rate  = PLAYBACK_SAMPLING_RATE;
  track->codec_type     = PLAYBACK_CODEC_TYPE;
#endif /* #ifdef CONFIG_AUDIOUTILS_PLAYLIST */

  return ret;
}

static void app_input_device_callback(uint32_t size)
{
    /* do nothing */
}

static bool app_init_simple_fifo(void)
{
  if (CMN_SimpleFifoInitialize(&s_player_info.fifo.handle,
                               s_player_info.fifo.fifo_area,
                               FIFO_QUEUE_SIZE,
                               NULL) != 0)
    {
      printf("Error: Fail to initialize simple FIFO.");
      return false;
    }
  CMN_SimpleFifoClear(&s_player_info.fifo.handle);

  s_player_info.fifo.input_device.simple_fifo_handler = (void*)(&s_player_info.fifo.handle);
  s_player_info.fifo.input_device.callback_function = app_input_device_callback;

  return true;
}

static int app_push_simple_fifo(int fd)
{
  int ret;

  ret = read(fd, &s_player_info.fifo.read_buf, FIFO_ELEMENT_SIZE);
  if (ret < 0)
    {
      printf("Error: Fail to read file. errno:%d\n", get_errno());
      return FIFO_RESULT_ERR;
    }

  if (CMN_SimpleFifoOffer(&s_player_info.fifo.handle, (const void*)(s_player_info.fifo.read_buf), ret) == 0)
    {
      return FIFO_RESULT_FUL;
    }
  s_player_info.file.size = (s_player_info.file.size - ret);
  if (s_player_info.file.size == 0)
    {
      return FIFO_RESULT_EOF;
    }
  return FIFO_RESULT_OK;
}

static bool app_first_push_simple_fifo(int fd)
{
  int i;
  int ret = 0;

  for(i = 0; i < FIFO_ELEMENT_NUM - 1; i++)
    {
      if ((ret = app_push_simple_fifo(fd)) != FIFO_RESULT_OK)
        {
          break;
        }
    }

  return (ret != FIFO_RESULT_ERR) ? true : false;
}

static bool app_refill_simple_fifo(int fd)
{
  int32_t ret = FIFO_RESULT_OK;
  size_t  vacant_size;

  vacant_size = CMN_SimpleFifoGetVacantSize(&s_player_info.fifo.handle);

  if ((vacant_size != 0) && (vacant_size > FIFO_ELEMENT_SIZE))
    {
      int push_cnt = vacant_size / FIFO_ELEMENT_SIZE;

      push_cnt = (push_cnt >= PLAYER_FIFO_PUSH_NUM_MAX) ?
                  PLAYER_FIFO_PUSH_NUM_MAX : push_cnt;

      for (int i = 0; i < push_cnt; i++)
        {
          if ((ret = app_push_simple_fifo(fd)) != FIFO_RESULT_OK)
            {
              break;
            }
        }
    }

  return (ret == FIFO_RESULT_OK) ? true : false;
}

static bool printAudCmdResult(uint8_t command_code, AudioResult& result)
{
  if (AUDRLT_ERRORRESPONSE == result.header.result_code) {
    printf("Command code(0x%x): AUDRLT_ERRORRESPONSE:"
           "Module id(0x%x): Error code(0x%lx)\n",
            command_code,
            result.error_response_param.module_id,
            result.error_response_param.error_code);
    return false;
  }
  else if (AUDRLT_ERRORATTENTION == result.header.result_code) {
    printf("Command code(0x%x): AUDRLT_ERRORATTENTION\n", command_code);
    return false;
  }
  return true;
}

static void app_attention_callback(const ErrorAttentionParam *attparam)
{
  printf("Attention!! %s L%d ecode %d subcode %ld\n",
          attparam->error_filename,
          attparam->line_number,
          attparam->error_code,
          attparam->error_att_sub_code);
}

static bool app_create_audio_sub_system(void)
{
  bool result = false;

  /* Create manager of AudioSubSystem. */

  AudioSubSystemIDs ids;
  ids.app         = MSGQ_AUD_APP;
  ids.mng         = MSGQ_AUD_MNG;
  ids.player_main = MSGQ_AUD_PLY;
  ids.player_sub  = 0xFF;
  ids.mixer       = MSGQ_AUD_OUTPUT_MIX;
  ids.recorder    = 0xFF;
  ids.effector    = 0xFF;
  ids.recognizer  = 0xFF;

  AS_CreateAudioManager(ids, app_attention_callback);

  /* Create player feature. */

  AsCreatePlayerParams_t player_create_param;
  player_create_param.msgq_id.player   = MSGQ_AUD_PLY;
  player_create_param.msgq_id.mng      = MSGQ_AUD_MNG;
  player_create_param.msgq_id.mixer    = MSGQ_AUD_OUTPUT_MIX;
  player_create_param.msgq_id.dsp      = MSGQ_AUD_DSP;
  player_create_param.pool_id.es       = S0_DEC_ES_MAIN_BUF_POOL;
  player_create_param.pool_id.pcm      = S0_REND_PCM_BUF_POOL;
  player_create_param.pool_id.dsp      = S0_DEC_APU_CMD_POOL;
  player_create_param.pool_id.src_work = S0_SRC_WORK_BUF_POOL;

  /* When calling AS_CreatePlayerMulti(), use the pool area
   * for multi-core playback processing.
   * When calling AS_CreatePlayer(), use the heap area.
   */

  result = AS_CreatePlayerMulti(AS_PLAYER_ID_0, &player_create_param, NULL);

  if (!result)
    {
      printf("Error: AS_CratePlayer() failure. system memory insufficient!\n");
      return false;
    }

  /* Create mixer feature. */

  AsCreateOutputMixParams_t output_mix_act_param;
  output_mix_act_param.msgq_id.mixer = MSGQ_AUD_OUTPUT_MIX;
  output_mix_act_param.msgq_id.render_path0_filter_dsp = MSGQ_AUD_PFDSP0;
  output_mix_act_param.msgq_id.render_path1_filter_dsp = 0xFF;
  output_mix_act_param.pool_id.render_path0_filter_pcm = S0_PF0_PCM_BUF_POOL;
  output_mix_act_param.pool_id.render_path1_filter_pcm = S0_NULL_POOL;
  output_mix_act_param.pool_id.render_path0_filter_dsp = S0_PF0_APU_CMD_POOL;
  output_mix_act_param.pool_id.render_path1_filter_dsp = S0_NULL_POOL;

  result = AS_CreateOutputMixer(&output_mix_act_param, NULL);
  if (!result)
    {
      printf("Error: AS_CreateOutputMixer() failed. system memory insufficient!\n");
      return false;
    }

  /* Create renderer feature. */

  AsCreateRendererParam_t renderer_create_param;
  renderer_create_param.msgq_id.dev0_req  = MSGQ_AUD_RND_PLY;
  renderer_create_param.msgq_id.dev0_sync = MSGQ_AUD_RND_PLY_SYNC;
  renderer_create_param.msgq_id.dev1_req  = 0xFF;
  renderer_create_param.msgq_id.dev1_sync = 0xFF;

  result = AS_CreateRenderer(&renderer_create_param);
  if (!result)
    {
      printf("Error: AS_CreateRenderer() failure. system memory insufficient!\n");
      return false;
    }

  return true;
}

static void app_deact_audio_sub_system(void)
{
  AS_DeleteAudioManager();
  AS_DeletePlayer(AS_PLAYER_ID_0);
  AS_DeleteOutputMix();
  AS_DeleteRenderer();
}

static bool app_power_on(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_POWERON;
  command.header.command_code  = AUDCMD_POWERON;
  command.header.sub_code      = 0x00;
  command.power_on_param.enable_sound_effect = AS_DISABLE_SOUNDEFFECT;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_power_off(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_POWEROFF_STATUS;
  command.header.command_code  = AUDCMD_SETPOWEROFFSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_set_ready(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_READY_STATUS;
  command.header.command_code  = AUDCMD_SETREADYSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_get_status(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_GETSTATUS;
  command.header.command_code  = AUDCMD_GETSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return result.notify_status.status_info;
}

static bool app_init_output_select(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INITOUTPUTSELECT;
  command.header.command_code  = AUDCMD_INITOUTPUTSELECT;
  command.header.sub_code      = 0;
  command.init_output_select_param.output_device_sel = PLAYER_MIXER_OUT;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_set_volume(int master_db)
{
    AudioCommand command;
    command.header.packet_length = LENGTH_SETVOLUME;
    command.header.command_code  = AUDCMD_SETVOLUME;
    command.header.sub_code      = 0;
    command.set_volume_param.input1_db = 0;
    command.set_volume_param.input2_db = 0;
    command.set_volume_param.master_db = master_db;
    AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_set_player_status(void)
{
    AudioCommand command;
    command.header.packet_length = LENGTH_SET_PLAYER_STATUS;
#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
    command.header.command_code = AUDCMD_SETPLAYERSTATUSPOST;
#else /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
    command.header.command_code = AUDCMD_SETPLAYERSTATUS;
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
    command.header.sub_code = 0x00;
    command.set_player_sts_param.active_player         = AS_ACTPLAYER_MAIN;
    command.set_player_sts_param.player0.input_device  = AS_SETPLAYER_INPUTDEVICE_RAM;
    command.set_player_sts_param.player0.ram_handler   = &s_player_info.fifo.input_device;
    command.set_player_sts_param.player0.output_device = PLAYER_OUTPUT_DEV;
#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
    command.set_player_sts_param.post0_enable          = PostFilterEnable;
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
    command.set_player_sts_param.player1.input_device  = 0x00;
    command.set_player_sts_param.player1.ram_handler   = NULL;
    command.set_player_sts_param.player1.output_device = 0x00;
#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
    command.set_player_sts_param.post1_enable          = PostFilterDisable;
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
    AS_SendAudioCommand(&command);

    AudioResult result;
    AS_ReceiveAudioResult(&result);
    return printAudCmdResult(command.header.command_code, result);
}

static int app_init_player(uint8_t codec_type,
                           uint32_t sampling_rate,
                           uint8_t channel_number,
                           uint8_t bit_length)
{
    AudioCommand command;
    command.header.packet_length = LENGTH_INIT_PLAYER;
    command.header.command_code  = AUDCMD_INITPLAYER;
    command.header.sub_code      = 0x00;
    command.player.player_id                 = AS_PLAYER_ID_0;
    command.player.init_param.codec_type     = codec_type;
    command.player.init_param.bit_length     = bit_length;
    command.player.init_param.channel_number = channel_number;
    command.player.init_param.sampling_rate  = sampling_rate;
    snprintf(command.player.init_param.dsp_path,
             AS_AUDIO_DSP_PATH_LEN,
             "%s",
             DSPBIN_FILE_PATH);
    AS_SendAudioCommand(&command);

    AudioResult result;
    AS_ReceiveAudioResult(&result);
    return printAudCmdResult(command.header.command_code, result);
}

static int app_play_player(void)
{
    AudioCommand command;
    command.header.packet_length = LENGTH_PLAY_PLAYER;
    command.header.command_code  = AUDCMD_PLAYPLAYER;
    command.header.sub_code      = 0x00;
    command.player.player_id     = AS_PLAYER_ID_0;
    AS_SendAudioCommand(&command);

    AudioResult result;
    AS_ReceiveAudioResult(&result);
    return printAudCmdResult(command.header.command_code, result);
}

static bool app_stop_player(int mode)
{
    AudioCommand command;
    command.header.packet_length = LENGTH_STOP_PLAYER;
    command.header.command_code  = AUDCMD_STOPPLAYER;
    command.header.sub_code      = 0x00;
    command.player.player_id            = AS_PLAYER_ID_0;
    command.player.stop_param.stop_mode = mode;
    AS_SendAudioCommand(&command);

    AudioResult result;
    AS_ReceiveAudioResult(&result);
    return printAudCmdResult(command.header.command_code, result);
}

static bool app_set_clkmode(int clk_mode)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SETRENDERINGCLK;
  command.header.command_code  = AUDCMD_SETRENDERINGCLK;
  command.header.sub_code      = 0x00;
  command.set_renderingclk_param.clk_mode = clk_mode;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

static bool app_init_outputmixer(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INIT_OUTPUTMIXER;
  command.header.command_code  = AUDCMD_INIT_OUTPUTMIXER;
  command.header.sub_code      = 0x00;
  command.init_mixer_param.player_id     = AS_PLAYER_ID_0;
#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
  command.init_mixer_param.postproc_type = AsPostprocTypeUserCustom;
#else /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
  command.init_mixer_param.postproc_type = AsPostprocTypeThrough;
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */
  snprintf(command.init_mixer_param.dsp_path,
           sizeof(command.init_mixer_param.dsp_path),
           "%s/POSTPROC", DSPBIN_FILE_PATH);

  AS_SendAudioCommand(&command);
  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}

#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
static bool app_send_initpostproc_command(void)
{
  InitParam initpostcmd;

  AudioCommand command;
  command.header.packet_length = LENGTH_INITMPP;
  command.header.command_code  = AUDCMD_INITMPP;
  command.init_mpp_param.player_id        = AS_PLAYER_ID_0;
  command.init_mpp_param.initpp_param.addr = reinterpret_cast<uint8_t *>(&initpostcmd);
  command.init_mpp_param.initpp_param.size = sizeof(initpostcmd);
 
  /* Create Postfilter command */

  AS_SendAudioCommand(&command);
  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);

}

static bool app_send_setpostproc_command(void)
{
  static bool s_toggle = false;
  s_toggle = (s_toggle) ? false : true;

  /* Create packet area (have to ensure until API returns.)  */

  SetParam setpostcmd;
  setpostcmd.enable = s_toggle;
  setpostcmd.coef = 99;

  AudioCommand command;
  command.header.packet_length = LENGTH_SUB_SETMPP_COMMON;
  command.header.command_code  = AUDCMD_SETMPPPARAM;
  command.init_mpp_param.player_id        = AS_PLAYER_ID_0;
  command.init_mpp_param.initpp_param.addr = reinterpret_cast<uint8_t *>(&setpostcmd);
  command.init_mpp_param.initpp_param.size = sizeof(SetParam);
 
  /* Create Postfilter command */

  AS_SendAudioCommand(&command);
  AudioResult result;
  AS_ReceiveAudioResult(&result);
  return printAudCmdResult(command.header.command_code, result);
}
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */

static bool app_init_libraries(void)
{
  int ret;
  uint32_t addr = AUD_SRAM_ADDR;

  /* Initialize shared memory.*/

  ret = mpshm_init(&s_shm, 1, 1024 * 128 * 2);
  if (ret < 0)
    {
      printf("Error: mpshm_init() failure. %d\n", ret);
      return false;
    }

  ret = mpshm_remap(&s_shm, (void *)addr);
  if (ret < 0)
    {
      printf("Error: mpshm_remap() failure. %d\n", ret);
      return false;
    }

  /* Initalize MessageLib. */

  err_t err = MsgLib::initFirst(NUM_MSGQ_POOLS, MSGQ_TOP_DRM);
  if (err != ERR_OK)
    {
      printf("Error: MsgLib::initFirst() failure. 0x%x\n", err);
      return false;
    }

  err = MsgLib::initPerCpu();
  if (err != ERR_OK)
    {
      printf("Error: MsgLib::initPerCpu() failure. 0x%x\n", err);
      return false;
    }

  void* mml_data_area = translatePoolAddrToVa(MEMMGR_DATA_AREA_ADDR);
  err = Manager::initFirst(mml_data_area, MEMMGR_DATA_AREA_SIZE);
  if (err != ERR_OK)
    {
      printf("Error: Manager::initFirst() failure. 0x%x\n", err);
      return false;
    }

  err = Manager::initPerCpu(mml_data_area, static_pools, pool_num, layout_no);
  if (err != ERR_OK)
    {
      printf("Error: Manager::initPerCpu() failure. 0x%x\n", err);
      return false;
    }

  /* Create static memory pool. */

  const uint8_t sec_no = AUDIO_SECTION;
  const NumLayout layout_no = MEM_LAYOUT_PLAYER_MAIN_ONLY;
  void* work_va = translatePoolAddrToVa(S0_MEMMGR_WORK_AREA_ADDR);
  const PoolSectionAttr *ptr  = &MemoryPoolLayouts[AUDIO_SECTION][layout_no][0];
  err = Manager::createStaticPools(sec_no, layout_no,
                             work_va,
                             S0_MEMMGR_WORK_AREA_SIZE,
                             ptr);
  if (err != ERR_OK)
    {
      printf("Error: Manager::createStaticPools() failure. %d\n", err);
      return false;
    }

  return true;
}

static bool app_finalize_libraries(void)
{
  /* Finalize MessageLib. */

  MsgLib::finalize();

  /* Destroy static pools. */

  MemMgrLite::Manager::destroyStaticPools(AUDIO_SECTION);

  /* Finalize memory manager. */

  MemMgrLite::Manager::finalize();

  /* Destroy shared memory. */

  int ret;
  ret = mpshm_detach(&s_shm);
  if (ret < 0)
    {
      printf("Error: mpshm_detach() failure. %d\n", ret);
      return false;
    }

  ret = mpshm_destroy(&s_shm);
  if (ret < 0)
    {
      printf("Error: mpshm_destroy() failure. %d\n", ret);
      return false;
    }

  return true;
}

static int app_play_file_open(FAR const char *file_path, FAR int32_t *file_size)
{
  int fd = open(file_path, O_RDONLY);

  *file_size = 0;
  if (fd >= 0)
    {
      struct stat stat_buf;
      if (stat(file_path, &stat_buf) == OK)
        {
          *file_size = stat_buf.st_size;
        }
    }

  return fd;
}

static bool app_open_next_play_file(char *str)
{
  /* Get next track */

  if (!app_get_next_track(&s_player_info.file.track))
    {
      printf("Error: No more tracks to play.\n");
      return false;
    }
  //Track* track;
  snprintf(s_player_info.file.track.title, sizeof(s_player_info.file.track.title), "%s", str);
  s_player_info.file.track.channel_number = PLAYBACK_CH_NUM;
  s_player_info.file.track.bit_length     = PLAYBACK_BIT_LEN;
  
  s_player_info.file.track.sampling_rate  = PLAYBACK_SAMPLING_RATE;
  
  s_player_info.file.track.codec_type     = PLAYBACK_CODEC_TYPE;

  //s_player_info.file.track = track;

  char full_path[128];
  snprintf(full_path,
           sizeof(full_path),
           "%s/%s",
           PLAYBACK_FILE_PATH,
           s_player_info.file.track.title);

  s_player_info.file.fd = app_play_file_open(full_path, &s_player_info.file.size);
  if (s_player_info.file.fd < 0)
    {
      printf("Error: %s open error. check paths and files!\n", full_path);
      return false;
    }
  if (s_player_info.file.size == 0)
    {
      close(s_player_info.file.fd);
      printf("Error: %s file size is abnormal. check files!\n",full_path);
      return false;
    }

  /* Push data to simple fifo */

  if (!app_first_push_simple_fifo(s_player_info.file.fd))
    {
      printf("Error: app_first_push_simple_fifo() failure.\n");
      CMN_SimpleFifoClear(&s_player_info.fifo.handle);
      close(s_player_info.file.fd);
      return false;
    }

  return true;
}

static bool app_close_play_file(void)
{
  if (close(s_player_info.file.fd) != 0)
    {
      printf("Error: close() failure.\n");
      return false;
    }

  CMN_SimpleFifoClear(&s_player_info.fifo.handle);

  return true;
}

static bool app_start(void)
{
  /* Init Player */

  Track *t = &s_player_info.file.track;
  if (!app_init_player(t->codec_type,
                       t->sampling_rate,
                       t->channel_number,
                       t->bit_length))
    {
      printf("Error: app_init_player() failure.\n");
      app_close_play_file();
      return false;
    }

  /* Play Player */

  if (!app_play_player())
    {
      printf("Error: app_play_player() failure.\n");
      app_close_play_file();
      return false;
    }

  return true;
}

static bool app_stop(void)
{
  bool result = true;

  /* Set stop mode.
   * If the end of the file is detected, play back until the data empty
   * and then stop.(select AS_STOPPLAYER_ESEND)
   * Otherwise, stop immediate reproduction.(select AS_STOPPLAYER_NORMAL)
   */

  int  stop_mode = (s_player_info.file.size != 0) ?
                    AS_STOPPLAYER_NORMAL : AS_STOPPLAYER_ESEND;

  if (!app_stop_player(stop_mode))
    {
      printf("Error: app_stop_player() failure.\n");
      result = false;
    }

  if (!app_close_play_file())
    {
      printf("Error: app_close_play_file() failure.\n");
      result = false;
    }

  return result;
}

void app_play_process(uint32_t play_time)
{
  /* Timer Start */
  time_t start_time;
  time_t cur_time;

  time(&start_time);

  do
    {
      /* Check the FIFO every 2 ms and fill if there is space. */

      usleep(2 * 1000);
      if (!app_refill_simple_fifo(s_player_info.file.fd))
        {
          break;
        }

#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
      static int cnt = 0;
      if (cnt++ > 100)
        {
          app_send_setpostproc_command();
          cnt = 0;
        }
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */

    } while((time(&cur_time) - start_time) < play_time);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

 int audioEnd(void)
{
  printf("\nClosing audio");
  if (board_external_amp_mute_control(true) != OK)
    {
      printf("Error: board_external_amp_mute_control(true) failuer.\n");
      return 1;
    }

  /* Return the state of AudioSubSystem before voice_call operation. */

  if (AS_MNG_STATUS_READY != app_get_status())
    {
      if (!app_set_ready())
        {
          printf("Error: app_set_ready() failure.\n");
          return 1;
        }
    }

  /* Change AudioSubsystem to PowerOff state. */

  if (!app_power_off())
    {
      printf("Error: app_power_off() failure.\n");
      return 1;
    }

  /* Unlock cpu frequency. */

  app_freq_release();

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  /* Close playlist. */

  if (!app_close_playlist())
    {
      printf("Error: app_close_playlist() failure.\n");
      return 1;
    }
#endif

  /* Close directory of play contents. */

errout_power_on:
  if (!app_close_contents_dir())
    {
      printf("Error: app_close_contents_dir() failure.\n");
      return 1;
    }

  /* Deactivate the features used by AudioSubSystem. */

errout_open_contents_dir:
  app_deact_audio_sub_system();

  /* finalize the shared memory and memory utility used by AudioSubSystem. */

errout_act_audio_sub_system:
  if (!app_finalize_libraries())
    {
      printf("Error: finalize_libraries() failure.\n");
      return 1;
    }

  printf("Exit\n");

  return 0;
}

 int audioSetup(void)
 {
   /* First, initialize the shared memory and memory utility used by AudioSubSystem. */

   printf("\n Setting up audio");

  if (!app_init_libraries())
    {
      printf("Error: init_libraries() failure.\n");
      return 1;
    }

  /* Next, Create the features used by AudioSubSystem. */

  if (!app_create_audio_sub_system())
    {
      printf("Error: act_audiosubsystem() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }

  /* Open directory of play contents. */

  if (!app_open_contents_dir())
    {
      printf("Error: app_open_contents_dir() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }

  /* Initialize frequency lock parameter. */

  app_init_freq_lock();

  /* Lock cpu frequency to high. */

  app_freq_lock();

  /* Change AudioSubsystem to Ready state so that I/O parameters can be changed. */

  if (!app_power_on())
    {
      printf("Error: app_power_on() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  /* Open playlist. */

  if (!app_open_playlist())
    {
      printf("Error: app_open_playlist() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }
#endif

  /* Initialize simple fifo. */

  if (!app_init_simple_fifo())
    {
      printf("Error: app_init_simple_fifo() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }

  /* Set the device to output the mixed audio. */

  if (!app_init_output_select())
    {
      printf("Error: app_init_output_select() failure.\n");

      /* Abnormal termination processing */

      return audioEnd();
    }

  printf("\nSetup done");
  return 0;
 }


int call_audio(char *str)
{
  /* Initialize clock mode.
   * Clock mode indicates whether the internal processing rate of
   * AudioSubSystem is Normal mode or Hi-Res mode. 
   * The sampling rate of the playback file determines which mode
   * will be taken. When playing a Hi-Res file,
   * please set player mode to Hi-Res mode with config.
   */
  

  //char *str = "westworld.mp3";

  

  printf("\nStarting AudioPlayer\n");


  int clk_mode = -1;

  for (int i = 0; i < PLAYER_PLAY_FILE_NUM; i++)
    {
      if (!app_open_next_play_file(str))
        {
          /* Abnormal termination processing */
          printf("/nFile not found");
          return 0;
        }

      /* Get current clock mode.
       * If the sampling rate is less than 48 kHz,
       * it will be in Normal mode. Otherwise, Hi-Res mode is set.
       */

      int cur_clk_mode;
      if (s_player_info.file.track.sampling_rate <= AS_SAMPLINGRATE_48000)
        {
          cur_clk_mode = AS_CLKMODE_NORMAL;
        }
      else
        {
          cur_clk_mode = AS_CLKMODE_HIRES;
        }

      /* If clockmode is Hi-Res and player mode is not Hi-Res,
       * play the next file.
       */

#ifdef CONFIG_EXAMPLES_MY_APP_MODE_NORMAL
      if (cur_clk_mode == AS_CLKMODE_HIRES)
        {
          printf("Hi-Res file is not supported.\n"
                 "Please change player mode to Hi-Res with config.\n");
          app_close_play_file();

          /* Play next file. */

          continue;
        }
#endif

      /* If current clock mode is different from the previous clock mode,
       * perform initial setting.
       */

      if (clk_mode != cur_clk_mode)
        {
          /* Update clock mode. */

          clk_mode = cur_clk_mode;

          /* Since the initial setting is required to be in the Ready state,
           * if it is not in the Ready state, it is set to the Ready state.
           */

          if (AS_MNG_STATUS_READY != app_get_status())
            {
              if (board_external_amp_mute_control(true) != OK)
                {
                  printf("Error: board_external_amp_mute_control(true) failuer.\n");

                  /* Abnormal termination processing */

                  return audioEnd();
                }

              if (!app_set_ready())
                {
                  printf("Error: app_set_ready() failure.\n");

                  /* Abnormal termination processing */

                  return audioEnd();
                }
            }

          /* Set the clock mode of the output function. */

          if (!app_set_clkmode(clk_mode))
            {
              printf("Error: app_set_clkmode() failure.\n");

              /* Abnormal termination processing */

              return audioEnd();
            }

          /* Set player operation mode. */

          if (!app_set_player_status())
            {
              printf("Error: app_set_player_status() failure.\n");

              /* Abnormal termination processing */

              return audioEnd();
            }

          /* Init OutputMixer. */

          if (!app_init_outputmixer())
            {
              printf("Error: app_init_outputmixer() failure.\n");

              return audioEnd();
            }

#ifdef CONFIG_EXAMPLES_MY_APP_USEPOSTPROC
          /* Init Postproc. */

          if (!app_send_initpostproc_command())
            {
              printf("Error: app_send_initpostproc_command() failure.\n");

              return audioEnd();
            }
#endif /* CONFIG_EXAMPLES_MY_APP_USEPOSTPROC */

           /* Cancel output mute. */

           app_set_volume(PLAYER_DEF_VOLUME);

          if (board_external_amp_mute_control(false) != OK)
            {
              printf("Error: board_external_amp_mute_control(false) failuer.\n");

              /* Abnormal termination processing */

              return audioEnd();
            }
        }

      /* Start player operation. */

      if (!app_start())
        {
          printf("Error: app_start_player() failure.\n");

          /* Abnormal termination processing */

          return audioEnd();
        }

      /* Running... */

      printf("Running time is %d sec\n", PLAYER_PLAY_TIME);

      app_play_process(PLAYER_PLAY_TIME);

      /* Stop player operation. */

      if (!app_stop())
        {
          printf("Error: app_stop() failure.\n");
          return 1;
        }

#ifndef CONFIG_AUDIOUTILS_PLAYLIST
      break;
#endif
    }

    printf("\n Call audio successful");
    return 0;
}

  /* Set output mute. */

//TF code

typedef struct
{
  char *model_path;
  char *pgm_path;
  bool skip_norm;
} my_setting_t;

struct gloveData
 {
   int leftGlove[7];
   int rightGlove[7];
   float lg[7];
   float rg[7];
 } glove = { {0.0,0.0,0.0,0.0,0.0,0.0,0.0} , {0.0,0.0,0.0,0.0,0.0,0.0,0.0} , {0.0,0.0,0.0,0.0,0.0,0.0,0.0} , {0.0,0.0,0.0,0.0,0.0,0.0,0.0}};

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TFLM_TENSOR_ARENA_SIZE  (40*1024)
#define MNIST_SIZE_PX           (14)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static float s_img_buffer[14];
extern unsigned char model_tflite[];
extern int model_tflite_len;
int image[] = {100,100,100,655,871,123,771,250};
extern int image0_len;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
 void getString(void)
 {
   //for(int m = 0 ; m < 7)
   
  bool getGlove1 = false , getGlove2 = false;
  do
  {
    int fd;                           //file locator
  char str[4];                      //read stream
  char bigstr[50] = ""; //store string
  int str_count = 0; 
  int retval;               //count of bigstr
  printf("Loopp\n");

  fd = open(TTYS_NAME, O_RDWR );
  
  while ( ( retval = read ( fd, &str, sizeof(str))) > 0) 
  {//read until end of file
  
  if(str[0] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[0];

  if(str[1] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[1];
  

  if(str[2] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[2];

  if(str[3] == '\n')
  {
    break;
  }
  bigstr[str_count++] = str[3];
  
 
  strcpy(str,"");  
  }

  bigstr[str_count++] = '\0';

  int i = 0;
  int j = 0;
  int k = 0;
  
  printf("LoopEnd\n");
  close(fd);
  char ab[10] = "";
  int arr[9];
  
  while(i<strlen(bigstr) & j < 8)
  {
      if(bigstr[i] == '/')
      {
          arr[j++] = atoi(ab);
          memset(ab, '\0', sizeof(ab));
          k=0;
      }
      else
      ab[k++] = bigstr[i];
       i++;
      
     
  }

  if(!arr[0])
  {printf("glove left\n");
  getGlove1 = true;
  for(i=1;i<8;i++)
  {
    //printf("%d\n" , arr[i]);
    glove.leftGlove[i-1] = arr[i];

  }}
  
  else
  {printf("glove right\n");
  getGlove2 = true;
  for(i=1;i<8;i++)
  {
    //printf("%d\n" , arr[i]);
    glove.rightGlove[i-1] = arr[i];
  }
  }
  } while (!(getGlove1 & getGlove2));
  
  
  
  
  

 }

static int convert_datatype(tflm_runtime_t *rt)
{
  int ret = 0;

  /* get datatype which this tflm_runtime_t expects */

  TfLiteTensor *var = tflm_runtime_input_variable(rt, 0);

  if (var->type == kTfLiteFloat32)
    {
      /* do nothing since the image data is stored as float */
      printf("FLoat\n");
    }
  else if (var->type == kTfLiteInt16)
    {
      /* convert the image data in-place to 16-bit fixed-point values */
      printf("int16\n");

      int16_t *int16_buffer = (int16_t *) s_img_buffer;
      uint16_t px;
      for (px = 0u; px < MNIST_SIZE_PX; px++)
        {
          int16_buffer[px] = (int16_t) (s_img_buffer[px] /
                             var->params.scale +
                             var->params.zero_point);
        }
    }
  else if (var->type == kTfLiteInt8)
    {
      /* convert the image data in-place to 8-bit fixed-point values */
      printf("int8\n");

      int8_t *int8_buffer = (int8_t *) s_img_buffer;
      uint16_t px;
      for (px = 0u; px < MNIST_SIZE_PX; px++)
        {
          int8_buffer[px] = (int8_t) (s_img_buffer[px] /
                            var->params.scale +
                            var->params.zero_point);
        }
    }
  else
    {
      ret = -1;
    }

  return ret;
}

static float get_output(tflm_runtime_t *rt, unsigned char index)
{
  float output;
  TfLiteTensor *var = tflm_runtime_output_variable(rt, 0);

  switch (var->type)
  {
    case kTfLiteFloat32:
      output = var->data.f[index];
      break;
    case kTfLiteInt16:
      output = (var->data.i16[index] - var->params.zero_point) *
               var->params.scale;
      break;
    case kTfLiteInt8:
      output = (var->data.int8[index] - var->params.zero_point) *
               var->params.scale;
      break;
    default:
      output = 0.0f;
      printf("Unsupported data type: %d\n", var->type);
      break;
  }

  return output;
}

static void parse_args(int argc, char *argv[], my_setting_t *setting)
{
  /* parse options by getopt() */

  int opt;
  while ((opt = getopt(argc, argv, "s")) != -1)
    {
      switch (opt)
        {
        case 's':              /* skip normalization */
          setting->skip_norm = true;
          break;
        }
    }

  /* set my_setting_t::{model_path,pgm_path} to argv[] if necessary */

  setting->model_path = (optind < argc) &&
                        (strcmp(argv[optind], "default") != 0) ?
                        argv[optind] : NULL;
  optind++;

  setting->pgm_path = (optind < argc) ? argv[optind] : NULL;

  /* print my_setting_t */

  if (setting->model_path)
    {
      printf("Load model file: %s\n", setting->model_path);
    }
  else
    {
      printf("Load built-in modellll\n");
    }

  if (setting->pgm_path)
    {
      printf("Load pgm image: %s\n", setting->pgm_path);
    }
  else
    {
      printf("Load built-in test image\n");
    }

  
}

 static int gpio_handler(int irq, FAR void *context, FAR void *arg)
{
  printf("\nINTERRUPT\n");
  board_gpio_write(PIN_SPI2_MISO, 1);
  return 1;
}

//tfcode
extern "C" int main(int argc, FAR char *argv[])
{
  /*
  char s[20];
  
  printf("Enter file name\n");
  scanf("%s",s);
  printf("\nEntered : %s",s);*/

  //tfstart


  int ret = 0;
  int i;
  float proc_time;
  float norm_factor;
  const void *inputs[1] = { s_img_buffer };
  tflm_runtime_t rt;
  tflm_config_t config = { .cpu_num = 1 };
  void *network;
  my_setting_t setting = { 0 };
  struct timeval begin;
  struct timeval end;
  int maxPred = 0;
  int lastPred = -2;
  int currentPred = -1;

  //char predictions[][15] = {"A.mp3" , "B.mp3" , "C.mp3" , "D.mp3" , "E.mp3" , "F.mp3" , "G.mp3" , "H.mp3" ,"Hello.mp3", "I.mp3" , "J.mp3" , "K.mp3" , "L.mp3" , "M.mp3" , "My name is.mp3" , "N.mp3" , "Namaste.mp3" , "O.mp3" , "P.mp3" , "Q.mp3" , "R.mp3" , "S.mp3" ,"Sorry.mp3", "T.mp3", "Team.mp3" ,"Thank You.mp3", "U.mp3" ,"V.mp3","W.mp3","We.mp3" , "Welcome.mp3" , "World.mp3" , "X.mp3" , "Y.mp3" , "Z.mp3" };
  char predictions[][15] = {"Hello.mp3" , "Namaste.mp3" , "Sorry.mp3", "Thank You.mp3" ,  "We.mp3" , "Welcome.mp3"};

  // //interrupt
  board_gpio_config(PIN_PWM2,0,true,false, PIN_PULLUP);
  board_gpio_intconfig(PIN_PWM2, INT_FALLING_EDGE, true, gpio_handler);
  board_gpio_int(PIN_PWM2, true);

  board_gpio_write(PIN_SPI2_MISO, -1);
  board_gpio_config(PIN_SPI2_MISO, 0, 0, true, PIN_PULLDOWN);
  board_gpio_write(PIN_SPI2_MISO, 0);



  parse_args(argc, argv, &setting);

  
  
    network = (void *) model_tflite;
  

  /* initialize the whole tflmrt subsystem */

  ret = tflm_initialize(&config);
  if (ret)
    {
      printf("tflm_initialize() failed due to %d", ret);
      goto tflm_error;
    }

  /* instantiate a neural network in a tflm_runtime_t object */

  ret = tflm_runtime_initialize(&rt, network, TFLM_TENSOR_ARENA_SIZE);
  if (ret)
    {
      printf("tflm_runtime_initialize() failed due to %d\n", ret);
      goto rt_error;
    }

  /* convert the image data to datatype this tflm_runtime_t expects */
int hash[6];


while (true)
{

  
  memset(hash , 0 , 6*sizeof(int));
  for(int a = 1 ; a <= 15 ; a++)
  {
     getString();
      glove.lg[0] = (float)glove.leftGlove[0]/360.0f;
      glove.lg[1] = (float)glove.leftGlove[1]/360.0f;
      glove.rg[0] = (float)glove.rightGlove[0]/360.0f;
      glove.rg[1] = (float)glove.rightGlove[1]/360.0f;
      for(i=2 ; i <= 6 ; i++)
      glove.lg[i] = (float)glove.leftGlove[i]/1023.0f;
      for(i=2 ; i <= 6 ; i++)
      glove.rg[i] = (float)glove.rightGlove[i]/1023.0f;
      //glove.leftGlove[2] -= 180;
      for (i = 0; i < 7; i++)
        {
          s_img_buffer[i] = glove.lg[i];
          //printf("\n%f",s_img_buffer[i]);
        }
        for (i = 0; i < 7; i++)
        {
          s_img_buffer[i+7] = glove.rg[i];
          //printf("\n%f",s_img_buffer[i+7]);
        }
        //printf("%f\n" , s_img_buffer[13]);
        //printf("%f\n" , s_img_buffer[13]);
        //for(i=0 ; i < 14 ; i++)
        //printf("%f " , s_img_buffer[i]);
        //printf("\n");
    

/*
  ret = convert_datatype(&rt);
  if (ret)
    {
      printf("convert_datatype() failed due to %d\n", ret);
      goto rt_error;
    }*/

  /* perform inference after feeding inputs */

  printf("start tflm_runtime_forward()\n");
  gettimeofday(&begin, 0);
  ret = tflm_runtime_forward(&rt, inputs, 1);
  gettimeofday(&end, 0);
  if (ret)
    {
      printf("tflm_runtime_forward() failed due to %d\n", ret);
      goto fin;
    }

  /* show the classification result and its processing time */
  maxPred = 0;
  for (i = 0; i < tflm_runtime_output_shape(&rt, 0, 1); i++)
    {
     // printf("output[%u]=%.6f\n", i, get_output(&rt, i));
      if(get_output(&rt,i) > get_output(&rt , maxPred))
      maxPred = i;
    }
    printf("max pred : %d iter: %d\n",maxPred , a);
    hash[maxPred] += 1;
    //sleep(0.1);
  }

  int min = 0;
  for(int l = 0 ; l < 6 ; l++)
  {
    if(hash[min] < hash[l])
    min = l;
  }
  printf("after 50 runs , most predicted : %s : %d" , predictions[min] , min);
  currentPred = min;
  if(lastPred == currentPred)
    continue;
   else
   {
  audioSetup();
    call_audio(predictions[min]);
    sleep(0.2);
    audioEnd();
    lastPred  = currentPred;
   }
  
}

  
  

   // getting audio utilities ready

   // function to play required audio file
  //sleep(2);
  //call_audio("westworld.mp3");

   // function to close audio utilities
  printf("\nBye\n");

fin:

  /* free memories allocated to tflm_runtime_t */

  tflm_runtime_finalize(&rt);
rt_error:

  /* finalize the whole tflmrt subsystem */

  tflm_finalize();
tflm_error:

  /* just call free() */

  
pgm_error:
  return ret;

}
