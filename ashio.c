#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#include "ashio.h"

struct termios def, raw;

void raw_mode(){
      tcsetattr(0, TCSANOW, &raw);
}

/* reset_term can only be called after getline_raw has been */
void reset_term(){
      tcsetattr(0, TCSANOW, &def);
}

/*
 * TODO: possibly implement python style tab completion
 * where they show up at the same time
 * i can check for screen width
 */

/* reads a line from stdin until an \n is found or a tab is found 
 * returns NULL on ctrl-c
 */
/* TODO: 
 * getline_raw() should assume that the terminal is in raw mode
 * it is too slow to put term in raw mode and back with each read
 * from stdin
 * especially when being used with tab_complete(), which requires
 * the terminal to be in raw mode as well
 */

char* getline_raw_internal(char* base, int baselen, int* bytes_read, _Bool* tab, int* ignore){
      tcgetattr(0, &raw);
      tcgetattr(0, &def);
      cfmakeraw(&raw);
      raw_mode();
      char c;

      int buf_sz = 2+baselen;
      char* ret = calloc(buf_sz, 1);

      *tab = 0;
      *bytes_read = (baselen && base) ? baselen : 0;

      /*
       * since in raw mode, we can prepend our base str
       * what if the base str has been deleted --
       * we need to add it to string in progress too
       */
      for(int i = 0; i < baselen; ++i){
            ret[i] = base[i];
            putc(base[i], stdout);
      }

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
            /* deletion */
            if(c == 8 || c == 127){
                  if(*bytes_read == 0)continue;
                  ret[--(*bytes_read)] = 0;
                  printf("\r%s%c\r%s", ret, ' ', ret);
                  continue;
            }
            /* buf_sz-1 to leave room for \0 */
            if(*bytes_read == buf_sz-1){
                  buf_sz *= 2;
                  char* tmp_s = calloc(buf_sz, 1);
                  memcpy(tmp_s, ret, *bytes_read);
                  free(ret);
                  ret = tmp_s;
            }
            ret[(*bytes_read)++] = c;
            ret[*bytes_read] = 0;
            putchar(c);
      }
      /* before exiting, we restore term to its
       * default settings
       */
      reset_term();

      return ret;
}

char* getline_raw(int* bytes_read, _Bool* tab, int* ignore){
      return getline_raw_internal(NULL, 0, bytes_read, tab, ignore);
}

/* tabcom operations */

struct tabcom* init_tabcom(struct tabcom* tbc){
      if(!tbc)tbc = malloc(sizeof(struct tabcom));
      tbc->n = 0;
      tbc->cap = 5;
      tbc->tbce = malloc(sizeof(struct tabcom_entry)*tbc->cap);
      return tbc;
}

void free_tabcom(struct tabcom* tbc){
      free(tbc->tbce);
}

/* data_offset is offset into data where char* can be found
 * this is set to 0 if data_douplep is a char*
 */
int insert_tabcom(struct tabcom* tbc, void* data_douplep, int data_blk_sz, int data_offset, int optlen){
      int ret = 1;
      if(tbc->n == tbc->cap){
            ++ret;
            tbc->cap *= 2;
            struct tabcom_entry* tmp_tbce = malloc(sizeof(struct tabcom_entry)*tbc->cap);
            memcpy(tmp_tbce, tbc->tbce, sizeof(struct tabcom_entry)*tbc->n);
            free(tbc->tbce);
            tbc->tbce = tmp_tbce;
      }

      tbc->tbce[tbc->n].data_douplep = data_douplep;
      tbc->tbce[tbc->n].data_blk_sz = data_blk_sz;
      tbc->tbce[tbc->n].data_offset = data_offset;
      tbc->tbce[tbc->n++].optlen = optlen;
      return ret;
}

struct tabcom_entry pop_tabcom(struct tabcom* tbc){
      return tbc->tbce[--tbc->n];
}

char* tab_complete_internal(struct tabcom* tbc, char* base_str, int bs_len, char iter_opts, int* bytes_read, _Bool* free_s){
      _Bool tab;
      char* ret = getline_raw_internal(base_str, bs_len, bytes_read, &tab, NULL), * tmp_ch = NULL;
      *free_s = 1;
      if(tab && tbc){
            _Bool select = 0;
            int maxlen = *bytes_read, tmplen;
            while(!select){
                  for(int tbc_i = 0; tbc_i < tbc->n; ++tbc_i){
                        for(int i = 0; i <= tbc->tbce[tbc_i].optlen; ++i){
                              /* TODO: improve readability */
                              /* select being set to 1 here indicates that we've received a ctrl-c */
                              if(select)break;
                              /* we treat i == optlen of the last index of tbc as the input string */
                              if(i == tbc->tbce[tbc_i].optlen){
                                    /* setting tmp_ch to NULL to indicate that we should skip
                                     * this index if the condition below is not met
                                     */
                                    tmp_ch = NULL;
                                    if(tbc_i == tbc->n-1)tmp_ch = ret;
                              }
                              else{
                                    void* inter = ((char*)tbc->tbce[tbc_i].data_douplep+(i*tbc->tbce[tbc_i].data_blk_sz)+tbc->tbce[tbc_i].data_offset);

                                    /* can't exactly remember this logic -- kinda hard to reason about */
                                    if(tbc->tbce[tbc_i].data_blk_sz == sizeof(char*))tmp_ch = *((char**)inter);
                                    else tmp_ch = (char*)inter;
                                    /* printf("[%i][%i]: (%s, %s)\n", tbc_i, i, ret, tmp_ch); */
                              }
                              if(tmp_ch && strstr(tmp_ch, ret)){
                                    /* printing match to screen and removing chars from * old string */
                                    tmplen = (tmp_ch == ret) ? *bytes_read : (int)strlen(tmp_ch);
                                    putchar('\r');
                                    printf("%s", tmp_ch);
                                    if(tmplen > maxlen)maxlen = tmplen;
                                    for(int j = 0; j < maxlen-tmplen; ++j)putchar(' ');
                                    putchar('\r');

                                    /* should we ever exit raw mode during this process? */
                                    raw_mode();

                                    char ch;
                                    while(((ch = getc(stdin)))){
                                          if(ch == 3){
                                                if(*free_s)free(ret);
                                                ret = NULL;
                                                select = 1;
                                                break;
                                          }
                                          if(ch == '\r'){
                                                *bytes_read = tmplen;
                                                if(ret != tmp_ch){
                                                      free(ret);
                                                      /* if we're returning a string that wasn't allocated
                                                       * by us, the user doesn't need to free it
                                                       */
                                                      *free_s = 0;
                                                      ret = tmp_ch;
                                                }
                                                select = 1;
                                                break;
                                          }
                                          if(ch == iter_opts)break;

                                          reset_term();

                                          /* we need to pass along the choice that we're currently on
                                           * before we can recurse, though, we need to append ch to the string
                                           */

                                          /* TODO: handle ch as delete or backspace key */
                                          /* in this case, we'd need to shorten base_str_recurse
                                           */
                                          char base_str_recurse[tmplen+1];
                                          memcpy(base_str_recurse, tmp_ch, tmplen);
                                          base_str_recurse[tmplen] = ch;
                                          if(*free_s)free(ret);

                                          /* this is a pretty nice solution :) */

                                          return tab_complete_internal(tbc, base_str_recurse, tmplen+1, iter_opts, bytes_read, free_s);
                                    }

                                    reset_term();

                                    /* TODO: improve readability */
                                    if(select)break;
                                    continue;
                              }
                              /* TODO: improve readability */
                              /* TODO: is this necessary? */
                              if(select)break;
                        }
                  }
            }
      }
      return ret;
}

/* tab_complete behaves like getline(), but does not include \n char in returned string */
/* *free_s is set to 1 if returned buffer should be freed */
char* tab_complete(struct tabcom* tbc, char iter_opts, int* bytes_read, _Bool* free_s){
      return tab_complete_internal(tbc, NULL, 0, iter_opts, bytes_read, free_s);
}
