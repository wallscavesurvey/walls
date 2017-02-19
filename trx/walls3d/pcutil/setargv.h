#ifndef HG_PCUTIL_SETARGV_H
#define HG_PCUTIL_SETARGV_H

// C++ version
extern void setArgV(const char *pszCmdLine, 
                    const char *argV0,                  // program name
                    int &argC,
                    char **&argV
                   );

// plain C - version
extern "C" void setArg(const char *pszCmdLine, 
                       const char *argV0,
                       int* argC,
                       char ***argV
                      );


#endif
