#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
#include "../lib/user/syscall.h"

int
main (int argc, char *argv[])
{	
	int f = 0, s = 0;
	int num[4], i;
	for (i = 0; i < argc - 1; i++) {
		num[i] = atoi(argv[i + 1]);
	}
	//printf("%d %d %d %d\n", num[0], num[1], num[2], num[3]);

	f = fibonacci(num[0]);
	s = sum_of_four_int(num[0], num[1], num[2], num[3]);

	printf("%d %d\n", f, s);

	return 0;  
}
