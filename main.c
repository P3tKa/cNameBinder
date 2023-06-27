#include "xxHash/xxhash.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/tree.h>

/* Struct to represent a duplicate names */
struct DuplicateName {
  char *fullName;
  STAILQ_ENTRY(DuplicateName) entries;
};

/* Struct to represent a student node */
struct Node {
  char *fullName;
  char *firstName;
  XXH64_hash_t hash;
  RB_ENTRY(Node) entry;
  STAILQ_HEAD(DuplicatesList, DuplicateName) duplicatesHead;
};

/* Function to compare two student nodes */
int node_cmp(struct Node *a, struct Node *b) {
  if (a->hash < b->hash)
    return -1;
  else if (a->hash > b->hash)
    return 1;
  return 0;
}

RB_HEAD(Tree, Node) root = RB_INITIALIZER(&root);
RB_PROTOTYPE(Tree, Node, entry, node_cmp)
RB_GENERATE(Tree, Node, entry, node_cmp)

/* Function to parse the student file and build the student nodes */
void parseFile(char *fileName) {
  FILE *fp = fopen(fileName, "r");
  if (fp == NULL) {
    puts("Could not open file\n");
    exit(-1);
  }

  /* Check for file length */
  fseek(fp, 0, SEEK_END);
  int fileLength = ftell(fp);
  rewind(fp);

  if (fileLength == 0) {
    puts("File is empty!");
    exit(-1);
  }

  char *line = NULL;
  size_t buffer_size = 0;
  ssize_t line_length;

  while ((line_length = getline(&line, &buffer_size, fp)) != -1) {
    /* Find the position of the first space character */
    char *firstSpace = strchr(line, ' ');

    if (firstSpace == NULL) {
      puts("No first name detected in this line!");
      continue;
    };

    struct Node *student = malloc(sizeof(struct Node));

    /* Dynamically allocate memory for the full name */
    student->fullName = malloc((line_length + 1) * sizeof(char));
    if (student->fullName == NULL) {
      puts("Memory allocation failed.\n");
      exit(-1);
    }

    /* Copy the full name into the struct */
    strncpy(student->fullName, line, line_length + 1);
    student->fullName[line_length - 1] = '\0';

    /* Calculate length of the first name */
    size_t firstNameLength = firstSpace - line;
    /* Dynamically allocate memory for the first name */
    student->firstName = malloc((firstNameLength + 1) * sizeof(char));
    if (student->firstName == NULL) {
      puts("Memory allocation failed.\n");
      exit(-1);
    }
    /* Copy the first name into the struct */
    strncpy(student->firstName, line, firstNameLength + 1);
    student->firstName[firstNameLength] = '\0';

    /* Create hash from the first name */
    student->hash = XXH64(student->firstName, firstNameLength, 0);

    /* Check if a node with the same key exists */
    struct Node *existingNode = RB_FIND(Tree, &root, student);
    if (existingNode == NULL) {
      /* Initialize the duplicates list for this node */
      STAILQ_INIT(&student->duplicatesHead);
      RB_INSERT(Tree, &root, student);
      continue;
    }
    /* Add fullName to duplicate list */
    struct DuplicateName *duplicate = malloc(sizeof(struct DuplicateName));
    if (duplicate == NULL) {
      puts("Memory allocation failed.\n");
      exit(-1);
    }

    /* Dynamically allocate memory for the duplicate name */
    duplicate->fullName = malloc((line_length + 1) * sizeof(char));
    strncpy(duplicate->fullName, line, line_length + 1);
    duplicate->fullName[line_length - 1] = '\0';

    STAILQ_INSERT_TAIL(&existingNode->duplicatesHead, duplicate, entries);

    /* Remove memory from duplicate node structure*/
    free(student->fullName);
    free(student->firstName);
    free(student);
  }

  fclose(fp);
  free(line);
}

/* Function to print the student nodes and their duplicates */
void printStudents() {
  struct Node *studentNode;
  int count = 0;

  RB_FOREACH(studentNode, Tree, &root) {
    printf("\n|-_-_-_ Unique student: %d _-_-_-|\n", count);
    printf("Full name: %s, First name: %s, hash: %llu\n", studentNode->fullName,
           studentNode->firstName, studentNode->hash);

    /* Print duplicates if any */
    struct DuplicateName *duplicate;
    STAILQ_FOREACH(duplicate, &studentNode->duplicatesHead, entries) {
      printf("\t\t\t^--> First name duplicate: %s\n", duplicate->fullName);
    }

    count++;
  }
}

/* Function to free the allocated memory */
void freeStudents() {
  struct Node *var, *next;
  for (var = RB_MIN(Tree, &root); var != NULL; var = next) {
    next = RB_NEXT(Tree, &root, var);
    RB_REMOVE(Tree, &root, var);
    free(var->fullName);
    free(var->firstName);

    // Free duplicates list
    struct DuplicateName *duplicate, *nextDuplicate;
    STAILQ_FOREACH_SAFE(duplicate, &var->duplicatesHead, entries,
                        nextDuplicate) {
      STAILQ_REMOVE(&var->duplicatesHead, duplicate, DuplicateName, entries);
      free(duplicate->fullName);
      free(duplicate);
    }

    free(var);
  }
}

int main(int argc, char *argv[]) {
  /* No command-line arguments provided */
  if (argc < 2) {
    char fileName[256];
    printf("Please enter the name of the text file: ");
    scanf("%255s", fileName);
    parseFile(fileName);
  } else {
    parseFile(argv[1]);
  }
  printStudents();
  freeStudents();
  return 0;
}