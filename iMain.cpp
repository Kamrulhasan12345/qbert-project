#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "iGraphics.h"
#include "iSound.h"

Image bg, help, life, frames[2], frames_1[2], spin_frame[6], ball_frame[2], qbert_invert[2], qbert,
    pause_button, pause_text, dialogue, gameover;
Sprite snake, qbert_jump, qbert_spin, ball, qbert_inverse;

#define PI 3.14159265
#define MAX_SIZE 10
#define ESC 0x1b
#define NUM_ENEMIES 2

typedef enum {
  STATE_MENU,
  STATE_GAME,
  STATE_GAME_MENU,
  STATE_EDITOR,
  STATE_RESUME,
  STATE_SETTING,
  STATE_HELP,
  STATE_HIGHSCORE,
  STATE_CREDITS,
  STATE_EXIT
} app_t;

typedef enum { TYPE_BLOCK, TYPE_PLAYER, TYPE_ENEMY, TYPE_NULL } object_t;

typedef struct {
  bool wireframe;
  bool grid;
  bool debug;
} editor_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} color_t;

typedef struct {
  bool valid;
  int state;
} tile_t;

typedef enum { LOOK_LEFT, LOOK_RIGHT, LOOK_UP, LOOK_DOWN } look_t;

typedef enum { ENEMY_COILY, ENEMY_UGG, ENEMY_WRONGWAY, ENEMY_SAM } enemytype_t;

typedef struct {
  double x, y, z;
} position_t;

typedef struct {
  position_t from;
  position_t to;
  float t, duration;
  bool active;
} jumper_t;

typedef struct {
  position_t pos;
  look_t la;
  jumper_t jump;
} body_t;

typedef struct {
  body_t km;
  int lives;
  int max_lives;
  int score;
  bool ko;
} player_t;

typedef struct {
  body_t km;
  enemytype_t type;
} enemy_t;

typedef struct {
  object_t type;
  position_t pos;
  uint8_t flags;
  void *ref;
} drawqueue_t;

typedef struct {
  double *(blocks[3]);
  int blocks_count;
  double *(visible[3]);
  int visible_count;
  int8_t level_num, state_num;
  color_t states[3];
  color_t l_color, r_color;
  enemy_t *enemies;
} world_t;

app_t app_state = STATE_MENU;

editor_t editor = {.wireframe = false, .grid = false, .debug = false};

// take this to world_t
tile_t tiles[MAX_SIZE][MAX_SIZE][MAX_SIZE];

drawqueue_t drawqueue[MAX_SIZE * MAX_SIZE * MAX_SIZE + 1 + NUM_ENEMIES + 1];

player_t player;

// world_t
enemy_t enemies[NUM_ENEMIES];

int enemy_step_timer;

int width = 800, height = 800;
int sound_1 = -1, sound_2 = -1, sound_3 = -1;

double start_x = width / 2.0;
double start_y = height * 0.9;
double tile_width = 40;
double tile_height = 40;

bool sound1 = true, sound2 = false, sound3 = false;
bool selected_yes = true;
bool selected_no = false;
bool pause = false;
bool hover_start = false, hover_resume = false, hover_setting = false, hover_help = false,
     hover_high = false;
bool hover_credits = false, hover_exit = false, endgame = false;

double blocksPos3d[][3] = {{7, 7, 0},
                           {6, 7, 1},
                           {5, 7, 2},
                           {4, 7, 3},
                           {3, 7, 4},
                           {2, 7, 5},
                           {1, 7, 6},
                           {0, 7, 7},
                           {6, 6, 0},
                           {5, 6, 1},
                           {4, 6, 2},
                           {3, 6, 3},
                           {2, 6, 4},
                           {1, 6, 5},
                           {0, 6, 6},
                           {5, 5, 0},
                           {4, 5, 1},
                           {3, 5, 2},
                           {2, 5, 3},
                           {1, 5, 4},
                           {0, 5, 5},
                           {4, 4, 0},
                           {3, 4, 1},
                           {2, 4, 2},
                           {1, 4, 3},
                           {0, 4, 4},
                           {3, 3, 0},
                           {2, 3, 1},
                           {1, 3, 2},
                           {0, 3, 3},
                           {2, 2, 0},
                           {1, 2, 1},
                           {0, 2, 2},
                           {1, 1, 0},
                           {0, 1, 1},
                           {0, 0, 0},
                           {0, 0, 1},
                           {1, 4, 4},
                           // {2,3,5}
                           // {5,4,3},
                           {4, 5, 2},
                           {3, 7, 5}};

double visible[100][3];

int visible_count;

color_t coily = {.r = 10, .g = 100, .b = 240};
color_t ugg = {.r = 100, .g = 200, .b = 150};
color_t sam = {.r = 10, .g = 255, .b = 200};

color_t states[3] = {
    {.r = 86, .g = 70, .b = 239}, {.r = 222, .g = 222, .b = 0}, {.r = 20, .g = 200, .b = 239}};
color_t l_color = {.r = 86, .g = 169, .b = 152}, r_color = {.r = 49, .g = 70, .b = 70};

position_t dirs[4] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};

int state_num = 3;

int n = sizeof(blocksPos3d) / sizeof(blocksPos3d[0]);

int cmp_dk(const void *a, const void *b) {
  drawqueue_t *A = (drawqueue_t *)a;
  drawqueue_t *B = (drawqueue_t *)b;
  int d1 = A->pos.x + A->pos.z + (MAX_SIZE - 1 - A->pos.y) + A->type;
  int d2 = B->pos.x + B->pos.z + (MAX_SIZE - 1 - B->pos.y) + B->type;
  return (d1 > d2) - (d1 < d2);
}

void iLoadResource() {
  iLoadImage(&bg, "assets/images/title.png");
  iLoadImage(&help, "assets/images/help.png");
  iLoadImage(&life, "assets/images/sprites/qbert/qbert06.png");
  iLoadImage(&qbert, "assets/images/sprites/qbert/qbert06.png");
  iLoadImage(&pause_button, "assets/images/pausebutton.png");
  iLoadImage(&pause_text, "assets/images/paused.png");
  iLoadImage(&dialogue, "assets/images/dialogue.png");
  iLoadImage(&gameover, "assets/images/gameover.png");
  iResizeImage(&qbert, 35, 40);
  iResizeImage(&help, 750, 700);
  iResizeImage(&life, 23, 23);
  iScaleImage(&pause_text, 1.6);
  iScaleImage(&dialogue, 1.5);
  iScaleImage(&gameover, 0.7);
  iInitSprite(&snake);
  iInitSprite(&qbert_jump);
  iInitSprite(&qbert_spin);
  iInitSprite(&ball);
  iInitSprite(&qbert_inverse);
  iLoadFramesFromFolder(frames, "assets/images/sprites/snake");
  iLoadFramesFromFolder(frames_1, "assets/images/sprites/qbert_jump");
  iLoadFramesFromFolder(spin_frame, "assets/images/sprites/spin");
  iLoadFramesFromFolder(ball_frame, "assets/images/sprites/ball");
  iLoadFramesFromFolder(qbert_invert, "assets/images/sprites/qbert_invert");
  iChangeSpriteFrames(&snake, frames, 2);
  iChangeSpriteFrames(&qbert_jump, frames_1, 2);
  iChangeSpriteFrames(&qbert_spin, spin_frame, 6);
  iChangeSpriteFrames(&ball, ball_frame, 2);
  iChangeSpriteFrames(&qbert_inverse, qbert_invert, 2);
  iScaleSprite(&snake, 2.0);
  iScaleSprite(&qbert_inverse, 2.0);
  iScaleSprite(&ball, 2.0);
  iScaleSprite(&qbert_jump, 2.0);
  iSetSpritePosition(&snake, -50, 400);
  iSetSpritePosition(&qbert_jump, -5, 400);
  iSetSpritePosition(&qbert_spin, 400, 200);
  iSetSpritePosition(&qbert_inverse, 801, 195);
  iSetSpritePosition(&ball, 850, 200);
}

void iLoadLevel(int level) {
  // load a selective level into global variables
}

void iCompleteLevel() {
  // load splash screen and then load next level
}

void iClearQueue() {
  for (int i = 0; i < MAX_SIZE * MAX_SIZE * MAX_SIZE + 1; i++)
    drawqueue[i].type = TYPE_NULL, drawqueue[i].pos.x = 0, drawqueue[i].pos.y = 0,
    drawqueue[i].pos.z = 0, drawqueue[i].flags = 0;
}

void iAnim() {
  if (app_state == STATE_MENU) {
    qbert_jump.x += 20;
    snake.x += 20;
    qbert_inverse.x -= 20;
    ball.x -= 20;

    if (snake.x > 800) {
      snake.x = -50;
    }
    if (qbert_jump.x > 800) {
      qbert_jump.x = -5;
    }
    if (qbert_inverse.x < 0) {
      qbert_inverse.x = 801;
    }
    if (ball.x < 0) {
      ball.x = 850;
    }
  }
  iAnimateSprite(&snake);
  iAnimateSprite(&qbert_jump);
  iAnimateSprite(&qbert_inverse);
  iAnimateSprite(&ball);
}

void iAnimSetting() {
  iAnimateSprite(&qbert_spin);
}

void iTile(double x, double y) {
  double x_coords[] = {x, x + tile_width * cos(PI / 6), x, x - tile_width * cos(PI / 6)};
  double y_coords[] = {y, y - tile_width / 2, y - tile_width, y - tile_width / 2};
  iFilledPolygon(x_coords, y_coords, 4);
}

void iTileOutline(double x, double y) {
  double x_coords[] = {x, x + tile_width * cos(PI / 6), x, x - tile_width * cos(PI / 6)};
  double y_coords[] = {y, y - tile_width / 2, y - tile_width, y - tile_width / 2};
  iPolygon(x_coords, y_coords, 4);
}

void iSide(double x, double y) {
  double x_coords[] = {x, x, x + tile_width * cos(PI / 6), x + tile_width * cos(PI / 6)};
  double y_coords[] = {y - tile_width, y - tile_width - tile_height,
                       y - tile_width / 2 - tile_height, y - tile_width / 2};
  iSetColor(r_color.r, r_color.g, r_color.b);
  iFilledPolygon(x_coords, y_coords, 4);
  x_coords[2] = x - tile_width * cos(PI / 6), x_coords[3] = x - tile_width * cos(PI / 6);
  iSetColor(l_color.r, l_color.g, l_color.b);
  iFilledPolygon(x_coords, y_coords, 4);
}

void iSideOutline(double x, double y) {
  double x_coords[] = {x, x, x + tile_width * cos(PI / 6), x + tile_width * cos(PI / 6)};
  double y_coords[] = {y - tile_width, y - tile_width - tile_height,
                       y - tile_width / 2 - tile_height, y - tile_width / 2};
  iPolygon(x_coords, y_coords, 4);
  x_coords[2] = x - tile_width * cos(PI / 6), x_coords[3] = x - tile_width * cos(PI / 6);
  iPolygon(x_coords, y_coords, 4);
}

void iDrawEnemy(enemy_t *enemy) {
  switch (enemy->type) {
  case ENEMY_COILY:
    iSetColor(coily.r, coily.g, coily.b);
    break;
  case ENEMY_UGG:
    iSetColor(ugg.r, ugg.g, ugg.b);
    break;
  case ENEMY_SAM:
    iSetColor(sam.r, sam.g, sam.b);
    break;
  default:
    iSetColor(255, 0, 0);
    break;
  }
  // printf("%d %d
  // %d\n",(int)enemy->km.pos.x,(int)enemy->km.pos.y,(int)enemy->km.pos.z);
  iSetColor(255, 0, 0);
  iFilledCircle(start_x + (enemy->km.pos.z - enemy->km.pos.x) * tile_width * cos(PI / 6),
                start_y - (enemy->km.pos.z + enemy->km.pos.x) * tile_width / 2 -
                    enemy->km.pos.y * tile_height - tile_width / 2,
                tile_width / 5);
}

bool collision(player_t *player, enemy_t *enemy) {
  if ((player->km.pos.x == enemy->km.pos.x) && (player->km.pos.y == enemy->km.pos.y) &&
      (player->km.pos.z == enemy->km.pos.z))
    return true;
  else
    return false;
}

void iDrawQueue() {
  // current approach: use a draw queue
  iClearQueue();
  int i = 0, j;
  for (int y = MAX_SIZE - 1; y >= 0; y--) {
    for (int x = 0; x < MAX_SIZE; x++) {
      for (int z = 0; z < MAX_SIZE; z++) {
        if (!tiles[y][x][z].valid)
          continue;
        drawqueue[i].pos.x = x, drawqueue[i].pos.y = y, drawqueue[i].pos.z = z;
        drawqueue[i].flags = tiles[y][x][z].state;
        drawqueue[i].ref = &tiles[y][x][z];
        drawqueue[i++].type = TYPE_BLOCK;
      }
    }
  }
  if (app_state == STATE_GAME) {
    drawqueue[i].pos.x = player.km.pos.x, drawqueue[i].pos.y = player.km.pos.y,
    drawqueue[i].pos.z = player.km.pos.z;
    drawqueue[i].flags = player.km.jump.active;
    drawqueue[i].ref = &player;
    drawqueue[i++].type = TYPE_PLAYER;
    for (j = 0; j < NUM_ENEMIES; j++) {
      drawqueue[i].pos.x = enemies[j].km.pos.x, drawqueue[i].pos.y = enemies[j].km.pos.y,
      drawqueue[i].pos.z = enemies[j].km.pos.z;
      drawqueue[i].flags = enemies[j].type;
      drawqueue[i].ref = &enemies[j];
      drawqueue[i++].type = TYPE_ENEMY;
    }
  }
  qsort(drawqueue, i, sizeof(drawqueue_t), cmp_dk);
  // draw them now
  // for(int k = 0; k < i; k++) printf("p: %lf %lf %lf f: %d t:
  // %d\n",drawqueue[k].x,drawqueue[k].y,drawqueue[k].z,drawqueue[k].flags,drawqueue[k].type);
  for (j = 0; j < i; j++) {
    double x = drawqueue[j].pos.x, y = drawqueue[j].pos.y, z = drawqueue[j].pos.z;
    switch (drawqueue[j].type) {
    case TYPE_BLOCK: {
      if (!editor.wireframe) {
        iSetColor(states[tiles[(int)y][(int)x][(int)z].state % state_num].r,
                  states[tiles[(int)y][(int)x][(int)z].state % state_num].g,
                  states[tiles[(int)y][(int)x][(int)z].state % state_num].b);
        iTile(start_x + (z - x) * tile_width * cos(PI / 6),
              start_y - (z + x) * tile_width / 2 - y * tile_height);
        iSide(start_x + (z - x) * tile_width * cos(PI / 6),
              start_y - (z + x) * tile_width / 2 - y * tile_height);
      } else {
        iSetColor(r_color.r, r_color.g, r_color.b);
        iTileOutline(start_x + (z - x) * tile_width * cos(PI / 6),
                     start_y - (z + x) * tile_width / 2 - y * tile_height);
        iSideOutline(start_x + (z - x) * tile_width * cos(PI / 6),
                     start_y - (z + x) * tile_width / 2 - y * tile_height);
      }
      break;
    }
    case TYPE_PLAYER: {
      iSetTransparentColor(0, 0, 0, 0.5);
      iShowLoadedImage(start_x + (z - x) * tile_width * cos(PI / 6),
                       start_y - (z + x) * tile_width / 2 - y * tile_height - tile_width / 2,
                       &qbert);
      iSetColor(240, 10, 10);
      // iFilledCircle(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,a/3);
      break;
    }
    case TYPE_ENEMY: {
      iDrawEnemy((enemy_t *)drawqueue[j].ref);
      break;
    }
    default:
      break;
    }
  }
}

void iGrid() {
  double c = start_x / sqrt(3) + start_y +
             ((int)((width - start_x) / sqrt(3) + height - start_y)) * tile_height;
  iSetTransparentColor(255, 255, 255, 0.25);
  for (; c >= 0; c -= tile_width)
    iLine(0, c, width, -width / sqrt(3) + c);
  c = start_y - start_x / sqrt(3) + ((int)(height - start_y + start_x / sqrt(3))) * tile_height;
  for (; width / sqrt(3) + c >= 0; c -= tile_width)
    iLine(0, c, width, width / sqrt(3) + c);
}

void iMenu() {
  app_state = STATE_MENU;
  iSetColor(32, 56, 94);
  iFilledRectangle(0, 0, 800, 800);
  iShowLoadedImage(88, 580, &bg);
  if (hover_start) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 494, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 40, 508, "Start");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 500, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 42, 510, "Start");
  }
  if (hover_resume) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 435 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 47, 445, "Resume");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 435, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 47, 445, "Resume");
  }
  if (hover_setting) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 370 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 50, 380, "Setting");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 370, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 50, 380, "Setting");
  }
  if (hover_help) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 305 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 37, 315, "Help");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 305, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 40, 315, "Help");
  }
  if (hover_high) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 240 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 60, 250, "High Score");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 240, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 60, 250, "High Score");
  }
  if (hover_credits) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 175 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 50, 185, "Credits");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 175, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 50, 185, "Credits");
  }
  if (hover_exit) {
    iSetColor(232, 163, 26);
    iFilledRectangle(width / 2 - 107, 110 - 6, 170, 38);
    iSetColor(101, 67, 33);
    iTextBold(width / 2 - 42, 120, "Exit");
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(width / 2 - 100, 110, 150, 30);
    iSetColor(255, 51, 51);
    iTextBold(width / 2 - 42, 120, "Exit");
  }
}

void iResume();

void iPauseMenu() {
  pause = true;
  iSetTransparentColor(32, 56, 94, 0.95);
  iFilledRectangle(187, 200, 500, 500);
  iSetColor(235, 73, 52);
  iFilledRectangle(352, 500, 154, 35);
  iFilledRectangle(352, 440, 154, 35);
  iFilledRectangle(352, 440 - 60, 154, 35);
  iFilledRectangle(352, 440 - 120, 154, 35);
  iSetColor(252, 252, 3);
  iTextAdvanced(347 + 30, 508, "Resume", 0.2, 2.0);
  iTextAdvanced(347 + 37, 448, "Restart", 0.2, 2.0);
  if (sound1 || sound2)
    iTextAdvanced(347 + 10, 440 - 52, "Sound: ON", 0.2, 2.0);
  else if (!sound1 && !sound2)
    iTextAdvanced(347 + 10, 440 - 52, "Sound: OFF", 0.2, 2.0);
  iTextAdvanced(347 + 55, 440 - 120 + 8, "Exit", 0.2, 2.0);
  iShowLoadedImage(300, 578, &pause_text);
}

void iGameOver() {
  endgame = true;
  iDrawQueue();
  iSetTransparentColor(32, 56, 94, 0.95);
  iFilledRectangle(187, 200, 500, 500);
  iShowLoadedImage(300, 578, &qbert);
  iShowLoadedImage(287, 613, &dialogue);
  iShowLoadedImage(315, 400, &gameover);
  iSetColor(247, 233, 30);
  iTextBold(287, 440, "Your Score:");
  char score[50];
  snprintf(score, 50, "%d", player.score);
  iTextBold(380, 440, score);
}

void iSetting() {
  app_state = STATE_SETTING;
  iSetColor(32, 56, 94);
  iFilledRectangle(0, 0, 800, 800);
  iSetColor(255, 255, 51);
  iFilledRectangle(225, 600, 450, 100);
  if (selected_yes) {
    iSetColor(185, 176, 46);
    iFilledRectangle(300, 500, 100, 45);
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(300, 500, 100, 45);
  }
  if (selected_no) {
    iSetColor(185, 176, 46);
    iFilledRectangle(300, 400, 100, 45);
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(300, 400, 100, 45);
  }
  iSetColor(255, 255, 51);
  iFilledRectangle(300, 100, 250, 45);
  iFilledRectangle(100, 300, 110, 45);
  if (sound1) {
    iSetColor(185, 176, 46);
    iFilledRectangle(300, 300, 150, 45);
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(300, 300, 150, 45);
  }
  if (sound2) {
    iSetColor(185, 176, 46);
    iFilledRectangle(500, 300, 150, 45);
  } else {
    iSetColor(255, 255, 51);
    iFilledRectangle(500, 300, 150, 45);
  }

  iSetColor(255, 51, 51);
  iTextAdvanced(260, 630, "Show Co-Ordinates?", 0.3, 2);

  iTextAdvanced(325, 515, "YES", 0.2, 1);
  iTextAdvanced(325, 415, "NO", 0.2, 1);
  iTextAdvanced(107, 310, "SOUND:", 0.2, 1);
  iTextAdvanced(309, 310, "SOUND 1", 0.2, 1);
  iTextAdvanced(509, 310, "SOUND 2", 0.2, 1);
  iTextAdvanced(320, 110, "BACK TO MENU", 0.2, 2);
}
void iHelp() {
  app_state = STATE_HELP;
  iSetColor(32, 56, 94);
  iFilledRectangle(0, 0, 800, 800);
  iShowLoadedImage(40, 100, &help);
  iSetColor(255, 255, 51);
  iFilledCircle(21, 700, 15);
  iFilledCircle(100, 475, 15);
  iFilledCircle(200, 220, 15);
  iFilledRectangle(350, 50, 100, 45);
  iSetColor(255, 51, 51);
  iTextAdvanced(375, 60, "EXIT", 0.2, 2);
  iFilledCircle(21, 700, 8);
  iFilledCircle(100, 475, 8);
  iFilledCircle(200, 220, 8);
}
void iHighscore();
void iCredits();

void iBlock() {
  for (int i = 0; i < n; i++) {
    int x = blocksPos3d[i][0], y = blocksPos3d[i][1], z = blocksPos3d[i][2];
    tiles[y][x][z].valid = true;
    tiles[y][x][z].state = 0;
  }
  int i, j = 0;
  for (int y = MAX_SIZE - 1; ~y; --y) {
    for (int x = MAX_SIZE - 1; ~x; --x) {
      for (int z = MAX_SIZE - 1; ~z; --z) {
        if (!tiles[y][x][z].valid)
          continue;
        if (y > 0) {
          for (i = y - 1; ~i; --i)
            if (tiles[i][x][z].valid)
              break;
          if (~i)
            continue;
        }
        visible[j][0] = 1. * x, visible[j][1] = 1. * y, visible[j++][2] = 1. * z;
      }
    }
  }
  visible_count = j;
}

void iPlayer() {
  int idx = rand() % visible_count;
  player.km.pos.x = visible[idx][0];
  player.km.pos.y = visible[idx][1];
  player.km.pos.z = visible[idx][2];
  player.km.jump.active = 0;
  player.lives = 3;
  player.score = 0;
  player.max_lives = 3;
  player.ko = true;
  if (pause) {

    player.km.pos.x = 0;
    player.km.pos.y = 0;
    player.km.pos.z = 0;
  }
}

void iLoseLife(player_t *player) {
  if (player->lives > 0)
    player->lives--;
  if (player->lives == 0) {
    iPauseTimer(enemy_step_timer);
    iGameOver();
  } else {
    int ind = rand() % n;
    player->km.pos.x = blocksPos3d[ind][0];
    player->km.pos.y = blocksPos3d[ind][1];
    player->km.pos.z = blocksPos3d[ind][2];
  }
}

int iBodyMove(position_t pos, body_t *km) {
  int x = pos.x, y = pos.y, z = pos.z;
  if (!(x >= 0 && x < MAX_SIZE && y >= 0 && y < MAX_SIZE && z >= 0 && z < MAX_SIZE)) {
    // die and reset
    // printf("so, %d %d %d is invalid\n",x,y,z);
    return 0;
  }
  if ((y - 1 >= 0 && tiles[y - 1][x][z].valid) && 1) {
    // printf("so, up?\n");
    if (y - 2 >= -1 && !tiles[y - 2][x][z].valid) {
      // printf("so, up one block actually?\n");
      km->pos.x = x, km->pos.y = y - 1, km->pos.z = z;
      // printf("%lf %lf %lf\n",km->pos.x,km->pos.y,km->pos.z);
    }
    // then go, otherwise stay where you are
  } else if (tiles[y][x][z].valid) {
    // simply walk to this one, with no jump anim
    // printf("so walk straight?\n");
    km->pos.x = x, km->pos.y = y, km->pos.z = z;
  } else if (y + 1 <= MAX_SIZE && tiles[y + 1][x][z].valid) {
    // then move to this one, still a jump anim
    // printf("so down one block?\n");
    km->pos.x = x, km->pos.y = y + 1, km->pos.z = z;
  } else {
    // dont move
    // printf("huh? no move? thats boring...\n");
    return 0;
  }
  // now initiate movement and jump animation
  // printf("time to go... yay!!!\n");
  // printf("%lf %lf %lf\n",km->pos.x,km->pos.y,km->pos.z);
  return 1;
}

position_t iPositionFinder(position_t dir, position_t pos) {
  int x = pos.x + dir.x;
  int y = pos.y + dir.y;
  int z = pos.z + dir.z;
  if (y - 1 >= 0 && tiles[y - 1][x][z].valid) {
    if (y - 2 == -1 || (y - 2 >= 0 && !tiles[y - 2][x][z].valid)) {
      return (position_t){.x = 1. * x, .y = 1. * y - 1, .z = 1. * z};
    }
  } else if (tiles[y][x][z].valid)
    return (position_t){.x = 1. * x, .y = 1. * y, .z = 1. * z};
  else if (y + 1 < MAX_SIZE && tiles[y + 1][x][z].valid)
    return (position_t){.x = 1. * x, .y = 1. * y + 1, .z = 1. * z};
  return (position_t){.x = -1, .y = -1, .z = -1};
}

position_t iGetNextStep(position_t s, position_t e) {
  int i, j;

  bool visited[MAX_SIZE][MAX_SIZE][MAX_SIZE] = {{{0}}};
  visited[(int)s.y][(int)s.x][(int)s.z] = 1;

  position_t prev[MAX_SIZE][MAX_SIZE][MAX_SIZE];
  for (int j = 0; j < MAX_SIZE; j++)
    for (int k = 0; k < MAX_SIZE; k++)
      for (int l = 0; l < MAX_SIZE; l++)
        prev[j][k][l] = {.x = -1, .y = -1, .z = -1};

  position_t queue[MAX_SIZE * MAX_SIZE * MAX_SIZE];
  for (int j = 0; j < MAX_SIZE * MAX_SIZE * MAX_SIZE; j++)
    queue[j] = {.x = -1, .y = -1, .z = -1};
  i = j = 0;
  queue[j++] = {.x = s.x, .y = s.y, .z = s.z};

  // printf("full tracer: \n");
  while (~(int)queue[i].x) {
    // printf("%d queue: %g, %g, %g\n", i, queue[i].x, queue[i].y, queue[i].z);
    for (int k = 0; k < 4; k++) {
      position_t n = iPositionFinder(dirs[k], queue[i]);
      if (~(int)n.x && !visited[(int)n.y][(int)n.x][(int)n.z]) {
        // printf("%d %d: %g %g %g\n", i, j + 1, n.x, n.y, n.z);
        queue[j++] = {.x = n.x, .y = n.y, .z = n.z};
        // printf("%d queue: %g, %g, %g\n", j - 1, queue[j - 1].x, queue[j - 1].y, queue[i].z);
        visited[(int)n.y][(int)n.x][(int)n.z] = 1;
        prev[(int)n.y][(int)n.x][(int)n.z] = {.x = queue[i].x, .y = queue[i].y, .z = queue[i].z};
      }
      // printf("\n");
    }
    i++;
  }
  position_t path[MAX_SIZE * MAX_SIZE * MAX_SIZE];
  i = 0;
  // printf("Path: \n");
  for (position_t at = {.x = e.x, .y = e.y, .z = e.z}; ~(int)at.x;
       at = {.x = prev[(int)at.y][(int)at.x][(int)at.z].x,
             .y = prev[(int)at.y][(int)at.x][(int)at.z].y,
             .z = prev[(int)at.y][(int)at.x][(int)at.z].z})
    /*printf("%d, %g %g %g\n", i, at.x, at.y, at.z), */ path[i++] = {
        .x = at.x, .y = at.y, .z = at.z};

  if (path[i - 1].x == s.x && path[i - 1].y == s.y && path[i - 1].z == s.z) {
    // printf("\n%g %g %g\n\n%g %g %g\n", path[i - 1].x, path[i - 1].y, path[i - 1].z, s.x, s.y,
    // s.z); printf("printing all %d\n", i);
    for (int j = 0; j < i; j++)
      // printf("%g %g %g\n", path[j].x, path[j].y, path[j].z);
      return (position_t){.x = path[i - 2].x, .y = path[i - 2].y, .z = path[i - 2].z};
  }
  return (position_t){.x = path[i - 1].x, .y = path[i - 1].y, .z = path[i - 1].z};
}

void iEnemyStep() {
  if (pause)
    return;
  for (int i = 0; i < NUM_ENEMIES; i++) {
    switch (enemies[i].type) {
    case ENEMY_COILY:
    case ENEMY_SAM:
    case ENEMY_UGG:
    case ENEMY_WRONGWAY:
    default: {
      // random enemy ai
      /*for (int j = 0; i < 4; j++)
        {
          int d = rand () % 4, s;
          switch (d)
            {
            case 0:
              s = iBodyMove (enemies[i].km.pos.x,
        enemies[i].km.pos.y, enemies[i].km.pos.z - 1,
        &enemies[i].km); break; case 1: s = iBodyMove
        (enemies[i].km.pos.x, enemies[i].km.pos.y,
                             enemies[i].km.pos.z + 1,
        &enemies[i].km); break; case 2: s = iBodyMove
        (enemies[i].km.pos.x - 1, enemies[i].km.pos.y,
        enemies[i].km.pos.z, &enemies[i].km); break; case
        3: s = iBodyMove (enemies[i].km.pos.x + 1,
                             enemies[i].km.pos.y,
        enemies[i].km.pos.z, &enemies[i].km); break;
            default:
              break;
            }
          if (s)
            break;
        }*/
      // pathfinding enemy animation
      position_t step = iGetNextStep(enemies[i].km.pos, player.km.pos);
      // printf("Moving to %g, %g, %g.\n", step.x, step.y, step.z);
      iBodyMove(step, &enemies[i].km);
      break;
    }
    }
    // printf("here we go: %lf %lf
    // %lf\n",enemies[i].km.pos.x,enemies[i].km.pos.y,enemies[i].km.pos.z);
  }
}

void iEnemy() {
  for (int i = 0; i < NUM_ENEMIES; i++) {
    enemies[i].type = ENEMY_COILY;
    int idx = rand() % visible_count;
    enemies[i].km.pos.x = visible[idx][0], enemies[i].km.pos.y = visible[idx][1],
    enemies[i].km.pos.z = visible[idx][2];
    if (pause) {
      enemies[i].km.pos.x = 3, enemies[i].km.pos.y = 7, enemies[i].km.pos.z = 4;
    }
  }
}

void iRestart() {
  app_state = STATE_GAME;
  iBlock();
  iPlayer();
  iEnemy();
  pause = false;
}

void iGame() {
  app_state = STATE_GAME;
  iBlock();
  iPlayer();
  iEnemy();
  if (!enemy_step_timer)
    enemy_step_timer = iSetTimer(1000, iEnemyStep);
  else
    iResumeTimer(enemy_step_timer);
  // now going to set a timer that will check if I have completed the level
  // drawqueue[i].pos.x=player.pos.x,drawqueue[i].pos.y=player.pos.y,drawqueue[i].pos.z=player.pos.z;
  // drawqueue[i].flags=0;
  // drawqueue[i].type=TYPE_PLAYER;
}

// /*
// function iGenTiles() is the function that generates tiles for
// the blocks
// */
// void iGenTiles()
// {
//     double cx=start_x-a*cos(PI/6),cy=start_y+(a*cos(PI/3)+a);
//     for(int i = 0; i< 7; i++) {
//         if (i&1) {
//             cx-=a*cos(PI/6),cy-=(a*cos(PI/3)+a);
//             tiles[i][0].x=cx,tiles[i][0].y=cy;
//             for(int j=0;j<=i;j++) {
//                 iDTile(tiles[i][j].x,tiles[i][j].y);
//                 cx+=(j==i?0:2*a*cos(PI/6));
//                 if (j<i)
//                 tiles[i][j+1].x=cx,tiles[i][j+1].y=cy;
//             }
//         } else {
//             cx+=a*cos(PI/6),cy-=(a*cos(PI/3)+a);
//             tiles[i][i].x=cx,tiles[i][i].y=cy;
//             for(int j=i;j>=0;j--) {
//                 iDTile(tiles[i][j].x,tiles[i][j].y);
//                 cx-=(j==0?0:2*a*cos(PI/6));
//                 if (j>0)
//                 tiles[i][j-1].x=cx,tiles[i][j-1].y=cy;
//             }
//         }
//     }
// }

// /*
// function iGenSides() is the function that generates the sides
// of the blocks
// */
// void iGenSides()
// {
//     double
//     current_x=start_x-a*cos(PI/6),current_y=start_y+a/2;
//     for(int i = 0; i < 7;i++) {
//         if (i&1) {
//             current_x-=a*cos(PI/6),current_y-=3*a/2;
//             for(int j = 0; j<=i;j++) {
//                 iDSide(current_x,current_y);
//                 current_x+=(j==i?0:2*a*cos(PI/6));
//             }
//         } else {
//             current_x+=a*cos(PI/6),current_y-=3*a/2;
//             for(int j = 0; j<=i;j++) {
//                 iDSide(current_x,current_y);
//                 current_x-=(j==i?0:2*a*cos(PI/6));
//             }
//         }
//     }
// }

/*
function iDraw() is called again and again by the system.
*/
void iDraw() {
  // place your drawing codes here
  iClear();

  if (app_state == STATE_MENU) {
    iMenu();
    iShowSprite(&snake);
    iShowSprite(&qbert_jump);
    iShowSprite(&qbert_inverse);
    iShowSprite(&ball);
  } else if (app_state == STATE_SETTING) {
    iSetting();
    iShowSprite(&qbert_spin);
  } else if (app_state == STATE_HELP) {
    iHelp();
  } else if (app_state == STATE_GAME) {
    if (!endgame) {
      iSetColor(255, 255, 255);
      char pos[50];
      snprintf(pos, 50, "%d, %d, %d", (int)player.km.pos.x, (int)player.km.pos.y,
               (int)player.km.pos.z);
      if (selected_yes) {
        iText(10, 30, pos, GLUT_BITMAP_TIMES_ROMAN_24);
      }
      iSetColor(245, 149, 66);
      iFilledRectangle(700, 715, 37, 37);
      iShowLoadedImage(703, 716, &pause_button);
      iSetColor(12, 47, 173);
      iTextBold(10, 750, "LIVES:");
      for (int i = 1; i <= player.lives; i++) {
        iShowLoadedImage(30 + i * 35, 740, &life);
      }
      iTextBold(10, 700, "SCORE:");
      char score[50];
      snprintf(score, 50, "%d", player.score);
      iTextBold(70, 700, score);
    }

    for (int i = 0; i < NUM_ENEMIES; i++) {
      if (collision(&player, &enemies[i])) {
        iLoseLife(&player);
        return;
      }
    }

    // iGame();
    iDrawQueue();
    if (pause) {
      iPauseMenu();
      return;
    }
  }

  else if (app_state == STATE_EDITOR) {
    iBlock();
    iDrawQueue();
    if (editor.grid)
      iGrid();
  }
}
/*
function iMouseMove() is called when the user moves the
mouse. (mx, my) is the position where the mouse pointer is.
*/
void iMouseMove(int mx, int my) {
  // place your codes here
  if (app_state == STATE_MENU) {
    if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 500 && my < 530) {
      hover_start = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 435 && my < 465) {
      hover_resume = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 370 && my < 400) {
      hover_setting = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 305 && my < 335) {
      hover_help = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 240 && my < 270) {
      hover_high = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 175 && my < 205) {
      hover_credits = true;
    } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 110 && my < 140) {
      hover_exit = true;
    } else {
      hover_start = false;
      hover_resume = false;
      hover_setting = false;
      hover_help = false;
      hover_high = false;
      hover_credits = false;
      hover_exit = false;
    }
  }
}

/*
function iMouseDrag() is called when the user presses and
drags the mouse. (mx, my) is the position where the mouse
pointer is.
*/
void iMouseDrag(int mx, int my) {
  // place your codes here
}

/*
function iMouse() is called when the user
presses/releases the mouse. (mx, my) is the position
where the mouse pointer is.
*/
void iMouse(int button, int state, int mx, int my) {
  // if (button == GLUT_LEFT_BUTTON && state ==
  // GLUT_DOWN)
  // {
  //     // pl
  // }
  // if (button == GLUT_RIGHT_BUTTON && state ==
  // GLUT_DOWN)
  // {
  //     // place your codes here
  // }
  if (app_state == STATE_MENU) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 500 && my < 530) {
        iGame();
      }
      /*  else if
        (mx>width/2-100&&mx<width/2+50&&my>435&&my<465)
        { iResume();
        }*/
      else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 370 && my < 400) {
        iSetting();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 305 && my < 335) {
        iHelp();
      } /*
      else if
      (mx>width/2-100&&mx<width/2+50&&my>240&&my<270)
      { iHighscore();
      }
      else if
      (mx>width/2-100&&mx<width/2+50&&my>175&&my<205)
      { iCredits();
      } */
      else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 110 && my < 140) {
        exit(0);
      }
    }
  } else if (app_state == STATE_EDITOR) {
  } else if (app_state == STATE_GAME) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      if (mx > 700 && mx < 737 && my > 715 && my < 715 + 37) {
        iPauseMenu();
      }
      if (pause && mx > 352 && mx < 352 + 145 && my > 500 && my < 500 + 35) {
        pause = false;
      } else if (pause && mx > 352 && mx < 352 + 145 && my > 440 && my < 440 + 35) {
        iRestart();
      } else if (pause && mx > 352 && mx < 352 + 145 && my > 440 - 120 && my < 440 - 120 + 35) {
        app_state = STATE_MENU;
        pause = false;
      } else if (pause && mx > 352 && mx < 352 + 154 && my > 440 - 60 && my < 440 - 60 + 35) {
        bool soundOn = (sound1 || sound2);

        if (soundOn) {
          sound1 = false;
          sound2 = false;
          iPauseSound(sound_1);
          iPauseSound(sound_2);
        } else {
          sound1 = true;
          sound2 = false;
          iResumeSound(sound_1);
          iPauseSound(sound_2);
        }
      }
    }
  } else if (app_state == STATE_SETTING) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      if (mx > 300 && mx < 400 && my > 500 && my < 545) {
        selected_yes = true;
        selected_no = false;
      } else if (mx > 300 && mx < 400 && my > 400 && my < 445) {
        selected_yes = false;
        selected_no = true;
      } else if (mx > 300 && mx < 550 && my > 100 && my < 145) {
        iMenu();
      } else if (mx > 300 && mx < 450 && my > 300 && my < 345) {
        sound1 = true;
        sound2 = false;
        iResumeSound(sound_1);
        iPauseSound(sound_2);
      } else if (mx > 500 && mx < 650 && my > 300 && my < 345) {
        sound2 = true;
        sound1 = false;
        iResumeSound(sound_2);
        iPauseSound(sound_1);
      }
    }
  } else if (app_state == STATE_HELP) {
    if (mx > 350 && mx < 450 && my > 50 && my < 95) {
      iMenu();
    }
  }
}

/*
function iMouseWheel() is called when the user scrolls
the mouse wheel. dir = 1 for up, -1 for down.
*/
void iMouseWheel(int dir, int mx, int my) {
  // place your code here
}

/*
function iKeyboard() is called whenever the user hits a
key in keyboard. key- holds the ASCII value of the key
pressed.
*/
void iKeyboard(unsigned char key) {
  if (app_state == STATE_EDITOR) {
    switch (key) {
    case 'w':
      editor.wireframe ^= 1;
      break;
    case 'g':
      editor.grid ^= 1;
      break;
    case 'q':
    case ESC:
      editor.wireframe = false;
      editor.grid = false;
      app_state = STATE_MENU;
      break;
    default:
      break;
    }
  } else if (app_state == STATE_GAME) {
    switch (key) {
    case 'q':
    case ESC:
      app_state = STATE_MENU;
      break;
    case 'r': {
      player.km.pos.x = 0;
      player.km.pos.y = 0;
      player.km.pos.z = 0;
      // printf("%lf %lf
      // %lf\n",player.km.pos.x,player.km.pos.y,player.km.pos.z);
      break;
    }
    default:
      break;
    }
  } else {
    switch (key) {
    default:
      break;
    }
  }
}

/*
function iSpecialKeyboard() is called whenver user hits
special keys likefunction keys, home, end, pg up, pg
down, arraows etc. you have to use appropriate
constants to detect them. A list is: GLUT_KEY_F1,
GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5,
GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9,
GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12,
GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT,
GLUT_KEY_DOWN, GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN,
GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT */
void iSpecialKeyboard(unsigned char key) {
  if (app_state == STATE_MENU) {
  } else if (app_state == STATE_EDITOR) {
  } else if (app_state == STATE_GAME) {
    int dir;
    switch (key) {
    case GLUT_KEY_END:
      break;
    case GLUT_KEY_LEFT:
      dir = 0;
      break;
    case GLUT_KEY_RIGHT:
      dir = 1;
      break;
    case GLUT_KEY_UP:
      dir = 2;
      break;
    case GLUT_KEY_DOWN:
      dir = 3;
      break;
    default:
      break;
    }
    if (key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT || key == GLUT_KEY_UP ||
        key == GLUT_KEY_DOWN) {
      // handle scores
      if (endgame)
        return;
      position_t target = iPositionFinder(dirs[dir], player.km.pos);
      if (target.x <= -1)
        return;
      player.km.la = (look_t)dir;
      iBodyMove(target, &player.km);
      if (tiles[(int)target.y][(int)target.x][(int)target.z].state < state_num - 1) {
        tiles[(int)target.y][(int)target.x][(int)target.z].state++;
        tiles[(int)target.y][(int)target.x][(int)target.z].state %= state_num;
        player.score += 25;
      }
      sound_3 = iPlaySound("assets/sounds/jump_sound.wav", false, 40);
    }
  }
}

int main(int argc, char *argv[]) {
  glutInit(&argc, argv);
  iSetTransparency(1);
  iLoadResource();
  printf("%d\n", sizeof(blocksPos3d) / sizeof(blocksPos3d[0]));
  iInitializeSound();
  sound_1 = iPlaySound("assets/sounds/undertale_1.wav", true, 80);
  sound_2 = iPlaySound("assets/sounds/undertale_2.wav", true, 80);
  sound_3 = iPlaySound("assets/sounds/jump_sound.wav", false, 40);
  iPauseSound(sound_3);
  iPauseSound(sound_2);
  iSetTimer(600, iAnim);
  iSetTimer(200, iAnimSetting);
  iInitialize(width, height, "Q*Bert");
  return 0;
}
