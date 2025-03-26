#include <Arduino.h>
#include <FS.h>
#include <SD.h>
//Users/Jacob/Library/Arduino15/packages/esp32/hardware/esp32/3.0.7/libraries/SD/src/
//#include <FFat.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include <stdlib.h>
#include <stdio.h>
#include <TouchScreen.h>

#define SD_CS 5
//#define SD FFat
#define YP 4   // must be an analog pin, use "An" notation!
#define XM 15  // must be an analog pin, use "An" notation!
#define YM 14  // can be a digital pin
#define XP 27  // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 150);

#define MINPRESSURE 1
#define MAXPRESSURE 6000
#define TS_MINY 750
#define TS_MINX 540
#define TS_MAXY -2275
#define TS_MAXX -2420


#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY 0x5ACB
#define DGREY 0x4A49

#define r2d 180 / 3.1415

TFT_eSPI tft = TFT_eSPI();

struct Vector3 {
  float x, y, z;
  Vector3(float x, float y, float z)
    : x(x), y(y), z(z) {}
};
struct Triangle {
  Vector3 a, b, c;
  uint16_t Colour;
  Triangle()
    : a(Vector3(0, 0, 0)), b(Vector3(0, 0, 0)), c(Vector3(0, 0, 0)), Colour(MAGENTA) {}
  Triangle(Vector3 a, Vector3 b, Vector3 c)
    : a(a), b(b), c(c), Colour(MAGENTA) {}
  Triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
    : a(Vector3(ax, ay, az)), b(Vector3(bx, by, bz)), c(Vector3(cx, cy, cz)), Colour(MAGENTA) {}
  Triangle(Vector3 a, Vector3 b, Vector3 c, uint16_t Colour)
    : a(a), b(b), c(c), Colour(Colour) {}
  Triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, uint16_t Colour)
    : a(Vector3(ax, ay, az)), b(Vector3(bx, by, bz)), c(Vector3(cx, cy, cz)), Colour(Colour) {}
};
struct Vector2 {
  float x, y;
  Vector2(float x, float y)
    : x(x), y(y) {}
};
struct Triangle2D {
  Vector2 a, b, c;
  Triangle2D(Vector2 a, Vector2 b, Vector2 c)
    : a(a), b(b), c(c) {}
  Triangle2D(float ax, float ay, float bx, float by, float cx, float cy)
    : a(Vector2(ax, ay)), b(Vector2(bx, by)), c(Vector2(cx, cy)) {}
};
struct Vertex {
  Vector3 pos;
  uint16_t colour;
  Vertex()
    : pos(Vector3(0, 0, 0)), colour(MAGENTA) {}
  Vertex(float x, float y, float z, uint16_t col)
    : pos(Vector3(x, y, z)), colour(col) {}
};
struct Mesh {
  struct Triangle triangles[1024];
};
struct sortItem {
  int index;
  int value;
};
Vector3 Pos = Vector3(0, 100, -290);
float xDir = -53;
float yDir = 15;
File file;




TSPoint waitOneTouch() {
  TSPoint p;
  do {
    p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    delay(1);
  } while (abs(p.z) < MINPRESSURE || abs(p.z) > MAXPRESSURE);
  int y = map(p.x, TS_MINX, TS_MAXX, 0, 240);
  p.x = map(p.y, TS_MINY, TS_MAXY, 0, 320);
  p.y = y;
  return p;
}



/*inline float atan2(float a, float b) {
    if (b > 0) return atan(a / b) * r2d;
    if (b < 0 && a >= 0) return (atan(a / b) * r2d) + 180;
    if (b < 0 && a < 0) return (atan(a / b) * r2d) - 180;
    if (b == 0) return (a > 0) ? 90 : (a < 0 ? -90 : 0);  // Handle edge cases for b == 0
    return 0;  // Default return to handle unexpected cases
}*/
inline float Length(float a, float b) {
  return sqrt((a * a) + (b * b));
}
double normalizeAngle(double angle) {
  while (angle > 180) angle -= 360;
  while (angle <= -180) angle += 360;
  return angle;
}

inline Vector2 convert(Vector3 p) {
  //Serial.println((atan2(p.z, p.x)));
  //Serial.println(atan2(p.y,Length(p.x, p.z)));
  return Vector2{
    ((normalizeAngle(((atan2(p.z - Pos.z, p.x - Pos.x) * r2d) + xDir)) / 30) * 100) + 50,
    ((normalizeAngle(((atan2(p.y - Pos.y, Length(p.x - Pos.x, p.z - Pos.z)) * r2d) + yDir)) / 30) * 100) + 50
  };
}
void drawTriangle(Triangle tri) {

  Triangle2D projection = Triangle2D(Vector2(0, 0), Vector2(0, 0), Vector2(0, 0));
  projection.a = convert(tri.a);
  projection.b = convert(tri.b);
  projection.c = convert(tri.c);
  //tft.drawTriangle(projection.a.x, projection.a.y, projection.b.x, projection.b.y, projection.c.x, projection.c.y, tri.Colour);
  tft.fillTriangle(projection.a.x, projection.a.y, projection.b.x, projection.b.y, projection.c.x, projection.c.y, tri.Colour);
}
sortItem dists[1000];
Triangle Mesh[1000];
void drawMesh(File& file) {
  int i = 0;
  for (int i = 0; i < 1000; i++) {
    Triangle tri = Mesh[i];
    dists[i].index = i;
    dists[i].value = dist(Triangle(
      tri.a.x - Pos.x, tri.a.y - Pos.y, tri.a.z - Pos.z,
      tri.b.x - Pos.x, tri.b.y - Pos.y, tri.b.z - Pos.z,
      tri.c.x - Pos.x, tri.c.y - Pos.y, tri.c.z - Pos.z));
  }
  sort(dists);
  tft.fillScreen(CYAN);
  for (int i = 0; i < 1000; i++) {
    drawTriangle(Mesh[dists[i].index]);
    //Serial.println(dists[i].value);
  }
}
/*void drawMesh(String name) {
  file = SD.open("/" + name + ".txt", FILE_READ);

  int size = readInt();
  if (size > 1000) {
    Serial.print("File too large, ");
    Serial.print(size);
    Serial.println(" Triangles, Max is 256");
    return;
  }
  Serial.print(size);
  sort(size);
  //file.seek(0);
  //readInt(file);
  //readInt();
  //Serial.println(file.curPosition());
  //for(int i=0;i<size;i++){drawTriangle(readTri(byteIndex[i].index));Serial.println(file.curPosition());/*delay(500);*HERE}
}*/
float readInt() {
  float out = file.readStringUntil('\n').toFloat();
  //Serial.println(out);
  return out;
}
Triangle readTri(int pos = 0) {
  file.seek(25 + (pos * 250));
  Triangle tri = Triangle(
    readInt(), readInt(), readInt(),
    readInt(), readInt(), readInt(),
    readInt(), readInt(), readInt());
  tri.Colour = readInt();
  /*Serial.println(tri.a.x);
  Serial.println(tri.a.y);
  Serial.println(tri.a.z);
  Serial.println();
  Serial.println(tri.b.x);
  Serial.println(tri.b.y);
  Serial.println(tri.b.z);
  Serial.println();
  Serial.println(tri.c.x);
  Serial.println(tri.c.y);
  Serial.println(tri.c.z);*/
  return tri;
}

//int values[256];
//int dists[1000];
int compareDescending(const void* a, const void* b) {
  const sortItem* elemA = (const sortItem*)a;
  const sortItem* elemB = (const sortItem*)b;
  return elemB->value - elemA->value;  // Descending order
}
void sort(sortItem items[]) {
  Serial.println("Begin Sort:");
  qsort(items, 1000, sizeof(sortItem), compareDescending);
  /*Serial.println("Read:");
  for (int i = 0; i < size; i++) {
    Triangle tri = readTri(i);
    dists[i] = dist(Triangle(
      Vector3(tri.a.x - Pos.x, tri.a.y - Pos.y, tri.a.z - Pos.z),
      Vector3(tri.b.x - Pos.x, tri.b.y - Pos.y, tri.b.z - Pos.z),
      Vector3(tri.c.x - Pos.x, tri.c.y - Pos.y, tri.c.z - Pos.z)));
  }
  Serial.println("Sort:");
  for (int i = 0; i < size; i++) {
    float maxDist = -999998;
    int maxIndex = 0;
    for (int j = 0; j < size; j++) {
      float test = dists[j];
      if (test > maxDist) {
        maxDist = test;
        maxIndex = j;
      }
    }
    //Serial.println("Draw:");
    //draw it
    Serial.println(dists[maxIndex] / 1000);
    dists[maxIndex] = -999999;
    Triangle tri = readTri(maxIndex);
    drawTriangle(Triangle(
      Vector3(tri.a.x - Pos.x, tri.a.y - Pos.y, tri.a.z - Pos.z),
      Vector3(tri.b.x - Pos.x, tri.b.y - Pos.y, tri.b.z - Pos.z),
      Vector3(tri.c.x - Pos.x, tri.c.y - Pos.y, tri.c.z - Pos.z),
      tri.Colour)
      );
    //delay(100);
  }*/
  Serial.println("Done");
}

int dist(Triangle tri) {
  /*Serial.println(tri.a.x);
  Serial.println(tri.a.y);
  Serial.println(tri.a.z);
  Serial.println();
  Serial.println(tri.b.x);
  Serial.println(tri.b.y);
  Serial.println(tri.b.z);
  Serial.println();
  Serial.println(tri.c.x);
  Serial.println(tri.c.y);
  Serial.println(tri.c.z);*/
  // Calculate the centroid of the triangle
  float centroidX = (tri.a.x + tri.b.x + tri.c.x) / 3.0;
  float centroidY = (tri.a.y + tri.b.y + tri.c.y) / 3.0;
  float centroidZ = (tri.a.z + tri.b.z + tri.c.z) / 3.0;
  //Serial.println(centroidX);
  //Serial.println(centroidY);
  //Serial.println(centroidZ);
  //Serial.println();
  // Calculate the distance from the camera (Pos) to the centroid
  float distance = sqrt(
    pow(centroidX, 2) + pow(centroidY, 2) + pow(centroidZ, 2));

  // Return an integer representation of the distance (scaled if necessary)
  return (int)(distance * 1000);
}


File f;
void setup() {
  Serial.begin(115200);
  tft.init();  // SDFP5408.  0x9341
  tft.setRotation(3);
  tft.fillScreen(CYAN);
  Serial.println(SD.begin(SD_CS));
  /*Serial.println(Pos.x);
  Serial.println(Pos.y);
  Serial.println(Pos.z);
  drawMesh("Elephant");*/
  //File file =
  f = SD.open("/World2.txt");
  f.seek(0);
  int i = 0;

  while (f.available()) {
    Triangle tri;
    tri.a = Vector3(
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil('\n').toFloat());
    tri.b = Vector3(
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil('\n').toFloat());
    tri.c = Vector3(
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil('\n').toFloat());
    tri.Colour = tft.color565(
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil(' ').toFloat(),
      f.readStringUntil('\n').toFloat());
    Mesh[i] = tri;
    i++;
  }
  drawMesh(f);
}

void loop() {
  /*Serial.println("X:");
  while (!Serial.available()) {}
  Pos.x = Serial.readString().toFloat();
  Serial.println("Y:");
  while (!Serial.available()) {}
  Pos.y = Serial.readString().toFloat();
  Serial.println("Z:");
  while (!Serial.available()) {}
  Pos.z = Serial.readString().toFloat();
  Serial.println("X Dir:");
  while (!Serial.available()) {}
  xDir = Serial.readString().toFloat();
  Serial.println(Pos.x);
  Serial.println(Pos.y);
  Serial.println(Pos.z);*/

  drawMesh(f);
  delay(100);
  Pos.z += 10;
}
