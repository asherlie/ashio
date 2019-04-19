#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#include "raw.h"

/* reads a line from stdin until an \n is found or a tab is found 
 * returns NULL on ctrl-c
 */
char* getline_raw(int* bytes_read, _Bool* tab){
      struct termios tiop, def;
      tcgetattr(0, &tiop);
      tcgetattr(0, &def);
      cfmakeraw(&tiop);
      tcsetattr(0, TCSANOW, &tiop);
      char c;

      int buf_sz = 2;
      char* ret = calloc(buf_sz, 1);

      *tab = *bytes_read = 0;

      while((c = fgetc(stdin)) != '\r'){
            if(c == 3){
                  free(ret);
                  ret = NULL;
                  break;
            }
            /* if tab is detected */
            if(c == 9){
                  *tab = 1;
                  break;
            }
            /* delete */
            if(c == 127){
                  if(*bytes_read == 0)continue;
                  ret[--(*bytes_read)] = 0;
                  printf("\r%s%c\r%s", ret, ' ', ret);
                  continue;
            }
            if(*bytes_read == buf_sz){
                  buf_sz *= 2;
                  char* tmp_s = calloc(buf_sz, 1);
                  memcpy(tmp_s, ret, *bytes_read);
                  free(ret);
                  ret = tmp_s;
            }
            ret[(*bytes_read)++] = c;
            putchar(c);
      }
      /* before exiting, we restore term to its
       * default settings
       */
      tcsetattr(0, TCSANOW, &def);

      return ret;
}
