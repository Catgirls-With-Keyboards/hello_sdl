#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>


// cc sdl_ffmpeg.c -lavcodec -lavformat -lavutil -lswscale -lSDL2 -lSDL2_ttf -lSDL2_mixer -lSDL2_image

int main(int argc, char* argv[]) {
    if (argc < 2)
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]), exit(1);

    const char* filename = argv[1];

    // Initialize FFmpeg
    avformat_network_init();

    // Open the video file
    AVFormatContext* formatContext = NULL;
    if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(1);
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    // Find the first video stream
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        exit(1);
    }

    // Get a pointer to the codec context for the video stream
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        exit(1);
    }

    // Open codec
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        fprintf(stderr, "Could not copy codec context\n");
        exit(1);
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "SDL Video Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        codecContext->width, codecContext->height,
        SDL_WINDOW_OPENGL
    );
    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        exit(1);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        codecContext->width, codecContext->height
    );

    // Prepare to read frames
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameYUV = av_frame_alloc();
    if (!frame || !frameYUV) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    if (AV_TIME_BASE != 1000000) {
        fprintf(stderr, "AV_TIME_BASE != 1000000\n");
        exit(1);
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height, 32);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer, AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height, 1);

    struct SwsContext* sws_ctx = sws_getContext(
        codecContext->width,
        codecContext->height,
        codecContext->pix_fmt,
        codecContext->width,
        codecContext->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    AVRational time_base_q = {1, AV_TIME_BASE};
    int64_t last_pts = 0;

    // Main loop
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    // Calculate delay, considering the previous frame's delay
                    // Current frame time in microseconds
                    int64_t pts = frame->pts != AV_NOPTS_VALUE ? frame->pts : 0;
                    if (last_pts > 0) {
                        int64_t delay_microseconds = pts - last_pts;
                        if (delay_microseconds > 0) {
                            int64_t delay_miliseconds = delay_microseconds / 1000;
                            SDL_Delay(delay_miliseconds);
                        }
                    }
                    last_pts = pts;

                    sws_scale(
                        sws_ctx,
                        (uint8_t const* const*)frame->data,
                        frame->linesize,
                        0,
                        codecContext->height,
                        frameYUV->data,
                        frameYUV->linesize
                    );

                    SDL_UpdateYUVTexture(
                        texture,
                        NULL,
                        frameYUV->data[0], frameYUV->linesize[0],
                        frameYUV->data[1], frameYUV->linesize[1],
                        frameYUV->data[2], frameYUV->linesize[2]
                    );

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);
                }
            }
        }
        av_packet_unref(packet);
    }

    // Free resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    av_free(buffer);
    av_frame_free(&frameYUV);
    av_frame_free(&frame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);

    return 0;
}
