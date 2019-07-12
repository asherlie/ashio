void raw_mode();
void reset_term();
char* getline_raw(int* bytes_read, _Bool* tab, int* ignore);
char* tab_complete(void* data_douplep, int data_blk_sz, int data_offset, int optlen,
                   char iter_opts, int* bytes_read, _Bool* free_s);


/* experimental/unimplemented struct tabcom code below */

struct tabcom* init_tabcom(struct tabcom* tbc);
void free_tabcom(struct tabcom* tbc);
void insert_tabcom(struct tabcom* tbc, void* data_douplep, int data_blk_sz, int data_offset, int optlen);

char* tab_complete_tbc(struct tabcom tbc, char iter_opts, int* bytes_read, _Bool* free_s);

struct tabcom_entry{
      void* data_douplep;
      int data_blk_sz, data_offset, optlen;
};

struct tabcom{
      struct tabcom_entry* tbce;
      int n, cap;
};
