#include "csapp.h"

int main(int argc, char **argv){
    if(argc != 2){
        unix_error("format: ./hostinfo <website>");
    }

    struct addrinfo hints;
    struct addrinfo *result, *cur;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[1], NULL, &hints, &result);
    for (cur = *result; cur; cur = cur -> ai_next){
        getnameinfo
    }
}