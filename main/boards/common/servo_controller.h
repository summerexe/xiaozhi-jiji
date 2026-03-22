#ifndef __SERVO_CONTROLLER_H__
#define __SERVO_CONTROLLER_H__

#include <cmath>
#include <cstring>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include <esp_timer.h>

// Simple servo controller using LEDC (PWM)
// Supports standard servos (0-180 degrees, 500-2500us pulse width)
// Uses smooth interpolation for natural movement when changing positions.
class ServoController {
private:
  static constexpr const char *kLogTag = "ServoController";
  // NOTE: Backlight PWM in this project uses LEDC (LOW_SPEED, TIMER_0,
  // CHANNEL_0). Servos need a different PWM frequency (50Hz), so we must NOT
  // reuse TIMER_0, otherwise whichever initializes last will reconfigure the
  // timer and break the other PWM output.
  static constexpr ledc_mode_t kSpeedMode = LEDC_LOW_SPEED_MODE;
  static constexpr ledc_timer_t kServoTimer = LEDC_TIMER_1;
  static constexpr ledc_channel_t kServo1Channel = LEDC_CHANNEL_1;
  static constexpr ledc_channel_t kServo2Channel = LEDC_CHANNEL_2;
  static constexpr ledc_timer_bit_t kDutyResolution = LEDC_TIMER_13_BIT;
  static constexpr uint32_t kServoPwmHz = 50; // 50Hz for servos (20ms period)
  static constexpr uint32_t kServoPeriodUs = 20000;
  static constexpr uint32_t kMaxDuty = (1U << 13) - 1; // 13-bit: 0..8191

  // Smooth interpolation: ~25ms tick, lerp 0.12 = gentle ease-out
  static constexpr int kInterpolationIntervalMs = 25;
  static constexpr float kLerpFactor = 0.12f;

  gpio_num_t servo1_gpio_;
  gpio_num_t servo2_gpio_;
  bool servo1_enabled_;
  bool servo2_enabled_;
  ledc_channel_t servo1_channel_;
  ledc_channel_t servo2_channel_;
  int servo1_position_;
  int servo2_position_;
  float servo1_current_;
  float servo2_current_;
  esp_timer_handle_t interpolation_timer_ = nullptr;

  void InitializeTimer() {
    static bool timer_initialized = false;

    if (timer_initialized) {
      return;
    }

    ledc_timer_config_t ledc_timer = {
        .speed_mode = kSpeedMode,
        .duty_resolution = kDutyResolution,
        .timer_num = kServoTimer,
        .freq_hz = kServoPwmHz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    timer_initialized = true;
  }

  ledc_timer_t GetTimer() const { return kServoTimer; }

  // Convert angle (0-180) to duty cycle for servo
  // Servos typically use 500-2500us pulse width (1-2ms is center, 1.5ms =
  // 90deg) For 50Hz (20ms period) with 13-bit resolution (8192 max) duty =
  // (pulse_width_us / 20000) * 8192
  uint32_t AngleToDuty(int angle) {
    if (angle < 0)
      angle = 0;
    if (angle > 180)
      angle = 180;

    // Map 0-180 degrees to 500-2500us (0.5-2.5ms)
    int pulse_width_us = 500 + (angle * 2000 / 180);
    // Convert to duty: (pulse_width_us / period_us) * max_duty
    return (static_cast<uint32_t>(pulse_width_us) * kMaxDuty) / kServoPeriodUs;
  }

  void InitializeChannel(gpio_num_t gpio, ledc_channel_t channel,
                         int initial_position) {
    if (gpio == GPIO_NUM_NC) {
      return;
    }

    const uint32_t duty = AngleToDuty(initial_position);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = gpio,
        .speed_mode = kSpeedMode,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = GetTimer(),
        .duty = duty,
        .hpoint = 0,
        .flags =
            {
                .output_invert = 0,
            },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  }

  void SetServoPosition(gpio_num_t gpio, ledc_channel_t channel, int position) {
    if (gpio == GPIO_NUM_NC) {
      return;
    }
    const uint32_t duty = AngleToDuty(position);
    ESP_ERROR_CHECK(ledc_set_duty(kSpeedMode, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(kSpeedMode, channel));
  }

  static void InterpolationTimerCallback(void *arg) {
    auto *ctrl = static_cast<ServoController *>(arg);
    ctrl->TickInterpolation();
  }

  void TickInterpolation() {
    // Smooth lerp toward target (ease-out for natural deceleration at end)
    if (servo1_enabled_) {
      float diff = servo1_position_ - servo1_current_;
      if (std::abs(diff) > 0.5f) {
        servo1_current_ += diff * kLerpFactor;
        servo1_current_ = std::max(0.0f, std::min(180.0f, servo1_current_));
      } else {
        servo1_current_ = static_cast<float>(servo1_position_);
      }
      SetServoPosition(servo1_gpio_, servo1_channel_,
                      static_cast<int>(std::round(servo1_current_)));
    }
    if (servo2_enabled_) {
      float diff = servo2_position_ - servo2_current_;
      if (std::abs(diff) > 0.5f) {
        servo2_current_ += diff * kLerpFactor;
        servo2_current_ = std::max(0.0f, std::min(180.0f, servo2_current_));
      } else {
        servo2_current_ = static_cast<float>(servo2_position_);
      }
      SetServoPosition(servo2_gpio_, servo2_channel_,
                      static_cast<int>(std::round(servo2_current_)));
    }
  }

public:
  ServoController(gpio_num_t servo1_gpio = GPIO_NUM_NC,
                  gpio_num_t servo2_gpio = GPIO_NUM_NC)
      : servo1_gpio_(servo1_gpio), servo2_gpio_(servo2_gpio),
        servo1_enabled_(servo1_gpio != GPIO_NUM_NC),
        servo2_enabled_(servo2_gpio != GPIO_NUM_NC),
        servo1_channel_(kServo1Channel), servo2_channel_(kServo2Channel),
        servo1_position_(90), servo2_position_(90),
        servo1_current_(90.0f), servo2_current_(90.0f) {

    if (!servo1_enabled_ && !servo2_enabled_) {
      ESP_LOGW(kLogTag, "No servo GPIO pins configured");
      return;
    }

    InitializeTimer();

    // Initialize servo 1
    if (servo1_enabled_) {
      InitializeChannel(servo1_gpio_, servo1_channel_, 90); // Center position
      ESP_LOGI(kLogTag, "Servo 1 initialized on GPIO %d", servo1_gpio_);
    }

    // Initialize servo 2
    if (servo2_enabled_) {
      InitializeChannel(servo2_gpio_, servo2_channel_, 90); // Center position
      ESP_LOGI(kLogTag, "Servo 2 initialized on GPIO %d", servo2_gpio_);
    }

    // Start interpolation timer for smooth movement
    esp_timer_create_args_t args = {};
    args.callback = &InterpolationTimerCallback;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "servo_interp";
    if (esp_timer_create(&args, &interpolation_timer_) == ESP_OK) {
      esp_timer_start_periodic(interpolation_timer_,
                              kInterpolationIntervalMs * 1000);
    }
  }

  // Set target position for servo 1 (0-180 degrees). Movement is interpolated.
  void SetServo1(int angle) {
    if (!servo1_enabled_) {
      return;
    }
    servo1_position_ = (angle < 0) ? 0 : (angle > 180 ? 180 : angle);
  }

  // Set target position for servo 2 (0-180 degrees). Movement is interpolated.
  void SetServo2(int angle) {
    if (!servo2_enabled_) {
      return;
    }
    servo2_position_ = (angle < 0) ? 0 : (angle > 180 ? 180 : angle);
  }

  // Set both servos to the same position
  void SetBoth(int angle) {
    SetServo1(angle);
    SetServo2(angle);
  }

  // Set positions independently
  void SetPositions(int servo1_angle, int servo2_angle) {
    SetServo1(servo1_angle);
    SetServo2(servo2_angle);
  }

  // Set positions instantly (no interpolation) - for quick transitions e.g.
  // touch release
  void SetPositionsInstant(int servo1_angle, int servo2_angle) {
    if (servo1_enabled_) {
      servo1_position_ = (servo1_angle < 0) ? 0 : (servo1_angle > 180 ? 180 : servo1_angle);
      servo1_current_ = static_cast<float>(servo1_position_);
      SetServoPosition(servo1_gpio_, servo1_channel_, servo1_position_);
    }
    if (servo2_enabled_) {
      servo2_position_ = (servo2_angle < 0) ? 0 : (servo2_angle > 180 ? 180 : servo2_angle);
      servo2_current_ = static_cast<float>(servo2_position_);
      SetServoPosition(servo2_gpio_, servo2_channel_, servo2_position_);
    }
  }

  // Set emotion-based positions
  // This maps emotions to servo positions (angles)
  void SetEmotion(const char *emotion) {
    auto is = [&](const char *name) { return strcmp(emotion, name) == 0; };

    // Default neutral position
    int servo1_pos = 90;
    int servo2_pos = 90;

    if (is("happy") || is("joy") || is("laughing")) {
      // Happy: slight upward movement (like a smile)
      servo1_pos = 120;
      servo2_pos = 120;
    } else if (is("sad") || is("crying")) {
      // Sad: downward movement
      servo1_pos = 60;
      servo2_pos = 60;
    } else if (is("angry")) {
      // Angry: extreme positions (like frowning)
      servo1_pos = 150;
      servo2_pos = 30;
    } else if (is("surprised") || is("shocked")) {
      // Surprised: wide positions
      servo1_pos = 45;
      servo2_pos = 135;
    } else if (is("thinking") || is("confused")) {
      // Thinking: slight offset positions
      servo1_pos = 100;
      servo2_pos = 80;
    } else if (is("winking")) {
      // Winking: one up, one down
      servo1_pos = 120;
      servo2_pos = 90;
    } else {
      // Neutral/idle: center position
      servo1_pos = 90;
      servo2_pos = 90;
    }

    SetPositions(servo1_pos, servo2_pos);
  }

  // Set emotion positions instantly (no interpolation)
  void SetEmotionInstant(const char *emotion) {
    auto is = [&](const char *name) { return strcmp(emotion, name) == 0; };
    int servo1_pos = 90;
    int servo2_pos = 90;
    if (is("happy") || is("joy") || is("laughing")) {
      servo1_pos = 120;
      servo2_pos = 120;
    } else if (is("sad") || is("crying")) {
      servo1_pos = 60;
      servo2_pos = 60;
    } else if (is("angry")) {
      servo1_pos = 150;
      servo2_pos = 30;
    } else if (is("surprised") || is("shocked")) {
      servo1_pos = 45;
      servo2_pos = 135;
    } else if (is("thinking") || is("confused")) {
      servo1_pos = 100;
      servo2_pos = 80;
    } else if (is("winking")) {
      servo1_pos = 120;
      servo2_pos = 90;
    }
    SetPositionsInstant(servo1_pos, servo2_pos);
  }

  int GetServo1Position() const { return servo1_position_; }
  int GetServo2Position() const { return servo2_position_; }
};

#endif // __SERVO_CONTROLLER_H__