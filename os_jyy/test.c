#include "stdio.h"
#include "stdlib.h"
#include <string.h>

struct test
{
	char *name;
};

struct test arr[10];

int main(int argc, char **argv, char **envp)
{
	struct test a;
	strcpy(a.name, "hhihihi");
	arr[0] = a;
	printf("%s\n", a.name);
	exit(0);
}
