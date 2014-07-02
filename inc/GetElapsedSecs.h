#ifndef _GETELAPSEDSECS_H
#define _GETELAPSEDSECS_H

#ifdef __cplusplus
extern "C" {
#endif

#define START_TIMER() QueryPerformanceCounter((LARGE_INTEGER *)&nti_timer0)
#define STOP_TIMER() {QueryPerformanceCounter((LARGE_INTEGER *)&nti_timer); nti_timer-=nti_timer0;}
#define TIMER_SECS() GetPerformanceSecs(&nti_timer)

extern unsigned __int64 nti_timer,nti_timer0;

void GetPerformanceCounter(unsigned __int64 *lCount);
double GetPerformanceSecs(unsigned __int64 *lCount);

#ifdef _USE_TM

	#define SIZ_NTI_SECS 10
	extern unsigned __int64 nti_tm0[SIZ_NTI_SECS];
	extern unsigned __int64 nti_tm[SIZ_NTI_SECS];

	#define SUM_NTI_ALL(s) {s=0; for(int i=0;i<SIZ_NTI_SECS;i++) s+=nti_tm[i];}
	#define CLR_NTI_ALL() {for(int i=0;i<SIZ_NTI_SECS;i++) nti_tm[i]=0;}
	#define CLR_NTI_TM(i) nti_tm[i]=0
	#define START_NTI_TM(i) GetPerformanceCounter(&nti_tm0[i])
	#define STOP_NTI_TM(i)  {GetPerformanceCounter(&nti_timer); nti_tm[i]+=(nti_timer-nti_tm0[i]);}
	#define NTI_SECS(i) GetPerformanceSecs(&nti_tm[i])

#else
	#define CLR_NTI_TM(i)
	#define START_NTI_TM(i)
	#define STOP_NTI_TM(i)
	#define NTI_SECS(i) 0
#endif

#ifdef __cplusplus
}
#endif
#endif
