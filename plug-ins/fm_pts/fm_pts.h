#ifndef FM_PTS_H
#define FM_PTS_H
int file_load (const char *filename);
int bfm_set_dir_src (const char *filename);
int bfm_set_dir_dest (const char *filename);
int file_save (const char *filename, int checkOverwrite);
#endif
