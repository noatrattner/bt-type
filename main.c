#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>

struct termios original;

void enable_raw_mode()
{
    // set global for restoring
    tcgetattr(STDIN_FILENO, &original);

    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

int main(int argc, char const *argv[])
{
    /*
    V 1. read file
      2. show line on screen
      3. every typed character if typed ok then turn green else red
     */
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <txt file path>\n", argv[0]);
        return 1;
    }

    const char *pathname = argv[1];
    FILE *file = fopen(pathname, "r");
    if (file == NULL)
    {
        fprintf(stderr, "could not open the file '%s': %s\n", pathname, strerror(errno));
        return 1;
    }

    const size_t line_length = 50;
    char line[line_length];

    enable_raw_mode();
    atexit(restore_mode);

    while (fgets(line, line_length, file) != NULL)
    {
        size_t len = strlen(line);
        if (line[len - 1] = '\n') // TODO: somehow use the enter to say to the user to press enter
        {
            line[len - 1] = '\0';
        }

        for (size_t i = 0; i < len; i++)
        {
            getc(stdin);
            printf("%c", line[i]);
        }
        printf("\n");
    }

    fclose(file);
    return 0;
}
