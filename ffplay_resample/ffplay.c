/*
 * FFplay : Simple Media Player based on the ffmpeg libraries
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <psppower.h>
#include <pspthreadman.h>
#include <pspiofilemgr.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <pspctrl.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include "PspLib/pg.h"
#include "myscreen.h"
#include "mycolor.h"
#include "mydrawbmp.h"
#include "myscrolbar.h"
#include "mytext.h"
#include "mystring.h"
#include "HanLib/han_lib.h"
#include "myjpeg.h"



#include "mem64.h"                      // [jonny]
#include "me.h"                         // [jonny]
#include "me_csc.h"                     // [jonny]
volatile struct me_struct *p_me_struct; // [jonny]



#define HAVE_AV_CONFIG_H
#include "avformat.h"											 

//#include "cmdutils.h"

//#include <SDL.h>
//#include <SDL_thread.h>

#ifdef CONFIG_WIN32
#undef main /* We don't want SDL to override our main() */
#endif

#if defined(__linux__)
#define HAVE_X11
#endif

#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

//#define DEBUG_SYNC

#define MAX_VIDEOQ_SIZE (5 * 128 * 1024)
//#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_AUDIOQ_SIZE (3 * 16 * 1024)
#define MAX_SUBTITLEQ_SIZE (5 * 16 * 1024)

/* SDL audio buffer size, in samples. Should be small to have precise
   A/V sync as SDL does not have hardware buffer fullness info. */
//#define SDL_AUDIO_BUFFER_SIZE 1024
#define SDL_AUDIO_BUFFER_SIZE 1024

/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
#define SAMPLE_ARRAY_SIZE (2*65536)

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int abort_request;
//    SDL_mutex *mutex;
//    SDL_cond *cond;
	SceUID mutex;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define SUBPICTURE_QUEUE_SIZE 4

typedef struct VideoPicture {
    double pts; /* presentation time stamp for this picture */
//    SDL_Overlay *bmp;
	AVFrame *bmp;
	uint8_t *buffer;
    int width, height; /* source height & width */
    int allocated;
} VideoPicture;

typedef struct SubPicture {
    double pts; /* presentation time stamp for this picture */
    AVSubtitle sub;
} SubPicture;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

struct AVResampleContext;

struct ReSampleContext {
    struct AVResampleContext *resample_context;
    short *temp[2];
    int temp_len;
    float ratio;
    /* channel convert */
    int input_channels, output_channels, filter_channels;
};

typedef struct VideoState {
//    SDL_Thread *parse_tid;
	SceUID parse_tid;
//    SDL_Thread *video_tid;
	SceUID video_tid;
    AVInputFormat *iformat;
    int no_background;
    int abort_request;
    int paused;
    int last_paused;
    int seek_req;
	int seek_flags;
    int64_t seek_pos;
    AVFormatContext *ic;
    int dtg_active_format;

    int audio_stream;
    
    int av_sync_type;
    double external_clock; /* external clock base */
    int64_t external_clock_time;
    
    double audio_clock;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
//    /* samples output by the codec. we reserve more space for avsync
//       compensation */
//    uint8_t __attribute__((aligned(16))) audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]; 
////    uint8_t __attribute__((aligned(16))) audio_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4]; 
    unsigned int audio_buf_size; /* in bytes */
    int audio_buf_index; /* in bytes */
    AVPacket audio_pkt;
    uint8_t *audio_pkt_data;
    int audio_pkt_size;
    
    int show_audio; /* if true, display audio samples */
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    
    double frame_timer;
    double frame_last_pts;
    double frame_last_delay;
    double video_clock;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double video_current_pts; /* current displayed pts (different from
                                 video_clock if frame fifos are used) */
    int64_t video_current_pts_time; /* time at which we updated
                                       video_current_pts - used to
                                       have running video pts */
    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
    int pictq_size, pictq_rindex, pictq_windex;
//    SDL_mutex *pictq_mutex;
//    SDL_cond *pictq_cond;
	SceUID pictq_mutex;
    
	SceUID video_decoder_mutex;
    SceUID audio_decoder_mutex;
	SceUID audio_buffer_mutex;
    //    QETimer *video_timer;
    char filename[1024];
    int width, height, xleft, ytop;
	int audio_resample;
    ReSampleContext *resample; /* for audio resampling */
    /* samples output by the codec. we reserve more space for avsync
       compensation */
    uint8_t __attribute__((aligned(16))) audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]; 
} VideoState;

typedef enum
{
	MEDIA_VIDEO,
	MEDIA_MUSIC
} media_type;

my_bmp_type *background;

void show_help(void);
static int audio_write_get_buf_size(VideoState *is);
void sdl_audio_callback(short* buffer, unsigned long length);
int get_prev_file(media_type type);
int get_next_file(media_type type);
static void stream_pause(VideoState *is);
static void stream_seek(VideoState *is, int64_t pos, int rel);
void do_exit(void);
static VideoState *stream_open(const char *filename, AVInputFormat *iformat);
static double get_master_clock(VideoState *is);
void show_info_box(const char *msg);

/* options specified by the user */
static AVInputFormat *file_iformat;
static AVImageFormat *image_format;
static const char *input_filename;
static char FileName[MAXPATHLEN];
static int fs_screen_width;
static int fs_screen_height;
static int screen_width = 480;
static int screen_height = 272;
static int audio_disable;
static int video_disable;
static int display_disable;
static int show_status;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
//static int av_sync_type = AV_SYNC_VIDEO_MASTER;
static int64_t start_time = AV_NOPTS_VALUE;
static int debug = 0;
static int debug_mv = 0;
static int step = 0;
static int thread_count = 1;
static int workaround_bugs = 1;

static int fast = 0;
static int genpts = 0;
static int lowres = 0;
static int idct = FF_IDCT_AUTO;
static enum AVDiscard skip_frame= AVDISCARD_DEFAULT;
static enum AVDiscard skip_idct= AVDISCARD_DEFAULT;
static enum AVDiscard skip_loop_filter= AVDISCARD_DEFAULT;
static int error_resilience = FF_ER_CAREFUL;
static int error_concealment = 3;

/* current context */
static int is_full_screen;
static VideoState *cur_stream;
static int64_t audio_callback_time;

int done = 0;

static int need_alloc = 0;
static double next_refresh_time;
static int need_timer_check;
static int disp_w, disp_h;
static int play_finished = 0;
static char saved_filename[MAXPATHLEN];
static double saved_pos;
static int return_from_sleep = 0;
static int catched_error = 0;
static int skip_frames = 0;
static double next_timer_check_time;
static int need_general_timer_check;
static int display_clock = 0;
static char clock_text[256] = {"clock : 266"};

void (* do_timer_action)(void) = NULL;

#define MAX_MEDIA_FILES 100

static char media_dir_name[MAXPATHLEN];

static SceIoDirent media_file_list[MAX_MEDIA_FILES];
static int media_file_num;
static int start_idx;
static int last_start_idx;
static int media_idx;
static int last_media_idx;
static int list_end;
static unsigned char has_scrolbar = 0;
static double progress = 0.0f;
static double last_progress = 0.0f;
u8 full_screen = 0;
int language = LANGUAGE_ENGLISH;

typedef enum
{
	NORMAL_MODE,
	FIT_MODE,
	FULL_SCREEN
} screen_mode;
screen_mode mode = NORMAL_MODE;
void set_disp_mode(VideoState *is, screen_mode mode);

//#define FF_ALLOC_EVENT   (SDL_USEREVENT)
//#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
//#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

//SDL_Surface *screen;
//#define printf	pspDebugScreenPrintf
PSP_MODULE_INFO("PSP Media Player", 0x1000, 1, 1);
//PSP_MODULE_INFO("PSP Media Player", 0, 1, 1);
/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(0);
/*
__attribute__((constructor)) void stdoutInit() 
{ 
   pspKernelSetKernelPC(); 
   pspDebugInstallStderrHandler(pspDebugScreenPrintData); 
}
*/

void MyExceptionHandler(PspDebugRegBlock * regs)
{
    /* Do normal initial dump, setup screen etc */
    //pspDebugScreenInit();

    /* I always felt BSODs were more interesting that white on black */
    pspDebugScreenSetBackColor(0x00FF0000);
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    //pspDebugScreenClear();

    pspDebugScreenSetXY(0,17);
    pspDebugScreenPrintf("\nI regret to inform you your psp has just crashed\n");
    pspDebugScreenPrintf("Exception Details:\n");
    pspDebugDumpException(regs);
    pspDebugScreenSetBackColor(0x00000000);
//    pspDebugScreenSetXY(0,31);
//    pspDebugScreenPrintf("\nI regret to inform you your psp has just crashed\n");
//    pspDebugScreenSetBackColor(0x00000000);
}

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	done = 1;
//	sceKernelExitGame();

	return 0;
}

void PowerCallback (int unknown, int pwrflags)
{
	if (pwrflags & PSP_POWER_CB_HOLD_SWITCH)
	{
	}
	
	if (pwrflags & PSP_POWER_CB_POWER_SWITCH)
	{
        if (cur_stream) {
            saved_pos = get_master_clock(cur_stream);
			strcpy(saved_filename, cur_stream->filename);
        }
		else
		{
			saved_pos = -1;
		}
	}
	else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) 
	{
		if (saved_pos >= 0)
		{
//			toggle_pause();
//			do_exit();
//			pstrcpy(FileName, sizeof(FileName), saved_filename);
//			input_filename = FileName;
//			start_time = (int64_t)(saved_pos * AV_TIME_BASE);
//			cur_stream = stream_open(input_filename, file_iformat);
			return_from_sleep = 1;
		}
	}

	if (pwrflags & PSP_POWER_CB_BATTERY_LOW){
	}

	int cbid;
	cbid = sceKernelCreateCallback ("Power Callback", (SceKernelCallbackFunction)PowerCallback, NULL);
	scePowerRegisterCallback       (0, cbid);
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback ("Power Callback", (SceKernelCallbackFunction)PowerCallback, NULL);
	scePowerRegisterCallback       (0, cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, THREAD_ATTR_USER, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

void print_error(const char *filename, int err)
{
	char msg[2048];

    switch(err) {
    case AVERROR_NUMEXPECTED:
//        fprintf(stderr, "%s: Incorrect image filename syntax.\n"
//                "Use '%%d' to specify the image number:\n"
//                "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
//                "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n", 
//                filename);
        snprintf(msg, 2048, "%s: Incorrect image filename syntax.\n"
                "Use '%%d' to specify the image number:\n"
                "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n", 
                filename);
		msg_box((const char *)msg);
        break;
    case AVERROR_INVALIDDATA:
//        fprintf(stderr, "%s: Error while parsing header\n", filename);
        snprintf(msg, 2048, "%s: Error while parsing header\n", filename);
		msg_box((const char *)msg);
        break;
    case AVERROR_NOFMT:
//        fprintf(stderr, "%s: Unknown format\n", filename);
        snprintf(msg, 2048, "%s: Unknown format\n", filename);
		msg_box((const char *)msg);
        break;
    default:
//        fprintf(stderr, "%s: Error while opening file\n", filename);
        snprintf(msg, 2048, "%s: Error while opening file\n", filename);
		msg_box((const char *)msg);
        break;
    }
}

/* packet queue handling */
static void packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
//    q->mutex = SDL_CreateMutex();
//    q->cond = SDL_CreateCond();
	q->mutex = sceKernelCreateSema("", 0, 1, 1, 0);
}

static void packet_queue_flush(PacketQueue *q)
{
    AVPacketList *pkt, *pkt1;

	sceKernelWaitSema(q->mutex, 1, 0);
    for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
		av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
	sceKernelSignalSema(q->mutex, 1);
}

static void packet_queue_end(PacketQueue *q)
{
    packet_queue_flush(q);
//    SDL_DestroyMutex(q->mutex);
//    SDL_DestroyCond(q->cond);
	sceKernelDeleteSema(q->mutex);
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;

	sceKernelWaitSema(q->mutex, 1, 0);
    /* duplicate the packet */
    if (av_dup_packet(pkt) < 0)
	{
		sceKernelSignalSema(q->mutex, 1);
        return -1;
	}
    
    pkt1 = av_malloc(sizeof(AVPacketList));
    if (!pkt1)
	{
		sceKernelSignalSema(q->mutex, 1);
        return -1;
	}
    pkt1->pkt = *pkt;
    pkt1->next = NULL;


//    SDL_LockMutex(q->mutex);
//	sceKernelWaitSema(q->mutex, 1, 0);

    if (!q->last_pkt)

        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    /* XXX: should duplicate packet data in DV case */
//    SDL_CondSignal(q->cond);

//    SDL_UnlockMutex(q->mutex);
	sceKernelSignalSema(q->mutex, 1);
    return 0;
}

static void packet_queue_abort(PacketQueue *q)
{
//    SDL_LockMutex(q->mutex);
	sceKernelWaitSema(q->mutex, 1, 0);

    q->abort_request = 1;
    
//    SDL_CondSignal(q->cond);

//    SDL_UnlockMutex(q->mutex);
	sceKernelSignalSema(q->mutex, 1);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int ret;

//    SDL_LockMutex(q->mutex);

    for(;;) {
		sceKernelWaitSema(q->mutex, 1, 0);
        if (q->abort_request) {
            ret = -1;
			sceKernelSignalSema(q->mutex, 1);
            break;
        }
            
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
			sceKernelSignalSema(q->mutex, 1);
            break;
        } else if (!block) {
            ret = 0;
			sceKernelSignalSema(q->mutex, 1);
            break;
        } else {
//            SDL_CondWait(q->cond, q->mutex);
			sceKernelSignalSema(q->mutex, 1);
//			sceDisplayWaitVblankStart();
			sceKernelDelayThread(1000);
        }
    }
//    SDL_UnlockMutex(q->mutex);
    return ret;
}
/*
static inline void fill_rectangle(SDL_Surface *screen, 
                                  int x, int y, int w, int h, int color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_FillRect(screen, &rect, color);
}
*/
static inline void fill_rectangle(int x, int y, int w, int h, int color)
{
	my_rect area;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	area.x = x;
	area.y = y;
	area.w = w;
	area.h = h;
	my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, color);
}

static inline void fill_rectangle_alpha(int x, int y, int w, int h, int color, unsigned char alpha)
{
	my_rect area;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	area.x = x;
	area.y = y;
	area.w = w;
	area.h = h;
	color |= (alpha << 24);
	my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, color);
}

#if 0
void MY_BlitSurface(unsigned int *dest, int dst_pitch, MY_Surface *surface, my_rect area)
{
	int w, h, i, pitch;
	int use_colorkey, colorkey;
	MY_Palette *palette;
//	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);
	unsigned int *dst;

	w = (surface->w <= area.w) ? surface->w : area.w;
	h = (surface->h <= area.h) ? surface->h : area.h;
	
	if (surface->bpp == 8)
	{
		unsigned char *src;
		unsigned char r,g,b;

		palette = surface->palette;
		dst = &dest[area.x + area.y * dst_pitch];
		src = (unsigned char *)surface->pixels;
		pitch = surface->pitch;
		use_colorkey = surface->use_colorkey;
		colorkey = surface->colorkey;
		while (h)
		{
			for (i = 0; i < w; i++)
			{
				if (!use_colorkey || colorkey != src[i])
				{
					r = palette->colors[src[i]].r;
					g = palette->colors[src[i]].g;
					b = palette->colors[src[i]].b;

					dst[i] = RGB(r,g,b);
				}
			}
			dst += dst_pitch;
			src += pitch;
			h--;
		}
	}
	else
	{
		unsigned int *src;
		
		dst = &dest[area.x + area.y * dst_pitch];
		src = (unsigned int *)surface->pixels;
		pitch = surface->pitch / 4;

		while (h)
		{			
			for (i = 0; i < w; i++)
			{
//				dst[i] = src[i];
				dst[i] = AlphaBlend(dst[i], src[i], (src[i] >> 24));
			}
			dst += dst_pitch;
			src += pitch;
			h--;
		}
	}
}
#endif

#if 0
// 8bit only
void MY_BlitSurfaceAlpha(unsigned int *dest, int dst_pitch, MY_Surface *surface, my_rect area, unsigned char alpha)
{
	int w, h, i, pitch;
	int use_colorkey, colorkey;
	MY_Palette *palette;
//	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);
	unsigned int *dst;

	w = (surface->w <= area.w) ? surface->w : area.w;
	h = (surface->h <= area.h) ? surface->h : area.h;
	
	if (surface->bpp == 8)
	{
		unsigned char *src;
		unsigned char r,g,b;

		palette = surface->palette;
		dst = &dest[area.x + area.y * dst_pitch];
		src = (unsigned char *)surface->pixels;
		pitch = surface->pitch;
		use_colorkey = surface->use_colorkey;
		colorkey = surface->colorkey;
		while (h)
		{
			for (i = 0; i < w; i++)
			{
				if (!use_colorkey || colorkey != src[i])
				{
					r = palette->colors[src[i]].r;
					g = palette->colors[src[i]].g;
					b = palette->colors[src[i]].b;

					dst[i] = RGBA(r,g,b,alpha);
				}
			}
			dst += dst_pitch;
			src += pitch;
			h--;
		}
	}
}
#endif

#if 0
/* draw only the border of a rectangle */
void fill_border(VideoState *s, int x, int y, int w, int h, int color)
{
    int w1, w2, h1, h2;

    /* fill the background */
    w1 = x;
    if (w1 < 0)
        w1 = 0;
    w2 = s->width - (x + w);
    if (w2 < 0)
        w2 = 0;
    h1 = y;
    if (h1 < 0)
        h1 = 0;
    h2 = s->height - (y + h);
    if (h2 < 0)
        h2 = 0;
    fill_rectangle(screen, 
                   s->xleft, s->ytop, 
                   w1, s->height, 
                   color);
    fill_rectangle(screen, 
                   s->xleft + s->width - w2, s->ytop, 
                   w2, s->height, 
                   color);
    fill_rectangle(screen, 
                   s->xleft + w1, s->ytop, 
                   s->width - w1 - w2, h1, 
                   color);
    fill_rectangle(screen, 
                   s->xleft + w1, s->ytop + s->height - h2,
                   s->width - w1 - w2, h2,
                   color);
}
#endif

#ifdef SUBTITLE


#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define RGB_TO_Y_CCIR(r, g, b) \
((FIX(0.29900*219.0/255.0) * (r) + FIX(0.58700*219.0/255.0) * (g) + \
  FIX(0.11400*219.0/255.0) * (b) + (ONE_HALF + (16 << SCALEBITS))) >> SCALEBITS)

#define RGB_TO_U_CCIR(r1, g1, b1, shift)\
(((- FIX(0.16874*224.0/255.0) * r1 - FIX(0.33126*224.0/255.0) * g1 +         \
     FIX(0.50000*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define RGB_TO_V_CCIR(r1, g1, b1, shift)\
(((FIX(0.50000*224.0/255.0) * r1 - FIX(0.41869*224.0/255.0) * g1 -           \
   FIX(0.08131*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define ALPHA_BLEND(a, oldp, newp, s)\
((((oldp << s) * (255 - (a))) + (newp * (a))) / (255 << s))

#define RGBA_IN(r, g, b, a, s)\
{\
    unsigned int v = ((const uint32_t *)(s))[0];\
    a = (v >> 24) & 0xff;\
    r = (v >> 16) & 0xff;\
    g = (v >> 8) & 0xff;\
    b = v & 0xff;\
}

#define YUVA_IN(y, u, v, a, s, pal)\
{\
    unsigned int val = ((const uint32_t *)(pal))[*(const uint8_t*)s];\
    a = (val >> 24) & 0xff;\
    y = (val >> 16) & 0xff;\
    u = (val >> 8) & 0xff;\
    v = val & 0xff;\
}

#define YUVA_OUT(d, y, u, v, a)\
{\
    ((uint32_t *)(d))[0] = (a << 24) | (y << 16) | (u << 8) | v;\
}


#define BPP 1

static void blend_subrect(AVPicture *dst, const AVSubtitleRect *rect)
{
    int wrap, wrap3, width2, skip2;
    int y, u, v, a, u1, v1, a1, w, h;
    uint8_t *lum, *cb, *cr;
    const uint8_t *p;
    const uint32_t *pal;

    lum = dst->data[0] + rect->y * dst->linesize[0];
    cb = dst->data[1] + (rect->y >> 1) * dst->linesize[1];
    cr = dst->data[2] + (rect->y >> 1) * dst->linesize[2];

    width2 = (rect->w + 1) >> 1;
    skip2 = rect->x >> 1;
    wrap = dst->linesize[0];
    wrap3 = rect->linesize;
    p = rect->bitmap;
    pal = rect->rgba_palette;  /* Now in YCrCb! */
    
    if (rect->y & 1) {
        lum += rect->x;
        cb += skip2;
        cr += skip2;
    
        if (rect->x & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
            cb++;
            cr++;
            lum++;
            p += BPP;
        }
        for(w = rect->w - (rect->x & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += 2 * BPP;
            lum += 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
        }
        p += wrap3 + (wrap3 - rect->w * BPP);
        lum += wrap + (wrap - rect->w - rect->x);
        cb += dst->linesize[1] - width2 - skip2;
        cr += dst->linesize[2] - width2 - skip2;
    }
    for(h = rect->h - (rect->y & 1); h >= 2; h -= 2) {
        lum += rect->x;
        cb += skip2;
        cr += skip2;
    
        if (rect->x & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            p += wrap3;
            lum += wrap;
            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += -wrap3 + BPP;
            lum += -wrap + 1;
        }
        for(w = rect->w - (rect->x & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            p += wrap3;
            lum += wrap;

            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);

            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 2);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 2);

            cb++;
            cr++;
            p += -wrap3 + 2 * BPP;
            lum += -wrap + 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            p += wrap3;
            lum += wrap;
            YUVA_IN(y, u, v, a, p, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
            cb++;
            cr++;
            p += -wrap3 + BPP;
            lum += -wrap + 1;
        }
        p += wrap3 + (wrap3 - rect->w * BPP);
        lum += wrap + (wrap - rect->w - rect->x);
        cb += dst->linesize[1] - width2 - skip2;
        cr += dst->linesize[2] - width2 - skip2;
    }
    /* handle odd height */
    if (h) {
        lum += rect->x;
        cb += skip2;
        cr += skip2;
    
        if (rect->x & 1) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
            cb++;
            cr++;
            lum++;
            p += BPP;
        }
        for(w = rect->w - (rect->x & 1); w >= 2; w -= 2) {
            YUVA_IN(y, u, v, a, p, pal);
            u1 = u;
            v1 = v;
            a1 = a;
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

            YUVA_IN(y, u, v, a, p + BPP, pal);
            u1 += u;
            v1 += v;
            a1 += a;
            lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
            cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u, 1);
            cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v, 1);
            cb++;
            cr++;
            p += 2 * BPP;
            lum += 2;
        }
        if (w) {
            YUVA_IN(y, u, v, a, p, pal);
            lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
            cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
            cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
        }
    }
}

static void free_subpicture(SubPicture *sp)
{
    int i;
    
    for (i = 0; i < sp->sub.num_rects; i++)
    {
        av_free(sp->sub.rects[i].bitmap);
        av_free(sp->sub.rects[i].rgba_palette);
    }
    
    av_free(sp->sub.rects);
    
    memset(&sp->sub, 0, sizeof(AVSubtitle));
}
#endif

static void video_image_display(VideoState *is)
{
    VideoPicture *vp;
    float aspect_ratio;
    int width, height, x, y;
//    SDL_Rect rect;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

    vp = &is->pictq[is->pictq_rindex];
    if (vp->bmp) {
#if 0
        /* XXX: use variable in the frame */
        if (is->video_st->codec.sample_aspect_ratio.num == 0) 
            aspect_ratio = 0;
        else
            aspect_ratio = av_q2d(is->video_st->codec.sample_aspect_ratio) 
                * is->video_st->codec.width / is->video_st->codec.height;;
        if (aspect_ratio <= 0.0)
            aspect_ratio = (float)is->video_st->codec.width / 
                (float)is->video_st->codec.height;
        /* if an active format is indicated, then it overrides the
           mpeg format */
#if 0
        if (is->video_st->codec.dtg_active_format != is->dtg_active_format) {
            is->dtg_active_format = is->video_st->codec.dtg_active_format;
            printf("dtg_active_format=%d\n", is->dtg_active_format);
        }
#endif
#if 0
        switch(is->video_st->codec.dtg_active_format) {
        case FF_DTG_AFD_SAME:
        default:
            /* nothing to do */
            break;
        case FF_DTG_AFD_4_3:
            aspect_ratio = 4.0 / 3.0;
            break;
        case FF_DTG_AFD_16_9:
            aspect_ratio = 16.0 / 9.0;
            break;
        case FF_DTG_AFD_14_9:
            aspect_ratio = 14.0 / 9.0;
            break;
        case FF_DTG_AFD_4_3_SP_14_9:
            aspect_ratio = 14.0 / 9.0;
            break;
        case FF_DTG_AFD_16_9_SP_14_9:
            aspect_ratio = 14.0 / 9.0;
            break;
        case FF_DTG_AFD_SP_4_3:
            aspect_ratio = 4.0 / 3.0;
            break;
        }
#endif

        /* XXX: we suppose the screen has a 1.0 pixel ratio */
        height = is->height;
        width = ((int)rint(height * aspect_ratio)) & -3;
        if (width > is->width) {
            width = is->width;
            height = ((int)rint(width / aspect_ratio)) & -3;
        }
        x = (is->width - width) / 2;
        y = (is->height - height) / 2;
        if (!is->no_background) {
            /* fill the background */
            //            fill_border(is, x, y, width, height, QERGB(0x00, 0x00, 0x00));
        } else {
            is->no_background = 0;
        }
        rect.x = is->xleft + x;
        rect.y = is->xleft + y;
        rect.w = width;
        rect.h = height;
//        SDL_DisplayYUVOverlay(vp->bmp, &rect);
#endif
		if (display_clock)
		{
			my_rect area;
			area.x = 0;
			area.y = 0;
//			area.w = is->video_st->codec.width;
//			area.h = is->video_st->codec.height;
			area.w = is->video_st->codec->width;
			area.h = is->video_st->codec->height;

			my_draw_text((u32*)vp->bmp->data[0], 512, 0, 0, clock_text, 11, MY_COLOR_RED, area);
		}

//		disp_w = 160;
//		disp_h = 120;
		guDrawBuffer((u32*)vp->bmp->data[0], is->video_st->codec->width, is->video_st->codec->height, 512, 0, 
					disp_w, disp_h);
//		guDrawBuffer((u32*)vp->bmp->data[0], is->video_st->codec->width, is->video_st->codec->height, 
//					 is->video_st->codec->width, 0, disp_w, disp_h);
//		disp_copy(vp->bmp->data[0], is->video_st->codec->width, 0, 0, 
//				  is->video_st->codec->width, is->video_st->codec->height, 0, 0);
		disp_update();
		scePowerTick(0);
    } else {
#if 0
        fill_rectangle(screen, 
                       is->xleft, is->ytop, is->width, is->height, 
                       QERGB(0x00, 0x00, 0x00));
#endif
    }
}

static inline int compute_mod(int a, int b)
{
    a = a % b;
    if (a >= 0) 
        return a;
    else
        return a + b;
}
#if 0
static void video_audio_display(VideoState *s)
{
	char *txt;
	int len;
	int y;
	my_rect area;
	static first = 1;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	if (first)
	{
		first = 0;
		fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MY_COLOR_BLACK);

		area.x = 0;
		area.y = 0;
		area.w = SCREEN_WIDTH;
		area.h = SCREEN_HEIGHT;

		y = 20;

		txt = s->ic->title;
		len = strlen((const char *)txt);
		if (len)
		{
			my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, MY_COLOR_LIGHT_GRAY, area);
			y += 20;
		}
		txt = s->ic->author;
		len = strlen((const char *)txt);
		if (len)
		{
			my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, MY_COLOR_LIGHT_GRAY, area);
			y += 20;
		}
		disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
		disp_update();
	}
}
#endif
static void video_audio_display(VideoState *s)
{
    int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
    int ch, channels, h, h2, bgcolor, fgcolor;
    int16_t time_diff;

	char *txt;
	int len;
	my_rect area;

	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

    /* compute display index : center on currently output samples */
//    channels = s->audio_st->codec.channels;
    channels = s->audio_st->codec->channels;
    nb_display_channels = channels;
    if (!s->paused) {
        n = 2 * channels;
        delay = audio_write_get_buf_size(s);
        delay /= n;
        
        /* to be more precise, we take into account the time spent since
           the last buffer computation */
        if (audio_callback_time) {
            time_diff = av_gettime() - audio_callback_time;
//            delay += (time_diff * s->audio_st->codec.sample_rate) / 1000000;
            delay += (time_diff * s->audio_st->codec->sample_rate) / 1000000;
        }
        
        delay -= s->width / 2;
        if (delay < s->width)
            delay = s->width;
        i_start = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
        s->last_i_start = i_start;
    } else {
        i_start = s->last_i_start;
    }

//    bgcolor = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	bgcolor = MY_COLOR_BLACK;
//    fill_rectangle(screen, 
//                   s->xleft, s->ytop, s->width, s->height, 
//                   bgcolor);
//
    fill_rectangle(s->xleft, s->ytop, s->width, s->height, bgcolor);
//    fgcolor = SDL_MapRGB(screen->format, 0xff, 0xff, 0xff);
	fgcolor = MY_COLOR_WHITE;

    /* total height for one channel */
    h = s->height / nb_display_channels;
    /* graph height / 2 */
    h2 = (h * 9) / 20;
    for(ch = 0;ch < nb_display_channels; ch++) {
        i = i_start + ch;
        y1 = s->ytop + ch * h + (h / 2); /* position of center line */
        for(x = 0; x < s->width; x++) {
            y = (s->sample_array[i] * h2) >> 15;
            if (y < 0) {
                y = -y;
                ys = y1 - y;
            } else {
                ys = y1;
            }
//            fill_rectangle(screen, 
//                           s->xleft + x, ys, 1, y, 
//                           fgcolor);
            fill_rectangle(s->xleft + x, ys, 1, y, fgcolor);
            i += channels;
            if (i >= SAMPLE_ARRAY_SIZE)
                i -= SAMPLE_ARRAY_SIZE;
        }
    }
/*
//    fgcolor = SDL_MapRGB(screen->format, 0x00, 0x00, 0xff);
	fgcolor = MY_COLOR_BLUE;

    for(ch = 1;ch < nb_display_channels; ch++) {
        y = s->ytop + ch * h;
//        fill_rectangle(screen, 
//                       s->xleft, y, s->width, 1, 
//                       fgcolor);
        fill_rectangle(s->xleft, y, s->width, 1, fgcolor);
    }
*/
	area.x = 0;
	area.y = 0;
	area.w = SCREEN_WIDTH;
	area.h = SCREEN_HEIGHT;

	y = SCREEN_HEIGHT / 2 - 20;

	txt = s->ic->title;
	len = strlen((const char *)txt);
	if (len)
	{
		my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, MY_COLOR_RED, area);
		y += 20;
	}
	else
	{
		txt = media_file_list[media_idx].d_name;
		len = strlen((const char *)txt);
		my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, MY_COLOR_RED, area);
		y += 20;
	}
	txt = s->ic->author;
	len = strlen((const char *)txt);
	if (len)
	{
		my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, MY_COLOR_ORANGE, area);
		y += 20;
	}
//    SDL_UpdateRect(screen, s->xleft, s->ytop, s->width, s->height);
	disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
	disp_update();
}

/* display the current picture, if any */
static void video_display(VideoState *is)
{
    if (is->audio_st && is->show_audio) 
        video_audio_display(is);
    else if (is->video_st)
        video_image_display(is);
}

//static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque)
//{
//    SDL_Event event;
//    event.type = FF_REFRESH_EVENT;
//    event.user.data1 = opaque;
//    SDL_PushEvent(&event);
//    return 0; /* 0 means stop timer */
//}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(VideoState *is, int delay)
{
//    SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
	next_refresh_time = delay * 1000 + av_gettime();
	need_timer_check = 1;
}

/* get the current audio clock value */
static double get_audio_clock(VideoState *is)
{
    double pts;
    int hw_buf_size, bytes_per_sec;
    pts = is->audio_clock;
    hw_buf_size = audio_write_get_buf_size(is);
    bytes_per_sec = 0;
    if (is->audio_st) {
//        bytes_per_sec = is->audio_st->codec.sample_rate * 
//            2 * is->audio_st->codec.channels;
        bytes_per_sec = is->audio_st->codec->sample_rate * 
            2 * is->audio_st->codec->channels;
    }
    if (bytes_per_sec)
        pts -= (double)hw_buf_size / bytes_per_sec;
    return pts;
}

/* get the current video clock value */
static double get_video_clock(VideoState *is)
{
    double delta;
    if (is->paused) {
        delta = 0;
    } else {
        delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
    }
    return is->video_current_pts + delta;
}

/* get the current external clock value */
static double get_external_clock(VideoState *is)
{
    int64_t ti;
    ti = av_gettime();
    return is->external_clock + ((ti - is->external_clock_time) * 1e-6);
}

/* get the current master clock value */
static double get_master_clock(VideoState *is)
{
    double val;

    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            val = get_video_clock(is);
        else
            val = get_audio_clock(is);
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            val = get_audio_clock(is);
        else
            val = get_video_clock(is);
    } else {
        val = get_external_clock(is);
    }
    return val;
}

/* seek in the stream */
static void stream_seek(VideoState *is, int64_t pos, int rel)
{
   if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
        is->seek_req = 1;
    }
}

/* pause or resume the video */
static void stream_pause(VideoState *is)
{
    is->paused = !is->paused;
    if (is->paused) {
//		pspAudioSetChannelCallback(0, NULL);
        is->video_current_pts = get_video_clock(is);
    }
//	else
//	{
//		pspAudioSetChannelCallback(0, sdl_audio_callback);
//	}
}

/* called to display each frame */
static void video_refresh_timer(void *opaque)
{
    VideoState *is = opaque;
    VideoPicture *vp;
    double actual_delay, delay, sync_threshold, ref_clock, diff;


    if (is->video_st) {
        if (is->pictq_size == 0) {
            /* if no picture, need to wait */
            schedule_refresh(is, 1);
        } else {
            video_display(is);
            /* dequeue the picture */
            vp = &is->pictq[is->pictq_rindex];

            /* update current video pts */
            is->video_current_pts = vp->pts;
            is->video_current_pts_time = av_gettime();

            /* compute nominal delay */
            delay = vp->pts - is->frame_last_pts;
            if (delay <= 0 || delay >= 1.0) {
                /* if incorrect delay, use previous one */
                delay = is->frame_last_delay;
            }
            is->frame_last_delay = delay;
            is->frame_last_pts = vp->pts;

            /* update delay to follow master synchronisation source */
            if (((is->av_sync_type == AV_SYNC_AUDIO_MASTER && is->audio_st) ||
                 is->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)) {
                /* if video is slave, we try to correct big delays by
                   duplicating or deleting a frame */
                ref_clock = get_master_clock(is);
                diff = vp->pts - ref_clock;
                
                /* skip or repeat frame. We take into account the
                   delay to compute the threshold. I still don't know
                   if it is the best guess */
                sync_threshold = AV_SYNC_THRESHOLD;
                if (delay > sync_threshold)
                    sync_threshold = delay;
                if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
                    if (diff <= -sync_threshold)
                        delay = 0;
                    else if (diff >= sync_threshold)
                        delay = 2 * delay;
                }
            }

            is->frame_timer += delay;
            /* compute the REAL delay (we need to do that to avoid
               long term errors */
            actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
            if (actual_delay < 0.010) {
                /* XXX: should skip picture */
//				if (!skip_frames)
//				{
//					skip_frames = (int)(fabs(actual_delay) * ((double)is->video_st->codec.frame_rate / (double)is->video_st->codec.frame_rate_base));
//					if (skip_frames > 2)
//						skip_frames = 2;
//				}
				skip_frames = 1;
                actual_delay = 0.010;
            }
//			else
//			{
//				skip_frames = 0;
//			}
            /* launch timer for next picture */
            schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));

#if defined(DEBUG_SYNC)
            printf("video: delay=%0.3f actual_delay=%0.3f pts=%0.3f A-V=%f\n", 
                   delay, actual_delay, vp->pts, -diff);
#endif

            /* display picture */
//            video_display(is);
            
            /* update queue size and signal for next picture */
            if (++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE)
                is->pictq_rindex = 0;
            
//            SDL_LockMutex(is->pictq_mutex);
			sceKernelWaitSema(is->pictq_mutex, 1, 0);
            is->pictq_size--;
//            SDL_CondSignal(is->pictq_cond);
//            SDL_UnlockMutex(is->pictq_mutex);
			sceKernelSignalSema(is->pictq_mutex, 1);
        }
    } else if (is->audio_st) {
        /* draw the next audio frame */

        schedule_refresh(is, 40);

        /* if only audio stream, then display the audio bars (better
           than nothing, just to test the implementation */
        
        /* display picture */
        video_display(is);
    } else {
        schedule_refresh(is, 100);
    }
    if (show_status) {
        static int64_t last_time;
        int64_t cur_time;
        int aqsize, vqsize;
        double av_diff;
        
        cur_time = av_gettime();
        if (!last_time || (cur_time - last_time) >= 500 * 1000) {
            aqsize = 0;
            vqsize = 0;
            if (is->audio_st)
                aqsize = is->audioq.size;
            if (is->video_st)
                vqsize = is->videoq.size;
            av_diff = 0;
            if (is->audio_st && is->video_st)
                av_diff = get_audio_clock(is) - get_video_clock(is);
            printf("%7.2f A-V:%7.3f aq=%5dKB vq=%5dKB    \n", 
                   get_master_clock(is), av_diff, aqsize / 1024, vqsize / 1024);
            fflush(stdout);
            last_time = cur_time;
        }
    }
}

/* allocate a picture (needs to do that in main thread to avoid
   potential locking problems */
static void alloc_picture(void *opaque)
{
    VideoState *is = opaque;
    VideoPicture *vp;

    vp = &is->pictq[is->pictq_windex];

    if (vp->bmp)
	{
		if (vp->buffer)
		{
			av_free(vp->buffer);
		}
		av_free(vp->bmp);
	}

#if 0
    /* XXX: use generic function */
    /* XXX: disable overlay if no hardware acceleration or if RGB format */
//    switch(is->video_st->codec.pix_fmt) {
    switch(is->video_st->codec->pix_fmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_YUV422P:
    case PIX_FMT_YUV444P:
    case PIX_FMT_YUV422:
    case PIX_FMT_YUV410P:
    case PIX_FMT_YUV411P:
        is_yuv = 1;
        break;
    default:
        is_yuv = 0;
        break;
    }
#endif
    // Allocate an AVFrame structure
    vp->bmp = avcodec_alloc_frame();
    if(vp->bmp)
	{
		int numBytes;
	    numBytes=avpicture_get_size(PIX_FMT_RGBA32, 512, is->video_st->codec->height);
//	    numBytes=avpicture_get_size(PIX_FMT_RGBA32, is->video_st->codec->width, is->video_st->codec->height);



//	    vp->buffer=(uint8_t*)av_malloc(numBytes); // [jonny]
		vp->buffer = malloc_64(4 * 512 * 272);    // [jonny]



		avpicture_fill((AVPicture *)vp->bmp, vp->buffer, PIX_FMT_RGBA32, 512, is->video_st->codec->height);
//		avpicture_fill((AVPicture *)vp->bmp, vp->buffer, PIX_FMT_RGBA32, is->video_st->codec->width, is->video_st->codec->height);
	}
    	
    vp->width = is->video_st->codec->width;
    vp->height = is->video_st->codec->height;

	sceKernelWaitSema(is->pictq_mutex, 1, 0);
    vp->allocated = 1;
	sceKernelSignalSema(is->pictq_mutex, 1);
}

static int queue_picture(VideoState *is, AVFrame *src_frame, double pts)
{
    VideoPicture *vp;
    int dst_pix_fmt;
    AVPicture pict;
    
    /* wait until we have space to put a new picture */
	while (1)
	{
		sceKernelWaitSema(is->pictq_mutex, 1, 0);
		if (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !is->videoq.abort_request)
		{
			sceKernelSignalSema(is->pictq_mutex, 1);
			sceKernelDelayThread(1000);
		}
		else
		{
			sceKernelSignalSema(is->pictq_mutex, 1);
			break;
		}
	}
    
    if (is->videoq.abort_request)
        return -1;

    vp = &is->pictq[is->pictq_windex];

    /* alloc or resize hardware picture buffer */
    if (!vp->bmp || 
        vp->width != is->video_st->codec->width ||
        vp->height != is->video_st->codec->height) {

        vp->allocated = 0;

        /* the allocation must be done in the main thread to avoid
           locking problems */
		need_alloc = 1;
        
        /* wait until the picture is allocated */
		while (1)
		{
			sceKernelWaitSema(is->pictq_mutex, 1, 0);
			if (!vp->allocated && !is->videoq.abort_request)
			{
				sceKernelSignalSema(is->pictq_mutex, 1);
				sceKernelDelayThread(1000);
			}
			else
			{
				sceKernelSignalSema(is->pictq_mutex, 1);
				break;
			}
		}

        if (is->videoq.abort_request)
            return -1;
    }

    /* if the frame is not skipped, then display it */
    if (vp->bmp) {
        /* get a pointer on the bitmap */



        me_csc                           // [jonny]
			(                            // [jonny]
			p_me_struct,                 // [jonny]
			src_frame->data[0],          // [jonny]
			src_frame->data[1],          // [jonny]
			src_frame->data[2],          // [jonny]
			src_frame->linesize[0],      // [jonny]
			src_frame->linesize[1],      // [jonny]
			src_frame->linesize[2],      // [jonny]
			vp->buffer,                  // [jonny]
			is->video_st->codec->width,  // [jonny]
			is->video_st->codec->height, // [jonny]
			512                          // [jonny]
			);                           // [jonny]

//		img_convert((AVPicture *)vp->bmp, PIX_FMT_RGBA32, (AVPicture*)src_frame,                            // [jonny]
//					is->video_st->codec->pix_fmt, is->video_st->codec->width, is->video_st->codec->height); // [jonny]



        vp->pts = pts;

        /* now we can update the picture count */
        if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
            is->pictq_windex = 0;
		sceKernelWaitSema(is->pictq_mutex, 1, 0);
        is->pictq_size++;
		sceKernelSignalSema(is->pictq_mutex, 1);
    }
    return 0;
}

/* compute the exact PTS for the picture if it is omitted in the stream */
static int output_picture2(VideoState *is, AVFrame *src_frame, double pts1)
{
    double frame_delay, pts;
    
    pts = pts1;

    if (pts != 0) {
        /* update video clock with pts, if present */
        is->video_clock = pts;
    } else {
        pts = is->video_clock;
    }
    /* update video clock for next frame */
	frame_delay = av_q2d(is->video_st->codec->time_base);

    /* for MPEG2, the frame can be repeated, so we update the
       clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;

#if defined(DEBUG_SYNC) && 0
    {
        int ftype;
        if (src_frame->pict_type == FF_B_TYPE)
            ftype = 'B';
        else if (src_frame->pict_type == FF_I_TYPE)
            ftype = 'I';
        else
            ftype = 'P';
        printf("frame_type=%c clock=%0.3f pts=%0.3f\n", 
               ftype, pts, pts1);
    }
#endif
    return queue_picture(is, src_frame, pts);
}

static VideoState* video_arg;

static int video_thread(void *arg)
{
//    VideoState *is = arg;
	VideoState *is = video_arg;
    AVPacket pkt1, *pkt = &pkt1;
    int len1, got_picture;
    AVFrame *frame= avcodec_alloc_frame();
    double pts;

    for(;;) {
        while (is->paused && !is->videoq.abort_request) {
//            SDL_Delay(10);
			sceKernelDelayThread(10000);
        }
        if (packet_queue_get(&is->videoq, pkt, 1) < 0)
            break;
        /* NOTE: ipts is the PTS of the _first_ picture beginning in
           this packet, if any */
        pts = 0;
        if (pkt->dts != AV_NOPTS_VALUE)
            pts = av_q2d(is->video_st->time_base)*pkt->dts;

//            SDL_LockMutex(is->video_decoder_mutex);
			sceKernelWaitSema(is->video_decoder_mutex, 1, 0);
            len1 = avcodec_decode_video(is->video_st->codec, 
                                        frame, &got_picture, 
                                        pkt->data, pkt->size);
//            SDL_UnlockMutex(is->video_decoder_mutex);
			sceKernelSignalSema(is->video_decoder_mutex, 1);
//            if (len1 < 0)
//                break;
            if (got_picture) {
				if (!skip_frames)
				{
                	if (output_picture2(is, frame, pts) < 0)
                    	goto the_end;
				}
				else
				{
					skip_frames--;
				}
            }
        av_free_packet(pkt);
        if (step) 
            if (cur_stream)
                stream_pause(cur_stream);
    }
the_end:
    av_free(frame);
    return 0;
}

#ifdef SUBTITLE
static int subtitle_thread(void *arg)
{
    VideoState *is = arg;
    SubPicture *sp;
    AVPacket pkt1, *pkt = &pkt1;
    int len1, got_subtitle;
    double pts;
    int i, j;
    int r, g, b, y, u, v, a;

    for(;;) {
        while (is->paused && !is->subtitleq.abort_request) {
            SDL_Delay(10);
        }
        if (packet_queue_get(&is->subtitleq, pkt, 1) < 0)
            break;
            
        SDL_LockMutex(is->subpq_mutex);
        while (is->subpq_size >= SUBPICTURE_QUEUE_SIZE &&
               !is->subtitleq.abort_request) {
            SDL_CondWait(is->subpq_cond, is->subpq_mutex);
        }
        SDL_UnlockMutex(is->subpq_mutex);
        
        if (is->subtitleq.abort_request)
            goto the_end;
            
        sp = &is->subpq[is->subpq_windex];

       /* NOTE: ipts is the PTS of the _first_ picture beginning in
           this packet, if any */
        pts = 0;
        if (pkt->pts != AV_NOPTS_VALUE)
            pts = av_q2d(is->subtitle_st->time_base)*pkt->pts;

        SDL_LockMutex(is->subtitle_decoder_mutex);
        len1 = avcodec_decode_subtitle(is->subtitle_st->codec, 
                                    &sp->sub, &got_subtitle, 
                                    pkt->data, pkt->size);
        SDL_UnlockMutex(is->subtitle_decoder_mutex);
//            if (len1 < 0)
//                break;
        if (got_subtitle && sp->sub.format == 0) {
            sp->pts = pts;
            
            for (i = 0; i < sp->sub.num_rects; i++)
            {
                for (j = 0; j < sp->sub.rects[i].nb_colors; j++)
                {
                    RGBA_IN(r, g, b, a, sp->sub.rects[i].rgba_palette + j);
                    y = RGB_TO_Y_CCIR(r, g, b);
                    u = RGB_TO_U_CCIR(r, g, b, 0);
                    v = RGB_TO_V_CCIR(r, g, b, 0);
                    YUVA_OUT(sp->sub.rects[i].rgba_palette + j, y, u, v, a);
                }
            }

            /* now we can update the picture count */
            if (++is->subpq_windex == SUBPICTURE_QUEUE_SIZE)
                is->subpq_windex = 0;
            SDL_LockMutex(is->subpq_mutex);
            is->subpq_size++;
            SDL_UnlockMutex(is->subpq_mutex);
        }
        av_free_packet(pkt);
//        if (step) 
//            if (cur_stream)
//                stream_pause(cur_stream);
    }
 the_end:
    return 0;
}
#endif

/* copy samples for viewing in editor window */
static void update_sample_display(VideoState *is, short *samples, int samples_size)
{
    int size, len, channels;

//    channels = is->audio_st->codec.channels;
    channels = is->audio_st->codec->channels;

    size = samples_size / sizeof(short);
    while (size > 0) {
        len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
        if (len > size)
            len = size;
        memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
        samples += len;
        is->sample_array_index += len;
        if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
            is->sample_array_index = 0;
        size -= len;
    }
}

/* return the new audio buffer size (samples can be added or deleted
   to get better sync if video or external master clock) */
static int synchronize_audio(VideoState *is, short *samples, 
                             int samples_size1, double pts)
{
    int n, samples_size;
    double ref_clock;
    
//    n = 2 * is->audio_st->codec.channels;
    n = 2 * is->audio_st->codec->channels;
    samples_size = samples_size1;

    /* if not master, then we try to remove or add samples to correct the clock */
    if (((is->av_sync_type == AV_SYNC_VIDEO_MASTER && is->video_st) ||
         is->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)) {
        double diff, avg_diff;
        int wanted_size, min_size, max_size, nb_samples;
            
        ref_clock = get_master_clock(is);
        diff = get_audio_clock(is) - ref_clock;
        
        if (diff < AV_NOSYNC_THRESHOLD) {
            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                /* not enough measures to have a correct estimate */
                is->audio_diff_avg_count++;
            } else {
                /* estimate the A-V difference */
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                if (fabs(avg_diff) >= is->audio_diff_threshold) {
//                    wanted_size = samples_size + ((int)(diff * is->audio_st->codec.sample_rate) * n);
                    wanted_size = samples_size + ((int)(diff * is->audio_st->codec->sample_rate) * n);
                    nb_samples = samples_size / n;
                
                    min_size = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX)) / 100) * n;
                    max_size = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX)) / 100) * n;
                    if (wanted_size < min_size)
                        wanted_size = min_size;
                    else if (wanted_size > max_size)
                        wanted_size = max_size;
                    
                    /* add or remove samples to correction the synchro */
                    if (wanted_size < samples_size) {
                        /* remove samples */
                        samples_size = wanted_size;
                    } else if (wanted_size > samples_size) {
                        uint8_t *samples_end, *q;
                        int nb;
                        
                        /* add samples */
                        nb = (samples_size - wanted_size);
                        samples_end = (uint8_t *)samples + samples_size - n;
                        q = samples_end + n;
                        while (nb > 0) {
                            memcpy(q, samples_end, n);
                            q += n;
                            nb -= n;
                        }
                        samples_size = wanted_size;
                    }
                }
#if 0
                printf("diff=%f adiff=%f sample_diff=%d apts=%0.3f vpts=%0.3f %f\n", 
                       diff, avg_diff, samples_size - samples_size1, 
                       is->audio_clock, is->video_clock, is->audio_diff_threshold);
#endif
            }
        } else {
            /* too big difference : may be initial PTS errors, so
               reset A-V filter */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
        }
    }

    return samples_size;
}
#if 1
/* decode one audio frame and returns its uncompressed size */
static int audio_decode_frame(VideoState *is, uint8_t *audio_buf, double *pts_ptr)
{
    AVPacket *pkt = &is->audio_pkt;
    int n, len1, data_size;
    double pts;

    for(;;) {
        /* NOTE: the audio packet can contain several frames */
        while (is->audio_pkt_size > 0) {
			sceKernelWaitSema(is->audio_decoder_mutex, 1, 0);
            len1 = avcodec_decode_audio(is->audio_st->codec, 
                                        (int16_t *)audio_buf, &data_size, 
                                        is->audio_pkt_data, is->audio_pkt_size);
			sceKernelSignalSema(is->audio_decoder_mutex, 1);

            if (len1 < 0) {
                /* if error, we skip the frame */
                is->audio_pkt_size = 0;
                break;
            }
            
            is->audio_pkt_data += len1;
            is->audio_pkt_size -= len1;
            if (data_size <= 0)
                continue;
            /* if no pts, then compute it */
            pts = is->audio_clock;
            *pts_ptr = pts;

            n = 2 * is->audio_st->codec->channels;
            is->audio_clock += (double)data_size / 
                (double)(n * is->audio_st->codec->sample_rate);

#if defined(DEBUG_SYNC)
            {
                static double last_clock;
                printf("audio: delay=%0.3f clock=%0.3f pts=%0.3f\n",
                       is->audio_clock - last_clock,
                       is->audio_clock, pts);
                last_clock = is->audio_clock;
            }
#endif
            return data_size;
        }

        /* free the current packet */
        if (pkt->data)
            av_free_packet(pkt);
        
        if (is->paused || is->audioq.abort_request) {
            return -1;
        }
        
        /* read next packet */
        if (packet_queue_get(&is->audioq, pkt, 1) < 0)
            return -1;
        is->audio_pkt_data = pkt->data;
        is->audio_pkt_size = pkt->size;
        
        /* if update the audio clock with the pts */
        if (pkt->pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
        }
    }
}
#else
static int audio_decode_frame(VideoState *is, uint8_t *audio_buf, double *pts_ptr)
{
    AVPacket *pkt = &is->audio_pkt;
    int n, len1, data_size;
    double pts;

        /* free the current packet */
        if (pkt->data)
            av_free_packet(pkt);
        
        if (is->paused || is->audioq.abort_request) {
            return -1;
        }
        
        /* read next packet */
        if (packet_queue_get(&is->audioq, pkt, 1) < 0)
            return -1;
        is->audio_pkt_data = pkt->data;
        is->audio_pkt_size = pkt->size;
        
        /* if update the audio clock with the pts */
        if (pkt->pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
        }
}
#endif
/* get the current audio output buffer size, in samples. With SDL, we
   cannot have a precise information */
static int audio_write_get_buf_size(VideoState *is)
{
    return is->audio_hw_buf_size - is->audio_buf_index;
}


/* prepare a new audio buffer */
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
static uint16_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

//void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
void sdl_audio_callback(short* buffer, unsigned long length)
{
//	Uint8 *stream;
	uint8_t *stream;
	int len;
	int num_ch;
	int i, j;
//    VideoState *is = opaque;
    VideoState *is = cur_stream;
    int audio_size, len1;
    double pts;
	int size_out;

	sceKernelWaitSema(is->audio_buffer_mutex, 1, 0);
	stream = (uint8_t *)buffer;
	if (!is->audio_st)
	{
		memset(stream, 0, length * 2 * 2);
		sceKernelSignalSema(is->audio_buffer_mutex, 1);
		return;
	}
	num_ch = is->audio_st->codec->channels;
	if (!is->audio_resample)
		len = length * num_ch * 2;
	else
		len = length * 2 * 2;
    audio_callback_time = av_gettime();

    while (len > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {

		   size_out = audio_decode_frame(is, audio_buf, &pts);
		   if (size_out > 0)
		   {
		     if (is->audio_resample)
		     {
				audio_size = audio_resample(is->resample, (short *)is->audio_buf, (short *)audio_buf, 
										  size_out / (is->audio_st->codec->channels * 2));
				audio_size = audio_size * 2 * 2;
		     }
		     else
		     {
				audio_size = size_out;
				memcpy(is->audio_buf, audio_buf, size_out);
		     }
		   }
		   else
		   {
			 audio_size = size_out;
		   }
           if (audio_size < 0) {
                /* if error, just output silence */
			   is->audio_buf_size = len;
               memset(is->audio_buf, 0, is->audio_buf_size);
           } else {
               if (is->show_audio)
                   update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
               audio_size = synchronize_audio(is, (int16_t *)is->audio_buf, audio_size, 
                                              pts);
               is->audio_buf_size = audio_size;
           }
           is->audio_buf_index = 0;
        }

        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;

		if (is->audio_resample || num_ch > 1)
        	memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		else
		{
			uint8_t *left, *right;
			uint8_t *buf;

			left = stream;
			right = stream+2;
			buf = is->audio_buf + is->audio_buf_index;

			i = 0;
			j = 0;
			do
			{
				memcpy(&left[j], &buf[i*2], 2);
				memcpy(&right[j], &buf[i*2], 2);
				j += 4;
				i++;
			} while (i < len1/2);
		}
        len -= len1;
       	stream += len1;
		if (num_ch == 1 && !is->audio_resample)
			stream += len1;
        is->audio_buf_index += len1;
    }
	sceKernelSignalSema(is->audio_buffer_mutex, 1);
}


/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *enc;
    AVCodec *codec;
//    SDL_AudioSpec wanted_spec, spec;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    enc = ic->streams[stream_index]->codec;
    
    /* prepare audio output */
    if (enc->codec_type == CODEC_TYPE_AUDIO) {
//        wanted_spec.freq = enc->sample_rate;
//        wanted_spec.format = AUDIO_S16SYS;
//        /* hack for AC3. XXX: suppress that */
//        if (enc->channels > 2)
//            enc->channels = 2;
//        wanted_spec.channels = enc->channels;
//        wanted_spec.silence = 0;
//        wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//        wanted_spec.callback = sdl_audio_callback;
//        wanted_spec.userdata = is;
//        if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
//            fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
//            return -1;
//        }

//		if (enc->channels != 2 || enc->sample_rate < 44100)
		if (enc->sample_rate < 44100)
		{
			const char *txt;
			txt = mytxt_get_string(MY_TXT_INIT_SOUND, language);
			show_info_box(txt);

			is->audio_resample = 1;
     		if(is->audio_resample){
            	is->resample = audio_resample_init(2, enc->channels,
	                                                44100, enc->sample_rate);
			}
		}
		else
		{
			is->audio_resample = 0;
		}

		pspAudioSetChannelCallback(0, sdl_audio_callback, NULL); // [jonny]
//        is->audio_hw_buf_size = spec.size;
        is->audio_hw_buf_size = 1024 * 2 * 2;
    }																	   

    codec = avcodec_find_decoder(enc->codec_id);
    enc->debug_mv = debug_mv;
    enc->debug = debug;
    enc->workaround_bugs = workaround_bugs;
	enc->lowres = lowres;
    if(lowres) enc->flags |= CODEC_FLAG_EMU_EDGE;
    enc->idct_algo= idct;
    if(fast) enc->flags2 |= CODEC_FLAG2_FAST;
    enc->skip_frame= skip_frame;
    enc->skip_idct= skip_idct;
    enc->skip_loop_filter= skip_loop_filter;
    enc->error_resilience= error_resilience;
    enc->error_concealment= error_concealment;
    if (!codec ||
        avcodec_open(enc, codec) < 0)
        return -1;
#if defined(HAVE_PTHREADS) || defined(HAVE_W32THREADS)
    if(thread_count>1)
        avcodec_thread_init(enc, thread_count);
#endif
    enc->thread_count= thread_count;
    switch(enc->codec_type) {
    case CODEC_TYPE_AUDIO:
        is->audio_stream = stream_index;
        is->audio_st = ic->streams[stream_index];
        is->audio_buf_size = 0;
        is->audio_buf_index = 0;

        /* init averaging filter */
        is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
        is->audio_diff_avg_count = 0;
        /* since we do not have a precise anough audio fifo fullness,
           we correct audio sync only if larger than this threshold */
        is->audio_diff_threshold = 2.0 * SDL_AUDIO_BUFFER_SIZE / enc->sample_rate;

        memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
        packet_queue_init(&is->audioq);
//	SDL_PauseAudio(0);
        break;
    case CODEC_TYPE_VIDEO:
//		if (ic->streams[stream_index]->codec.width > 480)
		if (ic->streams[stream_index]->codec->width > 480)
		{
			return -1;
		}

		scePowerSetClockFrequency(266,266,133);
        is->video_stream = stream_index;
        is->video_st = ic->streams[stream_index];

        is->frame_last_delay = 40e-3;
        is->frame_timer = (double)av_gettime() / 1000000.0;
        is->video_current_pts_time = av_gettime();

        packet_queue_init(&is->videoq);
		video_arg = is;
		set_disp_mode(is, mode);
		skip_frames = 0;
//        is->video_tid = SDL_CreateThread(video_thread, is);
        is->video_tid = sceKernelCreateThread("Video thread", video_thread, 0x15, 0x10000, THREAD_ATTR_USER, 0);
		if (is->video_tid >= 0)
		{
			sceKernelStartThread(is->video_tid, 0, 0);
		}
        break;
#ifdef SUBTITLE
    case CODEC_TYPE_SUBTITLE:
        is->subtitle_stream = stream_index;
        is->subtitle_st = ic->streams[stream_index];
        packet_queue_init(&is->subtitleq);
        
        is->subtitle_tid = SDL_CreateThread(subtitle_thread, is);
        break;
#endif
    default:
        break;
    }
    return 0;
}

static void stream_component_close(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *enc;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    enc = ic->streams[stream_index]->codec;
        
    switch(enc->codec_type) {
    case CODEC_TYPE_AUDIO:
        packet_queue_abort(&is->audioq);

//        SDL_CloseAudio();
		pspAudioSetChannelCallback(0, NULL, NULL); // [jonny]

		if (is->audio_resample)
		{
			audio_resample_close(is->resample);
		}

        packet_queue_end(&is->audioq);
        break;
    case CODEC_TYPE_VIDEO:
        packet_queue_abort(&is->videoq);

        /* note: we also signal this mutex to make sure we deblock the
           video thread in all cases */
//        SDL_LockMutex(is->pictq_mutex);
//        SDL_CondSignal(is->pictq_cond);
//        SDL_UnlockMutex(is->pictq_mutex);
		sceKernelWaitSema(is->pictq_mutex, 1, 0);
		sceKernelSignalSema(is->pictq_mutex, 1);

//        SDL_WaitThread(is->video_tid, NULL);
		sceKernelWaitThreadEnd(is->video_tid, NULL);
		sceKernelDeleteThread(is->video_tid);

        packet_queue_end(&is->videoq);
//		scePowerIdleTimerEnable();
		scePowerSetClockFrequency(222,222,111);
        break;
#ifdef SUBTITLE
    case CODEC_TYPE_SUBTITLE:
        packet_queue_abort(&is->subtitleq);
        
        /* note: we also signal this mutex to make sure we deblock the
           video thread in all cases */
        SDL_LockMutex(is->subpq_mutex);
        is->subtitle_stream_changed = 1;
    
        SDL_CondSignal(is->subpq_cond);
        SDL_UnlockMutex(is->subpq_mutex);

        SDL_WaitThread(is->subtitle_tid, NULL);

        packet_queue_end(&is->subtitleq);
        break;
#endif
    default:
        break;
    }

    avcodec_close(enc);
    switch(enc->codec_type) {
    case CODEC_TYPE_AUDIO:
        is->audio_st = NULL;
        is->audio_stream = -1;
        break;
    case CODEC_TYPE_VIDEO:
        is->video_st = NULL;
        is->video_stream = -1;
        break;
#ifdef SUBTITLE
    case CODEC_TYPE_SUBTITLE:
        is->subtitle_st = NULL;
        is->subtitle_stream = -1;
        break;
#endif
    default:
        break;
    }
}

void dump_stream_info(AVFormatContext *s)
{
    if (s->track != 0)
        fprintf(stderr, "Track: %d\n", s->track);
    if (s->title[0] != '\0')
        fprintf(stderr, "Title: %s\n", s->title);
    if (s->author[0] != '\0')
        fprintf(stderr, "Author: %s\n", s->author);
    if (s->album[0] != '\0')
        fprintf(stderr, "Album: %s\n", s->album);
    if (s->year != 0)
        fprintf(stderr, "Year: %d\n", s->year);
    if (s->genre[0] != '\0')
        fprintf(stderr, "Genre: %s\n", s->genre);
}

/* since we have only one decoding thread, we can use a global
   variable instead of a thread local variable */
static VideoState *global_video_state;

static int decode_interrupt_cb(void)
{
    return (global_video_state && global_video_state->abort_request);
}

/* this thread gets the stream from the disk or the network */
static VideoState* decode_arg;

static int decode_thread(void *arg)
{
//    VideoState *is = arg;
    VideoState *is = decode_arg;

    AVFormatContext *ic;
    int err, i, ret, video_index, audio_index, use_play;
    AVPacket pkt1, *pkt = &pkt1;
    AVFormatParameters params, *ap = &params;
	char err_msg[2048];

    video_index = -1;
    audio_index = -1;
    is->video_stream = -1;
    is->audio_stream = -1;
#ifdef SUBTITLE
	is->subtitle_stream = -1;
#endif

    global_video_state = is;
    url_set_interrupt_cb(decode_interrupt_cb);

    memset(ap, 0, sizeof(*ap));
    ap->image_format = image_format;
    ap->initial_pause = 1; /* we force a pause when starting an RTSP
                              stream */
    
    err = av_open_input_file(&ic, is->filename, is->iformat, 0, ap);
    if (err < 0) {
        print_error(is->filename, err);
		catched_error = 1;
        ret = -1;
        goto fail;
    }
    is->ic = ic;
#ifdef CONFIG_NETWORK
    use_play = (ic->iformat == &rtsp_demux);
#else
    use_play = 0;
#endif


    if(genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;

    if (!use_play) {
        err = av_find_stream_info(ic);
        if (err < 0) {
//            fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
            snprintf(err_msg, 2048, "%s: could not find codec parameters\n", is->filename);
			msg_box((const char *)err_msg);
			catched_error = 1;
            ret = -1;
            goto fail;
        }
		ic->pb.eof_reached= 0; //FIXME hack, ffplay maybe shouldnt use url_feof() to test for the end
    }

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = av_seek_frame(ic, -1, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            fprintf(stderr, "%s: could not seek to position %0.3f\n", 
                    is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    /* now we can begin to play (RTSP stream only) */
    av_read_play(ic);

    if (use_play) {
        err = av_find_stream_info(ic);
        if (err < 0) {
//            fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
            sprintf(err_msg, "%s: could not find codec parameters\n", is->filename);
			msg_box((const char *)err_msg);
			catched_error = 1;
            ret = -1;
            goto fail;
        }
    }

    for(i = 0; i < ic->nb_streams; i++) {
//        AVCodecContext *enc = &ic->streams[i]->codec;
        AVCodecContext *enc = ic->streams[i]->codec;
        switch(enc->codec_type) {
        case CODEC_TYPE_AUDIO:
            if (audio_index < 0 && !audio_disable)
                audio_index = i;
            break;
        case CODEC_TYPE_VIDEO:
            if (video_index < 0 && !video_disable)
                video_index = i;
            break;
        default:
            break;
        }
    }
    if (show_status) {
        dump_format(ic, 0, is->filename, 0);
        dump_stream_info(ic);
    }

    /* open the streams */
    if (audio_index >= 0) {
        stream_component_open(is, audio_index);
    }

    if (video_index >= 0) {
        stream_component_open(is, video_index);
    } else {
        if (!display_disable)
            is->show_audio = 1;
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
//        fprintf(stderr, "%s: could not open codecs\n", is->filename);
        snprintf(err_msg, 2048, "%s: could not open codecs\n", is->filename);
		msg_box((const char *)err_msg);
		catched_error = 1;
        ret = -1;
        goto fail;
    }

    for(;;) {
        if (is->abort_request)
            break;
#ifdef CONFIG_NETWORK
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused)
                av_read_pause(ic);
            else
                av_read_play(ic);
        }
        if (is->paused && ic->iformat == &rtsp_demux) {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
//            SDL_Delay(10);
			sceKernelDelayThread(10000);
            continue;
        }
#endif
        if (is->seek_req) {
			pspAudioSetChannelCallback(0, NULL, NULL); // [jonny]
            /* XXX: must lock decoder threads */
            sceKernelWaitSema(is->video_decoder_mutex, 1, 0);
            sceKernelWaitSema(is->audio_decoder_mutex, 1, 0);
#ifdef SUBTITLE
			sceKernelWaitSema(is->subtitle_decoder_mutex, 1, 0);
#endif
            ret = av_seek_frame(is->ic, -1, is->seek_pos, is->seek_flags);

            if (ret < 0) {
                fprintf(stderr, "%s: error while seeking\n", is->ic->filename);
            }else{
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                }
#ifdef SUBTITLE
				if (is->subtitle_stream >= 0) {
                    packet_queue_flush(&is->subtitleq);
                }
#endif
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    avcodec_flush_buffers(ic->streams[video_index]->codec);
                }
            }
#ifdef SUBTITLE
			sceKernelSignalSema(is->subtitle_decoder_mutex, 1);
#endif
			sceKernelSignalSema(is->audio_decoder_mutex, 1);
			sceKernelSignalSema(is->video_decoder_mutex, 1);

            is->seek_req = 0;
			pspAudioSetChannelCallback(0, sdl_audio_callback, NULL); // [jonny]
        }

        /* if the queue are full, no need to read more */
        if (is->audioq.size > MAX_AUDIOQ_SIZE ||
            is->videoq.size > MAX_VIDEOQ_SIZE || 
#ifdef SUBTITLE
			is->subtitleq.size > MAX_SUBTITLEQ_SIZE || 
#endif
            url_feof(&ic->pb)) {
            /* wait 10 ms */
//            SDL_Delay(10);
			if (url_feof(&ic->pb))
			{
				if (is->audioq.size == 0)
					play_finished = 1;
			}
			sceKernelDelayThread(10000);
            continue;
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
	    	if (url_ferror(&ic->pb) == 0) {
//                SDL_Delay(100); /* wait for user event */
				sceKernelDelayThread(100000);
				continue;
	    	} else
			{
				play_finished = 1;
            	break;
			}
        }
        if (pkt->stream_index == is->audio_stream) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream) {
            packet_queue_put(&is->videoq, pkt);
#ifdef SUBTITLE
		} else if (pkt->stream_index == is->subtitle_stream) {
            packet_queue_put(&is->subtitleq, pkt);
#endif
        } else {
            av_free_packet(pkt);
        }
    }
    /* wait until the end */
    while (!is->abort_request) {
//        SDL_Delay(100);
		sceKernelDelayThread(100000);
    }

    ret = 0;
 fail:
    /* disable interrupting */
    global_video_state = NULL;

    /* close each stream */
    if (is->audio_stream >= 0)
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
#ifdef SUBTITLE
    if (is->subtitle_stream >= 0)
        stream_component_close(is, is->subtitle_stream);
#endif
    if (is->ic) {
        av_close_input_file(is->ic);
        is->ic = NULL; /* safety */
    }
    url_set_interrupt_cb(NULL);

    if (ret != 0) {
//        SDL_Event event;
        
//        event.type = FF_QUIT_EVENT;
//        event.user.data1 = is;
//        SDL_PushEvent(&event);
//		done = 1;
    }
    return 0;
}

static VideoState *stream_open(const char *filename, AVInputFormat *iformat)
{
    VideoState *is;

    is = av_mallocz(sizeof(VideoState));
    if (!is)
        return NULL;
    pstrcpy(is->filename, sizeof(is->filename), filename);
    is->iformat = iformat;
//    if (screen) {
//        is->width = screen->w;
//        is->height = screen->h;
//    }
	is->width = 480;
	is->height = 272;
    is->ytop = 0;
    is->xleft = 0;

    /* start video display */
//    is->pictq_mutex = SDL_CreateMutex();
//    is->pictq_cond = SDL_CreateCond();
	is->pictq_mutex = sceKernelCreateSema("", 0, 1, 1, 0);
#ifdef SUBTITLE
    is->subpq_mutex = SDL_CreateMutex();
    is->subpq_cond = SDL_CreateCond();
    
    is->subtitle_decoder_mutex = SDL_CreateMutex();
#endif
    is->audio_decoder_mutex = sceKernelCreateSema("", 0, 1, 1, 0);
    is->video_decoder_mutex = sceKernelCreateSema("", 0, 1, 1, 0);
	is->audio_buffer_mutex = sceKernelCreateSema("", 0, 1, 1, 0);
    /* add the refresh timer to draw the picture */
    schedule_refresh(is, 40);

    is->av_sync_type = av_sync_type;

	decode_arg = is;
//    is->parse_tid = SDL_CreateThread(decode_thread, is);
    is->parse_tid = sceKernelCreateThread("Decode thread", decode_thread, 0x08, 0x10000, THREAD_ATTR_USER, 0);
//    if (!is->parse_tid) {
	if (is->parse_tid < 0) {
        av_free(is);
        return NULL;
    }
	else
	{
		sceKernelStartThread(is->parse_tid, 0, 0);
	}
    return is;
}

static void stream_close(VideoState *is)
{
    VideoPicture *vp;
    int i;
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
//    SDL_WaitThread(is->parse_tid, NULL);
	sceKernelWaitThreadEnd(is->parse_tid, NULL);
	sceKernelDeleteThread(is->parse_tid);

    /* free all pictures */
    for(i=0;i<VIDEO_PICTURE_QUEUE_SIZE; i++) {
        vp = &is->pictq[i];
        if (vp->bmp) {
//            SDL_FreeYUVOverlay(vp->bmp);
//            vp->bmp = NULL;



//			av_free(vp->buffer); // [jonny]
			free_64(vp->buffer); // [jonny]



			av_free(vp->bmp);
			vp->buffer = NULL;
			vp->bmp = NULL;
        }
    }
//    SDL_DestroyMutex(is->pictq_mutex);
//    SDL_DestroyCond(is->pictq_cond);
	sceKernelDeleteSema(is->pictq_mutex);
#ifdef SUBTITLE	
	SDL_DestroyMutex(is->subpq_mutex);
    SDL_DestroyCond(is->subpq_cond);
    SDL_DestroyMutex(is->subtitle_decoder_mutex);
#endif
    sceKernelDeleteSema(is->audio_decoder_mutex);
    sceKernelDeleteSema(is->video_decoder_mutex);
	sceKernelDeleteSema(is->audio_buffer_mutex);
// Jini ...
	av_free(is);
}

void stream_cycle_channel(VideoState *is, int codec_type)
{
    AVFormatContext *ic = is->ic;
    int start_index, stream_index;
    AVStream *st;

    if (codec_type == CODEC_TYPE_VIDEO)
        start_index = is->video_stream;
	else if (codec_type == CODEC_TYPE_AUDIO)
        start_index = is->audio_stream;
#ifdef SUBTITLE
    else
        start_index = is->subtitle_stream;
    if (start_index < (codec_type == CODEC_TYPE_SUBTITLE ? -1 : 0))
#else
	if (start_index < 0)
#endif
        return;
    stream_index = start_index;
    for(;;) {
        if (++stream_index >= is->ic->nb_streams)
		{
#ifdef SUBTITLE
            if (codec_type == CODEC_TYPE_SUBTITLE)
            {
                stream_index = -1;
                goto the_end;
            } else
#endif
            stream_index = 0;
		}
        if (stream_index == start_index)
            return;
        st = ic->streams[stream_index];
        if (st->codec->codec_type == codec_type) {
            /* check that parameters are OK */
            switch(codec_type) {
            case CODEC_TYPE_AUDIO:
                if (st->codec->sample_rate != 0 &&
                    st->codec->channels != 0)
                    goto the_end;
                break;
            case CODEC_TYPE_VIDEO:
#ifdef SUBTITLE
			case CODEC_TYPE_SUBTITLE:
#endif
                goto the_end;
            default:
                break;
            }
        }
    }
 the_end:
    stream_component_close(is, start_index);
    stream_component_open(is, stream_index);
}

#if 0
void toggle_full_screen(void)
{
    int w, h, flags;
    is_full_screen = !is_full_screen;
    if (!fs_screen_width) {
        /* use default SDL method */
        SDL_WM_ToggleFullScreen(screen);
    } else {
        /* use the recorded resolution */
        flags = SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_HWACCEL;
        if (is_full_screen) {
            w = fs_screen_width;
            h = fs_screen_height;
            flags |= SDL_FULLSCREEN;
        } else {
            w = screen_width;
            h = screen_height;
            flags |= SDL_RESIZABLE;
        }
        screen = SDL_SetVideoMode(w, h, 0, flags);
        cur_stream->width = w;
        cur_stream->height = h;
    }
}
#endif
void toggle_pause(void)
{
    if (cur_stream)
        stream_pause(cur_stream);
    step = 0;
}

void step_to_next_frame(void)
{
    if (cur_stream) {
        if (cur_stream->paused)
            cur_stream->paused=0;
        cur_stream->video_current_pts = get_video_clock(cur_stream);
    }
    step = 1;
}

void do_exit(void)
{
    if (cur_stream) {
        stream_close(cur_stream);
        cur_stream = NULL;
    }
    if (show_status)
        printf("\n");
//	scePowerSetClockFrequency(222, 222, 111);
//    SDL_Quit();
//	pgShutDown();
//	pspAudioEnd();
//	sceKernelExitGame();
//    exit(0);
}

void toggle_audio_display(void)
{
    if (cur_stream) {
        cur_stream->show_audio = !cur_stream->show_audio;
    }
}

void reset_display_clock(void)
{
	display_clock = 0;
	do_timer_action = NULL;
}

void change_cpu_clock(void)
{
	int cpu_freq;

	cpu_freq = scePowerGetCpuClockFrequency();
	if (cpu_freq >= 333)
	{
		scePowerSetClockFrequency(266,266,133);
		snprintf(clock_text, 256, "clock : %d", 266);
	}
	else
	{
		scePowerSetClockFrequency(333,333,166);
		snprintf(clock_text, 256, "clock : %d", 333);
	}

	next_timer_check_time = 2000000 + av_gettime();
	need_general_timer_check = 1;
	display_clock = 1;

	do_timer_action = reset_display_clock;
}

/* handle an event sent by the GUI */
void event_loop(void)
{
//    SDL_Event event;
    double incr, pos, frac;
	unsigned long key = 0;
	static unsigned long old_key = 0;
	SceCtrlData pad;

    for(;;) {
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons != old_key)
		{
			if (old_key)
			{
				key = old_key;
				old_key = 0;
			}
			else
			{
				key = 0;
				old_key = pad.Buttons;
			}					
		}
		if (key & PSP_CTRL_START)
		{
//			do_exit();
			toggle_pause();
			sceKernelDelayThread(1000);
			mode++;
			mode %= 3;
			set_disp_mode(cur_stream, mode);
			clear_screen();
			disp_update();
			clear_screen();
			disp_update();
			toggle_pause();
			key = 0;
		}
		if (key & PSP_CTRL_TRIANGLE)
		{
			toggle_pause();
			change_cpu_clock();
			key = 0;
			toggle_pause();
		}
		if (key & PSP_CTRL_SQUARE)
		{
			toggle_pause();
			do_exit();
			key = 0;
			return;
		}
		if (key & PSP_CTRL_CROSS)
		{
			toggle_pause();
			key = 0;
		}
		if (key & PSP_CTRL_LEFT)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = -10.0;
				goto do_seek;
			}
			else
			{
				key = 0;
			}
		}
		if (key & PSP_CTRL_RIGHT)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = 10.0;
				goto do_seek;
			}
			else
			{
				key = 0;
			}
		}
		if (key & PSP_CTRL_LTRIGGER)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = -300.0;
				goto do_seek;
			}
			else if (cur_stream->audio_st)
			{
				toggle_pause();
				do_exit();
				key = 0;
				if (get_prev_file(MEDIA_MUSIC))
				{
					input_filename = FileName;
					cur_stream = stream_open(input_filename, file_iformat);
				}
				else
					return;
			}
			else
				key = 0;
		}
		if (key & PSP_CTRL_RTRIGGER)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = 300.0;
				goto do_seek;
			}
			else if (cur_stream->audio_st)
			{
				toggle_pause();
				do_exit();
				key = 0;
				if (get_next_file(MEDIA_MUSIC))
				{
					input_filename = FileName;
					cur_stream = stream_open(input_filename, file_iformat);
				}
				else
					return;
			}
			else
				key = 0;
		}
		if (key & PSP_CTRL_UP)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = 60.0;
				goto do_seek;
			}
			else
			{
				key = 0;
			}
		}
		if (key & PSP_CTRL_DOWN)
		{
			if (cur_stream->video_st)
			{
				toggle_pause();
				incr = -60.0;
do_seek:
        	    if (cur_stream) {
            	    pos = get_master_clock(cur_stream);
                	pos += incr;
                    stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), incr);
    	        }
				toggle_pause();
				key = 0;
			}
			else
			{
				key = 0;
			}
		}
		if (need_alloc)
		{
			alloc_picture(cur_stream);
			need_alloc = 0;
		}
		if (need_timer_check && av_gettime() >= next_refresh_time)
		{
			need_timer_check = 0;
			video_refresh_timer(cur_stream);
		}
		if (need_general_timer_check && av_gettime() >= next_timer_check_time)
		{
			need_general_timer_check = 0;
			do_timer_action();
		}
		if (play_finished)
		{
			play_finished = 0;
//			if (cur_stream->video_st)
//			{
//			}
//			else
			if (cur_stream->audio_st && cur_stream->show_audio)
			{
				toggle_pause();
				do_exit();
				if (get_next_file(MEDIA_MUSIC))
				{
					input_filename = FileName;
					cur_stream = stream_open(input_filename, file_iformat);
				}
//				else
//					return;
			}
		}
		if (return_from_sleep)
		{
			return_from_sleep = 0;
			toggle_pause();
			do_exit();
			sceKernelDelayThread(500000);
			strcpy(FileName, saved_filename);
			input_filename = FileName;
			cur_stream = stream_open(input_filename, file_iformat);
			sceKernelDelayThread(500000);
			stream_seek(cur_stream, (int64_t)(saved_pos * AV_TIME_BASE), 0);
		}
		if (catched_error)
		{
			catched_error = 0;
			return;
		}
		if (done)
		{
			do_exit();
			pgShutDown();
			pspAudioEnd();
//			scePowerSetClockFrequency(222, 222, 111);
			reset_background();
			sceKernelExitGame();
		}
#if 0
        SDL_WaitEvent(&event);
        switch(event.type) {
#if 0
		case SDL_JOYBUTTONDOWN:
            switch(event.jbutton.button) {
            case 11: /* start */
                do_exit();
                break;
//            case SDLK_f:
//                toggle_full_screen();
//                break;
            case 2: /* cross */
                toggle_pause();
                break;
//            case 9: /* right */
//                step_to_next_frame();
//                break;
//            case SDLK_a:
//                if (cur_stream) 
//                    stream_cycle_channel(cur_stream, CODEC_TYPE_AUDIO);
//                break;
//            case SDLK_v:
//                if (cur_stream) 
//                    stream_cycle_channel(cur_stream, CODEC_TYPE_VIDEO);
//                break;
            case 0: /* triangle */
                toggle_audio_display();
                break;
            case 7: /* left */
                incr = -10.0;
                goto do_seek;
            case 9: /* right */
                incr = 10.0;
                goto do_seek;
            case 8: /* up */
                incr = 60.0;
                goto do_seek;
            case 6: /* down */
                incr = -60.0;
            do_seek:
                if (cur_stream) {
                    pos = get_master_clock(cur_stream);
                    pos += incr;
                    stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE));
                }
                break;
            default:
                break;
			}
			break;

        case SDL_MOUSEBUTTONDOWN:
	    if (cur_stream) {
		int ns, hh, mm, ss;
		int tns, thh, tmm, tss;
		tns = cur_stream->ic->duration/1000000LL;
		thh = tns/3600;
		tmm = (tns%3600)/60;
		tss = (tns%60);
		frac = (double)event.button.x/(double)cur_stream->width;
		ns = frac*tns;
		hh = ns/3600;
		mm = (ns%3600)/60;
		ss = (ns%60);
		fprintf(stderr, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
			hh, mm, ss, thh, tmm, tss);
		stream_seek(cur_stream, (int64_t)(cur_stream->ic->start_time+frac*cur_stream->ic->duration));
	    }
	    break;
//        case SDL_VIDEORESIZE:
//            if (cur_stream) {
//                screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, 
//                                          SDL_HWSURFACE|SDL_RESIZABLE|SDL_ASYNCBLIT|SDL_HWACCEL);
//                cur_stream->width = event.resize.w;
//                cur_stream->height = event.resize.h;
//            }
//            break;
#endif
        case SDL_QUIT:
        case FF_QUIT_EVENT:
            do_exit();
            break;
        case FF_ALLOC_EVENT:
            alloc_picture(event.user.data1);
            break;
        case FF_REFRESH_EVENT:
            video_refresh_timer(event.user.data1);
            break;
        default:
            break;
        }
#endif
    }
}

void file_list_reset()
{
}

int file_list_init(media_type type)
{
  int result;
  int i, len;
  int handle, ret;

  result = 1;

  if (type == MEDIA_VIDEO)
  	snprintf(media_dir_name, MAXPATHLEN, "ms0:/PSP/VIDEO/");
  else
    snprintf(media_dir_name, MAXPATHLEN, "ms0:/PSP/MUSIC/");
  handle = sceIoDopen(media_dir_name);
  if (handle < 0)
  {
	result = 0; /* failed */
  }
  else
  {
    i = 0;
	ret = 1;
    while ( (ret > 0) && i < MAX_MEDIA_FILES ) {
	  ret = sceIoDread(handle, &media_file_list[i]);
	  if (!FIO_SO_ISREG(media_file_list[i].d_stat.st_attr)) /* not a file so skip */
	  {
	    continue;
	  }
	  if (ret > 0)
	    i++;
	}
	media_file_num = i;
	list_end = media_file_num - 1;
  }
  sceIoDclose(handle);
  if (media_file_num == 0)
  {
    result = 0;
  }
  return result;
}

int media_list_init(media_type type)
{
  return file_list_init(type);
}

//#define TOP_HEIGHT 20
#define TOP_HEIGHT 0
#define ITEM_HEIGHT 18
#define BOTTOM_HEIGHT 20
//#define ITEM_NUM ((SCREEN_HEIGHT - TOP_HEIGHT - BOTTOM_HEIGHT) / ITEM_HEIGHT)
#define ITEM_NUM ((SCREEN_HEIGHT - 40) / ITEM_HEIGHT)
#define LINE_PAD 2

void clear_screen()
{
  my_rect area;
  unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

  area.x = 0;
  area.y = 0;
  area.w = LINESIZE;
  area.h = SCREEN_HEIGHT;
  my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK);
  disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
}

void draw_item(int scrn_idx, int file_idx, u8 *text, u8 scrolbar)
{
  int start_x, start_y;
  int text_len;
  my_rect area;
  unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

  area.x = 20;
  area.y = scrn_idx * ITEM_HEIGHT + 20;
  area.w = (SCREEN_WIDTH - 40);
  area.h = ITEM_HEIGHT;

//  if (scrolbar)
//  {
//    area.w -= MY_SCROLBAR_WIDTH;
//  }
//  my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_WHITE);

  if (text)
  {  
    start_x = 0;
    text_len = strlen((void *)text);
    start_y = LINE_PAD;
    my_draw_text(screen_buffer, LINESIZE, start_x, start_y, text, text_len, MY_COLOR_WHITE, area);
  }
}
void draw_item_selected(int scrn_idx, int file_idx, u8 *text, u8 scrolbar)
{
  int start_x, start_y;
  int text_len;
  my_rect area;
  my_rect src_area;
  unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

  if (text == NULL)
  {
    return;
  }

  area.x = 20;
  area.y = scrn_idx * ITEM_HEIGHT + 20;
  area.w = (SCREEN_WIDTH - 40);
  area.h = ITEM_HEIGHT;

//  src_area.x = 0;
//  src_area.y = 0;
//  src_area.w = SCREEN_WIDTH;
//  src_area.h = ITEM_HEIGHT;
//  if (scrolbar)
//  {
//    area.w -= MY_SCROLBAR_WIDTH;
//	src_area.w -= (MY_SCROLBAR_WIDTH+1);
//  }
//  my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_WHITE);
//  if (scrolbar)
//  {
//    area.w--;
//  }

//  my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_LIGHT_GRAY);
  my_clear_area_alpha(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_LIGHT_GRAY, 125);

  start_x = 0;
  text_len = strlen((void *)text);
  start_y = LINE_PAD;
  my_draw_text(screen_buffer, LINESIZE, start_x, start_y, text, text_len, MY_COLOR_YELLOW, area);
}

void draw_media_list(int last_start, int start, int last, int current, int end)
{
  int i;
  int scrn_idx;
  int index;
  int start_y;
  my_scrolbar scrol;
  my_rect area;
  unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	if (background)
	{
		my_copy_bmp(screen_buffer, LINESIZE, SCREEN_HEIGHT, 0, 0, background);
		area.x = 20;
		area.y = 20;
		area.w = (SCREEN_WIDTH - 40);
		area.h = (SCREEN_HEIGHT - 40);
		my_clear_area_alpha(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK, 126);
	}
	else
	{
		area.x = 0;
		area.y = 0;
		area.w = SCREEN_WIDTH;
		area.h = SCREEN_HEIGHT;
		my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_WHITE);
	}

    index = start;
    scrn_idx = 0;
    has_scrolbar = 0;
    for (i = 0; i < ITEM_NUM; i++)
	{
	  if (index <= end)
	  {
        if (index == current)
		{
		  draw_item_selected(scrn_idx, index, (u8 *)media_file_list[index].d_name, has_scrolbar);
		}
		else
		{
		  draw_item(scrn_idx, index, (u8 *)media_file_list[index].d_name, has_scrolbar);
		}
	  }
	  else
	  {
		draw_item(scrn_idx, 0, NULL, has_scrolbar);
	  }
	  scrn_idx++;
	  index++;
	}
    if (has_scrolbar)
	{
	  scrol.x = SCREEN_WIDTH - MY_SCROLBAR_WIDTH;
	  scrol.y = TOP_HEIGHT;
	  scrol.w = MY_SCROLBAR_WIDTH;
	  scrol.h = ITEM_HEIGHT * ITEM_NUM;

	  scrol.start = start+1;
	  scrol.end = start + ITEM_NUM;
	  scrol.total = end+1;
	  scrol.color = MY_COLOR_WHITE;
	  my_draw_scrolbar(screen_buffer, LINESIZE, SCREEN_HEIGHT, &scrol);
	}
  disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
}

void show_info_box(const char *msg)
{
	my_rect area;
	my_doc *doc;
	my_text *p;
	const char *text;
	int len, y;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	if (!msg)
		return;
	area.x = 40;
	area.y = 20;
	area.w = SCREEN_WIDTH - 80;
	area.h = SCREEN_HEIGHT - 40;

	my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK);
//	my_draw_rect(screen_buffer, LINESIZE, SCREEN_HEIGHT, area.x, area.y, area.w, area.h, MY_COLOR_LIGHT_GRAY);

	area.x += 2;
  	area.w -= 4;

	len = strlen((const char *)msg);
  	doc = my_str_to_doc((const char *)msg, len, area.w);

  	p = doc->head->next;
  	if (doc->linecnt * NORMAL_FONT_HEIGHT < area.h)
  	{
    	area.y += (area.h - doc->linecnt * NORMAL_FONT_HEIGHT) / 2;
    	area.h = doc->linecnt * NORMAL_FONT_HEIGHT;
  	}
  	else
  	{
    	area.y += 2;
    	area.h -= 4;
  	}
  	y = 0;
  	while (p != doc->tail)
  	{
    	text = (const char *)p->text;
		len = p->len;
		while (*text == ' ')
		{
	  		text++;
	  		len--;
		}
		if (len > 0)
		{
     		my_draw_aligned_text(screen_buffer, LINESIZE, y, text, len, ALIGN_MIDDLE, MY_COLOR_MSG, area);
		}
    	y += NORMAL_FONT_HEIGHT;
    	if (y > (area.h - NORMAL_FONT_HEIGHT))
      		break;
    	p = p->next;
  	}
  	my_doc_release(doc);

	disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0); 
	disp_update();
}

void msg_box(const char *msg)
{
	unsigned long key = 0;
	static unsigned long old_key = 0;
	SceCtrlData pad;

	my_rect area;
	my_doc *doc;
	my_text *p;
	const char *text;
	int len, y;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	if (!msg)
		return;
	area.x = 40;
	area.y = 20;
	area.w = SCREEN_WIDTH - 80;
	area.h = SCREEN_HEIGHT - 40;

	my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK);
	my_draw_rect(screen_buffer, LINESIZE, SCREEN_HEIGHT, area.x, area.y, area.w, area.h, MY_COLOR_LIGHT_GRAY);

	area.x += 2;
  	area.w -= 4;

	len = strlen((const char *)msg);
  	doc = my_str_to_doc((const char *)msg, len, area.w);

  	p = doc->head->next;
  	if (doc->linecnt * NORMAL_FONT_HEIGHT < area.h)
  	{
    	area.y += (area.h - doc->linecnt * NORMAL_FONT_HEIGHT) / 2;
    	area.h = doc->linecnt * NORMAL_FONT_HEIGHT;
  	}
  	else
  	{
    	area.y += 2;
    	area.h -= 4;
  	}
  	y = 0;
  	while (p != doc->tail)
  	{
    	text = (const char *)p->text;
		len = p->len;
		while (*text == ' ')
		{
	  		text++;
	  		len--;
		}
		if (len > 0)
		{
     		my_draw_aligned_text(screen_buffer, LINESIZE, y, text, len, ALIGN_MIDDLE, MY_COLOR_MSG, area);
		}
    	y += NORMAL_FONT_HEIGHT;
    	if (y > (area.h - NORMAL_FONT_HEIGHT))
      		break;
    	p = p->next;
  	}
  	my_doc_release(doc);
/*
	len = strlen((const char *)msg);
	area.x = 42;
	area.y = 22;
	area.w -= 4;
	area.h -= 4;
	my_draw_multi_text(screen_buffer, LINESIZE, 0, 0, msg, len, 1, MY_COLOR_WHITE, area);
*/
	area.x = 40;
	area.y = 20;
	area.w = SCREEN_WIDTH - 80;
	area.h = SCREEN_HEIGHT - 40;

	text = mytxt_get_string(MY_TXT_PRESS_CROSS, language);
	len = strlen(text);
	my_draw_aligned_text(screen_buffer, LINESIZE, area.h - 20, text, len, ALIGN_MIDDLE, MY_COLOR_RED, area);

	disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0); 
	disp_update();

	while (1)
	{
		if (done)
			break;
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons != old_key)
		{
			if (old_key)
			{
				key = old_key;
				old_key = 0;
			}
			else
			{
				key = 0;
				old_key = pad.Buttons;
			}					
		}
		if (key & PSP_CTRL_CROSS)
		{
			key = 0;
			return;
		}
	}
	if (done)
	{
		pgShutDown();
		pspAudioEnd();
		reset_background();
		sceKernelExitGame();
	}
}

void init_background()
{
	background = read_jpeg_file("background.jpg");	
}

void reset_background()
{
	free_img(background);
	background = NULL;
}

void greeting()
{
	unsigned long key = 0;
	static unsigned long old_key = 0;
	SceCtrlData pad;

	my_rect area;
	char *intro_txt;
	int len;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	area.x = 0;
	area.y = 0;
	area.w = SCREEN_WIDTH;
	area.h = SCREEN_HEIGHT;
	my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK);

	if (background)
	{
		my_copy_bmp(screen_buffer, LINESIZE, SCREEN_HEIGHT, 0, 0, background);
	}

	area.x = 50;
	area.y = 50;
	area.w = (SCREEN_WIDTH - 100);
	area.h = (SCREEN_HEIGHT - 100);

	my_clear_area_alpha(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK, 126);

	intro_txt = mytxt_get_string(MY_TXT_INTRO, language);  
	len = strlen(intro_txt);
	my_draw_multi_text(screen_buffer, LINESIZE, 0, 0, intro_txt, len, 1, MY_COLOR_WHITE, area);

//	area.y = 120;
//	area.h = 120;
	intro_txt = mytxt_get_string(MY_TXT_KEY_HELP, language);
	len = strlen(intro_txt);
	my_draw_multi_text(screen_buffer, LINESIZE, 0, 90, intro_txt, len, 1, MY_COLOR_WHITE, area);
	disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0); 
	disp_update();

	while (1)
	{
		if (done)
			break;
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons != old_key)
		{
			if (old_key)
			{
				key = old_key;
				old_key = 0;
			}
			else
			{
				key = 0;
				old_key = pad.Buttons;
			}					
		}
		if (key & PSP_CTRL_SELECT)
		{
			return;
		}
		if (key & PSP_CTRL_START)
		{
			done = 1;
			key = 0;
			pgShutDown();
			pspAudioEnd();
//			scePowerSetClockFrequency(222, 222, 111);
			reset_background();
			sceKernelExitGame();			
		}
	}
	if (done)
	{
		pgShutDown();
		pspAudioEnd();
//		scePowerSetClockFrequency(222, 222, 111);
		reset_background();
		sceKernelExitGame();
	}
}

void draw_select_type(media_type current)
{
	int x, y;
	const char * txt;
	int len;
	unsigned int selected, unselected;
	my_rect area;
	unsigned int *screen_buffer = (unsigned int *)pgGetVramAddr(0,0);

	if (current == MEDIA_VIDEO)
	{
		selected = MY_COLOR_RED;
		unselected = MY_COLOR_LIGHT_GRAY;
	}
	else
	{
		selected = MY_COLOR_LIGHT_GRAY;
		unselected = MY_COLOR_RED;
	}

	if (background)
	{
		my_copy_bmp(screen_buffer, LINESIZE, SCREEN_HEIGHT, 0, 0, background);
	}
	else
	{
		area.x = 0;
		area.y = 0;
		area.w = LINESIZE;
		area.h = SCREEN_HEIGHT;
		my_clear_area(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK);
	}

	area.x = 50;
	area.y = 50;
	area.w = (SCREEN_WIDTH - 100);
	area.h = (SCREEN_HEIGHT - 100);

	my_clear_area_alpha(screen_buffer, LINESIZE, SCREEN_HEIGHT, area, MY_COLOR_BLACK, 126);

	area.x = 0;
	area.y = 0;
	area.w = LINESIZE;
	area.h = SCREEN_HEIGHT;

 	y = (SCREEN_HEIGHT - NORMAL_FONT_HEIGHT * 2) / 2;

 	txt = mytxt_get_string(MY_TXT_VIDEO, language);
 	len = strlen((const char *)txt);
 	my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, selected, area);

 	y += 20;

 	txt = mytxt_get_string(MY_TXT_MUSIC, language);
 	len = strlen((const char *)txt);
 	my_draw_aligned_text(screen_buffer, LINESIZE, y, txt, len, ALIGN_MIDDLE, unselected, area);
 	disp_copy(screen_buffer, LINESIZE, 0, 0, LINESIZE, SCREEN_HEIGHT, 0, 0);
 	disp_update();
}

media_type select_type()
{
	unsigned long key = 0;
	static unsigned long old_key = 0;
	SceCtrlData pad;

	media_type type = MEDIA_VIDEO; // default
	
	draw_select_type(type);
	while (1)
	{
		if (done)
			break;
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons != old_key)
		{
			if (old_key)
			{
				key = old_key;
				old_key = 0;
			}
			else
			{
				key = 0;
				old_key = pad.Buttons;
			}					
		}
		if (key & PSP_CTRL_UP)
		{
			if (type == MEDIA_VIDEO)
			{
				key = 0;
			}
			else
			{
				type = MEDIA_VIDEO;
				draw_select_type(type);
				key = 0;
			}
		}
		if (key & PSP_CTRL_DOWN)
		{
			if (type == MEDIA_MUSIC)
			{
				key = 0;
			}
			else
			{
				type = MEDIA_MUSIC;
				draw_select_type(type);
				key = 0;
			}
		}
		if (key & PSP_CTRL_CIRCLE)
		{
			return type;
		}
	}
	if (done)
	{
		pgShutDown();
		pspAudioEnd();
//		scePowerSetClockFrequency(222, 222, 111);
		reset_background();
		sceKernelExitGame();
	}
	return type;	  
}

char * select_file(media_type type)
{
	char *filename = NULL;
	unsigned long key = 0;
	static unsigned long old_key = 0;
	SceCtrlData pad;

    last_start_idx = -1;
    last_media_idx = 0;
    start_idx = 0;
    media_idx = 0;
    list_end = -1;
	if (media_list_init(type))
	{
		draw_media_list(last_start_idx, start_idx, last_media_idx, media_idx, list_end);
		disp_update();
	}
	else
	{
		if (type == MEDIA_VIDEO)
		{
			const char *txt;
			txt = mytxt_get_string(MY_TXT_NO_VIDEO_FILE, language);
			msg_box(txt);
		}
		else
		{
			const char *txt;
			txt = mytxt_get_string(MY_TXT_NO_MUSIC_FILE, language);
			msg_box(txt);
		}
		return filename;
	}
	while (1)
	{
		if (done)
			break;

		sceCtrlReadBufferPositive(&pad, 1);
/*
		if (pad.Buttons != old_key)
		{
			if (old_key)
			{
				key = old_key;
				old_key = 0;
			}
			else
			{
				key = 0;
				old_key = pad.Buttons;
			}					
		}
*/
		key = pad.Buttons;

		if (key & PSP_CTRL_UP)
		{
			if ((media_idx - 1) < 0)
			{
			  last_media_idx = media_idx;
			  media_idx = list_end;
			  last_start_idx = start_idx;
		      if (list_end >= ITEM_NUM)
			  {
			    start_idx = list_end - ITEM_NUM + 1;
			  }
			  else
			  {
			    start_idx = 0;
			  }
			}
			else if ((media_idx - 1) < start_idx)
			{
			  last_media_idx = media_idx;
		      media_idx--;
			  last_start_idx = start_idx;
		      start_idx = media_idx;
		    }
			else
			{
			  last_media_idx = media_idx;
			  media_idx--;
		      last_start_idx = start_idx;
			}
			draw_media_list(last_start_idx, start_idx, last_media_idx, media_idx, list_end);
			disp_update();
			key = 0;
		}
		if (key & PSP_CTRL_DOWN)
		{
		 	if ((media_idx + 1) > list_end)
		 	{			  
		 	  last_media_idx = media_idx;
		 	  media_idx = 0;
		 	  last_start_idx = start_idx;
		 	  start_idx = 0;
		 	}
		 	else if ((media_idx + 1) >= start_idx + ITEM_NUM)
		 	{
		 	  last_media_idx = media_idx;
		       media_idx++;
		 	  last_start_idx = start_idx;
		       start_idx = media_idx - ITEM_NUM + 1;
		     }
		 	else
		 	{
		 	  last_media_idx = media_idx;
		 	  media_idx++;
		       last_start_idx = start_idx;
		 	}
		 	draw_media_list(last_start_idx, start_idx, last_media_idx, media_idx, list_end);
		 	disp_update();
			key = 0;
		}
		if (key & PSP_CTRL_CIRCLE)
		{
			/* ?????? ???????? copy???? ?????? ???? */
			if (type == MEDIA_VIDEO)
				strcpy(FileName, "ms0:/PSP/VIDEO/");
			else
				strcpy(FileName, "ms0:/PSP/MUSIC/");
			pstrcat(FileName, MAXPATHLEN, media_file_list[media_idx].d_name);
			filename = FileName;
			key = 0;
			break;
		}
		if (key & PSP_CTRL_TRIANGLE)
		{
			key = 0;
			return NULL;
		}
		if (key & PSP_CTRL_START)
		{
			key = 0;
			pgShutDown();
			pspAudioEnd();
//			scePowerSetClockFrequency(222, 222, 111);
			reset_background();
			sceKernelExitGame();
		}
		sceKernelDelayThread(100000);
	}
	if (done)
	{
		pgShutDown();
		pspAudioEnd();
//		scePowerSetClockFrequency(222, 222, 111);
		reset_background();
		sceKernelExitGame();
	}
	return filename;
}

void set_disp_mode(VideoState *is, screen_mode mode)
{
	double ratio;
	int width, height;

//	width = is->video_st->codec.width;
//	height = is->video_st->codec.height;
	width = is->video_st->codec->width;
	height = is->video_st->codec->height;
	switch(mode)
	{
		case FIT_MODE:
			ratio = (double)272.0f / (double)height;
			disp_w = width * ratio;
			disp_h = 272;
			break;
		case FULL_SCREEN:
			disp_w = 480;
			disp_h = 272;
			break;
		case NORMAL_MODE:
		default:
			disp_w = width;
			disp_h = height;
			break;
	}
	if (disp_w > 480)
		disp_w = 480;
}

int get_prev_file(media_type type)
{
	if ((media_idx - 1) < 0)
	{
	  last_media_idx = media_idx;
	  media_idx = list_end;
	  last_start_idx = start_idx;
       if (list_end >= ITEM_NUM)
	  {
	    start_idx = list_end - ITEM_NUM + 1;
	  }
	  else
	  {
	    start_idx = 0;
	  }
	}
	else if ((media_idx - 1) < start_idx)
	{
	  last_media_idx = media_idx;
       media_idx--;
	  last_start_idx = start_idx;
       start_idx = media_idx;
     }
	else
	{
	  last_media_idx = media_idx;
	  media_idx--;
       last_start_idx = start_idx;
	}
	if (last_media_idx == media_idx)
		return 0;
	if (type == MEDIA_VIDEO)
		strcpy(FileName, "ms0:/PSP/VIDEO/");
	else
		strcpy(FileName, "ms0:/PSP/MUSIC/");
	pstrcat(FileName, MAXPATHLEN, media_file_list[media_idx].d_name);
	return 1;
}

int get_next_file(media_type type)
{
	if ((media_idx + 1) > list_end)
	{			  
	  last_media_idx = media_idx;
	  media_idx = 0;
	  last_start_idx = start_idx;
	  start_idx = 0;
	}
	else if ((media_idx + 1) >= start_idx + ITEM_NUM)
	{
	  last_media_idx = media_idx;
        media_idx++;
	  last_start_idx = start_idx;
        start_idx = media_idx - ITEM_NUM + 1;
      }
	else
	{
	  last_media_idx = media_idx;
	  media_idx++;
        last_start_idx = start_idx;
	}
	if (last_media_idx == media_idx)
		return 0;
	if (type == MEDIA_VIDEO)
		strcpy(FileName, "ms0:/PSP/VIDEO/");
	else
		strcpy(FileName, "ms0:/PSP/MUSIC/");
	pstrcat(FileName, MAXPATHLEN, media_file_list[media_idx].d_name);
	return 1;
}

/* Called from the main */
int main(int argc, char **argv)
//int SDL_main(int argc, char **argv)
{
//    int flags, w, h;
	media_type type;
	const char *txt;

	pspDebugScreenInit();
	SetupCallbacks();
	pspDebugInstallErrorHandler(MyExceptionHandler);

	if (!init_font())
	{
		printf("Failed to font initialization!!!");
		sceKernelExitGame();
		return 1;
	}
	init_background();
		    
//	scePowerSetClockFrequency(333, 333, 166);
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	pspAudioInit();

    /* register all codecs, demux and protocols */
    av_register_all();

//	video_disable = 1;
//	audio_disable = 1;

//    if (display_disable) {
//        video_disable = 1;
//    }



	p_me_struct = me_struct_init();               // [jonny]
	me_startproc((u32) me_function, p_me_struct); // [jonny]



	pgInit();
	while(1)
	{
		if (done)
			break;
		greeting();
		type = select_type();
		input_filename = select_file(type);
		if (input_filename == NULL)
			continue;
		clear_screen();
		disp_update();
		clear_screen();
		disp_update();

		txt = mytxt_get_string(MY_TXT_LOADING, language);
		show_info_box(txt);

    	cur_stream = stream_open(input_filename, file_iformat);
		event_loop();
	}
	reset_background();
    return 0;
}
