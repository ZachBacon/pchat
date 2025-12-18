/* PChat Audio Player Plugin - Core Implementation
 * Copyright (C) 2025
 *
 * Audio player implementation using FFmpeg for decoding and FAudio for output
 */

#include "audioplayer.h"
#include "playlist_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <FAudio.h>

/* Internal structures */
typedef struct {
    AVFormatContext *format_ctx;
    AVCodecContext *codec_ctx;
    SwrContext *swr_ctx;
    int audio_stream_index;
} FFmpegContext;

typedef struct {
    FAudio *audio;
    FAudioMasteringVoice *mastering_voice;
    FAudioSourceVoice *source_voice;
} FAudioContext;

/* Helper function prototypes */
static void* playback_thread_func(void *arg);
static int init_ffmpeg_context(FFmpegContext *ctx, const char *filepath);
static void cleanup_ffmpeg_context(FFmpegContext *ctx);
static int init_faudio_context(FAudioContext *ctx);
static void cleanup_faudio_context(FAudioContext *ctx);
static PlaylistItem* create_playlist_item(const char *filepath);
static void free_playlist_item(PlaylistItem *item);

/* Create a new audio player instance */
AudioPlayer* audioplayer_create(void) {
    AudioPlayer *player = malloc(sizeof(AudioPlayer));
    if (!player) {
        return NULL;
    }
    
    memset(player, 0, sizeof(AudioPlayer));
    
    player->state = STATE_STOPPED;
    player->loop_playlist = false;
    player->shuffle = false;
    player->volume = 1.0f;  /* Default: 100% volume */
    
    pthread_mutex_init(&player->lock, NULL);
    
    return player;
}

/* Destroy audio player instance */
void audioplayer_destroy(AudioPlayer *player) {
    if (!player) {
        return;
    }
    
    audioplayer_stop(player);
    audioplayer_clear_playlist(player);
    
    pthread_mutex_destroy(&player->lock);
    free(player);
}

/* Internal: Stop playback (assumes lock is held) */
static void audioplayer_stop_locked(AudioPlayer *player) {
    if (player->state != STATE_STOPPED) {
        player->state = STATE_STOPPED;
        pthread_mutex_unlock(&player->lock);
        
        /* Wait for playback thread to finish */
        pthread_join(player->playback_thread, NULL);
        
        pthread_mutex_lock(&player->lock);
        
        if (player->current_track && !player->playlist_head) {
            /* Free standalone track */
            free_playlist_item(player->current_track);
            player->current_track = NULL;
        }
    }
}

/* Play a file immediately */
int audioplayer_play(AudioPlayer *player, const char *filepath) {
    if (!player || !filepath) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    /* Stop current playback if any */
    if (player->state != STATE_STOPPED) {
        audioplayer_stop_locked(player);
    }
    
    /* Create playlist item */
    PlaylistItem *item = create_playlist_item(filepath);
    if (!item) {
        pthread_mutex_unlock(&player->lock);
        return -1;
    }
    
    player->current_track = item;
    player->state = STATE_PLAYING;
    
    /* Start playback thread */
    if (pthread_create(&player->playback_thread, NULL, playback_thread_func, player) != 0) {
        free_playlist_item(item);
        player->current_track = NULL;
        player->state = STATE_STOPPED;
        pthread_mutex_unlock(&player->lock);
        return -1;
    }
    
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Pause playback */
int audioplayer_pause(AudioPlayer *player) {
    if (!player) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    if (player->state == STATE_PLAYING) {
        player->state = STATE_PAUSED;
        /* FAudio pause would be called here */
        if (player->faudio_device) {
            FAudioContext *fa_ctx = (FAudioContext*)player->faudio_device;
            if (fa_ctx->source_voice) {
                FAudioSourceVoice_Stop(fa_ctx->source_voice, 0, FAUDIO_COMMIT_NOW);
            }
        }
    }
    
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Resume playback */
int audioplayer_resume(AudioPlayer *player) {
    if (!player) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    if (player->state == STATE_PAUSED) {
        player->state = STATE_PLAYING;
        /* FAudio resume would be called here */
        if (player->faudio_device) {
            FAudioContext *fa_ctx = (FAudioContext*)player->faudio_device;
            if (fa_ctx->source_voice) {
                FAudioSourceVoice_Start(fa_ctx->source_voice, 0, FAUDIO_COMMIT_NOW);
            }
        }
    }
    
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Stop playback */
int audioplayer_stop(AudioPlayer *player) {
    if (!player) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    audioplayer_stop_locked(player);
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Play next track in playlist */
int audioplayer_next(AudioPlayer *player) {
    if (!player || !player->current_track) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    PlaylistItem *next = player->current_track->next;
    if (!next && player->loop_playlist) {
        next = player->playlist_head;
    }
    
    if (next) {
        bool was_playing = (player->state == STATE_PLAYING);
        pthread_mutex_unlock(&player->lock);
        
        audioplayer_stop(player);
        
        pthread_mutex_lock(&player->lock);
        player->current_track = next;
        pthread_mutex_unlock(&player->lock);
        
        if (was_playing) {
            audioplayer_play(player, next->filepath);
        }
        return 0;
    }
    
    pthread_mutex_unlock(&player->lock);
    return -1;
}

/* Play previous track in playlist */
int audioplayer_prev(AudioPlayer *player) {
    if (!player || !player->current_track) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    PlaylistItem *prev = player->current_track->prev;
    if (!prev && player->loop_playlist) {
        prev = player->playlist_tail;
    }
    
    if (prev) {
        bool was_playing = (player->state == STATE_PLAYING);
        pthread_mutex_unlock(&player->lock);
        
        audioplayer_stop(player);
        
        pthread_mutex_lock(&player->lock);
        player->current_track = prev;
        pthread_mutex_unlock(&player->lock);
        
        if (was_playing) {
            audioplayer_play(player, prev->filepath);
        }
        return 0;
    }
    
    pthread_mutex_unlock(&player->lock);
    return -1;
}

/* Add file to playlist */
int audioplayer_add_to_playlist(AudioPlayer *player, const char *filepath) {
    if (!player || !filepath) {
        return -1;
    }
    
    PlaylistItem *item = create_playlist_item(filepath);
    if (!item) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    if (!player->playlist_head) {
        player->playlist_head = item;
        player->playlist_tail = item;
    } else {
        player->playlist_tail->next = item;
        item->prev = player->playlist_tail;
        player->playlist_tail = item;
    }
    
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Clear entire playlist */
int audioplayer_clear_playlist(AudioPlayer *player) {
    if (!player) {
        return -1;
    }
    
    pthread_mutex_lock(&player->lock);
    
    PlaylistItem *item = player->playlist_head;
    while (item) {
        PlaylistItem *next = item->next;
        free_playlist_item(item);
        item = next;
    }
    
    player->playlist_head = NULL;
    player->playlist_tail = NULL;
    player->current_track = NULL;
    
    pthread_mutex_unlock(&player->lock);
    return 0;
}

/* Load playlist from file (.m3u, .m3u8, .pls) */
int audioplayer_load_playlist_file(AudioPlayer *player, const char *playlist_file) {
    if (!player || !playlist_file) {
        return -1;
    }
    
    /* Parse the playlist file */
    PlaylistParseResult *result = parse_playlist_file(playlist_file);
    if (!result) {
        return -1;
    }
    
    /* Clear existing playlist */
    audioplayer_clear_playlist(player);
    
    /* Add all files from the parsed playlist */
    int added = 0;
    for (int i = 0; i < result->count; i++) {
        if (audioplayer_add_to_playlist(player, result->files[i]) == 0) {
            added++;
        }
    }
    
    free_playlist_result(result);
    
    /* Return count even if 0 - let caller distinguish between parse error and no valid files */
    return added >= 0 ? added : -1;
}

/* Get playlist */
PlaylistItem* audioplayer_get_playlist(AudioPlayer *player) {
    if (!player) {
        return NULL;
    }
    return player->playlist_head;
}

/* Get playlist count */
int audioplayer_get_playlist_count(AudioPlayer *player) {
    if (!player) {
        return 0;
    }
    
    pthread_mutex_lock(&player->lock);
    
    int count = 0;
    PlaylistItem *item = player->playlist_head;
    while (item) {
        count++;
        item = item->next;
    }
    
    pthread_mutex_unlock(&player->lock);
    return count;
}

/* Get player state */
PlayerState audioplayer_get_state(AudioPlayer *player) {
    if (!player) {
        return STATE_STOPPED;
    }
    return player->state;
}

/* Get current track */
PlaylistItem* audioplayer_get_current_track(AudioPlayer *player) {
    if (!player) {
        return NULL;
    }
    return player->current_track;
}

/* Set loop mode */
void audioplayer_set_loop(AudioPlayer *player, bool loop) {
    if (player) {
        pthread_mutex_lock(&player->lock);
        player->loop_playlist = loop;
        pthread_mutex_unlock(&player->lock);
    }
}

/* Set shuffle mode */
void audioplayer_set_shuffle(AudioPlayer *player, bool shuffle) {
    if (player) {
        pthread_mutex_lock(&player->lock);
        player->shuffle = shuffle;
        pthread_mutex_unlock(&player->lock);
    }
}

/* Helper Functions */

static PlaylistItem* create_playlist_item(const char *filepath) {
    PlaylistItem *item = malloc(sizeof(PlaylistItem));
    if (!item) {
        return NULL;
    }
    
    memset(item, 0, sizeof(PlaylistItem));
    item->filepath = strdup(filepath);
    
    /* Extract title from filepath */
    const char *filename = strrchr(filepath, '/');
    if (!filename) {
        filename = strrchr(filepath, '\\');
    }
    filename = filename ? filename + 1 : filepath;
    
    item->title = strdup(filename);
    item->duration = 0; /* Will be filled when decoded */
    
    return item;
}

static void free_playlist_item(PlaylistItem *item) {
    if (!item) {
        return;
    }
    
    free(item->filepath);
    free(item->title);
    free(item);
}

static int init_ffmpeg_context(FFmpegContext *ctx, const char *filepath) {
    memset(ctx, 0, sizeof(FFmpegContext));
    ctx->audio_stream_index = -1;
    
    /* Open input file */
    if (avformat_open_input(&ctx->format_ctx, filepath, NULL, NULL) < 0) {
        fprintf(stderr, "[AudioPlayer] Failed to open file: %s\\n", filepath);
        return -1;
    }
    
    /* Retrieve stream information */
    if (avformat_find_stream_info(ctx->format_ctx, NULL) < 0) {
        fprintf(stderr, "[AudioPlayer] Failed to find stream info\\n");
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    /* Find audio stream */
    for (unsigned int i = 0; i < ctx->format_ctx->nb_streams; i++) {
        if (ctx->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            ctx->audio_stream_index = i;
            break;
        }
    }
    
    if (ctx->audio_stream_index == -1) {
        fprintf(stderr, "[AudioPlayer] No audio stream found\\n");
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    /* Get codec */
    AVCodecParameters *codecpar = ctx->format_ctx->streams[ctx->audio_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "[AudioPlayer] Codec not found (codec_id=%d)\\n", codecpar->codec_id);
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    fprintf(stderr, "[AudioPlayer] Found codec: %s\\n", codec->name);
    
    /* Allocate codec context */
    ctx->codec_ctx = avcodec_alloc_context3(codec);
    if (!ctx->codec_ctx) {
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    /* Copy codec parameters */
    if (avcodec_parameters_to_context(ctx->codec_ctx, codecpar) < 0) {
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    /* Open codec */
    if (avcodec_open2(ctx->codec_ctx, codec, NULL) < 0) {
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    /* Initialize resampler for converting to stereo 16-bit PCM at 48kHz */
    /* Use newer FFmpeg API (4.0+) */
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    int ret = swr_alloc_set_opts2(&ctx->swr_ctx,
        &out_ch_layout, AV_SAMPLE_FMT_S16, 48000,
        &ctx->codec_ctx->ch_layout, ctx->codec_ctx->sample_fmt, ctx->codec_ctx->sample_rate,
        0, NULL);
    
    if (ret < 0 || !ctx->swr_ctx || swr_init(ctx->swr_ctx) < 0) {
        if (ctx->swr_ctx) swr_free(&ctx->swr_ctx);
        avcodec_free_context(&ctx->codec_ctx);
        avformat_close_input(&ctx->format_ctx);
        return -1;
    }
    
    return 0;
}

static void cleanup_ffmpeg_context(FFmpegContext *ctx) {
    if (ctx->swr_ctx) {
        swr_free(&ctx->swr_ctx);
    }
    if (ctx->codec_ctx) {
        avcodec_free_context(&ctx->codec_ctx);
    }
    if (ctx->format_ctx) {
        avformat_close_input(&ctx->format_ctx);
    }
}

/* Callback to free buffer after FAudio is done with it */
static void FAUDIOCALL buffer_end_callback(FAudioVoiceCallback *callback, void *pBufferContext) {
    (void)callback;
    if (pBufferContext) {
        free(pBufferContext);
    }
}

static FAudioVoiceCallback voice_callbacks = {
    .OnBufferEnd = buffer_end_callback,
    .OnBufferStart = NULL,
    .OnLoopEnd = NULL,
    .OnStreamEnd = NULL,
    .OnVoiceError = NULL,
    .OnVoiceProcessingPassEnd = NULL,
    .OnVoiceProcessingPassStart = NULL
};

static int init_faudio_context(FAudioContext *ctx) {
    memset(ctx, 0, sizeof(FAudioContext));
    
    /* Create FAudio instance */
    if (FAudioCreate(&ctx->audio, 0, FAUDIO_DEFAULT_PROCESSOR) != 0) {
        return -1;
    }
    
    /* Create mastering voice */
    if (FAudio_CreateMasteringVoice(ctx->audio, &ctx->mastering_voice,
        FAUDIO_DEFAULT_CHANNELS, FAUDIO_DEFAULT_SAMPLERATE, 0, 0, NULL) != 0) {
        FAudio_Release(ctx->audio);
        return -1;
    }
    
    /* Create source voice (stereo 16-bit PCM at 48kHz) with callbacks */
    FAudioWaveFormatEx waveformat = {
        .wFormatTag = FAUDIO_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 48000,
        .wBitsPerSample = 16,
        .nBlockAlign = 4,
        .nAvgBytesPerSec = 192000,
        .cbSize = 0
    };
    
    if (FAudio_CreateSourceVoice(ctx->audio, &ctx->source_voice, &waveformat,
        0, FAUDIO_DEFAULT_FREQ_RATIO, &voice_callbacks, NULL, NULL) != 0) {
        FAudioVoice_DestroyVoice(ctx->mastering_voice);
        FAudio_Release(ctx->audio);
        return -1;
    }
    
    return 0;
}

static void cleanup_faudio_context(FAudioContext *ctx) {
    if (ctx->source_voice) {
        FAudioSourceVoice_Stop(ctx->source_voice, 0, FAUDIO_COMMIT_NOW);
        FAudioVoice_DestroyVoice(ctx->source_voice);
    }
    if (ctx->mastering_voice) {
        FAudioVoice_DestroyVoice(ctx->mastering_voice);
    }
    if (ctx->audio) {
        FAudio_Release(ctx->audio);
    }
}

/* Playback thread function */
static void* playback_thread_func(void *arg) {
    AudioPlayer *player = (AudioPlayer*)arg;
    
    pthread_mutex_lock(&player->lock);
    const char *filepath = player->current_track ? player->current_track->filepath : NULL;
    pthread_mutex_unlock(&player->lock);
    
    if (!filepath) {
        return NULL;
    }
    
    FFmpegContext ffmpeg_ctx;
    FAudioContext faudio_ctx;
    
    /* Initialize contexts */
    if (init_ffmpeg_context(&ffmpeg_ctx, filepath) < 0) {
        fprintf(stderr, "[AudioPlayer] Failed to initialize FFmpeg context for: %s\n", filepath);
        pthread_mutex_lock(&player->lock);
        player->state = STATE_STOPPED;
        pthread_mutex_unlock(&player->lock);
        return NULL;
    }
    fprintf(stderr, "[AudioPlayer] FFmpeg initialized successfully\n");
    
    if (init_faudio_context(&faudio_ctx) < 0) {
        fprintf(stderr, "[AudioPlayer] Failed to initialize FAudio context\n");
        cleanup_ffmpeg_context(&ffmpeg_ctx);
        pthread_mutex_lock(&player->lock);
        player->state = STATE_STOPPED;
        pthread_mutex_unlock(&player->lock);
        return NULL;
    }
    fprintf(stderr, "[AudioPlayer] FAudio initialized successfully\n");
    
    player->ffmpeg_ctx = &ffmpeg_ctx;
    player->faudio_device = &faudio_ctx;
    
    /* Set volume */
    FAudioVoice_SetVolume((FAudioVoice*)faudio_ctx.source_voice, player->volume, FAUDIO_COMMIT_NOW);
    
    /* Start playback */
    fprintf(stderr, "[AudioPlayer] Starting playback...\n");
    FAudioSourceVoice_Start(faudio_ctx.source_voice, 0, FAUDIO_COMMIT_NOW);
    
    /* Decode and play loop */
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    uint8_t *audio_buffer = malloc(192000); /* 1 second buffer */
    
    while (av_read_frame(ffmpeg_ctx.format_ctx, packet) >= 0) {
        pthread_mutex_lock(&player->lock);
        PlayerState state = player->state;
        pthread_mutex_unlock(&player->lock);
        
        if (state == STATE_STOPPED) {
            break;
        }
        
        /* Wait while paused */
        while (state == STATE_PAUSED) {
            usleep(10000);
            pthread_mutex_lock(&player->lock);
            state = player->state;
            pthread_mutex_unlock(&player->lock);
        }
        
        if (packet->stream_index == ffmpeg_ctx.audio_stream_index) {
            if (avcodec_send_packet(ffmpeg_ctx.codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(ffmpeg_ctx.codec_ctx, frame) == 0) {
                    /* Resample audio */
                    int out_samples = swr_convert(ffmpeg_ctx.swr_ctx,
                        &audio_buffer, frame->nb_samples,
                        (const uint8_t**)frame->data, frame->nb_samples);
                    
                    if (out_samples > 0) {
                        size_t buffer_size = out_samples * 4; /* 2 channels * 16-bit */
                        
                        /* Copy buffer data - FAudio will keep the pointer */
                        uint8_t *buffer_copy = malloc(buffer_size);
                        memcpy(buffer_copy, audio_buffer, buffer_size);
                        
                        /* Submit audio buffer to FAudio */
                        FAudioBuffer buffer = {
                            .AudioBytes = buffer_size,
                            .pAudioData = buffer_copy,
                            .PlayBegin = 0,
                            .PlayLength = 0,
                            .LoopBegin = 0,
                            .LoopLength = 0,
                            .LoopCount = 0,
                            .pContext = buffer_copy  /* Store pointer so we can free it */
                        };
                        
                        FAudioSourceVoice_SubmitSourceBuffer(faudio_ctx.source_voice, &buffer, NULL);
                        
                        /* Wait if too many buffers are queued */
                        FAudioVoiceState voice_state;
                        do {
                            FAudioSourceVoice_GetState(faudio_ctx.source_voice, &voice_state, 0);
                            if (voice_state.BuffersQueued > 4) {
                                usleep(5000);
                            }
                        } while (voice_state.BuffersQueued > 4);
                    }
                }
            }
        }
        
        av_packet_unref(packet);
    }
    
    /* Wait for remaining buffers to play */
    FAudioVoiceState voice_state;
    do {
        FAudioSourceVoice_GetState(faudio_ctx.source_voice, &voice_state, 0);
        if (voice_state.BuffersQueued > 0) {
            usleep(10000);
        }
    } while (voice_state.BuffersQueued > 0);
    
    /* Cleanup */
    free(audio_buffer);
    av_frame_free(&frame);
    av_packet_free(&packet);
    
    cleanup_faudio_context(&faudio_ctx);
    cleanup_ffmpeg_context(&ffmpeg_ctx);
    
    pthread_mutex_lock(&player->lock);
    player->ffmpeg_ctx = NULL;
    player->faudio_device = NULL;
    
    /* Auto-play next track if in playlist mode */
    if (player->state != STATE_STOPPED && player->current_track && player->current_track->next) {
        PlaylistItem *next = player->current_track->next;
        player->current_track = next;
        pthread_mutex_unlock(&player->lock);
        audioplayer_play(player, next->filepath);
    } else {
        player->state = STATE_STOPPED;
        pthread_mutex_unlock(&player->lock);
    }
    
    return NULL;
}

/* Set volume (0.0 to 1.0) */
void audioplayer_set_volume(AudioPlayer *player, float volume) {
    if (!player) {
        return;
    }
    
    /* Clamp volume to valid range */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    pthread_mutex_lock(&player->lock);
    player->volume = volume;
    
    /* Update FAudio volume if currently playing */
    if (player->faudio_device) {
        FAudioContext *fa_ctx = (FAudioContext*)player->faudio_device;
        if (fa_ctx->source_voice) {
            FAudioVoice_SetVolume((FAudioVoice*)fa_ctx->source_voice, volume, FAUDIO_COMMIT_NOW);
        }
    }
    
    pthread_mutex_unlock(&player->lock);
}

/* Get current volume */
float audioplayer_get_volume(AudioPlayer *player) {
    if (!player) {
        return 1.0f;
    }
    
    pthread_mutex_lock(&player->lock);
    float volume = player->volume;
    pthread_mutex_unlock(&player->lock);
    
    return volume;
}
