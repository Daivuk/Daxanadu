#pragma once

#include <cinttypes>
#include <vector>


class Patcher final
{
public:
    Patcher(uint8_t* rom);

    void apply_new_strings();                   // Adds new strings
    void apply_dialog_sound_patch();            // Make sure the text dialog typing sound is at constant rate, regardless of the speed setting.
    void apply_meditate_patch();                // Meditating should save progress in a save state, then show "Progress saved."
    void apply_progress_saved();                // Replace the string at C3 to show "progress saved." after saving
    void apply_welcome_back();                  // Replace the string at C3 to show "welcome back." after loading
    void apply_continue_patch();                // Load save state when selected continue from the title screen. (Useless now that we have main menu...)
    void apply_new_game_patch();                // Skips title screen and goes to new game when starting emulator
    void apply_mist_quality_patch();            // Implement better mist fog scrolling
    void load_cigarette_data();                 // Load cigarette sprite data
    void apply_king_golds_patch();              // New kings gold function
    void apply_double_golds_patch();            // Function that doubles the amount of gold reward from coins
    void apply_double_xp_patch();               // Function that doubles the amount of XP reward from killing enemies
    void apply_pause_patch();                   // Calls into C++ to show in-game menu and unbind input mapping
    void load_equip_in_shops_code();            // Load original equip in shop code so we can revert it on the fly
    void apply_inventory_input_patch();         // When inventory shows/hide, C++ is notified so we can swap input context
    void apply_start_full_mana_patch();         // Function that sets mana to full and redraws the mana bar.
    void apply_reset_gold_xp_patch();           // Save original code for the death xp and health
    void apply_xp_wingboots_patch();            // Save original values for wingboot timeouts
    void apply_xp_speed_patch();                // Save original values for xp speed increase
    void apply_sfx_patch();

    // Setting patches are re-applied when needed
    void apply_dialog_speed_setting_patch();    // Text dialog scrolls faster
    void apply_mist_quality_setting_patch();    // Apply the mist scroll setting. On or Off
    void apply_coins_despawn_setting_patch();   // Prevent coins to disapear after a few seconds
    void apply_cigarettes_setting_patch();      // Remove smoking imagery
    void apply_king_golds_setting_patch();      // King gives gold only once
    void apply_double_golds_setting_patch();    // Coins give 2x more golds
    void apply_double_xp_setting_patch();       // XP gives 2x more golds
    void apply_equip_in_shops_setting_patch();  // Allow the player to equip items in shops
    void apply_start_full_health_setting_patch(); // Player to start with full health
    void apply_secret_item_setting_patch();     // Dont need to enter secret room 4 times
    void apply_reset_gold_xp_setting_patch();   // When dying, keep or reset
    void apply_xp_wingboots_setting_patch();    // Item timer affected by XP or not
    void apply_xp_speed_setting_patch();        // XP affects player speed or not
    void apply_pendant_setting_patch();

private:
    static const int BANK15_EMPTY_SPACE = 0xFE00;

    void patch(int addr, const std::vector<uint8_t>& code);
    int patch_lo(int bank, int addr, int offset, const std::vector<uint8_t>& code);
    int patch_hi(int bank, int addr, int offset, const std::vector<uint8_t>& code);

    uint8_t* m_rom = nullptr;
    int m_next_bank15_empty_space = BANK15_EMPTY_SPACE;
    int m_mist_scroll_addr = 0;
    uint8_t m_cigarette_original_data[8 * 16];
    uint8_t m_cigarette_new_data[8 * 16];
    int m_king_golds_addr = 0;
    int m_double_golds_setting_addr = 0;
    int m_double_xp_setting_addr = 0;
    std::vector<uint8_t> m_equip_in_shops_original_code;
    int m_start_full_mana_addr = 0;
    std::vector<uint8_t> m_reset_gold_xp_original_code;
    std::vector<uint8_t> m_xp_wingboots_values;
    std::vector<uint8_t> m_xp_speed_values;
};
