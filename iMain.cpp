#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "iGraphics.h"
#include "iSound.h"

Image bg, help, life, qbert, pause_button, pause_text, dialogue, gameover, qbert_up, qbert_down,
    qbert_right, highscoreimage, gamecomplete, nxtlvl, bck, credits, ball1, snake1, ugg1, sam1,
    level1obj, level2obj, level3obj;
FrameSet frames, frames_1, spin_frame, ball_frame, qbert_invert;
Sprite snake, qbert_jump, qbert_spin, ball, qbert_inverse;

Image *qbert_looker;

#define PI 3.14159265
#define MAX_SIZE 10
#define MAX_BLOCKS 128
#define MAX_ENEMIES 8
#define MAX_STATES 3
#define ESC 0x1b

#define tiles(x, y, z)                                                                             \
  world.tiles[((int)(y)) * MAX_SIZE * MAX_SIZE + ((int)(x)) * MAX_SIZE + ((int)(z))]
#define visited(x, y, z)                                                                           \
  visited[((int)(y)) * MAX_SIZE * MAX_SIZE + ((int)(x)) * MAX_SIZE + ((int)(z))]
#define prev(x, y, z) prev[((int)(y)) * MAX_SIZE * MAX_SIZE + ((int)(x)) * MAX_SIZE + ((int)(z))]

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

typedef enum { ENEMY_COILY, ENEMY_UGG, ENEMY_WRONGWAY, ENEMY_SAM, ENEMY_RED } enemytype_t;

typedef struct {
  double x, y, z;
} vec3_t;

typedef struct {
  double x, y;
} vec2_t;

typedef struct {
  vec3_t from;
  vec3_t to;
  float t, duration;
  bool active;
} jumper_t;

typedef struct {
  jumper_t jump;
  vec3_t pos;
  look_t la;
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
  vec3_t pos;
  uint8_t flags;
  void *ref;
} drawqueue_t;

typedef struct {
  double blocks[MAX_BLOCKS][3];
  int blocks_count;
  double visible[MAX_BLOCKS][3];
  int visible_count;
  int level_num, states_count, target_idx;
  color_t states[MAX_STATES];
  color_t l_color, r_color;
  enemy_t enemies[MAX_ENEMIES];
  uint8_t enemy_count;
  player_t player;
  tile_t tiles[MAX_SIZE * MAX_SIZE * MAX_SIZE];
} world_t;

world_t world;

typedef struct {
  char name[100];
  int score;
} HighScore;

app_t app_state = STATE_MENU;

editor_t editor = {.wireframe = false, .grid = false, .debug = false};

drawqueue_t drawqueue[MAX_SIZE * MAX_SIZE * MAX_SIZE + 1 + MAX_ENEMIES + 1];

HighScore highscores[10];

int enemy_step_timer, world_timer;

int dt = 16;

int width = 800, height = 800;
int sound_1 = -1, sound_2 = -1, sound_3 = -1;

int numHighScore = 0;
char playername[100] = "";
int inputpos = 0;

int cheat = 0;

double start_x = width / 2.0;
double start_y = height * 0.9;
double tile_width = 1;
double tile_height = 1;
double unit = 40;

bool sound1 = true, sound2 = false, sound3 = false;
bool selected_yes = true;
bool selected_no = false;
bool pause = false;
bool hover_start = false, hover_resume = false, hover_setting = false, hover_help = false,
     hover_high = false;
bool hover_credits = false, hover_exit = false, end_game = false;
bool entername = false, show_highscore = false;
bool level_completed = false, cheat_on = false, win_cond = false;

// double blocksPos3d[][3] = {{7, 7, 0},
//                            {6, 7, 1},
//                            {5, 7, 2},
//                            {4, 7, 3},
//                            {3, 7, 4},
//                            {2, 7, 5},
//                            {1, 7, 6},
//                            {0, 7, 7},
//                            {6, 6, 0},
//                            {5, 6, 1},
//                            {4, 6, 2},
//                            {3, 6, 3},
//                            {2, 6, 4},
//                            {1, 6, 5},
//                            {0, 6, 6},
//                            {5, 5, 0},
//                            {4, 5, 1},
//                            {3, 5, 2},
//                            {2, 5, 3},
//                            {1, 5, 4},
//                            {0, 5, 5},
//                            {4, 4, 0},
//                            {3, 4, 1},
//                            {2, 4, 2},
//                            {1, 4, 3},
//                            {0, 4, 4},
//                            {3, 3, 0},
//                            {2, 3, 1},
//                            {1, 3, 2},
//                            {0, 3, 3},
//                            {2, 2, 0},
//                            {1, 2, 1},
//                            {0, 2, 2},
//                            {1, 1, 0},
//                            {0, 1, 1},
//                            {0, 0, 0},
//                            {0, 0, 1},
//                            {1, 4, 4},
//                            // {2,3,5}
//                            // {5,4,3},
//                            {4, 5, 2},
//                            {3, 7, 5}};
// int n = sizeof(blocksPos3d) / sizeof(blocksPos3d[0]);

color_t coily = {.r = 10, .g = 100, .b = 240};
color_t ugg = {.r = 100, .g = 200, .b = 150};
color_t sam = {.r = 10, .g = 255, .b = 200};

// color_t states[3] = {
//     {.r = 86, .g = 70, .b = 239}, {.r = 222, .g = 222, .b = 0}, {.r = 20, .g = 200, .b = 239}};
// color_t l_color = {.r = 86, .g = 169, .b = 152}, r_color = {.r = 49, .g = 70, .b = 70};

vec3_t dirs[4] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};

// int states_count = 3;

int cmp_dk(const void *a, const void *b) {
  drawqueue_t *A = (drawqueue_t *)a;
  drawqueue_t *B = (drawqueue_t *)b;
  int d1 = A->pos.x + A->pos.z + (MAX_SIZE - 1 - A->pos.y) + A->type;
  int d2 = B->pos.x + B->pos.z + (MAX_SIZE - 1 - B->pos.y) + B->type;
  return (d1 > d2) - (d1 < d2);
}

vec2_t iProjection(vec3_t pos, vec2_t offset) {
  return {.x = start_x + (pos.z - pos.x) * unit * tile_width * cos(PI / 6) + offset.x,
          .y = start_y - (pos.z + pos.x) * unit * tile_width / 2 - pos.y * unit * tile_height +
               offset.y};
}

void iLoadResource() {
  iLoadImage(&bg, "assets/images/title.png");
  iLoadImage(&help, "assets/images/help.png");
  iLoadImage(&life, "assets/images/sprites/qbert/qbert06.png");
  iLoadImage(&qbert, "assets/images/sprites/qbert/qbert06.png");
  iLoadImage(&qbert_up, "assets/images/sprites/qbert/qbert02.png");
  iLoadImage(&qbert_down, "assets/images/sprites/qbert/qbert08.png");
  iLoadImage(&qbert_right, "assets/images/sprites/qbert/qbert04.png");
  iLoadImage(&pause_button, "assets/images/pausebutton.png");
  iLoadImage(&pause_text, "assets/images/paused.png");
  iLoadImage(&dialogue, "assets/images/dialogue.png");
  iLoadImage(&gameover, "assets/images/gameover.png");
  iLoadImage(&highscoreimage, "assets/images/highscoreimage.png");
  iLoadImage(&gamecomplete, "assets/images/youwin.png");
  iLoadImage(&credits, "assets/images/credits.jpeg");
  iLoadImage(&nxtlvl, "assets/images/nxtlvl.png");
  iLoadImage(&bck, "assets/images/bck.png");
  iLoadImage(&ball1, "assets/images/sprites/ball/ball002.png");
  iLoadImage(&snake1, "assets/images/sprites/snake/snake002.png");
  iLoadImage(&ugg1, "assets/images/ugg.png");
  iLoadImage(&sam1, "assets/images/sam.png");
  iLoadImage(&level1obj, "assets/images/level1obj.png");
  iLoadImage(&level2obj, "assets/images/level2obj.png");
  iLoadImage(&level3obj, "assets/images/level3obj.png");
  iResizeImage(&qbert, 35, 40);
  iResizeImage(&qbert_up, 35, 40);
  iResizeImage(&qbert_down, 35, 40);
  iResizeImage(&qbert_right, 35, 40);
  iScaleImage(&snake1, 1.8);
  iScaleImage(&ball1, 1.8);
  iScaleImage(&ugg1, 1.8);
  iScaleImage(&sam1, 1.8);
  iScaleImage(&level1obj, 1.5);
  iScaleImage(&level2obj, 1.5);
  iScaleImage(&level3obj, 1.5);
  iResizeImage(&help, 750, 700);
  iResizeImage(&life, 23, 23);
  iScaleImage(&pause_text, 1.6);
  iScaleImage(&dialogue, 1.5);
  iScaleImage(&gameover, 0.7);
  iScaleImage(&nxtlvl, 0.7);
  iScaleImage(&bck, 0.5);
  iInitSprite(&snake);
  iInitSprite(&qbert_jump);
  iInitSprite(&qbert_spin);
  iInitSprite(&ball);
  iInitSprite(&qbert_inverse);
  iLoadFramesFromFolder(&frames, "assets/images/sprites/snake");
  iLoadFramesFromFolder(&frames_1, "assets/images/sprites/qbert_jump");
  iLoadFramesFromFolder(&spin_frame, "assets/images/sprites/spin");
  iLoadFramesFromFolder(&ball_frame, "assets/images/sprites/ball");
  iLoadFramesFromFolder(&qbert_invert, "assets/images/sprites/qbert_invert");
  iChangeSpriteFrames(&snake, &frames);
  iChangeSpriteFrames(&qbert_jump, &frames_1);
  iChangeSpriteFrames(&qbert_spin, &spin_frame);
  iChangeSpriteFrames(&ball, &ball_frame);
  iChangeSpriteFrames(&qbert_inverse, &qbert_invert);
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

void iExit() {
  // free everything
  // close window
  iCloseWindow();
}

void iLoadHighscore() {
  FILE *file = fopen("./saves/highscores/names.txt", "r");
  if (file == NULL) {
    numHighScore = 0;
    return;
  }
  numHighScore = 0;
  while (numHighScore < 10 && fscanf(file, "%99s %d", highscores[numHighScore].name,
                                     &highscores[numHighScore].score) == 2) {
    numHighScore++;
  }
  fclose(file);
}

void iSaveHighscore() {
  FILE *file = fopen("./saves/highscores/names.txt", "w");
  if (file == NULL) {
    printf("Error!\nCould not save High Score\n");
    return;
  }
  for (int i = 0; i < numHighScore; i++) {
    fprintf(file, "%s %d\n", highscores[i].name, highscores[i].score);
  }
  fclose(file);
}

void iAddHighScore(const char *name, int score) {
  int insertPos = numHighScore;
  for (int i = 0; i < numHighScore; i++) {
    if (score > highscores[i].score) {
      insertPos = i;
      break;
    }
  }

  if (insertPos < 10) {
    for (int i = (numHighScore < 10 ? numHighScore : 10 - 1); i > insertPos; i--) {
      strcpy(highscores[i].name, highscores[i - 1].name);
      highscores[i].score = highscores[i - 1].score;
    }
    strcpy(highscores[insertPos].name, name);
    highscores[insertPos].score = score;

    if (numHighScore < 10) {
      numHighScore++;
    }
    iSaveHighscore();
  }
}

bool iCheckHighScore(int score) {
  if (numHighScore < 10) {
    return true;
  }
  return score > highscores[9].score;
}

void iLoadPlayer(bool initialized) {
  world.player.km.jump.active = 0;
  world.player.km.jump.duration = 250;
  world.player.km.jump.t = 0;
  world.player.km.la = LOOK_LEFT;
  world.player.max_lives = 3;
  world.player.lives = 3;
  if (initialized) {
    printf("Setting score to zero.\n"), world.player.score = 0;
  }
  world.player.ko = true;
}

void iLoadEnemies() {
  for (int i = 0; i < world.enemy_count; i++) {
    int idx = rand() % world.visible_count;
    world.enemies[i].km.pos.x = world.visible[idx][0],
    world.enemies[i].km.pos.y = world.visible[idx][1],
    world.enemies[i].km.pos.z = world.visible[idx][2];
    world.enemies[i].km.jump.duration = 100;
    world.enemies[i].km.jump.t = 0;
  }
}

void iLoadBlocks() {
  for (int i = 0; i < world.blocks_count; i++) {
    int x = world.blocks[i][0], y = world.blocks[i][1], z = world.blocks[i][2];
    tiles(x, y, z).valid = true;
    tiles(x, y, z).state = 0;
  }
  int i, j = 0;
  for (int y = MAX_SIZE - 1; ~y; --y) {
    for (int x = MAX_SIZE - 1; ~x; --x) {
      for (int z = MAX_SIZE - 1; ~z; --z) {
        if (!tiles(x, y, z).valid)
          continue;
        if (y > 0) {
          for (i = y - 1; ~i; --i)
            if (tiles(x, i, z).valid)
              break;
          if (~i)
            continue;
        }
        world.visible[j][0] = 1. * x, world.visible[j][1] = 1. * y, world.visible[j++][2] = 1. * z;
      }
    }
  }
  world.visible_count = j;
}

void iLoadLevel(int level) {
  world_t gameLevel = {0};

  gameLevel.player.score = world.player.score;

  FILE *fp;

  int mode = -1;

  char line[128];
  char filePath[50];

  snprintf(filePath, 50, "./saves/levels/level%d.txt", level);
  fp = fopen(filePath, "r");
  if (fp == NULL) {
    printf("Could not read the %dth level file at %s.\n", level, filePath);
    return;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, "LEVEL", 5)) {
      sscanf(line, "LEVEL %d", &gameLevel.level_num);
    } else if (!strncmp(line, "START", 5)) {
      int x, y, z;
      sscanf(line, "START %d %d %d", &x, &y, &z);
      gameLevel.player.km.pos = {.x = 1. * x, .y = 1. * y, .z = 1. * z};
    } else if (!strncmp(line, "TARGET", 6)) {
      sscanf(line, "TARGET %d", &gameLevel.target_idx);
    } else if (!strncmp(line, "WORLD", 5))
      mode = 0;
    else if (!strncmp(line, "ENEMY", 5))
      mode = 1;
    else if (!strncmp(line, "STATES", 6))
      mode = 2;
    else if (!strncmp(line, "SIDES", 5))
      mode = 3;
    else if (!strncmp(line, "//", 2))
      continue;
    else {
      switch (mode) {
      case 0: {
        int x, y, z;
        if (sscanf(line, "%d %d %d", &x, &y, &z) == 3) {
          if (gameLevel.blocks_count < MAX_BLOCKS) {
            gameLevel.blocks[gameLevel.blocks_count][0] = 1. * x,
            gameLevel.blocks[gameLevel.blocks_count][1] = 1. * y,
            gameLevel.blocks[gameLevel.blocks_count][2] = 1. * z;
            gameLevel.blocks_count++;
          }
        }
        break;
      }
      case 1: {
        int enemy_idx;
        int x, y, z;
        if (sscanf(line, "%d %f %f %f", &enemy_idx, &x, &y, &z) == 4) {
          if (gameLevel.enemy_count < MAX_ENEMIES) {
            gameLevel.enemies[gameLevel.enemy_count++] = {
                .km = {.jump = {0},
                       .pos = {.x = 1. * x, .y = 1. * y, .z = 1. * z},
                       .la = (look_t)(rand() % 4)},
                .type = (enemytype_t)enemy_idx};
          }
        }
        break;
      }
      case 2: {
        int r, g, b;
        if (sscanf(line, "%d %d %d", &r, &g, &b) == 3) {
          if (gameLevel.states_count < MAX_STATES) {
            gameLevel.states[gameLevel.states_count++] = {
                .r = (uint8_t)r, .g = (uint8_t)g, .b = (uint8_t)b};
          }
        }
        break;
      }
      case 3: {
        sscanf(line, "%d %d %d %d %d %d %d %d", &gameLevel.l_color.r, &gameLevel.l_color.g,
               &gameLevel.l_color.b, &gameLevel.l_color.a, &gameLevel.r_color.r,
               &gameLevel.r_color.g, &gameLevel.r_color.b, &gameLevel.r_color.a);
        break;
      }
      default:
        break;
      }
    }
  }
  fclose(fp);
  // copy everything to global world
  world = gameLevel;
  printf("%g %g %g\n", world.player.km.pos.x, world.player.km.pos.y, world.player.km.pos.z);

  iLoadBlocks();
  iLoadEnemies();
  iLoadPlayer(false);
}

void iPrintWorld(world_t *world) {
  printf("%-20s: %d\n", "level_num", world->level_num);
  printf("%-20s: %d\n", "blocks_count", world->blocks_count);
  printf("%-20s: %d\n", "enemy_count", world->enemy_count);
  printf("%-20s: %d\n", "states_count", world->states_count);
  printf("%-20s: %d\n", "target_idx", world->target_idx);
  printf("%-20s: {%d, %d, %d, %d}\n", "l_color", world->l_color.r, world->l_color.g,
         world->l_color.b, world->l_color.a);
  printf("%-20s: {%d, %d, %d, %d}\n", "r_color", world->r_color.r, world->r_color.g,
         world->r_color.b, world->r_color.a);
}

void iCompleteLevel() {
  iSetTransparentColor(32, 56, 94, 0.95);
  iFilledRectangle(187, 200, 500, 500);
  iShowLoadedImage(280, 518, &gamecomplete);
  iShowLoadedImage(350, 400, &nxtlvl);
  iShowLoadedImage(340, 320, &bck);
  // load levelup/complete game screen and then load next level
  // Here show next level (will set the level_completed to false afterwards)
  // or show complete game (will set endgame to true and level_completed to false)
}

void iClearQueue() {
  for (int i = 0; i < MAX_SIZE * MAX_SIZE * MAX_SIZE + MAX_ENEMIES + 1; i++)
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
  double x_coords[] = {x, x + unit * tile_width * cos(PI / 6), x,
                       x - unit * tile_width * cos(PI / 6)};
  double y_coords[] = {y, y - unit * tile_width / 2, y - unit * tile_width,
                       y - unit * tile_width / 2};
  iFilledPolygon(x_coords, y_coords, 4);
}

void iTileOutline(double x, double y) {
  double x_coords[] = {x, x + unit * tile_width * cos(PI / 6), x,
                       x - unit * tile_width * cos(PI / 6)};
  double y_coords[] = {y, y - unit * tile_width / 2, y - unit * tile_width,
                       y - unit * tile_width / 2};
  iPolygon(x_coords, y_coords, 4);
}

void iSide(double x, double y) {
  double x_coords[] = {x, x, x + unit * tile_width * cos(PI / 6),
                       x + unit * tile_width * cos(PI / 6)};
  double y_coords[] = {y - unit * tile_width, y - unit * tile_width - unit * tile_height,
                       y - unit * tile_width / 2 - unit * tile_height, y - unit * tile_width / 2};
  iSetColor(world.r_color.r, world.r_color.g, world.r_color.b);
  iFilledPolygon(x_coords, y_coords, 4);
  x_coords[2] = x - unit * tile_width * cos(PI / 6),
  x_coords[3] = x - unit * tile_width * cos(PI / 6);
  iSetColor(world.l_color.r, world.l_color.g, world.l_color.b);
  iFilledPolygon(x_coords, y_coords, 4);
}

void iSideOutline(double x, double y) {
  double x_coords[] = {x, x, x + unit * tile_width * cos(PI / 6),
                       x + unit * tile_width * cos(PI / 6)};
  double y_coords[] = {y - unit * tile_width, y - unit * tile_width - unit * tile_height,
                       y - unit * tile_width / 2 - unit * tile_height, y - unit * tile_width / 2};
  iPolygon(x_coords, y_coords, 4);
  x_coords[2] = x - unit * tile_width * cos(PI / 6),
  x_coords[3] = x - unit * tile_width * cos(PI / 6);
  iPolygon(x_coords, y_coords, 4);
}

void iDrawEnemy(enemy_t *enemy) {

  // do your stuff here
  // instead of setting colors here, just declare a pointer to a enemy image/ sprite and then
  // finally draw it.
  Image *enemy_sprite;
  switch (enemy->type) {
  case ENEMY_COILY:
    enemy_sprite = &snake1;
    break;
  case ENEMY_UGG:
    enemy_sprite = &ugg1;
    break;
  case ENEMY_SAM:
    enemy_sprite = &sam1;
    break;
  default:
    enemy_sprite = &ball1;
    break;
  }
  vec2_t screenPos =
      iProjection(enemy->km.pos, {.x = -unit * tile_width / 4, .y = -unit * tile_width / 2});
  iShowLoadedImage(screenPos.x, screenPos.y, enemy_sprite);
}

bool iPECollision(enemy_t *enemy) {
  if (cheat_on)
    return false;
  if ((world.player.km.pos.x == enemy->km.pos.x) && (world.player.km.pos.y == enemy->km.pos.y) &&
      (world.player.km.pos.z == enemy->km.pos.z))
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
        if (!tiles(x, y, z).valid)
          continue;
        drawqueue[i].pos.x = x, drawqueue[i].pos.y = y, drawqueue[i].pos.z = z;
        drawqueue[i].flags = tiles(x, y, z).state;
        drawqueue[i].ref = &tiles(x, y, z);
        drawqueue[i++].type = TYPE_BLOCK;
      }
    }
  }
  if (app_state == STATE_GAME) {
    drawqueue[i].pos.x = world.player.km.pos.x, drawqueue[i].pos.y = world.player.km.pos.y,
    drawqueue[i].pos.z = world.player.km.pos.z;
    drawqueue[i].flags = world.player.km.jump.active;
    drawqueue[i].ref = &world.player;
    drawqueue[i++].type = TYPE_PLAYER;
    for (j = 0; j < world.enemy_count; j++) {
      drawqueue[i].pos.x = world.enemies[j].km.pos.x,
      drawqueue[i].pos.y = world.enemies[j].km.pos.y,
      drawqueue[i].pos.z = world.enemies[j].km.pos.z;
      drawqueue[i].flags = world.enemies[j].type;
      drawqueue[i].ref = &world.enemies[j];
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
      vec2_t screenPos = iProjection({.x = x, .y = y, .z = z}, {0});
      if (!editor.wireframe) {
        iSetColor(world.states[tiles(x, y, z).state % world.states_count].r,
                  world.states[tiles(x, y, z).state % world.states_count].g,
                  world.states[tiles(x, y, z).state % world.states_count].b);
        iTile(screenPos.x, screenPos.y);
        iSide(screenPos.x, screenPos.y);
      } else {
        iSetColor(world.r_color.r, world.r_color.g, world.r_color.b);
        iTileOutline(screenPos.x, screenPos.y);
        iSideOutline(screenPos.x, screenPos.y);
      }
      break;
    }
    case TYPE_PLAYER: {
      iSetTransparentColor(0, 0, 0, 0.5);
      if (world.player.km.la == 1) {
        qbert_looker = &qbert;
      } else if (world.player.km.la == 0) {
        qbert_looker = &qbert_right;
      } else if (world.player.km.la == 2) {
        qbert_looker = &qbert_up;
      } else if (world.player.km.la == 3) {
        qbert_looker = &qbert_down;
      }
      vec2_t screenPos = iProjection(world.player.km.pos,
                                     {.x = -unit * tile_width / 4, .y = -unit * tile_width / 2});
      iShowLoadedImage(screenPos.x, screenPos.y, qbert_looker);
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
             ((int)((width - start_x) / sqrt(3) + height - start_y)) * unit * tile_height;
  iSetTransparentColor(255, 255, 255, 0.25);
  for (; c >= 0; c -= unit * tile_width)
    iLine(0, c, width, -width / sqrt(3) + c);
  c = start_y - start_x / sqrt(3) +
      ((int)(height - start_y + start_x / sqrt(3))) * unit * tile_height;
  for (; width / sqrt(3) + c >= 0; c -= unit * tile_width)
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

void iQuitGame() {
  iPauseTimer(enemy_step_timer);
  iPauseTimer(world_timer);
}

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
  end_game = true;
  iSetTransparentColor(32, 56, 94, 0.95);
  iFilledRectangle(187, 200, 500, 500);
  iShowLoadedImage(300, 578, &qbert);
  iShowLoadedImage(287, 613, &dialogue);
  iShowLoadedImage(315, 400, &gameover);
  iSetColor(247, 233, 30);
  iTextBold(287, 440, "Your Score:");
  char score[50];
  snprintf(score, 50, "%d", world.player.score);
  iTextBold(380, 440, score);
  iSetColor(255, 255, 0);
  if (iCheckHighScore(world.player.score)) {
    iSetColor(247, 233, 30);
    iTextBold(250, 380, "NEW HIGH SCORE!");
  }
  iTextBold(250, 350, "Enter your name:");
  iSetColor(255, 255, 255);
  iRectangle(250, 320, 200, 25);
  iSetColor(0, 0, 0);
  iFilledRectangle(251, 321, 198, 23);
  iSetColor(255, 255, 0);
  iText(255, 325, playername, GLUT_BITMAP_HELVETICA_18);
  iTextBold(360, 290, "Press ENTER to save");
}
void iLastScreen() {
  win_cond = true;
  iSetTransparentColor(32, 56, 94, 0.95);
  iFilledRectangle(187, 200, 500, 500);
  iShowLoadedImage(300, 578, &qbert);
  iShowLoadedImage(287, 613, &dialogue);
  iShowLoadedImage(290, 450, &gamecomplete);
  iSetColor(247, 233, 30);
  iTextBold(287, 440, "Your Score:");
  char score[50];
  snprintf(score, 50, "%d", world.player.score);
  iTextBold(380, 440, score);
  iSetColor(255, 255, 0);
  if (iCheckHighScore(world.player.score)) {
    iSetColor(247, 233, 30);
    iTextBold(250, 380, "NEW HIGH SCORE!");
  }
  iTextBold(250, 350, "Enter your name:");
  iSetColor(255, 255, 255);
  iRectangle(250, 320, 200, 25);
  iSetColor(0, 0, 0);
  iFilledRectangle(251, 321, 198, 23);
  iSetColor(255, 255, 0);
  iText(255, 325, playername, GLUT_BITMAP_HELVETICA_18);
  iTextBold(360, 290, "Press ENTER to save");
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
void iHighscore() {
  app_state = STATE_HIGHSCORE;
  iSetColor(32, 56, 94);
  iFilledRectangle(0, 0, 800, 800);
  iShowLoadedImage(240, 507, &highscoreimage);
  iSetColor(255, 255, 51);
  iFilledRectangle(100, 560, 70, 28);
  iFilledRectangle(320, 560, 70, 28);
  iFilledRectangle(600, 560, 70, 28);
  iSetColor(255, 51, 51);
  iTextBold(115, 558 + 11, "Rank");
  iTextBold(335, 558 + 11, "Name");
  iTextBold(615, 558 + 11, "Score");
  for (int i = numHighScore - 1; i >= 0; i--) {
    iSetColor(255, 251, 51);
    char rank[10];
    char name[200];
    char score[50];
    snprintf(rank, sizeof(rank), "%d.", i + 1);
    snprintf(name, sizeof(name), "%s", highscores[i].name);
    snprintf(score, sizeof(score), "%d", highscores[i].score);
    iTextAdvanced(115, 530 - i * 50, rank, 0.2, 1.0);
    iTextAdvanced(320, 530 - i * 50, name, 0.2, 1.0);
    iTextAdvanced(605, 530 - i * 50, score, 0.2, 1.0);
  }
  iSetColor(255, 255, 51);
  iFilledRectangle(370, 40, 70, 28);
  iSetColor(255, 51, 51);
  iTextBold(385, 50, "Back");
}
void iCredits() {
  app_state = STATE_CREDITS;
  iSetColor(32, 56, 94);
  iFilledRectangle(0, 0, 800, 800);
  iShowLoadedImage(0, 50, &credits);
  iSetColor(255, 255, 51);
  iFilledRectangle(370, 40, 70, 28);
  iSetColor(255, 51, 51);
  iTextBold(385, 50, "Back");
}

void iLoseLife() {
  if (world.player.lives > 0)
    world.player.lives--;
  if (world.player.lives == 0) {
    end_game = true;
  } else {
    int ind = rand() % world.visible_count;
    world.player.km.pos.x = world.visible[ind][0];
    world.player.km.pos.y = world.visible[ind][1];
    world.player.km.pos.z = world.visible[ind][2];
  }
}

int iBodyMove(vec3_t pos, body_t *km) {
  int x = pos.x, y = pos.y, z = pos.z;
  if (!(x >= 0 && x < MAX_SIZE && y >= 0 && y < MAX_SIZE && z >= 0 && z < MAX_SIZE)) {
    // die and reset
    // printf("so, %d %d %d is invalid\n",x,y,z);
    return 0;
  }
  if ((y - 1 >= 0 && tiles(x, y - 1, z).valid) && 1) {
    // printf("so, up?\n");
    // we are using y-2 check here only for the uppest block since if we are at y = 1, we must be
    // able to move up regardless of y-2's appeareance since y-2 cant be present in this case
    if (y - 2 >= -1 && !tiles(x, y - 2, z).valid) {
      // printf("so, up one block actually?\n");
      x = x, z = z, y = y - 1;
      // printf("%lf %lf %lf\n",km->pos.x,km->pos.y,km->pos.z);
    }
    // then go, otherwise stay where you are
  } else if (tiles(x, y, z).valid) {
    // simply walk to this one, with no jump anim
    // printf("so walk straight?\n");
    x = x, y = y, z = z;
  } else if (y + 1 <= MAX_SIZE && tiles(x, y + 1, z).valid) {
    // then move to this one, still a jump anim
    // printf("so down one block?\n");
    x = x, z = z, y = y + 1;
  } else {
    // dont move
    // printf("huh? no move? thats boring...\n");
    return 0;
  }
  // now initiate movement and jump animation
  km->jump.active = true;
  km->jump.from.x = km->pos.x, km->jump.from.y = km->pos.y, km->jump.from.z = km->pos.z;
  km->jump.to.x = x, km->jump.to.y = y, km->jump.to.z = z;
  km->jump.t = 0;
  // printf("time to go... yay!!!\n");
  // printf("%lf %lf %lf\n",km->pos.x,km->pos.y,km->pos.z);
  return 1;
}

vec3_t iPositionFinder(vec3_t dir, vec3_t pos) {
  int x = pos.x + dir.x;
  int y = pos.y + dir.y;
  int z = pos.z + dir.z;
  if (y - 1 >= 0 && tiles(x, y - 1, z).valid) {
    if (y - 2 == -1 || (y - 2 >= 0 && !tiles(x, y - 2, z).valid)) {
      return (vec3_t){.x = 1. * x, .y = 1. * y - 1, .z = 1. * z};
    }
  } else if (tiles(x, y, z).valid)
    return (vec3_t){.x = 1. * x, .y = 1. * y, .z = 1. * z};
  else if (y + 1 < MAX_SIZE && tiles(x, y + 1, z).valid)
    return (vec3_t){.x = 1. * x, .y = 1. * y + 1, .z = 1. * z};
  return (vec3_t){.x = -1, .y = -1, .z = -1};
}

vec3_t iGetNextStep(vec3_t s, vec3_t e) {
  int i, j;

  bool visited[MAX_SIZE * MAX_SIZE * MAX_SIZE] = {0};
  visited(s.x, s.y, s.z) = 1;

  vec3_t prev[MAX_SIZE * MAX_SIZE * MAX_SIZE];
  for (int j = 0; j < MAX_SIZE; j++)
    for (int k = 0; k < MAX_SIZE; k++)
      for (int l = 0; l < MAX_SIZE; l++)
        prev(k, j, l) = {.x = -1, .y = -1, .z = -1};

  vec3_t queue[MAX_SIZE * MAX_SIZE * MAX_SIZE];
  for (int j = 0; j < MAX_SIZE * MAX_SIZE * MAX_SIZE; j++)
    queue[j] = {.x = -1, .y = -1, .z = -1};
  i = j = 0;
  queue[j++] = {.x = s.x, .y = s.y, .z = s.z};

  // printf("full tracer: \n");
  while (~(int)queue[i].x) {
    // printf("%d queue: %g, %g, %g\n", i, queue[i].x, queue[i].y, queue[i].z);
    for (int k = 0; k < 4; k++) {
      vec3_t n = iPositionFinder(dirs[k], queue[i]);
      if (~(int)n.x && !visited(n.x, n.y, n.z)) {
        // printf("%d %d: %g %g %g\n", i, j + 1, n.x, n.y, n.z);
        queue[j++] = {.x = n.x, .y = n.y, .z = n.z};
        // printf("%d queue: %g, %g, %g\n", j - 1, queue[j - 1].x, queue[j - 1].y, queue[i].z);
        visited(n.x, n.y, n.z) = 1;
        prev(n.x, n.y, n.z) = {.x = queue[i].x, .y = queue[i].y, .z = queue[i].z};
      }
      // printf("\n");
    }
    i++;
  }
  vec3_t path[MAX_SIZE * MAX_SIZE * MAX_SIZE];
  i = 0;
  // printf("Path: \n");
  for (vec3_t at = {.x = e.x, .y = e.y, .z = e.z}; ~(int)at.x; at = {.x = prev(at.x, at.y, at.z).x,
                                                                     .y = prev(at.x, at.y, at.z).y,
                                                                     .z = prev(at.x, at.y, at.z).z})
    /*printf("%d, %g %g %g\n", i, at.x, at.y, at.z), */ path[i++] = {
        .x = at.x, .y = at.y, .z = at.z};

  if (path[i - 1].x == s.x && path[i - 1].y == s.y && path[i - 1].z == s.z) {
    // printf("\n%g %g %g\n\n%g %g %g\n", path[i - 1].x, path[i - 1].y, path[i - 1].z, s.x, s.y,
    // s.z); printf("printing all %d\n", i);
    for (int j = 0; j < i; j++)
      // printf("%g %g %g\n", path[j].x, path[j].y, path[j].z);
      return (vec3_t){.x = path[i - 2].x, .y = path[i - 2].y, .z = path[i - 2].z};
  }
  return (vec3_t){.x = path[i - 1].x, .y = path[i - 1].y, .z = path[i - 1].z};
}

void iEnemyStep() {
  if (pause)
    return;
  if (level_completed)
    return;
  if (win_cond)
    return;
  for (int i = 0; i < world.enemy_count; i++) {
    vec3_t pos;
    switch (world.enemies[i].type) {
    case ENEMY_COILY: {
      pos = iGetNextStep(
          world.enemies[i].km.jump.active ? world.enemies[i].km.jump.to : world.enemies[i].km.pos,
          world.player.km.jump.active ? world.player.km.jump.to : world.player.km.pos);
      break;
    }
    case ENEMY_UGG: {
      int sequence[4] = {3, 1, 2, 0};
      switch (world.enemies[i].km.la) {
      case LOOK_LEFT:
        // keep it as it is
        break;
      case LOOK_RIGHT:
        sequence[1] = 2, sequence[2] = 0, sequence[3] = 1;
        break;
      case LOOK_UP:
        sequence[0] = 1, sequence[1] = 2, sequence[2] = 0, sequence[3] = 3;
        break;
      case LOOK_DOWN:
        sequence[2] = 0, sequence[3] = 2;
        break;
      default:
        break;
      }
      // printf("sequence: %d %d %d %d\n", sequence[0], sequence[1], sequence[2], sequence[3]);
      for (int j = 0; j < 4; j++) {
        pos = iPositionFinder(dirs[sequence[j]], world.enemies[i].km.pos);
        // printf("pos: %g %g %g\n", pos.x, pos.y, pos.z);
        if (~(int)(pos.x))
          break;
      }
      break;
    }
    case ENEMY_SAM:
    // {
    //   int sequence[4] = {2, 1, 3, 0};
    //   switch (world.enemies[i].km.la) {
    //   case LOOK_LEFT:
    //     sequence[1] = 3, sequence[2] = 0, sequence[3] = 1;
    //     break;
    //   case LOOK_RIGHT:
    //     // keep it as it is
    //     break;
    //   case LOOK_UP:
    //     sequence[0] = 1, sequence[1] = 3, sequence[2] = 0, sequence[3] = 2;
    //     break;
    //   case LOOK_DOWN:
    //     sequence[2] = 0, sequence[3] = 3;
    //     break;
    //   default:
    //     break;
    //   }
    //   for (int j = 0; j < 4; j++) {
    //     pos = iPositionFinder(dirs[sequence[j]], world.enemies[i].km.pos);
    //     if (~(int)(pos.x))
    //       break;
    //   }
    //   break;
    // }
    case ENEMY_WRONGWAY:
    default: {
      // random enemy ai
      while (1) {
        int idx = rand() % 4;
        pos = iPositionFinder(dirs[idx], world.enemies[i].km.pos);
        if (~(int)(pos.x))
          break;
      }
      break;
    }
    }
    int dZ = pos.z - world.enemies[i].km.pos.z, dX = pos.x - world.enemies[i].km.pos.x;
    if (dZ == -1)
      world.enemies[i].km.la = LOOK_LEFT;
    else if (dZ == 1)
      world.enemies[i].km.la = LOOK_RIGHT;
    else if (dX == -1)
      world.enemies[i].km.la = LOOK_UP;
    else if (dX == 1)
      world.enemies[i].km.la = LOOK_DOWN;
    // printf("%d %g %g %g\n", world.enemies[i].km.la, world.enemies[i].km.pos.x,
    //        world.enemies[i].km.pos.y, world.enemies[i].km.pos.z);
    iBodyMove(pos, (body_t *)&world.enemies[i]);
  }
}

void iRestart() {
  app_state = STATE_GAME;
  iLoadLevel(world.level_num);
  iLoadPlayer(true);
  pause = false;
}

void iCheckCompletion() {
  int i;
  for (i = 0; i < world.visible_count; i++) {
    int x = world.visible[i][0], y = world.visible[i][1], z = world.visible[i][2];
    if (tiles(x, y, z).state != world.target_idx)
      break;
  }
  if (i >= world.visible_count) {
    // completed
    // printf("level completed!\n");
    if (!level_completed) {
      level_completed = true;
      if (world.level_num >= 3)
        win_cond = true;
    }

    // go to next level, or show completion
  }
}

void iHandleJump(body_t *body) {
  if (!body->jump.active)
    return;
  // jump;
  body->jump.t += dt;
  double u = body->jump.t / body->jump.duration;
  if (u > 1.0)
    u = 1.0;
  body->pos.x = body->jump.from.x + (body->jump.to.x - body->jump.from.x) * u;
  body->pos.z = body->jump.from.z + (body->jump.to.z - body->jump.from.z) * u;
  body->pos.y =
      body->jump.from.y + (body->jump.to.y - body->jump.from.y) * u - 4 * 0.5 * (u - u * u);
  // body->pos.y = body->jump.from.y + (body->jump.to.y - body->jump.from.y) * u -
  //               0.0000196f * body->jump.t * (body->jump.duration - body->jump.t);
  if (body->jump.t >= body->jump.duration)
    body->jump.active = false, body->jump.t = 0;
}

void iJump() {
  // player jump
  iHandleJump((body_t *)&world.player);
  for (int i = 0; i < world.enemy_count; i++)
    iHandleJump((body_t *)&world.enemies[i]);
}

void iWorldFwd() {
  // do jump
  iJump();
  // check level completion
  iCheckCompletion();
}

void iSaveGame() {
  // save world binary file
  FILE *fp = fopen("./saves/resume.dat", "wb+");
  if (fp == NULL) {
    printf("Error opening resume file at ./saves/resume.dat\n");
    return;
  }
  int f = fwrite(&world, sizeof(world), 1, fp);
  if (f) {
    printf("Successfully saved progress.\n");
  } else {
    printf("Couldn't save progress.\n");
  }
  fclose(fp);
}

void iResume() {
  FILE *fp = fopen("./saves/resume.dat", "rb+");
  if (fp == NULL) {
    printf("Error opening resume file at ./saves/resume.dat\n");
    return;
  }
  fread(&world, sizeof(world), 1, fp);
  fclose(fp);
  remove("./saves/resume.dat");
  iPrintWorld(&world);
  app_state = STATE_GAME;
  end_game = false;
  level_completed = false;
  win_cond = false;
  if (!enemy_step_timer)
    enemy_step_timer = iSetTimer(1000, iEnemyStep);
  else
    iResumeTimer(enemy_step_timer);
  // set a 60 fps world progress timer that will do what I want at 60fps
  if (!world_timer)
    world_timer = iSetTimer(dt, iWorldFwd);
  else
    iResumeTimer(world_timer);
}

void iGame() {
  app_state = STATE_GAME;
  end_game = false;
  level_completed = false;
  win_cond = false;
  iLoadLevel(1);
  iLoadPlayer(true);
  if (!enemy_step_timer)
    enemy_step_timer = iSetTimer(1000, iEnemyStep);
  else
    iResumeTimer(enemy_step_timer);
  // set a 60 fps world progress timer that will do what I want at 60fps
  if (!world_timer)
    world_timer = iSetTimer(dt, iWorldFwd);
  else
    iResumeTimer(world_timer);
  // now going to set a timer that will check if I have completed the level
  // drawqueue[i].pos.x=world.player.pos.x,drawqueue[i].pos.y=world.player.pos.y,drawqueue[i].pos.z=world.player.pos.z;
  // drawqueue[i].flags=0;
  // drawqueue[i].type=TYPE_PLAYER;
}

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
  } else if (app_state == STATE_HIGHSCORE) {
    iHighscore();
  } else if (app_state == STATE_HELP) {
    iHelp();
  } else if (app_state == STATE_GAME) {
    if (!end_game && !level_completed && !win_cond) {
      iSetColor(255, 255, 255);
      char pos[50];
      snprintf(pos, 50, "%d, %d, %d", (int)world.player.km.pos.x, (int)world.player.km.pos.y,
               (int)world.player.km.pos.z);
      if (selected_yes) {
        iText(10, 30, pos, GLUT_BITMAP_TIMES_ROMAN_24);
      }
      iSetColor(245, 149, 66);
      iFilledRectangle(700, 715, 37, 37);
      iShowLoadedImage(703, 716, &pause_button);
      iSetColor(12, 47, 173);
      iTextBold(10, 750, "LIVES:");
      for (int i = 1; i <= world.player.lives; i++) {
        iShowLoadedImage(30 + i * 35, 740, &life);
      }
      iTextBold(10, 700, "SCORE:");
      if (world.level_num == 1) {
        iTextBold(10, 650, "Level: 1");
        iTextBold(10, 600, "Objective :");
        iShowLoadedImage(110, 580, &level1obj);
      }

      if (world.level_num == 2) {
        iTextBold(10, 650, "Level: 2");
        iTextBold(10, 600, "Objective :");
        iShowLoadedImage(110, 580, &level2obj);
      }
      if (world.level_num == 3) {
        iTextBold(10, 650, "Level: 3");
        iTextBold(10, 600, "Objective :");
        iShowLoadedImage(110, 580, &level3obj);
      }
      char score[50];
      snprintf(score, 50, "%d", world.player.score);
      iTextBold(70, 700, score);
      iSetColor(255, 255, 51);
      if (cheat_on)
        iTextBold(10, 550, "Invincible Mode: ON");
    }
    iDrawQueue();
    if (pause) {
      iPauseMenu();
      return;
    }
    if (end_game) {
      iQuitGame();
      iGameOver();
      return;
    }
    if (win_cond) {
      iQuitGame();
      iLastScreen();
    }
    if (level_completed && world.level_num <= 2) {
      iCompleteLevel();
      return;
    }
    for (int i = 0; i < world.enemy_count; i++) {
      if (iPECollision(&world.enemies[i])) {
        iLoseLife();
        return;
      }
    }
  }

  else if (app_state == STATE_EDITOR) {
    iLoadBlocks();
    iDrawQueue();
    if (editor.grid)
      iGrid();
  } else if (app_state == STATE_CREDITS) {
    iCredits();
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
  if (app_state == STATE_MENU) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 500 && my < 530) {
        iGame();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 435 && my < 465) {
        iResume();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 370 && my < 400) {
        iSetting();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 305 && my < 335) {
        iHelp();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 240 && my < 270) {
        iHighscore();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 175 && my < 205) {
        iCredits();
      } else if (mx > width / 2 - 100 && mx < width / 2 + 50 && my > 110 && my < 140) {
        iExit();
      }
    }
  } else if (app_state == STATE_EDITOR) {
  } else if (app_state == STATE_GAME) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      if (mx > 700 && mx < 737 && my > 715 && my < 715 + 37) {
        iPauseMenu();
      }
      if (pause) {
        if (mx > 352 && mx < 352 + 145 && my > 500 && my < 500 + 35) {
          pause = false;
        } else if (mx > 352 && mx < 352 + 145 && my > 440 && my < 440 + 35) {
          iRestart();
        } else if (mx > 352 && mx < 352 + 145 && my > 440 - 120 && my < 440 - 120 + 35) {
          iSaveGame();
          app_state = STATE_MENU;
          iQuitGame();
          pause = false;
        } else if (mx > 352 && mx < 352 + 154 && my > 440 - 60 && my < 440 - 60 + 35) {
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
      if (level_completed) {
        if (world.level_num < 3)
          iLoadLevel(++world.level_num);
        if (mx > 365 && mx < 495 && my > 442 && my < 522) {
          // next level
          level_completed = false;
        } else if (mx > 350 && mx < 500 && my > 330 && my < 385) {
          // back, but also save to file for resuming
          // so, basically save everything to resume.dat in this step
          if (world.level_num < 3)
            iSaveGame();
          app_state = STATE_MENU;
          iQuitGame();
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
  } else if (app_state == STATE_HIGHSCORE) {
    if (mx > 370 && mx < 370 + 70 && my > 40 && my < 40 + 28) {
      iMenu();
    }
  } else if (app_state == STATE_CREDITS) {
    if (mx > 370 && mx < 370 + 70 && my > 40 && my < 40 + 28) {
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
void iKeyPress(unsigned char key) {
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
  } else if (app_state == STATE_GAME && !end_game && !win_cond) {
    switch (key) {
    case ESC:
      iSaveGame();
      app_state = STATE_MENU;
      iQuitGame();
      break;
    // srmkh cheat activate
    case 's': {
      if (cheat == 0)
        cheat++;
      break;
    }
    case 'r': {
      if (cheat == 1)
        cheat++;
      break;
    }
    case 'm': {
      if (cheat == 2)
        cheat++;
      break;
    }
    case 'k': {
      if (cheat == 3)
        cheat++;
      break;
    }
    case 'h': {
      if (cheat == 4) {
        cheat++;
        cheat_on = true;
      }
      break;
    }
    // buetz cheat deactivate
    case 'b': {
      if (cheat_on && cheat == 5) {
        cheat--;
      } else if (!cheat_on)
        cheat = 0;
      break;
    }
    case 'u': {
      if (cheat_on && cheat == 4) {
        cheat--;
      } else if (!cheat_on)
        cheat = 0;
      else if (cheat_on)
        cheat = 5;
      break;
    }
    case 'e': {
      if (cheat_on && cheat == 3) {
        cheat--;
      } else if (!cheat_on)
        cheat = 0;
      else if (cheat_on)
        cheat = 5;
      break;
    }
    case 't': {
      if (cheat_on && cheat == 2) {
        cheat--;
      } else if (!cheat_on)
        cheat = 0;
      else if (cheat_on)
        cheat = 5;
      break;
    }
    case 'z': {
      if (cheat_on && cheat == 1) {
        cheat--;
        cheat_on = false;
      } else if (!cheat_on)
        cheat = 0;
      else if (cheat_on)
        cheat = 5;
      break;
    }
    default: {
      if (!cheat_on)
        cheat = 0;
      else if (cheat_on)
        cheat = 5;
    } break;
    }
  } else if ((app_state == STATE_GAME && end_game) || (app_state == STATE_GAME && win_cond)) {
    switch (key) {
    case '\r':
      if (strlen(playername) > 0) {
        if (iCheckHighScore(world.player.score)) {
          iAddHighScore(playername, world.player.score);
        }
        memset(playername, 0, sizeof(playername));
        inputpos = 0;
        app_state = STATE_MENU;
      }
      break;
    case '\b':
      if (inputpos > 0) {
        inputpos--;
        playername[inputpos] = '\0';
      }
      break;
    default:
      playername[inputpos] = key;
      inputpos++;
      playername[inputpos] = '\0';
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
void iSpecialKeyPress(unsigned char key) {
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
      if (end_game || level_completed || win_cond)
        return;
      if (world.player.km.jump.active)
        return;
      vec3_t target = iPositionFinder(dirs[dir], world.player.km.pos);
      if (target.x <= -1)
        return;
      world.player.km.la = (look_t)dir;
      iBodyMove(target, (body_t *)&world.player);
      if (tiles(target.x, target.y, target.z).state < world.states_count - 1) {
        tiles(target.x, target.y, target.z).state++;
        tiles(target.x, target.y, target.z).state %= world.states_count;
        world.player.score += 25;
      } else if (tiles(target.x, target.y, target.z).state == world.states_count - 1 &&
                 world.level_num >= 3) {
        tiles(target.x, target.y, target.z).state--;
        world.player.score -= 25;
      }
      sound_3 = iPlaySound("assets/sounds/jump_sound.wav", false, 30);
    }
  }
}

int main(int argc, char *argv[]) {
  glutInit(&argc, argv);
  iSetTransparency(1);
  iLoadLevel(1);
  iPrintWorld(&world);
  iLoadResource();
  iLoadHighscore();
  printf("%d\n", sizeof(world.blocks) / sizeof(world.blocks[0]));
  iInitializeSound();
  sound_1 = iPlaySound("assets/sounds/undertale_1.wav", true, 65);
  sound_2 = iPlaySound("assets/sounds/undertale_2.wav", true, 65);
  sound_3 = iPlaySound("assets/sounds/jump_sound.wav", false, 30);
  iPauseSound(sound_3);
  iPauseSound(sound_2);
  iSetTimer(600, iAnim);
  iSetTimer(200, iAnimSetting);
  iInitialize(width, height, "Q*Bert");
  return 0;
}
