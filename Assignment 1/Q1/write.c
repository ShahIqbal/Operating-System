#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	FILE *f = fopen("redirect.txt", "w");
	if (f == NULL)
	{
    		printf("Error opening file!\n");
	    	exit(1);
	}

	/* print some text */
	const char *text = "A simple program output.";
	fprintf(f, "%s\n", text);
	fclose(f);
}
