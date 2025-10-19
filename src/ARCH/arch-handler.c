#ifdef __linux__
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "arch-handler.h"

void *fill_info(void *pkg_list);

/**
 * Debug test for memory leaks.
 */
int mem_test(void) {
  package_list *test = arch_pkgs();

  free_pkg_list(test);

  return EXIT_SUCCESS;
}

/**
 * First synchronously collects a list of all Pacman packages installed on the system.
 * Then asynchronously grabs the package information.
 */
package_list *arch_pkgs(void) {
  package_list *pkgs = malloc(sizeof(package_list));

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return NULL;
  }

  int pid = fork();

  if (pid == -1) {
    perror("fork");
    return NULL;
  }

  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);

    execvp("pacman", (char *[]){"pacman", "-Qq", NULL});
    return NULL;
  }

  waitpid(pid, NULL, 0);

  close(pipefd[1]);

  int capacity = 16;
  char **package_names = malloc(sizeof(char *) * capacity);
  if (package_names == NULL) {
    perror("malloc");
    return NULL;
  }

  char buffer[BUFSIZ];
  int package_no = 0;
  FILE *r = fdopen(pipefd[0], "r");
  while (fgets(buffer, BUFSIZ, r) != NULL) {
    package_names[package_no++] = strdup(buffer);
    if (package_no >= capacity) {
      if ((package_names = realloc(package_names, sizeof(char *) * (capacity *= 2))) == NULL) {
        perror("realloc");
        fclose(r);
        return NULL;
      }
    }
  }

  fclose(r);

  pkgs->pkg = malloc(sizeof(package) * package_no);
  if (pkgs->pkg == NULL) {
    perror("malloc");
    return NULL;
  }

  for (int i = 0; i < package_no; i++) {
    pkgs->pkg[i].name = strdup(package_names[i]);
    atomic_store(&pkgs->pkg[i].rdy, 0);
    free(package_names[i]);
  }

  free(package_names);
  
  pkgs->len = package_no;

  pthread_t asynch_fill_pkg_info;
  if (pthread_create(&asynch_fill_pkg_info, NULL, fill_info, pkgs) != 0) {
    perror("pthread");
    return NULL;
  }
  pthread_detach(asynch_fill_pkg_info);

  return pkgs;
}

void *fill_info(void *pkg_list) {
  package_list *package = pkg_list;

  for (int i = 0; i < package->len; i++) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
      perror("pipe");
      return NULL;
    }

    int pid = fork();

    if (pid == -1) {
      perror("fork");
      return NULL;
    }

    if (pid == 0) {
      close(pipefd[0]);
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);

      char *name = strdup(package->pkg[i].name);
      name[strlen(name)-1] = '\0'; // replace new line with '\0'
      execvp("pacman", (char *[]) {"pacman", "-Qi", name, NULL});
    }

    waitpid(pid, NULL, 0);

    close(pipefd[1]);

    FILE *package_info = fdopen(pipefd[0], "rb");

    char buffer[BUFSIZ];
    while (fgets(buffer, sizeof(buffer), package_info)) {
      // An info line either starts with a space or a character
      // If it starts with a character, it's a new entry in the info
      // If it's a space, it's a continuation from the previous line
      if (!isspace(*buffer)) {
        // Get the key (first word)
        // We can ignore any second word for now 
        char *ptr = buffer;
        while (!isspace(*ptr)) ptr++;
        
        int len = ptr - buffer;
        char *key = malloc( len + 1);
        strncpy(key, buffer, len);
        key[len] = '\0';

        // Now we go to the value by going to the colon
        char *colon = strchr(ptr, ':');
        if (!colon) continue;
        ptr = colon + 2;  // skip ": "

        char *value = malloc(sizeof(buffer) - (ptr - buffer));
        strcpy(value, ptr);  // assumes ptr is within buffer
        value[strlen(value)-1] = '\0';

        if (strcmp(key, "Description") == 0) {
          package->pkg[i].desc = value;
        }

        free(key);
      }
    }

    fclose(package_info);

    atomic_store(&package->pkg[i].rdy, 1);
  }

  return NULL;
}
#endif
