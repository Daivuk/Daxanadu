#pragma once

#include <cinttypes>


static const uint8_t OP_ADC_AB  = 0x6D;
static const uint8_t OP_ADC_ABX = 0x7D;
static const uint8_t OP_ADC_ABY = 0x79;
static const uint8_t OP_ADC_IMM = 0x69;
static const uint8_t OP_ADC_INX = 0x61;
static const uint8_t OP_ADC_INY = 0x71;
static const uint8_t OP_ADC_ZP  = 0x65;
static const uint8_t OP_ADC_ZPX = 0x75;

static const uint8_t OP_AND_AB  = 0x2D;
static const uint8_t OP_AND_ABX = 0x3D;
static const uint8_t OP_AND_ABY = 0x39;
static const uint8_t OP_AND_IMM = 0x29;
static const uint8_t OP_AND_INX = 0x21;
static const uint8_t OP_AND_INY = 0x31;
static const uint8_t OP_AND_ZP  = 0x25;
static const uint8_t OP_AND_ZPX = 0x35;

static const uint8_t OP_ASL_AB  = 0x0E;
static const uint8_t OP_ASL_ABX = 0x1E;
static const uint8_t OP_ASL_ACC = 0x0A;
static const uint8_t OP_ASL_ZP  = 0x06;
static const uint8_t OP_ASL_ZPX = 0x16;

static const uint8_t OP_BCC_REL = 0x90;
static const uint8_t OP_BCS_REL = 0xB0;
static const uint8_t OP_BEQ_REL = 0xF0;

static const uint8_t OP_BIT_AB  = 0x2C;
static const uint8_t OP_BIT_ZP  = 0x24;

static const uint8_t OP_BMI_REL = 0x30;
static const uint8_t OP_BNE_REL = 0xD0;
static const uint8_t OP_BPL_REL = 0x10;

static const uint8_t OP_BRK     = 0x00;

static const uint8_t OP_BVC_REL = 0x50;
static const uint8_t OP_BVS_REL = 0x70;

static const uint8_t OP_CLC     = 0x18;
static const uint8_t OP_CLD     = 0xD8;
static const uint8_t OP_CLI     = 0x58;
static const uint8_t OP_CLV     = 0xB8;

static const uint8_t OP_CMP_AB  = 0xCD;
static const uint8_t OP_CMP_ABX = 0xDD;
static const uint8_t OP_CMP_ABY = 0xD9;
static const uint8_t OP_CMP_IMM = 0xC9;
static const uint8_t OP_CMP_INX = 0xC1;
static const uint8_t OP_CMP_INY = 0xD1;
static const uint8_t OP_CMP_ZP  = 0xC5;
static const uint8_t OP_CMP_ZPX = 0xD5;

static const uint8_t OP_CPX_AB  = 0xEC;
static const uint8_t OP_CPX_IMM = 0xE0;
static const uint8_t OP_CPX_ZP  = 0xE4;

static const uint8_t OP_CPY_AB  = 0xCC;
static const uint8_t OP_CPY_IMM = 0xC0;
static const uint8_t OP_CPY_ZP  = 0xC4;

static const uint8_t OP_DEC_AB  = 0xCE;
static const uint8_t OP_DEC_ABX = 0xDE;
static const uint8_t OP_DEC_ZP  = 0xC6;
static const uint8_t OP_DEC_ZPX = 0xD6;

static const uint8_t OP_DEX     = 0xCA;
static const uint8_t OP_DEY     = 0x88;

static const uint8_t OP_EOR_AB  = 0x4D;
static const uint8_t OP_EOR_ABX = 0x5D;
static const uint8_t OP_EOR_ABY = 0x59;
static const uint8_t OP_EOR_IMM = 0x49;
static const uint8_t OP_EOR_INX = 0x41;
static const uint8_t OP_EOR_INY = 0x51;
static const uint8_t OP_EOR_ZP  = 0x45;
static const uint8_t OP_EOR_ZPX = 0x55;

static const uint8_t OP_INC_AB  = 0xEE;
static const uint8_t OP_INC_ABX = 0xFE;
static const uint8_t OP_INC_ZP  = 0xE6;
static const uint8_t OP_INC_ZPX = 0xF6;

static const uint8_t OP_INX     = 0xE8;
static const uint8_t OP_INY     = 0xC8;

static const uint8_t OP_JMP_AB  = 0x4C;
static const uint8_t OP_JMP_IN  = 0x6C;

static const uint8_t OP_JSR_AB  = 0x20;

static const uint8_t OP_LDA_AB  = 0xAD;
static const uint8_t OP_LDA_ABX = 0xBD;
static const uint8_t OP_LDA_ABY = 0xB9;
static const uint8_t OP_LDA_IMM = 0xA9;
static const uint8_t OP_LDA_INX = 0xA1;
static const uint8_t OP_LDA_INY = 0xB1;
static const uint8_t OP_LDA_ZP  = 0xA5;
static const uint8_t OP_LDA_ZPX = 0xB5;

static const uint8_t OP_LDX_AB  = 0xAE;
static const uint8_t OP_LDX_ABY = 0xBE;
static const uint8_t OP_LDX_IMM = 0xA2;
static const uint8_t OP_LDX_ZP  = 0xA6;
static const uint8_t OP_LDX_ZPY = 0xB6;

static const uint8_t OP_LDY_AB  = 0xAC;
static const uint8_t OP_LDY_ABX = 0xBC;
static const uint8_t OP_LDY_IMM = 0xA0;
static const uint8_t OP_LDY_ZP  = 0xA4;
static const uint8_t OP_LDY_ZPX = 0xB4;

static const uint8_t OP_LSR_AB  = 0x4E;
static const uint8_t OP_LSR_ABX = 0x5E;
static const uint8_t OP_LSR_ACC = 0x4A;
static const uint8_t OP_LSR_ZP  = 0x46;
static const uint8_t OP_LSR_ZPX = 0x56;

static const uint8_t OP_ORA_IMM = 0x09;
static const uint8_t OP_ORA_ZP  = 0x05;
static const uint8_t OP_ORA_ZPX = 0x15;
static const uint8_t OP_ORA_AB  = 0x0D;
static const uint8_t OP_ORA_ABX = 0x1D;
static const uint8_t OP_ORA_ABY = 0x19;
static const uint8_t OP_ORA_INX = 0x01;
static const uint8_t OP_ORA_INY = 0x11;

static const uint8_t OP_NOP     = 0xEA;

static const uint8_t OP_PHA     = 0x48;
static const uint8_t OP_PHP     = 0x08;
static const uint8_t OP_PLA     = 0x68;
static const uint8_t OP_PLP     = 0x28;

static const uint8_t OP_ROL_AB  = 0x2E;
static const uint8_t OP_ROL_ABX = 0x3E;
static const uint8_t OP_ROL_ACC = 0x2A;
static const uint8_t OP_ROL_ZP  = 0x26;
static const uint8_t OP_ROL_ZPX = 0x36;

static const uint8_t OP_ROR_AB  = 0x6E;
static const uint8_t OP_ROR_ABX = 0x7E;
static const uint8_t OP_ROR_ACC = 0x6A;
static const uint8_t OP_ROR_ZP  = 0x66;
static const uint8_t OP_ROR_ZPX = 0x76;

static const uint8_t OP_RTI     = 0x40;
static const uint8_t OP_RTS     = 0x60;

static const uint8_t OP_SBC_IMM = 0xE9;
static const uint8_t OP_SBC_ZP  = 0xE5;
static const uint8_t OP_SBC_ZPX = 0xF5;
static const uint8_t OP_SBC_AB  = 0xED;
static const uint8_t OP_SBC_ABX = 0xFD;
static const uint8_t OP_SBC_ABY = 0xF9;
static const uint8_t OP_SBC_INX = 0xE1;
static const uint8_t OP_SBC_INY = 0xF1;

static const uint8_t OP_SEC     = 0x38;
static const uint8_t OP_SED     = 0xF8;
static const uint8_t OP_SEI     = 0x78;

static const uint8_t OP_STA_AB  = 0x8D;
static const uint8_t OP_STA_ABX = 0x9D;
static const uint8_t OP_STA_ABY = 0x99;
static const uint8_t OP_STA_INX = 0x81;
static const uint8_t OP_STA_INY = 0x91;
static const uint8_t OP_STA_ZP  = 0x85;
static const uint8_t OP_STA_ZPX = 0x95;

static const uint8_t OP_STX_ZP  = 0x86;
static const uint8_t OP_STX_ZPY = 0x96;
static const uint8_t OP_STX_AB  = 0x8E;

static const uint8_t OP_STY_ZP  = 0x84;
static const uint8_t OP_STY_ZPX = 0x94;
static const uint8_t OP_STY_AB  = 0x8C;

static const uint8_t OP_TAX     = 0xAA;
static const uint8_t OP_TAY     = 0xA8;
static const uint8_t OP_TSX     = 0xBA;
static const uint8_t OP_TXA     = 0x8A;
static const uint8_t OP_TXS     = 0x9A;
static const uint8_t OP_TYA     = 0x98;

static const uint8_t OP_WAI     = 0xCB;
