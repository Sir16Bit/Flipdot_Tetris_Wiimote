
/*
Todo:

- random select block
- new block function
- fix collision detection left/right (block width)

- clear lines



Done
- collision detection bottom
- add draw block function 
- control cant change within frame?
- fix collision detection playfield
*/


#define PANEL_NOF_COLUMNS 28                                       ///< Number of columns per panel
#define DISPLAY_NOF_PANELS 4                                       ///< Number of daisy chained panels that form the display.
#define DISPLAY_NOF_ROWS 16                                        ///< Number of rows of the display
#define DISPLAY_NOF_COLUMNS PANEL_NOF_COLUMNS* DISPLAY_NOF_PANELS  ///< Total number of columns of the display

#define ON_STATE 1
#define OFF_STATE 0
#define PLAYFIELD_TOP 2
#define PLAYFIELD_BOTTOM 13
#define PLAYFIELD_RIGHT 2

#include "ESP32Wiimote.h"

ESP32Wiimote wiimote;
int waiter = 0;
static bool logging = true;
static long last_ms = 0;
static int num_run = 0, num_updates = 0;
int playfield[112][16];
int exbutton = 0;

int lastbutton = 0;

int gamestate = 0;  //0 = splash, 1 = playing, 2 = dead
int blockX = 50;
int blockY = 7;
int blockWidth = 2;
int oldblockX = 0;
int oldblockY = 0;
int downWaiter = 0;

int logo1[] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 };
int logo2[] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0 };
int logo3[] = { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 };
int logo4[] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1 };
int logo5[] = { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1 };

//int blocks[6][3];

//int blocks[6][3];

//Super Rotation System        bar                                                                                          J                                                                                         L                                                                     Block                                                                                       S                                                                                         T                                                                               Z
int blocks[7][4] = { { 0b0000111100000000, 0b0010001000100010, 0b0000000011110000, 0b0100010001000100 }, { 0b1000111000000000, 0b0110010001000000, 0b0000111000100000, 0b0100010011000000 }, { 0b0010111000000000, 0b0100010001100000, 0b0000111010000000, 0b1100010001000000 }, { 0b0110011000000000, 0b0110011000000000, 0b0110011000000000, 0b0110011000000000 }, { 0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000 }, { 0b0100111000000000, 0b0100011001000000, 0b0000111001000000, 0b0100110001000000 }, { 0b1100011000000000, 0b0010011001000000, 0b0000110001100000, 0b0100110010000000 } };


int playfieldCollision = 0;
int blockType = 0;
int blockRotation = 0;






void drawDot(int col, int row, int pol) {
  /**
   * Serial command format: Two consecutive bytes containing x,y coordinates and dot polarity (on/off.)
   * CMDH = 1CCC CCCC
   * CMDL = 0xxP RRRR
   * 
   * C = column address
   * R = row address
   * P = dot polarity (1= on/ 0=off)
   * x = reserved for future use, set to 0 for now
   */
  //if(pol==1){pol=0;}
  //else{if(pol==0){pol=0;}}
  uint8_t cmdl, cmdh;

  cmdl = (pol ? (1 << 4) : 0) | (row & 0x0F);
  cmdh = (1 << 7) | (col & 0x7F);
  Serial2.write(cmdh);
  Serial2.write(cmdl);
  Serial2.flush();
}

void switchToSplash() {
  Serial.println("goto splashing");
  gamestate = 0;
  for (int i = 0; i <= 12; i++) {
    for (int j = 0; j <= 112; j++) {
      playfield[j][i] = 0;
    }
  }
  clearScreen();
  drawLogo();
  lastbutton = 0;
  exbutton = 0;
}

void switchToPlaying() {
  blockX = 50;
  blockY = 7;
  Serial.println("goto playing");
  gamestate = 1;
  lastbutton = 0;
  exbutton = 0;


  fillScreen();
  delay(500);
  clearScreen();

  for (int j = 111; j >= 0; j--) {
    drawDot(j, PLAYFIELD_TOP, ON_STATE);
    drawDot(j, PLAYFIELD_BOTTOM, ON_STATE);
  }
}



void fillScreen() {
  for (int i = 15; i >= 0; i--) {
    for (int j = 111; j >= 0; j--) {
      drawDot(j, i, ON_STATE);
    }
  }
}


void clearScreen() {
  for (int i = 15; i >= 0; i--) {
    for (int j = 111; j >= 0; j--) {
      drawDot(j, i, OFF_STATE);
      //delay(10);
    }
  }
}


void newBlock() {

  blockType = random(6);
  blockRotation = random(3);
  ;


  blockX = 50;
  blockY = 7;
}

int detectPlayfieldCollision() {
  int detected = 0;
  for (int i = 0; i <= 15; i++) {
    //if(i % 4 + blockX) == (detected = 1;)
    if (bitRead(blocks[blockType][blockRotation], i)) {
      if (playfield[i % 4 + blockX][i / 4 + blockY] == 1) { detected = 1; }
    }
  }
  Serial.println("detecting");
  return detected;
}





void drawLogo() {
  for (int i = 0; i < 21; i++) {
    drawDot(60 - i, 4, logo1[i]);
    drawDot(60 - i, 5, logo2[i]);
    drawDot(60 - i, 6, logo3[i]);
    drawDot(60 - i, 7, logo4[i]);
    drawDot(60 - i, 8, logo5[i]);
  }
}




void setup() {
  Serial.begin(115200);
  Serial.println("ESP32Wiimote");

  wiimote.init();
  if (!logging)
    wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL);  // optional
  Serial.println("Started");
  last_ms = millis();

  Serial2.begin(74880);
  clearScreen();
  switchToSplash();
}



void loop() {

  wiimote.task();
  num_run++;

  if (wiimote.available() > 0) {
    ButtonState button = wiimote.getButtonState();
    AccelState accel = wiimote.getAccelState();
    NunchukState nunchuk = wiimote.getNunchukState();

    num_updates++;
    if (logging) {
      char ca = (button & BUTTON_A) ? 'A' : '.';
      char cb = (button & BUTTON_B) ? 'B' : '.';
      char cc = (button & BUTTON_C) ? 'C' : '.';
      char cz = (button & BUTTON_Z) ? 'Z' : '.';
      char c1 = (button & BUTTON_ONE) ? '1' : '.';
      char c2 = (button & BUTTON_TWO) ? '2' : '.';
      char cminus = (button & BUTTON_MINUS) ? '-' : '.';
      char cplus = (button & BUTTON_PLUS) ? '+' : '.';
      char chome = (button & BUTTON_HOME) ? 'H' : '.';
      char cleft = (button & BUTTON_LEFT) ? '<' : '.';
      char cright = (button & BUTTON_RIGHT) ? '>' : '.';
      char cup = (button & BUTTON_UP) ? '^' : '.';
      char cdown = (button & BUTTON_DOWN) ? 'v' : '.';


      if (button & BUTTON_LEFT) { exbutton = 2; }
      if (button & BUTTON_RIGHT) { exbutton = 3; }
      if (button & BUTTON_UP) { exbutton = 4; }
      if (button & BUTTON_DOWN) { exbutton = 5; }
      if (button & BUTTON_HOME) { exbutton = 6; }
      //Serial.println(button);
      //Serial.println(BUTTON_A);
      //Serial.println(button & BUTTON_A);
      if (button & BUTTON_A) { exbutton = 1; }


      //Serial.println(ca);
      //Serial.println();

      //Serial.printf("button: %05x = ", (int)button);
      //Serial.print(ca);
      //Serial.print(cb);
      //Serial.print(cc);
      //Serial.print(cz);
      //Serial.print(c1);
      //Serial.print(c2);
      //Serial.print(cminus);
      //Serial.print(chome);
      //Serial.print(cplus);
      //Serial.print(cleft);
      //Serial.print(cright);
      //Serial.print(cup);
      //Serial.print(cdown);
      //Serial.printf(", wiimote.axis: %3d/%3d/%3d", accel.xAxis, accel.yAxis, accel.zAxis);
      //Serial.printf(", nunchuk.axis: %3d/%3d/%3d", nunchuk.xAxis, nunchuk.yAxis, nunchuk.zAxis);
      //Serial.printf(", nunchuk.stick: %3d/%3d\n", nunchuk.xStick, nunchuk.yStick);
    }
  }

  if (!logging) {
    long ms = millis();
    if (ms - last_ms >= 1000) {
      Serial.printf("Run %d times per second with %d updates\n", num_run, num_updates);
      num_run = num_updates = 0;
      last_ms += 1000;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  //// Tetris game section
  if (waiter == 2000) {
    //wait for input to start game
    if (gamestate == 1) {

      oldblockX = blockX;
      oldblockY = blockY;

      //turn off old block

      for (int i = 0; i <= 15; i++) {

        if (bitRead(blocks[blockType][blockRotation], i)) {
          drawDot(i % 4 + blockX, i / 4 + blockY, OFF_STATE);
        }
      }





      if (exbutton > 0) {
        if (exbutton == 1) {
          blockRotation++;
          if (blockRotation == 4) { blockRotation = 0; }
        }
        //if (exbutton == 1) { delay(1); } //A
        //if (exbutton == 2) { delay(1); } //LEft
        // if (exbutton == 3) { delay(1); } //Right
        if (exbutton == 4) {
          if (blockY > PLAYFIELD_TOP - blockWidth) { blockY--; }

          /*
 for (int i = 0; i <= 15; i++) { blockY--;
            if(bitRead(blocks[blockType][blockRotation], i))
            { if(playfield[i % 4 + blockX][i / 4 + blockY] == 1) { playfieldCollision = 1;}
            
            
            
            
             }
          }
          if (playfieldCollision == 1)
          {blockY++;}

playfieldCollision = 0;
*/


        }  //Up

        if (exbutton == 5) {
          if (blockY < PLAYFIELD_BOTTOM - blockWidth) { blockY++; }
        }                                         //Down
        if (exbutton == 6) { switchToSplash(); }  //reset

        /* button debug
        if (lastbutton == 1) { drawDot(33, 8, ON_STATE); }
        if (lastbutton == 2) { drawDot(31, 8, ON_STATE); }
        if (lastbutton == 3) { drawDot(29, 8, ON_STATE); }
        if (lastbutton == 4) { drawDot(30, 7, ON_STATE); }
        if (lastbutton == 5) { drawDot(30, 9, ON_STATE); }*/
      }
      //Drawshape

      //if nothing:
      //block bottom detection

      if (downWaiter >= 4) {
        downWaiter = 0;

        if (blockX > 0) {
          blockX--;

          //detectPlayfieldCollision();
          if (detectPlayfieldCollision() == 1) {
            //if block collides with playfield, move back up and convert to playfield
            blockX++;
            for (int i = 0; i <= 15; i++) {

              if (bitRead(blocks[blockType][blockRotation], i)) {

                playfield[i % 4 + blockX][i / 4 + blockY] = 1;
              }
            }
            newBlock();
          }
        }

        else {
          //if block is at bottom; lock and convert to playfield
          for (int i = 0; i <= 15; i++) {

            if (bitRead(blocks[blockType][blockRotation], i)) {

              playfield[i % 4 + blockX][i / 4 + blockY] = 1;
              //drawDot(i % 4 + blockX, i / 4 + blockY, OFF_STATE);
            }
          }



          newBlock();
        }
      } else {
        downWaiter++;
      }
      //update playfield
      for (int i = 0; i <= 12; i++) {
        for (int j = 0; j <= 112; j++) {
          if (playfield[j][i] == 1) { drawDot(j, i, ON_STATE); }
        }
        //delay(10);
      }




      for (int i = 0; i <= 15; i++) {

        if (bitRead(blocks[blockType][blockRotation], i)) {
          drawDot(i % 4 + blockX, i / 4 + blockY, ON_STATE);
        }
      }
    }
    if (gamestate == 0) {
      ///Serial.print("exbutton is:");
      // Serial.println(exbutton);
      if (exbutton > 0 and exbutton < 6) {
        lastbutton = 0;
        exbutton = 0;
        switchToPlaying();
      }
    }

    lastbutton = 0;
    exbutton = 0;
    playfieldCollision = 0;
    delay(5);
    waiter = 0;
  } else {
    waiter++;
  }
}
