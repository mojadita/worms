/*
                   :COMINFO:
       Nombre      :gusanos.c:
       Autor       :Luis Colorado Urcola:
       Expediente  :00000000:
       Fecha Com.  :9/11/89:
       Fecha Fin   :12/11/89:
       Versi¢n     :2.02:
       Fich. Rel.  ::
                   :FININFO:

*/
/*
       Descripci¢n:
       ============

       Este programa crea unos gusanos que se comen el contenido de la
       pantalla.

       Uso:
       ====

       gusanos [-prob] [long ...]

       El primer par metro, indicado por un signo menos delante del n£mero
       indica la probabilidad de giro de los gusanos. Debe ser un valor entre
       0 y 100.
       Los dem s par metros indican la longitud m xima del gusano creado.
       Si se introduce una longitud no v lida se genera una longitud al azar
       entre MIN_LONGITUD y MAX_LONGITUD.
       Si no se especifican parametros de longitud se genera un n£mero al
       azar de gusanos de longitudes al azar. El n£mero de gusanos estar 
       entre MIN_GUSANOS y MAX_GUSANOS.

*/


#include <conio.h>
#include <dos.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PROB_C_DIREC    50
#define TRUE            1
#define FALSE           0
#define LINEAS          25
#define COLUMNAS        80
#define MIN_LONGITUD    3
#define MAX_LONGITUD    100
#define MIN_GUSANOS     3
#define MAX_GUSANOS     30
#define RETARDO()	

/* Autor: Luis Colorado Urcola */

/* Subrutina mem_vid () que detecta el comienzo de la memoria de
   video */

/* el valor inicial no debe cambiarse pues se utiliza para no tener que lla-
	mar m s de una vez a la rutina que obtiene el modo de video. */
static char far *puntero = NULL;

char far *mem_vid ()
{
        union REGS regs;

	/* si ya hemos inicializado la variable, retornamos su valor */
	if (puntero != NULL) return puntero;

        regs.h.ah = 0x0f; /* leer modo de video */
        int86 (0x10, &regs, &regs);

	/* en funci¢n del modo obtenido ajustamos el valor de puntero */
        switch (regs.h.al) { /* el modo se retorna en al */

        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
	     puntero = (char far *)(0xb8000000L);
	     break;

        case 7:
	     puntero = (char far *)(0xb0000000L);
	     break;

        case 0x0d: case 0x0e: case 0x0f: case 0x10:
	     puntero = (char far *)(0xa0000000L);
	     break;

        default: return NULL;
        }
	/* devolvemos el valor de puntero */
	return puntero;
}

imprime(c, x, y, col)
{
        int dir;
	dir = (COLUMNAS * y + x)<<1;
        mem_vid () [ dir ] = c;
        mem_vid () [ dir | 1 ] = col;
}




typedef struct posicion {
       int      x;
       int      y;
       struct posicion *sig;
}posicion, *refposicion;


typedef struct worm {
       int color;
       int max_longitud;
       int longitud_act;
       int direccion;
       refposicion lista_puntos;
       struct worm *sig;
} worm, *refworm;

int tablero [COLUMNAS][LINEAS];
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
}

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
}


#define GIRO_I       0
#define RECTO        1
#define GIRO_D       2

#define ARRIBA       0
#define DERECHA      1
#define ABAJO        2
#define IZQUIERDA    3

struct {
       char caracter;
       int  nueva_dir;
} tabla_movim [4][3] = { /* GIRO_I            RECTO           GIRO_D */
                         /* ======            =====           ====== */
     /* ARRIBA */      '¿', IZQUIERDA,   '³', ARRIBA,    'Ú', DERECHA,
     /* DERECHA */     'Ù', ARRIBA,      'Ä', DERECHA,   '¿', ABAJO,
     /* ABAJO */       'À', DERECHA,     '³', ABAJO,     'Ù', IZQUIERDA,
     /* IZQUIERDA */   'Ú', ABAJO,       'Ä', IZQUIERDA, 'À', ARRIBA
                       };

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
      if (random (100) <= prob_c_direc)
         if (random (2)) tipo_movim = GIRO_I;
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
         imprime (tabla_movim [dir_act][tipo_movim].caracter,
                 cabe->x,
                 cabe->y,
                 w->color);

         /* creamos la nueva cabecera */
         crea_pos (w, posx, posy);

         /* marcamos la nueva posici¢n */
         tablero [posx][posy] = TRUE;

         /* pintamos la nueva cabeza */
         imprime ((char) 01, posx, posy, w->color);

         /* ajustamos la nueva direccion **/
         w->direccion = dir_fut;
      }
      else { /* quitamos de la cola */
         /* solo eliminamos si la longitud actual es mayor que 1 */
         if (w->longitud_act > 1) {
            posx = cola->x;
            posy = cola->y;

            /* limpiamos la zona de la pantalla afectada */
            imprime (' ', posx, posy, w->color);

            /* limpiamos la zona del tablero afectada */
            tablero [posx][posy] = FALSE;

            /* eliminamos la posici¢n de cola del worm */
            dest_pos (w);
         }
      }
}

refworm lista_gusanos = NULL; /* lista con los gusanos */

main (argc, argv)
char *argv [];
{
      refworm gusano_actual;
      argc--;
      argv++;
      randomize ();
      while (argc){
            refworm p;
            p = (refworm) malloc (sizeof (worm));
	    if (mem_vid () == (char far *) 0xb0000000L)
	            p->color = (random (2) << 3)|7;
	    else
	            p->color =1 + random (15);
            p->max_longitud = atoi (argv [0]);
            if (p->max_longitud < 0) {
               prob_c_direc = - p->max_longitud;
               argc--;
               argv++;
               continue;
            }
            if (p->max_longitud < 2)
               p->max_longitud = MIN_LONGITUD +
                                 random (MAX_LONGITUD - MIN_LONGITUD);
            p->longitud_act = 1;
            p->direccion = random (4);
            p->lista_puntos = (refposicion) malloc (sizeof (posicion));
            p->lista_puntos->x = random (COLUMNAS);
            p->lista_puntos->y = random (LINEAS);
            p->lista_puntos->sig = p->lista_puntos;
            p->sig = lista_gusanos;
            lista_gusanos = p;
            argc--;
            argv++;
      }
      if (lista_gusanos == NULL) {
         argc = random (MAX_GUSANOS - MIN_GUSANOS) + MIN_GUSANOS;
         while (argc){
            refworm p;
            p = (refworm) malloc (sizeof (worm));
	    if (mem_vid () == (char far *) 0xb0000000L)
	            p->color = (random (2) << 3)|7;
	    else
	            p->color =1 + random (15);
            p->max_longitud = MIN_LONGITUD +
                              random (MAX_LONGITUD - MIN_LONGITUD);
            p->longitud_act = 1;
            p->direccion = random (4);
            p->lista_puntos = (refposicion) malloc (sizeof (posicion));
            p->lista_puntos->x = random (COLUMNAS);
            p->lista_puntos->y = random (LINEAS);
            p->lista_puntos->sig = p->lista_puntos;
            p->sig = lista_gusanos;
            lista_gusanos = p;
            argc--;
         }
      }

      gusano_actual = lista_gusanos;
      while (kbhit ()) getch ();
      while (TRUE){
            mueve_worm (gusano_actual);
            gusano_actual = gusano_actual->sig;
            if (gusano_actual == NULL){
	           gusano_actual = lista_gusanos;
	       RETARDO();
            }
            if (kbhit ()) {
               getch ();
            }
      }
}
