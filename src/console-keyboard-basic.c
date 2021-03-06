// Copyright (c) 2019 Daniel Abrecht
// SPDX-License-Identifier: GPL-3.0-or-later

#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <libconsolekeyboard.h>

struct keyboard_key {
  const char* display_name;
  const char* escape_sequence;
  void(*callback)(void);
  int color;
  WINDOW* win;
};

enum colors {
  WHITE_BLACK,
  BLACK_WHITE,
  BLACK_GREEN
};

static struct winsize winsize;
static int winchfd[2];

void next_mode(void);
void shift(void);
void toggle_ctrl(void);

struct keyboard_key keyboard_matrix_default[5][14] = {
  {{"⇅",0,next_mode},{"ESC","!ESCAPE"},{"PgUp","!PAGE_UP"},{"PgDn","!PAGE_DOWN"},{"HOME","!HOME"},{"END","!END"},{"&"},{"/"},{"|"},{"◀","!LEFT"},{"▶","!RIGHT"},{"▼","!DOWN"},{"▲","!UP"},{"DEL","!DELETE"}},
  {{":"},{"1"},{"2"},{"3"},{"4"},{"5"},{"6"},{"7"},{"8"},{"9"},{"0"},{"'"},{"\""},{"⌫ BACKSPACE","!BACKSPACE"}},
  {{"TAB","!TAB"},{"q"},{"w"},{"e"},{"r"},{"t"},{"z"},{"u"},{"i"},{"o"},{"p"},{"["},{"]"},{"⏎ ENTER","!ENTER"}},
  {{"⇧",0,shift},{"a"},{"s"},{"d"},{"f"},{"g"},{"h"},{"j"},{"k"},{"l"},{"{"},{"}"},{"$"},{" "}},
  {{"CTRL",0,toggle_ctrl},{"\\"},{"y"},{"x"},{"c"},{"v"},{"b"},{"n"},{"m"},{"."},{","},{";"},{"-"},{" "}},
};
struct keyboard_key keyboard_matrix_shift[5][14] = {
  {{"⇅",0,next_mode},{"ESC","!ESCAPE"},{"PgUp","!PAGE_UP"},{"PgDn","!PAGE_DOWN"},{"HOME","!HOME"},{"END","!END"},{"&"},{"/"},{"|"},{"◀","!LEFT"},{"▶","!RIGHT"},{"▼","!DOWN"},{"▲","!UP"},{"DEL","!DELETE"}},
  {{"#"},{"+"},{"@"},{"#"},{"*"},{"%"},{"&"},{"~"},{"("},{")"},{"="},{"?"},{"^"},{"⌫ BACKSPACE","!BACKSPACE"}},
  {{"TAB","!TAB"},{"Q"},{"W"},{"E"},{"R"},{"T"},{"Z"},{"U"},{"I"},{"O"},{"P"},{"["},{"]"},{"⏎ ENTER","!ENTER"}},
  {{"⇪",0,shift},{"A"},{"S"},{"D"},{"F"},{"G"},{"H"},{"J"},{"K"},{"L"},{"<"},{">"},{"$"},{" "}},
  {{"CTRL",0,toggle_ctrl},{"\\"},{"Y"},{"X"},{"C"},{"V"},{"B"},{"N"},{"M"},{"."},{","},{"!"},{"_"},{" "}},
};
struct keyboard_key (*keyboard_matrix)[5][14] = &keyboard_matrix_default;
static const int nx = 14;
static int ny = 5;

enum shift_state {
  ST_NO_SHIFT,
  ST_ONE_SHIFT,
  ST_SHIFT,
  ST_COUNT
};

enum shift_state shift_state = ST_NO_SHIFT;
bool ctrl = false;

void redraw(void);
int set_shift_state(enum shift_state s){
  if(shift_state == s)
    return 0;
  switch(s){
    case ST_NO_SHIFT: keyboard_matrix = &keyboard_matrix_default; break;
    case ST_ONE_SHIFT:
    case ST_SHIFT: keyboard_matrix = &keyboard_matrix_shift; break;
    default: return -1;
  }
  shift_state = s;
  redraw();
  if(s == ST_SHIFT){
    struct keyboard_key*const k = &(*keyboard_matrix)[3][0];
    wbkgd(k->win, COLOR_PAIR(BLACK_GREEN));
    wrefresh(k->win);
  }
  return 0;
}

void shift(void){
  set_shift_state((shift_state+1) % ST_COUNT);
}

void key_release(struct keyboard_key*const k){
  if(k == &keyboard_matrix_default[3][0] || k == &keyboard_matrix_shift[3][0])
    return;
  if(ctrl && k == &(*keyboard_matrix)[4][0])
    return;
  wbkgd(k->win, COLOR_PAIR(k->color));
  wrefresh(k->win);
}

void key_press(struct keyboard_key*const k){
  wbkgd(k->win, COLOR_PAIR(BLACK_GREEN));
  wrefresh(k->win);
  if(k->callback){
    k->callback();
    return;
  }
  const char* seq = k->escape_sequence;
  if(!seq)
    seq = k->display_name;
  if(!seq)
    return;
  if(seq[0] == '!'){
    lck_send_key(seq+1);
  }else{
    enum lck_key_modifier_mask mask = 0;
    if(ctrl)
      mask |= LCK_MODIFIER_KEY_CTRL;
    lck_send_string(seq, mask);
  }
  if(ctrl){
    ctrl = false;
    struct keyboard_key*const k = &(*keyboard_matrix)[4][0];
    wbkgd(k->win, COLOR_PAIR(k->color));
    wrefresh(k->win);
  }
  if( shift_state == ST_ONE_SHIFT && (k != &keyboard_matrix_default[3][0] || k != &keyboard_matrix_shift[3][0]) )
    set_shift_state(ST_NO_SHIFT);
  refresh();
}

enum display_state {
  STATE_WHOLE_KEYBOARD,
  STATE_SHOW_SPECIAL_KEYS_ONLY,
  STATE_COUNT
};

enum display_state display_state = STATE_COUNT;

void redraw(void);

int set_display_state(enum display_state s){
  if(display_state == s)
    return 0;
  int ret = 0;
  switch(s){
    case STATE_WHOLE_KEYBOARD: {
      ny = 5;
      ret = lck_set_height((struct lck_super_size){15});
    } break;
    case STATE_SHOW_SPECIAL_KEYS_ONLY: {
      ny=1;
      ret = lck_set_height((struct lck_super_size){3});
    } break;
    default: return -1;
  }
  display_state = s;
  redraw();
  return ret;
}

void toggle_ctrl(){
  ctrl = !ctrl;
  if(ctrl){
    struct keyboard_key*const k = &(*keyboard_matrix)[4][0];
    wbkgd(k->win, COLOR_PAIR(ctrl?BLACK_GREEN:k->color));
    wrefresh(k->win);
  }
}

void next_mode(void){
  set_display_state( (display_state+1) % STATE_COUNT );
}

void onwinch(int sig){
  (void)sig;
  while(write(winchfd[1], (uint8_t[]){1}, 1) == -1 && errno == EINTR);
}

int init(void){

  for(int y=0; y<5; y++)
  for(int x=0; x<nx; x++){
    struct keyboard_key*const k = &(*keyboard_matrix)[y][x];
    if(!k->win)
      continue;
    delwin(k->win);
    k->win = 0;
  }

  ioctl(0, TIOCGWINSZ, &winsize);

  // Using endwin and initscr instead of resizeterm, because resizeterm failed to reset the scrolling region.
  endwin();
  if(!initscr()){
    fprintf(stderr,"initscr failed\n");
    return -1;
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

  keypad(stdscr, true);
  mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, 0);
  mouseinterval(0);

  return 0;
}

void redraw(void){
  int w = winsize.ws_col / nx;
  int h = winsize.ws_row / ny;
  clear();
  for(int y=0; y<ny; y++)
  for(int x=0; x<nx; x++){
    struct keyboard_key*const k = &(*keyboard_matrix)[y][x];
    if(!k->display_name)
      continue;
    WINDOW* win = subwin(stdscr, h, w, y*h, x*w);
    int tx = (w-strlen(k->display_name))/2;
    if(tx<0) tx = 0;
    mvwaddstr(win, h/2, tx, k->display_name);
    k->color = (x+y) % 2 ? WHITE_BLACK : BLACK_WHITE;
    if(x==0 && y==4 && ctrl)
      k->color = BLACK_GREEN;
    wbkgd(win, COLOR_PAIR(k->color));
    k->win = win;
    wrefresh(win);
  }
  refresh();
}

int main(){
  if(pipe(winchfd) == -1){
    fprintf(stderr,"pipe failed\n");
    return 1;
  }
  signal(SIGWINCH, onwinch);
  set_display_state(STATE_WHOLE_KEYBOARD);
  setlocale(LC_CTYPE, "C.UTF-8");

  if(init())
    return 1;
  redraw();

  enum {
    PFD_INPUT,
    PFD_WINCH,
  };

  struct pollfd fds[] = {
    [PFD_INPUT] = {
      .fd = 0,
      .events = POLLIN
    },
    [PFD_WINCH] = {
      .fd = winchfd[0],
      .events = POLLIN
    },
  };
  size_t nfds = sizeof(fds)/sizeof(*fds);

  struct keyboard_key* last = 0;
  while(true){
    int ret = poll(fds, nfds, -1);
    if( ret == -1 ){
      if( errno == EINTR )
        continue;
      perror("poll failed");
      return 1;
    }
    if(!ret)
      continue;
    if(fds[PFD_WINCH].revents & POLLIN){
      uint8_t c;
      ssize_t res = read(winchfd[0], &c, 1);
      if(res == -1 || c != 1)
        continue;
      if(init())
        return 1;
      redraw();
    }
    if(fds[PFD_INPUT].revents & POLLIN){
      int ch = getch();
      if(ch == '\x1B') break;
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
            int w = winsize.ws_col / nx;
            int h = winsize.ws_row / ny;
            int x = event.x/w;
            int y = event.y/h;
            if(x>=nx || y>=ny)
              break;
            struct keyboard_key* k = &(*keyboard_matrix)[y][x];
            if(!k->win)
              break;
            key_press(k);
            last = k;
          }
        } break;
      }
    }
  }

  endwin();

  return 0;
}
