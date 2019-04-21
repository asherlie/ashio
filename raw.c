#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#include "raw.h"

struct termios def, raw;

void raw_mode(){
      tcsetattr(0, TCSANOW, &raw);
}

/* reset_term can only be called after getline_raw has been */
void reset_term(){
      tcsetattr(0, TCSANOW, &def);
}

/* reads a line from stdin until an \n is found or a tab is found 
 * returns NULL on ctrl-c
 */
char* getline_raw(int* bytes_read, _Bool* tab, int* ignore){
      tcgetattr(0, &raw);
      tcgetattr(0, &def);
      cfmakeraw(&raw);
      raw_mode();
      char c;

      int buf_sz = 2;
      char* ret = calloc(buf_sz, 1);

      *tab = *bytes_read = 0;

      while((c = fgetc(stdin)) != '\r'){
            if(ignore){
                  for(int* i = ignore; *i > 0; ++i){
                        if(c == *i)continue;
                  }
            }
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
      reset_term();

      return ret;
}

/*char* tab_complete(char** opts, int optlen){*/
/* data_offset is offset into data where char* can be found
 * this is set to 0 if data_douplep is a char*
 */
/* tab_complete behaves like getline(), but does not include \n char in returned string
 */
char* tab_complete(void* data_douplep, int data_offset, int optlen, char iter_opts, int* bytes_read){
      _Bool tab, found_m;
      char* ret = getline_raw(bytes_read, &tab, NULL), * tmp_ch;
      if(tab){
            found_m = 0;
            _Bool select = 0;
            int maxlen = *bytes_read, tmplen;
            while(!select){
                  for(int i = 0; i <= optlen; ++i){
                        /* we treat i == optlen as input string
                         */
                        tmp_ch = (i != optlen) ? ((char**)(data_douplep+data_offset))[i] : ret;
                        if(strstr(tmp_ch, ret)){
                              found_m = 1;

                              /* printing match to screen and removing chars from
                               * old string
                               */
                              tmplen = strlen(tmp_ch);
                              putchar('\r');
                              printf("%s", tmp_ch);
                              if(tmplen > maxlen)maxlen = tmplen;
                              for(int i = 0; i < maxlen-tmplen; ++i)putchar(' ');
                              putchar('\r');

                              raw_mode();

                              char ch;
                              while(((ch = getc(stdin)))){
                                    if(ch == '\r'){
                                          *bytes_read = tmplen;
                                          free(ret);
                                          ret = tmp_ch;
                                          select = 1;
                                          break;
                                    }
                                    if(ch == iter_opts)break;
                              }

                              reset_term();

                              if(select)break;
                              continue;
                        }
                        else if(i == optlen-1 && !found_m){
                              select = 1;
                              break;
                        }
                  }
            }
      }
      return ret;
}
