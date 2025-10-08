#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "paclist.h"

int main(void) {
  WINDOW *win = initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(win, TRUE);

  int x, y;
  getmaxyx(win, y, x);

	package_list *packages = osx_pkgs();

  attron(A_REVERSE);
  printw("%s", packages->pkg[0].name);
  attroff(A_REVERSE);
  for (int i = 1; i < packages->len && i < y; i++) {
    printw("%s", packages->pkg[i].name);
  }

  refresh();

  int str_index = 0;
  int str_select = 0;
  int a = 0;
  while ((a = getch()) != 'q') {
    if (a == KEY_DOWN) {
      str_select = (str_select >= packages->len - 1) ? str_select : str_select + 1;
      str_index = (str_index >= packages->len - y || str_select < y) ? str_index : str_index + 1;
      for (int i = 0; i < packages->len && i < y; i++) {
        if (i == str_select - str_index)
          attron(A_REVERSE);
        mvprintw(i, 0, "%s", packages->pkg[i + str_index].name);
        if (i == str_select - str_index)
          attroff(A_REVERSE);
      }
      refresh();
    }
    if (a == KEY_UP) {
      str_select = (str_select <= 0) ? str_select : str_select - 1;
      str_index = (str_index <= 0 || str_select >= str_index) ? str_index : str_index - 1;
      for (int i = 0; i < packages->len && i < y; i++) {
        if (i == str_select - str_index)
          attron(A_REVERSE);
        mvprintw(i, 0, "%s", packages->pkg[i + str_index].name);
        if (i == str_select - str_index)
          attroff(A_REVERSE);
      }
      refresh();
    }
  }

  endwin();

  return EXIT_SUCCESS;
}
