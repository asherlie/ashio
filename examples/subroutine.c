#if 0
compilation:
gcc subroutine.c ashio.c -o sub 
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ashio.h"

void* routine(void* arg){
      struct gr_subroutine_arg* gsa = (struct gr_subroutine_arg*)arg;
      (void)gsa;
      /*puts("CLLED");*/
      printf("read a char: %c\n", *gsa->char_recvd);
      return NULL;
}

int main(int a, char** b){
      int sz;
      char* ln;
      if(a == 1){
            
            _Bool tab = 0;

            struct gr_subroutine_arg gsa;
            init_gsa(&gsa);
            gsa.pthread_arg = &gsa;

            while((ln = getline_raw_sub(&sz, &tab, NULL, routine, &gsa))){
                  printf("\n%i %s\n", sz, ln);
                  free(ln);
            }

            free_gsa(&gsa);

            return 0;
      }

      struct tabcom tbc;
      init_tabcom(&tbc);
      insert_tabcom(&tbc, b, sizeof(char*), 0, a);

      puts("enter tab to trigger auto completion and 'n' to iterate through options");

      _Bool free_s;
      char iter[2] = {'n', 'p'};
      while((ln = tab_complete(&tbc, iter, &sz, &free_s))){
            printf("\n%i %s\n", sz, ln);
            if(free_s)free(ln);
      }

      free_tabcom(&tbc);

      return 0;
}
