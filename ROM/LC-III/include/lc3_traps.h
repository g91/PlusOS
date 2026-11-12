/* lc3_traps.h - Macintosh LC III (Universal ROM $067C, 1024 KB) Trap Definitions
 *
 * Generated from verified symbol map C:\Users\Evil\PlusOS\ROM\LC-III\maps\lc3_rom.map.
 * ROM Base in 32-bit mode: 0x40800000.
 */
#ifndef LC3_TRAPS_H
#define LC3_TRAPS_H

#include <stdint.h>

/* ROM Identification */
#define LC3_ROM_VERSION          0x067C
#define LC3_ROM_SIZE             0x00100000  /* 1024 KB */
#define LC3_ROM_BASE_32BIT       0x40800000
#define LC3_ROM_BASE_24BIT       0x00800000

/* ---- OS Traps ($A000 - $A0FF / $A200 - $A2FF) ---- */
#define LC3_TRAP_OPEN            0xA000  /* Handler @ 0x4080B766 */
#define LC3_TRAP_CLOSE           0xA001  /* Handler @ 0x4080BAAA */
#define LC3_TRAP_READ            0xA002  /* Handler @ 0x4080BB1C */
#define LC3_TRAP_WRITE           0xA003  /* Handler @ 0x4080BB9A */
#define LC3_TRAP_CONTROL         0xA004  /* Handler @ 0x4080BBAE */
#define LC3_TRAP_STATUS          0xA005  /* Handler @ 0x4080BBBE */
#define LC3_TRAP_KILLIO          0xA006  /* Handler @ 0x4080BBE4 */
#define LC3_TRAP_GETVOLINFO      0xA007
#define LC3_TRAP_CREATE          0xA008
#define LC3_TRAP_DELETE          0xA009
#define LC3_TRAP_OPENRF          0xA00A
#define LC3_TRAP_RENAME          0xA00B
#define LC3_TRAP_GETFINFO        0xA00C
#define LC3_TRAP_SETFINFO        0xA00D
#define LC3_TRAP_UNMOUNTVOL      0xA00E
#define LC3_TRAP_MOUNTVOL        0xA00F
#define LC3_TRAP_ALLOCATE        0xA010
#define LC3_TRAP_GETEOF          0xA011
#define LC3_TRAP_SETEOF          0xA012
#define LC3_TRAP_FLUSHVOL        0xA013
#define LC3_TRAP_GETVOL          0xA014
#define LC3_TRAP_SETVOL          0xA015
#define LC3_TRAP_FINITQUEUE      0xA016
#define LC3_TRAP_EJECT           0xA017
#define LC3_TRAP_GETFPOS         0xA018
#define LC3_TRAP_INITZONE        0xA019  /* Handler @ 0x4080CF14 */
#define LC3_TRAP_GETZONE         0xA01A
#define LC3_TRAP_SETZONE         0xA01B
#define LC3_TRAP_FREEMEM         0xA01C
#define LC3_TRAP_MAXMEM          0xA01D
#define LC3_TRAP_NEWPTR          0xA01E
#define LC3_TRAP_DISPOSEPTR      0xA01F
#define LC3_TRAP_SETPTRSIZE      0xA020
#define LC3_TRAP_GETPTRSIZE      0xA021
#define LC3_TRAP_NEWHANDLE       0xA022
#define LC3_TRAP_DISPOSEHANDLE   0xA023
#define LC3_TRAP_SETHANDLESIZE   0xA024
#define LC3_TRAP_GETHANDLESIZE   0xA025
#define LC3_TRAP_HANDLEZONE      0xA026
#define LC3_TRAP_REALLOCHANDLE   0xA027
#define LC3_TRAP_RECOVERHANDLE   0xA028
#define LC3_TRAP_HLOCK           0xA029
#define LC3_TRAP_HUNLOCK         0xA02A
#define LC3_TRAP_HPURGE          0xA02B
#define LC3_TRAP_HNO_PURGE       0xA02C
#define LC3_TRAP_SETGZPROC       0xA02D
#define LC3_TRAP_INITAPPLZONE    0xA02E  /* Handler @ 0x4080CDE4 */
#define LC3_TRAP_SETAPPLLIMIT    0xA02F  /* Handler @ 0x4080D260 */
#define LC3_TRAP_BLOCKMOVE       0xA030  /* Handler @ 0x4080C9DC (68030 unrolled) */
#define LC3_TRAP_POSTEVENT       0xA031  /* Handler @ 0x4080B3AE */
#define LC3_TRAP_OSEVENTAVAIL    0xA032  /* Handler @ 0x4080B418 */
#define LC3_TRAP_GETOSEVENT      0xA033
#define LC3_TRAP_FLUSHEVENTS     0xA034  /* Handler @ 0x4080B4CC */
#define LC3_TRAP_VINSTALL        0xA035  /* Handler @ 0x4080A23C */
#define LC3_TRAP_VREMOVE         0xA036
#define LC3_TRAP_OFFLINE         0xA037
#define LC3_TRAP_CRITERR         0xA038
#define LC3_TRAP_SETFILTYP       0xA039
#define LC3_TRAP_SETOSTYP        0xA03A
#define LC3_TRAP_DELAY           0xA03B  /* Handler @ 0x4080C08E */

/* 32-Bit Clean & MMU Extension OS Traps in LC III */
#define LC3_TRAP_STRIPADDRESS    0xA055  /* Handler @ 0x4080D764 */
#define LC3_TRAP_SWAPMMUMODE     0xA05D  /* 24-bit <-> 32-bit toggle */
#define LC3_TRAP_FSDISPATCH      0xA060  /* Handler @ 0x4080EF40 */
#define LC3_TRAP_MAXBLOCK        0xA061
#define LC3_TRAP_PURGESPACE      0xA062  /* Handler @ 0x4080D68E */
#define LC3_TRAP_MAXBLOCKSYS     0xA063
#define LC3_TRAP_STACKSPACE      0xA065
#define LC3_TRAP_NEWMEMPTYPTR    0xA066
#define LC3_TRAP_HMOVEHI         0xA067  /* Handler @ 0x4080D786 */
#define LC3_TRAP_HGETSTATE       0xA069
#define LC3_TRAP_HSETSTATE       0xA06A
#define LC3_TRAP_INITAPPLCACHE   0xA06C
#define LC3_TRAP_COUNTADBS       0xA077  /* Handler @ 0x4080A360 */
#define LC3_TRAP_GETINDADB       0xA078  /* Handler @ 0x4080A37A */
#define LC3_TRAP_GETADBINFO      0xA079  /* Handler @ 0x4080A3A6 */
#define LC3_TRAP_SETADBINFO      0xA07A  /* Handler @ 0x4080A3AC */
#define LC3_TRAP_ADBREINIT       0xA07B  /* Handler @ 0x4080A75A */
#define LC3_TRAP_ADBDELAY        0xA07C
#define LC3_TRAP_SETHFILTYP      0xA07D
#define LC3_TRAP_SETHOSTYP       0xA07E
#define LC3_TRAP_HOLDMEMORY      0xA0BC  /* Lock virtual pages in RAM */
#define LC3_TRAP_UNHOLDMEMORY    0xA0BD
#define LC3_TRAP_LOCKMEMORY      0xA0BE  /* Prevent VM paging */
#define LC3_TRAP_UNLOCKMEMORY    0xA0BF

/* ---- Color QuickDraw Traps ($AA00 - $AAFF) ---- */
#define LC3_TRAP_OPENCPORT       0xAA00  /* Initializes CGrafPort */
#define LC3_TRAP_INITGDEVICE     0xAA01  /* Initializes GDevice record */
#define LC3_TRAP_NEWGDEVICE      0xAA02  /* Allocates GDevice */
#define LC3_TRAP_DISPOSEGDEVICE  0xAA03  /* Frees GDevice */
#define LC3_TRAP_SETGDEVICE      0xAA04  /* Sets active screen device */
#define LC3_TRAP_GETGDEVICE      0xAA05  /* Returns active GDevice */
#define LC3_TRAP_DEVICELOOP      0xAA06  /* Iterates across screen devices */
#define LC3_TRAP_SETPXLPAT       0xAA0D  /* Sets pixel pattern */
#define LC3_TRAP_NEWPIXPAT       0xAA0E  /* Allocates PixPat */
#define LC3_TRAP_DISPOSEPIXPAT   0xAA0F  /* Frees PixPat */
#define LC3_TRAP_COPYPIXPAT      0xAA10  /* Deep copies PixPat */
#define LC3_TRAP_MAKEPIXPAT      0xAA11  /* Creates PixPat from pattern */
#define LC3_TRAP_NEWPIXMAP       0xAA12  /* Allocates PixMap */
#define LC3_TRAP_DISPOSEPIXMAP   0xAA13  /* Frees PixMap */
#define LC3_TRAP_COPYPIXMAP      0xAA14  /* Deep copies PixMap */
#define LC3_TRAP_SETCPORTPIX     0xAA15  /* Sets CGrafPort PixMap */
#define LC3_TRAP_GETCPORTPIX     0xAA16  /* Gets CGrafPort PixMap */
#define LC3_TRAP_RGBFORECOLOR    0xAA1E  /* Sets RGB forecolor */
#define LC3_TRAP_RGBBACKCOLOR    0xAA1F  /* Sets RGB backcolor */
#define LC3_TRAP_SETCPIXEL       0xAA20  /* Sets pixel color at (h, v) */
#define LC3_TRAP_GETCPIXEL       0xAA21  /* Gets pixel color at (h, v) */
#define LC3_TRAP_GETCTABLE       0xAA22  /* Reads ColorTable */
#define LC3_TRAP_DISPOSECTABLE   0xAA23  /* Frees ColorTable */

/* ---- SCSI & Sound Dispatch Traps ---- */
#define LC3_TRAP_SCSIDISPATCH    0xA815  /* Handler @ 0x40807460 */
#define LC3_TRAP_SNDDOCOMMAND    0xA803  /* Sound Manager: Enqueues sound command */
#define LC3_TRAP_SNDDOIMMEDIATE  0xA804  /* Sound Manager: Immediate sound hardware cmd */
#define LC3_TRAP_SNDPLAY         0xA805  /* Sound Manager: Async play 'snd ' rsrc */
#define LC3_TRAP_SNDNEWCHANNEL   0xA807  /* Sound Manager: Allocates audio channel */
#define LC3_TRAP_SNDDISPOSECHAN  0xA808  /* Sound Manager: Destroys audio channel */
#define LC3_TRAP_POPUPMENUSEL    0xA80B  /* PopUpMenuSelect */
#define LC3_TRAP_RESOURCEDISP    0xA822  /* Resource Manager partial read/write */
#define LC3_TRAP_ALIASDISPATCH   0xA823  /* System 7 Alias Manager */
#define LC3_TRAP_FONTDISPATCH    0xA854  /* TrueType rasterizer in ROM */

#endif /* LC3_TRAPS_H */
