#include "stdio.h"
#include "stdlib.h"
#include <string.h>

struct test
{
	char *name;
};

struct test arr[10];

int a()
{
	printf("hi\n");
	return 0;
}

int main(int argc, char **argv, char **envp)
{

	int (*entry)(void);
	entry = &a;
	entry();
	exit(0);
}
