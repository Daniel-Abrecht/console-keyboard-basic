#include <curses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct keyboard_key {
  const char* display_name;
  const char* escape_sequence;
  WINDOW* win;
};

struct keyboard_key keyboard_matrix[][14] = {
  {{"§"},{"1"},{"2"},{"3"},{"4"},{"5"},{"6"},{"7"},{"8"},{"9"},{"0"},{"'"},{"^"},{"BACKSPACE","\b"}},
  {{"TAB","\t"},{"q"},{"w"},{"e"},{"r"},{"t"},{"z"},{"u"},{"i"},{"o"},{"p"},{"ü"},{"¨"},{"ENTER","\n"}},
  {{"CAPS"},{"a"},{"s"},{"d"},{"f"},{"g"},{"h"},{"j"},{"k"},{"l"},{"ö"},{"ä"},{"$"}},
  {{"SHIFT"},{"<"},{"y"},{"x"},{"c"},{"v"},{"b"},{"n"},{"m"},{","},{"."},{"-"}},
};

int main(){
  if(!initscr()){
    fprintf(stderr,"initscr failed\n");
    return 1;
  }
  start_color();
  clear();
  noecho();
  cbreak();

  enum {
    WHITE_BLACK,
    BLACK_WHITE,
    BLACK_GREEN
  };

  init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
  init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
  init_pair(BLACK_GREEN, COLOR_BLACK, COLOR_GREEN);

  color_set(BLACK_WHITE, 0);

  int nx = 14;
  int ny = 4;
  int w = COLS / nx;
  int h = LINES / ny;
  for(int y=0; y<ny; y++)
  for(int x=0; x<nx; x++){
    if(!keyboard_matrix[y][x].display_name)
      continue;
    WINDOW* win = subwin(stdscr, h, w, y*h, x*w);
    int tx = (w-strlen(keyboard_matrix[y][x].display_name))/2;
    if(tx<0) tx = 0;
    mvwaddstr(win, h/2, tx, keyboard_matrix[y][x].display_name);
    wbkgd(win, COLOR_PAIR((x+y)%2?WHITE_BLACK:BLACK_WHITE));
    keyboard_matrix[y][x].win = win;
  }

  refresh();

  keypad(stdscr, true);
  mousemask(ALL_MOUSE_EVENTS, 0);
  int ch;
  while( (ch = getch()) != 'q' ){
    switch(ch){
      case KEY_MOUSE: {
        MEVENT event;
        if(getmouse(&event) != OK)
          break;
        if(event.bstate & BUTTON1_PRESSED){
          int mx = event.x/w;
          int my = event.y/h;
          if(mx>=nx || my>=ny)
            break;
          if(!keyboard_matrix[my][mx].win)
            break;
          wbkgd(keyboard_matrix[my][mx].win, COLOR_PAIR(BLACK_GREEN));
          wrefresh(keyboard_matrix[my][mx].win);
          const char* seq = keyboard_matrix[my][mx].escape_sequence;
          if(!seq)
            seq = keyboard_matrix[my][mx].display_name;
          write(3,seq,strlen(seq));
          refresh();
          sleep(1);
          wbkgd(keyboard_matrix[my][mx].win, COLOR_PAIR((mx+my)%2?WHITE_BLACK:BLACK_WHITE));
          wrefresh(keyboard_matrix[my][mx].win);
          refresh();
        }
      } break;
    }
  }

  endwin();

  return 0;
}
