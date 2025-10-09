#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "osx-handle.h"
#include "cJSON.h"

#define CHUNK_SIZE 4096

void *fill_info(void *pkg_list);

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

      if (strcmp(package->pkg[i].name, "tailscale") == 0)
        execvp("brew", (char *[]){"brew", "info", "--json=v2", "--formula", package->pkg[i].name, NULL});
      else
        execvp("brew", (char *[]){"brew", "info", "--json=v2", package->pkg[i].name, NULL});
      return NULL;
    }

    waitpid(pid, NULL, 0);

    close(pipefd[1]);

    FILE *buf = fdopen(pipefd[0], "rb");

    char *json = NULL;
    size_t total_read = 0;
    char buffer[CHUNK_SIZE];

    while (!feof(buf)) {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, buf);
        if (bytes_read > 0) {
            char *new_json = realloc(json, total_read + bytes_read + 1);
            if (!new_json) {
                free(json);
                fclose(buf);
                return NULL;
            }
            json = new_json;
            memcpy(json + total_read, buffer, bytes_read);
            total_read += bytes_read;
        }
    }
    json[total_read] = '\0';

    cJSON *pkg = cJSON_Parse(json);

    const cJSON *formula = cJSON_GetObjectItemCaseSensitive(pkg, "formulae");
    const cJSON *cask = cJSON_GetObjectItemCaseSensitive(pkg, "casks");
    
    if (!cJSON_GetArraySize(cask)) { cask = NULL; }
    if (!cJSON_GetArraySize(formula)) { formula = NULL; }

    const cJSON *pkg_info = NULL;

    if ((pkg_info = (cask) ? cask : NULL)) {
      pkg_info = pkg_info->child;

      package->pkg[i].full_name = cJSON_GetObjectItemCaseSensitive(pkg_info, "full_token")->valuestring;
      package->pkg[i].desc = cJSON_GetObjectItemCaseSensitive(pkg_info, "desc")->valuestring;
      package->pkg[i].version = cJSON_GetObjectItemCaseSensitive(pkg_info, "version")->valuestring;
      package->pkg[i].homepage = cJSON_GetObjectItemCaseSensitive(pkg_info, "homepage")->valuestring;
      // We ignore dependencies rn because casks are NOT as easy as formulae
      // package->pkg[0].dep_len = cJSON_GetObjectItemCaseSensitive(pkg_info, "homepage")->valuestring;
      // package->pkg[0].deps = cJSON_GetObjectItemCaseSensitive(pkg_info, "homepage")->valuestring;
      // package->pkg[0].is_dep = cJSON_GetObjectItemCaseSensitive(pkg_info, "homepage")->valuestring;
    } else if ((pkg_info = (formula) ? formula : NULL)) {
      pkg_info = pkg_info->child;

      package->pkg[i].full_name = cJSON_GetObjectItemCaseSensitive(pkg_info, "full_name")->valuestring;
      package->pkg[i].desc = cJSON_GetObjectItemCaseSensitive(pkg_info, "desc")->valuestring;
      // package->pkg[0].version = cJSON_GetObjectItemCaseSensitive(pkg_info, "version")->valuestring;
      package->pkg[i].homepage = cJSON_GetObjectItemCaseSensitive(pkg_info, "homepage")->valuestring;

      // "dependencies" is a list
      cJSON *pkg_deps = cJSON_GetObjectItemCaseSensitive(pkg_info, "dependencies");
      package->pkg[i].dep_len = cJSON_GetArraySize(pkg_deps);
      if (package->pkg[i].dep_len > 0) {
        package->pkg[i].deps = malloc(sizeof(char*) * package->pkg[i].dep_len);
        const cJSON *dep;
        int j = 0; cJSON_ArrayForEach(dep, pkg_deps) package->pkg[i].deps[j++] = dep->valuestring;
      } else {
        package->pkg[i].deps = NULL;
      }

      cJSON *install_info = cJSON_GetObjectItemCaseSensitive(pkg_info, "installed")->child;
      package->pkg[i].is_dep = cJSON_GetObjectItemCaseSensitive(install_info, "installed_as_dependency")->type == cJSON_True;
    }

    cJSON_Delete(pkg); pkg = NULL;
    fclose(buf); buf = NULL;
    free(json); json = NULL;
  }
  
  return NULL;
}
