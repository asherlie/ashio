#if 0
compilation:
gcc example.c ashio.c -o ex
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ashio.h"

int main(int a, char** b){
      int sz;
      char* ln;
      if(a == 1){
            _Bool tab = 0;
            while((ln = getline_raw(&sz, &tab, NULL))){
                  printf("\n%i %s\n", sz, ln);
                  free(ln);
            }
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
