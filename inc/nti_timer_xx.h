#ifndef NTI_TIMER_H
#define NTI_TIMER_H
	extern LARGE_INTEGER nti_swTimer,nti_swTimer0;
	extern LARGE_INTEGER nti_tm_inf,nti_tm_def;
	extern LARGE_INTEGER nti_tm[10],nti_tm0[10];
	extern int nti_tmSz;
	extern bool nti_timing;
	double GetPerformanceSecs(LARGE_INTEGER &lCount);
	#define START_TIMER() QueryPerformanceCounter(&nti_swTimer0)
	#define STOP_TIMER() {QueryPerformanceCounter(&nti_swTimer);nti_swTimer.QuadPart-=nti_swTimer0.QuadPart;}
	#define TIMER_SECONDS() GetPerformanceSecs(nti_swTimer)

#ifdef NTI_TIMING
	#define START_NTI_TM(i) if(nti_timing) QueryPerformanceCounter(&nti_tm0[i])
	#define INC_NTI_TM(i)  if(nti_timing) {QueryPerformanceCounter(&nti_swTimer); \
							nti_tm[i].QuadPart+=(nti_swTimer.QuadPart-nti_tm0[i].QuadPart);}
	#define CLR_NTI_TM(i) nti_tm[i].QuadPart=0

	#define NTI_SECONDS(i) GetPerformanceSecs(nti_tm[i])
	#define END_NTI_TM(i) {nti_tmSz=i+1;}
#else
	#define START_NTI_TM(i)
	#define INC_NTI_TM(i)
	#define CLR_NTI_TM(i)
	#define NTI_SECONDS(i)
	#define END_NTI_TM(i)
#endif
#endif
