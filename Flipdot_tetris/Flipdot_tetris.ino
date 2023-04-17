

/*
Todo:

- Redrawing entire playfield is slow (only draw last block)
- death (collision on new block)
- Fixed: Bug: wobbly dots
- Fixed: Bug: slowdown after ~10 blocks
- block dissapears when line clearing
- movement too fast (wait before repeating)
- rotation too fast (only rotate once per press)
- Bug: rotation causes ghosting
- score

- Request: show next block chasing
- Request: Show pixelbar logo when standby



notes:
Done: Move undraw to not 'collide with bottom' then playfield update is not needed. 

Last button should be oldExButton



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

int gamestate = 0;  // 0 = splash, 1 = playing, 2 = dead
int blockX = 50;
int blockY = 7;

int oldblockX = 0;
int oldblockY = 0;
int downWaiter = 0;





int gameover[28] = { 0b0011110000111000, 0b1100011011111110, 0b0011111001110011, 0b0111111101111110,
                     0b0110000001111100, 0b1110111011100000, 0b0111001101110011, 0b0111000001110011,
                     0b1110000011100110, 0b1111111011100000, 0b0111001101110011, 0b0111000001110011,
                     0b1110111011100110, 0b1111111011111100, 0b0111001101110011, 0b0111111001110110,
                     0b1110011011111110, 0b1101011011100000, 0b0111001100110110, 0b0111000001111100,
                     0b0110011011100110, 0b1100011011100000, 0b0111001100011100, 0b0111000001110110,
                     0b0011111011100110, 0b1100011011111110, 0b0011111000001000, 0b0111111101110011 };





int logo1[] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 };
int logo2[] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0 };
int logo3[] = { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 };
int logo4[] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1 };
int logo5[] = { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1 };

// Super Rotation System        bar J L Block S T Z
int blocks[7][4] = { { 0b0000111100000000, 0b0010001000100010, 0b0000000011110000, 0b0100010001000100 },
                     { 0b1000111000000000, 0b0110010001000000, 0b0000111000100000, 0b0100010011000000 },
                     { 0b0010111000000000, 0b0100010001100000, 0b0000111010000000, 0b1100010001000000 },
                     { 0b0110011000000000, 0b0110011000000000, 0b0110011000000000, 0b0110011000000000 },
                     { 0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000 },
                     { 0b0100111000000000, 0b0100011001000000, 0b0000111001000000, 0b0100110001000000 },
                     { 0b1100011000000000, 0b0010011001000000, 0b0000110001100000, 0b0100110010000000 } };

int blockType = 0;
int blockRotation = 0;

void drawDot(int col, int row, int pol) {
  /**
   * Serial command format: Two consecutive bytes containing x,y coordinates and
   * dot polarity (on/off.) CMDH = 1CCC CCCC CMDL = 0xxP RRRR
   *
   * C = column address
   * R = row address
   * P = dot polarity (1= on/ 0=off)
   * x = reserved for future use, set to 0 for now
   */
  // if(pol==1){pol=0;}
  // else{if(pol==0){pol=0;}}
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

void turnOffOldblock() {

  // turn off old block
  for (int i = 0; i <= 15; i++) {
    if (bitRead(blocks[blockType][blockRotation], i)) { drawDot(i % 4 + oldblockX, i / 4 + oldblockY, OFF_STATE); }
  }
}
void switchToPlaying() {
  Serial.println("goto playing");
  gamestate = 1;
  lastbutton = 0;
  exbutton = 0;
  fillScreen();
  delay(500);
  clearScreen();

  for (int j = 111; j >= 0; j--) {  // draw playfield edges
    drawDot(j, PLAYFIELD_TOP, ON_STATE);
    drawDot(j, PLAYFIELD_BOTTOM, ON_STATE);
  }
  newBlock();
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
      // delay(10);
    }
  }
}

void newBlock() {
  int linesCleared = 0;
  // clear lines
  for (int i = 0; i <= 112; i++) {
    int lineCount = 0;
    for (int j = 3; j <= 12; j++) {
      if (playfield[i][j] == 1) { lineCount++; }
    }
    if (lineCount == 10) {  // full line detected
      // clear line + update playfield
      linesCleared = 1;
      for (int k = 3; k <= 12; k++) {
        drawDot(i, k, OFF_STATE);
        delay(100);
        playfield[i][k] = 0;  // <--- not needed?
      }
      // squash playfield

      for (int cl = i; cl <= 112; cl++) {
        for (int m = 3; m <= 12; m++) {
          playfield[cl][m] = playfield[cl + 1][m];
        }
      }
      i--;
    }

  }


    for (int a = 3; a <= 12; a++) {
      for (int b = 0; b <= 112; b++) {
        if (playfield[b][a] == 0) { drawDot(b, a, OFF_STATE); }
        if (playfield[b][a] == 1) { drawDot(b, a, ON_STATE); }
      }
    }




    blockType = random(6);
    blockRotation = random(3);

    blockX = 20;
    blockY = 7;
  
}
  int detectPlayfieldCollision() {
    int detected = 0;
    for (int i = 0; i <= 15; i++) {

      if (bitRead(blocks[blockType][blockRotation], i)) {
        if (playfield[i % 4 + blockX][i / 4 + blockY] == 1) { detected = 1; }  // detect playfield
        if (i % 4 + blockX == -1) { detected = 1; }                            // detect Right
      }
    }
    Serial.println("detecting");
    return detected;
  }

  int detectPlayfieldCollisionSide() {
    int detected = 0;
    for (int i = 0; i <= 15; i++) {

      if (bitRead(blocks[blockType][blockRotation], i)) {
        if (i / 4 + blockY == 2) { detected = 1; }                             // detect top
        if (i / 4 + blockY == 13) { detected = 1; }                            // detect bottom
        if (playfield[i % 4 + blockX][i / 4 + blockY] == 1) { detected = 1; }  // detect playfield
      }
    }
    Serial.println("detecting");
    return detected;
  }

  int detectPlayfieldCollisionRotate() {
    int detected = 0;
    for (int i = 0; i <= 15; i++) {

      if (bitRead(blocks[blockType][blockRotation], i)) {
        if (i / 4 + blockY == 2) { detected = 1; }   // detect top
        if (i / 4 + blockY == 13) { detected = 1; }  // detect bottom
        if (i % 4 + blockX == -1) { detected = 1; }
        if (playfield[i % 4 + blockX][i / 4 + blockY] == 1) { detected = 1; }  // detect playfield
      }
    }
    //Serial.println("detecting");
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
    if (!logging) wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL);  // optional
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
        // Serial.println(button);
        // Serial.println(BUTTON_A);
        // Serial.println(button & BUTTON_A);
        if (button & BUTTON_A) { exbutton = 1; }

        // Serial.println(ca);
        // Serial.println();

        // Serial.printf("button: %05x = ", (int)button);
        // Serial.print(ca);
        // Serial.print(cb);
        // Serial.print(cc);
        // Serial.print(cz);
        // Serial.print(c1);
        // Serial.print(c2);
        // Serial.print(cminus);
        // Serial.print(chome);
        // Serial.print(cplus);
        // Serial.print(cleft);
        // Serial.print(cright);
        // Serial.print(cup);
        // Serial.print(cdown);
        // Serial.printf(", wiimote.axis: %3d/%3d/%3d", accel.xAxis, accel.yAxis,
        // accel.zAxis); Serial.printf(", nunchuk.axis: %3d/%3d/%3d",
        // nunchuk.xAxis, nunchuk.yAxis, nunchuk.zAxis); Serial.printf(",
        // nunchuk.stick: %3d/%3d\n", nunchuk.xStick, nunchuk.yStick);
      }
    }

    if (!logging) {
      long ms = millis();
      if (ms - last_ms >= 1000) {
        Serial.printf("Run %d times per second with %d updates\n", num_run,
                      num_updates);
        num_run = num_updates = 0;
        last_ms += 1000;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    //// Tetris game section
    if (waiter == 2000) {
      // wait for input to start game
      if (gamestate == 1) {
        oldblockX = blockX;
        oldblockY = blockY;



        if (exbutton > 0) {

          if (exbutton == 1) {  // Button A; Rotate
            blockRotation++;
            turnOffOldblock();
            if (blockRotation == 4) { blockRotation = 0; }
            if (detectPlayfieldCollisionRotate() == 1) {
              blockRotation--;
              if (blockRotation == -1) { blockRotation = 3; }
            }
          }

          if (exbutton == 4) {  // Button Up
            blockY--;
            turnOffOldblock();
            if (detectPlayfieldCollisionSide() == 1) { blockY++; }
          }

          if (exbutton == 5) {  // Button Down
            blockY++;
            turnOffOldblock();
            if (detectPlayfieldCollisionSide() == 1) { blockY--; }
          }
          if (exbutton == 6) { switchToSplash(); }  // reset
        }

        if (downWaiter >= 4) {  // Auto move block down
          downWaiter = 0;
          blockX--;

          if (detectPlayfieldCollision() == 1) {
            // if block collides with playfield, move back up and convert to playfield
            blockX++;
            for (int i = 0; i <= 15; i++) {
              if (bitRead(blocks[blockType][blockRotation], i)) { playfield[i % 4 + blockX][i / 4 + blockY] = 1; }
            }
            newBlock();
          } else {

            turnOffOldblock();
          }

        } else {

          downWaiter++;
        }

        /*
      // update playfield
      for (int i = 0; i <= 12; i++) {
        for (int j = 0; j <= 112; j++) {
          if (playfield[j][i] == 1) { drawDot(j, i, ON_STATE); }
        }
      }
      */

        // draw Block
        for (int i = 0; i <= 15; i++) {
          if (bitRead(blocks[blockType][blockRotation], i)) { drawDot(i % 4 + blockX, i / 4 + blockY, ON_STATE); }
        }
      }
      if (gamestate == 0) {  // start game
        if (exbutton > 0 and exbutton < 6) {
          lastbutton = 0;
          exbutton = 0;
          switchToPlaying();
        }
      }

      lastbutton = 0;
      exbutton = 0;

      delay(5);
      waiter = 0;
    } else {
      waiter++;
    }
  }
