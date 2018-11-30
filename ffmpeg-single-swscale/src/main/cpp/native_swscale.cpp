
#include "native_swscale.h"
#include "logger.h"

int NativeSwscale::Transform(const char *videoPath, const char *outputPath) {

    // 1.注册所有组件
    av_register_all();
    // 2.创建AVFormatContext结构体
    pFormatCtx = avformat_alloc_context();

    // 3.打开一个输入文件
    if (avformat_open_input(&pFormatCtx, videoPath, NULL, NULL) != 0) {
        LOGE("could not open input stream");
        goto end_line;
    }
    // 4.获取媒体的信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("could not find stream information");
        goto end_line;
    }
    //获取视频轨的下标
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    if (videoIndex == -1) {
        LOGE("could not find a video stream");
        goto end_line;
    }
    // 5.查找解码器
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoIndex]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGE("could not find Codec");
        goto end_line;
    }

    // 6.配置解码器
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoIndex]->codecpar);
    pCodecCtx->thread_count = 1;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("could not open codec");
        goto end_line;
    }

    pFrame = av_frame_alloc();
    vPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    pFrameYUV = av_frame_alloc();


    bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,
                                          1);
    out_buffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));

    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P,
                         pCodecCtx->width, pCodecCtx->height, 1);

    //RGB24转换
    /*bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    out_buffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_RGB24,
                         pCodecCtx->width, pCodecCtx->height, 1);*/

    outputFile = fopen(outputPath, "wb+");

    while (av_read_frame(pFormatCtx, vPacket) >= 0) {
        if (vPacket->stream_index == videoIndex) {
            if (avcodec_send_packet(pCodecCtx, vPacket) != 0) {
                return -1;
            }
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                //转换
                sws_ctx = sws_getContext(pCodecCtx->width,
                                         pCodecCtx->height,
                                         pCodecCtx->pix_fmt,
                                         pCodecCtx->width,
                                         pCodecCtx->height,
                                         AV_PIX_FMT_YUV420P, //AV_PIX_FMT_RGB24
                                         SWS_BICUBIC,
                                         NULL, NULL, NULL);
                sws_scale(sws_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize,
                          0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                //YUV420P
                fwrite(pFrameYUV->data[0], (pCodecCtx->width) * (pCodecCtx->height), 1, outputFile);
                fwrite(pFrameYUV->data[1], (pCodecCtx->width) * (pCodecCtx->height) / 4, 1,
                       outputFile);
                fwrite(pFrameYUV->data[2], (pCodecCtx->width) * (pCodecCtx->height) / 4, 1,
                       outputFile);

                //RGB24
                //fwrite(pFrameYUV->data[0], (pCodecCtx->width) * (pCodecCtx->height) * 3, 1, outputFile);
                //UYVY
                //fwrite(pFrameYUV->data[0],(pCodecCtx->width)*(pCodecCtx->height),2,outputFile);
            }
        }
        av_packet_unref(vPacket);
    }

    if (outputFile != NULL) {
        fclose(outputFile);
        outputFile = NULL;
    }

    end_line:

    if (pCodecCtx != NULL) {
        avcodec_close(pCodecCtx);
        avcodec_free_context(&pCodecCtx);
        pCodecCtx = NULL;
    }
    if (pFrame != NULL) {
        av_free(pFrame);
        pFrame = NULL;
    }
    if (pFrameYUV != NULL) {
        av_free(pFrameYUV);
        pFrameYUV = NULL;
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    return 0;
}