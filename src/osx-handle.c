#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "osx-handle.h"


/**
 * Debug test for memory leaks.
 */
int mem_test(void) {
  package_list *test = osx_pkgs();

  free_pkg_list(test);

  return EXIT_SUCCESS;
}

package_list *osx_pkgs(void) {
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

    execvp("brew", (char *[]){"brew", "ls", "-1", NULL});
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
    free(package_names[i]);
  }

  free(package_names);
  
  pkgs->len = package_no;

  return pkgs;
}
