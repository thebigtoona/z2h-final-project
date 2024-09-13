#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database filepath>\n", argv[0]);
  printf("\t-n\t\t-\tcreate a new database file\n");
  printf("\t-f <filepath>\t-\t(required) a path to the new database file\n");
}

int main(int argc, char *argv[]) { 
  char* addstring = NULL;
  char* filepath = NULL;
  bool newfile = false;
  bool listemp = false;
	int c = -1; 
  int db_fd = -1;
  struct dbheader_t* db_header = NULL;
  struct employee_t* employees = NULL;
  
  // parse command line args here
  while ((c = getopt(argc, argv, "nf:a:l")) != -1) {
    switch(c) {
      case 'n':
        newfile = true;
        break;
      case 'f':
        filepath = optarg;
        break;
      case 'a':
        addstring = optarg;
        break;
      case 'l':
        listemp = true;
        break;
      case '?':
        printf("unknown option: %c\n", c);
        break;
      default:
        // we should never reach this case. getopt sets `c` to '?' when it has an error (ref: man 3 getopt)
        return -1;
    }
  }

  // check for filepath 
  if (filepath == NULL) { 
    printf("filepath (-f) is a required argument. see usage below\n\n"); 
    print_usage(argv);
  }

  // either create a new db or open the db depending on wether newfile flag was set 
  if (newfile) {
    db_fd = create_db_file(filepath);
    if (db_fd == STATUS_ERROR) {
      printf("unable to create db file. exiting..");
      return STATUS_ERROR;
    }
    
    if (create_db_header(db_fd, &db_header) == STATUS_ERROR) {
      printf("error creating the database header, exiting.. ");
      return STATUS_ERROR;
    }
  } else {
      db_fd = open_db_file(filepath);
      if (db_fd == STATUS_ERROR) {
        printf("unable to open db file. exiting..");
        return STATUS_ERROR;
      }

      if (validate_db_header(db_fd, &db_header) == STATUS_ERROR) {
        printf("error validating the database header, exiting.. ");
        return STATUS_ERROR;
      }
  }

  if (read_employees(db_fd, db_header, &employees) == STATUS_ERROR) {
    printf("error reading employees. exiting.. ");
    return STATUS_ERROR;
  }

  if (addstring != NULL) {
    // reallocate employees so that enough space is allocated for one more employee
    employees = realloc(employees, (db_header->count+1 * sizeof(struct employee_t)));
    if (employees == NULL) {
      perror("realloc");
      return STATUS_ERROR;
    }

    // now literally add the employee since there is a enough space
    if (add_employee(db_header, employees, addstring) == STATUS_ERROR) {
      printf("error adding employee to db. exiting...");
      return STATUS_ERROR;
    }
    
    // IMPORTANT! increment the counter for the db_header since the add is succesful.
    db_header->count += 1;
  }
  
  if (listemp) {
    list_employees(db_header, employees);
  }
  
  if (output_file(db_fd, db_header, employees) == STATUS_ERROR) {
    printf("error writing out file. exiting.. \n");
    return STATUS_ERROR;
  }
  
  return 0;
}
