_G.ffi = require ("ffi")
--start of module (table of functions id assume)
local SNES = {}
local gme700 = ffi.load("libgme.dll")

ffi.cdef[[
    typedef const char* gme_err_t;
    typedef struct Music_Emu Music_Emu;
    typedef const struct gme_type_t_* gme_type_t;

    extern const gme_type_t
	gme_ay_type,
	gme_gbs_type,
	gme_gym_type,
	gme_hes_type,
	gme_kss_type,
	gme_nsf_type,
	gme_nsfe_type,
	gme_sap_type,
	gme_spc_type,
	gme_vgm_type,
	gme_vgz_type;

    void gme_set_stereo_depth( Music_Emu*, double depth );
    gme_type_t gme_type( Music_Emu const* );
    Music_Emu* gme_new_emu_multi_channel( gme_type_t, int sample_rate );
    void gme_set_fade( Music_Emu*, int start_msec );
    gme_err_t gme_open_file( const char path [], Music_Emu** out, int sample_rate );
    gme_err_t gme_play( Music_Emu*, int count, short out [] );
    gme_err_t gme_load_file( Music_Emu*, const char path [] );

	// Resets DSP to power-on state
	void reset();

	// Emulates pressing reset switch on SNES
	void soft_reset();

	// Reads/writes DSP registers. For accuracy, you must first call spc_run_dsp()
	// to catch the DSP up to present.
	int  read ( int addr ) const;
	void write( int addr, int data );

	// Runs DSP for specified number of clocks (~1024000 per second). Every 32 clocks
	// a pair of samples is be generated.
	void run( int clock_count );
]]

local spc = nil
local spctype = nil
local emu = nil
local buffer = nil
local bfs = 512

function SNES.load()
    spc = ffi.new("Music_Emu*[1]")
    spctype = gme700.gme_spc_type
    emu = gme700.gme_new_emu_multi_channel(spctype, 32000)
    gme700.gme_set_fade(emu, 1000)
    --Debug, delete soon
    print(spc)
    print(spctype)
end

-- local function pcm2snd(fn)
--     local source = love.audio.newQueueableSource(32000, 16, 2, 8)
--     for i=1,8 do
--         fn()
--         local soundData = love.sound.newSoundData(bfs, 32000, 16, 2)
--         for x=0,bfs/2-1 do
--             soundData:
--         end
--     end
-- end

function SNES.play(file)
    bfs = 512
    buffer = ffi.new("short[?]", bfs)
    gme700.gme_load_file(emu, file)
    local x, y = pcall(function () gme700.gme_play(emu, 10, buffer) end)
    if not x then print (y) end
end


--for some reason ts is necessary now :/
return SNES