#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include "colors.h"

struct termios original;

struct glyph
{
    const char *g;
    size_t len;
};

struct glyph g_font[256] = {
    ['\n'] = {"\U000023CE", 2},
    [' '] = {"\U0000FE4D", 2}};

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

enum state
{
    NOT_TYPED = 0,
    CORRECT,
    INCORRECT,
};

struct bt
{
    size_t current_line;

    char **lines;
    size_t lines_count;
};

void free_game(struct bt *game)
{
    for (size_t i = 0; i < game->lines_count; i++)
    {
        free(game->lines[i]);
    }
    free(game->lines);
}

int count_lines(FILE *file)
{
    // save curser position
    long int curser_pos = ftell(file);

    int count = 0;
    int c = getc(file);
    while (c != EOF)
    {
        if (c == '\n')
        {
            count++;
        }
        c = getc(file);
    }

    // return the curser to the last position
    fseek(file, curser_pos, SEEK_SET);
    return count;
}

bool create_game(struct bt *game, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "could not open the file '%s': %s\n", filename, strerror(errno));
        return false;
    }

    // count lines
    int lines_count = count_lines(file);
    game->lines = malloc(sizeof(game->lines[0]) * lines_count);

    // load each line
    for (size_t i = 0; i < lines_count + 1; i++)
    {
        size_t buff_size = 128;
        char buff[buff_size];
        fgets(buff, buff_size, file);

        size_t size_of_line = strlen(buff) + 1;
        game->lines[i] = malloc(sizeof(char) * size_of_line);
        memcpy(game->lines[i], buff, size_of_line);
    }

    game->current_line = 0;
    game->lines_count = lines_count;

    fclose(file);
    return true;
}

void print_char(char c, const char *color)
{
    if (color != NULL)
    {
        printf("%s", color);
    }

    struct glyph g = g_font[c];
    if (g.g == NULL)
    {
        printf("%c", c);
    }
    else
    {
        printf("%s", g.g);
    }

    printf(C_RESET);
}

void cursor_backward(int x) { printf("\033[%dD", (x)); }

void print_line(struct bt *game, int line_to_print)
{
    const char *line = game->lines[line_to_print];
    for (; *line != '\0'; line++)
    {
        print_char(*line, NULL);
    }
}

void print_lines(struct bt *game, int game_line)
{
    if (game->current_line > game_line) // line deleted
    {
        // remove future line && add past line && move curser to end of last line
    }
    else if (game->current_line < game_line) // line completed
    {
        // remove past line && add future line && move curser to next line
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <txt file path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *pathname = argv[1];

    struct bt game;
    if (!create_game(&game, pathname))
    {
        printf("error: could not open txt file\n");
        return EXIT_FAILURE;
    }

    enable_raw_mode();
    atexit(restore_mode);

    // Game loop
    int game_line = 0;
    print_line(&game, game.current_line);
    putchar('\r');
    fflush(stdout);

    while (true) // FIXME: check if the player finished the file
    {
        const char *line = game.lines[game.current_line];
        size_t line_len = strlen(line);
        size_t curser = 0;
        for (; curser < line_len; curser++)
        {
            int c = getchar();

            if (c == 127) // backspace
            {
                if (curser == 0)
                {
                    // TODO: move to last line
                    continue;
                }

                size_t steps = 1;
                struct glyph g = g_font[line[curser - 1]];
                if (g.len != 0)
                {
                    steps = g.len;
                }

                cursor_backward(steps);
                curser -= 2;
            }
            else
            {
                if (c == line[curser])
                {
                    print_char(line[curser], C_GRN);
                }
                else
                {
                    print_char(game.lines[game.current_line][curser], C_RED);
                }
            }
        }
        game.current_line++;
        putchar('\n');
        print_line(&game, game.current_line);
        putchar('\r');
        fflush(stdout);
        // new_line--;
        // new_line++;

        // TODO: check end game
    }

    free_game(&game);
    return EXIT_SUCCESS;
}

// TODO: fix segfault in the last print line