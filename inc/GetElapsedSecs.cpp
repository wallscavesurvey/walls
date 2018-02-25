#include <windows.h>
#include "GetElapsedSecs.h"

#ifdef __cplusplus
extern "C" {
#endif

//#ifdef _USE_NTI_TIMER
#ifdef _USE_TM
unsigned __int64 nti_tm0[SIZ_NTI_SECS];
unsigned __int64 nti_tm[SIZ_NTI_SECS];
#endif

unsigned __int64 nti_timer, nti_timer0;

void GetPerformanceCounter(unsigned __int64 *lCount)
{
	QueryPerformanceCounter((LARGE_INTEGER *)lCount);
}

double GetPerformanceSecs(unsigned __int64 *lCount)
{
	LARGE_INTEGER swFreq;
	return QueryPerformanceFrequency(&swFreq)?(*lCount/(double)swFreq.QuadPart):0.0;
}

#ifdef __cplusplus
}
#endif
