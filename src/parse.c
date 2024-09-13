#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  for (int i; i < dbhdr->count; i++) {
    struct employee_t emp = employees[i];
    printf("%d: %s,%s,%d\n", i+1, emp.name, emp.address, emp.hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
  char* name = strtok(addstring, ",");
  char* addr = strtok(NULL, ",");
  char* hours = strtok(NULL, ",");

  printf("add_employee: name=%s;addr=%s;hours=%s;\n", name, addr, hours);

  // copy the employee into the last entry in employees which is assumed to be big enough at 
  // this point.
  strncpy(employees[dbhdr->count].name, name, sizeof(employees[dbhdr->count].name));
  strncpy(employees[dbhdr->count].address, addr, sizeof(employees[dbhdr->count].address));
  
  // atoi returns the converted value or 0 on error;
  employees[dbhdr->count].hours = atoi(hours);

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
  // validate the fd is good
  if (fd < 0) {
    printf("read_employees: received bad file descriptor. cannot read header from file. exiting..\n");
    return STATUS_ERROR;
  }

  // at this point validate_db_header has already unpacked the header to host endian
  int count = dbhdr->count;

  // create space on the heap to read out the employees from the database. calloc returns a 
  // freeable ptr if there are 0 so no worries there.
  struct employee_t* employees = calloc(count, sizeof(struct employee_t));
  if (employees == NULL) {
    perror("calloc");
    printf("read_employees: allocation failure at calloc. exiting.. \n");
    return STATUS_ERROR;
  }
  printf("emp size %ld\n", sizeof(struct employee_t));
  printf("emp size at calloc: %ld\n", sizeof(*employees));

  // because we've already read the header at this point. the cursor is in the correct 
  // spot to start reading employees. 
  if (read(fd, employees, (count*sizeof(struct employee_t))) < 0) {
    perror("read");
    free(employees);
    return STATUS_ERROR;
  }
  //
  // for (int i; i < count; i++) {
  //   printf("\nread_employees: %s, %s, %d\n", employees[i].name, employees[i].address, employees[i].hours);
  // }

  // NUMERIC information on the employee_t needs to be unpacked at this step from network 
  // endian to host endian.
  for (int i = 0; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  // pass the employees ptr to the caller in main 
  *employeesOut = employees;
  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
  // validate the fd is good
  if (fd < 0) {
    printf("output_file: received bad file descriptor. cannot read header from file. exiting..\n");
    return STATUS_ERROR;
  } 

  // save a copy of this count because we need it later and we're packing it for network endian
  unsigned int realcount = dbhdr->count;
  // printf("output_file: realcount: %d\n", realcount);

  // convert the header to network endian for writing out to file 
  dbhdr->version = htons(dbhdr->version);
  dbhdr->count = htons(dbhdr->count);
  dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
  dbhdr->magic = htonl(dbhdr->magic);

  // printf("output_file: filesize = %ld\n", (sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount)));
  
  // set the offset to 0 on the file
  if (lseek(fd, 0, SEEK_SET) < 0) {
    perror("lseek");
    return STATUS_ERROR;
  }

  // write the header
  if (write(fd, dbhdr, sizeof(struct dbheader_t)) < 0) {
    perror("write");
    return STATUS_ERROR;
  }


  // iterate through the employees and write the employees data 
  for (int i; i < realcount; i++) {
    printf("output_file: employees[i] = { name = %s; address = %s; hours = %d }\n", employees[i].name, employees[i].address, employees[i].hours);
    
    // do conversion for numerical values to network endian 
    employees[i].hours = htonl(employees[i].hours);

    // write to the file
    if (write(fd, &employees[i], sizeof(struct employee_t)) < 0) {
      perror("write");
      return STATUS_ERROR;
    }
  }
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  // validate the fd is good
  if (fd < 0) {
    printf("validate_db_header: received bad file descriptor. cannot read header from file. exiting..\n");
    return STATUS_ERROR;
  }

  // attempt the allocation for header 
	struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    perror("calloc");
    printf("validate_db_header: failure at malloc. calloc failed to create `struct dbheader_t header`\n");
    return STATUS_ERROR;
  }

  // attempt to read the header from the file
  if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  // set the endianess of the header values to the host machine for reading. (stored in network endian on file)
  // this is important when working with binary on network or multiple targets
  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);

  // validate the header here
  if (header->magic != HEADER_MAGIC) {
    printf("validate_db_header: header magic value mismatch");
    free(header);
    return STATUS_ERROR;
  }
  
  if (header->version != 1) {
    printf("validate_db_header: header version does not match. expected 1, got %d", header->version);
    free(header);
    return STATUS_ERROR;
  }
  
  struct stat db_stat = {0};
  if (fstat(fd, &db_stat) < 0) {
    perror("fstat");
    free(header);
    return STATUS_ERROR;
  }  
  
  if (header->filesize != db_stat.st_size) {
    printf("validate_db_header: db filesize data does not match. expected %ld, got %d", db_stat.st_size, header->filesize);
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;
  
  return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
  // attempt the allocation for header 
	struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    perror("calloc");
    printf("create_db_header: failure at malloc. calloc failed to create `struct dbheader_t header`\n");
    return STATUS_ERROR;
  }

  // initialize the header values and set the headerOut ptr to the header ptr given by malloc.
  header->version = 0x1;
  header->magic = HEADER_MAGIC;
  header->count = 0;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_SUCCESS;
}


