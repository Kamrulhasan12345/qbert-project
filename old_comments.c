// /*
// function iGenworld.tiles() is the function that generates world.tiles for
// the blocks
// */
// void iGenworld.tiles()
// {
//     double cx=start_x-a*cos(PI/6),cy=start_y+(a*cos(PI/3)+a);
//     for(int i = 0; i< 7; i++) {
//         if (i&1) {
//             cx-=a*cos(PI/6),cy-=(a*cos(PI/3)+a);
//             world.tiles[i][0].x=cx,world.tiles[i][0].y=cy;
//             for(int j=0;j<=i;j++) {
//                 iDTile(world.tiles[i][j].x,world.tiles[i][j].y);
//                 cx+=(j==i?0:2*a*cos(PI/6));
//                 if (j<i)
//                 world.tiles[i][j+1].x=cx,world.tiles[i][j+1].y=cy;
//             }
//         } else {
//             cx+=a*cos(PI/6),cy-=(a*cos(PI/3)+a);
//             world.tiles[i][i].x=cx,world.tiles[i][i].y=cy;
//             for(int j=i;j>=0;j--) {
//                 iDTile(world.tiles[i][j].x,world.tiles[i][j].y);
//                 cx-=(j==0?0:2*a*cos(PI/6));
//                 if (j>0)
//                 world.tiles[i][j-1].x=cx,world.tiles[i][j-1].y=cy;
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