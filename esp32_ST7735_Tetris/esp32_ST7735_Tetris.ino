/* 
 *  Controller configuration:
 *  Buttons UP, RIGHT, DOWN, LEFT, and ROTATE1, ROTATE2 are each assigned on characters '8', '6', '2', '4', 'x', 'z' in the both case of SerialPort and WiFi UDP.
 */

const char ssid[] = "ESP32";
const char password[] = "esp32pass";
const int localPort = 10000;

#include "Adafruit_ST7735.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "BlockImage.h"
#include "BackgroundImage.h"
#define TFT_CS     5
#define TFT_RST    16
#define TFT_DC     17

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
uint16_t backBuffer[80][160];
const int Length = 8;     // the number of pixels for a side of a block
const int Width = 10;     // the number of horizontal blocks
const int Height = 20;    // the number of vertical blocks
int screen[Width][Height] = {0};// it shows color-numbers of all positions
WiFiUDP udp;

struct Point {
  int X;
  int Y;
};

struct Block {
  Point square[4][4];
  int numRotate;
  int color;
};

Point pos;
Block block;
int rot;
int fall_cnt = 0;
bool started = false;
bool gameover = false;

Block blocks[7] = {
  {{{{ -1, 0}, {0, 0}, {1, 0}, {2, 0}}, {{0, -1}, {0, 0}, {0, 1}, {0, 2}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 1},
  {{{{0, -1}, {1, -1}, {0, 0}, {1, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 1, 2},
  {{{{ -1, -1}, { -1, 0}, {0, 0}, {1, 0}}, {{ -1, 1}, {0, 1}, {0, 0}, {0, -1}}, {{ -1, 0}, {0, 0}, {1, 0}, {1, 1}}, {{1, -1}, {0, -1}, {0, 0}, {0, 1}}}, 4, 3},
  {{{{ -1, 0}, {0, 0}, {0, 1}, {1, 1}}, {{0, -1}, {0, 0}, { -1, 0}, { -1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 4},
  {{{{ -1, 0}, {0, 0}, {1, 0}, {1, -1}}, {{ -1, -1}, {0, -1}, {0, 0}, {0, 1}}, {{ -1, 1}, { -1, 0}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {0, 1}, {1, 1}}}, 4, 5},
  {{{{ -1, 1}, {0, 1}, {0, 0}, {1, 0}}, {{0, -1}, {0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}, 2, 6},
  {{{{ -1, 0}, {0, 0}, {1, 0}, {0, -1}}, {{0, -1}, {0, 0}, {0, 1}, { -1, 0}}, {{ -1, 0}, {0, 0}, {1, 0}, {0, 1}}, {{0, -1}, {0, 0}, {0, 1}, {1, 0}}}, 4, 7}
};

void Draw() {
  for (int i = 0; i < Width; ++i) for (int j = 0; j < Height; ++j)
      for (int k = 0; k < Length; ++k) for (int l = 0; l < Length; ++l)
          backBuffer[i * Length + k][j * Length + l] = BlockImage[screen[i][j]][k][l];
  tft.fillImage(backBuffer, 0, 24, 160, 80);
}

void PutStartPos() {
  pos.X = 4;
  pos.Y = 1;
  block = blocks[random(7)];
  rot = random(block.numRotate);
}

bool GetSquares(Block block, Point pos, int rot, Point* squares) {
  bool overlap = false;
  for (int i = 0; i < 4; ++i) {
    Point p;
    p.X = pos.X + block.square[rot][i].X;
    p.Y = pos.Y + block.square[rot][i].Y;
    overlap |= p.X < 0 || p.X >= Width || p.Y < 0 || p.Y >= Height || screen[p.X][p.Y] != 0;
    squares[i] = p;
  }
  return !overlap;
}

void GameOver() {
  for (int i = 0; i < Width; ++i) for (int j = 0; j < Height; ++j) if (screen[i][j] != 0) screen[i][j] = 4;
  gameover = true;
}

boolean but_A = false;
boolean but_B = false;
boolean but_UP = false;
boolean but_DOWN = false;
boolean but_LEFT = false;
boolean but_RIGHT = false;

void ClearKeys() {
  but_A=false;
  but_B=false;
  but_UP=false;
  but_DOWN=false;
  but_LEFT=false;
  but_RIGHT=false;
}

bool KeyPadLoop(){
  if (Serial.available()) {
    char r = Serial.read();
    while(Serial.read() != -1);
    if (r == 'z') { ClearKeys();  but_A=true; } //else but_A=false;
    if (r == 'x') { ClearKeys();  but_B=true; }  //else but_B=false;
    if (r == '8') { ClearKeys();  but_UP=true; }  //else but_UP=false;
    if (r == '2') { ClearKeys();  but_DOWN=true; }  //else but_DOWN=false;
    if (r == '4') { ClearKeys();  but_LEFT=true; }  // else but_LEFT=false;
    if (r == '6') { ClearKeys();  but_RIGHT=true; }  //else but_RIGHT=false;
    return true;
  }
  if (udp.parsePacket()) {
    char r = udp.read();
    if (r == 'z') { ClearKeys();  but_A=true; } //else but_A=false;
    if (r == 'x') { ClearKeys();  but_B=true; }  //else but_B=false;
    if (r == '8') { ClearKeys();  but_UP=true; }  //else but_UP=false;
    if (r == '2') { ClearKeys();  but_DOWN=true; }  //else but_DOWN=false;
    if (r == '4') { ClearKeys();  but_LEFT=true; }  // else but_LEFT=false;
    if (r == '6') { ClearKeys();  but_RIGHT=true; }  //else but_RIGHT=false;  
    return true;
  }
  return false;
}

void GetNextPosRot(Point* pnext_pos, int* pnext_rot) {
  bool received = KeyPadLoop();
  if (but_A) started = true;
  if (!started) return;
  pnext_pos->X = pos.X;
  pnext_pos->Y = pos.Y;
  if ((fall_cnt = (fall_cnt + 1) % 10) == 0) pnext_pos->Y += 1;
  else if (received) {
    if (but_LEFT) {
      but_LEFT = false;
      pnext_pos->X -= 1;
    }
    else if (but_RIGHT) {
      but_RIGHT = false;
      pnext_pos->X += 1;
    }
    else if (but_DOWN) {
      but_DOWN = false;
      pnext_pos->Y += 1;
    }
    else if (but_B) {
      but_B = false;
      *pnext_rot = (*pnext_rot + 1)%block.numRotate;
    }
    else if (but_A) {
      but_A = false;
      *pnext_rot = (*pnext_rot + block.numRotate - 1)%block.numRotate; 
    }
  }
}

void DeleteLine() {
  for (int j = 0; j < Height; ++j) {
    bool Delete = true;
    for (int i = 0; i < Width; ++i) if (screen[i][j] == 0) Delete = false;
    if (Delete) for (int k = j; k >= 1; --k) for (int i = 0; i < Width; ++i) screen[i][k] = screen[i][k - 1];
  }
}

void ReviseScreen(Point next_pos, int next_rot) {
  if (!started) return;
  Point next_squares[4];
  for (int i = 0; i < 4; ++i) screen[pos.X + block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = 0;
  if (GetSquares(block, next_pos, next_rot, next_squares)) {
    for (int i = 0; i < 4; ++i) screen[next_squares[i].X][next_squares[i].Y] = block.color;
    pos = next_pos;
    rot = next_rot;
  }
  else {
    for (int i = 0; i < 4; ++i) screen[pos.X + block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
    if (next_pos.Y == pos.Y + 1) {
      DeleteLine();
      PutStartPos();
      if (!GetSquares(block, pos, rot, next_squares)) {
        for (int i = 0; i < 4; ++i) screen[pos.X + block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
        GameOver();
      }
    }
  }
  Draw();
}

void setup(void) {
  WiFi.softAP(ssid, password);
  udp.begin(localPort);
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.fillImage(BackgroundImage, 0, 0, 160, 128);
  PutStartPos();
  for (int i = 0; i < 4; ++i) screen[pos.X + block.square[rot][i].X][pos.Y + block.square[rot][i].Y] = block.color;
  Draw();
}

void loop() {
  if (gameover) return;
  Point next_pos;
  int next_rot = rot;
  GetNextPosRot(&next_pos, &next_rot);
  ReviseScreen(next_pos, next_rot);
}


