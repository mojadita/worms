/* $Id: gusanos.c,v 1.10 2014/02/11 10:04:04 luis Exp $
 * Author: Luis.Colorado@HispaLinux.ES
 * Date: Sat Mar 11 22:05:03 MET 2000
 * UNIX version, with ncurses lib.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <getopt.h>

#define DIRECT_CHG_PROB 50
#define MIN_LENGTH      3
#define MAX_LENGTH      150
#define MIN_WORMS     3
#define MAX_WORMS     30
#define DELAY()       do {if (delay_flag) usleep(20000);} while(0)
#define my_random(X)    (random()%(X))

#ifndef USE_COLORS
#define USE_COLORS 1
#endif

#if USE_COLORS
int colors;
int want_colors             = true;
#endif

int debug                   = false;
int delay_flag              = true;
int ascii_chars             = false;
int has_changed_window_size = false;

#if USE_COLORS
void do_print(int c, int x, int y, int col)
{
    mvaddch(y,x, COLOR_PAIR(col) | c);
}
#else
void do_print(int c, int x, int y)
{
    mvaddch(y,x, c);
}
#endif

/* structs defining the worm */
typedef struct segment {
   short            x,
                    y;
} segment;

typedef struct worm {
#if USE_COLORS
   int              color;
#endif
   int              max_length;
   int              curr_length;
   int              direction;
   struct segment  *segments_array;
   int              head,
                    tail;
} worm;

/* global variables */
worm    *worms_array        = NULL;
struct segment
        *segments_array     = NULL;
int      segments_number    = 0,
         worms_number       = 0;
char   **board;
int      heading_chg_probab = DIRECT_CHG_PROB;

#define TURN_L       0
#define STRAIGHT     1
#define TURN_R       2
#define TURNS_NUMBER 3

#define UP_DIR       0
#define RIGHT_DIR    1
#define DOWN_DIR     2
#define LEFT_DIR     3
#define DIRS_NUMBER  4

struct {
   int              character;
   int              new_direction;
} moves_table [DIRS_NUMBER][TURNS_NUMBER];

/* initialize movements table, defined just above */
void init_moves_table(void)
{
    moves_table[UP_DIR  ][TURN_L   ].character = ascii_chars
            ? '.'
            : ACS_URCORNER;
    moves_table[UP_DIR  ][STRAIGHT ].character = ascii_chars
            ? '|'
            : ACS_VLINE;
    moves_table[UP_DIR  ][TURN_R   ].character = ascii_chars
            ? ','
            : ACS_ULCORNER;
    moves_table[LEFT_DIR][TURN_L   ].character = ascii_chars
            ? ','
            : ACS_ULCORNER;
    moves_table[LEFT_DIR][STRAIGHT ].character = ascii_chars
            ? '-'
            : ACS_HLINE;
    moves_table[LEFT_DIR][TURN_R   ].character = ascii_chars
            ? '`'
            : ACS_LLCORNER;
    moves_table[DOWN_DIR][TURN_L   ].character = ascii_chars
            ? '`'
            : ACS_LLCORNER;
    moves_table[DOWN_DIR][STRAIGHT ].character = ascii_chars
            ? '|'
            : ACS_VLINE;
    moves_table[DOWN_DIR][TURN_R   ].character = ascii_chars
            ? '\''
            : ACS_LRCORNER;
    moves_table[RIGHT_DIR][TURN_L  ].character = ascii_chars
            ? '\''
            : ACS_LRCORNER;
    moves_table[RIGHT_DIR][STRAIGHT].character = ascii_chars
            ? '-'
            : ACS_HLINE;
    moves_table[RIGHT_DIR][TURN_R  ].character = ascii_chars
            ? '.'
            : ACS_URCORNER;

    moves_table[UP_DIR  ][TURN_L   ].new_direction = LEFT_DIR;
    moves_table[UP_DIR  ][STRAIGHT ].new_direction = UP_DIR;
    moves_table[UP_DIR  ][TURN_R   ].new_direction = RIGHT_DIR;
    moves_table[LEFT_DIR][TURN_L   ].new_direction = DOWN_DIR;
    moves_table[LEFT_DIR][STRAIGHT ].new_direction = LEFT_DIR;
    moves_table[LEFT_DIR][TURN_R   ].new_direction = UP_DIR;
    moves_table[DOWN_DIR][TURN_L   ].new_direction = RIGHT_DIR;
    moves_table[DOWN_DIR][STRAIGHT ].new_direction = DOWN_DIR;
    moves_table[DOWN_DIR][TURN_R   ].new_direction = LEFT_DIR;
    moves_table[RIGHT_DIR][TURN_L  ].new_direction = UP_DIR;
    moves_table[RIGHT_DIR][STRAIGHT].new_direction = RIGHT_DIR;
    moves_table[RIGHT_DIR][TURN_R  ].new_direction = DOWN_DIR;
} /* init_moves_table */

/* o First we calculate the new movement direction, put in the
 *   screen the character corresponding to the actual heading and
 *   the new heading.  Adjust the new direction.
 * o Check if the worm can move to the new position.
 *   If so, move the worm head.
 *   If not, move the worm tail.
 * o The worm can move to the new address if it doesn't get
 *   longer than the maximum allowed length and if the new position
 *   is not filled by another worm.
 * o To move the head we put the head pointing to the next segment.
 *   We increase the segment head, including a new segment in the
 *   circular buffer of segments, and marking the board position as
 *   busy.
 * o We advance the pointers to the tail when a segment is to be
 *   eliminated from the tail.
 */
void do_move_worm (worm *w)
{
    segment *head     = &w->segments_array[w->head];
    segment *tail     = &w->segments_array[w->tail];
    int      curr_hdg = w->direction;

    int move_type     = STRAIGHT;
    if (my_random(100) <= heading_chg_probab) {
        move_type     = my_random(2)
                ? TURN_L
                : TURN_R;
    }

    int fut_hdg = moves_table [curr_hdg][move_type].new_direction,
        x_pos         = head->x,
        y_pos         = head->y; /* current segment */

    /* calculate next position */
    switch (fut_hdg) {
    case UP_DIR:    --y_pos; break;
    case DOWN_DIR:  ++y_pos; break;
    case LEFT_DIR:  --x_pos; break;
    case RIGHT_DIR: ++x_pos; break;
    }

    /* if nothing happens we'll move */
    int can_move = true;

    /* if out of board, cannot move */
    if (       x_pos < 0 || x_pos >= COLS
            || y_pos < 0 || y_pos >= LINES)
        can_move = false;

    /* if this cell is already occupied, cannot move there */
    if (can_move && board[x_pos][y_pos])
        can_move = false;

    /* if length = max we cannot move */
    if (w->curr_length == w->max_length)
        can_move = false;

    if (can_move) { /* do move */

        /* draw the worm neck where the head was */
#if USE_COLORS
        do_print(moves_table[curr_hdg][move_type].character,
            head->x, head->y, w->color);
#else
        do_print(moves_table[curr_hdg][move_type].character,
            head->x, head->y);
#endif

        /* grow a worm segment */
        ++head; ++w->head;
        if (w->head == w->max_length) {
            head       = w->segments_array;
            w->head  = 0;
        }
        head->x = x_pos;
        head->y = y_pos;

        /* grow the curent length */
        ++w->curr_length;

        /* mark the position on the board */
        board[x_pos][y_pos] = true;

        /* draw the new head */
#if USE_COLORS
        do_print('@', x_pos, y_pos, w->color);
#else
        do_print('@', x_pos, y_pos);
#endif

        /* adjust the heading */
        w->direction = fut_hdg;

    } else if (w->curr_length > 1) {
        /*     ^^^^^^^^^^^^^^^^^^^
         * only eliminate if length is bigger than 1 */

        x_pos = tail->x;
        y_pos = tail->y;

        /* clean the freed square */
#if USE_COLORS
        do_print(' ', x_pos, y_pos, 0);
#else
        do_print(' ', x_pos, y_pos);
#endif

        /* adjust the length */
        --w->curr_length;

        /* unmark the board cell */
        board[x_pos][y_pos] = false;

        /* To eliminate the tail from the worm list,
         * just advance the tail reference */
        ++tail; ++w->tail;
        if (w->tail == w->max_length) {
            tail     = w->segments_array;
            w->tail  = 0;
        }
    } /* if */
} /* do_move_worm */

void do_usage(void)
{
    printf("Usage: worms [ -s ] [ -p prob ] [ max_sz ... ]\n");
    printf("paremeters:\n");
    printf("   -p prob allows to indicate the probability of heading\n"
           "           change of the worms.\n");
    printf("   -d      debugging run (shows verbose output).\n");
    printf("   -s      don't use delay between refreshes.\n");
    printf("   -a      use ASCII characters for worms, instead of\n"
           "           box chars.\n");
#if USE_COLORS
    printf("   -c      don't use colors.\n");
#endif
    printf("   max_sz  use this maximum size for the i-esim worm\n");
} /* do_usage */

/* MAIN PROGRAM */
int main(int argc, char **argv)
{
    worm  *current_worm;
    int    opt,
           i;
    int    saved_argc = argc;
    char **saved_argv = argv;
    int    old_LINES,
           old_COLS;

#if USE_COLORS
    while ((opt = getopt(argc, argv, "p:dcsa")) != EOF) {
#else
    while ((opt = getopt(argc, argv, "p:dsa")) != EOF) {
#endif
        switch(opt){
        case 'd': debug        = true;         break;
#if USE_COLORS
        case 'c': want_colors  = false;        break;
#endif
        case 'p':
            heading_chg_probab = atoi(optarg); break;
        case 's':
            delay_flag         = false;        break;
        case 'a':
            ascii_chars        = true;         break;
        case 'h':
        default: do_usage(); exit(0);
        } /* switch */
    } /* while */

    argc -= optind;
    argv += optind;

    srandom(time(NULL));

    /* LET'S START... */
    initscr();

#if USE_COLORS /*     { */
    if (want_colors && has_colors()) {
        start_color();
    }
    if (debug) {
        printw("has_colors()     == %d\n", has_colors());
        printw("COLOR_PAIRS      == %d\n", COLOR_PAIRS);
        printw("CAN_CHANGE_COLOR == %d\n", can_change_color());
    } /* if */

    colors = 0;

    /* check if we should use colors */
    if (want_colors && has_colors()) {

        /* if (can_change_color()) { */

#define DEFCOL(X) do {  /*           { */                \
                if (COLOR_PAIRS > colors)                \
                    init_pair(colors++, X, COLOR_BLACK); \
            } while (0) /* DEFCOL(X) } */

        DEFCOL(COLOR_RED);
        DEFCOL(COLOR_GREEN);
        DEFCOL(COLOR_YELLOW);
        DEFCOL(COLOR_BLUE);
        DEFCOL(COLOR_MAGENTA);
        DEFCOL(COLOR_CYAN);
        DEFCOL(COLOR_WHITE);

        /* } else colors = COLOR_PAIRS; */

    } /* if */

    if (debug) {
        printw("COLORS == %d\n", colors);
    } /* if */

#endif /* USE_COLORS  } */

    /* initialize movements table */
    init_moves_table();

    /* initialize board */
    board = calloc(COLS, sizeof(*board));
    for (i = 0; i < COLS; i++)
        board[i] = calloc(LINES, sizeof(*board[i]));

    /* INIT THE WORMS */
    worms_number = argc;
    if (worms_number == 0) {
        worms_number = my_random(MAX_WORMS - MIN_WORMS)
                + MIN_WORMS;
    }

    /* create the array of worms */
    worms_array = calloc(worms_number, sizeof(worms_array[0]));

    {   /* phase #1 */
        worm *w = worms_array;
        for (int i = 0; i < worms_number; ++i, ++w) {
#if USE_COLORS
            if (want_colors)
                w->color = my_random(colors);

            if (debug)
                printw("WORM #%d COLOR: %d\n", i, w->color);
#endif

            if (i < argc)
                w->max_length = atoi( argv[i] );
            else
                w->max_length = 0;

            if (w->max_length < MIN_LENGTH)
                w->max_length = my_random( MAX_LENGTH - MIN_LENGTH )
                        + MIN_LENGTH;

            /* add the maximum worm length to the segments_number
             * to know the actual number of segments we will
             * allocate */
            segments_number += w->max_length;
        } /* while */
    }

    /* GET THE ARRAY OF SEGMENTS */
    segments_array = calloc(
            segments_number,
            sizeof segments_array[0]);

    {   /* worm initialization (the first
         * segment with the worm head) */

        worm    *g = worms_array;
        segment *s = segments_array;
        for (   int i = 0;
                i < worms_number;
                ++i, s += g->max_length, ++g)
        {
            g->segments_array  = s;
            segment *segm      = g->segments_array;
            segm->x            = my_random(COLS);
            segm->y            = my_random(LINES);
            g->direction       = my_random(4);
            g->curr_length      = 1;
            g->head            = 0;
            g->tail            = 0;
        }
    }

#if USE_COLORS
    if(debug) {
        refresh();
        sleep(5);
    } /* if */
#endif

    /* START RUNNING THE LOOP */
    old_LINES = LINES;
    old_COLS  = COLS;

    while (true) {
        /* window geometry is handled internally by curses */
        if ( LINES != old_LINES || COLS != old_COLS ) {
            endwin();
            execvp(saved_argv[0], saved_argv);
        } /* if */

        /* move all worms */
        const worm *end_worms    = worms_array + worms_number;
        for ( worm *current_worm = worms_array;
              current_worm < end_worms;
              ++current_worm)
        {
            do_move_worm (current_worm);
        }

        /* put the cursor at the lowest left position,
         * so it doesn't seem to follow the last worm
         * in the set */
        move(LINES-1, 0);

        /* draw */
        refresh();

        DELAY();
    } /* while */
} /* main */
