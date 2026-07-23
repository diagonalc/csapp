#include "csapp.h"

int main(void)
{
    char *buf = getenv("QUERY_STRING");

    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1, n2;
    if (buf)
    {
        char *p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    int ofs = 0;
    ofs += sprintf(content + ofs, "QUERY_STRING = %s", buf);
    ofs += sprintf(content + ofs, "<html><h1>WELCOME TO ADDER</h1>\r\n");
    ofs += sprintf(content + ofs, "<p>The answer is: %d + %d = %d</p>\r\n", n1, n2, n1 + n2);
    ofs += sprintf(content + ofs, "<p>Thank you for visiting!</p>\r\n");

    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}