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
    size_t curser;
    size_t active_curser;

    char *txt;
    enum state *state;
    size_t size;
};

void free_game(struct bt *game)
{
    free(game->state);
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

    game->state = malloc(sizeof(game->state[0]) * size);
    if (game->state == NULL)
    {
        free(game->txt);
        fclose(file);
        fprintf(stderr, "error: cannot malloc %ld bytes for the state: %s\n", size, strerror(errno));
        return false;
    }

    game->curser = 0;
    game->active_curser = 0;
    game->size = size;
    fread(game->txt, 1, size, file);

    fclose(file);
    return true;
}

void line_print(struct bt *game)
{
    while (game->txt[game->curser] != '\0')
    {
        char c = game->txt[game->curser];
        printf("%c", c);
        game->curser++;

        if (game->txt[game->curser] == '\n')
        {
            return;
        }
    }
}

int main(int argc, char const *argv[])
{
    /*
    V 1. read file
    v 2. show line on screen
    v 3. every typed character if typed ok then turn green else red
     */
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

    while (game.curser != game.size)
    {
        line_print(&game);
        printf("\r");
        fflush(stdout);
        for (; game.active_curser < game.curser; game.active_curser++)
        {
            int c = getchar();

            if (c == game.txt[game.active_curser])
            {
                printf(C_GRN "%c" C_RESET, game.txt[game.active_curser]);
            }
            else
            {
                printf(C_RED "%c" C_RESET, game.txt[game.active_curser]);
            }
        }
    }

    free_game(&game);
    return EXIT_SUCCESS;
}
