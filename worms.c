/* $Id: gusanos.c,v 1.10 2014/02/11 10:04:04 luis Exp $
 * vim: ts=4 sw=4 nu ai
 * Author: Luis.Colorado@HispaLinux.ES
 * Date: Sat Mar 11 22:05:03 MET 2000
 * Version UNIX, con ncurses.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <getopt.h>

#define PROB_C_DIREC    50
#define LINEAS          LINES
#define COLUMNAS        COLS
#define MIN_LONGITUD    3
#define MAX_LONGITUD    1000
#define MIN_GUSANOS     3
#define MAX_GUSANOS     30
#ifndef RETARDO
#define RETARDO()       do {if (delay_flag) usleep(20000);} while(0)
#endif
#define my_random(X) (random()%(X))

#ifndef USE_COLORS
#define USE_COLORS 1
#endif

#if USE_COLORS
int colors;
int want_colors = true;
#endif
int debug       = false;
int delay_flag  = true;
int ascii_chars = false;
int has_changed_window_size = false;

#if USE_COLORS
void imprime(int c, int x, int y, int col)
{
    mvaddch(y,x, COLOR_PAIR(col) | c);
}
#else
void imprime(int c, int x, int y)
{
    mvaddch(y,x, c);
}
#endif

/* estructuras que definen al gusano */
typedef struct posicion {
   short            x;
   short            y;
}posicion;

typedef struct worm {
#if USE_COLORS
   int              color;
#endif
   int              max_longitud;
   int              longitud_act;
   int              direccion;
   struct posicion *array_segmentos;
   int              cabeza,
                    cola;
} worm;

worm            *array_gusanos    = NULL; /* lista con los gusanos */
struct posicion *array_posiciones = NULL; /* array con las posiciones */
int numero_de_posiciones = 0,
	numero_de_gusanos    = 0;

int **tablero;
int prob_c_direc = PROB_C_DIREC;



#define GIRO_I       0
#define RECTO        1
#define GIRO_D       2
#define NUM_GIROS    3

#define ARRIBA       0
#define DERECHA      1
#define ABAJO        2
#define IZQUIERDA    3
#define NUM_POSIC    4

struct {
   int              caracter;
   int              nueva_dir;
} tabla_movim [NUM_POSIC][NUM_GIROS];

/* inicializamos la tabla de movimientos, definida justo aquí arriba */
void init_tabla_movim(void)
{
    tabla_movim[ARRIBA   ][GIRO_I].caracter = ascii_chars ? '.'  : ACS_URCORNER;
    tabla_movim[ARRIBA   ][RECTO ].caracter = ascii_chars ? '|'  : ACS_VLINE;
    tabla_movim[ARRIBA   ][GIRO_D].caracter = ascii_chars ? ','  : ACS_ULCORNER;
    tabla_movim[IZQUIERDA][GIRO_I].caracter = ascii_chars ? ','  : ACS_ULCORNER;
    tabla_movim[IZQUIERDA][RECTO ].caracter = ascii_chars ? '-'  : ACS_HLINE;
    tabla_movim[IZQUIERDA][GIRO_D].caracter = ascii_chars ? '`'  : ACS_LLCORNER;
    tabla_movim[ABAJO    ][GIRO_I].caracter = ascii_chars ? '`'  : ACS_LLCORNER;
    tabla_movim[ABAJO    ][RECTO ].caracter = ascii_chars ? '|'  : ACS_VLINE;
    tabla_movim[ABAJO    ][GIRO_D].caracter = ascii_chars ? '\'' : ACS_LRCORNER;
    tabla_movim[DERECHA  ][GIRO_I].caracter = ascii_chars ? '\'' : ACS_LRCORNER;
    tabla_movim[DERECHA  ][RECTO ].caracter = ascii_chars ? '-'  : ACS_HLINE;
    tabla_movim[DERECHA  ][GIRO_D].caracter = ascii_chars ? '.'  : ACS_URCORNER;

    tabla_movim[ARRIBA   ][GIRO_I].nueva_dir = IZQUIERDA;
    tabla_movim[ARRIBA   ][RECTO ].nueva_dir = ARRIBA;
    tabla_movim[ARRIBA   ][GIRO_D].nueva_dir = DERECHA;
    tabla_movim[IZQUIERDA][GIRO_I].nueva_dir = ABAJO;
    tabla_movim[IZQUIERDA][RECTO ].nueva_dir = IZQUIERDA;
    tabla_movim[IZQUIERDA][GIRO_D].nueva_dir = ARRIBA;
    tabla_movim[ABAJO    ][GIRO_I].nueva_dir = DERECHA;
    tabla_movim[ABAJO    ][RECTO ].nueva_dir = ABAJO;
    tabla_movim[ABAJO    ][GIRO_D].nueva_dir = IZQUIERDA;
    tabla_movim[DERECHA  ][GIRO_I].nueva_dir = ARRIBA;
    tabla_movim[DERECHA  ][RECTO ].nueva_dir = DERECHA;
    tabla_movim[DERECHA  ][GIRO_D].nueva_dir = ABAJO;
} /* init_tabla_movim */

/* -- primero calculamos la nueva direcci¢n,
      colocamos en la pantalla el caracter correspondiente a la direcci¢n
      actual y la nueva direcci¢n. Ajustamos la nueva direcci¢n.
   -- Comprobamos si se puede mover el worm en la nueva direcci¢n.
      Si es as¡, movemos la cabeza.
      Si no es as¡, movemos la cola.
   -- El worm se podr  mover en la nueva direcci¢n si no sobrepasa su lon-
      gitud m xima permitida y si la casilla a donde va no est  ocupada.
   -- Para mover la cabeza se coloca el worm en la nueva posici¢n.
      Se aumenta la posici¢n de cabeza incluyendo la nueva posici¢n en la
      lista de posiciones visitadas de cada worm.
   -- Para mover la cola
*/
void mueve_worm (worm *w)
{
    posicion *cabe    = &w->array_segmentos[w->cabeza];
    posicion *cola    = &w->array_segmentos[w->cola];
    int       dir_act = w->direccion;

    int tipo_movim = RECTO;
    if (my_random (100) <= prob_c_direc) {
        tipo_movim = my_random(2)
                ? GIRO_I
                : GIRO_D;
    }

    int dir_fut = tabla_movim [dir_act][tipo_movim].nueva_dir,
        posx    = cabe->x,
        posy    = cabe->y; /* nueva posici¢n */

    /* calculamos la nueva posici¢n */
    switch (dir_fut) {
	case ARRIBA:    --posy; break;
	case ABAJO:     ++posy; break;
	case IZQUIERDA: --posx; break;
	case DERECHA:   ++posx; break;
    }

    /* si nada lo impide, podremos mover */
    int podemos_mover = true;

    /* si estamos fuera del tablero, no podremos mover */
    if (       posx < 0 || posx >= COLUMNAS
            || posy < 0 || posy >= LINEAS)
        podemos_mover = false;

    /* si esta posici¢n ya esta ocupada no podremos mover */
    if (podemos_mover && tablero [posx][posy])
        podemos_mover = false;

    /* si la longitud es la longitud maxima, no podremos mover */
    if (w->max_longitud == w->longitud_act)
        podemos_mover = false;

    if (podemos_mover) { /* movemos */
        /* imprimimos el cuello en la antigua posici¢n */
#if USE_COLORS
        imprime(tabla_movim[dir_act][tipo_movim].caracter,
            cabe->x, cabe->y, w->color);
#else
        imprime(tabla_movim[dir_act][tipo_movim].caracter,
            cabe->x, cabe->y);
#endif

        /* crecemos por la cabeza */
        ++cabe; ++w->cabeza;
        if (w->cabeza >= w->max_longitud) {
            cabe      -= w->max_longitud;
            w->cabeza -= w->max_longitud;
		}
		cabe->x = posx;
		cabe->y = posy;
		++w->longitud_act;

        /* marcamos la nueva posici¢n */
        tablero[posx][posy] = true;

        /* pintamos la nueva cabeza */
#if USE_COLORS
        imprime('O', posx, posy, w->color);
#else
        imprime('O', posx, posy);
#endif

        /* ajustamos la nueva direccion **/
        w->direccion = dir_fut;

    } else { /* quitamos de la cola */

        /* solo eliminamos si la longitud actual es mayor que 1 */
        if (w->longitud_act > 1) {
            posx = cola->x;
            posy = cola->y;

        /* limpiamos la zona de la pantalla afectada */
#if USE_COLORS
            imprime (' ', posx, posy, 0);
#else
            imprime (' ', posx, posy);
#endif

            /* limpiamos la zona del tablero afectada */
            tablero[posx][posy] = false;

            /* eliminamos la posici¢n de cola del worm */
			++cola; ++w->cola;
			if (w->cola >= w->max_longitud) {
				cola    -= w->max_longitud;
				w->cola -= w->max_longitud;
			}

        } /* if */
    } /* if */
} /* mueve_worm */

void do_usage(void)
{
    printf("Uso: gusanos [ -s ] [ -p prob ] [ tam ... ]\n");
    printf("parámetros:\n");
    printf("   -p prob permite indicar la probabilidad de cambio de dirección de los\n");
    printf("           gusanos.\n");
    printf("   -d      opción de depurado.\n");
    printf("   -s      no usar retardo.\n");
    printf("   -a      usar caracteres ASCII para representar los gusanos.\n");
#if USE_COLORS
    printf("   -c      opción de eliminación de colores.  No usa colores.\n");
#endif
    printf("   tam     indica el tamaño en caracteres del gusano.\n");
} /* do_usage */

/* PROGRAMA PRINCIPAL */
int main(int argc, char **argv)
{
    worm  *gusano_actual;
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
        case 'd': debug       = true;         break;
#if USE_COLORS
        case 'c': want_colors = false;        break;
#endif
        case 'p':
            prob_c_direc      = atoi(optarg); break;
        case 's':
            delay_flag        = false;        break;
        case 'a':
            ascii_chars       = true;         break;
        case 'h':
        default:
            do_usage(); exit(0);
        } /* switch */
    } /* while */

    argc -= optind;
    argv += optind;

    srandom(time(NULL));
    initscr();

#if USE_COLORS
    start_color();
    if (debug) {
        printw("has_colors()     == %d\n", has_colors());
        printw("COLOR_PAIRS      == %d\n", COLOR_PAIRS);
        printw("CAN_CHANGE_COLOR == %d\n", can_change_color());
    } /* if */
    colors = 0;
    if (want_colors && has_colors()) {
    /* if (can_change_color()) { */
#define DEFCOL(X) do {                                     \
                if (COLOR_PAIRS > colors)                  \
                    init_pair(colors++, X, COLOR_BLACK);   \
            } while (0) /* DEFCOL(X) */
        DEFCOL(COLOR_RED);
        DEFCOL(COLOR_GREEN);
        DEFCOL(COLOR_YELLOW);
        DEFCOL(COLOR_BLUE);
        DEFCOL(COLOR_MAGENTA);
        DEFCOL(COLOR_CYAN);
    /*} else colors = COLOR_PAIRS; */
    } /* if */
    if (debug) {
        printw("COLORS == %d\n", colors);
    } /* if */

#endif

    /* inicializamos la tabla de movimientos */
    init_tabla_movim();

    /* inicializamos el tablero */
    tablero = calloc(COLUMNAS, sizeof(*tablero));
    for (i = 0; i < COLUMNAS; i++)
        tablero[i] = calloc(LINEAS, sizeof(*tablero[i]));

    numero_de_gusanos = argc;
    if (numero_de_gusanos == 0) {
        numero_de_gusanos = my_random(MAX_GUSANOS - MIN_GUSANOS) + MIN_GUSANOS;
    }

	/* creamos el array de gusanos */
    array_gusanos = calloc(numero_de_gusanos, sizeof(array_gusanos[0]));

    {   /* bloque local */
        worm *p                  = array_gusanos;
        for (int i = 0; i < numero_de_gusanos; ++i, ++p) {
    #if USE_COLORS
            p->color = has_colors() ? my_random(colors) : 0;

            if (debug)
                printw("COLOR DEL GUSANO<%d>: %d\n", i, p->color);
    #endif

            if (i < argc)
                p->max_longitud = atoi( argv[i] );
            else
                p->max_longitud = 0;

            if (p->max_longitud < MIN_LONGITUD)
                p->max_longitud = my_random( MAX_LONGITUD - MIN_LONGITUD )
                        + MIN_LONGITUD;

            numero_de_posiciones += p->max_longitud;
        } /* while */
    }

    array_posiciones = calloc(
            numero_de_posiciones,
            sizeof array_posiciones[0]);

    {   /* contenido de los gusanos (el primer segmento) */
        worm     *g = array_gusanos;
        posicion *p = array_posiciones;
        for (int i = 0; i < numero_de_gusanos; ++i, p += g->max_longitud, ++g) {
            g->array_segmentos  = array_posiciones;
            posicion *segm      = g->array_segmentos;
            segm->x             = my_random(COLUMNAS);
            segm->y             = my_random(LINEAS);
            g->direccion        = my_random(4);
            g->longitud_act     = 1;
            g->cabeza           = 0;
            g->cola             = 0;
        }
    }

#if USE_COLORS
    if(debug) {
        refresh();
        sleep(5);
    } /* if */
#endif

    old_LINES = LINES;
    old_COLS  = COLS;
    while (true){
        if (LINES != old_LINES || COLS != old_COLS) {
            int i;
            endwin();
            execvp(saved_argv[0], saved_argv);
        } /* if */
        gusano_actual = array_gusanos;
        for (int i = 0; i < numero_de_gusanos; ++i, ++gusano_actual) {
            mueve_worm (gusano_actual);
        }
        move(LINES-1, 0);
        refresh();
        RETARDO();
    } /* while */
} /* main */

/* $Id: gusanos.c,v 1.10 2014/02/11 10:04:04 luis Exp $ */
