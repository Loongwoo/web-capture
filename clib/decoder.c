#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
} ImageData;

AVCodec *pCodec = NULL;
AVCodecContext *pCodecCtx = NULL;
AVFrame *pFrame = NULL;
AVFrame *pFrameRGB = NULL;

void destroy(void) {
    av_free(pCodec);
    avcodec_close(pCodecCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
}

int init() {
    av_register_all();

    /* find the h264 video decoder */
    pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec) {
        fprintf(stderr, "codec not found\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);

    /* open the coderc */
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
    }
    // Allocate video frame
    pFrame = av_frame_alloc();
    if (pFrame == NULL) return -1;

    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) return -1;

    return 0;
}

ImageData *decode(uint8_t *buff, int buffLength) {
    AVPacket packet;
    av_init_packet(&packet);
    packet.size = buffLength;
    packet.data = (uint8_t *)buff;

    if (avcodec_send_packet(pCodecCtx, &packet) != 0) {
        fprintf(stderr, "avcodec_send_packet failed\n");
        av_packet_unref(&packet);
        return NULL;
    }

    int width = pCodecCtx->width;
    int height = pCodecCtx->height;

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t *frameBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, frameBuffer, AV_PIX_FMT_RGB24, width, height, 1);

    ImageData *imageData = NULL;
    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext(width, height, pCodecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
        sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, height, pFrameRGB->data, pFrameRGB->linesize);

        uint8_t *data = (uint8_t *)malloc(height * width * 3);
        for (int y = 0; y < height; y++) {
            memcpy(data + y * pFrameRGB->linesize[0], pFrameRGB->data[0] + y * pFrameRGB->linesize[0], width * 3);
        }
        imageData = (ImageData *)malloc(sizeof(ImageData));
        imageData->width = (uint32_t)width;
        imageData->height = (uint32_t)height;
        imageData->data = data;
    }
    av_packet_unref(&packet);
    av_free(frameBuffer);
    sws_freeContext(sws_ctx);

    return imageData;
}
