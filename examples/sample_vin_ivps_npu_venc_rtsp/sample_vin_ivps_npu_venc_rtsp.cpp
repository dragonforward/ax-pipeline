/*
 * AXERA is pleased to support the open source community by making ax-samples available.
 *
 * Copyright (c) 2022, AXERA Semiconductor (Shanghai) Co., Ltd. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

/*
 * Author: ZHEQIUSHUI
 */

#include "../libaxdl/include/c_api.h"
#include "../libaxdl/include/ax_osd_helper.hpp"

#include "../common/common_func.h"

#include "common_pipeline.h"
#include "../utilities/sample_log.h"

#include "ax_ivps_api.h"

#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#define pipe_count 2

AX_S32 s_sample_framerate = 25;
int SAMPLE_MAJOR_STREAM_WIDTH;
int SAMPLE_MAJOR_STREAM_HEIGHT;

int SAMPLE_IVPS_ALGO_WIDTH = 960;
int SAMPLE_IVPS_ALGO_HEIGHT = 540;
volatile AX_S32 gLoopExit;

static struct _g_sample_
{
    int bRunJoint;
    void *gModels;
#ifdef AXERA_TARGET_CHIP_AX620
    CAMERA_T gCams[MAX_CAMERAS];
#elif defined(AXERA_TARGET_CHIP_AX620E)

#endif

    AX_S32 g_isp_force_loop_exit;
    ax_osd_helper osd_helper;
    std::vector<pipeline_t *> pipes_need_osd;
    void Init()
    {
#ifdef AXERA_TARGET_CHIP_AX620
        memset(gCams, 0, sizeof(gCams));
#elif defined(AXERA_TARGET_CHIP_AX620E)

#endif
        g_isp_force_loop_exit = 0;
        bRunJoint = 0;
        gModels = nullptr;
        ALOGN("g_sample Init\n");
    }
    void Deinit()
    {
        pipes_need_osd.clear();

        ALOGN("g_sample Deinit\n");
    }
} g_sample;

void ai_inference_func(pipeline_buffer_t *buff)
{
    if (g_sample.bRunJoint)
    {
        static axdl_results_t mResults;
        axdl_image_t tSrcFrame = {0};
        switch (buff->d_type)
        {
        case po_buff_nv12:
            tSrcFrame.eDtype = axdl_color_space_nv12;
            break;
        case po_buff_bgr:
            tSrcFrame.eDtype = axdl_color_space_bgr;
            break;
        case po_buff_rgb:
            tSrcFrame.eDtype = axdl_color_space_rgb;
            break;
        default:
            break;
        }
        tSrcFrame.nWidth = buff->n_width;
        tSrcFrame.nHeight = buff->n_height;
        tSrcFrame.pVir = (unsigned char *)buff->p_vir;
        tSrcFrame.pPhy = buff->p_phy;
        tSrcFrame.tStride_W = buff->n_stride;
        tSrcFrame.nSize = buff->n_size;

        axdl_inference(g_sample.gModels, &tSrcFrame, &mResults);
        g_sample.osd_helper.Update(&mResults);
    }
}
#ifdef AXERA_TARGET_CHIP_AX620
static void *IspRun(void *args)
{
    AX_U32 i = (AX_U32)args;

    ALOGN("cam %d is running...\n", i);

    while (!g_sample.g_isp_force_loop_exit)
    {
        if (!g_sample.gCams[i].bOpen)
        {
            usleep(40 * 1000);
            continue;
        }

        AX_ISP_Run(g_sample.gCams[i].nPipeId);
    }
    return NULL;
}

static AX_S32 SysRun()
{
    AX_S32 s32Ret = 0, i;

    g_sample.g_isp_force_loop_exit = 0;
    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (g_sample.gCams[i].bOpen)
            pthread_create(&g_sample.gCams[i].tIspProcThread, NULL, IspRun, (AX_VOID *)i);
    }

    while (!gLoopExit)
    {
        sleep(1);
    }

    g_sample.g_isp_force_loop_exit = 1;

    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (g_sample.gCams[i].bOpen)
        {
            pthread_cancel(g_sample.gCams[i].tIspProcThread);
            s32Ret = pthread_join(g_sample.gCams[i].tIspProcThread, NULL);
            if (s32Ret < 0)
            {
                ALOGE(" isp run thread exit failed,s32Ret:0x%x\n", s32Ret);
            }
        }
    }
    return 0;
}
#endif
// 允许外部调用
extern "C" AX_VOID __sigExit(int iSigNo)
{
    // ALOGN("Catch signal %d!\n", iSigNo);
    gLoopExit = 1;
    return;
}

static AX_VOID PrintHelp(char *testApp)
{
    printf("Usage:%s -h for help\n\n", testApp);
    printf("\t-p: model config file path\n");
#ifdef AXERA_TARGET_CHIP_AX620
    printf("\t-c: ISP Test Case:\n");
    printf("\t\t0: Single OS04A10\n");
    printf("\t\t1: Single IMX334\n");
    printf("\t\t2: Single GC4653\n");
    printf("\t\t3: Single OS08A20\n");
    printf("\t\t4: Single OS04A10 Online\n");
#endif
    printf("\t-e: SDR/HDR Mode:\n");
    printf("\t\t1: SDR\n");
    printf("\t\t2: HDR 2DOL\n");

    printf("\t-r: Sensor&Video Framerate (framerate need supported by sensor), default is 25\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    optind = 0;
    gLoopExit = 0;
    g_sample.Init();

    AX_S32 isExit = 0, i, ch;
    AX_S32 s32Ret = 0;

    COMMON_SYS_ARGS_T tCommonArgs = {0};
    COMMON_SYS_ARGS_T tPrivArgs = {0};
    AX_SNS_HDR_MODE_E eHdrMode = AX_SNS_LINEAR_MODE;
#ifdef AXERA_TARGET_CHIP_AX620
    COMMON_SYS_CASE_E eSysCase = SYS_CASE_SINGLE_GC4653;
    SAMPLE_SNS_TYPE_E eSnsType = GALAXYCORE_GC4653;
#endif
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);
    char config_file[256];

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:c:e:r:h")) != -1)
    {
        switch (ch)
        {
        case 'p':
        {
            strcpy(config_file, optarg);
            break;
        }
#ifdef AXERA_TARGET_CHIP_AX620
        case 'c':
            eSysCase = (COMMON_SYS_CASE_E)atoi(optarg);
            break;
#endif
        case 'e':
            eHdrMode = (AX_SNS_HDR_MODE_E)atoi(optarg);
            break;
        case 'r':
            s_sample_framerate = (AX_S32)atoi(optarg);
            if (s_sample_framerate <= 0)
            {
                s_sample_framerate = 30;
            }
            break;
        case 'h':
            isExit = 1;
            break;
        default:
            isExit = 1;
            break;
        }
    }

    if (isExit)
    {
        PrintHelp(argv[0]);
        exit(0);
    }
#ifdef AXERA_TARGET_CHIP_AX620
    ALOGN("eSysCase=%d,eHdrMode=%d\n", eSysCase, eHdrMode);

    s32Ret = COMMON_SET_CAM(g_sample.gCams, eSysCase, eHdrMode, &eSnsType, &tCommonArgs, s_sample_framerate);
    if (s32Ret != 0)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

    SAMPLE_MAJOR_STREAM_WIDTH = g_sample.gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
    SAMPLE_MAJOR_STREAM_HEIGHT = g_sample.gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
#elif defined(AXERA_TARGET_CHIP_AX650)
    SAMPLE_MAJOR_STREAM_WIDTH = 1920;
    SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

    s32Ret = SAMPLE_VIN_Init();
    if (0 != s32Ret)
    {
        ALOGE("SAMPLE_VIN_Init failed, ret:0x%x", s32Ret);
        return -1;
    }

    s32Ret = SAMPLE_VIN_Open();
    if (0 != s32Ret)
    {
        ALOGE("SAMPLE_VIN_Open failed, ret:0x%x", s32Ret);
        return -1;
    }
#elif defined(AXERA_TARGET_CHIP_AX620E)
    SAMPLE_MAJOR_STREAM_WIDTH = 1920;
    SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

    s32Ret = SAMPLE_VIN_Init(SAMPLE_VIN_SINGLE_OS04A10);
    if (0 != s32Ret)
    {
        ALOGE("SAMPLE_VIN_Init failed, ret:0x%x", s32Ret);
        return -1;
    }

    s32Ret = SAMPLE_VIN_Open();
    if (0 != s32Ret)
    {
        ALOGE("SAMPLE_VIN_Open failed, ret:0x%x", s32Ret);
        return -1;
    }

#endif

    /*step 2:npu init*/
    // #ifdef AXERA_TARGET_CHIP_AX620

    // #else
    // #ifdef AXERA_TARGET_CHIP_AX650
    //     AX_ENGINE_NPU_ATTR_T npu_attr;
    //     memset(&npu_attr, 0, sizeof(npu_attr));
    //     npu_attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE;
    //     s32Ret = AX_ENGINE_Init(&npu_attr);
    // #elif defined(AXERA_TARGET_CHIP_AX620E)
    //     s32Ret = AX_ENGINE_Init();
    // #endif
    //     if (0 != s32Ret)
    //     {
    //         ALOGE("AX_ENGINE_Init 0x%x", s32Ret);
    //         return -1;
    //     }
    // #endif

    s32Ret = axdl_parse_param_init(config_file, &g_sample.gModels);
    if (s32Ret != 0)
    {
        ALOGE("sample_parse_param_det failed");
        g_sample.bRunJoint = 0;
    }
    else
    {
        s32Ret = axdl_get_ivps_width_height(g_sample.gModels, config_file, &SAMPLE_IVPS_ALGO_WIDTH, &SAMPLE_IVPS_ALGO_HEIGHT);
        ALOGI("IVPS AI channel width=%d height=%d", SAMPLE_IVPS_ALGO_WIDTH, SAMPLE_IVPS_ALGO_HEIGHT);
        g_sample.bRunJoint = 1;
    }

#ifdef AXERA_TARGET_CHIP_AX620
    AX_NPU_SDK_EX_ATTR_T sNpuAttr;
    sNpuAttr.eHardMode = AX_NPU_VIRTUAL_1_1;
    s32Ret = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
    if (0 != s32Ret)
    {
        ALOGE("AX_NPU_SDK_EX_Init_with_attr failed,s32Ret:0x%x\n", s32Ret);
        return -1;
    }
    /*step 3:camera init*/
    s32Ret = COMMON_CAM_Init();
    if (0 != s32Ret)
    {
        ALOGE("COMMON_CAM_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }

    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        s32Ret = COMMON_CAM_Open(&g_sample.gCams[i]);
        if (s32Ret)
        {
            ALOGE("COMMON_CAM_Open failed,s32Ret:0x%x\n", s32Ret);
            goto EXIT_3;
        }
        g_sample.gCams[i].bOpen = AX_TRUE;
        ALOGN("camera %d is open\n", i);
    }
#elif defined(AXERA_TARGET_CHIP_AX620E)

#endif

    pipeline_t pipelines[pipe_count];
    memset(&pipelines[0], 0, sizeof(pipelines));
    // 创建pipeline
    {

        pipeline_t &pipe0 = pipelines[0];
        {
            pipeline_ivps_config_t &config0 = pipe0.m_ivps_attr;
            config0.n_ivps_grp = 0;
            config0.n_ivps_fps = s_sample_framerate;
            config0.n_ivps_width = 1920;
            config0.n_ivps_height = 1080;
            config0.n_osd_rgn = 4; // osd rgn 的个数（最多五个），一个rgn可以osd 32个目标，现在用的是自定义的rgba画布，所以指挥占用一个rgn里的一个目标，所以这里只创建一个
        }
        pipe0.enable = 1;
        pipe0.pipeid = 0;
        pipe0.m_input_type = pi_vin;
        pipe0.m_output_type = po_rtsp_h264; // 可以创建265，降低带宽压力
        pipe0.n_loog_exit = 0;              // 可以用来控制线程退出（如果有的话）
        pipe0.n_vin_pipe = 0;
        pipe0.n_vin_chn = 0;
        sprintf(pipe0.m_venc_attr.end_point, "%s", "axstream0"); // 重复的会创建失败
        pipe0.m_venc_attr.n_venc_chn = 0;                        // 重复的会创建失败

        pipeline_t &pipe1 = pipelines[1];
        {
            pipeline_ivps_config_t &config1 = pipe1.m_ivps_attr;
            config1.n_ivps_grp = 1; // 重复的会创建失败
            config1.n_ivps_fps = 60;
            config1.n_ivps_width = SAMPLE_IVPS_ALGO_WIDTH;
            config1.n_ivps_height = SAMPLE_IVPS_ALGO_HEIGHT;
            if (axdl_get_model_type(g_sample.gModels) != MT_SEG_PPHUMSEG && axdl_get_model_type(g_sample.gModels) != MT_SEG_DINOV2)
            {
                config1.b_letterbox = 1;
            }
            config1.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = 1;
        pipe1.pipeid = 1;
        pipe1.m_input_type = pi_vin;
        if (g_sample.gModels && g_sample.bRunJoint)
        {
#ifdef AXERA_TARGET_CHIP_AX620E
            pipe1.m_output_type = po_buff_nv12;
#else
            switch (axdl_get_color_space(g_sample.gModels))
            {
            case axdl_color_space_rgb:
                pipe1.m_output_type = po_buff_rgb;
                break;
            case axdl_color_space_bgr:
                pipe1.m_output_type = po_buff_bgr;
                break;
            case axdl_color_space_nv12:
            default:
                pipe1.m_output_type = po_buff_nv12;
                break;
            }
#endif
        }
        else
        {
            pipe1.enable = 0;
        }
        pipe1.n_loog_exit = 0;
        pipe1.n_vin_pipe = 0;
        pipe1.n_vin_chn = 0;
        pipe1.output_func = ai_inference_func; // 图像输出的回调函数

        // pipeline_t &pipe2 = pipelines[2];
        // {
        //     pipeline_ivps_config_t &config2 = pipe2.m_ivps_attr;
        //     config2.n_ivps_grp = 2;    // 重复的会创建失败
        //     config2.n_ivps_rotate = 1; // 旋转90度，现在rtsp流是竖着的画面了
        //     config2.n_ivps_fps = s_sample_framerate;
        //     config2.n_ivps_width = 960;
        //     config2.n_ivps_height = 540;
        //     config2.n_osd_rgn = 4;
        // }
        // pipe2.enable = 1;
        // pipe2.pipeid = 2; // 重复的会创建失败
        // pipe2.m_input_type = pi_vin;
        // pipe2.m_output_type = po_rtsp_h264;
        // pipe2.n_loog_exit = 0;
        // pipe2.n_vin_pipe = 0;
        // pipe2.n_vin_chn = 0;
        // sprintf(pipe2.m_venc_attr.end_point, "axstream1"); // 重复的会创建失败
        // pipe2.m_venc_attr.n_venc_chn = 1;                  // 重复的会创建失败

        for (size_t i = 0; i < pipe_count; i++)
        {
            create_pipeline(&pipelines[i]);
            if (pipelines[i].m_ivps_attr.n_osd_rgn > 0)
            {
                g_sample.pipes_need_osd.push_back(&pipelines[i]);
            }
        }

        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Start(g_sample.gModels, g_sample.pipes_need_osd);
        }
    }
#ifdef AXERA_TARGET_CHIP_AX620
    s32Ret = SysRun();
    if (0 != s32Ret)
    {
        ALOGE("SysRun error,s32Ret:0x%x\n", s32Ret);
        goto EXIT_6;
    }
#else
    s32Ret = SAMPLE_VIN_Start();
    if (0 != s32Ret)
    {
        ALOGE("SAMPLE_VIN_Start failed, ret:0x%x", s32Ret);
        return -1;
    }
    while (!gLoopExit)
    {
        usleep(1000 * 1000);
    }
#endif

    // 销毁pipeline
    {
        gLoopExit = 1;
        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Stop();
        }

        for (size_t i = 0; i < pipe_count; i++)
        {
            destory_pipeline(&pipelines[i]);
        }
    }

EXIT_6:
#ifdef AXERA_TARGET_CHIP_AX620
    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        if (!g_sample.gCams[i].bOpen)
            continue;
        COMMON_CAM_Close(&g_sample.gCams[i]);
    }
    COMMON_CAM_Deinit();
#else
    SAMPLE_VIN_Deinit();
#endif
EXIT_3:

    axdl_deinit(&g_sample.gModels);

EXIT_2:

    COMMON_SYS_DeInit();

    g_sample.Deinit();

    ALOGN("sample end\n");
    return 0;
}