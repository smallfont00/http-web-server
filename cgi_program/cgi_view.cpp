#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
    sscanf(buf, "title=%s", title);
    sprintf(path, "%s/%s", "database", title);
    FILE *fp = fopen(path, "r");
    if (fp != 0) {
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        rewind(fp);
        fread(text, len, 1, fp);
        text[len] = '\0';
        //fscanf(fp, "%s", text);
    } else {
        sprintf(text, "Not Found");
    }

    // output to stdout
    printf("<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n");
    printf("<TITLE>View message</TITLE>\n");
    printf("<BODY><h>%s :</h><p>%s</p></BODY></HTML>\n", title, text);
}