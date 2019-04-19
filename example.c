#if 0
compilation:
gcc example.c raw.c -o ex
#endif

#include <stdio.h>
#include <stdlib.h>

#include "raw.h"

int main(){
      int sz;
      _Bool tab = 0;
      char* ln;
      while((ln = getline_raw(&sz, &tab))){
            printf("\n%i %s\n", sz, ln);
            free(ln);
      }
      return 0;
}
