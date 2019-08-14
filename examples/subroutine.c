#if 0
compilation:
gcc subroutine.c ashio.c -o sub 
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ashio.h"

void* routine(void* arg){
      struct gr_subroutine_arg* gsa = (struct gr_subroutine_arg*)arg;
      printf("\r\nread a char: '%c', str is now: \"%s\"\n", *gsa->char_recvd, *gsa->str_recvd);
      return NULL;
}

int main(){
      int sz;
      char* ln;
            
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
