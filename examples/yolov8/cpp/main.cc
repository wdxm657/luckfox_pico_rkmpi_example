// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"
#include "common.h"
#include "yolov8.h"
#include "rtsp_demo.h"
#include "luckfox_mpi.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#if defined(RV1106_1103)
#include "dma_alloc.hpp"
#endif

#define DISP_WIDTH 1280
#define DISP_HEIGHT 720
int width = DISP_WIDTH;
int height = DISP_HEIGHT;
// model size
int model_width = 640;
int model_height = 640;
float scale;
int leftPadding;
int topPadding;

cv::Mat letterbox(cv::Mat input)
{
    float scaleX = (float)model_width / (float)width;
    float scaleY = (float)model_height / (float)height;
    scale = scaleX < scaleY ? scaleX : scaleY;

    int inputWidth = (int)((float)width * scale);
    int inputHeight = (int)((float)height * scale);

    leftPadding = (model_width - inputWidth) / 2;
    topPadding = (model_height - inputHeight) / 2;

    cv::Mat inputScale;
    cv::resize(input, inputScale, cv::Size(inputWidth, inputHeight), 0, 0, cv::INTER_LINEAR);
    cv::Mat letterboxImage(640, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Rect roi(leftPadding, topPadding, inputWidth, inputHeight);
    inputScale.copyTo(letterboxImage(roi));

    return letterboxImage;
}

static volatile int run = 1;
static void sig_proc(int signo)
{
    printf("received signo %d\n", signo);
    run = 1;
}
void mapCoordinates(int *x, int *y)
{
    int mx = *x - leftPadding;
    int my = *y - topPadding;

    *x = (int)((float)mx / scale);
    *y = (int)((float)my / scale);
}

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    // signal(SIGTERM, sig_proc);
    // signal(SIGINT, sig_proc);
    system("RkLunch-stop.sh");
    RK_S32 s32Ret = 0;
    int sX, sY, eX, eY;
    char text[16];

    if (argc != 2)
    {
        printf("%s <model_path> \n", argv[0]);
        return -1;
    }
    char fps_text[16];
    float fps = 0;
    memset(fps_text, 0, 16);
    const char *model_path = argv[1];
    int ret;
    rknn_app_context_t rknn_app_ctx;
    object_detect_result_list od_results;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    ret = init_yolov8_model(model_path, &rknn_app_ctx);
    init_post_process();

    // h264_frame
    VENC_STREAM_S stFrame;
    stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
    RK_U32 H264_TimeRef = 0;
    VIDEO_FRAME_INFO_S stViFrame;

    // Create Pool
    MB_POOL_CONFIG_S PoolCfg;
    memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    PoolCfg.u64MBSize = width * height * 3;
    PoolCfg.u32MBCnt = 1;
    PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
    // PoolCfg.bPreAlloc = RK_FALSE;
    MB_POOL src_Pool = RK_MPI_MB_CreatePool(&PoolCfg);
    printf("Create Pool successs!\n");

    // Get MB from Pool
    MB_BLK src_Blk = RK_MPI_MB_GetMB(src_Pool, width * height * 3, RK_TRUE);

    // Build h264_frame
    VIDEO_FRAME_INFO_S h264_frame;
    h264_frame.stVFrame.u32Width = width;
    h264_frame.stVFrame.u32Height = height;
    h264_frame.stVFrame.u32VirWidth = width;
    h264_frame.stVFrame.u32VirHeight = height;
    h264_frame.stVFrame.enPixelFormat = RK_FMT_RGB888;
    h264_frame.stVFrame.u32FrameFlag = 160;
    h264_frame.stVFrame.pMbBlk = src_Blk;
    unsigned char *data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(src_Blk);
    cv::Mat frame(cv::Size(width, height), CV_8UC3, data);

    // rkaiq init
    RK_BOOL multi_sensor = RK_FALSE;
    const char *iq_dir = "/etc/iqfiles";
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    // hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
    SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
    SAMPLE_COMM_ISP_Run(0);

    // rkmpi init
    if (RK_MPI_SYS_Init() != RK_SUCCESS)
    {
        RK_LOGE("rk mpi sys init fail!");
        return -1;
    }

    // rtsp init
    rtsp_demo_handle g_rtsplive = NULL;
    rtsp_session_handle g_rtsp_session;
    g_rtsplive = create_rtsp_demo(554);
    g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
    rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

    // vi init
    vi_dev_init();
    vi_chn_init(0, width, height);

    // venc init
    RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
    venc_init(0, width, height, enCodecType);

    printf("init success\n");

    while (run)
    {
        // 获取VI帧
        h264_frame.stVFrame.u32TimeRef = H264_TimeRef++;
        h264_frame.stVFrame.u64PTS = TEST_COMM_GetNowUs();
        s32Ret = RK_MPI_VI_GetChnFrame(0, 0, &stViFrame, -1);
        if (s32Ret == RK_SUCCESS)
        {
            // 需要进行YUV420SP到RGB888的转换
            unsigned char *vi_data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            cv::Mat yuv420sp(height + height / 2, width, CV_8UC1, vi_data);
            cv::Mat bgr(height, width, CV_8UC3, data);

            cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
            cv::resize(bgr, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
            // letterbox
            cv::Mat letterboxImage = letterbox(frame);
            memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width * model_height * 3);
            inference_yolov8_model(&rknn_app_ctx, &od_results);
            for (int i = 0; i < od_results.count; i++)
            {
                if (od_results.count >= 1)
                {
                    object_detect_result *det_result = &(od_results.results[i]);

                    sX = (int)(det_result->box.left);
                    sY = (int)(det_result->box.top);
                    eX = (int)(det_result->box.right);
                    eY = (int)(det_result->box.bottom);
                    mapCoordinates(&sX, &sY);
                    mapCoordinates(&eX, &eY);

                    printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
                           sX, sY, eX, eY, det_result->prop);

                    cv::rectangle(frame, cv::Point(sX, sY),
                                  cv::Point(eX, eY),
                                  cv::Scalar(0, 255, 0), 3);
                    sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
                    cv::putText(frame, text, cv::Point(sX, sY - 8),
                                cv::FONT_HERSHEY_SIMPLEX, 1,
                                cv::Scalar(0, 255, 0), 2);
                }
            }
            memcpy(data, frame.data, width * height * 3);

            // encode H264
            RK_MPI_VENC_SendFrame(0, &h264_frame, -1);

            // rtsp
            s32Ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
            if (s32Ret == RK_SUCCESS)
            {
                if (g_rtsplive && g_rtsp_session)
                {
                    // printf("len = %d PTS = %d \n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS);
                    void *pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
                    rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, stFrame.pstPack->u32Len,
                                  stFrame.pstPack->u64PTS);
                    rtsp_do_event(g_rtsplive);
                }
            }
            RK_U64 nowUs = TEST_COMM_GetNowUs();
            fps = (float)1000000 / (float)(nowUs - h264_frame.stVFrame.u64PTS);
            printf("fps = %.2f\n", fps);
            // 释放VI帧
            s32Ret = RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame);
            if (s32Ret != RK_SUCCESS)
            {
                RK_LOGE("RK_MPI_VI_ReleaseChnFrame fail %x", s32Ret);
            }
            s32Ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
            if (s32Ret != RK_SUCCESS)
            {
                RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
            }
            memset(text, 0, 8);
        }
        else
        {
            printf("获取VI帧失败！\n");
            goto out;
        }
    }

out:
    // Destory MB
    RK_MPI_MB_ReleaseMB(src_Blk);
    // Destory Pool
    RK_MPI_MB_DestroyPool(src_Pool);

    RK_MPI_VI_DisableChn(0, 0);
    RK_MPI_VI_DisableDev(0);

    SAMPLE_COMM_ISP_Stop(0);

    RK_MPI_VENC_StopRecvFrame(0);
    RK_MPI_VENC_DestroyChn(0);

    free(stFrame.pstPack);

    if (g_rtsplive)
        rtsp_del_demo(g_rtsplive);

    RK_MPI_SYS_Exit();

    deinit_post_process();
    ret = release_yolov8_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov8_model fail! ret=%d\n", ret);
    }

    return 0;
}
