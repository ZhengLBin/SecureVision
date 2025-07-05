#include "rtspthread.h"
#include <QDebug>

RtspThread::RtspThread(QObject *parent)
    : QThread(parent), m_running(false) {}

RtspThread::~RtspThread() {
    setThreadStart(false);
    wait();
}

void RtspThread::setRtspUrl(const QString &url) {
    qDebug() << "setRtspUrl in RtspThread" << url;
    m_url = url;
}

void RtspThread::setThreadStart(bool start) {
    m_running = start;
}

void RtspThread::run() {
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVCodec *codec = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *pkt = nullptr;
    SwsContext *sws_ctx = nullptr;

    avformat_network_init();

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);  // ← 关键
    av_dict_set(&opts, "stimeout", "5000000", 0);    // 超时可配置

    if (avformat_open_input(&fmt_ctx, m_url.toStdString().c_str(), nullptr, &opts) != 0) {
        qDebug() << "cant open rtSP" << m_url;
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        qDebug() << "cant find stream info";
        avformat_close_input(&fmt_ctx);
        return;
    }

    int video_stream_index = -1;
    for (int i = 0; i < static_cast<int>(fmt_ctx->nb_streams); i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }


    if (video_stream_index == -1) {
        qDebug() << "cant find video stream";
        avformat_close_input(&fmt_ctx);
        return;
    }

    codec = avcodec_find_decoder_by_name("hevc");
    if (!codec) {
        qDebug() << "cant find codec";
        avformat_close_input(&fmt_ctx);
        return;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    qDebug() << "Video stream codec:" << codec_ctx->codec_id;
    qDebug() << "Source resolution:" << codec_ctx->width << "x" << codec_ctx->height;
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        qDebug() << "cant open codec";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return;
    }
    qDebug() << "Pixel format:" << av_get_pix_fmt_name(codec_ctx->pix_fmt);

    frame = av_frame_alloc();
    pkt = av_packet_alloc();

    AVPixelFormat pix_fmt = codec_ctx->pix_fmt;
    if (pix_fmt == AV_PIX_FMT_YUVJ420P) {
        pix_fmt = AV_PIX_FMT_YUV420P;
    }

    int target_width = 1280;
    int target_height = 720;
    sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, pix_fmt,
                             target_width, target_height, AV_PIX_FMT_RGB24,
                             SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (sws_ctx) {
        sws_setColorspaceDetails(sws_ctx,
            sws_getCoefficients(SWS_CS_ITU709), 0,
            sws_getCoefficients(SWS_CS_ITU709), 0,
            0, 1 << 16, 1 << 16);
    }
    uint8_t *rgb_data[4];
    int rgb_linesize[4];
    av_image_alloc(rgb_data, rgb_linesize, target_width, target_height, AV_PIX_FMT_RGB24, 1);

    while (m_running && av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {

                    if (frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUVJ420P && frame->format != AV_PIX_FMT_NV12) {
                        continue;
                    }

                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height,
                              rgb_data, rgb_linesize);
                    QImage image(rgb_data[0], target_width, target_height, rgb_linesize[0], QImage::Format_RGB888);
                    // 添加旋转逻辑
//                    QTransform transform;
//                    transform.rotate(-270);  // 逆时针旋转90°
//                    QImage rotatedImage = image.transformed(transform);

                    emit resultReady(image.copy());
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_freep(&rgb_data[0]);
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    qDebug() << "RTSP thread stopped";
}
