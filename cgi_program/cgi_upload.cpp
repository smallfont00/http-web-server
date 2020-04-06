#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
using namespace std;
/*
Please make sure you understand host.c
*/

int main(void) {
    int unread;
    char *buf;

    // wait for stdin
    while (unread < 1) {
        if (ioctl(STDIN_FILENO, FIONREAD, &unread)) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }
    }
    buf = (char *)malloc(sizeof(char) * (unread + 1));

    // read from stdin fd
    read(STDIN_FILENO, buf, unread);

    char title[128], text[128], path[128];

    sscanf(buf, "title=%[^&]&text=%s", title, text);
    for (char *p = text; (*p); p++) {
        if ((*p) == '+') (*p) = ' ';
    }
        
    sprintf(path, "%s/%s", "database", title);
    FILE *fp = fopen(path, "w");
    fprintf(fp, "%s", text);

    fclose(fp);
    // output to stdout
    printf("<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n");
    printf("<TITLE>Message upload</TITLE>\n");
    printf("<BODY>title: %s has been uploaded</BODY></HTML>\n", title);
}