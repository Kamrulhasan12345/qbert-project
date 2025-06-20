#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "iGraphics.h"

#define PI 3.14159265
#define MAX_SIZE 10
#define ESC 0x1b

typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_EDITOR
} app_t;

typedef enum {
    TYPE_BLOCK,
    TYPE_PLAYER
} object_t;

typedef struct {
    bool wireframe;
    bool grid;
    bool debug;
} editor_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

typedef struct {
    bool valid;
    int state;
} tile_t;

typedef enum {
    LOOK_LEFT,
    LOOK_RIGHT,
    LOOK_UP,
    LOOK_DOWN
} look_t;

typedef struct {
    double x, y, z;
} position_t;

typedef struct {
    position_t from;
    position_t to;
    float t,duration;
    bool active;
} jumper_t;

typedef struct {
    position_t pos;
    look_t la;
    jumper_t jump;
} body_t;

typedef struct {
    object_t type;
    position_t pos;
    uint8_t flags;
} drawqueue_t;

app_t app_state = STATE_MENU;

editor_t editor = {.wireframe=false,.grid=false,.debug=false};

tile_t tiles[MAX_SIZE][MAX_SIZE][MAX_SIZE];

drawqueue_t drawqueue[MAX_SIZE*MAX_SIZE*MAX_SIZE+1];

body_t player;

int width=800,height=800;

double start_x=width/2;
double start_y=height*0.9;
double a = 50;

double blocksPos3d[][3]={
    {7,7,0},{6,7,1},{5,7,2},{4,7,3},{3,7,4},{2,7,5},{1,7,6},{0,7,7},
    {6,6,0},{5,6,1},{4,6,2},{3,6,3},{2,6,4},{1,6,5},{0,6,6},
    {5,5,0},{4,5,1},{3,5,2},{2,5,3},{1,5,4},{0,5,5},
    {4,4,0},{3,4,1},{2,4,2},{1,4,3},{0,4,4},
    {3,3,0},{2,3,1},{1,3,2},{0,3,3},
    {2,2,0},{1,2,1},{0,2,2},
    {1,1,0},{0,1,1},
    {0,0,0},
    {0,0,1},
    {1,4,4},
    // {2,3,5}
    // {5,4,3},
    {4,5,2},
    {3,7,5}
};

int n =  sizeof(blocksPos3d)/sizeof(blocksPos3d[0]);

int cmp_dk (const void *a, const void *b) {
    drawqueue_t* A=(drawqueue_t*)a;
    drawqueue_t* B=(drawqueue_t*)b;
    int d1 = A->pos.x+A->pos.z+(MAX_SIZE-1-A->pos.y)+A->type;
    int d2 = B->pos.x+B->pos.z+(MAX_SIZE-1-B->pos.y)+B->type;
    return (d1>d2)-(d1<d2);
}

void iTile(double x, double y) {
    double x_coords[]={x,x+a*cos(PI/6),x,x-a*cos(PI/6)};
    double y_coords[]={y,y-a/2,y-a,y-a/2};
    iFilledPolygon(x_coords,y_coords,4);
}

void iTileOutline(double x, double y) {
    double x_coords[]={x,x+a*cos(PI/6),x,x-a*cos(PI/6)};
    double y_coords[]={y,y-a/2,y-a,y-a/2};
    iPolygon(x_coords,y_coords,4);
}

void iSide(double x, double y) {
    double x_coords[]={x,x,x+a*cos(PI/6),x+a*cos(PI/6)};
    double y_coords[]={y-a,y-2*a,y-3*a/2,y-a/2};
    iSetColor(49, 70, 70);
    iFilledPolygon(x_coords,y_coords,4);
    x_coords[2]=x-a*cos(PI/6),x_coords[3]=x-a*cos(PI/6);
    iSetColor(86, 169, 152);
    iFilledPolygon(x_coords,y_coords,4);
}

void iSideOutline(double x, double y) {
    double x_coords[]={x,x,x+a*cos(PI/6),x+a*cos(PI/6)};
    double y_coords[]={y-a,y-2*a,y-3*a/2,y-a/2};
    iPolygon(x_coords,y_coords,4);
    x_coords[2]=x-a*cos(PI/6),x_coords[3]=x-a*cos(PI/6);
    iPolygon(x_coords,y_coords,4);
}

void iDrawQueue() {
    // for(int s=0; s<MAX_SIZE*2; s++) {
    //     for(int x=0; x<=s&&x<MAX_SIZE; x++) {
    //         int z = s-x;
    //         if (z>=MAX_SIZE)continue;
    //         for(int y =MAX_SIZE-1;y>=0;y--) {
    //             if (tiles[y][x][z].valid) {
    //                 if (!editor.wireframe) {
    //                     iSetColor(86, 70, 239);
    //                     iTile(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
    //                     iSide(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
    //                 } else {
    //                     iSetColor(49, 70, 70);
    //                     iTileOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
    //                     iSideOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
    //                 }
    //                 if (player.pos.x==x&&player.pos.y==y&&player.pos.z==z) {
    //                     iSetTransparentColor(0,0,0,0.5);
    //                     iFilledEllipse(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,a/4,a/8);
    //                     iSetColor(240,10,10);
    //                     iFilledCircle(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,a/3);
    //                 }
    //             }
    //         }
    //     }
    // }
    // current approach: use a draw queue
    int i = 0;
    for(int y=MAX_SIZE-1;y>=0;y--) {
        for(int x = 0; x < MAX_SIZE;x++) {
            for(int z = 0; z < MAX_SIZE;z++) {
                if (!tiles[y][x][z].valid) continue;
                drawqueue[i].pos.x=x,drawqueue[i].pos.y=y,drawqueue[i].pos.z=z;
                drawqueue[i].flags=tiles[y][x][z].state;
                drawqueue[i++].type=TYPE_BLOCK;
            }
        }
    }
    if (app_state==STATE_GAME) {
        drawqueue[i].pos.x=player.pos.x,drawqueue[i].pos.y=player.pos.y,drawqueue[i].pos.z=player.pos.z;
        drawqueue[i].flags=player.jump.active;
        drawqueue[i++].type=TYPE_PLAYER;
    }
    qsort(drawqueue, i, sizeof(drawqueue_t),cmp_dk);
    // draw them now
    // for(int k = 0; k < i; k++) printf("p: %lf %lf %lf f: %d t: %d\n",drawqueue[k].x,drawqueue[k].y,drawqueue[k].z,drawqueue[k].flags,drawqueue[k].type);
    for(int j=0; j<i;j++) {
        double x=drawqueue[j].pos.x,y=drawqueue[j].pos.y,z=drawqueue[j].pos.z;
        switch(drawqueue[j].type) {
            case TYPE_BLOCK: {
                if (!editor.wireframe) {
                    iSetColor(86, 70, 239);
                    iTile(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    iSide(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                } else {
                    iSetColor(49, 70, 70);
                    iTileOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    iSideOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                }
                break;
            }
            case TYPE_PLAYER: {
                iSetTransparentColor(0,0,0,0.5);
                iFilledEllipse(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,a/4,a/8);
                iSetColor(240,10,10);
                iFilledCircle(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,a/3);
                break;
            }
        }
    }
}

void iGrid() {
    double c = start_x/sqrt(3)+start_y+((int)((width-start_x)/sqrt(3)+height-start_y))*a;
    iSetTransparentColor(255,255,255,0.25);
    for(;c>=0;c-=a)
        iLine(0,c,width,-width/sqrt(3)+c);
    c=start_y-start_x/sqrt(3)+((int)(height-start_y+start_x/sqrt(3)))*a;
    for(;width/sqrt(3)+c>=0;c-=a)
        iLine(0,c,width,width/sqrt(3)+c);
}

void iMenu() {
    iSetColor(255,255,255);
    iTextAdvanced(start_x-200,start_y-100,"Q*Bert",1, 2);
    iSetColor(86, 169, 152);
    iFilledRectangle(width/2-100,height/2-20,200,40);
    iSetColor(0,0,0);
    iText(width/2-20,height/2-5,"Start");
}

void iBlock() {
    for(int i = 0; i < n;i++) {
        int x=blocksPos3d[i][0],y=blocksPos3d[i][1],z=blocksPos3d[i][2];
        tiles[y][x][z].valid=true;
        tiles[y][x][z].state=0;
    }
}

void iPlayer() {
    player.pos.x=0;
    player.pos.y=0;
    player.pos.z=1;
    player.jump.active=0;
}

void iPlayerMove(int x, int y, int z) {
    printf("%d %d %d\n",x,y,z);
    if (!(x>=0&&x<MAX_SIZE&&y>=0&&y<MAX_SIZE&&z>=0&&z<MAX_SIZE)) {
        // die and reset
        printf("so, %d %d %d is invalid\n",x,y,z);
        return;
    }
    if ((y-1>=0&&tiles[y-1][x][z].valid) && 1) {
        printf("so, up?\n");
        if (y-2>=-1&&!tiles[y-2][x][z].valid) {
            printf("so, up one block actually?\n");
            player.pos.x=x,player.pos.y=y-1,player.pos.z=z;
            printf("%lf %lf %lf\n",player.pos.x,player.pos.y,player.pos.z);
        }
        // then go, otherwise stay where you are
    } else if (tiles[y][x][z].valid) {
        // simply walk to this one, with no jump anim
        printf("so walk straight?\n");
        player.pos.x=x,player.pos.y=y,player.pos.z=z;
    } else if (y+1<=MAX_SIZE&&tiles[y+1][x][z].valid) {
        // then move to this one, still a jump anim
        printf("so down one block?\n");
        player.pos.x=x,player.pos.y=y+1,player.pos.z=z;
    } else {
        // dont move
        printf("huh? no move? thats boring...\n");
        return;
    }
    // now initiate movement and jump animation
    printf("time to go... yay!!!\n");
    printf("%lf %lf %lf\n",player.pos.x,player.pos.y,player.pos.z);
    return;
}

void iGame() {
    app_state=STATE_GAME;
    iBlock();
    iPlayer();
    // drawqueue[i].pos.x=player.pos.x,drawqueue[i].pos.y=player.pos.y,drawqueue[i].pos.z=player.pos.z;
    // drawqueue[i].flags=0;
    // drawqueue[i].type=TYPE_PLAYER;
}



// /*
// function iGenTiles() is the function that generates tiles for the blocks
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
//                 if (j<i) tiles[i][j+1].x=cx,tiles[i][j+1].y=cy;
//             }
//         } else {
//             cx+=a*cos(PI/6),cy-=(a*cos(PI/3)+a);
//             tiles[i][i].x=cx,tiles[i][i].y=cy;
//             for(int j=i;j>=0;j--) {
//                 iDTile(tiles[i][j].x,tiles[i][j].y);
//                 cx-=(j==0?0:2*a*cos(PI/6));
//                 if (j>0) tiles[i][j-1].x=cx,tiles[i][j-1].y=cy;
//             }
//         }
//     }
// }

// /*
// function iGenSides() is the function that generates the sides of the blocks
// */
// void iGenSides()
// {
//     double current_x=start_x-a*cos(PI/6),current_y=start_y+a/2;
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

    if (app_state==STATE_MENU) {
        iMenu();
    } else if (app_state==STATE_GAME) {
        iSetColor(255,255,255);
        char pos[50];
        snprintf(pos, 50, "%d, %d, %d",(int)player.pos.x, (int)player.pos.y, (int)player.pos.z);
        iText(10, 10, pos,GLUT_BITMAP_TIMES_ROMAN_24);
        // iGame();
        iDrawQueue();
    } else if (app_state==STATE_EDITOR) {
        iBlock();
        iDrawQueue();
        if (editor.grid) iGrid();
    }
}
/*
function iMouseMove() is called when the user moves the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouseMove(int mx, int my) {
    // place your codes here
}

/*
function iMouseDrag() is called when the user presses and drags the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouseDrag(int mx, int my) {
    // place your codes here
}

/*
function iMouse() is called when the user presses/releases the mouse.
(mx, my) is the position where the mouse pointer is.
*/
void iMouse(int button, int state, int mx, int my) {
    // if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    // {
    //     // pl
    // }
    // if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    // {
    //     // place your codes here
    // }
    if (app_state==STATE_MENU) {
        if (button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
            if (mx>width/2-100&&mx<width/2+100&&my>height/2-20&&my<height/2+20) {
                // start menu clicked
                iGame();
            }
        }

    } else if (app_state==STATE_EDITOR) {

    } else if (app_state==STATE_GAME) {
        
    }
}

/*
function iMouseWheel() is called when the user scrolls the mouse wheel.
dir = 1 for up, -1 for down.
*/
void iMouseWheel(int dir, int mx, int my) {
    // place your code here
}

/*
function iKeyboard() is called whenever the user hits a key in keyboard.
key- holds the ASCII value of the key pressed.
*/
void iKeyboard(unsigned char key) {
    if (app_state==STATE_EDITOR) {
        switch(key) {
            case 'w': editor.wireframe^=1; break;
            case 'g': editor.grid^=1; break;
            case 'q': case ESC: editor.wireframe=false;editor.grid=false; app_state=STATE_MENU; break;
            default: break;
        }
    } else if (app_state==STATE_GAME) {
        switch(key) {
            case 'q': case ESC: app_state=STATE_MENU; break;
            case 'r': 
            {
                player.pos.x=0;
                player.pos.y=0;
                player.pos.z=0; 
                printf("%lf %lf %lf\n",player.pos.x,player.pos.y,player.pos.z); 
                break;
            }            
            default: break;
        }
    } else {
        switch(key) {
            default: break;
        }
    }
}

/*
function iSpecialKeyboard() is called whenver user hits special keys likefunction
keys, home, end, pg up, pg down, arraows etc. you have to use
appropriate constants to detect them. A list is:
GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6,
GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11,
GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN,
GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END,
GLUT_KEY_INSERT */
void iSpecialKeyboard(unsigned char key)
{
    if (app_state==STATE_MENU) {

    } else if (app_state==STATE_EDITOR) {

    } else if (app_state==STATE_GAME) {
        switch(key) {
            case GLUT_KEY_END: break;
            case GLUT_KEY_LEFT: iPlayerMove(player.pos.x, player.pos.y,player.pos.z-1); break;
            case GLUT_KEY_RIGHT: iPlayerMove(player.pos.x, player.pos.y,player.pos.z+1); break;
            case GLUT_KEY_UP: iPlayerMove(player.pos.x-1, player.pos.y,player.pos.z); break;
            case GLUT_KEY_DOWN: iPlayerMove(player.pos.x+1, player.pos.y,player.pos.z); break;
            default: break;
        }
    }
    
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    iSetTransparency(1);
    printf("%d\n",sizeof(blocksPos3d)/sizeof(blocksPos3d[0]));
    iInitialize(width, height, "Q*Bert");
    return 0;
}
