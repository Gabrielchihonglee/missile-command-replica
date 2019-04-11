#define FRAME_WIDTH 124
#define FRAME_HEIGHT 40

#define START_PADDING_HORIZONTAL 30
#define START_PADDING_VERTICAL 12

struct carouselThreadArg {
    WINDOW *screen;
    int live;
    int start_x, end_x, y;
    char *text;
    int color_pair;
};

static pthread_mutex_t lock;

static int start_explosion_pos[80][2];
static char *STAGE_1, *STAGE_2;

static enum drawMode {ERASE, DRAW};

static WINDOW *carousel_thread_screen;
static int carousel_thread_live;
static int carousel_thread_start_x, carousel_thread_end_x, carousel_thread_y;
static char *carousel_thread_text;
static int carousel_thread_color_pair;

void drawFromFile(WINDOW *screen, int start_x, int start_y, char file[], enum drawMode mode) { // mode 1: draw 0: erase/draw with backgound
    FILE *fp = fopen(file, "r");
    char symbol;
    int x, y;
    wmove(screen, start_y, start_x);
    while ((symbol = getc(fp)) != EOF) {
        switch (symbol) {
            case '.':
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#':
                if (mode == 1) {
                    waddch(screen, ACS_CKBOARD);
                } else {
                    waddch(screen, ' ');
                }
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found in %s: '%c'", file, symbol);
                break;
        }
        wrefresh(screen);
    }
}

void drawFromString(WINDOW *screen, int start_x, int start_y, char *line, enum drawMode mode) {
    int length = strlen(line);
    int x, y;
    wmove(screen, start_y, start_x);
    for (int i = 0; i < length; i++) {
        switch (line[i]) {
            case '.':
                getyx(screen, y, x);
                wmove(screen, y, x + 1);
                break;
            case '#':
                if (mode == 1) {
                    waddch(screen, ACS_CKBOARD);
                } else {
                    waddch(screen, ' ');
                }
                break;
            case '\n':
                getyx(screen, y, x);
                wmove(screen, y + 1, start_x);
                break;
            default:
                fprintf(stderr, "Unexpected character found: '%i'", line[i]);
                break;
        }
        wrefresh(screen);
    }
}

void updateSmallExplosionStage(WINDOW *screen, int from_missile, int to_missile, int color) {
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_1, DRAW); // draw stage 1
    }
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, DRAW); // draw stage 2
    }
    pthread_mutex_unlock(&lock);
    usleep(100000);
    pthread_mutex_lock(&lock);
    wattron(screen, COLOR_PAIR(color));
    for (int i = from_missile; i < to_missile; i++) {
        drawFromString(screen, start_explosion_pos[i][0], start_explosion_pos[i][1], STAGE_2, ERASE); // erase stage 2
    }
    pthread_mutex_unlock(&lock);
    usleep(1000);
}

void refreshHighScore(WINDOW *screen, int cur_score, int high_score) {
    char cur_score_text[10];
    char high_score_text[10];
    sprintf(cur_score_text, "%i", cur_score);
    sprintf(high_score_text, "%i", high_score);
    wattron(screen, COLOR_PAIR(2));
    mvwprintw(screen, 0, FRAME_WIDTH / 2, cur_score_text);
    mvwprintw(screen, 0, FRAME_WIDTH / 2 - 15, cur_score_text);
}

void *carouselFromString(void *argument) {
    wrefresh(carousel_thread_screen);
    int carousel_thread_head_x = carousel_thread_start_x;
    while (carousel_thread_live) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < (FRAME_WIDTH - 1 - carousel_thread_head_x) && i < strlen(carousel_thread_text); i++) {
            if ((carousel_thread_head_x + i) < carousel_thread_end_x) {
                continue;
            }
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + i, carousel_thread_text[i]);
        }
        if ((carousel_thread_head_x + strlen(carousel_thread_text)) < (FRAME_WIDTH - 1)) {
            wattron(carousel_thread_screen, COLOR_PAIR(carousel_thread_color_pair));
            mvwaddch(carousel_thread_screen, carousel_thread_y, carousel_thread_head_x + strlen(carousel_thread_text), ' ');
        }
        pthread_mutex_unlock(&lock);
        wrefresh(carousel_thread_screen);
        usleep(160000);
        carousel_thread_head_x--;
        if (carousel_thread_head_x + strlen(carousel_thread_text) + 1 <= carousel_thread_end_x) {
            carousel_thread_head_x = carousel_thread_start_x;
        }
    }
    return NULL;
}
