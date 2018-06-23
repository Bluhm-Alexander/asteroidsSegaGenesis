#include "types.h"

__attribute__((externally_visible))
const struct
{
    char console[16];               /* Console Name (16)                             1 */
    char copyright[16];             /* Copyright Information (16)                    2 */
    char title_local[48];           /* Domestic Name (48)                            3 */
    char title_int[48];             /* Overseas Name (48)                            4 */
    char serial[14];                /* Serial Number (2, 12)                         5 */
    u16 checksum;                   /* Checksum (2)                                  6 */
    char IOSupport[16];             /* I/O Support (16)                              7 */
    u32 rom_start;                  /* ROM Start Address (4)                         8 */
    u32 rom_end;                    /* ROM End Address (4)                           9 */
    u32 ram_start;                  /* Start of Backup RAM (4)                      10 */
    u32 ram_end;                    /* End of Backup RAM (4)                        11 */
    char sram_sig[2];               /* "RA" for save ram (2)                        12 */
    u16 sram_type;                  /* 0xF820 for save ram on odd bytes (2)         13 */
    u32 sram_start;                 /* SRAM start address - normally 0x200001 (4)   14 */
    u32 sram_end;                   /* SRAM end address - start + 2*sram_size (4)   15 */
    char modem_support[12];         /* Modem Support (24)                           16 */
    char notes[40];                 /* Memo (40)                                    17 */
    char region[16];                /* Country Support (16)                         18 */
} rom_header = {
    "SEGA MEGA DRIVE ",                                 //1
    "(C)Okoboji",                                       //2
    "Asteroids                                       ", //3
    "Asteroids                                       ", //4
    "GM 00000000-00",                                   //5
    0x0000,                                             //6
    "JD              ",                                 //7
    0x00000000,                                         //8
    0x00100000,                                         //9
    0x00FF0000,                                         //10
    0x00FFFFFF,                                         //11
    "  ",                                               //12
    0x0000,                                             //13
    0x00200000,                                         //14
    0x002001FF,                                         //15
    "            ",                                     //16
    "This Program is still in development    ",         //17
    "JUE             "                                  //18
};
