// Copyright (c) 2019 Daniel Abrecht
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <libconsolekeyboard.h>

struct keyboard_key {
  const char* display_name;
  const char* escape_sequence;
  int color;
  WINDOW* win;
};

enum colors {
  WHITE_BLACK,
  BLACK_WHITE,
  BLACK_GREEN
};

struct keyboard_key keyboard_matrix[][14] = {
  {{"⇅"},{"ESC","!ESCAPE"},{"PgUp","!PAGE_UP"},{"PgDn","!PAGE_DOWN"},{"HOME","!HOME"},{"END","!END"},{"-"},{"/"},{"|"},{"◀","!LEFT"},{"▶","!RIGHT"},{"▼","!DOWN"},{"▲","!UP"},{"DEL","!DELETE"}},
  {{"§"},{"1"},{"2"},{"3"},{"4"},{"5"},{"6"},{"7"},{"8"},{"9"},{"0"},{"'"},{"^"},{"⌫ BACKSPACE","!BACKSPACE"}},
  {{"TAB","!TAB"},{"q"},{"w"},{"e"},{"r"},{"t"},{"z"},{"u"},{"i"},{"o"},{"p"},{"ü"},{"¨"},{"⏎ ENTER","!ENTER"}},
  {{"⇧⇪"},{"a"},{"s"},{"d"},{"f"},{"g"},{"h"},{"j"},{"k"},{"l"},{"ö"},{"ä"},{"$"}},
  {{"CTRL"},{"<"},{"y"},{"x"},{"c"},{"v"},{"b"},{"n"},{"m"},{","},{"."},{"-"},{" "}},
};

void key_press(struct keyboard_key*const k){
  wbkgd(k->win, COLOR_PAIR(BLACK_GREEN));
  wrefresh(k->win);
  const char* seq = k->escape_sequence;
  if(!seq)
    seq = k->display_name;
  if(!seq)
    return;
  if(seq[0] == '!'){
    lck_send_key(seq+1);
  }else{
    lck_send_string(seq);
  }
  refresh();
}

void key_release(struct keyboard_key*const k){
  wbkgd(k->win, COLOR_PAIR(k->color));
  wrefresh(k->win);
}

enum display_state {
  STATE_WHOLE_KEYBOARD,
  STATE_SHOW_SPECIAL_KEYS_ONLY,
};

enum display_state display_state;

int set_display_state(enum display_state s){
  int ret = 0;
  switch(s){
    case STATE_WHOLE_KEYBOARD: ret = lck_set_height((struct lck_super_size){15}); break;
    case STATE_SHOW_SPECIAL_KEYS_ONLY: ret = lck_set_height((struct lck_super_size){3}); break;
    default: return -1;
  }
  if(ret != -1)
    display_state = s;
  return ret;
}

int main(){
  set_display_state(STATE_WHOLE_KEYBOARD);
  int sleep(int);
  sleep(1); // TODO: do this properly and implement redrawing of keyboard
  setlocale(LC_CTYPE, "C.UTF-8");
  if(!initscr()){
    fprintf(stderr,"initscr failed\n");
    return 1;
  }
  start_color();
  clear();
  noecho();
  cbreak();
  curs_set(0);

  init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
  init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
  init_pair(BLACK_GREEN, COLOR_BLACK, COLOR_GREEN);

  color_set(BLACK_WHITE, 0);

  int nx = 14;
  int ny = 5;
  int w = COLS / nx;
  int h = LINES / ny;
  for(int y=0; y<ny; y++)
  for(int x=0; x<nx; x++){
    struct keyboard_key*const k = &keyboard_matrix[y][x];
    if(!k->display_name)
      continue;
    WINDOW* win = subwin(stdscr, h, w, y*h, x*w);
    int tx = (w-strlen(k->display_name))/2;
    if(tx<0) tx = 0;
    mvwaddstr(win, h/2, tx, k->display_name);
    k->color = (x+y) % 2 ? WHITE_BLACK : BLACK_WHITE;
    wbkgd(win, COLOR_PAIR(k->color));
    k->win = win;
  }

  refresh();

  keypad(stdscr, true);
  mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, 0);
  mouseinterval(0);
  int ch;
  struct keyboard_key* last = 0;
  while( (ch = getch()) != 'q' ){
    switch(ch){
      case KEY_MOUSE: {
        MEVENT event;
        if(getmouse(&event) != OK)
          break;
        if(event.bstate & BUTTON1_RELEASED){
          if(last)
            key_release(last);
        }
        if(event.bstate & BUTTON1_PRESSED){
          if(last)
            key_release(last);
          int x = event.x/w;
          int y = event.y/h;
          if(x>=nx || y>=ny)
            break;
          struct keyboard_key* k = &keyboard_matrix[y][x];
          if(!k->win)
            break;
          key_press(k);
          last = k;
        }
      } break;
    }
  }

  endwin();

  return 0;
}
