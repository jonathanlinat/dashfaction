#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <common/version/version.h>
#include <xlog/xlog.h>
#include <cstring>
#include "../rf/ui.h"
#include "../rf/gr.h"
#include "../rf/gr_font.h"
#include "../rf/input.h"
#include "../rf/file.h"
#include "../rf/multi.h"
#include "../main/main.h"

#define SHARP_UI_TEXT 1

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

constexpr double PI = 3.14159265358979323846;

int g_version_click_counter = 0;
int g_egg_anim_start;

namespace rf
{
    static auto& menu_version_label = addr_as_ref<UiGadget>(0x0063C088);
}

// Note: fastcall is used because MSVC does not allow free thiscall functions
using UiLabel_Create2_Type = void __fastcall(rf::UiGadget*, int, rf::UiGadget*, int, int, int, int, const char*, int);
extern CallHook<UiLabel_Create2_Type> UiLabel_create2_version_label_hook;
void __fastcall UiLabel_create2_version_label(rf::UiGadget* self, int edx, rf::UiGadget* parent, int x, int y, int w,
                                             int h, const char* text, int font_id)
{
    text = PRODUCT_NAME_VERSION;
    rf::gr_get_string_size(&w, &h, text, -1, font_id);
#if SHARP_UI_TEXT
    w = static_cast<int>(w / rf::ui_scale_x);
    h = static_cast<int>(h / rf::ui_scale_y);
#endif
    x = 430 - w;
    w += 5;
    h += 2;
    UiLabel_create2_version_label_hook.call_target(self, edx, parent, x, y, w, h, text, font_id);
}
CallHook<UiLabel_Create2_Type> UiLabel_create2_version_label_hook{0x0044344D, UiLabel_create2_version_label};

CallHook<void()> main_menu_process_mouse_hook{
    0x004437B9,
    []() {
        main_menu_process_mouse_hook.call_target();
        if (rf::mouse_was_button_pressed(0)) {
            int x, y, z;
            rf::mouse_get_pos(x, y, z);
            rf::UiGadget* gadgets_to_check[1] = {&rf::menu_version_label};
            int matched = rf::ui_get_gadget_from_pos(x, y, gadgets_to_check, std::size(gadgets_to_check));
            if (matched == 0) {
                xlog::trace("Version clicked");
                ++g_version_click_counter;
                if (g_version_click_counter == 3)
                    g_egg_anim_start = GetTickCount();
            }
        }
    },
};

int load_easter_egg_image()
{
    HRSRC res_handle = FindResourceA(g_hmodule, MAKEINTRESOURCEA(100), RT_RCDATA);
    if (!res_handle) {
        xlog::error("FindResourceA failed");
        return -1;
    }
    HGLOBAL res_data_handle = LoadResource(g_hmodule, res_handle);
    if (!res_data_handle) {
        xlog::error("LoadResource failed");
        return -1;
    }
    void* res_data = LockResource(res_data_handle);
    if (!res_data) {
        xlog::error("LockResource failed");
        return -1;
    }

    constexpr int easter_egg_size = 128;

    int hbm = rf::bm_create(rf::BM_FORMAT_8888_ARGB, easter_egg_size, easter_egg_size);

    rf::GrLockInfo lock;
    if (!rf::gr_lock(hbm, 0, &lock, rf::GR_LOCK_WRITE_ONLY))
        return -1;

    rf::bm_convert_format(lock.data, lock.format, res_data, rf::BM_FORMAT_8888_ARGB,
                        easter_egg_size * easter_egg_size);
    rf::gr_unlock(&lock);

    return hbm;
}

CallHook<void()> main_menu_render_hook{
    0x00443802,
    []() {
        main_menu_render_hook.call_target();
        if (g_version_click_counter >= 3) {
            static int img = load_easter_egg_image(); // data.vpp
            if (img == -1)
                return;
            int w, h;
            rf::bm_get_dimensions(img, &w, &h);
            int anim_delta_time = GetTickCount() - g_egg_anim_start;
            int pos_x = (rf::gr_screen_width() - w) / 2;
            int pos_y = rf::gr_screen_height() - h;
            if (anim_delta_time < EGG_ANIM_ENTER_TIME) {
                float enter_progress = anim_delta_time / static_cast<float>(EGG_ANIM_ENTER_TIME);
                pos_y += h - static_cast<int>(sinf(enter_progress * static_cast<float>(PI) / 2.0f) * h);
            }
            else if (anim_delta_time > EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME) {
                int leave_delta = anim_delta_time - (EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME);
                float leave_progress = leave_delta / static_cast<float>(EGG_ANIM_LEAVE_TIME);
                pos_y += static_cast<int>((1.0f - cosf(leave_progress * static_cast<float>(PI) / 2.0f)) * h);
                if (leave_delta > EGG_ANIM_LEAVE_TIME)
                    g_version_click_counter = 0;
            }
            rf::gr_bitmap(img, pos_x, pos_y, rf::gr_bitmap_clamp_mode);
        }
    },
};

struct ServerListEntry
{
    char name[32];
    char level_name[32];
    char mod_name[16];
    int game_type;
    rf::NwAddr addr;
    char current_players;
    char max_players;
    int16_t ping;
    int field_60;
    char field_64;
    int flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook<int(const int&, const int&)> server_list_cmp_func_hook{
    0x0044A6D0,
    [](const int& index1, const int& index2) {
        auto server_list = addr_as_ref<ServerListEntry*>(0x0063F62C);
        bool has_ping1 = server_list[index1].ping >= 0;
        bool has_ping2 = server_list[index2].ping >= 0;
        if (has_ping1 != has_ping2)
            return has_ping1 ? -1 : 1;
        else
            return server_list_cmp_func_hook.call_target(index1, index2);
    },
};

void apply_main_menu_patches()
{
    // Version in Main Menu
    UiLabel_create2_version_label_hook.install();

    // Version Easter Egg
    main_menu_process_mouse_hook.install();
    main_menu_render_hook.install();

    // Put not responding servers at the bottom of server list
    server_list_cmp_func_hook.install();
}
