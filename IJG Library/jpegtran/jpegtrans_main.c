#include <stdlib.h>

extern int jpegtran(int code,char *infile,char *outfile);

int main (int argc, char **argv)
{
	if(argc<4) return 1;
	return jpegtran(atoi(argv[1]), argv[2], argv[3])?0:2;
}
