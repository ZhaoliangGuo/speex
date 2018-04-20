/**************************************************   
Author   : ZL Guo   
Email    : 1799580752@qq.com
TechBlog : https://www.jianshu.com/u/b971a1d12a6f  
Description: 
本程序主要用于利用speex进行单声道的回声消除测试。
主要包括两大部分。
1. 针对多个分辨率进行回声消除，分为8k 16k 32k 48k
2. 测试收敛时间  
**************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include "arch.h"
#include <windows.h>

#ifdef __cplusplus  
extern "C" {    //如果被C++文件引用
#endif

#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#ifdef __cplusplus
}
#endif

enum EPxSampleRateFreq
{
	kePxSampleRateFreq_Invalid = -1,
	kePxSampleRateFreq_8k      = 8000,
	kePxSampleRateFreq_16k     = 16000,
	kePxSampleRateFreq_32k     = 32000,
	kePxSampleRateFreq_48k     = 48000,
	kePxSampleRateFreq_Cnt
};

int main(int argc, char **argv)
{
	FILE *echo_fd, *ref_fd, *e_fd;
	SpeexEchoState *st;
	SpeexPreprocessState *den;
	int sampleRate = 48000; // 提供了8000 16000 32000 48000的测试样本

	int nFrameSizeInMS = 10;  // 每次取多少毫秒的数据进行处理
	int nFilterLenInMS = 120; // 滤波器长度为多少毫秒

	SYSTEMTIME sysTime;
	::GetLocalTime(&sysTime);

	int frame_size    = (nFrameSizeInMS  * sampleRate  * 1.0) / 1000;
	int filter_length = (nFilterLenInMS  * sampleRate  * 1.0) / 1000;

	char szEchoFileName[512]   = {0};
	char szRefFileName[512]    = {0};

#if 1
	// test 1 : 多采样率测试
	// 这里给出的micin 和 speaker的数据是完全对齐的
	if (kePxSampleRateFreq_8k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "micin_8k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_8k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_16k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "micin_16k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_16k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_32k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "micin_32k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_32k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_48k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "micin_48k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_48k_s16_mono.pcm");
	}

#else 
	// test 2 : 测试收敛速度
	// 用同一个文件测试 看什么时候开始输出连续的静音 就是收敛了
	if (kePxSampleRateFreq_8k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "speaker_8k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_8k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_16k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "speaker_16k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_16k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_32k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "speaker_32k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_32k_s16_mono.pcm");
	}
	else if (kePxSampleRateFreq_48k == sampleRate)
	{
		sprintf(szEchoFileName, "%s", "speaker_48k_s16_mono.pcm");
		sprintf(szRefFileName,  "%s", "speaker_48k_s16_mono.pcm");
	}
#endif 

	char szOutputFileName[512];
	sprintf(szOutputFileName, ".\\output\\mono_%s_%s_%04d%02d%02d_%02d%02d%02d.pcm",
		szEchoFileName, szRefFileName,
		sysTime.wYear, sysTime.wMonth, sysTime.wDay, 
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	printf("%s\n", szOutputFileName);

	ref_fd  = fopen(szEchoFileName,   "rb");
	echo_fd = fopen(szRefFileName,    "rb");
	e_fd    = fopen(szOutputFileName, "wb");

	short *echo_buf; 
	short *ref_buf;  
	short *e_buf;  

	echo_buf = new short[frame_size];
	ref_buf  = new short[frame_size];
	e_buf    = new short[frame_size];

	// 非常重要的函数 直接影响收敛速度
	// TAIL越小，收敛速度越快，但对近端数据和远端数据的同步要求提高
	// 比如TAIL设置为100ms的samples，那么如果near和far数据相差超过100ms，则无法消除
	// TAIL过大, 收敛速度变慢，且可能造成滤波器不稳定
	st  = speex_echo_state_init(frame_size, filter_length); 
	                                       
	den = speex_preprocess_state_init(frame_size, sampleRate);

	speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
	speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);

	LARGE_INTEGER timeStartCount;
	LARGE_INTEGER timeEndCount;
	LARGE_INTEGER timeFreq;
	QueryPerformanceFrequency(&timeFreq);
	QueryPerformanceCounter(&timeStartCount);

	while (!feof(ref_fd) && !feof(echo_fd))
	{
		fread(ref_buf,  sizeof(short), frame_size, ref_fd);
		fread(echo_buf, sizeof(short), frame_size, echo_fd);

		// ref_buf  : 从speaker处获取到的数据  
		// echo_buf : 从麦克采集到的数据 
		// e_buf    : 回声消除后的数据  
		speex_echo_cancellation(st, ref_buf, echo_buf, e_buf);
		speex_preprocess_run(den, e_buf);
		fwrite(e_buf, sizeof(short), frame_size, e_fd);
	}

	QueryPerformanceCounter(&timeEndCount);
	double elapsed = (((double)(timeEndCount.QuadPart - timeStartCount.QuadPart) * 1000/ timeFreq.QuadPart));

	printf("AEC Done. TimeDuration: %.2f ms\n", elapsed);

	speex_echo_state_destroy(st);
	speex_preprocess_state_destroy(den);

	fclose(e_fd);
	fclose(echo_fd);
	fclose(ref_fd);

	if (echo_buf)
	{
		delete [] echo_buf;
		echo_buf = NULL;
	}

	if (ref_buf)
	{
		delete [] ref_buf;
		ref_buf = NULL;
	}

	if (e_buf)
	{
		delete [] e_buf;
		e_buf = NULL;
	}

	getchar();

	return 0;
}
