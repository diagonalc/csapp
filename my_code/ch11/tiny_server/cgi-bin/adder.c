#include "../csapp.h"

int main(void)
{
    char *buf = getenv("QUERY_STRING");
    char *method = getenv("REQUEST_METHOD");
    char *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1, n2;
    if (buf)
    {
        p = strchr(buf, '&');
        *p = '\0';

        n1 = strchr(buf, '=') ? atoi(strchr(buf, '=') + 1) : atoi(buf);
        n2 = strchr(p + 1, '=') ? atoi(strchr(p + 1, '=') + 1) : atoi(p + 1);
    }

    int ofs = 0;
    ofs += sprintf(content + ofs, "QUERY_STRING = ?%s&%s\r\n", buf, p + 1);
    ofs += sprintf(content + ofs, "<html>\r\n");
    ofs += sprintf(content + ofs, "<h1>WELCOME TO ADDER</h1>\r\n");
    ofs += sprintf(content + ofs, "<p>The answer is: %d + %d = %d</p>\r\n", n1, n2, n1 + n2);
    ofs += sprintf(content + ofs, "<p>Thank you for visiting!</p>\r\n");
    ofs += sprintf(content + ofs, "<a href=\"../home.html\">Home</a>\r\n");
    ofs += sprintf(content + ofs, "</html>\r\n");

    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    if (strcmp("HEAD", method) != 0)
        printf("%s", content);
    fflush(stdout);
    exit(0);
}
