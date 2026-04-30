#include "electron_emoji_display.h"

#include <esp_log.h>

#include <cstring>

#include "assets/lang_config.h"
#include "display/lvgl_display/emoji_collection.h"
#include "display/lvgl_display/lvgl_image.h"
#include "display/lvgl_display/lvgl_theme.h"
#include "otto_gif_assets.h"

#define TAG "ElectronEmojiDisplay"
ElectronEmojiDisplay::ElectronEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                                           bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy) {
    InitializeElectronEmojis();
    SetupChatLabel();
}

void ElectronEmojiDisplay::InitializeElectronEmojis() {
    ESP_LOGI(TAG, "初始化Electron GIF表情");

    auto otto_emoji_collection = std::make_shared<EmojiCollection>();

    // 中性/平静类表情 -> staticstate
    AddOttoGifAliases(otto_emoji_collection, OttoStaticStateGif(),
                      {"staticstate", "neutral", "relaxed", "sleepy", "idle"});

    // 积极/开心类表情 -> happy
    AddOttoGifAliases(otto_emoji_collection, OttoHappyGif(),
                      {"happy", "laughing", "funny", "loving", "confident",
                       "winking", "cool", "delicious", "kissy", "silly"});

    // 悲伤类表情 -> sad
    AddOttoGifAliases(otto_emoji_collection, OttoSadGif(), {"sad", "crying"});

    // 愤怒类表情 -> anger
    AddOttoGifAliases(otto_emoji_collection, OttoAngerGif(), {"anger", "angry"});

    // 惊讶类表情 -> scare
    AddOttoGifAliases(otto_emoji_collection, OttoScareGif(), {"scare", "surprised", "shocked"});

    // 思考/困惑类表情 -> buxue
    AddOttoGifAliases(otto_emoji_collection, OttoBuxueGif(),
                      {"buxue", "thinking", "confused", "embarrassed"});

    // 将表情集合添加到主题中
    auto& theme_manager = LvglThemeManager::GetInstance();
    auto light_theme = theme_manager.GetTheme("light");
    auto dark_theme = theme_manager.GetTheme("dark");

    if (light_theme != nullptr) {
        light_theme->set_emoji_collection(otto_emoji_collection);
    }
    if (dark_theme != nullptr) {
        dark_theme->set_emoji_collection(otto_emoji_collection);
    }

    // 设置默认表情为staticstate
    SetEmotion("staticstate");

    ESP_LOGI(TAG, "Electron GIF表情初始化完成");
}

void ElectronEmojiDisplay::SetupChatLabel() {
    DisplayLockGuard lock(this);

    if (chat_message_label_) {
        lv_obj_del(chat_message_label_);
    }

    chat_message_label_ = lv_label_create(container_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, width_ * 0.9);                        // 限制宽度为屏幕宽度的 90%
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);            // 设置为自动换行模式
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);  // 设置文本居中对齐
    lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
    SetTheme(LvglThemeManager::GetInstance().GetTheme("dark"));
}

LV_FONT_DECLARE(OTTO_ICON_FONT);
void ElectronEmojiDisplay::SetStatus(const char* status) {
    auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    auto text_font = lvgl_theme->text_font()->font();
    DisplayLockGuard lock(this);
    if (!status) {
        ESP_LOGE(TAG, "SetStatus: status is nullptr");
        return;
    }

    if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x84\xB0");  // U+F130 麦克风图标
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x80\xA8");  // U+F028 说话图标
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::CONNECTING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x83\x81");  // U+F0c1 连接图标
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        lv_obj_set_style_text_font(status_label_, text_font, 0);
        lv_label_set_text(status_label_, "");
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_set_style_text_font(status_label_, text_font, 0);
    lv_label_set_text(status_label_, status);
    lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
}
