#if 0
compilation:
gcc example.c ashio.c -o ex
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ashio.h"

struct x{
      int a, b, c;
      double d;
      char some_string[10];
      float some_other_entry;
};

int main(int a, char** b){
      struct x* x_arr = calloc(6, sizeof(struct x));

      strncpy(x_arr[0].some_string, "zero", 4+1);
      strncpy(x_arr[1].some_string, "one", 3+1);
      strncpy(x_arr[2].some_string, "two", 3+1);
      strncpy(x_arr[3].some_string, "three", 5+1);
      strncpy(x_arr[4].some_string, "four", 4+1);

      int sz;
      char* ln;

      puts("enter tab to trigger auto completion and 'n' to iterate through options");
      _Bool free_s;

      struct tabcom tbc;
      init_tabcom(&tbc);
      insert_tabcom(&tbc, b, sizeof(char*), 0, a);
      insert_tabcom(&tbc, x_arr, sizeof(struct x), (char*)x_arr[0].some_string-(char*)&x_arr[0], 5);

      while((ln = tab_complete_tbc(&tbc, 'n', &sz, &free_s))){
            printf("\n%i %s\n", sz, ln);
            if(free_s)free(ln);
      }

      free_tabcom(&tbc);
      free(x_arr);

      return 0;
}
