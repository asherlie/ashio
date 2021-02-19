#include "ashio.h"
#include <stdlib.h>
#include <stdio.h>

/* TODO: this example seg faults when tab is pressed twice
 * fix this
 */
int main(){
      char** cpp = malloc(sizeof(char*)*1e6);
      for(int i = 0; i < 1e6; ++i){
            cpp[i] = calloc(sizeof(char)*8, 1);
            sprintf(cpp[i], "%i", i);
      }

      struct tabcom tbc;
      init_tabcom(&tbc);
      insert_tabcom(&tbc, cpp, sizeof(char*), 0, 1e6);

      int sz;
      _Bool fr;
      char* ret;
      while((ret = tab_complete(&tbc, "np", &sz, &fr)))printf("%i: \"%s\"\n", sz, ret);

      free_tabcom(&tbc);
      for(int i = 0; i < 1e6; ++i)free(cpp[i]);
      free(cpp);
}
