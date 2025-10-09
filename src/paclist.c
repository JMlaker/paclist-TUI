#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "paclist.h"

int select_str(int sel, char **str, int up_down, int max_sel);

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
	move(0, 0);
  refresh();

  int str_select = 0;

	char **names = malloc(sizeof(char*) * packages->len);
	for (int i = 0; i < packages->len; i++) names[i] = packages->pkg[i].name; 

  int a = 0;
  while ((a = getch()) != 'q') {
    if (a == KEY_DOWN) {
      str_select = select_str(str_select, names, 1, packages->len);
      refresh();
    }
    if (a == KEY_UP) {
			str_select = select_str(str_select, names, 0, packages->len);
      refresh();
    }
  }

  endwin();

  return EXIT_SUCCESS;
}

int select_str(int sel, char **str, int up_down, int max_sel) {
	int y = getmaxy(stdscr);

	// If the selected is at the bottom, do nothing if moving down
	if (up_down && sel >= max_sel - 1) return max_sel - 1;

	// If the selected is at the top, do nothing if moving up
	if (!up_down && sel <= 0) return 0;

	// Else, move the selector
	// up_down = 0 ==> moving up
	// up_down = 1 ==> moving down
  sel = (up_down) ? sel + 1 : sel - 1;

	int cur_y = getcury(stdscr);

	int index = 0;
	if (up_down) {
		// If selecting downward, move the screen down if needed
		index = (cur_y >= y - 1) ? sel - y + 1 : 0;
	} else {
		// If selecting upward, move the screen up if needed
		index = (cur_y <= 0) ? sel: sel - cur_y + 1;
	}
  for (int i = 0; i < y; i++) {
    if (i == sel - index) attron(A_REVERSE);
    mvprintw(i, 0, "%s", str[i + index]);
    if (i == sel - index) attroff(A_REVERSE);
  }

	move(sel - index, 0);
	return sel;
}
