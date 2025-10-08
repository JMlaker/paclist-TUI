#ifndef PACKAGE_H
#define PACKAGE_H

typedef struct package {
  char *name;
  char *full_name;
  char *desc;
  char *version;
  char *homepage;
  int dep_len;
  char **deps;
  char is_dep;
} package;

typedef struct package_list {
  package *pkg;
  int len;
} package_list;

void free_pkg_list(package_list *list);

#endif
