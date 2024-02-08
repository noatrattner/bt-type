#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/time.h>
#include <stdint.h>

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

uint64_t get_time_ms()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

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

struct stats
{
    size_t typed_chars;
    size_t correctly_typed;
    size_t wrongly_typed;
    size_t backspace_typed;

    size_t words_typed;
    uint64_t start_time; // used for calculation of speed
};

struct bt
{
    size_t current_line;

    char **lines;
    size_t lines_count;

    struct stats stat;
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
    for (size_t i = 0; i <= lines_count; i++)
    {
        size_t buff_size = 128;
        char buff[buff_size];
        if (fgets(buff, buff_size, file) != NULL)
        {
            size_t size_of_line = strlen(buff) + 1;
            game->lines[i] = malloc(sizeof(char) * size_of_line);
            memcpy(game->lines[i], buff, size_of_line);
        }
        else
        {
            size_t size_of_line = 1;
            game->lines[i] = malloc(sizeof(char));
            memcpy(game->lines[i], "\0", size_of_line);
        }
    }

    game->current_line = 0;
    game->lines_count = lines_count;

    game->stat.typed_chars = 0;
    game->stat.correctly_typed = 0;
    game->stat.wrongly_typed = 0;
    game->stat.backspace_typed = 0;

    game->stat.words_typed = 1; // because the last word wont count
    game->stat.start_time = get_time_ms();

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
void cursor_forward(int x) { printf("\033[%dC", (x)); }
void cursor_up(int x) { printf("\033[%dA", (x)); }

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

void print_stat(struct stats stat)
{
    printf("stats:\n");
    printf("    typed_chars:           %ld\n", stat.typed_chars);
    printf("    correctly_typed:       %ld\n", stat.correctly_typed);
    printf("    wrongly_typed:         %ld\n", stat.wrongly_typed);
    printf("    backspace_typed:       %ld\n", stat.backspace_typed);
    printf("    words_typed:           %ld\n", stat.words_typed);
    const float time_took = (get_time_ms() - stat.start_time) / 1000.f;
    printf("    time took in seconds:  %.2f\n", time_took);
    printf("    word per minute:       %.2f\n", stat.words_typed / (time_took / 60.f));
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

    size_t curser = 0;
    while (true)
    {
        const char *line = game.lines[game.current_line];
        int c = getchar();

        if (c == 127) // backspace
        {
            game.stat.backspace_typed++;
            if (curser == 0)
            {
                cursor_up(1);
                game.current_line -= 1;

                line = game.lines[game.current_line];
                size_t line_len = strlen(line);

                size_t move_forward = 0;
                for (size_t i = 0; i < line_len - 1; i++)
                {
                    size_t glyph_size = g_font[line[i]].len;
                    if (glyph_size == 0)
                    {
                        glyph_size = 1;
                    }
                    move_forward += glyph_size;
                }
                cursor_forward(move_forward);
                curser = line_len - 1;
                continue;
            }
            if (line[curser - 1] == '\n' || line[curser - 1] == ' ' || line[curser - 1] == '\t')
            {
                game.stat.words_typed--;
            }

            size_t steps = 1;
            struct glyph g = g_font[line[curser - 1]];
            if (g.len != 0)
            {
                steps = g.len;
            }
            cursor_backward(steps);
            curser -= 1;
        }
        else
        {
            if (line[curser] == '\n' || line[curser] == ' ' || line[curser] == '\t')
            {
                game.stat.words_typed++;
            }

            game.stat.typed_chars++;
            if (c == line[curser])
            {
                print_char(line[curser], C_GRN);
                game.stat.correctly_typed++;
            }
            else
            {
                game.stat.wrongly_typed++;
                print_char(game.lines[game.current_line][curser], C_RED);
            }
            curser++;

            // check for end of line
            if (line[curser - 1] == '\n')
            {
                game.current_line++;
                putchar('\n');
                print_line(&game, game.current_line);
                putchar('\r');
                fflush(stdout);
                curser = 0;
            }

            if (line[curser] == '\0' || game.lines[game.current_line][0] == '\0')
            {
                break;
            }
        }
    }
    printf("\n\t\033[34mgame over!\033[0m\n");
    print_stat(game.stat);

    free_game(&game);
    return EXIT_SUCCESS;
}

// TODO: fix segfault in the last print line