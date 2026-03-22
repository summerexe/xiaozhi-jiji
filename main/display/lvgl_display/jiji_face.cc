#include "jiji_face.h"
#include "lvgl.h"
#include <cmath>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>

#define TAG "JijiFace"

JijiFace::JijiFace(lv_obj_t *parent, int width, int height)
    : screenWidth(width), screenHeight(height) {

  // Create canvas for drawing - centered on screen with padding
  canvas_ = lv_canvas_create(parent);
  lv_obj_set_size(canvas_, width, height);
  lv_obj_align(canvas_, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(canvas_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(canvas_, 0, 0);
  lv_obj_set_style_pad_all(canvas_, 0, 0);
  // Make sure canvas is on top layer
  lv_obj_move_foreground(canvas_);

  // Allocate canvas buffer - calculate size manually for RGB565 (2 bytes per
  // pixel)
  size_t buf_size = width * height * sizeof(lv_color_t);
  canvas_buf_ = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM |
                                                             MALLOC_CAP_8BIT);
  if (canvas_buf_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate canvas buffer");
    return;
  }

  // Use LV_COLOR_FORMAT_RGB565 for LVGL 9.x
  lv_canvas_set_buffer(canvas_, canvas_buf_, width, height,
                       LV_COLOR_FORMAT_RGB565);

  // Scale for full 240x240: use 180px reference so face fills the screen
  float scale = (float)width / 180.0f;
  // Wide eyes, shorter height (oval shape)
  eyeLwidthDefault = (int)(86 * scale);
  eyeLheightDefault = (int)(48 * scale);
  eyeLwidthCurrent = eyeLwidthDefault;
  eyeLheightNext = eyeLheightDefault;
  eyeRwidthDefault = eyeLwidthDefault;
  eyeRheightDefault = eyeLheightDefault;
  eyeRwidthCurrent = eyeRwidthDefault;
  eyeRheightNext = eyeRheightDefault;
  // Wider gap between eyes
  spaceBetweenDefault = (int)(44 * scale);
  spaceBetweenCurrent = spaceBetweenDefault;
  spaceBetweenNext =
      spaceBetweenDefault; // ensure interpolation targets the default
  eyeLborderRadiusDefault = (uint8_t)(12 * scale);
  eyeRborderRadiusDefault = eyeLborderRadiusDefault;
  eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
  eyeRborderRadiusCurrent = eyeRborderRadiusDefault;

  // Reduce how far the eyes can travel on the canvas (smaller left/right moves)
  travel_margin_x_ = (int)(screenWidth * 0.28f);
  if (travel_margin_x_ < (int)(12 * scale))
    travel_margin_x_ = (int)(12 * scale);
  travel_margin_y_ = (int)(screenHeight * 0.15f);
  if (travel_margin_y_ < (int)(10 * scale))
    travel_margin_y_ = (int)(10 * scale);

  // Initialize default positions (shift face left by 12px)
  const int face_offset_x = -12;
  eyeLxDefault = ((screenWidth) -
                  (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault)) /
                     2 +
                 face_offset_x;
  eyeLyDefault = ((screenHeight - eyeLheightDefault) / 2);
  eyeLx = eyeLxDefault;
  eyeLy = eyeLyDefault;
  eyeLxNext = eyeLx;
  eyeLyNext = eyeLy;

  eyeRxDefault = eyeLx + eyeLwidthCurrent + spaceBetweenDefault;
  eyeRyDefault = eyeLy;
  eyeRx = eyeRxDefault;
  eyeRy = eyeRyDefault;
  eyeRxNext = eyeRx;
  eyeRyNext = eyeRy;

  // Default frame rate (lower = less frequent redraws, easier on PSRAM/CPU)
  setFramerate(16);

  ESP_LOGI(TAG, "JijiFace initialized: %dx%d @ %d FPS", width, height,
           1000 / frameInterval);
}

JijiFace::~JijiFace() {
  if (canvas_buf_) {
    heap_caps_free(canvas_buf_);
  }
  if (canvas_) {
    lv_obj_del(canvas_);
  }
}

unsigned long JijiFace::millis() {
  return (unsigned long)(esp_timer_get_time() / 1000);
}

void JijiFace::begin(int width, int height, uint8_t frameRate) {
  screenWidth = width;
  screenHeight = height;
  if (canvas_ != nullptr) {
    lv_canvas_fill_bg(canvas_, bg_color_, LV_OPA_COVER);
  }
  eyeLheightCurrent = 1; // start with closed eyes
  eyeRheightCurrent = 1; // start with closed eyes
  setFramerate(frameRate);
}

void JijiFace::update() {
  // Limit drawing updates to defined max framerate
  if (millis() - fpsTimer >= frameInterval) {
    drawEyes();
    fpsTimer = millis();
  }
}

void JijiFace::setFramerate(uint8_t fps) {
  if (fps > 0) {
    frameInterval = 1000 / fps;
  } else {
    frameInterval = 50; // Default to 20 FPS
  }
}

void JijiFace::setDisplayColors(lv_color_t background, lv_color_t main) {
  bg_color_ = background;
  fg_color_ = main;
}

void JijiFace::SetColors(lv_color_t background, lv_color_t main) {
  setDisplayColors(background, main);
}

void JijiFace::setWidth(uint8_t leftEye, uint8_t rightEye) {
  eyeLwidthNext = leftEye;
  eyeRwidthNext = rightEye;
  eyeLwidthDefault = leftEye;
  eyeRwidthDefault = rightEye;
}

void JijiFace::setHeight(uint8_t leftEye, uint8_t rightEye) {
  eyeLheightNext = leftEye;
  eyeRheightNext = rightEye;
  eyeLheightDefault = leftEye;
  eyeRheightDefault = rightEye;
}

void JijiFace::setBorderradius(uint8_t leftEye, uint8_t rightEye) {
  eyeLborderRadiusNext = leftEye;
  eyeRborderRadiusNext = rightEye;
  eyeLborderRadiusDefault = leftEye;
  eyeRborderRadiusDefault = rightEye;
}

void JijiFace::setSpacebetween(int space) {
  spaceBetweenNext = space;
  spaceBetweenDefault = space;
}

void JijiFace::setMood(unsigned char mood) {
  switch (mood) {
  case TIRED:
    tired = 1;
    angry = 0;
    happy = 0;
    break;
  case ANGRY:
    tired = 0;
    angry = 1;
    happy = 0;
    break;
  case HAPPY:
    tired = 0;
    angry = 0;
    happy = 1;
    break;
  default:
    tired = 0;
    angry = 0;
    happy = 0;
    break;
  }
}

void JijiFace::setPosition(unsigned char position) {
  switch (position) {
  case N:
    // North, top center
    eyeLxNext = getScreenConstraint_X() / 2;
    eyeLyNext = 0;
    break;
  case NE:
    // North-east, top right
    eyeLxNext = getScreenConstraint_X();
    eyeLyNext = 0;
    break;
  case E:
    // East, middle right
    eyeLxNext = getScreenConstraint_X();
    eyeLyNext = getScreenConstraint_Y() / 2;
    break;
  case SE:
    // South-east, bottom right
    eyeLxNext = getScreenConstraint_X();
    eyeLyNext = getScreenConstraint_Y();
    break;
  case S:
    // South, bottom center
    eyeLxNext = getScreenConstraint_X() / 2;
    eyeLyNext = getScreenConstraint_Y();
    break;
  case SW:
    // South-west, bottom left
    eyeLxNext = 0;
    eyeLyNext = getScreenConstraint_Y();
    break;
  case W:
    // West, middle left
    eyeLxNext = 0;
    eyeLyNext = getScreenConstraint_Y() / 2;
    break;
  case NW:
    // North-west, top left
    eyeLxNext = 0;
    eyeLyNext = 0;
    break;
  default:
    // Middle center
    eyeLxNext = getScreenConstraint_X() / 2;
    eyeLyNext = getScreenConstraint_Y() / 2;
    break;
  }
}

void JijiFace::setAutoblinker(bool active, int interval, int variation) {
  autoblinker = active;
  blinkInterval = interval;
  blinkIntervalVariation = variation;
}

void JijiFace::setAutoblinker(bool active) { autoblinker = active; }

void JijiFace::setIdleMode(bool active, int interval, int variation) {
  idle = active;
  idleInterval = interval;
  idleIntervalVariation = variation;
}

void JijiFace::setIdleMode(bool active) { idle = active; }

void JijiFace::setCuriosity(bool curiousBit) { curious = curiousBit; }

void JijiFace::setCyclops(bool cyclopsBit) { cyclops = cyclopsBit; }

void JijiFace::setHFlicker(bool flickerBit, uint8_t amplitude) {
  hFlicker = flickerBit;
  hFlickerAmplitude = amplitude;
}

void JijiFace::setHFlicker(bool flickerBit) { hFlicker = flickerBit; }

void JijiFace::setVFlicker(bool flickerBit, uint8_t amplitude) {
  vFlicker = flickerBit;
  vFlickerAmplitude = amplitude;
}

void JijiFace::setVFlicker(bool flickerBit) { vFlicker = flickerBit; }

void JijiFace::setSweat(bool sweatBit) { sweat = sweatBit; }

int JijiFace::getScreenConstraint_X() {
  return screenWidth - eyeLwidthCurrent - spaceBetweenCurrent -
         eyeRwidthCurrent;
}

int JijiFace::getScreenConstraint_Y() {
  return screenHeight - eyeLheightDefault;
}

void JijiFace::close() {
  eyeLheightNext = 1;
  eyeRheightNext = 1;
  eyeL_open = 0;
  eyeR_open = 0;
}

void JijiFace::open() {
  eyeL_open = 1;
  eyeR_open = 1;
}

void JijiFace::blink() {
  close();
  open();
}

void JijiFace::close(bool left, bool right) {
  if (left) {
    eyeLheightNext = 1;
    eyeL_open = 0;
  }
  if (right) {
    eyeRheightNext = 1;
    eyeR_open = 0;
  }
}

void JijiFace::open(bool left, bool right) {
  if (left) {
    eyeL_open = 1;
  }
  if (right) {
    eyeR_open = 1;
  }
}

void JijiFace::blink(bool left, bool right) {
  close(left, right);
  open(left, right);
}

void JijiFace::anim_confused() { confused = 1; }

void JijiFace::anim_laugh() { laugh = 1; }

void JijiFace::fillRoundRect(lv_layer_t *layer, int x, int y, int w, int h,
                             int radius, lv_color_t color) {
  if (canvas_ == nullptr || layer == nullptr)
    return;

  // Ensure valid dimensions
  if (w <= 0 || h <= 0)
    return;

  // Clamp radius to fit within dimensions
  if (radius > w / 2)
    radius = w / 2;
  if (radius > h / 2)
    radius = h / 2;
  if (radius < 0)
    radius = 0;

  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = color;
  dsc.bg_opa = LV_OPA_COVER;
  dsc.border_width = 0;
  dsc.radius = radius;

  // Canvas layer coordinates are 0..w-1 / 0..h-1
  lv_area_t rect_area;
  rect_area.x1 = x;
  rect_area.y1 = y;
  rect_area.x2 = x + w - 1;
  rect_area.y2 = y + h - 1;

  lv_draw_rect(layer, &dsc, &rect_area);
}

void JijiFace::fillTriangle(lv_layer_t *layer, int x1, int y1, int x2, int y2,
                            int x3, int y3, lv_color_t color) {
  if (canvas_ == nullptr || layer == nullptr)
    return;

  lv_draw_triangle_dsc_t tri_dsc;
  lv_draw_triangle_dsc_init(&tri_dsc);
  tri_dsc.color = color;
  tri_dsc.opa = LV_OPA_COVER;

  tri_dsc.p[0].x = x1;
  tri_dsc.p[0].y = y1;
  tri_dsc.p[1].x = x2;
  tri_dsc.p[1].y = y2;
  tri_dsc.p[2].x = x3;
  tri_dsc.p[2].y = y3;

  lv_draw_triangle(layer, &tri_dsc);
}

void JijiFace::drawEyes() {
  //// PRE-CALCULATIONS - EYE SIZES AND VALUES FOR ANIMATION TWEENINGS ////

  // Clamp target positions to reduce travel range (covers setPosition(), idle,
  // and any external target updates).
  auto clamp_targets = [&]() {
    const int min_x = travel_margin_x_;
    int max_x = getScreenConstraint_X() - travel_margin_x_;
    if (max_x < min_x)
      max_x = min_x;
    if (eyeLxNext < min_x)
      eyeLxNext = min_x;
    else if (eyeLxNext > max_x)
      eyeLxNext = max_x;

    const int min_y = travel_margin_y_;
    int max_y = getScreenConstraint_Y() - travel_margin_y_;
    if (max_y < min_y)
      max_y = min_y;
    if (eyeLyNext < min_y)
      eyeLyNext = min_y;
    else if (eyeLyNext > max_y)
      eyeLyNext = max_y;
  };
  clamp_targets();

  // Vertical size offset for larger eyes when looking left or right (curious
  // gaze)
  if (curious) {
    const int min_x = travel_margin_x_;
    int max_x = getScreenConstraint_X() - travel_margin_x_;
    if (max_x < min_x)
      max_x = min_x;

    if (eyeLxNext <= min_x + 10) {
      eyeLheightOffset = 8;
    } else if (eyeLxNext >= (max_x - 10) && cyclops) {
      eyeLheightOffset = 8;
    } else {
      eyeLheightOffset = 0;
    }
    if (eyeRxNext >= screenWidth - eyeRwidthCurrent - 10) {
      eyeRheightOffset = 8;
    } else {
      eyeRheightOffset = 0;
    }
  } else {
    eyeLheightOffset = 0;
    eyeRheightOffset = 0;
  }

  // Left eye height - interpolate smoothly
  int target_height = eyeLheightNext + eyeLheightOffset;
  eyeLheightCurrent = (eyeLheightCurrent + target_height) / 2;

  // Right eye height - interpolate smoothly
  target_height = eyeRheightNext + eyeRheightOffset;
  eyeRheightCurrent = (eyeRheightCurrent + target_height) / 2;

  // Open eyes again after closing them
  if (eyeL_open) {
    if (eyeLheightCurrent <= 2 + eyeLheightOffset) {
      eyeLheightNext = eyeLheightDefault;
    }
  } else {
    // If eyes should be closed, ensure they close
    if (eyeLheightCurrent > 2) {
      eyeLheightNext = 1;
    }
  }
  if (eyeR_open) {
    if (eyeRheightCurrent <= 2 + eyeRheightOffset) {
      eyeRheightNext = eyeRheightDefault;
    }
  } else {
    // If eyes should be closed, ensure they close
    if (eyeRheightCurrent > 2) {
      eyeRheightNext = 1;
    }
  }

  // Left eye width
  eyeLwidthCurrent = (eyeLwidthCurrent + eyeLwidthNext) / 2;
  // Right eye width
  eyeRwidthCurrent = (eyeRwidthCurrent + eyeRwidthNext) / 2;

  // Space between eyes
  spaceBetweenCurrent = (spaceBetweenCurrent + spaceBetweenNext) / 2;

  // Left eye coordinates - interpolate position
  eyeLx = (eyeLx + eyeLxNext) / 2;
  // Calculate Y position: center vertically, adjust for eye height changes
  int eyeL_center_y = eyeLyDefault +
                      ((eyeLheightDefault - eyeLheightCurrent) / 2) -
                      (eyeLheightOffset / 2);
  eyeLy = (eyeLy + eyeL_center_y) / 2;

  // Right eye coordinates
  eyeRxNext = eyeLxNext + eyeLwidthCurrent + spaceBetweenCurrent;
  // Right eye Y should match left eye Y for alignment
  int eyeR_center_y = eyeRyDefault +
                      ((eyeRheightDefault - eyeRheightCurrent) / 2) -
                      (eyeRheightOffset / 2);
  eyeRx = (eyeRx + eyeRxNext) / 2;
  eyeRy = (eyeRy + eyeR_center_y) / 2;

  // Left eye border radius
  eyeLborderRadiusCurrent =
      (eyeLborderRadiusCurrent + eyeLborderRadiusNext) / 2;
  // Right eye border radius
  eyeRborderRadiusCurrent =
      (eyeRborderRadiusCurrent + eyeRborderRadiusNext) / 2;

  //// APPLYING MACRO ANIMATIONS ////

  if (autoblinker) {
    if (millis() >= blinktimer) {
      blink();
      blinktimer = millis() + (blinkInterval * 1000) +
                   (rand() % (blinkIntervalVariation * 1000));
    }
  }

  // Laughing - eyes shaking up and down
  if (laugh) {
    if (laughToggle) {
      setVFlicker(1, 5);
      laughAnimationTimer = millis();
      laughToggle = 0;
    } else if (millis() >= laughAnimationTimer + laughAnimationDuration) {
      setVFlicker(0, 0);
      laughToggle = 1;
      laugh = 0;
    }
  }

  // Confused - eyes shaking left and right
  if (confused) {
    if (confusedToggle) {
      setHFlicker(1, 20);
      confusedAnimationTimer = millis();
      confusedToggle = 0;
    } else if (millis() >= confusedAnimationTimer + confusedAnimationDuration) {
      setHFlicker(0, 0);
      confusedToggle = 1;
      confused = 0;
    }
  }

  // Idle - eyes moving to random positions on screen
  if (idle) {
    if (millis() >= idleAnimationTimer) {
      eyeLxNext = rand() % getScreenConstraint_X();
      eyeLyNext = rand() % getScreenConstraint_Y();
      clamp_targets();
      idleAnimationTimer = millis() + (idleInterval * 1000) +
                           (rand() % (idleIntervalVariation * 1000));
    }
  }

  // Adding offsets for horizontal flickering/shivering
  if (hFlicker) {
    if (hFlickerAlternate) {
      eyeLx += hFlickerAmplitude;
      eyeRx += hFlickerAmplitude;
    } else {
      eyeLx -= hFlickerAmplitude;
      eyeRx -= hFlickerAmplitude;
    }
    hFlickerAlternate = !hFlickerAlternate;
  }

  // Adding offsets for vertical flickering/shivering
  if (vFlicker) {
    if (vFlickerAlternate) {
      eyeLy += vFlickerAmplitude;
      eyeRy += vFlickerAmplitude;
    } else {
      eyeLy -= vFlickerAmplitude;
      eyeRy -= vFlickerAmplitude;
    }
    vFlickerAlternate = !vFlickerAlternate;
  }

  // Cyclops mode, set second eye's size and space between to 0
  if (cyclops) {
    eyeRwidthCurrent = 0;
    eyeRheightCurrent = 0;
    spaceBetweenCurrent = 0;
  }

  //// ACTUAL DRAWINGS ////

  if (canvas_ == nullptr)
    return;

  // Draw everything on a single LVGL canvas layer (fast path in LVGL 9.x)
  lv_layer_t layer;
  lv_canvas_init_layer(canvas_, &layer);

  // Clear background
  lv_draw_rect_dsc_t bg_dsc;
  lv_draw_rect_dsc_init(&bg_dsc);
  bg_dsc.bg_color = bg_color_;
  bg_dsc.bg_opa = LV_OPA_COVER;
  bg_dsc.border_width = 0;
  bg_dsc.radius = 0;
  lv_area_t bg_area = {0, 0, screenWidth - 1, screenHeight - 1};
  lv_draw_rect(&layer, &bg_dsc, &bg_area);

  // Prepare mood type transitions (eyelids)
  if (tired) {
    eyelidsTiredHeightNext = eyeLheightCurrent / 2;
    eyelidsAngryHeightNext = 0;
  } else {
    eyelidsTiredHeightNext = 0;
  }
  if (angry) {
    eyelidsAngryHeightNext = eyeLheightCurrent / 2;
    eyelidsTiredHeightNext = 0;
  } else {
    eyelidsAngryHeightNext = 0;
  }
  // Happy uses curved arcs now (not bottom squint masks)
  eyelidsHappyBottomOffsetNext = 0;

  // Interpolate eyelid values
  eyelidsTiredHeight = (eyelidsTiredHeight + eyelidsTiredHeightNext) / 2;
  eyelidsAngryHeight = (eyelidsAngryHeight + eyelidsAngryHeightNext) / 2;
  eyelidsHappyBottomOffset =
      (eyelidsHappyBottomOffset + eyelidsHappyBottomOffsetNext) / 2;

  // Draw eyes
  if (happy) {
    // Curved "smile" eyes: draw a thick, filled crescent (not a thin line arc)
    auto draw_smile_eye = [&](int x, int y, int w, int h) {
      if (w <= 0 || h <= 0)
        return;

      // Use a square "eye box" and build a crescent by subtracting an inner
      // circle, then masking the bottom.
      int size = w;
      if (size > h)
        size = h;
      if (size < 6)
        return;

      // Place the smile slightly lower within the eye box for a friendlier look
      int box_x = x + (w - size) / 2;
      int box_y = y + (h - size) / 2 + size / 8;

      int thickness = size / 4; // much thicker than before
      if (thickness < 4)
        thickness = 4;
      if (thickness > size / 2 - 1)
        thickness = size / 2 - 1;

      // Outer filled circle (FG)
      fillRoundRect(&layer, box_x, box_y, size, size, size / 2, fg_color_);

      // Inner filled circle (BG), shifted DOWN to leave an upper crescent
      int inner = size - thickness;
      if (inner > 2) {
        fillRoundRect(&layer, box_x + thickness / 2,
                      box_y + thickness + (thickness / 3), inner, inner,
                      inner / 2, bg_color_);
      }

      // Mask the lower half so it doesn't look like a ring
      fillRoundRect(&layer, box_x - 2, box_y + (size / 2), size + 4, size, 0,
                    bg_color_);
    };

    draw_smile_eye(eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent);
    if (!cyclops) {
      draw_smile_eye(eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent);
    }
  } else {
    // Normal eyes (rounded rectangles)
    if (eyeLheightCurrent > 0 && eyeLwidthCurrent > 0) {
      fillRoundRect(&layer, eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent,
                    eyeLborderRadiusCurrent, fg_color_);
    }
    if (!cyclops && eyeRheightCurrent > 0 && eyeRwidthCurrent > 0) {
      fillRoundRect(&layer, eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent,
                    eyeRborderRadiusCurrent, fg_color_);
    }

    // Angry/tired eyelids: use background triangles like the Python reference
    if (tired && eyelidsTiredHeight > 0) {
      const int lid = eyelidsTiredHeight;
      // Left eye: diagonal down to the left
      fillTriangle(&layer, eyeLx, eyeLy - 1, eyeLx + eyeLwidthCurrent,
                   eyeLy - 1, eyeLx, eyeLy + lid - 1, bg_color_);
      if (!cyclops) {
        // Right eye: diagonal down to the right
        fillTriangle(&layer, eyeRx, eyeRy - 1, eyeRx + eyeRwidthCurrent,
                     eyeRy - 1, eyeRx + eyeRwidthCurrent, eyeRy + lid - 1,
                     bg_color_);
      }
    }

    if (angry && eyelidsAngryHeight > 0) {
      const int lid = eyelidsAngryHeight;
      // Left eye: diagonal down to the right
      fillTriangle(&layer, eyeLx, eyeLy - 1, eyeLx + eyeLwidthCurrent,
                   eyeLy - 1, eyeLx + eyeLwidthCurrent, eyeLy + lid - 1,
                   bg_color_);
      if (!cyclops) {
        // Right eye: diagonal down to the left
        fillTriangle(&layer, eyeRx, eyeRy - 1, eyeRx + eyeRwidthCurrent,
                     eyeRy - 1, eyeRx, eyeRy + lid - 1, bg_color_);
      }
    }
  }

  // Cute little "cat nose" between the eyes (always visible)
  {
    // Center the nose between eyes (or under the single eye in cyclops mode)
    int center_x = 0;
    int center_y = 0;
    // Move the nose lower relative to the eyes (larger drop = more space)
    const int nose_drop = eyeLheightCurrent / 3;
    if (cyclops) {
      center_x = eyeLx + (eyeLwidthCurrent / 2);
      center_y = eyeLy + (eyeLheightCurrent / 2) + nose_drop;
    } else {
      center_x = eyeLx + eyeLwidthCurrent + (spaceBetweenCurrent / 2);
      // Below eye centers
      center_y = (eyeLy + eyeRy) / 2 + (eyeLheightCurrent / 2) + nose_drop;
    }

    // Bigger pill nose (wider and taller)
    int nose_w = eyeLwidthCurrent / 2;
    if (nose_w < 12)
      nose_w = 12;
    int nose_h = nose_w / 2;
    if (nose_h < 6)
      nose_h = 6;

    // Clamp inside canvas
    if (center_x - nose_w / 2 < 1)
      center_x = 1 + nose_w / 2;
    if (center_x + nose_w / 2 > screenWidth - 2)
      center_x = screenWidth - 2 - nose_w / 2;
    if (center_y < 1)
      center_y = 1;
    if (center_y + nose_h > screenHeight - 2)
      center_y = screenHeight - 2 - nose_h;

    // Ellipse-like pill nose (rounded rect with radius = half height)
    fillRoundRect(&layer, center_x - nose_w / 2, center_y, nose_w, nose_h,
                  nose_h / 2, fg_color_);
  }

  // Sweat / tear drop (single, eye-relative, teardrop-shaped)
  if (sweat) {
    // Small teardrop size relative to the eye
    int drop_w = eyeLwidthCurrent / 9;
    if (drop_w < 6)
      drop_w = 6;
    int drop_h = (drop_w * 4) / 3;

    // Base position: near the outer corner of the right eye (or the only eye)
    int base_x = 0;
    int base_y = 0;
    if (cyclops) {
      base_x = eyeLx + eyeLwidthCurrent - drop_w;
      base_y = eyeLy + drop_w / 2;
    } else {
      base_x = eyeRx + eyeRwidthCurrent - drop_w;
      base_y = eyeRy + drop_w / 2;
    }

    // Simple falling animation
    if (sweat1YPos <= 18) {
      sweat1YPos += 0.6f;
    } else {
      sweat1YPos = 0;
    }
    int x = base_x;
    int y = base_y + (int)sweat1YPos;

    // Clamp
    if (x < 1)
      x = 1;
    if (x + drop_w > screenWidth - 2)
      x = screenWidth - 2 - drop_w;
    if (y < 1)
      y = 1;
    if (y + drop_h > screenHeight - 2)
      y = screenHeight - 2 - drop_h;

    // Teardrop: round top + pointed bottom
    fillRoundRect(&layer, x, y, drop_w, drop_w, drop_w / 2, fg_color_);
    fillTriangle(&layer, x, y + drop_w / 2, x + drop_w, y + drop_w / 2,
                 x + drop_w / 2, y + drop_h, fg_color_);
  }

  // Flush draw tasks to the canvas and invalidate once
  lv_canvas_finish_layer(canvas_, &layer);
}
