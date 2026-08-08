#include "msxgl.h" 
#include "vdp.h" 
#include "input.h" 
#include "system.h"

#define MAP_W 24 
#define MAP_H 16 
#define MAP_OFFSET_X 3 
#define MAP_OFFSET_Y 1 

// 🛑【重大修正】改為 128，絕對不能用 64！否則會覆蓋掉 65(A) ~ 90(Z) 的英文字型導致崩潰
#define PREF_START_CHR 128 
#define TOTAL_PREFECTURES 47 
#define SEA -1 

const char* const g_PrefNames[TOTAL_PREFECTURES] = { 
    "HOKKAIDO ", "AOMORI   ", "IWATE    ", "MIYAGI   ", "AKITA    ", // 0-4 
    "YAMAGATA ", "FUKUSHIMA", "IBARAKI  ", "TOCHIGI  ", "GUNMA    ", // 5-9 
    "SAITAMA  ", "CHIBA    ", "TOKYO    ", "KANAGAWA ", "NIIGATA  ", // 10-14 
    "TOYAMA   ", "ISHIKAWA ", "FUKUI    ", "YAMANASHI", "NAGANO   ", // 15-19 
    "GIFU     ", "SHIZUOKA ", "AICHI    ", "MIE      ", "SHIGA    ", // 20-24 
    "KYOTO    ", "OSAKA    ", "HYOGO    ", "NARA     ", "WAKAYAMA ", // 25-29 
    "TOTTORI  ", "SHIMANE  ", "OKAYAMA  ", "HIROSHIMA", "YAMAGUCHI", // 30-34 
    "TOKUSHIMA", "KAGAWA   ", "EHIME    ", "KOCHI    ", "FUKUOKA  ", // 35-39 
    "SAGA     ", "NAGASAKI ", "KUMAMOTO ", "OITA     ", "MIYAZAKI ", // 40-44 
    "KAGOSHIMA", "OKINAWA  "                                         // 45-46 
}; 

u8 g_MyPrefLevels[TOTAL_PREFECTURES] = {0}; 
u8 g_SelectedPrefIdx = 0; 
u8 g_PreviewLevel = 0; 

const u8 g_LevelColors[6] = { 
    COLOR_WHITE, COLOR_LIGHT_BLUE, COLOR_LIGHT_GREEN, 
    COLOR_LIGHT_YELLOW, COLOR_MAGENTA, COLOR_MEDIUM_RED    
}; 

const i8 g_JapanRealMap[MAP_H][MAP_W] = { 
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  0,  0,-1,-1,-1,-1,-1 }, // [0] 北海道
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  0,  0,-1,-1,-1,-1,-1 }, // [1] 北海道
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, // [2] (增加的留白) 津輕海峽
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  1,-1,-1,-1,-1,-1,-1 }, // [3] 青森
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  4,  2,-1,-1,-1,-1,-1 }, // [4] 秋田, 岩手
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  5,  3,-1,-1,-1,-1,-1 }, // [5] 山形, 宮城
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 14,  6,-1,-1,-1,-1,-1 }, // [6] 新潟, 福島
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 16, 15,  9,  8,  7,-1,-1,-1 }, // [7] 石川, 富山, 群馬, 栃木, 茨城
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 17, 20, 19, 10, 11,-1,-1,-1,-1 }, // [8] 福井, 岐阜, 長野, 埼玉, 千葉
    { -1,-1,-1,-1,-1,-1, 41, 40, 39,-1, 34, 31, 30, 27, 25, 24, 22, 18, 12, 11,-1,-1,-1,-1 }, // [9] 九州頂部, 中國, 關西, 中部, 東京, 千葉
    { -1,-1,-1,-1,-1,-1, 41, 42, 43,-1, 34, 33, 32, 27, 26, 28, 23, 21, 13,-1,-1,-1,-1,-1 }, // [10] 九州中部, 中國, 關西, 中部, 神奈川
    { -1,-1,-1,-1,-1,-1,-1, 45, 44,-1,-1,-1,-1,-1, 29,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, // [11] 鹿兒島, 宮崎, (海), 和歌山
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 37, 36,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, // [12] 愛媛, 香川
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 38, 35,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, // [13] 高知, 德島
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, // [14] (留白)
    { -1,-1,-1,-1, 46,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }  // [15] 沖繩
};

// 內建迷你字型庫：包含 ASCII 32(' ') 到 90('Z') 的所有字元 (扁平 1D 陣列防止 SDCC 報錯)
const u8 g_MiniFont[59 * 8] = {
    0,0,0,0,0,0,0,0,            // 32: Space
    0x10,0x10,0x10,0x10,0,0x10,0,0, // 33: !
    0x28,0x28,0,0,0,0,0,0,      // 34: "
    0x28,0x7c,0x28,0x7c,0x28,0,0,0, // 35: #
    0x10,0x3c,0x10,0x3c,0x10,0,0,0, // 36: $
    0x60,0x64,0x08,0x10,0x26,0x06,0,0, // 37: %
    0x38,0x44,0x38,0x44,0x78,0,0,0, // 38: &
    0x10,0x10,0x20,0,0,0,0,0,   // 39: '
    0x08,0x10,0x20,0x20,0x20,0x10,0x08,0, // 40: (
    0x20,0x10,0x08,0x08,0x08,0x10,0x20,0, // 41: )
    0,0x14,0x08,0x3e,0x08,0x14,0,0, // 42: *
    0,0,0x10,0x38,0x10,0,0,0,   // 43: +
    0,0,0,0,0,0x10,0x20,0,      // 44: ,
    0,0,0,0,0x3e,0,0,0,         // 45: -
    0,0,0,0,0,0x10,0,0,         // 46: .
    0,0x04,0x08,0x10,0x20,0x40,0,0, // 47: /
    0x3c,0x42,0x4a,0x52,0x62,0x42,0x3c,0, // 48: 0
    0x18,0x28,0x08,0x08,0x08,0x08,0x3e,0, // 49: 1
    0x3c,0x42,0x02,0x1c,0x20,0x40,0x7e,0, // 50: 2
    0x3c,0x42,0x02,0x1c,0x02,0x42,0x3c,0, // 51: 3
    0x0c,0x14,0x24,0x44,0x7e,0x04,0x04,0, // 52: 4
    0x7e,0x40,0x7c,0x02,0x02,0x42,0x3c,0, // 53: 5
    0x3c,0x40,0x40,0x7c,0x42,0x42,0x3c,0, // 54: 6
    0x7e,0x02,0x04,0x08,0x10,0x10,0x10,0, // 55: 7
    0x3c,0x42,0x42,0x3c,0x42,0x42,0x3c,0, // 56: 8
    0x3c,0x42,0x42,0x3e,0x02,0x02,0x3c,0, // 57: 9
    0,0,0x18,0x18,0,0x18,0x18,0, // 58: :
    0,0,0x18,0x18,0,0x18,0x20,0, // 59: ;
    0,0x08,0x10,0x20,0x10,0x08,0,0, // 60: <
    0,0,0,0x3e,0,0x3e,0,0,      // 61: =
    0,0x20,0x10,0x08,0x10,0x20,0,0, // 62: >
    0x3c,0x42,0x02,0x1c,0x10,0,0x10,0, // 63: ?
    0x3c,0x42,0x5a,0x5a,0x5e,0x40,0x3c,0, // 64: @
    0x18,0x24,0x42,0x42,0x7e,0x42,0x42,0, // 65: A
    0x7c,0x42,0x42,0x7c,0x42,0x42,0x7c,0, // 66: B
    0x3c,0x42,0x40,0x40,0x40,0x42,0x3c,0, // 67: C
    0x78,0x44,0x42,0x42,0x42,0x44,0x78,0, // 68: D
    0x7e,0x40,0x40,0x7c,0x40,0x40,0x7e,0, // 69: E
    0x7e,0x40,0x40,0x7c,0x40,0x40,0x40,0, // 70: F
    0x3c,0x42,0x40,0x4e,0x42,0x42,0x3e,0, // 71: G
    0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0, // 72: H
    0x3e,0x08,0x08,0x08,0x08,0x08,0x3e,0, // 73: I
    0x0e,0x04,0x04,0x04,0x04,0x44,0x38,0, // 74: J
    0x44,0x48,0x50,0x60,0x50,0x48,0x44,0, // 75: K
    0x40,0x40,0x40,0x40,0x40,0x40,0x7e,0, // 76: L
    0x42,0x66,0x5a,0x42,0x42,0x42,0x42,0, // 77: M
    0x42,0x62,0x52,0x4a,0x46,0x42,0x42,0, // 78: N
    0x3c,0x42,0x42,0x42,0x42,0x42,0x3c,0, // 79: O
    0x7c,0x42,0x42,0x7c,0x40,0x40,0x40,0, // 80: P
    0x3c,0x42,0x42,0x42,0x4a,0x44,0x3a,0, // 81: Q
    0x7c,0x42,0x42,0x7c,0x50,0x48,0x44,0, // 82: R
    0x3c,0x42,0x40,0x3c,0x02,0x42,0x3c,0, // 83: S
    0x7e,0x10,0x10,0x10,0x10,0x10,0x10,0, // 84: T
    0x42,0x42,0x42,0x42,0x42,0x42,0x3c,0, // 85: U
    0x42,0x42,0x42,0x42,0x42,0x24,0x18,0, // 86: V
    0x42,0x42,0x42,0x42,0x5a,0x66,0x42,0, // 87: W
    0x42,0x42,0x24,0x18,0x24,0x42,0x42,0, // 88: X
    0x42,0x42,0x24,0x18,0x18,0x18,0x18,0, // 89: Y
    0x7e,0x04,0x08,0x10,0x20,0x40,0x7e,0  // 90: Z
};

const u8 g_BlockPattern[8] = { 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00 }; 

// 讓文字在 SCREEN 2 全螢幕都能正確顯示 (解決底部文字隱形問題)
void SetupScreen2Text() 
{ 
    u8 i;
    u8 colorByte = (COLOR_BLACK << 4) | COLOR_TRANSPARENT;
    
    // 1. 設定文字顏色：ASCII 32~90 (全螢幕 3 個區塊皆設為 0xF0 白字黑底)
    // 59 個字元 * 8 bytes = 472 bytes
    VDP_FillVRAM_16K(colorByte, 0x2000 + (32 * 8), 472); 
    VDP_FillVRAM_16K(colorByte, 0x2800 + (32 * 8), 472); 
    VDP_FillVRAM_16K(colorByte, 0x3000 + (32 * 8), 472); 

    // 2. 將我們寫死的字型陣列，硬塞進顯示晶片 (VDP) 的圖案表裡
    for(i = 0; i < 59; i++) 
    { 
        u16 offset = (32 + i) * 8; // 計算出這個字元該放的記憶體位址
        
        // 寫入頂部 (Row 0~7)
        VDP_WriteVRAM_16K(&g_MiniFont[i * 8], 0x0000 + offset, 8); 
        // 寫入中部 (Row 8~15)
        VDP_WriteVRAM_16K(&g_MiniFont[i * 8], 0x0800 + offset, 8); 
        // 寫入底部 (Row 16~23，你的 UI 就在這裡！)
        VDP_WriteVRAM_16K(&g_MiniFont[i * 8], 0x1000 + offset, 8); 
    }
}

void InitPrefectureGraphics() 
{ 
    u8 i; 
    for(i = 0; i < TOTAL_PREFECTURES; i++) 
    { 
        u16 offset = (PREF_START_CHR + i) * 8;
        // 將方塊寫入 Screen 2 的三個區段 (解決之前地圖只有一半的問題)
        VDP_WriteVRAM_16K(g_BlockPattern, 0x0000 + offset, 8); 
        VDP_WriteVRAM_16K(g_BlockPattern, 0x0800 + offset, 8); 
        VDP_WriteVRAM_16K(g_BlockPattern, 0x1000 + offset, 8); 
    } 
} 

void UpdateMapColors() 
{ 
    u8 i;
    for(i = 0; i < TOTAL_PREFECTURES; i++) 
    { 
        u8 level = g_MyPrefLevels[i]; 
        u8 colorCode = g_LevelColors[level]; 
        u8 colorByte = (colorCode << 4) | COLOR_BLACK; 
        u16 offset;
        
        if (i == g_SelectedPrefIdx) 
        { 
            colorByte = (colorCode << 4) | COLOR_DARK_BLUE; 
        } 
        
        offset = (PREF_START_CHR + i) * 8;
        // 色彩表也是切成三等份
        VDP_FillVRAM_16K(colorByte, 0x2000 + offset, 8); 
        VDP_FillVRAM_16K(colorByte, 0x2800 + offset, 8); 
        VDP_FillVRAM_16K(colorByte, 0x3000 + offset, 8); 
    } 
} 

void DrawMapLayout() 
{ 
    u8 y, x;
    for(y = 0; y < MAP_H; y++) 
    { 
        for(x = 0; x < MAP_W; x++) 
        { 
            i8 prefID = g_JapanRealMap[y][x]; 
            u16 targetAddr = VDP_GetLayoutTable() + ((y + MAP_OFFSET_Y) * 32) + (x + MAP_OFFSET_X); 
            
            if(prefID == SEA) { 
                VDP_Poke_16K(' ', targetAddr); 
            } else { 
                VDP_Poke_16K(PREF_START_CHR + prefID, targetAddr); 
            } 
        } 
    } 
} 

u16 CalculateTotalScore() 
{ 
    u16 total = 0; 
    u8 i;
    for(i = 0; i < TOTAL_PREFECTURES; i++) 
    { 
        total += g_MyPrefLevels[i]; 
    } 
    return total; 
}

// 🛡️ 自製 100% 絕對安全的字串繪製函式 (繞過 BIOS 陷阱)
void DrawTextSafe(u8 x, u8 y, const char* text) {
    // 計算螢幕上的絕對記憶體位址 (Y 座標 * 每行 32 格 + X 座標)
    u16 addr = VDP_GetLayoutTable() + (y * 32) + x;
    while (*text) {
        VDP_Poke_16K(*text, addr); // 直接把字母寫進顯示晶片
        addr++;
        text++;
    }
}

// 🛡️ 自製安全的 3 位數分數繪製函式 (免用迴圈，效能極高)
void DrawScoreSafe(u8 x, u8 y, u16 value) {
    u16 addr = VDP_GetLayoutTable() + (y * 32) + x;
    u8 d100 = value / 100;
    u8 d10 = (value / 10) % 10;
    u8 d1 = value % 10;
    
    // 百位數 (如果是 0 則補空白，避免前一次的分數殘留)
    if (d100 > 0) VDP_Poke_16K('0' + d100, addr++);
    else VDP_Poke_16K(' ', addr++);
    
    // 十位數
    if (d100 > 0 || d10 > 0) VDP_Poke_16K('0' + d10, addr++);
    else VDP_Poke_16K(' ', addr++);
    
    // 個位數
    VDP_Poke_16K('0' + d1, addr++);
}

void DrawUI() 
{
    u16 score;
    
    // 全部改用我們自製的安全函式！
    DrawTextSafe(2, 18, "----------------------------"); 
     
    DrawTextSafe(2, 19, "TARGET: "); 
    DrawTextSafe(10, 19, g_PrefNames[g_SelectedPrefIdx]); 
     
    DrawTextSafe(2, 21, "LEVEL: "); 
    
    // 等級只有 0~5，直接塞進 VRAM ( ASCII '0' 加上數字 )
    VDP_Poke_16K('0' + g_PreviewLevel, VDP_GetLayoutTable() + (21 * 32) + 9);
     
    if (g_PreviewLevel != g_MyPrefLevels[g_SelectedPrefIdx]) { 
        DrawTextSafe(11, 21, "* "); 
    } else { 
        DrawTextSafe(11, 21, "  "); 
    } 
     
    switch(g_PreviewLevel) { 
        case 5: DrawTextSafe(13, 21, "(SUNDA)   "); break; 
        case 4: DrawTextSafe(13, 21, "(TOMATTA) "); break; 
        case 3: DrawTextSafe(13, 21, "(ARUKITA) "); break; 
        case 2: DrawTextSafe(13, 21, "(NORITATU)"); break; 
        case 1: DrawTextSafe(13, 21, "(TUKA)    "); break; 
        default: DrawTextSafe(13, 21, "(MITOU)   "); break; 
    } 

    score = CalculateTotalScore(); 
    DrawTextSafe(20, 19, "TOTAL SCORE"); 
     
    DrawScoreSafe(23, 21, score); 
    DrawTextSafe(26, 21, "/235");  
     
    if (g_PreviewLevel != g_MyPrefLevels[g_SelectedPrefIdx]) { 
        DrawTextSafe(2, 23, "PRESS SPACE TO CONFIRM... "); 
    } else { 
        DrawTextSafe(2, 23, "USE ARROWS TO NAVIGATE    "); 
    }
} 

void main() 
{ 
    u8 bNeedsUpdate = 1; 
    u8 delay_cnt = 0; 

    VDP_SetMode(VDP_MODE_SCREEN2);
    VDP_EnableVBlank(1); 
    VDP_ClearVRAM();

    SetupScreen2Text();

    InitPrefectureGraphics(); 
    DrawMapLayout(); 
    
    g_PreviewLevel = g_MyPrefLevels[g_SelectedPrefIdx]; 
    
    while(1) 
    { 
        // 這裡會等待 VBlank 中斷。現在中斷有開啟了，系統會絲滑順暢運行！
        Halt(); 
        
        if (bNeedsUpdate) 
        {
            DisableInterrupt();
            UpdateMapColors(); 
            DrawUI(); 
            bNeedsUpdate = 0; 
            EnableInterrupt();
        }
        
        if(Keyboard_IsKeyPressed(KEY_LEFT)) 
        { 
            if(g_SelectedPrefIdx > 0) g_SelectedPrefIdx--; 
            else g_SelectedPrefIdx = TOTAL_PREFECTURES - 1; 
            g_PreviewLevel = g_MyPrefLevels[g_SelectedPrefIdx]; 
            
            bNeedsUpdate = 1; 
            for(delay_cnt = 0; delay_cnt < 10; delay_cnt++) Halt(); 
        } 
        else if(Keyboard_IsKeyPressed(KEY_RIGHT)) 
        { 
            if(g_SelectedPrefIdx < TOTAL_PREFECTURES - 1) g_SelectedPrefIdx++; 
            else g_SelectedPrefIdx = 0; 
            g_PreviewLevel = g_MyPrefLevels[g_SelectedPrefIdx]; 
            
            bNeedsUpdate = 1; 
            for(delay_cnt = 0; delay_cnt < 10; delay_cnt++) Halt(); 
        } 
        else if(Keyboard_IsKeyPressed(KEY_UP)) 
        { 
            if(g_PreviewLevel < 5) g_PreviewLevel++; 
            
            bNeedsUpdate = 1; 
            for(delay_cnt = 0; delay_cnt < 10; delay_cnt++) Halt(); 
        } 
        else if(Keyboard_IsKeyPressed(KEY_DOWN)) 
        { 
            if(g_PreviewLevel > 0) g_PreviewLevel--; 
            
            bNeedsUpdate = 1; 
            for(delay_cnt = 0; delay_cnt < 10; delay_cnt++) Halt(); 
        } 
        else if(Keyboard_IsKeyPressed(KEY_SPACE)) 
        { 
            if (g_MyPrefLevels[g_SelectedPrefIdx] != g_PreviewLevel) { 
                g_MyPrefLevels[g_SelectedPrefIdx] = g_PreviewLevel; 
                bNeedsUpdate = 1; 
            } 
            for(delay_cnt = 0; delay_cnt < 15; delay_cnt++) Halt();  
        } 
    } 
}