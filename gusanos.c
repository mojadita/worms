/* $Id: gusanos.c,v 1.6 2010/01/03 16:01:35 luis Exp $
 * Author: Luis.Colorado@HispaLinux.ES
 * Date: Sat Mar 11 22:05:03 MET 2000
 * Version UNIX, con ncurses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <getopt.h>

#define PROB_C_DIREC    50
#ifndef FALSE
#define TRUE            1
#define FALSE           0
#endif
#define LINEAS          LINES
#define COLUMNAS        COLS
#define MIN_LONGITUD    3
#define MAX_LONGITUD    100
#define MIN_GUSANOS     3
#define MAX_GUSANOS     30
#ifndef RETARDO
#define RETARDO()		if (delay_flag) usleep(2000);
#endif
#define my_random(X) (random()%(X))

#ifndef USE_COLORS
#define USE_COLORS 1
#endif

#if USE_COLORS
int colors;
int want_colors = TRUE;
#endif
int debug = 0;
int delay_flag = FALSE;
int ascii_chars = FALSE;

#if USE_COLORS
imprime(c, x, y, col)
{
		mvaddch(y,x, COLOR_PAIR(col) | c);
} /* imprime */
#else
imprime(c, x, y)
{
		mvaddch(y,x, c);
} /* imprime */
#endif

/* estructuras que definen al gusano */
typedef struct posicion {
       int      x;
       int      y;
       struct posicion *sig;
}posicion, *refposicion;

typedef struct worm {
#if USE_COLORS
       int color;
#endif
       int max_longitud;
       int longitud_act;
       int direccion;
       refposicion lista_puntos;
       struct worm *sig;
} worm, *refworm;

int **tablero;
int prob_c_direc = PROB_C_DIREC;

#define obten_pos_cabe(w)         ((w)->lista_puntos)

#define obten_pos_cola(w)         ((w)->lista_puntos->sig)

crea_pos (w, x, y)
refworm w;
{
         refposicion p;

         p = (refposicion) malloc (sizeof (posicion));
         p->x = x;
         p->y = y;
         p->sig = obten_pos_cola (w);
         w->lista_puntos->sig = p;
         w->lista_puntos = obten_pos_cola (w);
         w->longitud_act++;
} /* crea_pos */

dest_pos (w)
refworm w;
{
         refposicion p, q;

         p = obten_pos_cabe (w);
         /* solo borramos si hay mas de un nodo */
         if (p->sig == p) return;

         q = p->sig;
         p->sig = q->sig;
         free (q);
         /* ajustamos la nueva longitud */
         w->longitud_act--;
} /* dest_pos */


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
       int caracter;
       int nueva_dir;
} tabla_movim [NUM_POSIC][NUM_GIROS];

/* inicializamos la tabla de movimientos, definida justo aquí arriba */
init_tabla_movim()
{
	tabla_movim[ARRIBA   ][GIRO_I].caracter = ascii_chars ? '+' : ACS_URCORNER;
	tabla_movim[ARRIBA   ][RECTO ].caracter = ascii_chars ? '|' : ACS_VLINE;
	tabla_movim[ARRIBA   ][GIRO_D].caracter = ascii_chars ? '+' : ACS_ULCORNER;
	tabla_movim[IZQUIERDA][GIRO_I].caracter = ascii_chars ? '+' : ACS_ULCORNER;
	tabla_movim[IZQUIERDA][RECTO ].caracter = ascii_chars ? '-' : ACS_HLINE;
	tabla_movim[IZQUIERDA][GIRO_D].caracter = ascii_chars ? '+' : ACS_LLCORNER;
	tabla_movim[ABAJO    ][GIRO_I].caracter = ascii_chars ? '+' : ACS_LLCORNER;
	tabla_movim[ABAJO    ][RECTO ].caracter = ascii_chars ? '|' : ACS_VLINE;
	tabla_movim[ABAJO    ][GIRO_D].caracter = ascii_chars ? '+' : ACS_LRCORNER;
	tabla_movim[DERECHA  ][GIRO_I].caracter = ascii_chars ? '+' : ACS_LRCORNER;
	tabla_movim[DERECHA  ][RECTO ].caracter = ascii_chars ? '-' : ACS_HLINE;
	tabla_movim[DERECHA  ][GIRO_D].caracter = ascii_chars ? '+' : ACS_URCORNER;

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
mueve_worm (w)
refworm w;
{
      refposicion cabe, cola;
      int posx, posy, dir_act, dir_fut, tipo_movim, podemos_mover;

      cabe = obten_pos_cabe (w);
      cola = obten_pos_cola (w);
      dir_act = w->direccion;

      tipo_movim = RECTO;
      if (my_random (100) <= prob_c_direc)
         if (my_random (2)) tipo_movim = GIRO_I;
         else tipo_movim = GIRO_D;



      dir_fut = tabla_movim [dir_act][tipo_movim].nueva_dir;
      posx = cabe->x;
      posy = cabe->y; /* nueva posici¢n */

      /* calculamos la nueva posici¢n */
      switch (dir_fut){
      case ARRIBA:
           posy--;
           break;
      case ABAJO:
           posy++;
           break;
      case IZQUIERDA:
           posx--;
           break;
      case DERECHA:
           posx++;
      }

      /* si nada lo impide, podremos mover */
      podemos_mover = TRUE;

      /* si estamos fuera del tablero, no podremos mover */
      if (posx < 0 || posx >= COLUMNAS || posy < 0 || posy >= LINEAS)
         podemos_mover = FALSE;

      /* si esta posici¢n ya est  ocupada no podremos mover */
      if (podemos_mover && tablero [posx][posy])
         podemos_mover = FALSE;

      /* si la longitud es la longitud m xima, no podremos mover */
      if (w->max_longitud == w->longitud_act)
         podemos_mover = FALSE;

      if (podemos_mover){ /* movemos */
         /* imprimimos el cuello en la antigua posici¢n */
#if USE_COLORS
         imprime (tabla_movim [dir_act][tipo_movim].caracter,
                 cabe->x,
                 cabe->y,
                 w->color);
#else
         imprime (tabla_movim [dir_act][tipo_movim].caracter,
                 cabe->x,
                 cabe->y);
#endif

         /* creamos la nueva cabecera */
         crea_pos (w, posx, posy);

         /* marcamos la nueva posici¢n */
         tablero [posx][posy] = TRUE;

         /* pintamos la nueva cabeza */
#if USE_COLORS
         imprime ('O', posx, posy, w->color);
#else
         imprime ('O', posx, posy);
#endif

         /* ajustamos la nueva direccion **/
         w->direccion = dir_fut;
      }
      else { /* quitamos de la cola */
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
            tablero [posx][posy] = FALSE;

            /* eliminamos la posici¢n de cola del worm */
            dest_pos (w);
         }
      }
} /* mueve_worm */

refworm lista_gusanos = NULL; /* lista con los gusanos */

void do_usage()
{
	printf("Uso: gusanos [ -s ] [ -p prob ] [ tam ... ]\n");
	printf("parametros:\n");
	printf("   -p prob permite indicar la probabilidad de cambio de direccion de los\n");
	printf("           gusanos.\n");
	printf("   -d      opción de depurado\n");
	printf("   -s      utilizar un delay de 1s.\n");
#if USE_COLORS
	printf("   -c      opción de eliminación de colores.  No usa colores\n");
#endif
	printf("   tam     indica el tamaóo en caracteres del gusano.\n");
} /* do_usage */

/* PROGRAMA PRINCIPAL */
main (argc, argv)
char *argv [];
{
      refworm gusano_actual;
	  int opt, i;

#if USE_COLORS
	  while ((opt = getopt(argc, argv, "p:dcsa")) != EOF) {
#else
	  while ((opt = getopt(argc, argv, "p:dsa")) != EOF) {
#endif
	  	switch(opt){
		case 'd': debug = TRUE; break;
#if USE_COLORS
		case 'c': want_colors = FALSE; break;
#endif
		case 'p':
			prob_c_direc = atoi(optarg);
			break;
		case 's':
			delay_flag = TRUE; break;
		case 'a':
			ascii_chars = TRUE; break;
		case 'h':
		default:
			do_usage(); exit(0);
		}
	  }
      argc -= optind;
      argv += optind;

      srandom (time(NULL));
	  initscr();
#if USE_COLORS
	  start_color();
	  if (debug) {
			  printw ("has_colors() == %d\n", has_colors());
			  printw ("COLOR_PAIRS == %d\n", COLOR_PAIRS);
			  printw ("CAN_CHANGE_COLOR == %d\n", can_change_color());
	  }
	  colors = 1;
	  if (want_colors && has_colors()) {
			  /* if (can_change_color()) { */
#define DEFCOL(X) if (COLOR_PAIRS > colors) init_pair(colors++, X, COLOR_BLACK);
				DEFCOL(COLOR_RED);
				DEFCOL(COLOR_GREEN);
				DEFCOL(COLOR_YELLOW);
				DEFCOL(COLOR_BLUE);
				DEFCOL(COLOR_MAGENTA);
				DEFCOL(COLOR_CYAN);
			  /*} else colors = COLOR_PAIRS; */
	  }
	  if (debug) {
	  	printw("COLORS == %d\n", colors);
	  }

#endif

	  /* inicializamos la tabla de movimientos */
	  init_tabla_movim();

	  /* inicializamos el tablero */
	  tablero = calloc(COLUMNAS, sizeof(int *));
	  for (i = 0; i < COLUMNAS; i++)
	  	tablero[i] = calloc(LINEAS, sizeof(int));

      while (argc){
            refworm p;
            p = (refworm) malloc (sizeof (worm));
#if USE_COLORS
	    p->color = has_colors() ? my_random (colors) : 0;
		if (debug)
				printw("COLOR DEL GUSANO: %d\n", p->color);
#endif
        p->max_longitud = atoi (argv [0]);
        if (p->max_longitud < 0) {
               prob_c_direc = - p->max_longitud;
               argc--;
               argv++;
               continue;
        }
        if (p->max_longitud < 2)
               p->max_longitud = MIN_LONGITUD +
                                 my_random (MAX_LONGITUD - MIN_LONGITUD);
        p->longitud_act = 1;
        p->direccion = my_random (4);
        p->lista_puntos = (refposicion) malloc (sizeof (posicion));
        p->lista_puntos->x = my_random (COLUMNAS);
        p->lista_puntos->y = my_random (LINEAS);
        p->lista_puntos->sig = p->lista_puntos;
        p->sig = lista_gusanos;
        lista_gusanos = p;
        argc--;
        argv++;
      } /* while */
      if (lista_gusanos == NULL) {
         argc = my_random (MAX_GUSANOS - MIN_GUSANOS) + MIN_GUSANOS;
         while (argc){
            refworm p;
            p = (refworm) malloc (sizeof (worm));
#if USE_COLORS
	        p->color = has_colors() ?my_random (colors) : 0;
			if (debug)
					printw("COLOR DEL GUSANO: %d\n", p->color);
#endif
            p->max_longitud = MIN_LONGITUD +
                              my_random (MAX_LONGITUD - MIN_LONGITUD);
            p->longitud_act = 1;
            p->direccion = my_random (4);
            p->lista_puntos = (refposicion) malloc (sizeof (posicion));
            p->lista_puntos->x = my_random (COLUMNAS);
            p->lista_puntos->y = my_random (LINEAS);
            p->lista_puntos->sig = p->lista_puntos;
            p->sig = lista_gusanos;
            lista_gusanos = p;
            argc--;
         }
      }
#if USE_COLORS
	  if(debug) {
	  	refresh();
	  	sleep(5);
	  }
#endif

      gusano_actual = lista_gusanos;
      while (TRUE){
            mueve_worm (gusano_actual);
            gusano_actual = gusano_actual->sig;
            if (gusano_actual == NULL){
	           gusano_actual = lista_gusanos;
			   refresh();
	       	   RETARDO();
            }
      }
} /* main */

/* $Id: gusanos.c,v 1.6 2010/01/03 16:01:35 luis Exp $ */
