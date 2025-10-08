#include <stdlib.h>

#include "package.h"

void free_pkg_list(package_list *list) {
  for (int i = 0; i < list->len; i++) {
    if (list->pkg[i].name) { free(list->pkg[i].name); list->pkg[i].name = NULL; }
    if (list->pkg[i].full_name) { free(list->pkg[i].full_name); list->pkg[i].full_name = NULL; }
    if (list->pkg[i].desc) { free(list->pkg[i].desc); list->pkg[i].desc = NULL; }
    if (list->pkg[i].version) { free(list->pkg[i].version); list->pkg[i].version = NULL; }
    if (list->pkg[i].homepage) { free(list->pkg[i].homepage); list->pkg[i].homepage = NULL; }
    if (list->pkg[i].deps) { 
      for (int j = 0; j < list->pkg[i].dep_len; j++) {
        if (list->pkg[i].deps[i]) { free(list->pkg[i].deps[i]); list->pkg[i].deps[i] = NULL; }
      }
      free(list->pkg[i].deps);
      list->pkg[i].deps = NULL;
    }
  }
  free(list->pkg);
  free(list);
}
