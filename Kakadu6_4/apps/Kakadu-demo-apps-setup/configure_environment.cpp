#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char *argv[])
{
  char executable_path[MAX_PATH+1];
  memset(executable_path,0,MAX_PATH+1);
  GetModuleFileName(NULL,executable_path,MAX_PATH);
  char *app_name = strrchr(executable_path,'\\');
  if ((app_name == NULL) || (app_name == executable_path))
    {
      fprintf(stderr,"Cannot find executable path??\n");
      return -1;
    }
  *app_name = '\0';
  bool uninstalling = ((argc >= 2) && (strcmp(argv[1],"/Uninstall") == 0));
  DWORD disposition = 0;
  HKEY env_key = NULL;;
  if (RegCreateKeyEx(HKEY_CURRENT_USER,"Environment",0,REG_NONE,
                     REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                     &env_key,&disposition) == ERROR_SUCCESS)
    {
      char *path_buf = new char[65536];
      DWORD path_len = 65535;
      DWORD value_type=0;
      if (RegQueryValueEx(env_key,"path",NULL,&value_type,(BYTE *) path_buf,
                          &path_len) != ERROR_SUCCESS)
        {
          *path_buf = '\0';
          value_type = REG_SZ;
        }
      if ((value_type == REG_SZ) && (strlen(path_buf) < 32000))
        { // Otherwise things seem a bit too weird -- proceeding seems dangerous
          char *existing = strstr(path_buf,executable_path);
          if (existing == NULL)
            { // path does not currently contain the executable path component
              if (!uninstalling)
                {
                  if (strlen(path_buf) > 0) 
                    strcat(path_buf,";");
                  strcat(path_buf,executable_path);
                  RegSetValueEx(env_key,"path",0,REG_SZ,
                                (BYTE *)path_buf,(int)strlen(path_buf));
                }
            }
          else
            { // path already contains the executable path component
              if (uninstalling)
                {
                  char *lim = existing + strlen(executable_path);
                  if ((existing > path_buf) && (existing[-1] == ';'))
                    existing--;
                  while (*lim != '\0')
                    *(existing++) = *(lim++);
                  *existing = '\0';
                  RegSetValueEx(env_key,"path",0,REG_SZ,
                                (BYTE *)path_buf,(int)strlen(path_buf));
                }
            }
        }
      RegCloseKey(env_key);
      delete[] path_buf;
    }
	return 0;
}

