/*
 * JijiFace for LVGL Displays V 1.1.1
 * Draws smoothly animated robot face on LVGL displays, based on LVGL canvas
 * drawing primitives such as rounded rectangles and triangles.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _JIJI_FACE_H
#define _JIJI_FACE_H

#include <cstdint>
#include <cstdlib>
#include <lvgl.h>

// Display colors
#define BGCOLOR 0   // background and overlays
#define MAINCOLOR 1 // drawings

// For mood type switch
#define DEFAULT 0
#define TIRED 1
#define ANGRY 2
#define HAPPY 3

// For turning things on or off
#define ON 1
#define OFF 0

// For switch "predefined positions"
#define N 1  // north, top center
#define NE 2 // north-east, top right
#define E 3  // east, middle right
#define SE 4 // south-east, bottom right
#define S 5  // south, bottom center
#define SW 6 // south-west, bottom left
#define W 7  // west, middle left
#define NW 8 // north-west, top left
// for middle center set "DEFAULT"

class JijiFace {
private:
  // LVGL canvas for drawing
  lv_obj_t *canvas_ = nullptr;
  lv_draw_buf_t *draw_buf_ = nullptr;
  lv_color_t *canvas_buf_ = nullptr;

  // Reference to LVGL display object
  lv_display_t *display_ = nullptr;

  // For general setup - screen size and max. frame rate
  // Default canvas size for 240x240 round display with 20px padding = 200x200
  int screenWidth =
      200; // Canvas width, in pixels (240x240 display with padding)
  int screenHeight =
      200; // Canvas height, in pixels (240x240 display with padding)
  int frameInterval = 80; // default value for 12.5 frames per second (1000/12.5
                          // = 50 milliseconds) - reduced for PSRAM
  unsigned long fpsTimer = 0; // for timing the frames per second

  // For controlling mood types and expressions
  bool tired = 0;
  bool angry = 0;
  bool happy = 0;
  bool curious =
      0; // if true, draw the outer eye larger when looking left or right
  bool cyclops = 0;   // if true, draw only one eye
  bool eyeL_open = 0; // left eye opened or closed?
  bool eyeR_open = 0; // right eye opened or closed?

  // Limit how far the face can travel horizontally/vertically (in px).
  // These are computed from canvas size in the ctor.
  int travel_margin_x_ = 0;
  int travel_margin_y_ = 0;

  //*********************************************************************************************
  //  Eyes Geometry
  //*********************************************************************************************

  // EYE LEFT - size and border radius
  int eyeLwidthDefault = 36;
  int eyeLheightDefault = 36;
  int eyeLwidthCurrent = eyeLwidthDefault;
  int eyeLheightCurrent =
      1; // start with closed eye, otherwise set to eyeLheightDefault
  int eyeLwidthNext = eyeLwidthDefault;
  int eyeLheightNext = eyeLheightDefault;
  int eyeLheightOffset = 0;
  // Border Radius
  uint8_t eyeLborderRadiusDefault = 8;
  uint8_t eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
  uint8_t eyeLborderRadiusNext = eyeLborderRadiusDefault;

  // EYE RIGHT - size and border radius
  int eyeRwidthDefault = eyeLwidthDefault;
  int eyeRheightDefault = eyeLheightDefault;
  int eyeRwidthCurrent = eyeRwidthDefault;
  int eyeRheightCurrent =
      1; // start with closed eye, otherwise set to eyeRheightDefault
  int eyeRwidthNext = eyeRwidthDefault;
  int eyeRheightNext = eyeRheightDefault;
  int eyeRheightOffset = 0;
  // Border Radius
  uint8_t eyeRborderRadiusDefault = 8;
  uint8_t eyeRborderRadiusCurrent = eyeRborderRadiusDefault;
  uint8_t eyeRborderRadiusNext = eyeRborderRadiusDefault;

  // EYE LEFT - Coordinates
  int eyeLxDefault;
  int eyeLyDefault;
  int eyeLx = 0;
  int eyeLy = 0;
  int eyeLxNext = 0;
  int eyeLyNext = 0;

  // EYE RIGHT - Coordinates
  int eyeRxDefault;
  int eyeRyDefault;
  int eyeRx = 0;
  int eyeRy = 0;
  int eyeRxNext = 0;
  int eyeRyNext = 0;

  // BOTH EYES
  // Eyelid top size
  uint8_t eyelidsHeightMax = eyeLheightDefault / 2; // top eyelids max height
  uint8_t eyelidsTiredHeight = 0;
  uint8_t eyelidsTiredHeightNext = eyelidsTiredHeight;
  uint8_t eyelidsAngryHeight = 0;
  uint8_t eyelidsAngryHeightNext = eyelidsAngryHeight;
  // Bottom happy eyelids offset
  uint8_t eyelidsHappyBottomOffsetMax = (eyeLheightDefault / 2) + 3;
  uint8_t eyelidsHappyBottomOffset = 0;
  uint8_t eyelidsHappyBottomOffsetNext = 0;
  // Space between eyes
  int spaceBetweenDefault = 10;
  int spaceBetweenCurrent = spaceBetweenDefault;
  int spaceBetweenNext = 10;

  // Colors
  lv_color_t bg_color_ = LV_COLOR_MAKE(0, 0, 0);
  lv_color_t fg_color_ = LV_COLOR_MAKE(255, 255, 255);

  //*********************************************************************************************
  //  Macro Animations
  //*********************************************************************************************

  // Animation - horizontal flicker/shiver
  bool hFlicker = 0;
  bool hFlickerAlternate = 0;
  uint8_t hFlickerAmplitude = 2;

  // Animation - vertical flicker/shiver
  bool vFlicker = 0;
  bool vFlickerAlternate = 0;
  uint8_t vFlickerAmplitude = 10;

  // Animation - auto blinking
  bool autoblinker = 0;  // activate auto blink animation
  int blinkInterval = 1; // basic interval between each blink in full seconds
  int blinkIntervalVariation =
      4; // interval variaton range in full seconds, random number inside of
         // given range will be add to the basic blinkInterval, set to 0 for no
         // variation
  unsigned long blinktimer = 0; // for organising eyeblink timing

  // Animation - idle mode: eyes looking in random directions
  bool idle = 0;
  int idleInterval =
      1; // basic interval between each eye repositioning in full seconds
  int idleIntervalVariation =
      3; // interval variaton range in full seconds, random number inside of
         // given range will be add to the basic idleInterval, set to 0 for no
         // variation
  unsigned long idleAnimationTimer = 0; // for organising eyeblink timing

  // Animation - eyes confused: eyes shaking left and right
  bool confused = 0;
  unsigned long confusedAnimationTimer = 0;
  int confusedAnimationDuration = 500;
  bool confusedToggle = 1;

  // Animation - eyes laughing: eyes shaking up and down
  bool laugh = 0;
  unsigned long laughAnimationTimer = 0;
  int laughAnimationDuration = 500;
  bool laughToggle = 1;

  // Animation - sweat on the forehead
  bool sweat = 0;
  uint8_t sweatBorderradius = 3;

  // Sweat drop 1
  int sweat1XPosInitial = 2;
  int sweat1XPos;
  float sweat1YPos = 2;
  int sweat1YPosMax;
  float sweat1Height = 2;
  float sweat1Width = 1;

  // Sweat drop 2
  int sweat2XPosInitial = 2;
  int sweat2XPos;
  float sweat2YPos = 2;
  int sweat2YPosMax;
  float sweat2Height = 2;
  float sweat2Width = 1;

  // Sweat drop 3
  int sweat3XPosInitial = 2;
  int sweat3XPos;
  float sweat3YPos = 2;
  int sweat3YPosMax;
  float sweat3Height = 2;
  float sweat3Width = 1;

  // Helper functions for LVGL drawing
  void fillRoundRect(lv_layer_t *layer, int x, int y, int w, int h, int radius,
                     lv_color_t color);
  void fillTriangle(lv_layer_t *layer, int x1, int y1, int x2, int y2, int x3,
                    int y3, lv_color_t color);
  unsigned long millis();

public:
  JijiFace(lv_obj_t *parent, int width, int height);
  ~JijiFace();

  // Startup JijiFace with defined screen-width, screen-height and max. frames
  // per second
  void begin(int width, int height, uint8_t frameRate);

  void update();

  //*********************************************************************************************
  //  SETTERS METHODS
  //*********************************************************************************************

  // Calculate frame interval based on defined frameRate
  void setFramerate(uint8_t fps);

  // Set color values
  void setDisplayColors(lv_color_t background, lv_color_t main);

  void setWidth(uint8_t leftEye, uint8_t rightEye);

  void setHeight(uint8_t leftEye, uint8_t rightEye);

  // Set border radius for left and right eye
  void setBorderradius(uint8_t leftEye, uint8_t rightEye);

  // Set space between the eyes, can also be negative
  void setSpacebetween(int space);

  // Set mood expression
  void setMood(unsigned char mood);

  // Set predefined position
  void setPosition(unsigned char position);

  // Set automated eye blinking, minimal blink interval in full seconds and
  // blink interval variation range in full seconds
  void setAutoblinker(bool active, int interval, int variation);
  void setAutoblinker(bool active);

  // Set idle mode - automated eye repositioning, minimal time interval in full
  // seconds and time interval variation range in full seconds
  void setIdleMode(bool active, int interval, int variation);
  void setIdleMode(bool active);

  // Set curious mode - the respectively outer eye gets larger when looking left
  // or right
  void setCuriosity(bool curiousBit);

  // Set cyclops mode - show only one eye
  void setCyclops(bool cyclopsBit);

  // Set horizontal flickering (displacing eyes left/right)
  void setHFlicker(bool flickerBit, uint8_t amplitude);
  void setHFlicker(bool flickerBit);

  // Set vertical flickering (displacing eyes up/down)
  void setVFlicker(bool flickerBit, uint8_t amplitude);
  void setVFlicker(bool flickerBit);

  void setSweat(bool sweatBit);

  //*********************************************************************************************
  //  GETTERS METHODS
  //*********************************************************************************************

  // Returns the max x position for left eye
  int getScreenConstraint_X();

  // Returns the max y position for left eye
  int getScreenConstraint_Y();

  //*********************************************************************************************
  //  BASIC ANIMATION METHODS
  //*********************************************************************************************

  // BLINKING FOR BOTH EYES AT ONCE
  // Close both eyes
  void close();

  // Open both eyes
  void open();

  // Trigger eyeblink animation
  void blink();

  // BLINKING FOR SINGLE EYES, CONTROL EACH EYE SEPARATELY
  // Close eye(s)
  void close(bool left, bool right);

  // Open eye(s)
  void open(bool left, bool right);

  // Trigger eyeblink(s) animation
  void blink(bool left, bool right);

  //*********************************************************************************************
  //  MACRO ANIMATION METHODS
  //*********************************************************************************************

  // Play confused animation - one shot animation of eyes shaking left and right
  void anim_confused();

  // Play laugh animation - one shot animation of eyes shaking up and down
  void anim_laugh();

  // Get the canvas object
  lv_obj_t *GetCanvas() const { return canvas_; }

  // Set colors (for monochrome displays, use 0 for bg, 1 for fg)
  void SetColors(lv_color_t background, lv_color_t main);

  //*********************************************************************************************
  //  PRE-CALCULATIONS AND ACTUAL DRAWINGS
  //*********************************************************************************************

  void drawEyes();
};

#endif // _JIJI_FACE_H
