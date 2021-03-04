#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
} ImageData;

AVCodec         *pCodec = NULL;
AVCodecContext  *pCodecCtx = NULL;
AVFrame         *pFrame = NULL;

void destroy(void)
{
  av_free(pCodec);
  avcodec_close(pCodecCtx);
  av_frame_free(&pFrame);
}

int init() {
  av_register_all();

  /* find the h264 video decoder */
  pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!pCodec) {
    fprintf(stderr, "codec not found\n");
  }
  pCodecCtx = avcodec_alloc_context3(pCodec);

  //初始化参数，下面的参数应该由具体的业务决定  
  //pCodecCtx->time_base.num = 1;
  //pCodecCtx->frame_number = 1; //每包一个视频帧  
  //pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  //pCodecCtx->bit_rate = 0;
  //pCodecCtx->time_base.den = 30;//帧率  
  // pCodecCtx->width = 320;//视频宽  
  // pCodecCtx->height = 240;//视频高  

  /* open the coderc */
  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    fprintf(stderr, "could not open codec\n");
  }
  // Allocate video frame  
  pFrame = av_frame_alloc();
  if (pFrame == NULL)
    return -1;

  return 0;
}

AVFrame *initAVFrame(uint8_t **frameBuffer) {
  AVFrame *pFrameRGB = av_frame_alloc();
  if (pFrameRGB == NULL) {
      return NULL;
  }

  int numBytes;
  numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

  *frameBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, *frameBuffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

  return pFrameRGB;
}

// 读取帧数据并返回 uint8 buffer
uint8_t *getFrameBuffer(AVFrame *frame) {
  int width = pCodecCtx->width;
  int height = pCodecCtx->height;

  uint8_t *buffer = (uint8_t *)malloc(height * width * 3);
  for (int y = 0; y < height; y++) {
      memcpy(buffer + y * frame->linesize[0], frame->data[0] + y * frame->linesize[0], width * 3);
  }
  return buffer;
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

  if (pCodecCtx->width == 0 || pCodecCtx->height == 0) {
    fprintf(stderr, "not get an image\n");
    av_packet_unref(&packet);
    return NULL;
  }

  ImageData *imageData = NULL;

  uint8_t *frameBuffer;
  AVFrame *pFrameRGB = initAVFrame(&frameBuffer);

  struct SwsContext *sws_ctx = NULL;
  sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
  if (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
    sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
    
    if (pFrameRGB == NULL) {
      fprintf(stderr, "readAVFrame failed\n");
    } else {
      imageData = (ImageData *)malloc(sizeof(ImageData));
      imageData->width = (uint32_t)pCodecCtx->width;
      imageData->height = (uint32_t)pCodecCtx->height;
      imageData->data = getFrameBuffer(pFrameRGB);
    }
  }
  av_packet_unref(&packet);
  av_free(frameBuffer);
  av_frame_free(&pFrameRGB);
  sws_freeContext(sws_ctx);

  return imageData;
}

