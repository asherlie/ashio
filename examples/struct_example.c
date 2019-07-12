#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ashio.h"

struct x{
      int a, b, c;
      double d;
      char some_string[10];
      float some_other_entry;
};

int main(){
      struct x* x_arr = calloc(5, sizeof(struct x));

      strncpy(x_arr[0].some_string, "zero", 4+1);
      strncpy(x_arr[1].some_string, "one", 3+1);
      strncpy(x_arr[2].some_string, "two", 3+1);
      strncpy(x_arr[3].some_string, "three", 5+1);
      strncpy(x_arr[4].some_string, "four", 4+1);

      int length;
      _Bool free_s;

      struct tabcom tbc;
      init_tabcom(&tbc);
      insert_tabcom(&tbc,
                  /* data pointer */
                  x_arr,
                  /* size of each block of data */
                  sizeof(struct x),
                  /* offset into each entry to char* */
                  (char*)x_arr[0].some_string-(char*)&x_arr[0],
                  /* n options */
                  5);

      char* str = tab_complete(&tbc, /* iterate through options with 'n' */ 'n', &length, &free_s);

      printf("\n%i %s\n", length, str);
      if(free_s)free(str);
      free(x_arr);
}
