#pragma once

#include <cinttypes>
#include <string>
#include <vector>


#define PATCH_ADDR(addr) (uint8_t)((addr) & 0xFF), (uint8_t)(((addr) >> 8) & 0xFF)
#define PATCH_CALL_CPP(id) 0xA9, id, 0x8D, PATCH_ADDR(0x6000)


#define OP_PHA() 0x48
#define OP_PLA() 0x68
#define OP_LDA_ABS(addr) 0xAD, PATCH_ADDR(addr)
#define OP_LDA_IMM(value) 0xA9, value
#define OP_LDA_ZPG(addr) 0xA5, addr
#define OP_LDA_ZPGX(addr) 0xB5, addr
#define OP_LDA_ABSX(addr) 0xBD, PATCH_ADDR(addr)
#define OP_LDA_ABSY(addr) 0xB9, PATCH_ADDR(addr)
#define OP_LDA_INDY(addr) 0xB1, PATCH_ADDR(addr)
#define OP_STA_ZPG(addr) 0x85, addr
#define OP_STA_ABS(addr) 0x8D, PATCH_ADDR(addr)
#define OP_STA_ABSX(addr) 0x9D, PATCH_ADDR(addr)
#define OP_STA_ABSY(addr) 0x99, PATCH_ADDR(addr)
#define OP_LDY_IMM(value) 0xA0, value
#define OP_LDY_ABS(addr) 0xAC, PATCH_ADDR(addr)
#define OP_LDY_ABSX(addr) 0xBC, PATCH_ADDR(addr)
#define OP_STY_ZPG(addr) 0x84, addr
#define OP_LDX_ABS(addr) 0xAE, PATCH_ADDR(addr)
#define OP_LDX_ABSY(addr) 0xBE, PATCH_ADDR(addr)
#define OP_LDX_IMM(value) 0xA2, value
#define OP_STX_ABS(addr) 0x8E, PATCH_ADDR(addr)
#define OP_STX_ZPG(addr) 0x86, addr
#define OP_AND_IMM(value) 0x29, value
#define OP_CMP_IMM(value) 0xC9, value
#define OP_CMP_ABS(addr) 0xCD, PATCH_ADDR(addr)
#define OP_CMP_ABSX(addr) 0xDD, PATCH_ADDR(addr)
#define OP_CMP_ABSY(addr) 0xD9, PATCH_ADDR(addr)
#define OP_CPX_IMM(value) 0xE0, value
#define OP_CPY_IMM(value) 0xC0, value
#define OP_BMI(rel) 0x30, rel
#define OP_BCC(rel) 0x90, rel
#define OP_BNE(rel) 0xD0, rel
#define OP_BEQ(rel) 0xF0, rel
#define OP_BPL(rel) 0x10, rel
#define OP_JMP_ABS(addr) 0x4C, PATCH_ADDR(addr)
#define OP_JSR(addr) 0x20, PATCH_ADDR(addr)
#define OP_RTS() 0x60
#define OP_NOP() 0xEA
#define OP_CLC() 0x18
#define OP_SEC() 0x38
#define OP_ADC_IMM(value) 0x69, value
#define OP_ADC_ABS(addr) 0x6D, PATCH_ADDR(addr)
#define OP_ADC_ABSY(addr) 0x79, PATCH_ADDR(addr)
#define OP_SBC_IMM(value) 0xE9, value
#define OP_TAY() 0xA8
#define OP_TYA() 0x98
#define OP_TAX() 0xAA
#define OP_TXA() 0x8A
#define OP_INC_ZPG(addr) 0xE6, addr
#define OP_INX() 0xE8
#define OP_INY() 0xC8
#define OP_DEX() 0xCA
#define OP_DEC_ZPG(addr) 0xC6, addr
#define OP_DEC_ABSX(addr) 0xDE, PATCH_ADDR(addr)
#define OP_INC_ABS(addr) 0xEE, PATCH_ADDR(addr)
#define OP_INC_ABSX(addr) 0xFE, PATCH_ADDR(addr)
#define OP_ORA_IMM(value) 0x09, value
#define OP_ASL_A() 0x0A


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
    void apply_i_am_error_patch();
    void patch_ap_message(const std::string& msg);

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

    void patch(int addr, const std::vector<uint8_t>& code);
    void patch(int bank, int addr, int offset, const std::vector<uint8_t>& code);
    int patch_new_code(int bank, const std::vector<uint8_t>& code);
    void advance_new_code(int bank, int size);
    int get_new_code_addr(int bank);

    void print_usage();

private:
    uint8_t* m_rom = nullptr;
    int m_next_banks_empty_space[16] = { 0 };
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
    int m_ap_message_addr = 0;
};
