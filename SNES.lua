_G.ffi = require ("ffi")
--start of module (table of functions id assume)
local SNES = {}
local gme700 = ffi.load("sndEMU.dll")

ffi.cdef[[
typedef struct SPC_DSP SPC_DSP;

/* Creates new DSP emulator. NULL if out of memory. */
SPC_DSP* spc_dsp_new( void );

/* Frees DSP emulator */
void spc_dsp_delete( SPC_DSP* );

/* Initializes DSP and has it use the 64K RAM provided */
void spc_dsp_init( SPC_DSP*, void* ram_64k );

/* Sets destination for output samples. If out is NULL or out_size is 0,
doesn't generate any. */
typedef short spc_dsp_sample_t;
void spc_dsp_set_output( SPC_DSP*, spc_dsp_sample_t* out, int out_size );

/* Number of samples written to output since it was last set, always
a multiple of 2. Undefined if more samples were generated than
output buffer could hold. */
int spc_dsp_sample_count( SPC_DSP const* );


/**** Emulation *****/

/* Resets DSP to power-on state */
void spc_dsp_reset( SPC_DSP* );

/* Emulates pressing reset switch on SNES */
void spc_dsp_soft_reset( SPC_DSP* );

/* Reads/writes DSP registers. For accuracy, you must first call spc_dsp_run() */
/* to catch the DSP up to present. */
int  spc_dsp_read ( SPC_DSP const*, int addr );
void spc_dsp_write( SPC_DSP*, int addr, int data );

/* Runs DSP for specified number of clocks (~1024000 per second). Every 32 clocks */
/* a pair of samples is be generated. */
void spc_dsp_run( SPC_DSP*, int clock_count );


/**** Sound control *****/

/* Mutes voices corresponding to non-zero bits in mask. Reduces emulation accuracy. */
enum { spc_dsp_voice_count = 8 };
void spc_dsp_mute_voices( SPC_DSP*, int mask );

/* If true, prevents channels and global volumes from being phase-negated.
Only supported by fast DSP; has no effect on accurate DSP. */
void spc_dsp_disable_surround( SPC_DSP*, int disable );


/**** State save/load *****/

/* Resets DSP and uses supplied values to initialize registers */
enum { spc_dsp_register_count = 128 };
void spc_dsp_load( SPC_DSP*, unsigned char const regs [spc_dsp_register_count] );

/* Saves/loads exact emulator state (accurate DSP only) */
enum { spc_dsp_state_size = 640 }; /* maximum space needed when saving */
typedef void (*spc_dsp_copy_func_t)( unsigned char** io, void* state, size_t );
void spc_dsp_copy_state( SPC_DSP*, unsigned char** io, spc_dsp_copy_func_t );

/* Returns non-zero if new key-on events occurred since last call (accurate DSP only) */
int spc_dsp_check_kon( SPC_DSP* );

typedef const char* spc_err_t;

typedef struct SNES_SPC SNES_SPC;

/* Creates new SPC emulator. NULL if out of memory. */
SNES_SPC* spc_new( void );

/* Frees SPC emulator */
void spc_delete( SNES_SPC* );

/* Sets IPL ROM data. Library does not include ROM data. Most SPC music files
don't need ROM, but a full emulator must provide this. */
enum { spc_rom_size = 0x40 };
void spc_init_rom( SNES_SPC*, unsigned char const rom [spc_rom_size] );

/* Sets destination for output samples */
typedef short spc_sample_t;
void spc_set_output( SNES_SPC*, spc_sample_t* out, int out_size );

/* Number of samples written to output since last set */
int spc_sample_count( SNES_SPC const* );

/* Resets SPC to power-on state. This resets your output buffer, so you must
call spc_set_output() after this. */
void spc_reset( SNES_SPC* );

/* Emulates pressing reset switch on SNES. This resets your output buffer, so
you must call spc_set_output() after this. */
void spc_soft_reset( SNES_SPC* );

/* 1024000 SPC clocks per second, sample pair every 32 clocks */
typedef int spc_time_t;
enum { spc_clock_rate = 1024000 };
enum { spc_clocks_per_sample = 32 };

/* Reads/writes port at specified time */
enum { spc_port_count = 4 };
int  spc_read_port ( SNES_SPC*, spc_time_t, int port );
void spc_write_port( SNES_SPC*, spc_time_t, int port, int data );

/* Runs SPC to end_time and starts a new time frame at 0 */
void spc_end_frame( SNES_SPC*, spc_time_t end_time );


/**** Sound control ****/

/*Mutes voices corresponding to non-zero bits in mask. Reduces emulation accuracy. */
enum { spc_voice_count = 8 };
void spc_mute_voices( SNES_SPC*, int mask );

/* If true, prevents channels and global volumes from being phase-negated.
Only supported by fast DSP; has no effect on accurate DSP. */
void spc_disable_surround( SNES_SPC*, int disable );

/* Sets tempo, where spc_tempo_unit = normal, spc_tempo_unit / 2 = half speed, etc. */
enum { spc_tempo_unit = 0x100 };
void spc_set_tempo( SNES_SPC*, int );


/**** SPC music playback *****/

/* Loads SPC data into emulator. Returns NULL on success, otherwise error string. */
spc_err_t spc_load_spc( SNES_SPC*, void const* spc_in, long size );

/* Clears echo region. Useful after loading an SPC as many have garbage in echo. */
void spc_clear_echo( SNES_SPC* );

/* Plays for count samples and write samples to out. Discards samples if out
is NULL. Count must be a multiple of 2 since output is stereo. */
spc_err_t spc_play( SNES_SPC*, int count, short* out );

/* Skips count samples. Several times faster than spc_play(). */
spc_err_t spc_skip( SNES_SPC*, int count );


/**** State save/load (only available with accurate DSP) ****/

/* Saves/loads exact emulator state */
enum { spc_state_size = 67 * 1024L }; /* maximum space needed when saving */
typedef void (*spc_copy_func_t)( unsigned char** io, void* state, size_t );
void spc_copy_state( SNES_SPC*, unsigned char** io, spc_copy_func_t );

/* Writes minimal SPC file header to spc_out */
void spc_init_header( void* spc_out );

/* Saves emulator state as SPC file data. Writes spc_file_size bytes to spc_out.
Does not set up SPC header; use spc_init_header() for that. */
enum { spc_file_size = 0x10200 }; /* spc_out must have this many bytes allocated */
void spc_save_spc( SNES_SPC*, void* spc_out );

/* Returns non-zero if new key-on events occurred since last check. Useful for 
trimming silence while saving an SPC. */
int spc_check_kon( SNES_SPC* );


/**** SPC_Filter ****/

typedef struct SPC_Filter SPC_Filter;

/* Creates new filter. NULL if out of memory. */
SPC_Filter* spc_filter_new( void );

/* Frees filter */
void spc_filter_delete( SPC_Filter* );

/* Filters count samples of stereo sound in place. Count must be a multiple of 2. */
void spc_filter_run( SPC_Filter*, spc_sample_t* io, int count );

/* Clears filter to silence */
void spc_filter_clear( SPC_Filter* );

/* Sets gain (volume), where spc_filter_gain_unit is normal. Gains greater than
spc_filter_gain_unit are fine, since output is clamped to 16-bit sample range. */
enum { spc_filter_gain_unit = 0x100 };
void spc_filter_set_gain( SPC_Filter*, int gain );

/* Sets amount of bass (logarithmic scale) */
enum { spc_filter_bass_none =  0 };
enum { spc_filter_bass_norm =  8 }; /* normal amount */
enum { spc_filter_bass_max  = 31 };
void spc_filter_set_bass( SPC_Filter*, int bass );

]]

local spc
local dsp
local emu
local buffer
local aram
local filter

function SNES.load()
    buffer = ffi.new("short[?]", 512)
    aram = ffi.new("unsigned char[?]", 0x10000)
	spc=gme700.spc_new()
    dsp=gme700.spc_dsp_new()
    filter=gme700.spc_filter_new()
    gme700.spc_dsp_init(dsp, aram)
    local ipl = love.filesystem.read("ipl.sfc")
    local x,y = pcall(function()gme700.spc_init_rom(spc,ipl)end)
    if not x then print (y) else print("success") end
    gme700.spc_set_output(spc, buffer, 512)
    gme700.spc_dsp_set_output(dsp, buffer, 512)
end

---@param clocks integer Runs DSP for specified number of clocks (~1024000 per second). Every 32 clocks a pair of samples is generated. Preferred for accuracy
local function run(clocks)
    gme700.spc_dsp_run(dsp, clocks)
end

---@param addr integer Address (0x00 up to 0x7F, beacuse you are writing to DSP and not to SMP). If unsure, use a SPC700 reference.
local function read(addr)
    return gme700.spc_dsp_read(dsp, addr)
end

---@param addr integer Address (0x00 up to 0x7F, beacuse you are writing to DSP and not to SMP). If unsure, use a SPC700 reference.
---@param data integer Data to send to Address argument (0x00 up to 0xFF). If unsure, use a SPC700 reference.
local function write(addr, data)
    run(1)
    gme700.spc_dsp_write(dsp, addr, data)
end

--0x5d is the destination of samples (DIR)
--0xV4 is the sample number/chosen sample for channel V (VxSRCN)

local smpPOS = 0
if smpPOS > 255 then smpPOS = 0
print("You can't load any more samples") end --max limit for smpPOS (data) is 255 (0xff)

function SNES.brr2aram(x)
    local rawx = io.open(x, "rb")
    local xb = rawx:read("*a")
    write(0x5d, smpPOS) --set sample destination (DIR)
    local brrsize = #xb
    local lpOFF = 0
    --finding loop offest
    if brrsize % 9 == 2 then
        local xb1, xb2 = xb:byte(1,2)
        local y = string.format("%02X%02X", xb1, xb2)
        lpOFF = y:byte()
        print("Debug: Loop Offset: "..lpOFF) --debug
    elseif brrsize % 9 == 0 then return else error("SizeError: Corrupt BRR file") end
    local xbunsigned = ffi.new("unsigned char", xb:byte())
    aram[smpPOS] = xb:byte() --Sample start
    print("Debug: Sample Start: "..aram[smpPOS])
    print("Debug: Loop offset type: "..type(lpOFF))
    print("Debug: Sample start type: "..type(xb:byte()))
    local lpSTART = lpOFF + xb:byte()
    print("Debug: Loop Start: "..lpSTART)
    print(read(0x5d))
    smpPOS = smpPOS + 1
end

function SNES.play(x)
end

--for some reason ts is necessary now :/
return SNES