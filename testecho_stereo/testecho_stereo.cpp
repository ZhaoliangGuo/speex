#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include "arch.h"

#ifdef __cplusplus  
extern "C" {
#endif

#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#ifdef __cplusplus
}
#endif

#include <windows.h>

int main(int argc, char **argv)
{
	int sampleRate     = 48000;
	int nFrameSizeInMS = 10;
	int nFilterLenInMS = 120;

	SpeexEchoState       *echo_state       = NULL;
	SpeexPreprocessState *preprocess_state = NULL; 

	SYSTEMTIME st;
	::GetLocalTime(&st);

	int frame_size    = (nFrameSizeInMS  * sampleRate  * 1.0) / 1000;
	int filter_length = (nFilterLenInMS  * sampleRate  * 1.0) / 1000;

	char szMicFileName[]     = ".\\near48_stereo.pcm";
	char szSpeakerFileName[] = ".\\far48_stereo.pcm";

	char szOutputFileName[512];
	sprintf(szOutputFileName, ".\\output_stereo_samplerate_%d_NN_%4d_TAIL_%4d_%04d%02d%02d_%02d%02d%02d.pcm",
		sampleRate,
		frame_size, filter_length,
		st.wYear, st.wMonth, st.wDay, 
		st.wHour, st.wMinute, st.wSecond);
	printf("%s\n", szOutputFileName);

	LARGE_INTEGER timeStartCount;
	LARGE_INTEGER timeEndCount;
	LARGE_INTEGER timeFreq;
	QueryPerformanceFrequency(&timeFreq);
	QueryPerformanceCounter(&timeStartCount);

	FILE *echo_fd,  *ref_fd, *e_fd;

	ref_fd  = fopen(szMicFileName,     "rb");
	echo_fd = fopen(szSpeakerFileName, "rb"); 
	e_fd    = fopen(szOutputFileName,  "wb");

	echo_state       = speex_echo_state_init_mc(frame_size, filter_length, 2, 2); // for stereo
	frame_size *= 2; // length read each time

	preprocess_state = speex_preprocess_state_init(frame_size, sampleRate);

	speex_echo_ctl(echo_state,             SPEEX_ECHO_SET_SAMPLING_RATE,    &sampleRate);
	speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);

	short *echo_buf; 
	short *ref_buf;  
	short *e_buf;  

	echo_buf = new short[frame_size];
	ref_buf  = new short[frame_size];
	e_buf    = new short[frame_size];

	while (!feof(ref_fd) && !feof(echo_fd))
	{
		int nLen = fread(ref_buf,  sizeof(short), frame_size, ref_fd);
		if (nLen < 0)
		{
			break;
		}

		nLen = fread(echo_buf, sizeof(short), frame_size, echo_fd);
		if (nLen < 0)
		{
			break;
		}

		speex_echo_cancellation(echo_state, ref_buf, echo_buf, e_buf);
		speex_preprocess_run(preprocess_state, e_buf);

		fwrite(e_buf, sizeof(short), frame_size, e_fd);
	}

	// Destroys an echo canceller state
	speex_echo_state_destroy(echo_state);
	speex_preprocess_state_destroy(preprocess_state);

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

	QueryPerformanceCounter(&timeEndCount);
	double elapsed = (((double)(timeEndCount.QuadPart - timeStartCount.QuadPart) * 1000/ timeFreq.QuadPart));

	printf("AEC Done. TimeDuration: %.2f ms\n", elapsed);

	getchar();

	return 0;
}