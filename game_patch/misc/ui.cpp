#include <cstring>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include "../rf/ui.h"
#include "../rf/input.h"
#include "../rf/os.h"

#define DEBUG_UI_LAYOUT 0
#define SHARP_UI_TEXT 1

static inline void debug_ui_layout([[ maybe_unused ]] rf::UiGadget& gadget)
{
#if DEBUG_UI_LAYOUT
    int x = gadget.GetAbsoluteX() * ui_scale_x;
    int y = gadget.GetAbsoluteY() * ui_scale_y;
    int w = gadget.w * ui_scale_x;
    int h = gadget.h * ui_scale_y;
    gr_set_color((x ^ y) & 255, 0, 0, 64);
    gr_rect(x, y, w, h);
#endif
}

void __fastcall UiButton_create(rf::UiButton& this_, int, const char *normal_bm, const char *selected_bm, int x, int y, int id, const char *text, int font)
{
    this_.key = id;
    this_.x = x;
    this_.y = y;
    if (*normal_bm) {
        this_.bg_bitmap = rf::bm_load(normal_bm, -1, false);
        rf::gr_tcache_add_ref(this_.bg_bitmap);
        rf::bm_get_dimensions(this_.bg_bitmap, &this_.w, &this_.h);
    }
    if (*selected_bm) {
        this_.selected_bitmap = rf::bm_load(selected_bm, -1, false);
        rf::gr_tcache_add_ref(this_.selected_bitmap);
        if (this_.bg_bitmap < 0) {
            rf::bm_get_dimensions(this_.selected_bitmap, &this_.w, &this_.h);
        }
    }
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiButton_create_hook{0x004574D0, UiButton_create};

void __fastcall UiButton_set_text(rf::UiButton& this_, int, const char *text, int font)
{
    delete[] this_.text;
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiButton_set_text_hook{0x00457710, UiButton_set_text};

void __fastcall UiButton_render(rf::UiButton& this_)
{
    int x = static_cast<int>(this_.get_absolute_x() * rf::ui_scale_x);
    int y = static_cast<int>(this_.get_absolute_y() * rf::ui_scale_y);
    int w = static_cast<int>(this_.w * rf::ui_scale_x);
    int h = static_cast<int>(this_.h * rf::ui_scale_y);

    if (this_.bg_bitmap >= 0) {
        rf::gr_set_color(255, 255, 255, 255);
        rf::gr_bitmap_stretched(this_.bg_bitmap, x, y, w, h, 0, 0, this_.w, this_.h);
    }

    if (!this_.enabled) {
        rf::gr_set_color(96, 96, 96, 255);
    }
    else if (this_.highlighted) {
        rf::gr_set_color(240, 240, 240, 255);
    }
    else {
        rf::gr_set_color(192, 192, 192, 255);
    }

    if (this_.enabled && this_.highlighted && this_.selected_bitmap >= 0) {
        auto state = addr_as_ref<rf::GrMode>(0x01775B0C);
        rf::gr_bitmap_stretched(this_.selected_bitmap, x, y, w, h, 0, 0, this_.w, this_.h, false, false, state);
    }

    // Change clip region for text rendering
    int clip_x, clip_y, clip_w, clip_h;
    rf::gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    rf::gr_set_clip(x, y, w, h);

    std::string_view text_sv{this_.text};
    int num_lines = 1 + std::count(text_sv.begin(), text_sv.end(), '\n');
    int text_h = rf::gr_get_font_height(this_.font) * num_lines;
    int text_y = (h - text_h) / 2;
    rf::gr_string(rf::center_x, text_y, this_.text, this_.font);

    // Restore clip region
    rf::gr_set_clip(clip_x, clip_y, clip_w, clip_h);

    debug_ui_layout(this_);
}
FunHook UiButton_render_hook{0x004577A0, UiButton_render};

void __fastcall UiLabel_create(rf::UiLabel& this_, int, rf::UiGadget *parent, int x, int y, const char *text, int font)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    int text_w, text_h;
    rf::gr_get_string_size(&text_w, &text_h, text, -1, font);
    this_.w = static_cast<int>(text_w / rf::ui_scale_x);
    this_.h = static_cast<int>(text_h / rf::ui_scale_y);
    this_.text = strdup(text);
    this_.font = font;
    this_.align = rf::GR_ALIGN_LEFT;
    this_.clr.set(0, 0, 0, 255);
}
FunHook UiLabel_create_hook{0x00456B60, UiLabel_create};

void __fastcall UiLabel_create2(rf::UiLabel& this_, int, rf::UiGadget *parent, int x, int y, int w, int h, const char *text, int font)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    this_.w = w;
    this_.h = h;
    if (*text == ' ') {
        while (*text == ' ') {
            ++text;
        }
        this_.align = rf::GR_ALIGN_CENTER;
    }
    else {
        this_.align = rf::GR_ALIGN_LEFT;
    }
    this_.text = strdup(text);
    this_.font = font;
    this_.clr.set(0, 0, 0, 255);
}
FunHook UiLabel_create2_hook{0x00456C20, UiLabel_create2};

void __fastcall UiLabel_set_text(rf::UiLabel& this_, int, const char *text, int font)
{
    delete[] this_.text;
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiLabel_set_text_hook{0x00456DC0, UiLabel_set_text};

void __fastcall UiLabel_render(rf::UiLabel& this_)
{
    if (!this_.enabled) {
        rf::gr_set_color(48, 48, 48, 128);
    }
    else if (this_.highlighted) {
        rf::gr_set_color(240, 240, 240, 255);
    }
    else {
        gr_set_color(this_.clr);
    }
    int x = static_cast<int>(this_.get_absolute_x() * rf::ui_scale_x);
    int y = static_cast<int>(this_.get_absolute_y() * rf::ui_scale_y);
    int text_w, text_h;
    rf::gr_get_string_size(&text_w, &text_h, this_.text, -1, this_.font);
    if (this_.align == rf::GR_ALIGN_CENTER) {
        x += static_cast<int>(this_.w * rf::ui_scale_x / 2);
    }
    else if (this_.align == rf::GR_ALIGN_RIGHT) {
        x += static_cast<int>(this_.w * rf::ui_scale_x);
    }
    else {
        x += static_cast<int>(1 * rf::ui_scale_x);
    }
    gr_string_aligned(this_.align, x, y, this_.text, this_.font);

    debug_ui_layout(this_);
}
FunHook UiLabel_render_hook{0x00456ED0, UiLabel_render};

void __fastcall UiInputBox_create(rf::UiInputBox& this_, int, rf::UiGadget *parent, int x, int y, const char *text, int font, int w)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    this_.w = w;
    this_.h = static_cast<int>(rf::gr_get_font_height(font) / rf::ui_scale_y);
    this_.max_text_width = static_cast<int>(w * rf::ui_scale_x);
    this_.font = font;
    std::strncpy(this_.text, text, std::size(this_.text));
    this_.text[std::size(this_.text) - 1] = '\0';
}
FunHook UiInputBox_create_hook{0x00456FE0, UiInputBox_create};

void __fastcall UiInputBox_render(rf::UiInputBox& this_, void*)
{
    if (this_.enabled && this_.highlighted) {
        rf::gr_set_color(240, 240, 240, 255);
    }
    else {
        rf::gr_set_color(192, 192, 192, 255);
    }

    int x = static_cast<int>((this_.get_absolute_x() + 1) * rf::ui_scale_x);
    int y = static_cast<int>(this_.get_absolute_y() * rf::ui_scale_y);
    int clip_x, clip_y, clip_w, clip_h;
    rf::gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    rf::gr_set_clip(x, y, this_.max_text_width, static_cast<int>(this_.h * rf::ui_scale_y + 5)); // for some reason input fields are too thin
    int text_offset_x = static_cast<int>(1 * rf::ui_scale_x);
    rf::gr_string(text_offset_x, 0, this_.text, this_.font);

    if (this_.enabled && this_.highlighted) {
        rf::ui_update_input_box_cursor();
        if (rf::ui_input_box_cursor_visible) {
            int text_w, text_h;
            rf::gr_get_string_size(&text_w, &text_h, this_.text, -1, this_.font);
            rf::gr_string(text_offset_x + text_w, 0, "_", this_.font);
        }
    }
    rf::gr_set_clip(clip_x, clip_y, clip_w, clip_h);

    debug_ui_layout(this_);
}
FunHook UiInputBox_render_hook{0x004570E0, UiInputBox_render};

void __fastcall UiCycler_add_item(rf::UiCycler& this_, int, const char *text, int font)
{
    if (this_.num_items < this_.max_items) {
        this_.items_text[this_.num_items] = strdup(text);
        this_.items_font[this_.num_items] = font;
        ++this_.num_items;
    }
}
FunHook UiCycler_add_item_hook{0x00458080, UiCycler_add_item};

void __fastcall UiCycler_render(rf::UiCycler& this_)
{
    if (this_.enabled && this_.highlighted) {
        rf::gr_set_color(255, 255, 255, 255);
    }
    else if (this_.enabled) {
        rf::gr_set_color(192, 192, 192, 255);
    }
    else {
        rf::gr_set_color(96, 96, 96, 255);
    }

    int x = static_cast<int>(this_.get_absolute_x() * rf::ui_scale_x);
    int y = static_cast<int>(this_.get_absolute_y() * rf::ui_scale_y);

    auto text = this_.items_text[this_.current_item];
    auto font = this_.items_font[this_.current_item];
    auto font_h = rf::gr_get_font_height(font);
    auto text_x = x + static_cast<int>(this_.w * rf::ui_scale_x / 2);
    auto text_y = y + static_cast<int>((this_.h * rf::ui_scale_y - font_h) / 2);
    rf::gr_string_aligned(rf::GR_ALIGN_CENTER, text_x, text_y, text, font);

    debug_ui_layout(this_);
}
FunHook UiCycler_render_hook{0x00457F40, UiCycler_render};

CallHook<void(int*, int*, const char*, int, int, char, int)> GrFitMultilineText_UiSetModalDlgText_hook{
    0x00455A7D,
    [](int *len_array, int *offset_array, const char *text, int max_width, int max_lines, char unk_char, int font) {
        max_width = static_cast<int>(max_width * rf::ui_scale_x);
        GrFitMultilineText_UiSetModalDlgText_hook.call_target(len_array, offset_array, text, max_width, max_lines, unk_char, font);
    },
};

FunHook<void()> menu_init_hook{
    0x00442BB0,
    []() {
        menu_init_hook.call_target();
#if SHARP_UI_TEXT
        xlog::info("UI scale: %.4f %.4f", rf::ui_scale_x, rf::ui_scale_y);
        if (rf::ui_scale_y > 1.0f) {
            int large_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 14.5f))); // 32
            int medium_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 9.0f))); // 20
            int small_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 7.5f))); // 16
            xlog::info("UI font sizes: %d %d %d", large_font_size, medium_font_size, small_font_size);

            rf::ui_large_font = rf::gr_load_font(string_format("boldfont.ttf:%d", large_font_size).c_str());
            rf::ui_medium_font_0 = rf::gr_load_font(string_format("regularfont.ttf:%d", medium_font_size).c_str());
            rf::ui_medium_font_1 = rf::ui_medium_font_0;
            rf::ui_small_font = rf::gr_load_font(string_format("regularfont.ttf:%d", small_font_size).c_str());
        }
#endif
    },
};

auto UiInputBox_add_char = reinterpret_cast<bool (__thiscall*)(void *this_, char c)>(0x00457260);

extern FunHook<bool __fastcall(void *this_, int edx, rf::Key key)> UiInputBox_process_key_hook;
bool __fastcall UiInputBox_process_key_new(void *this_, int edx, rf::Key key)
{
    if (key == (rf::KEY_V | rf::KEY_CTRLED)) {
        char buf[256];
        rf::os_get_clipboard_text(buf, std::size(buf) - 1);
        for (int i = 0; buf[i]; ++i) {
            UiInputBox_add_char(this_, buf[i]);
        }
        return true;
    }
    else {
        return UiInputBox_process_key_hook.call_target(this_, edx, key);
    }
}
FunHook<bool __fastcall(void *this_, int edx, rf::Key key)> UiInputBox_process_key_hook{
    0x00457300,
    UiInputBox_process_key_new,
};

void ui_apply_patch()
{
    // Sharp UI text
#if SHARP_UI_TEXT
    UiButton_create_hook.install();
    UiButton_set_text_hook.install();
    UiButton_render_hook.install();
    UiLabel_create_hook.install();
    UiLabel_create2_hook.install();
    UiLabel_set_text_hook.install();
    UiLabel_render_hook.install();
    UiInputBox_create_hook.install();
    UiInputBox_render_hook.install();
    UiCycler_add_item_hook.install();
    UiCycler_render_hook.install();
    GrFitMultilineText_UiSetModalDlgText_hook.install();
#endif

    // Init
    menu_init_hook.install();

    // Handle CTRL+V in input boxes
    UiInputBox_process_key_hook.install();
}