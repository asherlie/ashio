#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <pthread.h>

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
      tbc->n_flattened = tbc->n = 0;
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
      tbc->n_flattened += optlen;
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
      tbc->n_flattened -= tbc->tbce[tbc->n-1].optlen;
      return tbc->tbce[--tbc->n];
}

/* returns a NULL terminated list of strings of size *n_matches */
char** find_matches(struct tabcom* tbc, char* needle, int* n_matches){
      /* TODO: dynamically resize */
      char** ret = malloc(sizeof(char*)*(tbc->n_flattened+2)), * tmp_ch;

      int sz = 0;
      for(int i = 0; i < tbc->n; ++i){
            for(int j = 0; j < tbc->tbce[i].optlen; ++j){
                  void* inter = ((char*)tbc->tbce[i].data_douplep+(j*tbc->tbce[i].data_blk_sz)+tbc->tbce[i].data_offset);

                  /* can't exactly remember this logic -- kinda hard to reason about */
                  if(tbc->tbce[i].data_blk_sz == sizeof(char*))tmp_ch = *((char**)inter);
                  else tmp_ch = (char*)inter;

                  if(strstr(tmp_ch, needle))ret[sz++] = tmp_ch;
            }
      }
      ret[sz++] = needle;
      ret[sz] = 0;
      *n_matches = sz;
      return ret;
}

/* used for calling find_matches from a new thread */
struct find_matches_arg{
      struct tabcom* tbc;
      char* needle;
      char*** ret;
};

void* find_matches_pth(void* fma_v){
      struct find_matches_arg* fma = (struct find_matches_arg*)fma_v;
      int n_matches;
      *fma->ret = find_matches(fma->tbc, fma->needle, &n_matches);
      /* casting n_matches to uintptr_t to avoid an int to pointer cast warning */
      return (void*)((uintptr_t)n_matches);
}

int narrow_matches(char** cpp, char* needle){
      int n_removed = 0;
      for(char** i = cpp; *i; ++i){
            if(!strstr(*i, needle)){
                  ++n_removed;
                  for(char** j = i; *j; ++j){
                        /* this implicitly deals with moving over the NULL */
                        *j = j[1];
                  }
                  --i;
            }
      }
      return n_removed;
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
                              #if 0 
                              should we first iterate through all strings and find all matches?
                              this would make it very easy to iterate both backwards and forward
                              the only thing is that it would take a lot of time to compute this initially
                              
                              we could do this original finding of matches in a separate thread!!
                              this would also help to make this program more modular

                              in this case if the user starts iterating while the structure is being built its nbd
                              #endif

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

/* x is base_str, y is string checking against it */
_Bool n_char_equiv(char* x, char* y, int n){
      for(int i = 0; i < n; ++i)
            if(!x[i] || !y[i] || x[i] != y[i])return 0;
      return 1;
}

void clear_line(int len, char* str){
      printf("\r%s", str);
      for(int j = 0; j < len; ++j)putchar(' ');
      putchar('\r');
}

struct shared_d{
      _Bool thread_spawned;
      /* this field is used only if !thread_spawned */
      int n_matches;
};

pthread_t fmp;

char* tab_complete_internal_extra_mem_low_computation(struct tabcom* tbc, struct shared_d* shared, char* base_str, int bs_len, char*** base_match, char iter_opts[2], int* bytes_read, _Bool* free_s){
      /* TODO:
       * all narrowing and recreating/rescanning/initial scanning should be done in different threads
       *
       * i actually think this is important
       * each time a character is entered we create an initial scan
       * or maybe when strings with >= 2 chars are entered
       *
       * i can even create a system where each time a new char is appended we can add a new char** of
       * adjustments to the base char** 
       * each time a char is deleted from current stream we pop off the relevant char**
       * until the base char** which was created from find_matches() has been removed
       */
      _Bool tab;
      char* ret = getline_raw_internal(base_str, bs_len, bytes_read, &tab, NULL), ** tmp_str, ** match = NULL;
      /* ret is only null if ctrl-c */
      *free_s = ret;

      if(tab && tbc){

            int n_matches;

            {
            _Bool new_search = 1;
            if(base_match){
                  /* if base_match was computed in a separate thread we'll have to join it */
                  if(shared->thread_spawned){
                        pthread_join(fmp, (void*)&n_matches);
                        shared->thread_spawned = 0;
                  }
                  /* if we don't need to join thread, n_matches is stored in shared */
                  else n_matches = shared->n_matches;

                  /* n_char_equiv is essentially ensuring that no chars have been removed in getline_raw()
                   * this is the only circumstance that base_match is usable
                   */
                  if(base_str && bs_len && n_char_equiv(base_str, ret, bs_len)){
                        match = *base_match;
                        new_search = 0;
                        /* last index of match must be overwritten to be user input */
                        match[n_matches-1] = ret;
                        n_matches -= narrow_matches(match, ret);
                  }
            }
            if(new_search){
                  if(base_match)free(*base_match);
                  match = find_matches(tbc, ret, &n_matches);
            }
            }

            tmp_str = match;

            int tmplen, maxlen = 0;
            char ch = 0;
            raw_mode();

            while(1){

                  if(!*tmp_str){
                        if(tmp_str != match)tmp_str = match;
                        /* there are no matches 
                         * this should never happen
                         * TODO: handle this
                         */
                        else {}
                  }

                  tmplen = strlen(*tmp_str);
                  /* TODO: use prev_len not maxlen */
                  if(maxlen < tmplen)maxlen = tmplen;

                  clear_line(maxlen-tmplen, *tmp_str);

                  ch = getc(stdin);

                  /* selection */
                  if(ch == '\r'){
                        *bytes_read = tmplen;
                        if(ret != *tmp_str){
                              if(ret)free(ret);
                              *free_s = 0;
                              ret = *tmp_str;
                        }
                        break;
                  }

                  if(ch == *iter_opts){
                        ++tmp_str;
                        continue;
                  }
                  if(ch == iter_opts[1]){
                        if(tmp_str != match){
                              --tmp_str;
                              continue;
                        }
                        tmp_str = match+(n_matches-1);
                        continue;
                  }
                  /* ctrl-c */
                  if(ch == 3){
                        if(*free_s)free(ret);
                        ret = NULL;
                        break;
                  }

                  /* if we've gotten this far it's time to ~recurse~ */

                  /* deletion */
                  _Bool del = ch == 127 || ch == 8;

                  /* +1 for extra char, +1 for \0 */
                  char recurse_str[tmplen+1+((del) ? 0 : 1)];
                  memcpy(recurse_str, *tmp_str, tmplen);
      
                  /*
                   * we could keep an array of match char**s [tmplen, tmplen-1]
                   * each time a deletion is made we go back a step
                   */

                  if(del){
                        clear_line(tmplen, "");
                        recurse_str[--tmplen] = 0;
                        /* deletion makes match useless for recurse */
                        /* TODO: this might be bad if match == base_match */
                        free(match);

                        /* we're generating matches in a new thread before we make next
                         * recursive call
                         * it's safe to assume that matches will be found before geline_raw() returns
                         * in the next call to tab_complete_internal_extra_mem_low_computation()
                         * otherwise, we wait for the find match thread to join
                         */
                        struct find_matches_arg fma;
                        /* reason it's here needs to be upheld tho
                         * find_matches() sets the last index of its return
                         * to needle - is it ok for needle to be on the stack
                         */
                        fma.needle = recurse_str;
                        fma.tbc = tbc;
                        fma.ret = &match;
                        shared->thread_spawned = 1;
                        pthread_create(&fmp, NULL, &find_matches_pth, &fma);
                  }
                  else{
                        /*shared->thread_spawned = 0;*/
                        /*if(!end_ptr)end_ptr = tmp_str+(n_matches-1);*/
                        recurse_str[tmplen++] = ch;
                        recurse_str[tmplen] = 0;
                        /* adjusting the last index of match to user input */
                        /* TODO: is it a safe assumption that the last index of match
                         * will always be user input 
                         */
                        /* TODO: free this */
                        match[n_matches-1] = malloc(tmplen);
                        /**end_ptr = malloc(tmplen);*/
                        /*memcpy(*end_ptr, recurse_str, tmplen);*/
                        memcpy(match[n_matches-1], recurse_str, tmplen);

                        shared->n_matches = n_matches-narrow_matches(match, recurse_str);
                  }
                  if(*free_s)free(ret);

                  reset_term();
                  return tab_complete_internal_extra_mem_low_computation(tbc, shared, recurse_str, tmplen, &match, iter_opts, bytes_read, free_s);

            }

            reset_term();

            /*if(base_match && *end_ptr)free(*end_ptr);*/
            free(match);
      }
      return ret;
}

/* tab_complete behaves like getline(), but does not include \n char in returned string */
/* *free_s is set to 1 if returned buffer should be freed */
char* tab_complete(struct tabcom* tbc, char iter_opts[2], int* bytes_read, _Bool* free_s){
      #if LOW_MEM
      return tab_complete_internal(tbc, NULL, 0, *iter_opts, bytes_read, free_s);
      #else
      struct shared_d shared;
      shared.thread_spawned = 0;
      shared.n_matches = 0;
      char* ret = tab_complete_internal_extra_mem_low_computation(tbc, &shared, NULL, 0, NULL, iter_opts, bytes_read, free_s);
      return ret;
      #endif
}
