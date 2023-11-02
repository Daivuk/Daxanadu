#include "cart.h"

#include <onut/Files.h>
#include <onut/Log.h>
#include <onut/onut.h>
#include <onut/Texture.h>

#include <queue>
#include <set>


static const int NES_HEADER_SIZE = 0x10;
static const int BANK_SIZE = 0x4000; // 16K. 256K PRG ROM, so 16 banks
static const int SHOP_PALETTE_OFFSET = 0x11; // Shop ids are from 0 to 9. This is the offset of 0 into the global palette array
static const int PALETTE_COUNT = 31;
static const int TILESET_COUNT = 9;
static const int DOOR_REQUIREMENT_COUNT = 64;
static const int PRG_START_ADDR = 0x8000; // Some pointers are ROM addresses.

static const int DOOR_INDEX_SHOPS           = 0x20;
static const int DOOR_INDEX_PREVIOUS_CHUNK  = 0xFE;
static const int DOOR_INDEX_NEXT_CHUNK      = 0xFF;

static const int TOOL_PALETTE = 8;

//--- Banks
// Bank 0
//   Screens for Eolis
//   Screens for Misty
//   Screens for Towns
// Bank 1
//   Screens for Trunk
//   Screens for Branches
// Bank 2
//   Screens for Dartmoor castle
//   Screens for Shops
//   Screens for Evil fortress
// Bank 3
//   Metadata for all chunks
// Bank 11
//   Palettes
//   Entities
// Bank 12
//   Item names
//   Dialog scripts
// Bank 13
//   Text blocks
// Bank 15
//   Tilesets
//   Lookup tables
//   Local and Remote Scrolling
//   Titles strings

static const int BANK_CHUNK_0_1_2   = 0;
static const int BANK_CHUNK_3_4     = 1;
static const int BANK_CHUNK_5_6_7   = 2;
static const int BANK_TILESETS      = 2;
static const int BANK_METADATA      = 3;
static const int BANK_MUSIC         = 5;
static const int BANK_TEXT_SETS     = 10;
static const int BANK_ENTITIES      = 11;
static const int BANK_SCROLLING     = 12;

//--- Level's Metadata pointers
static const int META_ATTRIBUTE_PTR                 = 0;
static const int META_BLOCK_PROPERTIES_DATA_PTR     = 1;
static const int META_SCROLL_DATA_PTR               = 2;
static const int META_DOOR_LOCATION_DATA_PTR        = 3;
static const int META_DOOR_DESTINATION_TABLE_PTR    = 4;
static const int META_ATTRIBUTES_DATA_PTR           = 5;
static const int META_TSA_TL_PTR                    = 6;
static const int META_TSA_TR_PTR                    = 7;
static const int META_TSA_BL_PTR                    = 8;
static const int META_TSA_BR_PTR                    = 9;
static const int META_PTR_COUNT                     = 10;

//--- Addresses
static const int PALETTES_ADDR              = 0x0002C010; // Bank 11
static const int ENTITY_POINTERS_ADDR       = 0x0002C220; //   "
static const int ITEM_NAMES_START_ADDR      = 0x00031B4E; // Bank 12
static const int ITEM_NAMES_END_ADDR        = 0x00031DBE; //   "
static const int TEXT_SETS_PTR_LO_ADDR      = 0x00031F7B; //   "
static const int TEXT_SETS_PTR_HI_ADDR      = 0x00032013; //   "
static const int TEXT_BLOCKS_START_ADDR     = 0x00034310; // Bank 13
static const int TEXT_BLOCKS_END_ADDR       = 0x000373CA; //   "
static const int TILESET_POINTERS_ADDR      = 0x0003CF17; // Bank 15
static const int TILESET_OFFSETS_ADDR       = 0x0003CF29; //   "
static const int TILESET_COUNTS_ADDR        = 0x0003CF32; //   "
static const int CHUNK_TILESETS_TABLE_ADDR  = 0x0003DF54; //   "
static const int CHUNK_PALETTES_TABLE_ADDR  = 0x0003DF5C; //   "
static const int LINK_KEY_TABLE_ADDR        = 0x0003E5EB; //   "
static const int LINK_CHUNK_TABLE_ADDR      = 0x0003E5F7; //   "
static const int LINK_SCREEN_TABLE_ADDR     = 0x0003E603; //   "
static const int SHOP_TILESETS_TABLE_ADDR   = 0x0003E623; //   "
static const int LOCAL_SCROLLING_ADDR       = 0x0003EA47; //   "
static const int REMOTE_SCROLLING_ADDR      = 0x0003EAAC; //   "
static const int TITLES_STRINGS_ADDR        = 0x0003F659; //   "

// Maps for chunk data lookup. Explanation bellow:
//
// Chunk IDs are stored in the order they are loaded.
// We will call this one CHUNK order:
//   0: Eolis
//   1: Misty
//   2: Towns
//   3: Trunk
//   4: Branches
//   5: Dartmoor castle
//   6: Shops
//   7: Evil fortress

// For certain stored data, we need to remap them so they are in linear order from gameplay perspective.
// We will call this one LEVEL order:
//   0: Eolis
//   1: Trunk
//   2: Misty
//   3: Branches
//   4: Dartmoor castle
//   5: Evil fortress

static const int CHUNK_TO_LEVEL_MAP[] = { 0, 2, -1, 1, 3, 4, -1, 5 };
static const int LEVEL_TO_CHUNK_MAP[] = { 0, 3, 1, 4, 5, 7 };

// Another mapping exists for other type of data, that includes towns and shops.
// We will call this one ALL_LEVELS order:
//   0: Eolis
//   1: Trunk
//   2: Misty
//   3: Towns  <--
//   4: Shops  <--
//   5: Branches
//   6: Dartmoor castle
//   7: Evil fortress

static const int CHUNK_TO_ALL_LEVELS_MAP[] = { 0, 2, 3, 1, 5, 6, 4, 7 };
static const int ALL_LEVELS_TO_CHUNK_MAP[] = { 0, 3, 1, 2, 6, 4, 5, 7 };

// For storing all of our data, we will use CHUNK order. Since it is the order
// it is stored in the cart.

std::vector<uint8_t> COLORS_RAW;
cart_t cart;


static void load_tables(FILE* f);
static void load_musics(FILE* f);
static void load_tilesets(FILE* f);
static void load_spritesets(FILE* f);
static void load_items(FILE* f);
static void load_metasprites(FILE* f);
static void load_strings(FILE* f);
static void load_palettes(FILE* f);
static void load_entities(FILE* f);
static void load_chunks(FILE* f, int bank);
static void load_scroll_data(FILE* f);
static OTextureRef create_screen_texture(const cart_screen_t& screen, cart_chunk_t& chunk);
static void build_rooms();
static void load_text_sets(FILE* f);
static void create_shops();


int bank_offset_to_rom_addr(int bank_id, int offset)
{
    return bank_id * BANK_SIZE + NES_HEADER_SIZE + offset;
}


cart_tileset_t* get_tileset(int chunk_id, int screen_id)
{
    if (chunk_id == CHUNK_SHOPS)
        return &cart.tilesets[cart.shop_tilesets_table[screen_id]];
    return &cart.tilesets[cart.chunk_tilesets_table[CHUNK_TO_ALL_LEVELS_MAP[chunk_id]]];
}


struct read_bits_ctx_t
{
    int nbit = 8;
    uint8_t val = 0;
};


static read_bits_ctx_t create_read_bits_ctx()
{
    return {8, 0};
}


static uint8_t read_bits(FILE* f, int count, read_bits_ctx_t& ctx)
{
    uint8_t ret = 0;

    for (int i = 0; i < count; ++i)
    {
        if (ctx.nbit == 8)
        {
            fread(&ctx.val, 1, 1, f);
            ctx.nbit = 0;
        }

        uint8_t bit = (ctx.val >> 7) & 1;
        ctx.val <<= 1;
        ctx.nbit++;
        ret <<= 1;
        ret |= bit;
    }

    return ret;
}


void init_cart()
{
    COLORS_RAW = onut::getFileData("FCEUX.pal");

    FILE* f = fopen("Faxanadu (U).nes", "rb");
    if (!f)
    {
        onut::showMessageBox("ERROR", "Failed to load Faxanadu (U).nes");
        onut::quit();
        return;
    }

    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    cart.rom.resize(size);
    fseek(f, 0, SEEK_SET);
    fread(cart.rom.data(), 1, size, f);

    load_musics(f);
    load_palettes(f);
    load_tilesets(f);
    load_spritesets(f);
    load_metasprites(f);
    load_tables(f);
    load_strings(f);
    load_items(f);
    load_chunks(f, BANK_CHUNK_0_1_2);
    load_chunks(f, BANK_CHUNK_3_4);
    load_chunks(f, BANK_CHUNK_5_6_7);
    create_shops();
    load_entities(f);
    load_scroll_data(f);
    load_text_sets(f);

    // Build rooms
    build_rooms();

    // Add default chunk palette for unpainted screens
    int chunk_id = 0;
    for (auto& chunk : cart.chunks)
    {
        int screen_id = 0;
        for (auto& screen : chunk.screens)
        {
            if (screen.palette) continue; // Already resolved
            if (chunk_id == CHUNK_SHOPS)
            {
                screen.palette = cart.palettes[screen_id + SHOP_PALETTE_OFFSET].palette;
            }
            else
            {
                screen.palette = cart.palettes[cart.chunks_palette_table[CHUNK_TO_ALL_LEVELS_MAP[chunk_id]]].palette;
            }
            ++screen_id;
        }
        ++chunk_id;
    }

    // Generate screens textures
    for (auto& chunk : cart.chunks)
        for (auto& screen : chunk.screens)
            screen.texture = create_screen_texture(screen, chunk);

    fclose(f);
}


static void load_tables(FILE* f)
{
    // Level table
    uint8_t chunk_table[12];
    fseek(f, LINK_CHUNK_TABLE_ADDR, SEEK_SET);
    fread(chunk_table, 1, 12, f);

    // Screen table
    uint8_t screen_table[12];
    fseek(f, LINK_SCREEN_TABLE_ADDR, SEEK_SET);
    fread(screen_table, 1, 12, f);

    // Key requirement table
    uint8_t key_table[12];
    fseek(f, LINK_KEY_TABLE_ADDR, SEEK_SET);
    fread(key_table, 1, 12, f);

    // Level tilesets table
    fseek(f, CHUNK_TILESETS_TABLE_ADDR, SEEK_SET);
    fread(cart.chunk_tilesets_table, 1, 8, f);

    // Level palette table
    fseek(f, CHUNK_PALETTES_TABLE_ADDR, SEEK_SET);
    fread(cart.chunks_palette_table, 1, 8, f);

    // Shop tilesets table
    fseek(f, SHOP_TILESETS_TABLE_ADDR, SEEK_SET);
    fread(cart.shop_tilesets_table, 1, 10, f);

    for (int i = 0; i < 6; ++i)
    {
        int chunk_id = LEVEL_TO_CHUNK_MAP[i];

        cart.chunk_link_table[chunk_id].previous_chunk = LEVEL_TO_CHUNK_MAP[chunk_table[i * 2 + 0]];
        cart.chunk_link_table[chunk_id].previous_screen = screen_table[i * 2 + 0];
        cart.chunk_link_table[chunk_id].previous_palette = cart.palettes[cart.chunks_palette_table[CHUNK_TO_ALL_LEVELS_MAP[cart.chunk_link_table[chunk_id].previous_chunk]]].palette;

        cart.chunk_link_table[chunk_id].next_chunk = LEVEL_TO_CHUNK_MAP[chunk_table[i * 2 + 1]];
        cart.chunk_link_table[chunk_id].next_screen = screen_table[i * 2 + 1];
        cart.chunk_link_table[chunk_id].next_palette = cart.palettes[cart.chunks_palette_table[CHUNK_TO_ALL_LEVELS_MAP[cart.chunk_link_table[chunk_id].next_chunk]]].palette;
        cart.chunk_link_table[chunk_id].next_chunk_key_requirement = key_table[i * 2 + 1];
    }
}


static std::map<int, cart_tile_data_t> tile_cache;


static std::vector<cart_tile_data_t> load_tileset(FILE* f, int addr, int count, int palette = -1)
{
    if (!count) return {};

    std::vector<cart_tile_data_t> tiles_data;
    tiles_data.resize(count);

    fseek(f, addr, SEEK_SET);

    uint8_t* pal = cart.palettes[TOOL_PALETTE].palette;
    if (palette != -1)
    {
        pal = cart.palettes[palette].palette;
    }
    
    int col = 0;
    for (int i = 0; i < count; ++i, addr += 16)
    {
        auto it = tile_cache.find(addr);
        if (it != tile_cache.end())
        {
            tiles_data[i] = it->second;
            fseek(f, addr + 16, SEEK_SET);
            continue;
        }

        auto& cart_data = tiles_data[i];
        fread(cart_data.data, 1, 16, f);

        // Create temporary texture for it so we can display it in our debug tool
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                int b1 = (cart_data.data[y] >> (7 - x)) & 1;
                int b2 = (cart_data.data[y + 8] >> (7 - x)) & 1;
                col = (b1) | (b2 << 1);

                tiles_data[i].cooked[(y * 8 + x) * 4 + 0] = COLORS_RAW[pal[col] * 3 + 0];
                tiles_data[i].cooked[(y * 8 + x) * 4 + 1] = COLORS_RAW[pal[col] * 3 + 1];
                tiles_data[i].cooked[(y * 8 + x) * 4 + 2] = COLORS_RAW[pal[col] * 3 + 2];
                tiles_data[i].cooked[(y * 8 + x) * 4 + 3] = 255;
            }
        }

        tiles_data[i].texture = OTexture::createFromData(tiles_data[i].cooked, {8, 8}, false);
        tile_cache[addr] = tiles_data[i];
    }

    return tiles_data;
}


static void load_tilesets(FILE* f)
{
    std::vector<uint16_t> tileset_pointers;
    std::vector<uint8_t> tileset_offsets;
    std::vector<uint8_t> tile_counts;

    tileset_pointers.resize(TILESET_COUNT);
    tileset_offsets.resize(TILESET_COUNT);
    tile_counts.resize(TILESET_COUNT);

    fseek(f, TILESET_POINTERS_ADDR, SEEK_SET);
    fread(tileset_pointers.data(), 2, TILESET_COUNT, f);
    fseek(f, TILESET_OFFSETS_ADDR, SEEK_SET);
    fread(tileset_offsets.data(), 1, TILESET_COUNT, f);
    fseek(f, TILESET_COUNTS_ADDR, SEEK_SET);
    fread(tile_counts.data(), 1, TILESET_COUNT, f);

    for (int i = 0; i < TILESET_COUNT; ++i)
    {
        const int tile_count = (tile_counts[i] & 0xFF) * 0x10; // In rows of 16 tiles
        const int offset = (tileset_offsets[i] & 0x0F) << 4;
        const int tiles_addr = bank_offset_to_rom_addr(BANK_TILESETS, tileset_pointers[i]);

        cart_tileset_t tileset;
        tileset.offset = offset;
        tileset.tiles_data = load_tileset(f, tiles_addr, tile_count);

        cart.tilesets.push_back(tileset);
    }

    // Inventory item tiles
    {
        cart_tileset_t tileset;
        tileset.offset = 0;
        tileset.tiles_data = load_tileset(f, 0x00028010 + 0x50 * 16, 256/* 128 + 16*/);
        cart.tilesets.push_back(tileset);
    }
}

static void load_spritesets(FILE* f)
{
    static const int ENTITY_TYPE_COUNT = 101;

    cart.entity_types.resize(ENTITY_TYPE_COUNT);

    // Entity type infos
    {
        std::vector<uint8_t> sprite_sizes_h;
        sprite_sizes_h.resize(7);
        fseek(f, 0x3B4E1, SEEK_SET);
        fread(sprite_sizes_h.data(), 1, sprite_sizes_h.size(), f);

        std::vector<uint8_t> sprite_sizes_v;
        sprite_sizes_v.resize(7);
        fseek(f, 0x3B4E8, SEEK_SET);
        fread(sprite_sizes_v.data(), 1, sprite_sizes_v.size(), f);

        std::vector<uint8_t> sprites_sizes;
        sprites_sizes.resize(ENTITY_TYPE_COUNT);
        fseek(f, 0x3B4EF, SEEK_SET);
        fread(sprites_sizes.data(), 1, sprites_sizes.size(), f);

        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            cart_entity_type_t& entity_type = cart.entity_types[i];

            entity_type.width = (int)(sprite_sizes_h[sprites_sizes[i]] + 1) / 8;
            entity_type.height = (int)(sprite_sizes_v[sprites_sizes[i]] + 1) / 8;
            entity_type.spritesheet_id = i;
        }

        // GFXes and drops use a static spritesheet on the chr rom
        cart.entity_types[1].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[2].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[19].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[20].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[81].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[83].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[84].spritesheet_id = ENTITY_TYPE_COUNT;
        cart.entity_types[100].spritesheet_id = ENTITY_TYPE_COUNT;
    }

    // Sprite sheets
    {
        std::vector<uint8_t> sprites_tiles_count;
        sprites_tiles_count.resize(ENTITY_TYPE_COUNT);
        fseek(f, 0x3CE2B, SEEK_SET);
        fread(sprites_tiles_count.data(), 1, sprites_tiles_count.size(), f);

        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            //if (i == 1) __debugbreak();
            int offset = 0x18010;
            if (i >= 55) offset += 0x4000;

            uint16_t sprite_pointer_table_offset;
            fseek(f, offset, SEEK_SET);
            fread(&sprite_pointer_table_offset, 2, 1, f);

            int index = i;
            if (i >= 55) index -= 55; // Falls into another bank

            uint16_t sprite_data_pointer;
            fseek(f, offset + (int)sprite_pointer_table_offset + index * 2, SEEK_SET);
            fread(&sprite_data_pointer, 2, 1, f);

		    int tile_count = (int)sprites_tiles_count[i];

            cart_tileset_t tileset;
            tileset.offset = 0;
            tileset.addr = offset + (int)sprite_data_pointer;
            if (tile_count > 0)
                tileset.tiles_data = load_tileset(f, offset + (int)sprite_data_pointer, tile_count, ENTITY_PALETTE);
            cart.spritesets.push_back(tileset);
        }
    }

    // Static GFX for drops and magic
    {
        cart_tileset_t tileset;
        tileset.offset = 0x40;
        tileset.tiles_data = load_tileset(f, 0x000201B0 + 4 + 0xA6 * 16, 0xEF - 0xA6);
        cart.spritesets.push_back(tileset);
    }

    // Character animation and projectile sprites
    //{
    //    cart_tileset_t tileset;
    //    tileset.offset = 0;
    //    tileset.tiles_data = load_tileset(f, 0x000201B0 + 4, 256 + 8);
    //    cart.spritesets.push_back(tileset);
    //}
    
    // Title and UI sprites
    {
        cart_tileset_t tileset;
        tileset.offset = 0;
        tileset.tiles_data = load_tileset(f, 0x00028010, 512 + 256 - 3 * 16 - 8 + 1);
        cart.spritesets.push_back(tileset);
    }

    //// Health bar
    //{
    //    cart_tileset_t tileset;
    //    tileset.offset = 0;
    //    tileset.tiles_data = load_tileset(f, 0x0003F800 + 16 * 16 * 3 + 4 * 16 + 6, 7);
    //    cart.spritesets.push_back(tileset);
    //}

    // Animation frames
    {
        uint16_t ptr;

        // Phase table
        std::vector<uint8_t> phase_index_table;
        phase_index_table.resize(ENTITY_TYPE_COUNT);
        fseek(f, 0x00038CAF, SEEK_SET);
        fread(phase_index_table.data(), 1, ENTITY_TYPE_COUNT, f);

        // Frames pointer
        int frames_pointer_addr = bank_offset_to_rom_addr(7, 6); // 0x1C010;
        fseek(f, frames_pointer_addr, SEEK_SET);
        fread(&ptr, 2, 1, f);
        int frames_pointer = bank_offset_to_rom_addr(7, (int)ptr);

        // Read all pointers
        std::vector<int> pointers;
        pointers.resize(ENTITY_TYPE_COUNT);
        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            int phase_index = phase_index_table[i];
            fseek(f, frames_pointer + phase_index * 2, SEEK_SET);
            fread(&ptr, 2, 1, f);
            pointers[i] = bank_offset_to_rom_addr(7, (int)ptr);
        }

        // order pointers to find gaps between them so we know how big each is
        struct pointer_gap_t
        {
            int addr;
            int index;
            int gap;
        };
        std::vector<pointer_gap_t> pointer_gaps;
        pointer_gaps.resize(pointers.size());
        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            pointer_gaps[i] = {pointers[i], i, 0};
        }
        std::sort(pointer_gaps.begin(), pointer_gaps.end(), [](const pointer_gap_t& a, const pointer_gap_t& b) -> bool
        {
            return a.addr < b.addr;
        });
        for (int i = 0; i < ENTITY_TYPE_COUNT - 1; ++i)
        {
            pointer_gaps[i].gap = pointer_gaps[i + 1].addr - pointer_gaps[i].addr;
        }
        // Order back so we can access by index
        std::sort(pointer_gaps.begin(), pointer_gaps.end(), [](const pointer_gap_t& a, const pointer_gap_t& b) -> bool
        {
            return a.index < b.index;
        });
        // Those with 0 gap, find the next one with a gap, because they are same address (We can probably find a table for this somewhere?)
        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            if (pointer_gaps[i].gap == 0)
            {
                for (int j = i + 1; j < ENTITY_TYPE_COUNT; ++j)
                {
                    if (pointer_gaps[j].gap != 0)
                    {
                        pointer_gaps[i].gap = pointer_gaps[j].gap;
                        break;
                    }
                }
            }
        }

        // Read frames
        for (int i = 0; i < ENTITY_TYPE_COUNT; ++i)
        {
            int size = pointer_gaps[i].gap;
            cart_entity_type_t& entity_type = cart.entity_types[i];
            entity_type.raw_frames.resize(size);

            const auto& spriteset = cart.spritesets[entity_type.spritesheet_id];

            fseek(f, pointers[i], SEEK_SET);
            fread(entity_type.raw_frames.data(), 1, size, f);

            int idx = 0;
            while (idx + 4 < size)
            {
                // Frame header
                struct frame_header_t
                {
                    uint8_t w : 4;
                    uint8_t h : 4;
                    int8_t offset_x : 8;
                    int8_t offset_y : 8;
                    uint8_t unknown3 : 8;
                } header;

                header = *reinterpret_cast<frame_header_t*>(&entity_type.raw_frames[idx]);
                idx += sizeof(frame_header_t);

                cart_frame_t frame;
                frame.w = header.w + 1;
                frame.h = header.h + 1;
                frame.offset_x = header.offset_x;
                frame.offset_y = header.offset_y;

                int count = frame.w * frame.h;
                int sprite_id = 0;
                while (count-- && idx < size)
                {
                    auto byte = entity_type.raw_frames[idx++];
                    if (idx >= size) break;
                    if (byte == 0xFF)
                    {
                        ++sprite_id;
                        continue; // Empty sprite
                    }

                    cart_sprite_t sprite;

                    sprite.sprite_id = byte;
                    auto flags = entity_type.raw_frames[idx++];
                    auto pal = flags & 0b11;
                    sprite.palette = cart.palettes[ENTITY_PALETTE].palette + pal * 4;
                    sprite.offset_x = (sprite_id % frame.w) * 8;
                    sprite.offset_y = (sprite_id / frame.w) * 8;
                    sprite.flip_h = (flags & 0b01000000) ? true : false;
                    sprite.flip_v = (flags & 0b10000000) ? true : false;
                    frame.sprites.push_back(sprite);

                    ++sprite_id;
                }

                // Create a texture for the frame
                std::vector<uint8_t> img_data;
                img_data.resize(frame.w * 8 * frame.h * 8 * 4);
                for (const auto& sprite : frame.sprites)
                {
                    auto sprite_id = sprite.sprite_id - spriteset.offset;
                    if (sprite_id < 0 || sprite_id >= (int)spriteset.tiles_data.size()) continue; // oops? Could be from invalid sprites
                    const auto& tile_data = spriteset.tiles_data[sprite_id];
                    for (int y = 0; y < 8; ++y)
                    {
                        int dst_y = sprite.offset_y + (sprite.flip_v ? (7 - y) : y);
                        for (int x = 0; x < 8; ++x)
                        {
                            int dst_x = sprite.offset_x + (sprite.flip_h ? (7 - x) : x);
                            int dst_k = dst_y * frame.w * 8 * 4 + dst_x * 4;

                            int b1 = (tile_data.data[y] >> (7 - x)) & 1;
                            int b2 = (tile_data.data[y + 8] >> (7 - x)) & 1;
                            int col = (b1) | (b2 << 1);

                            img_data[dst_k + 0] = COLORS_RAW[sprite.palette[col] * 3 + 0];
                            img_data[dst_k + 1] = COLORS_RAW[sprite.palette[col] * 3 + 1];
                            img_data[dst_k + 2] = COLORS_RAW[sprite.palette[col] * 3 + 2];
                            img_data[dst_k + 3] = (col == 0) ? 0 : 255;
                        }
                    }
                }
                frame.texture = OTexture::createFromData(img_data.data(), {frame.w * 8, frame.h * 8}, false);

                entity_type.frames.push_back(frame);
            }
        }
    }

    // King
    // 0x0002124A
    // 8A 8B 8C 8D 8E 8F 90 91 92 93 96 94 95 98 97 60 9A 99 63 65 9B 9C 9D FF
    // 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17
    //
    // YYYY YYYY
    // TTTT TTTT
    // VHB- --PP
    // XXXX XXXX
    //
    // 17 00 01 01 00 17
    // 17 02 03 03 02 17
    // 17 04 05 05 04 17
    // 17 08 09 09 08 17
    // 0A 0B 0C 0C 0B 0A
    
    // Portrait tilesets
    {
        // Cache global Portrait spritesheet
        cart_tileset_t portraits_tileset;
        portraits_tileset.offset = 0;
        portraits_tileset.addr = 0x0002137B;
        portraits_tileset.tiles_data = load_tileset(f, 0x0002137B, 255);
        cart.spritesets.push_back(portraits_tileset);

        const int PORTRAIT_COUNT = 11;
        fseek(f, 0x00021234, SEEK_SET);
        std::vector<uint16_t> portrait_tileset_pointers;
        portrait_tileset_pointers.resize(PORTRAIT_COUNT);
        fread(portrait_tileset_pointers.data(), 2, PORTRAIT_COUNT, f);

        for (const auto portrait_tileset_pointer : portrait_tileset_pointers)
        {
            int bank_offset = bank_offset_to_rom_addr(8, (int)portrait_tileset_pointer);

            cart_tileset_t tileset;
            tileset.offset = 0;
            uint8_t tile = 0;
            int i = 0;
            while (tile != 0xFF)
            {
                fseek(f, bank_offset + i++, SEEK_SET);
                fread(&tile, 1, 1, f);
                tileset.tiles_data.push_back(load_tileset(f, 0x0002137B + (int)tile * 16, 1)[0]);
            }
            cart.portrait_tilesets.push_back(tileset);
        }
    }
}


static void load_item(FILE* f, int id)
{
    int sequential_id = (int)cart.item_types.size();

    cart_item_type_t item_type;

    item_type.id = id;
    item_type.name = cart.item_names[sequential_id];

    int tile_table_addr = 0x0002B4B0 + sequential_id * 4;
    fseek(f, tile_table_addr, SEEK_SET);
    fread(item_type.tiles_raw, 1, 4, f);

    uint8_t img_data[16 * 16 * 4];

    // Generate texture for it
    const auto& tileset = cart.tilesets[9];
    auto pal = cart.palettes[0].palette + 12;
    for (int i = 0; i < 4; ++i)
    {
        uint8_t tile_id = item_type.tiles_raw[i];
        const auto& tile_data = tileset.tiles_data[tile_id];
        for (int y = 0; y < 8; ++y)
        {
            int dst_y = (i / 2) * 8 + y;
            for (int x = 0; x < 8; ++x)
            {
                int dst_x = (i % 2) * 8 + x;
                int dst_k = dst_y * 16 * 4 + dst_x * 4;

                int b1 = (tile_data.data[y] >> (7 - x)) & 1;
                int b2 = (tile_data.data[y + 8] >> (7 - x)) & 1;
                int col = (b1) | (b2 << 1);

                img_data[dst_k + 0] = COLORS_RAW[pal[col] * 3 + 0];
                img_data[dst_k + 1] = COLORS_RAW[pal[col] * 3 + 1];
                img_data[dst_k + 2] = COLORS_RAW[pal[col] * 3 + 2];
                img_data[dst_k + 3] = 255;
            }
        }
    }

    item_type.texture = OTexture::createFromData(img_data, {16, 16}, false);

    cart.item_types.push_back(item_type);
}


static void load_items(FILE* f)
{
    // Weapons
    for (int i = 0; i <= 3; ++i)
        load_item(f, i);

    // Shields
    for (int i = 32; i <= 35; ++i)
        load_item(f, i);

    // Armors
    for (int i = 64; i <= 67; ++i)
        load_item(f, i);

    // Magics
    for (int i = 96; i <= 100; ++i)
        load_item(f, i);

    // Items
    for (int i = 128; i <= 149; ++i)
        load_item(f, i);
}


static std::string get_fixed_string(FILE* f, int addr)
{
    char raw[16];
    fseek(f, addr, SEEK_SET);
    fread(raw, 1, 16, f);
    int j = 14;
    for (; j >= 0; --j)
    {
        if (raw[j] != 0x20)
        {
            raw[j + 1] = '\0';
            ++j;
            break;
        }
    }
    raw[15] = '\0';

    return raw;
}


static void load_strings(FILE* f)
{
    // Multiline strings
    {
        fseek(f, TEXT_BLOCKS_START_ADDR, SEEK_SET);
        int addr = TEXT_BLOCKS_START_ADDR;
        do
        {
            cart_string_t string;
            string.address = addr;
            string.len = 0;
            string.text_blocks.push_back("");

            int c = 0;
            int cur_line_len = 0;
            do
            {
                fread(&c, 1, 1, f);
                if (c == 0xFC)
                {
                    string.text_blocks.push_back("");
                    cur_line_len = 0;
                }
                else if (c == 0xFD)
                {
                    string.text_blocks.back() += ' ';
                    cur_line_len++;
                }
                else if (c == 0xFE)
                {
                    string.text_blocks.back() += '\n';
                    cur_line_len = 0;
                }
                else if (c != 0xFF)
                {
                    string.text_blocks.back() += (char)c;
                    cur_line_len++;
                }

                if (cur_line_len == 16) // Forced line wrap
                {
                    string.text_blocks.back() += '\n';
                    cur_line_len = 0;
                }

            } while (c != 0xFF);

            cart.strings.push_back(string);

            addr = ftell(f);
        } while (addr < TEXT_BLOCKS_END_ADDR);
    }

    // Titles
    for (int i = 0; i < 15; ++i)
        cart.titles.push_back(get_fixed_string(f, TITLES_STRINGS_ADDR + i * 16));

    // Item names
    for (int i = ITEM_NAMES_START_ADDR; i < ITEM_NAMES_END_ADDR; i += 16)
        cart.item_names.push_back(get_fixed_string(f, i));
}


static void load_palettes(FILE* f)
{
    fseek(f, PALETTES_ADDR, SEEK_SET);
    for (int i = 0; i < PALETTE_COUNT; ++i)
    {
        cart_palette_t palette;
        fread(&palette, 1, 16, f);
        cart.palettes.push_back(palette);
    }
}


static void load_metasprites(FILE* f)
{
}


static std::vector<int> read_pointer_table(FILE* f, int start, int offset)
{
    std::vector<uint16_t> pointers;
    uint16_t end;
    fseek(f, offset + start, SEEK_SET);
    fread(&end, 2, 1, f);
    pointers.push_back(end);
    const int count = ((end + offset) - (start + offset)) / 2;
    pointers.resize(count);
    if (count > 1)
        fread(pointers.data() + 1, 2, count - 1, f);

    std::vector<int> ret;
    for (int i = 0; i < (int)pointers.size(); ++i)
        ret.push_back((int)pointers[i]/* + NES_HEADER_SIZE*/);

    return ret;
}


static cart_screen_t read_screen(FILE* f, int screen_pointer)
{
    uint8_t control;
    int i = 0;
    cart_screen_t screen;

    screen.blocks.resize(SCREEN_BLOCKS_WIDTH * SCREEN_BLOCKS_HEIGHT);
    fseek(f, screen_pointer, SEEK_SET);

    read_bits_ctx_t ctx = create_read_bits_ctx();
    for (int y = 0; y < SCREEN_BLOCKS_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_BLOCKS_WIDTH; ++x, ++i)
        {
            control = read_bits(f, 2, ctx);
            switch (control)
            {
                case 0b00:
                    screen.blocks[i] = screen.blocks[i - 1];
                    break;
                case 0b01:
                    screen.blocks[i] = screen.blocks[i - SCREEN_BLOCKS_WIDTH];
                    break;
                case 0b10:
                    screen.blocks[i] = screen.blocks[i - SCREEN_BLOCKS_WIDTH - 1];
                    break;
                case 0b11:
                    screen.blocks[i] = read_bits(f, 8, ctx);
                    break;
            }
        }
    }

    return screen;
}


static uint8_t screen_texture_data[SCREEN_BLOCKS_WIDTH * 16 * SCREEN_BLOCKS_HEIGHT * 16 * 4];


static OTextureRef create_screen_texture(const cart_screen_t& screen, cart_chunk_t& chunk)
{
    if (screen.id == -1) return nullptr;

    int offset = screen.tileset->offset;
    auto tile_data = screen.tileset->tiles_data.data();
    int tile_count = (int)screen.tileset->tiles_data.size();
    auto pal = screen.palette;
    int col;
    uint8_t* screen_ptr;

    memset(screen_texture_data, 0, sizeof(screen_texture_data));

    const uint8_t* block_ptr = screen.blocks.data();
    for (int by = 0; by < SCREEN_BLOCKS_HEIGHT; ++by)
    {
        for (int bx = 0; bx < SCREEN_BLOCKS_WIDTH; ++bx, ++block_ptr)
        {
            const int block_id = *block_ptr;
            auto& block = chunk.blocks[block_id];

            if (block.top_left - offset < 0) block.top_left = offset + (int)screen.tileset->tiles_data.size() - 1;
            if (block.top_right - offset < 0) block.top_right = offset + (int)screen.tileset->tiles_data.size() - 1;
            if (block.bottom_left - offset < 0) block.bottom_left = offset + (int)screen.tileset->tiles_data.size() - 1;
            if (block.bottom_right - offset < 0) block.bottom_right = offset + (int)screen.tileset->tiles_data.size() - 1;

            const auto tl = tile_data + (block.top_left - offset);
            const auto tr = tile_data + (block.top_right - offset);
            const auto bl = tile_data + (block.bottom_left - offset);
            const auto br = tile_data + (block.bottom_right - offset);

            pal = screen.palette + block.attrib_tl * 4;

            screen_ptr = screen_texture_data + by * SCREEN_BLOCKS_WIDTH * 16 * 16 * 4 + bx * 16 * 4;
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    int b1 = (tl->data[y] >> (7 - x)) & 1;
                    int b2 = (tl->data[y + 8] >> (7 - x)) & 1;
                    col = (b1) | (b2 << 1);

                    screen_ptr[0] = COLORS_RAW[pal[col] * 3 + 0];
                    screen_ptr[1] = COLORS_RAW[pal[col] * 3 + 1];
                    screen_ptr[2] = COLORS_RAW[pal[col] * 3 + 2];
                    screen_ptr[3] = 255;

                    screen_ptr += 4;
                }
                screen_ptr += (SCREEN_BLOCKS_WIDTH) * 16 * 4 - 8 * 4;
            }

            screen_ptr = screen_texture_data + by * SCREEN_BLOCKS_WIDTH * 16 * 16 * 4 + bx * 16 * 4 + 8 * 4;
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    int b1 = (tr->data[y] >> (7 - x)) & 1;
                    int b2 = (tr->data[y + 8] >> (7 - x)) & 1;
                    col = (b1) | (b2 << 1);

                    screen_ptr[0] = COLORS_RAW[pal[col] * 3 + 0];
                    screen_ptr[1] = COLORS_RAW[pal[col] * 3 + 1];
                    screen_ptr[2] = COLORS_RAW[pal[col] * 3 + 2];
                    screen_ptr[3] = 255;

                    screen_ptr += 4;
                }
                screen_ptr += (SCREEN_BLOCKS_WIDTH) * 16 * 4 - 8 * 4;
            }

            screen_ptr = screen_texture_data + by * SCREEN_BLOCKS_WIDTH * 16 * 16 * 4 + bx * 16 * 4 + SCREEN_BLOCKS_WIDTH * 16 * 8 * 4;
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    int b1 = (bl->data[y] >> (7 - x)) & 1;
                    int b2 = (bl->data[y + 8] >> (7 - x)) & 1;
                    col = (b1) | (b2 << 1);

                    screen_ptr[0] = COLORS_RAW[pal[col] * 3 + 0];
                    screen_ptr[1] = COLORS_RAW[pal[col] * 3 + 1];
                    screen_ptr[2] = COLORS_RAW[pal[col] * 3 + 2];
                    screen_ptr[3] = 255;

                    screen_ptr += 4;
                }
                screen_ptr += (SCREEN_BLOCKS_WIDTH) * 16 * 4 - 8 * 4;
            }

            screen_ptr = screen_texture_data + by * SCREEN_BLOCKS_WIDTH * 16 * 16 * 4 + bx * 16 * 4 + 8 * 4 + SCREEN_BLOCKS_WIDTH * 16 * 8 * 4;
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    int b1 = (br->data[y] >> (7 - x)) & 1;
                    int b2 = (br->data[y + 8] >> (7 - x)) & 1;
                    col = (b1) | (b2 << 1);

                    screen_ptr[0] = COLORS_RAW[pal[col] * 3 + 0];
                    screen_ptr[1] = COLORS_RAW[pal[col] * 3 + 1];
                    screen_ptr[2] = COLORS_RAW[pal[col] * 3 + 2];
                    screen_ptr[3] = 255;

                    screen_ptr += 4;
                }
                screen_ptr += (SCREEN_BLOCKS_WIDTH) * 16 * 4 - 8 * 4;
            }
        }
    }

    return OTexture::createFromData(screen_texture_data, {SCREEN_BLOCKS_WIDTH * 16, SCREEN_BLOCKS_HEIGHT * 16}, false);
}


static void load_chunk_metadata(FILE* f, cart_chunk_t& chunk, int chunk_id)
{
    int base_offset = bank_offset_to_rom_addr(BANK_METADATA, 0);

    // Load a bunch of pointers first (we shouldn't be redoing this for each chunks)
    uint16_t meta_table_pointer;
    fseek(f, base_offset, SEEK_SET);
    fread(&meta_table_pointer, 2, 1, f);

    uint16_t meta_pointer;
    fseek(f, base_offset + meta_table_pointer + CHUNK_TO_ALL_LEVELS_MAP[chunk_id] * 2, SEEK_SET);
    fread(&meta_pointer, 2, 1, f);

    uint16_t meta_pointers[META_PTR_COUNT];
    fseek(f, base_offset + meta_pointer, SEEK_SET);
    fread(meta_pointers, 2, META_PTR_COUNT, f);

    int block_count = meta_pointers[META_TSA_TR_PTR] - meta_pointers[META_TSA_TL_PTR];
    int door_count = (meta_pointers[META_DOOR_DESTINATION_TABLE_PTR] - meta_pointers[META_DOOR_LOCATION_DATA_PTR]) / 4;

    uint16_t attribute_pointer;
    fseek(f, base_offset + meta_pointers[META_ATTRIBUTE_PTR], SEEK_SET);
    fread(&attribute_pointer, 2, 1, f);

    // Attributes
    fseek(f, base_offset + attribute_pointer, SEEK_SET);
    std::vector<uint8_t> raw_attributes;
    raw_attributes.resize(block_count);
    fread(raw_attributes.data(), 1, block_count, f);
    chunk.blocks.resize(block_count);
    for (int i = 0; i < block_count; ++i)
    {
        int attribute = raw_attributes[i];
		chunk.blocks[i].attrib_tl = (attribute >> 6) & 3;
		chunk.blocks[i].attrib_tr = (attribute >> 4) & 3;
		chunk.blocks[i].attrib_bl = (attribute >> 2) & 3;
		chunk.blocks[i].attrib_br = (attribute >> 0) & 3;
    }

    // Properties
    fseek(f, base_offset + meta_pointers[META_BLOCK_PROPERTIES_DATA_PTR], SEEK_SET);
    std::vector<uint8_t> raw_properties;
    raw_properties.resize(block_count);
    fread(raw_properties.data(), 1, block_count, f);
    for (int i = 0; i < block_count; ++i)
    {
		chunk.blocks[i].properties = raw_properties[i];
    }

    // Scroll data
    std::vector<cart_scroll_t> raw_scrolls;
    raw_scrolls.resize(chunk.screens.size());
    fseek(f, base_offset + meta_pointers[META_SCROLL_DATA_PTR], SEEK_SET);
    fread(raw_scrolls.data(), 1, sizeof(cart_scroll_t) * raw_scrolls.size(), f);
    for (int i = 0; i < (int)chunk.screens.size(); ++i)
        chunk.screens[i].scroll = raw_scrolls[i];
    
    // Door locations
    auto size = sizeof(cart_door_location_t);
    std::vector<cart_door_location_t> door_locations;
    door_locations.resize(door_count);
    fseek(f, base_offset + meta_pointers[META_DOOR_LOCATION_DATA_PTR], SEEK_SET);
    fread(door_locations.data(), 1, sizeof(cart_door_location_t) * door_count, f);

    // Door destinations
    std::vector<cart_door_destination_t> door_destiniations;
    door_destiniations.resize(64);
    fseek(f, base_offset + meta_pointers[META_DOOR_DESTINATION_TABLE_PTR], SEEK_SET);
    fread(door_destiniations.data(), 1, sizeof(cart_door_destination_t) * DOOR_REQUIREMENT_COUNT, f);

    for (int i = 0; i < door_count; ++i)
    {
        auto index = door_locations[i].index;

        cart_door_t door;

        door.x = door_locations[i].x;
        door.y = door_locations[i].y;
        door.raw_location = door_locations[i];

        if (index < DOOR_INDEX_SHOPS) // Teleport within the same chunk
        {
            door.raw_destination = door_destiniations[index];
            door.dest_palette = cart.palettes[door_destiniations[index].palette].palette;
            door.dest_chunk_id = chunk_id;
            door.dest_screen_id = door_destiniations[index].screen_id;
            door.dest_x = door_locations[i].dest_x;
            door.dest_y = door_locations[i].dest_y;
            if (door_destiniations[index].key != 0 && door_destiniations[index].key != 0xFF)
                door.key = door_destiniations[index].key;
        }
        else if (index < DOOR_INDEX_PREVIOUS_CHUNK) // Shop
        {
            if (chunk_id == CHUNK_TOWNS) index -= DOOR_INDEX_SHOPS;
            
            door.raw_destination = door_destiniations[index];
            door.dest_palette = cart.palettes[door_destiniations[index].palette + SHOP_PALETTE_OFFSET].palette;
            door.dest_chunk_id = 6;
            door.dest_screen_id = door_destiniations[index].screen_id;
            door.dest_shop_id = door_destiniations[index].shop_screen_id;
            door.dest_x = 14;
            door.dest_y = 8; // He spawns in the air and falls down 1 tile
            if (door_destiniations[index].key != 0 && door_destiniations[index].key != 0xFF)
                door.key = door_destiniations[index].key;

            // Add this screen
            cart.shop_doors.push_back(door);
        }
        else if (index == DOOR_INDEX_PREVIOUS_CHUNK) // Previous chunk
        {
            const auto& link = cart.chunk_link_table[chunk_id];

            door.dest_palette = link.previous_palette;
            door.dest_chunk_id = link.previous_chunk;
            door.dest_screen_id = link.previous_screen;
            door.dest_x = door_locations[i].dest_x;
            door.dest_y = door_locations[i].dest_y;
        }
        else if (index == DOOR_INDEX_NEXT_CHUNK) // Next chunk
        {
            const auto& link = cart.chunk_link_table[chunk_id];

            door.dest_palette = link.next_palette;
            door.dest_chunk_id = link.next_chunk;
            door.dest_screen_id = link.next_screen;
            door.dest_x = door_locations[i].dest_x;
            door.dest_y = door_locations[i].dest_y;
            door.key = link.next_chunk_key_requirement;
        }

        auto& screen = chunk.screens[door_locations[i].screen_id];
        screen.doors.push_back(door);
    }

    // Blocks (TSA, Tile Square Assembly)
    std::vector<uint8_t> raw_tsa;
    raw_tsa.resize(block_count * 4);
    fseek(f, base_offset + meta_pointers[META_TSA_TL_PTR], SEEK_SET);
    fread(raw_tsa.data() + block_count * 0, 1, block_count, f);
    fseek(f, base_offset + meta_pointers[META_TSA_TR_PTR], SEEK_SET);
    fread(raw_tsa.data() + block_count * 1, 1, block_count, f);
    fseek(f, base_offset + meta_pointers[META_TSA_BL_PTR], SEEK_SET);
    fread(raw_tsa.data() + block_count * 2, 1, block_count, f);
    fseek(f, base_offset + meta_pointers[META_TSA_BR_PTR], SEEK_SET);
    fread(raw_tsa.data() + block_count * 3, 1, block_count, f);
    chunk.blocks.resize(block_count);
    for (int i = 0; i < block_count; ++i)
    {
        chunk.blocks[i].top_left = raw_tsa[block_count * 0 + i];
        chunk.blocks[i].top_right = raw_tsa[block_count * 1 + i];
        chunk.blocks[i].bottom_left = raw_tsa[block_count * 2 + i];
        chunk.blocks[i].bottom_right = raw_tsa[block_count * 3 + i];
    }
}


void load_scroll_data(FILE* f)
{
    struct local_scrolling_t // We're missing information about which direction to go that will trigger this
    {
        uint8_t screen_id;
        uint8_t dest_screen_id;
        uint8_t dest_x : 4;
        uint8_t dest_y : 4;
        uint8_t dest_palette_id;
    };

    struct remote_scrolling_t // We're missing information about which direction to go that will trigger this
    {
        uint8_t screen_id;
        uint8_t all_chunks_id;
        uint8_t dest_screen_id;
        uint8_t dest_x : 4;
        uint8_t dest_y : 4;
        uint8_t dest_palette_id;
    };

    // Local scrolling pointers
    std::vector<uint16_t> local_scrolling_pointers;
    local_scrolling_pointers.resize(8);
    fseek(f, LOCAL_SCROLLING_ADDR, SEEK_SET);
    fread(local_scrolling_pointers.data(), 2, 8, f);

    // Remote scrolling pointers
    std::vector<uint16_t> remote_scrolling_pointers;
    remote_scrolling_pointers.resize(8);
    fseek(f, REMOTE_SCROLLING_ADDR, SEEK_SET);
    fread(remote_scrolling_pointers.data(), 2, 8, f);

    for (auto& chunk : cart.chunks)
    {
        // Read local scrolling data (To another screen within the same chunk)
        uint16_t local_scrolling_pointer = local_scrolling_pointers[CHUNK_TO_ALL_LEVELS_MAP[chunk.id]];
        int local_scrolling_base_offset = bank_offset_to_rom_addr(BANK_SCROLLING, local_scrolling_pointer);
        std::vector<local_scrolling_t> local_scrollings;
        fseek(f, local_scrolling_base_offset, SEEK_SET);
        while (true)
        {
            local_scrolling_t local_scrolling;
            fread(&local_scrolling, 1, sizeof(local_scrolling_t), f);
            if (local_scrolling.screen_id == 0xFF) break; // End of data
            local_scrollings.push_back(local_scrolling);
        }

        // Read remote scrolling data (To another chunk)
        uint16_t remote_scrolling_pointer = remote_scrolling_pointers[CHUNK_TO_ALL_LEVELS_MAP[chunk.id]];
        int remote_scrolling_base_offset = bank_offset_to_rom_addr(BANK_SCROLLING, remote_scrolling_pointer);
        auto ss = sizeof(remote_scrolling_t);
        std::vector<remote_scrolling_t> remote_scrollings;
        fseek(f, remote_scrolling_base_offset, SEEK_SET);
        while (true)
        {
            remote_scrolling_t remote_scrolling;
            fread(&remote_scrolling, 1, sizeof(remote_scrolling_t), f);
            if (remote_scrolling.screen_id == 0xFF) break; // End of data
            remote_scrollings.push_back(remote_scrolling);
        }

        // Create pathways from the raw data
        for (const auto& local_scrolling : local_scrollings)
        {
            cart_pathway_t pathway;

            pathway.dest_palette = cart.palettes[local_scrolling.dest_palette_id].palette;
            pathway.dest_chunk_id = chunk.id;
            pathway.dest_screen_id = local_scrolling.dest_screen_id;
            pathway.dest_x = local_scrolling.dest_x;
            pathway.dest_y = local_scrolling.dest_y;

            auto& screen = chunk.screens[local_scrolling.screen_id];
            screen.pathways.push_back(pathway);
        }

        for (const auto& remote_scrolling : remote_scrollings)
        {
            cart_pathway_t pathway;

            pathway.dest_palette = cart.palettes[remote_scrolling.dest_palette_id].palette;
            pathway.dest_chunk_id = ALL_LEVELS_TO_CHUNK_MAP[remote_scrolling.all_chunks_id];
            pathway.dest_screen_id = remote_scrolling.dest_screen_id;
            pathway.dest_x = remote_scrolling.dest_x;
            pathway.dest_y = remote_scrolling.dest_y;

            if ((chunk.id == CHUNK_TRUNK && pathway.dest_chunk_id == CHUNK_TOWNS && remote_scrolling.screen_id == 0) ||
                (chunk.id == CHUNK_TOWNS && pathway.dest_chunk_id == CHUNK_TRUNK && remote_scrolling.dest_screen_id == 0))
            {
                // This is a bug in the data. The first screen of Trunk after exiting Eolis,
                // has scroll data to travel to the Apolune town.
                // I tried to fly up/left with the boots to see if it triggers.
                // The game doesn't let you go. Hardcoded rule?
                continue;
            }

            auto& screen = chunk.screens[remote_scrolling.screen_id];
            screen.pathways.push_back(pathway);
        }
    }
}


static void load_chunks(FILE* f, int bank)
{
    int bank_offset = bank_offset_to_rom_addr(bank, 0);

    // Read pointer table
    const auto pointers = read_pointer_table(f, 0, bank_offset);
    for (const auto pointer : pointers)
    {
        cart_chunk_t chunk;
        int chunk_id = (int)cart.chunks.size();
        chunk.id = chunk_id;
        cart.chunk_to_bank_table[chunk_id] = bank;

        const auto screen_pointers = read_pointer_table(f, pointer, bank_offset);
        for (const auto screen_pointer : screen_pointers)
        {
            int screen_id = (int)chunk.screens.size();
            if (chunk_id == CHUNK_SHOPS)
                screen_id = (int)cart.shop_screens.size();

            cart_screen_t screen = read_screen(f, screen_pointer + bank_offset);

            screen.id = screen_id;
            screen.tileset = get_tileset(chunk_id, screen_id);

            if (chunk_id == CHUNK_SHOPS)
                cart.shop_screens.push_back(screen);
            else
                chunk.screens.push_back(screen);
        }

        load_chunk_metadata(f, chunk, chunk_id);

        cart.chunks.push_back(chunk);
    }
}


static void load_entities(FILE* f)
{
    uint16_t pointers[8];
    fseek(f, ENTITY_POINTERS_ADDR, SEEK_SET);
    fread(pointers, 2, 8, f);

    for (auto& chunk : cart.chunks)
    {
        int pointers_bank_offset = bank_offset_to_rom_addr(BANK_ENTITIES, (int)pointers[CHUNK_TO_ALL_LEVELS_MAP[chunk.id]] - PRG_START_ADDR);
        std::vector<uint16_t> entity_pointers;
        entity_pointers.resize(chunk.screens.size());
        fseek(f, pointers_bank_offset, SEEK_SET);
        fread(entity_pointers.data(), 2, entity_pointers.size(), f);

        for (int screen_id = 0; screen_id < (int)chunk.screens.size(); ++screen_id)
        {
            int screen_bank_offset = bank_offset_to_rom_addr(BANK_ENTITIES, (int)entity_pointers[screen_id] - PRG_START_ADDR);
            fseek(f, screen_bank_offset, SEEK_SET);

            struct raw_entity_t
            {
                uint8_t entity_id = 0xFF;
                struct entity_pos_t
                {
                    uint8_t x : 4;
                    uint8_t y : 4;
                } pos = {0xFF, 0xFF};
                int addr;
            };

            std::vector<raw_entity_t> raw_entities;
            std::vector<uint8_t> raw_entity_dialogs;

            while (true)
            {
                raw_entity_t raw_entity;
                raw_entity.addr = (int)ftell(f) - 0x10;
                fread(&raw_entity.entity_id, 1, 1, f);
                if (raw_entity.entity_id == 0xFF) break;
                fread(&raw_entity.pos, 1, 1, f);
                raw_entities.push_back(raw_entity);
            }
            while (true)
            {
                uint8_t dialog_id;
                fread(&dialog_id, 1, 1, f);
                if (dialog_id == 0xFF) break;
                raw_entity_dialogs.push_back(dialog_id);
            }

            for (int i = 0; i < (int)raw_entities.size(); ++i)
            {
                cart_entity_t entity;
                entity.type = raw_entities[i].entity_id;
                entity.x = raw_entities[i].pos.x;
                entity.y = raw_entities[i].pos.y;
                entity.addr = raw_entities[i].addr;
                if (i < (int)raw_entity_dialogs.size())
                    entity.dialog_id = raw_entity_dialogs[i];
                chunk.screens[screen_id].entities.push_back(entity);
            }
        }
    }
}


static int create_room(int chunk_id, int screen_id)
{
    auto& chunk = cart.chunks[chunk_id];
    auto start_screen = &chunk.screens[screen_id];

    // Make sure this room is not already generated
    if (cart.chunks[chunk_id].screens[screen_id].room_id != -1) return -1;

    cart_room_t room;
    room.id = (int)cart.rooms.size();
    room.chunk_id = chunk_id;

    if (chunk_id == CHUNK_EVIL_FORTRESS && screen_id == 8)
    {
        // Evil Fortress' connections don't make sense. They will work in-game, but for editor visual, let's arrange those rooms manually
#define ADD_SCREEN(screen_id, x, y) \
        chunk.screens[screen_id].room_id = room.id; \
        chunk.screens[screen_id].x_offset_in_room = x; \
        chunk.screens[screen_id].y_offset_in_room = y; \
        room.screens.push_back(screen_id);

        ADD_SCREEN(1, 1, 1);  ADD_SCREEN(2, 2, 1);  ADD_SCREEN(3, 3, 1);  ADD_SCREEN(4, 4, 1);  ADD_SCREEN(5, 5, 1);
        ADD_SCREEN(6, 1, 2);  ADD_SCREEN(7, 2, 2);                        ADD_SCREEN(8, 4, 2);  ADD_SCREEN(9, 5, 2);
        ADD_SCREEN(10, 1, 3); ADD_SCREEN(11, 2, 3); ADD_SCREEN(12, 3, 3); ADD_SCREEN(13, 4, 3); ADD_SCREEN(14, 5, 3);
        ADD_SCREEN(15, 1, 4); ADD_SCREEN(16, 2, 4); ADD_SCREEN(17, 3, 4); ADD_SCREEN(18, 4, 4); ADD_SCREEN(19, 5, 4);
    }
    else
    {
        std::set<cart_screen_t*> added_screens;
        std::queue<cart_screen_t*> to_check;

        // Flood fill
        to_check.push(start_screen);
        added_screens.insert(start_screen);
        while (!to_check.empty())
        {
            auto screen = to_check.front();
            to_check.pop();
            if (screen->id == -1) continue;

            screen->room_id = room.id;
            room.screens.push_back(screen->id);

            if (screen->scroll.left != 0xFF)
            {
                auto target_screen = &chunk.screens[screen->scroll.left];
                if (!added_screens.count(target_screen))
                {
                    target_screen->x_offset_in_room = screen->x_offset_in_room - 1;
                    target_screen->y_offset_in_room = screen->y_offset_in_room;
                    to_check.push(target_screen);
                    added_screens.insert(target_screen);
                }
            }
            if (screen->scroll.right != 0xFF)
            {
                auto target_screen = &chunk.screens[screen->scroll.right];
                if (!added_screens.count(target_screen))
                {
                    if (!(screen->scroll.right == 22 && chunk_id == CHUNK_DARTMOOR_CASTLE)) // The boss fight was originally intended to be 2 screens wide. That second screen is still there. Ignore it
                    {
                        target_screen->x_offset_in_room = screen->x_offset_in_room + 1;
                        target_screen->y_offset_in_room = screen->y_offset_in_room;
                        to_check.push(target_screen);
                        added_screens.insert(target_screen);
                    }
                }
            }
            if (screen->scroll.top != 0xFF)
            {
                auto target_screen = &chunk.screens[screen->scroll.top];
                if (!added_screens.count(target_screen))
                {
                    if (!(screen->scroll.top == 15 && chunk_id == CHUNK_BRANCHES) && // There's a connection in game that shouldn't be here. It's innacessible from the game, blocked off by collisions. Those are different rooms.
                        !(screen->scroll.top == 26 && chunk_id == CHUNK_MISTY)) // Useless connection. Misty is split in 3 section. This creates a large one. You need to pass by Victim.
                    {
                        target_screen->x_offset_in_room = screen->x_offset_in_room;
                        target_screen->y_offset_in_room = screen->y_offset_in_room - 1;
                        if (screen->scroll.top == 42 && chunk_id == CHUNK_MISTY) // Connection glitch in the editor, but shouldn't cause any issue in-game. In misty, there's a gap from a staircase
                            target_screen->y_offset_in_room--;
                        to_check.push(target_screen);
                        added_screens.insert(target_screen);
                    }
                }
            }
            if (screen->scroll.bottom != 0xFF)
            {
                auto target_screen = &chunk.screens[screen->scroll.bottom];
                if (!added_screens.count(target_screen))
                {
                    if (!(screen->scroll.bottom == 13 && chunk_id == CHUNK_BRANCHES) && // There's a connection in game that shouldn't be here. It's innacessible from the game, blocked off by collisions. Those are different rooms.
                        !(screen->scroll.bottom == 34 && chunk_id == CHUNK_MISTY)) // Useless connection. Misty is split in 3 section. This creates a large one. YOu need to pass by Victim
                    {
                        target_screen->x_offset_in_room = screen->x_offset_in_room;
                        target_screen->y_offset_in_room = screen->y_offset_in_room + 1;
                        if (screen->scroll.bottom == 4 && chunk_id == CHUNK_MISTY) // Connection glitch in the editor, but shouldn't cause any issue in-game. In misty, there's a gap from a staircase
                            target_screen->y_offset_in_room++;
                        to_check.push(target_screen);
                        added_screens.insert(target_screen);
                    }
                }
            }
        }
    }

    cart.rooms.push_back(room);

    // Check doors, continue flood fill in other rooms
    for (auto screen_id : room.screens)
    {
        auto screen = &cart.chunks[room.chunk_id].screens[screen_id];
        for (auto& door : screen->doors)
        {
            auto room_id = create_room(door.dest_chunk_id, door.dest_screen_id);
            if (room_id != -1)
            {
                auto target_room = &cart.rooms[room_id];
                door.dest_room_id = room_id;

                // Paint the next room with proper palette
                if (door.dest_palette)
                {
                    for (auto dest_screen_id : target_room->screens)
                    {
                        auto dest_screen = &cart.chunks[target_room->chunk_id].screens[dest_screen_id];
                        dest_screen->palette = door.dest_palette;
                    }
                }
            }
        }
    }

    // Check pathways
    for (auto screen_id : room.screens)
    {
        auto screen = &cart.chunks[room.chunk_id].screens[screen_id];
        for (auto& pathway : screen->pathways)
        {
            auto room_id = create_room(pathway.dest_chunk_id, pathway.dest_screen_id);
            if (room_id != -1)
            {
                auto target_room = &cart.rooms[room_id];
                pathway.dest_room_id = room_id;

                // Paint the next room with proper palette
                if (pathway.dest_palette)
                {
                    for (auto dest_screen_id : target_room->screens)
                    {
                        auto dest_screen = &cart.chunks[target_room->chunk_id].screens[dest_screen_id];
                        dest_screen->palette = pathway.dest_palette;
                    }
                }
            }
        }
    }

    // Return room id
    return room.id;
}


static void build_rooms()
{
    // We start at chunk 0, screen 0. And we crawl everything to create rooms
    create_room(0, 0);
}


static void create_shops()
{
    auto& chunk = cart.chunks[CHUNK_SHOPS];
    for (const auto& door : cart.shop_doors)
    {
        chunk.screens.resize(onut::max((int)chunk.screens.size(), door.dest_screen_id + 1));
        chunk.screens[door.dest_screen_id] = cart.shop_screens[door.dest_shop_id];
        chunk.screens[door.dest_screen_id].id = door.dest_screen_id;
    }
}


#define DIALOG_CODE_TERMINATE 0x00
#define DIALOG_CODE_SHOW_DIALOG_TYPE_1 0x01
#define DIALOG_CODE_SHOW_DIALOG_TYPE_2 0x02
#define DIALOG_CODE_SHOW_DIALOG_TYPE_3 0x03
#define DIALOG_CODE_JUMP_IF_CAN_LEVEL_UP 0x04
#define DIALOG_CODE_JUMP_IF_HAS_MONEY 0x05
#define DIALOG_CODE_SET_GURU 0x06
#define DIALOG_CODE_GIVE_ITEM 0x07
#define DIALOG_CODE_SHOW_BUY 0x08
#define DIALOG_CODE_GIVE_MONEY 0x09
#define DIALOG_CODE_GIVE_MAGIC 0x0A
#define DIALOG_CODE_JUMP_IF_FOUNTAIN 0x0B
#define DIALOG_CODE_JUMP_IF_HAS_TITLE 0x0C
#define DIALOG_CODE_JUMP_IF_HAS_ANY_MONEY 0x0D
#define DIALOG_CODE_FLOW_FOUNTAIN 0x0E
#define DIALOG_CODE_ASK_BUY_OR_SELL 0x0F
#define DIALOG_CODE_TAKE_ITEM 0x10
#define DIALOG_CODE_SHOW_SELL 0x11
#define DIALOG_CODE_JUMP_CONDITION 0x12
#define DIALOG_CODE_GIVE_HEALTH 0x13
#define DIALOG_CODE_MEDITATE 0x14
#define DIALOG_CODE_JUMP 0x17


namespace std
{
    std::string to_string(const cart_string_t& string)
    {
        std::string ret;
        for (const auto& text_block : string.text_blocks)
        {
            ret += text_block + " ";
        }
        return ret;
    }
}


static std::vector<cart_dialog_command_t> disassemble_dialog_script(int pc)
{
    int bank_offset = bank_offset_to_rom_addr(BANK_TEXT_SETS, 0);
    std::vector<cart_dialog_command_t> commands;
    
    cart_dialog_command_t cmd;
    cmd.addr = pc - 0x10;
    uint8_t code = cart.rom[pc++];
    while (code != DIALOG_CODE_TERMINATE)
    {
        switch (code)
        {
            case DIALOG_CODE_SHOW_DIALOG_TYPE_1:
            {
                cmd.command = cart_dialog_commands_t::SHOW_DIALOG_1;
                cmd.text_block = cart.rom[pc++];
                cmd.string = std::to_string(cart.strings[cmd.text_block - 1]);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_SHOW_DIALOG_TYPE_2:
            {
                cmd.command = cart_dialog_commands_t::SHOW_DIALOG_2;
                cmd.text_block = cart.rom[pc++];
                cmd.string = std::to_string(cart.strings[cmd.text_block - 1]);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_SHOW_DIALOG_TYPE_3:
            {
                cmd.command = cart_dialog_commands_t::SHOW_DIALOG_3;
                cmd.text_block = cart.rom[pc++];
                cmd.string = std::to_string(cart.strings[cmd.text_block - 1]);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_IF_HAS_ANY_MONEY:
            {
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.command = cart_dialog_commands_t::JUMP_IF_HAS_ANY_MONEY;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_GIVE_ITEM:
            {
                cmd.addr = pc - 0x10;
                cmd.item = (item_t)cart.rom[pc++];
                cmd.command = cart_dialog_commands_t::GIVE_ITEM;
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_TAKE_ITEM:
            {
                cmd.item = (item_t)cart.rom[pc++];
                cmd.command = cart_dialog_commands_t::TAKE_ITEM;
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_CONDITION:
            {
                cmd.addr = pc - 0x10;
                cmd.item = (item_t)cart.rom[pc++];
                int jump_addr = 0;
                jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.command = cart_dialog_commands_t::JUMP_IF_HAS_ITEM;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_GIVE_MONEY:
            {
                cmd.command = cart_dialog_commands_t::GIVE_MONEY;
                cmd.golds = (int)cart.rom[pc++];
                cmd.golds |= (int)cart.rom[pc++] << 8;
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_IF_HAS_MONEY:
            {
                cmd.command = cart_dialog_commands_t::JUMP_IF_HAS_MONEY;
                cmd.golds = (int)cart.rom[pc++];
                cmd.golds |= (int)cart.rom[pc++] << 8;
                cmd.branch_commands = disassemble_dialog_script(pc);
                commands.push_back(cmd);

                cmd.command = cart_dialog_commands_t::SHOW_DIALOG_2;
                cmd.text_block = 3;
                cmd.string = std::to_string(cart.strings[3 - 1]);
                commands.push_back(cmd);

                return commands;
            }
            case DIALOG_CODE_GIVE_HEALTH:
            {
                cmd.command = cart_dialog_commands_t::GIVE_HEALTH;
                cmd.health = (int)cart.rom[pc++];
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_GIVE_MAGIC:
            {
                cmd.command = cart_dialog_commands_t::GIVE_MAGIC;
                cmd.magic = (int)cart.rom[pc++];
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_ASK_BUY_OR_SELL:
            {
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.command = cart_dialog_commands_t::ASK_BUY_OR_SELL;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_SHOW_BUY:
            {
                cmd.command = cart_dialog_commands_t::SHOW_BUY;
                int items_addr = (int)cart.rom[pc++];
                items_addr |= (int)cart.rom[pc++] << 8;
                items_addr += bank_offset;
                while (cart.rom[items_addr] != 0xFF)
                {
                    cart_shop_item_t shop_item;
                    shop_item.addr = items_addr - 0x10;
                    shop_item.item = (item_t)cart.rom[items_addr++];
                    shop_item.price = (int)cart.rom[items_addr++];
                    shop_item.price |= (int)cart.rom[items_addr++] << 8;
                    cmd.shop_items.push_back(shop_item);
                }
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_SHOW_SELL:
            {
                cmd.command = cart_dialog_commands_t::SHOW_SELL;
                int items_addr = (int)cart.rom[pc++];
                items_addr |= (int)cart.rom[pc++] << 8;
                items_addr += bank_offset;
                while (cart.rom[items_addr] != 0xFF)
                {
                    cart_shop_item_t shop_item;
                    shop_item.item = (item_t)cart.rom[items_addr++];
                    shop_item.price = (int)cart.rom[items_addr++];
                    shop_item.price |= (int)cart.rom[items_addr++] << 8;
                    cmd.shop_items.push_back(shop_item);
                }
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_IF_HAS_TITLE:
            {
                cmd.command = cart_dialog_commands_t::JUMP_IF_HAS_TITLE;
                cmd.title = (int)cart.rom[pc++];
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_SET_GURU:
            {
                cmd.command = cart_dialog_commands_t::SET_GURU;
                cmd.guru = (int)cart.rom[pc++];
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_IF_CAN_LEVEL_UP:
            {
                cmd.command = cart_dialog_commands_t::JUMP_IF_CAN_LEVEL_UP;
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP:
            {
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                auto branch_commands = disassemble_dialog_script(jump_addr);
                commands.insert(commands.end(), branch_commands.begin(), branch_commands.end());

                return commands;
            }
            case DIALOG_CODE_MEDITATE:
            {
                cmd.command = cart_dialog_commands_t::MEDITATE;
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_JUMP_IF_FOUNTAIN:
            {
                cmd.command = cart_dialog_commands_t::JUMP_IF_FOUNTAIN;
                cmd.fountain = (int)cart.rom[pc++];
                int jump_addr = (int)cart.rom[pc++];
                jump_addr |= (int)cart.rom[pc++] << 8;
                jump_addr += bank_offset;
                cmd.branch_commands = disassemble_dialog_script(jump_addr);
                commands.push_back(cmd);
                break;
            }
            case DIALOG_CODE_FLOW_FOUNTAIN:
            {
                cmd.command = cart_dialog_commands_t::FLOW_FOUNTAIN;
                cmd.fountain = (int)cart.rom[pc++];
                commands.push_back(cmd);
                break;
            }
            case 0x15:
            {
                // Unsure, not used by any Entity in-game.
                // The dialog associated with this sounds like if you were
                // to return to the kind after beating the Evil One.
                // 0x15 might trigger end cutscene.
                // Currently automatically triggered by killing Evil One.
                break;
            }
            default:
            {
                __debugbreak();
                break;
            }
        }

        cmd.addr = pc - 0x10;
        code = cart.rom[pc++];
    }

    return commands;
}


static void load_text_sets(FILE* f)
{
    int lo_start = TEXT_SETS_PTR_LO_ADDR;
    int hi_start = TEXT_SETS_PTR_HI_ADDR;

    int pointer_count = hi_start - lo_start;

    std::vector<uint8_t> lo_pointers;
    lo_pointers.resize(pointer_count);
    fseek(f, lo_start, SEEK_SET);
    fread(lo_pointers.data(), 1, lo_pointers.size(), f);

    std::vector<uint8_t> hi_pointers;
    hi_pointers.resize(pointer_count);
    fseek(f, hi_start, SEEK_SET);
    fread(hi_pointers.data(), 1, hi_pointers.size(), f);

    std::vector<int> pointers;
    for (int i = 0; i < pointer_count; ++i)
    {
        int pointer = (hi_pointers[i] << 8) | lo_pointers[i];
        pointer += bank_offset_to_rom_addr(BANK_TEXT_SETS, 0);
        pointers.push_back(pointer);
    }

    // Now this is tricky, because we don't know how big is the script. We stop executing it when we hit 0x00.
    // And pointers are not in order. So let's order the pointers first just so we can get the size of each pointers.
    std::vector<int> pointers_copy = pointers;
    std::map<int, int> pointer_to_size;
    std::sort(pointers_copy.begin(), pointers_copy.end());
    for (int i = 0; i < (int)pointers_copy.size() - 1; ++i)
    {
        auto pointer = pointers_copy[i];
        auto next_pointer = pointers_copy[i + 1];
        pointer_to_size[pointer] = next_pointer - pointer;
    }
    // Then last one
    pointer_to_size[pointers_copy.back()] = 0x326D7 - pointers_copy[pointers_copy.size() - 2];

    for (int i = 0; i < (int)pointers.size(); ++i)
    {
        auto pointer = pointers[i];
        fseek(f, pointer, SEEK_SET);

        cart_dialog_t dialog;
        std::vector<uint8_t> raw;
        raw.resize(pointer_to_size[pointer]);
        fread(raw.data(), 1, (int)raw.size(), f);
        dialog.addr = pointer;

        for (int j = 0; j < (int)raw.size(); ++j)
        {
            cart_dialog_raw_code_t code;
            code.addr = pointer + j - BANK_TEXT_SETS * BANK_SIZE - NES_HEADER_SIZE; // Store addresses as local to the bank
            code.code = raw[j];
            dialog.raw_codes.push_back(code);
        }

        cart.dialogs.push_back(dialog);
    }

    // Now we "decompile" the scripts into something we can use and that is readable.
    for (int i = 0; i < (int)cart.dialogs.size(); ++i)
    {
        cart.dialogs[i].portrait = (int)cart.rom[cart.dialogs[i].addr];
        auto commands = disassemble_dialog_script(cart.dialogs[i].addr + 1);
        cart.dialogs[i].commands = commands;
    }
}


static void load_musics(FILE* f)
{
    int offset = bank_offset_to_rom_addr(BANK_MUSIC, 0);

    for (int i = 0; i < 16; ++i)
    {
        cart_music_t music;

        music.id = i;
        music.name = MUSIC_NAMES[i];

        cart.musics.push_back(music);
    }
}
