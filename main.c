#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>

#define C_RED "\x1B[31m"
#define C_GRN "\x1B[32m"
#define C_YEL "\x1B[33m"
#define C_BLU "\x1B[34m"
#define C_MAG "\x1B[35m"
#define C_CYN "\x1B[36m"
#define C_WHT "\x1B[37m"
#define C_RESET "\x1B[0m"

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

enum state
{
    NOT_TYPED = 0,
    CORRECT,
    INCORRECT,
};

struct bt
{
    size_t game_curser;
    size_t player_curser;
    size_t line;

    char *txt;
    size_t size;
};

void free_game(struct bt *game)
{
    free(game->txt);
}

bool create_game(struct bt *game, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "could not open the file '%s': %s\n", filename, strerror(errno));
        return false;
    }

    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    game->txt = malloc(sizeof(game->txt[0]) * size);
    if (game->txt == NULL)
    {
        fclose(file);
        fprintf(stderr, "error: cannot malloc %ld bytes for the txt: %s\n", size, strerror(errno));
        return false;
    }

    game->game_curser = 0;
    game->player_curser = 0;
    game->line = 0;
    game->size = size;
    fread(game->txt, 1, size, file);

    fclose(file);
    return true;
}

void print_char(char c, const char *color)
{
    if (color != NULL)
    {
        printf("%s", color);
    }

    switch (c)
    {
    case '\n':
    {
        printf("\U000023CE");
    }
    break;
    case ' ':
    {
        printf("\U0000FE4D");
    }
    break;
    default:
        printf("%c", c);
    }

    printf(C_RESET);
}

void line_print(struct bt *game)
{
    if (game->line > 0)
    {
        printf("\n");
    }

    while (game->txt[game->game_curser] != '\0')
    {
        char c = game->txt[game->game_curser];
        game->game_curser++;
        print_char(c, NULL);
        if (c == '\n')
        {
            game->line++;
            printf("\r");
            fflush(stdout);
            return;
        }
    }
    printf("\r");
    fflush(stdout);
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
    while (game.game_curser != game.size)
    {
        line_print(&game);

        for (; game.player_curser < game.game_curser; game.player_curser++)
        {
            int c = getchar();

            if (c == game.txt[game.player_curser])
            {
                print_char(game.txt[game.player_curser], C_GRN);
            }
            else
            {
                print_char(game.txt[game.player_curser], C_RED);
            }
        }
    }

    free_game(&game);
    return EXIT_SUCCESS;
}
