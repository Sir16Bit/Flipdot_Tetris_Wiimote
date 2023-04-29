

/*
Todo:

features:

- add: score
- movement needs timeout + repeating (now only has timeout)

- Bug: rotation sometimes skips timout when holding down button
- Bug: when holding button sometimes input is missed?
- Bug: full drop causes block to go to deep/invisible?

- not fully random?

- Request: show next block chasing
- Request: Show pixelbar logo when standby

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
int oldExbutton = 0;

int gamestate = 0;  // 0 = splash, 1 = playing, 2 = dead
int blockX = 50;
int blockY = 7;

int oldblockX = 0;
int oldblockY = 0;
int oldblockRotation = 0;

int downWaiter = 0;

int gameover[7][4] = { { 0b0011110000111000, 0b1100011011111110, 0b0011111001110011, 0b0111111101111110 },
                       { 0b0110000001111100, 0b1110111011100000, 0b0111001101110011, 0b0111000001110011 },
                       { 0b1110000011100110, 0b1111111011100000, 0b0111001101110011, 0b0111000001110011 },
                       { 0b1110111011100110, 0b1111111011111100, 0b0111001101110011, 0b0111111001110110 },
                       { 0b1110011011111110, 0b1101011011100000, 0b0111001100110110, 0b0111000001111100 },
                       { 0b0110011011100110, 0b1100011011100000, 0b0111001100011100, 0b0111000001110110 },
                       { 0b0011111011100110, 0b1100011011111110, 0b0011111000001000, 0b0111111101110011 } };




// Super Rotation System        bar J L Block S T Z
int blocks[7][4] = { { 0b0000111100000000, 0b0010001000100010, 0b0000000011110000, 0b0100010001000100 },
                     { 0b1000111000000000, 0b0110010001000000, 0b0000111000100000, 0b0100010011000000 },
                     { 0b0010111000000000, 0b0100010001100000, 0b0000111010000000, 0b1100010001000000 },
                     { 0b0110011000000000, 0b0110011000000000, 0b0110011000000000, 0b0110011000000000 },
                     { 0b0110110000000000, 0b0100011000100000, 0b0000011011000000, 0b1000110001000000 },
                     { 0b0100111000000000, 0b0100011001000000, 0b0000111001000000, 0b0100110001000000 },
                     { 0b1100011000000000, 0b0010011001000000, 0b0000110001100000, 0b0100110010000000 } };


int logoBitmap[16][14] = { { 0b11111110, 0b00001111, 0b10000000, 0b00011111, 0b10000111, 0b11000010, 0b00000100, 0b00110000, 0b00011111, 0b01000111, 0b00110111, 0b11000110, 0b01000011, 0b10110011 },
                           { 0b11111110, 0b00001111, 0b10000110, 0b00011111, 0b10000111, 0b11000010, 0b00001100, 0b00100000, 0b00001111, 0b01000111, 0b00110110, 0b00001100, 0b01001011, 0b11000111 },
                           { 0b11111110, 0b00001111, 0b10000111, 0b00011111, 0b10000111, 0b11000010, 0b00011100, 0b00100001, 0b00001111, 0b11000111, 0b00110111, 0b00011000, 0b01100101, 0b11101111 },
                           { 0b11111110, 0b00001111, 0b10000111, 0b10011111, 0b10000111, 0b11000010, 0b00111100, 0b00100011, 0b00001111, 0b11101111, 0b00111101, 0b00011000, 0b11100101, 0b11101111 },
                           { 0b11111110, 0b01001111, 0b10010111, 0b11011111, 0b10010111, 0b11001010, 0b00111100, 0b10100110, 0b01001111, 0b11111111, 0b00111101, 0b00011000, 0b11100101, 0b11101111 },
                           { 0b11111110, 0b10001111, 0b10100111, 0b01111111, 0b10100111, 0b11010010, 0b01111101, 0b00111100, 0b10101111, 0b11111111, 0b00111101, 0b00011001, 0b11000101, 0b11101111 },
                           { 0b11111110, 0b01001111, 0b10010110, 0b01111111, 0b10010111, 0b11001010, 0b11111100, 0b10111001, 0b01001111, 0b11111111, 0b11111111, 0b00011001, 0b11001001, 0b11101111 },
                           { 0b11111110, 0b10001111, 0b10100100, 0b01111111, 0b10100111, 0b11010010, 0b01111101, 0b00110010, 0b10011111, 0b11111111, 0b11111110, 0b10011001, 0b11000011, 0b11010111 },
                           { 0b11111110, 0b01001111, 0b10010100, 0b01111111, 0b10010111, 0b11001010, 0b00011100, 0b10100111, 0b00111111, 0b11111111, 0b11111111, 0b01101101, 0b10100111, 0b11110111 },
                           { 0b11111111, 0b11001111, 0b11110110, 0b01111111, 0b11110111, 0b11111011, 0b11011111, 0b10101110, 0b01111111, 0b11111111, 0b11111111, 0b11010100, 0b10001111, 0b11101111 },
                           { 0b11101111, 0b11001110, 0b11110111, 0b01110111, 0b11110111, 0b01111011, 0b11101111, 0b10111100, 0b11001111, 0b11111111, 0b11111111, 0b11101010, 0b10011111, 0b11111111 },
                           { 0b11110111, 0b11001100, 0b11110111, 0b10011011, 0b11110110, 0b01111011, 0b11101111, 0b10111101, 0b10101111, 0b11111111, 0b11111111, 0b11110110, 0b01111111, 0b11111111 },
                           { 0b11110111, 0b11001110, 0b11110111, 0b01011011, 0b11110111, 0b01111011, 0b11101111, 0b10111110, 0b01101110, 0b11011101, 0b11111111, 0b11111000, 0b01111111, 0b11111111 },
                           { 0b11111011, 0b11001010, 0b11110110, 0b11011101, 0b11110111, 0b01111011, 0b11011111, 0b10101111, 0b11101110, 0b11010101, 0b11111111, 0b11111110, 0b11111111, 0b11111111 },
                           { 0b11100000, 0b00000000, 0b10000100, 0b00010000, 0b10000000, 0b01000010, 0b00011100, 0b00110000, 0b00011100, 0b01001001, 0b11111111, 0b11111101, 0b11111111, 0b11111111 },
                           { 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111101, 0b11111111, 0b11111111 } };













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

void drawBlock() {
  for (int i = 0; i <= 15; i++) {
    if (bitRead(blocks[blockType][blockRotation], i)) { drawDot(i % 4 + blockX, i / 4 + blockY, ON_STATE); }
  }
}

void convertToPlayfield() {
  for (int i = 0; i <= 15; i++) {
    if (bitRead(blocks[blockType][blockRotation], i)) { playfield[i % 4 + blockX][i / 4 + blockY] = 1; }
  }
}

void drawLogo() {
  for (int j = 0; j <= 111; j++) {
    for (int i = 0; i <= 15; i++) {
      if (bitRead(logoBitmap[i][13 - j / 8], j % 8)) { drawDot(j, 15 - i, ON_STATE); }
    }
  }
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
  oldExbutton = 0;
  exbutton = 0;
}

void turnOffOldblock() {

  // turn off old block
  for (int i = 0; i <= 15; i++) {
    if (bitRead(blocks[blockType][oldblockRotation], i)) { drawDot(i % 4 + oldblockX, i / 4 + oldblockY, OFF_STATE); }
  }
}


void switchToDeath() {
  oldExbutton = 0;
  exbutton = 0;
  gamestate = 2;
  clearScreen();
  delay(500);



  for (int j = 0; j <= 63; j++) {
    for (int i = 0; i <= 6; i++) {
      if (bitRead(gameover[i][3 - j / 16], j % 16)) { drawDot(j + 25, i + 4, ON_STATE); }
    }
  }
}

void switchToPlaying() {
  Serial.println("goto playing");
  gamestate = 1;
  oldExbutton = 0;
  exbutton = 0;
  fillScreen();
  delay(500);
  clearScreen();



  for (int j = 111; j >= 0; j--) {  // draw playfield edges
    drawDot(j, PLAYFIELD_TOP, ON_STATE);
    drawDot(j, PLAYFIELD_BOTTOM, ON_STATE);
  }
  drawDot(50, 0, ON_STATE);
  drawDot(50, 1, ON_STATE);
  drawDot(50, 14, ON_STATE);
  drawDot(50, 15, ON_STATE);
  newBlock();

  //   if (0) {  //add lines for debug
  //     //add lines
  //     for (int i = 0; i <= 11; i++) {
  //       for (int j = 0; j <= 4; j++) {
  //         playfield[j][i] = 1;
  //       }
  //     }
  //   playfield[2][5] = 0;

  //     for (int a = 3; a <= 12; a++) {
  //       for (int b = 0; b <= 112; b++) {
  //         if (playfield[b][a] == 0) { drawDot(b, a, OFF_STATE); }
  //         if (playfield[b][a] == 1) { drawDot(b, a, ON_STATE); }
  //       }
  //     }
  //   }
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
  // search and clear lines
  for (int i = 0; i <= 112; i++) {  //searchplayfield for lines
    int lineCount = 0;
    for (int j = 3; j <= 12; j++) {
      if (playfield[i][j] == 1) { lineCount++; }
    }
    if (lineCount == 10) {  // full line detected
      linesCleared++;
      for (int k = 3; k <= 12; k++) {  //animate line clear
        drawDot(i + linesCleared - 1, k, OFF_STATE);
        delay(50);
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

  if (linesCleared) {
    //update playfield
    for (int a = 3; a <= 12; a++) {
      for (int b = 0; b <= 112; b++) {
        if (playfield[b][a] == 0) { drawDot(b, a, OFF_STATE); }
        if (playfield[b][a] == 1) { drawDot(b, a, ON_STATE); }
      }
    }
  }

  blockType = random(7);
  blockRotation = random(3);

  blockX = 50;
  blockY = 7;

  if (detectPlayfieldCollision() == 1) { switchToDeath(); }  // if block collides with playfield, trigger death
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

  return detected;
}

int detectPlayfieldCollisionRotate() {
  int detected = 0;
  for (int i = 0; i <= 15; i++) {

    if (bitRead(blocks[blockType][blockRotation], i)) {
      if (i / 4 + blockY == 2) { detected = 1; }                             // detect top
      if (i / 4 + blockY == 13) { detected = 1; }                            // detect bottom
      if (i % 4 + blockX == -1) { detected = 1; }                            // detect end (right)
      if (playfield[i % 4 + blockX][i / 4 + blockY] == 1) { detected = 1; }  // detect playfield
    }
  }

  return detected;
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

      if (button & BUTTON_LEFT) { exbutton = 5; }   //left      2
      if (button & BUTTON_RIGHT) { exbutton = 4; }  //right  3
      if (button & BUTTON_UP) { exbutton = 2; }     //up  4
      if (button & BUTTON_DOWN) { exbutton = 3; }   //down  5
      if (button & BUTTON_HOME) { exbutton = 6; }

      if (button & BUTTON_A) { exbutton = 1; }
      if (button & BUTTON_ONE) { exbutton = 7; }
      if (button & BUTTON_TWO) { exbutton = 8; }




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
    switch (gamestate) {
      case 1:  //playing
        oldblockX = blockX;
        oldblockY = blockY;
        oldblockRotation = blockRotation;

        if (exbutton > 0) {  //any button pressed
          turnOffOldblock();
          if (exbutton == 3 and oldExbutton != 3) {  //Drop block all the way if right is pressed
            while (detectPlayfieldCollision() == 0) { blockX--; }
            blockX++;
            convertToPlayfield();
            drawBlock();
            newBlock();
          }

          if (exbutton == 8 and oldExbutton != 8) {  // Button one; Rotate clockwise
            blockRotation = (blockRotation + 3) % 4;
            if (detectPlayfieldCollisionRotate() == 1) { blockRotation = (blockRotation + 1) % 4; }
          }

          if (exbutton == 7 and oldExbutton != 7) {  // Button two; Rotate anti-clockwise
            blockRotation = (blockRotation + 1) % 4;
            if (detectPlayfieldCollisionRotate() == 1) { blockRotation = (blockRotation + 3) % 4; }
          }

          if (exbutton == 4 and oldExbutton != 4) {  // Button Up
            blockY--;
            if (detectPlayfieldCollisionSide() == 1) { blockY++; }
          }

          if (exbutton == 5 and oldExbutton != 5) {  // Button Down
            blockY++;
            if (detectPlayfieldCollisionSide() == 1) { blockY--; }
          }
          if (exbutton == 6) { switchToSplash(); }
        }
        if (downWaiter >= 4) {  // Auto move block down one step
          downWaiter = 0;
          blockX--;
          if (detectPlayfieldCollision() == 1) {  // if block collides with playfield, move back up and convert to playfield
            blockX++;
            drawBlock();
            convertToPlayfield();
            newBlock();
          } else turnOffOldblock();
        } else downWaiter++;

        if (gamestate == 1) { drawBlock(); }
        break;

      case 0:  //at splashscreen
        if (exbutton > 0 and exbutton < 6) { switchToPlaying(); }
        break;

      case 2:                                     //player is dead
        if (exbutton == 6) { switchToSplash(); }  // reset from game over
        break;
    }
    oldExbutton = exbutton;
    exbutton = 0;

    delay(5);
    waiter = 0;
  } else waiter++;
}
