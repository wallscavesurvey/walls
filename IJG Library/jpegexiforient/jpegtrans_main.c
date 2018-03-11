#iclude <stdlib.h>

int main (int argc, char **argv)
{
	if(argc<4) return 1;
	return jpegtran(atoi(argv[1]), argv[3], argv[2])?0:2;
}
