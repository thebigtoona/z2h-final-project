#ifndef FILE_H
#define FILE_H

// returns the file descriptor of the new db file created or -1 (STATUS_ERROR) if 
// the open function encounters an error or the file exists already.
int create_db_file(char *filename);

// opens an existing db file. if the file does not exist or open returns an error 
// the function will return -1 (STATUS_ERROR)
int open_db_file(char *filename);

#endif
