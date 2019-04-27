#if 0
compilation:
gcc example.c raw.c -o ex
#endif

#include <stdio.h>
#include <stdlib.h>

#include "raw.h"

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

      puts("enter tab to trigger auto completion and 'n' to iterate through options");
      _Bool free_s;
      while((ln = tab_complete(b, sizeof(char*), 0, a, 'n', &sz, &free_s))){
            printf("\n%i %s\n", sz, ln);
            if(free_s)free(ln);
      }
      return 0;
}
