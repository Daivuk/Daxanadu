#include "Patcher.h"

#include <onut/Dialogs.h>
#include <onut/Images.h>
#include <onut/onut.h>
#include <onut/Settings.h>


static const int BANKS_EMPTY_SPACE[16] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,
    0x8000, // Bank 9 not used at all
    0x0000,
    0x0000,

    0xBE00,
    0x0000,
    0xBDB5,
    0xFCCE
};


static const int BANKS_EMPTY_SPACE_LIMIT[16] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,
    0xFFFF,
    0x0000,
    0x0000,

    0xBFA0,
    0x0000,
    0xBFFF,
    0xFFE7
};


Patcher::Patcher(uint8_t* rom)
    : m_rom(rom)
{
    for (int i = 0; i < 16; ++i)
        m_next_banks_empty_space[i] = BANKS_EMPTY_SPACE[i];

    apply_new_strings();
    apply_dialog_speed_setting_patch();
    apply_dialog_sound_patch();
    apply_meditate_patch();
    apply_continue_patch();
    apply_new_game_patch();
    apply_mist_quality_patch();
    apply_mist_quality_setting_patch();
    apply_coins_despawn_setting_patch();
    load_cigarette_data();
    apply_cigarettes_setting_patch();
    apply_king_golds_patch();
    apply_king_golds_setting_patch();
    apply_double_golds_patch();
    apply_double_golds_setting_patch();
    apply_double_xp_patch();
    apply_double_xp_setting_patch();
    apply_pause_patch();
    load_equip_in_shops_code();
    apply_equip_in_shops_setting_patch();
    apply_inventory_input_patch();
    apply_start_full_mana_patch();
    apply_start_full_health_setting_patch();
    apply_secret_item_setting_patch();
    apply_reset_gold_xp_patch();
    apply_reset_gold_xp_setting_patch();
    apply_xp_speed_patch();
    apply_xp_speed_setting_patch();
    apply_pendant_setting_patch();
    apply_sfx_patch();
}


static void load_tile_from_png(const char* filename, uint8_t* out_data)
{
    Point size;
    auto png_data = onut::loadPNG(filename, size);
    if (size.x != 8 || size.y != 8)
    {
        onut::showMessageBox("ERROR", "(0x80000008) Corrupted or missing tile file: " + std::string(filename));
        OQuit();
    }

    for (int y = 0; y < 8; ++y)
    {
        uint8_t plane0 = out_data[y];
        uint8_t plane1 = out_data[y + 8];
        for (int x = 0; x < 8; ++x)
        {
            uint32_t col32 = *reinterpret_cast<uint32_t*>(&png_data[y * 8 * 4 + x * 4]);
            int b0 = 0;
            int b1 = 0;
            switch (col32)
            {
                case 0xFF000000: b0 = 0; b1 = 0; break;
                case 0xFF002c40: b0 = 1; b1 = 0; break;
                case 0xFF007088: b0 = 1; b1 = 0; break;
                case 0xFF0c4cc8: b0 = 0; b1 = 1; break;
                case 0xFF6074fb: b0 = 0; b1 = 1; break;
                case 0xFF6074fc: b0 = 1; b1 = 1; break;
                case 0xFFfcfcfc: b0 = 1; b1 = 1; break;
                default: continue; // Leave original for other pixel colors
            }
            plane0 = (plane0 & ~(1 << (7 - x))) | (b0 << (7 - x));
            plane1 = (plane1 & ~(1 << (7 - x))) | (b1 << (7 - x));
        }
        out_data[y] = plane0;
        out_data[y + 8] = plane1;
    }
}


void Patcher::patch(int addr, const std::vector<uint8_t>& code)
{
    for (int i = 0, end = (int)code.size(); i < end; ++i)
    {
        auto byte = code[i];
        //if (byte == 0xFF) continue; // This means we skip over this instruction, we don't patch it
        m_rom[addr + i] = byte;
    }
}


void Patcher::patch(int bank, int addr, int offset, const std::vector<uint8_t>& code)
{
    if (bank < 15)
    {
        patch(bank * 0x4000 + addr - 0x8000 + offset, code);
    }
    else
    {
        patch(bank * 0x4000 + addr - 0xC000 + offset, code);
    }
}


int Patcher::patch_new_code(int bank, const std::vector<uint8_t>& code)
{
    if (m_next_banks_empty_space[bank] == 0 || BANKS_EMPTY_SPACE_LIMIT[bank] == 0)
    {
        onut::showMessageBox("ERROR", "Bank " + std::to_string(bank) + " not configured for new code");
        OQuit();
    }

    auto addr = m_next_banks_empty_space[bank];
    if (addr + (int)code.size() >= BANKS_EMPTY_SPACE_LIMIT[bank])
    {
        onut::showMessageBox("ERROR", "No more free space in bank " + std::to_string(bank));
        OQuit();
    }

    patch(bank, addr, 0, code);
    m_next_banks_empty_space[bank] += (int)code.size();

    return addr;
}


void Patcher::apply_new_strings()
{
    std::vector<std::string> strings = {
        "Progress saved.", // C3
        "I've got the""\xFE""Key Jack.", // C4
        "I've got the""\xFE""Key Queen.", // C5
        "I've got the""\xFE""Key King.", // C6
        "I've got the""\xFE""Key Ace.", // C7
        "I've got the""\xFE""Key Joker.", // C8
        
        "I've got the""\xFE""Ring of Elf.", // C9
        "I've got the""\xFE""Ring of Ruby.", // CA
        "I've got the""\xFE""Ring of Dworf.", // CB
        "I've got the""\xFE""Demons Ring.", // CC

        "I've got""\xFE""Deluge.", // CD
        "I've got""\xFE""Thunder.", // CE
        "I've got""\xFE""Fire.", // CF
        "I've got""\xFE""Death.", // D0
        "I've got""\xFE""Tilte.", // D1

        "I've got the""\xFE""Spring Elixir.", // D2
    };

    int addr = 13 * 0x4000 + 0xB45B - 0x8000;
    for (const auto& string : strings)
    {
        memcpy(&m_rom[addr], string.c_str(), string.size());
        m_rom[addr + (int)string.size()] = 0xFF; // end of string
        addr += (int)string.size() + 1;
    }
}


void Patcher::apply_dialog_speed_setting_patch()
{
    int DIALOG_SPEEDS[] = {0x03, 0x01, 0x00};
    int dialog_speed = 0x03;
    try
    {
        int option = std::stoi(oSettings->getUserSetting("dialog_speed"));
        if (option < 0 || option > 2) option = 0;
        dialog_speed = DIALOG_SPEEDS[option];
    }
    catch (...)
    {
        dialog_speed = 0x03;
    }

    if (dialog_speed != 0x00 && dialog_speed != 0x01 && dialog_speed != 0x03) dialog_speed = 0x03; // Default if invalid value

    patch(15, 0xF49E, 1, { (uint8_t)dialog_speed });
}


void Patcher::apply_dialog_sound_patch()
{
    // Play the sound, but not too much. Using the value at $021D which increases constantly.
    auto addr = patch_new_code(15, {
        OP_PHA(),
        OP_LDA_ABS(0x021D),
        OP_AND_IMM(3),
        OP_BNE(4),
        OP_PLA(),
        OP_JMP_ABS(0xD0E4), // play_sound
        OP_PLA(),
        OP_RTS(),
    });

    // When playing a dialog typing sound, jump to our own function
    patch(15, 0xF4FB, 1, { PATCH_ADDR(addr) });
}


void Patcher::apply_meditate_patch()
{
    // Write new dialog script
    patch(12, 0xADA0, 0, {
        0x03, // Show dialog
        0x22, // "You need peace of mind, I'll meditate with you."
        0x14, // Meditate - This calls a C++ function to save state, see bellow
        0x03, // Show dialog
        0xC3, // "Progress saved."
        0x00, // End script
    });

    // Point the old guru dialog to this new one
    patch(12, 0xA5FC, 0, {
        0x17, // Jump SCRIPT
        PATCH_ADDR(0xADA0),
    });

    // Erase existing meditate function with NOPes
    std::vector<uint8_t> nopes;
    for (int i = 0x8737; i < 0x8754; ++i)
        nopes.push_back(OP_NOP());
    patch(12, 0x8737, 0, nopes);

    // Insert our code instead that will call our C++ save state function
    patch(12, 0x8737, 0, {
        OP_PHA(),
        PATCH_CALL_CPP(0x01), // Save state function
        OP_PLA(),
    });
}


void Patcher::apply_progress_saved()
{
    int addr = 13 * 0x4000 + 0xB45B - 0x8000;
    memcpy(&m_rom[addr], "Progress saved.", strlen("Progress saved."));
}


void Patcher::apply_welcome_back()
{
    int addr = 13 * 0x4000 + 0xB45B - 0x8000;
    memcpy(&m_rom[addr], "Welcome back.  ", strlen("Welcome back.  "));
}


void Patcher::apply_continue_patch()
{
    patch(15, 0xFC89, 0, {
        PATCH_CALL_CPP(0x02), // Continue C++ function
        // It will load a state, we don't care what follows
    });
}


void Patcher::apply_new_game_patch()
{
    std::vector<uint8_t> original_function;
    for (int i = 0xFC98; i <= 0xFCA6; ++i)
    {
        original_function.push_back(m_rom[15 * 0x4000 + (i - 0xC000)]);
    }

    std::vector<uint8_t> new_code = {
        PATCH_CALL_CPP(0x05), // Notify C++ that we start the game.
        OP_LDA_ABS(0x0687),
    };

    new_code.insert(new_code.end(), original_function.begin(), original_function.end());

    // Replace the code that deals with new game with out own that inserts a callback into C++
    auto addr = patch_new_code(15, new_code);

    patch(15, 0xFC98, 0, { OP_JMP_ABS(addr) });
}


void Patcher::apply_mist_quality_patch()
{
    // Modification of the function at 0xCF3C, where we inserted our own stuff and change jump addrs
    m_mist_scroll_addr = patch_new_code(15, {
        0xA5, // LDA $24                    This is where current level is stored
        0x24,
        0xC9, // CMP $02                    2 is the ID for the mist world.
        0x02,
        0xD0, // BNE +5
        0x05,

        0xA9, // LDA #$03                   C++: Scroll mist in CHR RAM
        0x03,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),

        0xA5, // LDA $1F
        0x1F,
        0xC5, // CMP $20
        0x20,
        0xF0, // BEQ +25
        0x19,
        0xA6, // LDX $1F
        0x1F,
        0xA9, // LDA #0
        0x00,
        0x38, // SEC
        0xFD, // SBC $0500,X
        PATCH_ADDR(0x0500),
        0xC9, // CMP #7
        0x07,
        0xB0, // BCS +16
        0x10,
        0xE6, // INC $1F
        0x1F,
        0x0A, // ASL A
        0xA8, // TAY
        0xB9, // LDA $CFBC+1,Y
        PATCH_ADDR(0xCFBC + 1),
        0x48, // PHA
        0xB9, // LDA $CFBC,Y
        PATCH_ADDR(0xCFBC),
        0x48, // PHA
        0x60, // RTS

        0x4C, // JMP $CF3B
        PATCH_ADDR(0xCF3B),

        0x4C, // JMP $CF5B
        PATCH_ADDR(0xCF5B),
    });
}


void Patcher::apply_mist_quality_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("mist_quality");

    if (setting == "1")
    {
        // Jump to our own mist scroll function
        patch(15, 0xCF3C, 0, {
            0x4C, // JMP $CF3B
            PATCH_ADDR(m_mist_scroll_addr)
        });
        
        // Skip the original graphic shift
        patch(15, 0xCD56, 0, {
            0x60, // RTS
        });
    }
    else
    {
        // Restore original 3 bytes
        patch(15, 0xCF3C, 0, {
            0xA5, // LDA $1F
            0x1F,
            0xC5, // CMP
        });

        // Restore the original byte at 0xCD56
        patch(15, 0xCD56, 0, {
            0xAD, // LDA
        });
    }
}


void Patcher::apply_coins_despawn_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("coins_despawn");

    if (setting == "1")
    {
        patch(14, 0x8D06, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
    }
    else
    {
        patch(14, 0x8D06, 0, {
            0x9D, // STA $02CC, X
            PATCH_ADDR(0x02CC)
        });
    }
}


void Patcher::load_cigarette_data()
{
    memcpy(&m_cigarette_original_data[0 * 16], &m_rom[0x0001C446 + 0x00 * 16], 16); // Key seller
    memcpy(&m_cigarette_original_data[1 * 16], &m_rom[0x0001C446 + 0x07 * 16], 16); // Key seller
    memcpy(&m_cigarette_original_data[2 * 16], &m_rom[0x0001C4C6 + 0x00 * 16], 16); // Town person
    memcpy(&m_cigarette_original_data[3 * 16], &m_rom[0x0001C4C6 + 0x08 * 16], 16); // Town person
    memcpy(&m_cigarette_original_data[4 * 16], &m_rom[0x0002136B + 0xD0 * 16], 16); // Portrait closed
    memcpy(&m_cigarette_original_data[5 * 16], &m_rom[0x0002136B + 0xD1 * 16], 16); // Portrait closed
    memcpy(&m_cigarette_original_data[6 * 16], &m_rom[0x0002136B + 0xD4 * 16], 16); // Portrait open
    memcpy(&m_cigarette_original_data[7 * 16], &m_rom[0x0002136B + 0xD5 * 16], 16); // Portrait open

    memcpy(m_cigarette_new_data, m_cigarette_original_data, sizeof(m_cigarette_original_data));

    load_tile_from_png("assets/images/key_salesman_patch_00.png", &m_cigarette_new_data[0 * 16]);
    load_tile_from_png("assets/images/key_salesman_patch_00.png", &m_cigarette_new_data[1 * 16]);
    load_tile_from_png("assets/images/smoking_town_person_patch_00.png", &m_cigarette_new_data[2 * 16]);
    load_tile_from_png("assets/images/smoking_town_person_patch_00.png", &m_cigarette_new_data[3 * 16]);
    load_tile_from_png("assets/images/key_salesman_portrait_patch_00.png", &m_cigarette_new_data[4 * 16]);
    load_tile_from_png("assets/images/key_salesman_portrait_patch_01.png", &m_cigarette_new_data[5 * 16]);
    load_tile_from_png("assets/images/key_salesman_portrait_patch_02.png", &m_cigarette_new_data[6 * 16]);
    memcpy(&m_cigarette_new_data[7 * 16], &m_cigarette_new_data[5 * 16], 16);
}


void Patcher::apply_cigarettes_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("cigarettes");

    if (setting == "1")
    {
        memcpy(&m_rom[0x0001C446 + 0x00 * 16], &m_cigarette_new_data[0 * 16], 16);
        memcpy(&m_rom[0x0001C446 + 0x07 * 16], &m_cigarette_new_data[1 * 16], 16);
        memcpy(&m_rom[0x0001C4C6 + 0x00 * 16], &m_cigarette_new_data[2 * 16], 16);
        memcpy(&m_rom[0x0001C4C6 + 0x08 * 16], &m_cigarette_new_data[3 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD0 * 16], &m_cigarette_new_data[4 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD1 * 16], &m_cigarette_new_data[5 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD4 * 16], &m_cigarette_new_data[6 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD5 * 16], &m_cigarette_new_data[7 * 16], 16);
    }
    else
    {
        memcpy(&m_rom[0x0001C446 + 0x00 * 16], &m_cigarette_original_data[0 * 16], 16);
        memcpy(&m_rom[0x0001C446 + 0x07 * 16], &m_cigarette_original_data[1 * 16], 16);
        memcpy(&m_rom[0x0001C4C6 + 0x00 * 16], &m_cigarette_original_data[2 * 16], 16);
        memcpy(&m_rom[0x0001C4C6 + 0x08 * 16], &m_cigarette_original_data[3 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD0 * 16], &m_cigarette_original_data[4 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD1 * 16], &m_cigarette_original_data[5 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD4 * 16], &m_cigarette_original_data[6 * 16], 16);
        memcpy(&m_rom[0x0002136B + 0xD5 * 16], &m_cigarette_original_data[7 * 16], 16);
    }
}


void Patcher::apply_king_golds_patch()
{
    // Create our own function to replace the money check. It will
    // instead call into C++ to check our custom registers to see if
    // the gift was given already or not.
    m_king_golds_addr = patch_new_code(15, {
        0xA9, // LDA #$04                   C++: Check if money was given, and set to true
        0x04,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0xAD, // LDA $6000
        PATCH_ADDR(0x6000),
        0xD0, // BNE +3
        0x03,
        0x4C, // JMP $8616        give 1500 to player
        PATCH_ADDR(0x8616),
        0x4C, // JMP $862D        skip giving 1500 to player
        PATCH_ADDR(0x862D),
    });
}


void Patcher::apply_king_golds_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("king_golds");

    if (setting == "1")
    {
        patch(12, 0x861F, 0, {
            0x4C,
            PATCH_ADDR(m_king_golds_addr)
        });
    }
    else
    {
        patch(12, 0x861F, 0, {
            0xAD,
            PATCH_ADDR(0x0392)
        });
    }
}


void Patcher::apply_double_golds_patch()
{
    // Add a setting byte that we will read to know if we should double or not
    m_double_golds_setting_addr = patch_new_code(15, { 0x00 });

    // Redo the function that gives the gold, and add the reward twice.
    auto addr = patch_new_code(15, {
        // Add money once (Reward is stored at $036C[X], where X is current entity)
        0xAD, // LDA $0392
        PATCH_ADDR(0x0392),
        0x18, // CLC
        0x7D, // ADC $036C,X
        PATCH_ADDR(0x036C),
        0x8D, // STA $0392
        PATCH_ADDR(0x0392),
        0xAD, // LDA $0393
        PATCH_ADDR(0x0393),
        0x69, // ADC #0
        0x00,
        0x8D, // STA $0393
        PATCH_ADDR(0x0393),
        0xAD, // LDA $0394
        PATCH_ADDR(0x0394),
        0x69, // ADC #0
        0x00,
        0x8D, // STA $0394
        PATCH_ADDR(0x0394),

        // Check our setting if we should double or not
        0xAD, // LDA m_double_golds_setting_addr
        PATCH_ADDR(m_double_golds_setting_addr),
        0xF0, // BEQ +26
        0x1A,

        // Do it again
        0xAD, // LDA $0392
        PATCH_ADDR(0x0392),
        0x18, // CLC
        0x7D, // ADC $036C,X
        PATCH_ADDR(0x036C),
        0x8D, // STA $0392
        PATCH_ADDR(0x0392),
        0xAD, // LDA $0393
        PATCH_ADDR(0x0393),
        0x69, // ADC #0
        0x00,
        0x8D, // STA $0393
        PATCH_ADDR(0x0393),
        0xAD, // LDA $0394
        PATCH_ADDR(0x0394),
        0x69, // ADC #0
        0x00,
        0x8D, // STA $0394
        PATCH_ADDR(0x0394),

        0x20, // JSR $F9E7          Update money bar
        PATCH_ADDR(0xF9E7),
        0xAE, // LDX $02CC          Sprite number
        PATCH_ADDR(0x02CC),
        0x60, // RTS
    });

    // Jump to our function instead of the original one
    patch(14, 0x8B9F, 0, {
        0x4C, // JMP addr
        PATCH_ADDR(addr)
    });
}


void Patcher::apply_double_golds_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("double_golds");

    if (setting == "1")
    {
        patch(15, m_double_golds_setting_addr, 0, { 0x01 });
    }
    else
    {
        patch(15, m_double_golds_setting_addr, 0, { 0x00 });
    }
}


void Patcher::apply_double_xp_patch()
{
    // Add a setting byte that we will read to know if we should double or not
    m_double_xp_setting_addr = patch_new_code(15, { 0x00 });

    // Redo the function that gives the gold, and add the reward twice.
    auto addr = patch_new_code(15, {
        // Add xp once

        0xAD, // LDA $0390
        PATCH_ADDR(0x0390),
        0x18, // CLC
        0x65, // ADC $EC
        0xEC,
        0x8D, // STA $0390
        PATCH_ADDR(0x0390),
        0xAD, // LDA $0391
        PATCH_ADDR(0x0391),
        0x65, // ADC $ED
        0xED,
        0x8D, // STA $0391
        PATCH_ADDR(0x0391),
        0x90, // BCC +8
        0x08,
        0xA9, // LDA #$FF
        0xFF,
        0x8D, // STA $0390
        PATCH_ADDR(0x0390),
        0x8D, // STA $0391
        PATCH_ADDR(0x0391),

        // Check our setting if we should double or not
        0xAD, // LDA m_double_xp_setting_addr
        PATCH_ADDR(m_double_xp_setting_addr),
        0xF0, // BEQ +27
        0x1B,

        0xAD, // LDA $0390
        PATCH_ADDR(0x0390),
        0x18, // CLC
        0x65, // ADC $EC
        0xEC,
        0x8D, // STA $0390
        PATCH_ADDR(0x0390),
        0xAD, // LDA $0391
        PATCH_ADDR(0x0391),
        0x65, // ADC $ED
        0xED,
        0x8D, // STA $0391
        PATCH_ADDR(0x0391),
        0x90, // BCC +8
        0x08,
        0xA9, // LDA #$FF
        0xFF,
        0x8D, // STA $0390
        PATCH_ADDR(0x0390),
        0x8D, // STA $0391
        PATCH_ADDR(0x0391),

        0x4C, // JMP $F972
        PATCH_ADDR(0xF972),
    });

    // Jump to our function instead of the original one
    patch(15, 0xF957, 0, {
        0x4C, // JMP addr
        PATCH_ADDR(addr)
    });
}


void Patcher::apply_double_xp_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("double_xp");

    if (setting == "1")
    {
        patch(15, m_double_xp_setting_addr, 0, { 0x01 });
    }
    else
    {
        patch(15, m_double_xp_setting_addr, 0, { 0x00 });
    }
}


void Patcher::apply_pause_patch()
{
    // Pause patch
    auto addr = patch_new_code(15, {
        0xA9, // LDA #$06                   C++: Show in-game menu and remove input context
        0x06,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0xA9, // LDA #1
        0x01,
        0x8D, // STA $0120                  Pause flag
        PATCH_ADDR(0x0120),
        0x60, // RTS
    });

    patch(15, 0xE031, 0, {
        0x20, // JSR addr
        PATCH_ADDR(addr),
        0xEA, // NOP
        0xEA, // NOP
    });

    // Unpause patch
    addr = patch_new_code(15, {
        0xA9, // LDA #0
        0x00,
        0x8D, // STA $0120                  Pause flag
        PATCH_ADDR(0x0120),
        0xA9, // LDA #$07                   C++: Hide in-game menu and restore input context
        0x07,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0x60, // RTS
    });

    patch(15, 0xE042, 0, {
        0x20, // JSR addr
        PATCH_ADDR(addr),
        0x60, // RTS
    });
}


void Patcher::load_equip_in_shops_code()
{
    for (int i = 0x8B86; i < 0x8B86 + 4; ++i)
    {
        m_equip_in_shops_original_code.push_back(m_rom[12 * 0x4000 + (i - 0x8000)]);
    }
}


void Patcher::apply_equip_in_shops_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("equip_in_shops");

    if (setting == "1")
    {
        // We remove the check that checks if we are in a building
        patch(12, 0x8B86, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
    }
    else
    {
        // Restore original code
        patch(12, 0x8B86, 0, m_equip_in_shops_original_code);
    }
}


void Patcher::apply_inventory_input_patch()
{
    // Code that will trigger the menu context
    auto addr = patch_new_code(15, {
        0x85, // STA $DE
        0xDE,
        0x86, // STX $DF
        0xDF,
        0x48, // PHA
        0xA9, // LDA #$08                   C++: Use menu input context
        0x08,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0x68, // PLA
        0x60, // RTS
    });

    // Patch the original generic show message function.
    patch(15, 0xF859, 0, {
        0x20, // JSR addr
        PATCH_ADDR(addr),
        0xEA, // NOP
    });

    // Now for the restore, I haven't found where the code is to dismiss dialogs.
    // We do a hack instead. If the code to check if we show dialog by pressing select is called,
    // that means we're in normal gameplay.

    // Code that will trigger the menu context
    auto check_select_addr = patch_new_code(15, {
        0xA9, // LDA #$09                   C++: Use gameplay input context
        0x09,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0xA5, // LDA $19                    Load input pressed
        0x19,
        0x29, // AND #100000b               Check if select is pressed
        0x20,
        0xF0, // BEQ +6
        0x06,
        0x20, // JSR $F859                  Call function that show generic message
        PATCH_ADDR(0xF859),
        0x0C, 0x92, 0x8A, // What's this? Invalid instructions
        0x60, // RTS
    });

    // Call our subroutine first
    patch(15, 0xE01C, 0, {
        0xEA, // NOP
        0xEA, // NOP
        0xEA, // NOP
        0x4C, // JMP addr
        PATCH_ADDR(check_select_addr),
    });
}


void Patcher::apply_start_full_mana_patch()
{
    m_start_full_mana_addr = patch_new_code(15, {
        0xA9, // LDA #$50
        0x50,
        0x8D, // STA $039A                Mana amount
        PATCH_ADDR(0x039A),
        0x20, // JSR $FA85                Redraws mana bar
        PATCH_ADDR(0xFA85),
        0x60, // RTS
    });

    // Call our subroutine and override original code that sets mana to 0
    patch(15, 0xDB2F, 0, {
        0x20, // JSR addr
        PATCH_ADDR(m_start_full_mana_addr),
        0xEA, // NOP
        0xEA, // NOP
    });
}


void Patcher::apply_start_full_health_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("start_full_health");

    if (setting == "1")
    {
        // We remove the check that checks if we are in a building
        patch(15, 0xDEAE, 1, { 0x50 });
        patch(15, m_start_full_mana_addr, 1, { 0x50 });
    }
    else
    {
        // Restore original code
        patch(15, 0xDEAE, 1, { 0x10 });
        patch(15, m_start_full_mana_addr, 1, { 0x00 });
    }
}


void Patcher::apply_secret_item_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("secret_items_counter");

    if (setting == "1")
    {
        patch(14, 0xA52C, 0, {
            0xA9, // LDA #4
            0x04,
            0xEA, // NOP
        });
    }
    else
    {
        patch(14, 0xA52C, 0, {
            0xAD, // LDA $043A
            PATCH_ADDR(0x043A)
        });
    }
}


void Patcher::apply_reset_gold_xp_patch()
{
    for (int i = 0x95B9; i < 0x95D8; ++i)
    {
        m_reset_gold_xp_original_code.push_back(m_rom[12 * 0x4000 + (i - 0x8000)]);
    }
}


void Patcher::apply_reset_gold_xp_setting_patch()
{
    patch(12, 0x95B9, 0, m_reset_gold_xp_original_code);

    if (oSettings->getUserSetting("keep_gold") == "1")
    {
        // We just override the code that stores accumulator into Gold bytes.
        patch(12, 0x95CA, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
        patch(12, 0x95D0, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
        patch(12, 0x95D3, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
    }

    if (oSettings->getUserSetting("keep_xp") == "1")
    {
        // We just override the code that stores accumulator into XP bytes.
        patch(12, 0x95BE, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
        patch(12, 0x95C4, 0, {
            0xEA, // NOP
            0xEA, // NOP
            0xEA, // NOP
        });
    }
}


void Patcher::apply_xp_wingboots_patch()
{
    for (int i = 0xC599; i < 0xC599 + 4; ++i)
    {
        m_xp_wingboots_values.push_back(m_rom[15 * 0x4000 + (i - 0xC000)]);
    }
}


void Patcher::apply_xp_wingboots_setting_patch()
{
    if (oSettings->getUserSetting("xp_wingboots") == "1")
    {
        patch(15, 0xC599, 0, { 0x28, 0x28, 0x28, 0x28 });
    }
    else
    {
        patch(15, 0xC599, 0, m_xp_wingboots_values);
    }
}


void Patcher::apply_xp_speed_patch()
{
    for (int i = 0xE2C4; i < 0xE2C4 + 4; ++i)
    {
        m_xp_speed_values.push_back(m_rom[15 * 0x4000 + (i - 0xC000)]);
    }
}


void Patcher::apply_xp_speed_setting_patch()
{
    if (oSettings->getUserSetting("xp_speed") == "1")
    {
        patch(15, 0xE2C4, 0, { 0x08, 0x08, 0x08, 0x08 });
    }
    else
    {
        patch(15, 0xE2C4, 0, m_xp_speed_values);
    }
}


void Patcher::apply_pendant_setting_patch()
{
    const auto& setting = oSettings->getUserSetting("pendant");

    if (setting == "0")
    {
        patch(14, 0x8879, 0, { 0xD0 /* BNE */ });
    }
    else if (setting == "1")
    {
        patch(14, 0x8879, 0, { 0xF0 /* BEQ */ });
    }
}


void Patcher::apply_sfx_patch()
{
    patch(15, 0xD0E4, 0, {
        0x48, // PHA
        0xA9, // LDA #$0A                   C++: Play sound
        0x0A,
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0x68, // PLA
        0x8D, // STA $6000
        PATCH_ADDR(0x6000),
        0x60, // RTS
    });
}
