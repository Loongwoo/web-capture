#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

typedef struct
{
  uint32_t width;
  uint32_t height;
  uint32_t dataLength;
  uint8_t *data;
} ImageData;

ImageData *av_init_imagedata(int width, int height, int format)
{
  ImageData *imageData = (ImageData *)malloc(sizeof(ImageData));
  imageData->width = width;
  imageData->height = height;
  imageData->dataLength = av_image_get_buffer_size(format, width, height, 1);
  imageData->data = (uint8_t *)av_malloc(imageData->dataLength * sizeof(uint8_t));
  return imageData;
}

typedef struct
{
  uint32_t colorTransform;
} AVParams;

AVParams *av_init_params(uint8_t *buff, int buffLength)
{
  AVParams *par = (AVParams *)malloc(sizeof(AVParams));
  par->colorTransform = buffLength > 0 ? buff[0] : 0;

  printf("colorTransform: %d \n", par->colorTransform);

  return par;
}

AVParams *pParams = NULL;
AVCodec *pCodec = NULL;
AVCodecContext *pCodecCtx = NULL;
AVFrame *pFrame = NULL;
AVFrame *pFrame2 = NULL;

void destroy(void)
{
  free(pParams);
  av_free(pCodec);
  avcodec_close(pCodecCtx);
  av_frame_free(&pFrame);
  av_frame_free(&pFrame2);
}

int init(uint8_t *buff, int buffLength)
{
  pParams = av_init_params(buff, buffLength);

  /* find the h264 video decoder */
  pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!pCodec)
  {
    fprintf(stderr, "codec not found\n");
    return -1;
  }
  pCodecCtx = avcodec_alloc_context3(pCodec);

  /* open the coderc */
  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
  {
    fprintf(stderr, "could not open codec\n");
  }
  // Allocate video frame
  pFrame = av_frame_alloc();
  if (pFrame == NULL)
    return -1;

  pFrame2 = av_frame_alloc();
  if (pFrame2 == NULL)
    return -1;

  return 0;
}

ImageData *decode(uint8_t *buff, int buffLength)
{
  AVPacket packet;
  av_init_packet(&packet);
  packet.size = buffLength;
  packet.data = (uint8_t *)buff;

  if (avcodec_send_packet(pCodecCtx, &packet) != 0)
  {
    fprintf(stderr, "avcodec_send_packet failed\n");
    av_packet_unref(&packet);
    return NULL;
  }

  ImageData *imageData = NULL;
  if (avcodec_receive_frame(pCodecCtx, pFrame) == 0)
  {
    int width = pFrame->width;
    int height = pFrame->height;

    printf("width: %d, height: %d, format: %d \n", width, height, pFrame->format);

    struct SwsContext *sws_ctx = NULL;
    if (pParams->colorTransform)
    {
      imageData = av_init_imagedata(width, height, AV_PIX_FMT_RGB24);
      av_image_fill_arrays(pFrame2->data, pFrame2->linesize, imageData->data, AV_PIX_FMT_RGB24, width, height, 1);

      sws_ctx = sws_getContext(width, height, pCodecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
      sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, height, pFrame2->data, pFrame2->linesize);

      memcpy(imageData->data, pFrame2->data[0], imageData->dataLength);
    }
    else
    {
      int ysize = width * height;
      if (pFrame->format == AV_PIX_FMT_YUV420P)
      {
        imageData = av_init_imagedata(width, height, AV_PIX_FMT_YUV420P);

        memcpy(imageData->data, pFrame->data[0], ysize);
        memcpy(imageData->data + ysize, pFrame->data[1], ysize / 4);
        memcpy(imageData->data + ysize + ysize / 4, pFrame->data[2], ysize / 4);
      }
      else if (pFrame->format == AV_PIX_FMT_YUV422P)
      {
        imageData = av_init_imagedata(width, height, AV_PIX_FMT_YUV422P);

        memcpy(imageData->data, pFrame->data[0], ysize);
        memcpy(imageData->data + ysize, pFrame->data[1], ysize / 2);
        memcpy(imageData->data + ysize + ysize / 2, pFrame->data[2], ysize / 2);
      }
      else
      {
        imageData = av_init_imagedata(width, height, AV_PIX_FMT_YUV420P);
        av_image_fill_arrays(pFrame2->data, pFrame2->linesize, imageData->data, AV_PIX_FMT_YUV420P, width, height, 1);

        sws_ctx = sws_getContext(width, height, pCodecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, height, pFrame2->data, pFrame2->linesize);

        memcpy(imageData->data, pFrame2->data[0], ysize);
        memcpy(imageData->data + ysize, pFrame2->data[1], ysize / 4);
        memcpy(imageData->data + ysize + ysize / 4, pFrame2->data[2], ysize / 4);
      }
    }
    sws_freeContext(sws_ctx);
  }
  av_packet_unref(&packet);

  return imageData;
}
