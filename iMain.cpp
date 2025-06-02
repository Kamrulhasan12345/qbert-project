#include <math.h>
#include <stdbool.h>

#include "iGraphics.h"

#define PI 3.14159265
#define MAX_SIZE 10
#define ESC 0x1b

typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_EDITOR
} app_t;

typedef struct {
    bool wireframe;
    bool grid;
    bool debug;
} editor_t;

typedef struct {
    bool valid;
    int state;
} tile_t;

app_t app_state = STATE_GAME;

editor_t editor = {.wireframe=false,.grid=false,.debug=false};

tile_t tiles[MAX_SIZE][MAX_SIZE][MAX_SIZE];

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
    {0,0,0},{0,0,1},
    {2,3,5},{1,5,5},
    {5,4,3},
    // {5,5,7}
};

double playerPos3d[3] = {0,0,0};

int n =  sizeof(blocksPos3d)/sizeof(blocksPos3d[0]);

void iDTile(double x, double y) {
    double x_coords[]={x,x+a*cos(PI/6),x,x-a*cos(PI/6)};
    double y_coords[]={y,y-a/2,y-a,y-a/2};
    iFilledPolygon(x_coords,y_coords,4);
}

void iDTileOutline(double x, double y) {
    double x_coords[]={x,x+a*cos(PI/6),x,x-a*cos(PI/6)};
    double y_coords[]={y,y-a/2,y-a,y-a/2};
    iPolygon(x_coords,y_coords,4);
}

void iDSide(double x, double y) {
    double x_coords[]={x,x,x+a*cos(PI/6),x+a*cos(PI/6)};
    double y_coords[]={y-a,y-2*a,y-3*a/2,y-a/2};
    iSetColor(49, 70, 70);
    iFilledPolygon(x_coords,y_coords,4);
    x_coords[2]=x-a*cos(PI/6),x_coords[3]=x-a*cos(PI/6);
    iSetColor(86, 169, 152);
    iFilledPolygon(x_coords,y_coords,4);
}

void iDSideOutline(double x, double y) {
    double x_coords[]={x,x,x+a*cos(PI/6),x+a*cos(PI/6)};
    double y_coords[]={y-a,y-2*a,y-3*a/2,y-a/2};
    iPolygon(x_coords,y_coords,4);
    x_coords[2]=x-a*cos(PI/6),x_coords[3]=x-a*cos(PI/6);
    iPolygon(x_coords,y_coords,4);
}

void iGenTiles() {
    // for(int y = MAX_SIZE-1;y>=0;y--) {
    //     for(int x=0;x<MAX_SIZE;x++) {
    //         for(int z=0;z<MAX_SIZE;z++) {
    //             if (tiles[y][x][z].valid) {
                    // if (!editor.wireframe) {
                    //     iSetColor(86, 70, 239);
                    //     iDTile(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    //     iDSide(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    // } else {
                    //     iSetColor(49, 70, 70);
                    //     iDTileOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    //     iDSideOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    // }
    //             }
    //         }
    //     }
    // }
    for(int s=0; s<MAX_SIZE*2; s++) {
        for(int x=0; x<=s&&x<MAX_SIZE; x++) {
            int z = s-x;
            if (z>=MAX_SIZE)continue;
            for(int y =MAX_SIZE-1;y>=0;y--) {
                if (tiles[y][x][z].valid) {
                    if (!editor.wireframe) {
                        iSetColor(86, 70, 239);
                        iDTile(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                        iDSide(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    } else {
                        iSetColor(49, 70, 70);
                        iDTileOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                        iDSideOutline(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a);
                    }
                    if (playerPos3d[0]==x&&playerPos3d[1]==y&&playerPos3d[2]==z) {
                        iSetColor(240,10,10);
                        iFilledCircle(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,15);
                    }
                }
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
}

void iGame() {
    for(int i = 0; i < n;i++) {
        int x=blocksPos3d[i][0],y=blocksPos3d[i][1],z=blocksPos3d[i][2];
        tiles[y][x][z].valid=true;
        tiles[y][x][z].state=0;
    }
}

void iPlayer() {
    int x=playerPos3d[0],y=playerPos3d[1],z=playerPos3d[2];
    iSetColor(240,10,10);
    iFilledCircle(start_x+(z-x)*a*cos(PI/6),start_y-(z+x)*a/2-y*a-a/2,15);
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
    iSetColor(255,255,255);
    char pos[50];
    snprintf(pos, 50, "%d, %d, %d",(int)playerPos3d[0], (int)playerPos3d[1], (int)playerPos3d[2]);
    iText(10, 10, pos,GLUT_BITMAP_TIMES_ROMAN_24);
    if (app_state==STATE_MENU) {
        iMenu();
    } else if (app_state==STATE_GAME) {
        iGame();
        iGenTiles();
    } else if (app_state==STATE_EDITOR) {
        iGenTiles();
        if (editor.grid) iGrid();
    }
    // iSetColor(255,255,255);
    // iGenSides();
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
                playerPos3d[0]=0;
                playerPos3d[1]=0;
                playerPos3d[2]=0; 
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]); 
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
            case 'r': 
            {
                playerPos3d[0]=0;
                playerPos3d[1]=0;
                playerPos3d[2]=0; 
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]); 
                break;
            }
            case GLUT_KEY_LEFT: 
            {
                playerPos3d[0]++; 
                while (!tiles[(int)playerPos3d[1]][(int)playerPos3d[0]][(int)playerPos3d[2]].valid) {
                    playerPos3d[1]++; 
                    printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                }
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                break;
            }
            case GLUT_KEY_RIGHT: 
            {
                playerPos3d[2]++; 
                while (!tiles[(int)playerPos3d[1]][(int)playerPos3d[0]][(int)playerPos3d[2]].valid) {
                    playerPos3d[1]++; 
                    printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                }
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                break;
            }
            case GLUT_KEY_UP: 
            {
                playerPos3d[0]<=0?:playerPos3d[0]--; 
                if(playerPos3d[1]-1>=0&&tiles[(int)playerPos3d[1]-1][(int)playerPos3d[0]][(int)playerPos3d[2]].valid) 
                    playerPos3d[1]--; 
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                break;
            }
            case GLUT_KEY_DOWN: 
            {
                playerPos3d[2]<=0?:playerPos3d[2]--; 
                if(playerPos3d[1]-1>=0&&tiles[(int)playerPos3d[1]-1][(int)playerPos3d[0]][(int)playerPos3d[2]].valid)
                    playerPos3d[1]--; 
                printf("%lf %lf %lf\n",playerPos3d[0],playerPos3d[1],playerPos3d[2]);
                break;
            }
            default: break;
        }
    }
    
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    iSetTransparency(1);
    printf("%d",sizeof(blocksPos3d)/sizeof(blocksPos3d[0]));
    iInitialize(width, height, "Q*Bert");
    return 0;
}
