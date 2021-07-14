#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>

#define BOARD_HEIGHT 15
#define BOARD_WIDTH 40
#define SNEK_CHAR '#'
#define FOOD_CHAR '$'

//Snek and coordinate structures
struct pair{
  int x;
  int y;
};

struct Snek{
  struct pair body[BOARD_HEIGHT*BOARD_WIDTH];
  int tail;
  int head;
  int max_size;
};

struct pair board[BOARD_HEIGHT*BOARD_WIDTH];

//Declare functions
void init_snek(WINDOW* window, struct Snek* snek, int length, int y, int x);
void game_loop(WINDOW* window, struct Snek* snek);
void move_snek(WINDOW* window, struct Snek* snek, int ch);
void food(WINDOW* window, struct Snek* snek);
void gameover(WINDOW* window, struct Snek* snek);

//MAIN
int main()
{

  //Init variables
  WINDOW* game_win;
  struct Snek* snek1 = (struct Snek*)malloc(sizeof(struct Snek));

  //Start ncurses and setup environment
	initscr(); //Default ncurses screen (stdscr)
  cbreak(); //Continuous key input
  noecho(); //No input echo
  curs_set(0); //Invisible cursor
  start_color(); //Enable colors
  init_pair(1,COLOR_GREEN,COLOR_BLACK);
  init_pair(2,COLOR_YELLOW,COLOR_BLACK);
  init_pair(3,COLOR_RED,COLOR_BLACK);
  printw("Press BACKSPACE to exit\n");
  refresh();

  //Create main game window in center of screen
  game_win = newwin(BOARD_HEIGHT,BOARD_WIDTH,(LINES-BOARD_HEIGHT)/2,(COLS-BOARD_WIDTH)/2);
  keypad(game_win, TRUE); //Recieve special characters
  nodelay(game_win, TRUE);
  wrefresh(game_win);

  //Spawn border, snek and food
  init_snek(game_win, snek1, 10, BOARD_HEIGHT/2, BOARD_WIDTH/2);

  //Game loop until BACKSPACE is hit
  game_loop(game_win, snek1);

  //Close window
	endwin();
	return 0;
}


//FUNCTIONS

//Create snek structure, then draw snek of 'length' size, spawn food and draw border
void init_snek(WINDOW* window, struct Snek* snek, int length, int y, int x){
  wclear(window);
  wrefresh(window);
  box(window,0,0);
  wattron(window,COLOR_PAIR(1));
  x += x/2;
  for(int i = 0; i < length; i++){
    snek->body[i].y = y;
    snek->body[i].x = x;
    x--;
    snek->tail = 0;
    snek->head = length;
    snek->max_size = BOARD_WIDTH*BOARD_HEIGHT;
  }

  for(int i = snek->tail; i < snek->head; i++){
    mvwaddch(window, snek->body[i].y, snek->body[i].x, SNEK_CHAR);
  }
  wattroff(window,COLOR_PAIR(1));
  wrefresh(window);
  food(window, snek);
  return;
}

//Gameplay loop until BACKSPACE is hit
void game_loop(WINDOW* window, struct Snek* snek){
  int ch;
  while((ch = wgetch(window)) != KEY_BACKSPACE){
    flushinp();
    move_snek(window, snek, ch);
    usleep(100*1000);
  }
  return;
}

//Check keyboard input and move accordingly
//If the space being moved into is occupied then eat or die
void move_snek(WINDOW* window, struct Snek* snek, int ch){

  static int direction = KEY_LEFT;
  int prev_head;

  //Prevent going backwards
  if((direction == KEY_LEFT  && ch == KEY_RIGHT) ||
     (direction == KEY_RIGHT && ch == KEY_LEFT)  ||
     (direction == KEY_UP    && ch == KEY_DOWN)  ||
     (direction == KEY_DOWN  && ch == KEY_UP)){
       ch = direction;
  }
  //Ignore invalid inputs
  if(ch != KEY_LEFT && ch != KEY_RIGHT && ch != KEY_DOWN && ch != KEY_UP){
    ch = direction;
  }
  //Take previous head and increment based on which direction snek is moving
  if(snek->head == 0){
    prev_head = snek->max_size-1;
  }
  else{
    prev_head = snek->head-1;
  }

  switch(ch){
    case KEY_LEFT:
      snek->body[snek->head].x = snek->body[prev_head].x - 1;
      snek->body[snek->head].y = snek->body[prev_head].y;
      break;
    case KEY_RIGHT:
      snek->body[snek->head].x = snek->body[prev_head].x + 1;
      snek->body[snek->head].y = snek->body[prev_head].y;
      break;
    case KEY_DOWN:
      snek->body[snek->head].y = snek->body[prev_head].y + 1;
      snek->body[snek->head].x = snek->body[prev_head].x;
      break;
    case KEY_UP:
      snek->body[snek->head].y = snek->body[prev_head].y - 1;
      snek->body[snek->head].x = snek->body[prev_head].x;
      break;
  }

  //Check what character is in next coordinates
  int inch = mvwinch(window, snek->body[snek->head].y, snek->body[snek->head].x) & A_CHARTEXT;
  switch(inch){
    case ' ': //If empty, move tail up
      mvwaddch(window, snek->body[snek->tail].y, snek->body[snek->tail].x, ' ');
      snek->tail++;
      snek->tail %= (snek->max_size);
      break;
    case FOOD_CHAR: //If snek found food spawn new food
      food(window, snek);
      break;
    default: //If snek hit anything else it's probably dead
      gameover(window, snek);
      direction = KEY_LEFT;
      return;
  }

  //Move head forward
  wattron(window, COLOR_PAIR(1));
  mvwaddch(window, snek->body[snek->head].y, snek->body[snek->head].x, SNEK_CHAR);
  wattroff(window, COLOR_PAIR(1));
  wrefresh(window);
  snek->head++;
  snek->head %= (snek->max_size);

  direction = ch;
  return;
}

void food(WINDOW* window, struct Snek* snek){
  //Generate random coordinates within screen size and draw new food
  int i = 0;
  int is_snek = 0;
  for(int y = 1; y < BOARD_HEIGHT - 1; y++){
    for(int x = 1; x < BOARD_WIDTH - 1; x++){
      is_snek = 0;
      for(int s = snek->tail; s < snek->head; s++){
        s %= (snek->max_size); //Incase snek wraps around buffer
        if(y == snek->body[s].y && x == snek->body[s].x){
          is_snek = 1;
          break;
        }
      }
      //Create array of coordinates that are not currently occupied
      //by the snek or borders
      if(!is_snek){
        board[i].y = y;
        board[i].x = x;
        i++;
      }
    }
  }
  //Get random number within range of array
  int rnd = (rand() % i);
  //Draw new food to coordinates
  wattron(window,COLOR_PAIR(2));
  mvwaddch(window, board[rnd].y, board[rnd].x, FOOD_CHAR);
  wattroff(window,COLOR_PAIR(2));
  wrefresh(window);
  return;

}

void gameover(WINDOW* window, struct Snek* snek){
  //Draw gameover screen and wait for keypress to either restart or quit
  int ch;
  WINDOW* gameover_win = newwin(BOARD_HEIGHT,BOARD_WIDTH,(LINES-BOARD_HEIGHT)/2,(COLS-BOARD_WIDTH)/2);

  keypad(gameover_win, TRUE);
  nodelay(gameover_win, TRUE);
  box(gameover_win, 0, 0);

  wattron(gameover_win, COLOR_PAIR(3));
  mvwprintw(gameover_win, (BOARD_HEIGHT/2) - 1, (BOARD_WIDTH/2) - 4, "GAMEOVER");
  wattroff(gameover_win, COLOR_PAIR(3));
  mvwprintw(gameover_win, BOARD_HEIGHT/2, (BOARD_WIDTH/2) - 13, "Press ENTER to play again!");
  wrefresh(gameover_win);

  while((ch = wgetch(gameover_win)) != KEY_BACKSPACE){
    flushinp();
    if(ch == '\n'){
      wclear(gameover_win);
      delwin(gameover_win);
      //Creates fresh snek game and returns to gameplay loop
      init_snek(window, snek, 10, BOARD_HEIGHT/2, BOARD_WIDTH/2);
      return;
    }
    usleep(100*1000);
  }

  endwin();
  exit(0);
}
