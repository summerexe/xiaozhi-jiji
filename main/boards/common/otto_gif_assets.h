#ifndef OTTO_GIF_ASSETS_H
#define OTTO_GIF_ASSETS_H

#include "display/lvgl_display/emoji_collection.h"
#include "display/lvgl_display/lvgl_image.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

struct OttoGifAsset {
    const uint8_t* data;
    size_t data_size;
};

extern const uint8_t otto_staticstate_gif_start[] asm("_binary_staticstate_gif_start");
extern const uint8_t otto_staticstate_gif_end[] asm("_binary_staticstate_gif_end");
extern const uint8_t otto_happy_gif_start[] asm("_binary_happy_gif_start");
extern const uint8_t otto_happy_gif_end[] asm("_binary_happy_gif_end");
extern const uint8_t otto_sad_gif_start[] asm("_binary_sad_gif_start");
extern const uint8_t otto_sad_gif_end[] asm("_binary_sad_gif_end");
extern const uint8_t otto_anger_gif_start[] asm("_binary_anger_gif_start");
extern const uint8_t otto_anger_gif_end[] asm("_binary_anger_gif_end");
extern const uint8_t otto_scare_gif_start[] asm("_binary_scare_gif_start");
extern const uint8_t otto_scare_gif_end[] asm("_binary_scare_gif_end");
extern const uint8_t otto_buxue_gif_start[] asm("_binary_buxue_gif_start");
extern const uint8_t otto_buxue_gif_end[] asm("_binary_buxue_gif_end");

inline OttoGifAsset OttoStaticStateGif() {
    return {otto_staticstate_gif_start,
            static_cast<size_t>(otto_staticstate_gif_end - otto_staticstate_gif_start)};
}

inline OttoGifAsset OttoHappyGif() {
    return {otto_happy_gif_start,
            static_cast<size_t>(otto_happy_gif_end - otto_happy_gif_start)};
}

inline OttoGifAsset OttoSadGif() {
    return {otto_sad_gif_start,
            static_cast<size_t>(otto_sad_gif_end - otto_sad_gif_start)};
}

inline OttoGifAsset OttoAngerGif() {
    return {otto_anger_gif_start,
            static_cast<size_t>(otto_anger_gif_end - otto_anger_gif_start)};
}

inline OttoGifAsset OttoScareGif() {
    return {otto_scare_gif_start,
            static_cast<size_t>(otto_scare_gif_end - otto_scare_gif_start)};
}

inline OttoGifAsset OttoBuxueGif() {
    return {otto_buxue_gif_start,
            static_cast<size_t>(otto_buxue_gif_end - otto_buxue_gif_start)};
}

inline void AddOttoGifAliases(const std::shared_ptr<EmojiCollection>& collection,
                              OttoGifAsset asset,
                              std::initializer_list<const char*> names) {
    for (const char* name : names) {
        collection->AddEmoji(name, new LvglRawImage(const_cast<uint8_t*>(asset.data), asset.data_size));
    }
}

#endif // OTTO_GIF_ASSETS_H
