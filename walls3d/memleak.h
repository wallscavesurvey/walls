/*
 * File    :  memleak.h
 *
 * Author  :  Keith Andrews, IICM, TU Graz, Austria
 *
 * Created :  27 Jan 92
 *
 * Changed :  26 Mar 92
 *
 */





#ifndef memleak_h
#define memleak_h


#ifdef MEMLEAK


#include <stddef.h>

#define New ::new(__FILE__,__LINE__)
#define Delete __debugpos(__FILE__,__LINE__), ::delete


void __debugpos(const char* filename, unsigned line);

void* operator new(size_t size, char* file, unsigned line);
void  operator delete(void* p);

void MemLeakBegin(int verbose = 1);
void MemLeakEnd();

#else

#define New new
#define Delete delete

#endif




#endif
