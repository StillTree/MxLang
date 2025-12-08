#include <stdio.h>

int main(int argc, char* argv[])
{
	printf("MxLang v" MX_VERSION "\n\n");

	if (argc < 2) {
		printf("No input file to run\n");
		return 1;
	}

	return 0;
}
