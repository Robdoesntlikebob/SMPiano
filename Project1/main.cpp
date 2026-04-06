//written by: Jonny (Discord @jonnyptn), Rob (Discord @rob_doesnt_like_bob6962), Screwtape (Discord @screwtapello)

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include "sndEMU/SPC_DSP.h"
#include "sndEMU/SPC_Filter.h"
#include "stdc++.h"
#include "imgui.h"
#include "imgui-SFML.h"

enum g_regs {
	MVOLL = 0x0C,
	MVOLR = 0x1C,
	EVOLL = 0x2C,
	EVOLR = 0x3C,
	KON = 0x4C,
	KOF = 0x5C,
	FLG = 0x6C, //0x60
	ENDX = 0x7C,
	EFB = 0x0D,
	PMON = 0x2D,
	NON = 0x3D,
	EON = 0x4D,
	DIR = 0x5D,
	ESA = 0x6D,
	EDL = 0x7D,
};
enum g_values {
	ADSRMODE = 128,
	FLGRESET = 128,
	FLGMUTE = 64,
	FLGECEN = 32,
};

enum g_edlbytes {
	DL0 = 0,
	DL1 = 2048,
	DL2 = 4096,
	DL3 = 6144,
	DL4 = 8192,
	DL5 = 10240,
	DL6 = 12288,
	DL7 = 14336,
	DL8 = 16384,
	DL9 = 18432,
	DLA = 20480,
	DLB = 22528,
	DLC = 24576,
	DLD = 26624,
	DLE = 28672,
	DLF = 30720,
};

unsigned char BRR_SAWTOOTH[] = {
	0xB0, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
	0xB3, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
};

unsigned char c700sqwave[] = {
	0b10000100, 0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
	0b11000000, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
	0b11000000, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
	0b11000000, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
	0b11000011, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77
};

// Constants for the DSP registers.
#define vvoll(n) (n<<4)
#define vvolr(n) (n<<4)+1
#define vpl(n) (n<<4)+2
#define vph(n) (n<<4)+3
#define vsrcn(n) (n<<4)+4
#define vadsr1(n) (n<<4)+5
#define vadsr2(n) (n<<4)+6
#define vgain(n) (n<<4)+7
#define venvx(n) (n<<4)+8
#define voutx(n) (n<<4)+9
#define len(x) *(&x+1)-x
#define lobit(x) x&0xff
#define hibit(x) x>>8
#define print(x) std::cout << x << std::endl;
#define CHANNELS { sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight }
#define nKON 255-dsp.read(KON)

class SPC700 : public sf::SoundStream
{
private:
	std::array<unsigned char, 0x10000> aram{};
	unsigned dpos = 0;
	unsigned ROMdpos = 0;
	unsigned vxkon = 0;
	unsigned pos = 0x7c00;
	unsigned smptotalsize = 0;
	unsigned ROMpos = 0;
	unsigned aramdir = 0;

public:
	SPC_DSP dsp{};
	SPC_Filter filter{};
	std::array<SPC_DSP::sample_t, 2> buffer{};
	std::array<unsigned char, 15204352> ROM{}; /*might need it and might not lmao*/
	bool f = false;
	unsigned smpCOUNT = 0;
	SPC700()
	{
		aram.fill(0);
		dsp.init(aram.data());
		initialize(2, 32000, CHANNELS);
		dsp.set_output(buffer.data(), buffer.size());
		dsp.reset();
		dsp.write(MVOLL, 127);
		dsp.write(MVOLR, 127);
		dsp.write(DIR, 0);
		dsp.write(ESA, 4);
		dsp.write(FLG, 0);
		/*echo MAX test*/
		dsp.write(0x0F, 127);
		dsp.write(EFB, 0x70);
		dsp.write(EVOLL, 0xC);
		dsp.write(EVOLR, 0xC);
		dsp.write(EDL, 0xF);
		/*-------------*/
		dsp.write(NON, 0x00);
		dsp.write(PMON, 0x00);
		dsp.write(KON, 0);
		dsp.write(KOF, nKON);
	}
	bool onGetData(Chunk& data) override
	{
		dsp.set_output(buffer.data(), buffer.size());
		//for (int i = 0; i < 8; i++) {
		//	if (vxkon > 0) { dsp.write(vvoll(i), dsp.read(vvoll(i)) / vxkon); dsp.write(vvolr(i), dsp.read(vvolr(i)) / vxkon); }
		//	else continue;
		//}
		if (smptotalsize > dsp.read(EDL)*2048) {
			dsp.write(EDL, dsp.read(EDL) - 1);
		}
		if (f) {
			filter.run(buffer.data(), 32*(buffer.size()/2));
		}
		dsp.run(32 * (buffer.size()/2));
		data.sampleCount = buffer.size();
		data.samples = buffer.data();
		return true;
		//}
	} void onSeek(sf::Time timeOffset) override {};

	void pitch(unsigned pitch, unsigned voice) {
		dsp.write(vpl(voice), lobit(pitch));
		dsp.write(vph(voice), hibit(pitch)&0x3f);
	}

	void newsample(unsigned char* sample, size_t len, unsigned loop) {
		if (pos + len > 0xffff || smptotalsize >= 33792) {
			memcpy(ROM.data() + ROMpos, sample, len);
			ROM.data()[ROMdpos] = lobit(ROMpos); ROMdpos++;
			ROM.data()[ROMdpos] = hibit(ROMpos); ROMdpos++;
			ROM.data()[ROMdpos] = lobit(loop + ROMpos); ROMdpos++;
			ROM.data()[ROMdpos] = hibit(loop + ROMpos); ROMdpos++;
			ROMpos += len;
			smpCOUNT++;
		}
		else {
			memcpy(aram.data() + pos, sample, len);
			aram.data()[aramdir + dpos] = lobit(pos); dpos++;
			aram.data()[aramdir + dpos] = hibit(pos); dpos++;
			aram.data()[aramdir + dpos] = lobit(loop+pos); dpos++;
			aram.data()[aramdir + dpos] = hibit(loop+pos); dpos++;
			pos += len;
			smpCOUNT++;
			smptotalsize += len;
		}
	}

	void note(unsigned voice, bool hz, unsigned p, unsigned srcn, unsigned voll, unsigned volr, unsigned adsr1, unsigned gadsr2) {
		vxkon++;
		dsp.write(vsrcn(voice), srcn);
		dsp.write(vvoll(voice), voll/vxkon);
		dsp.write(vvolr(voice), volr/vxkon);
		if (hz) pitch(pow(2, 12) * (p / 32000), voice);
		else pitch(p, voice);
		dsp.write(vadsr1(voice), adsr1);
		dsp.write(vadsr2(voice), gadsr2);
		if (adsr1 & 128 != 128) dsp.write(vgain(voice), gadsr2);
		dsp.write(KON, dsp.read(KON) | 1 << voice);
		dsp.write(KOF, nKON);
	}

	void note(unsigned voice, bool hz, unsigned p, unsigned srcn, unsigned vol, unsigned adsr1, unsigned gadsr2) {
		vxkon++;
		dsp.write(vsrcn(voice), srcn);
		dsp.write(vvoll(voice), vol/vxkon);
		dsp.write(vvolr(voice), vol/vxkon);
		if (hz) pitch(4096 * (p / 32000), voice);
		else pitch(p, voice);
		dsp.write(vadsr1(voice), adsr1);
		dsp.write(vadsr2(voice), gadsr2);
		if (adsr1 & 128 != 128) dsp.write(vgain(voice), gadsr2);
		dsp.write(KON, dsp.read(KON) | 1 << voice);
		dsp.write(KOF, nKON);
	}
	void endnote(unsigned voice) {
		dsp.write(KON, dsp.read(KON) - (1<<voice) );
		dsp.write(KOF, nKON);
		vxkon--;
	}
	void fON() {
		f = !f;
	}
	void eon(unsigned voice) {
		assert(0 < voice && voice < 7);
		dsp.write(EON, 1 << voice);
	}
	void samplefromfile(const char* brr) {
		std::ifstream x(brr, std::ios::binary);
		x.seekg(0, std::ios::end); std::streamsize size = x.tellg();
		if (size % 9 == 2) {
			x.seekg(1, std::ios::beg);
			std::vector<unsigned char> lp(size);
			int loop = lp.data()[0];
			print(loop);
			x.seekg(2, std::ios::beg);
			std::vector<unsigned char> y(size); newsample(y.data(), y.size(), 0);
		}
		else if (size%9==0) {
			x.seekg(0, std::ios::beg);
			std::vector<unsigned char> y(size); newsample(y.data(), y.size(), 0);
		}
		else {
			perror("BRRError: Corrupt BRR");
		}
	}
};

using namespace ImGui;
SPC700 emu;


int main()
{
	emu.play();
	/*test: add smas-ac and smas-jc (~90KB total)*/
	//emu.samplefromfile("smas/07 SMW @5.brr");
	/*ngngngngngngngngngngngngngngngngngngngngngng*/
	emu.newsample(c700sqwave, len(c700sqwave), 9);
	emu.newsample(BRR_SAWTOOTH, len(BRR_SAWTOOTH), 0);
	emu.eon(0);
	sf::RenderWindow window(sf::VideoMode({1280,720}), "SMPiano");
	ImGui::SFML::Init(window);
	sf::Clock dt;
	bool s1on = false;
	bool h1on = true;
	bool s2on = false;
	bool h2on = true;
	bool s3on = false;
	bool h3on = true;
	bool s4on = false;
	bool h4on = true;
	bool s5on = false;
	bool h5on = true;
	bool s6on = false;
	bool h6on = true;
	bool s7on = false;
	bool h7on = true;
	bool s8on = false;
	bool h8on = true;
	/*notation: n = note, c = current, t = tuning*/
	int nvoice = 0;
	int csrcn = 0;
	int cvoll = 127;
	int cvolr = 127;
	char cad1 = 138;
	char cgad2 = 224;
	char vvoll[8] = { 127, 127, 127, 127, 127, 127, 127, 127 };
	char vvolr[8] = { 127, 127, 127, 127, 127, 127, 127, 127 };
	int tSPC = 0x107f;
	int zoomlv = 1;

#define black 0,0,0,255
#define white 255,255,255,255
#define blank 0,0,0,0
#define gy1 0x7f,0x7f,0x7f,255
#define gy2 0x3f,0x3f,0x3f,255
#define o1 0xff,0x7f,0,255

	constexpr uint8_t m_on[] = {
		white,white,blank,white,white,
		white,black,blank,black,white,
		white,black,blank,black,white,
		blank,blank,blank,blank,blank
	};
	constexpr uint8_t m_off[] = {
		blank,blank,blank,blank,blank,
		blank,blank,blank,blank,blank,
		black,black,blank,black,black,
		blank,blank,blank,blank,blank
	};
	constexpr uint8_t au_on[] = {
		blank,blank,blank,gy1,blank,blank,blank,o1,blank,blank,
		blank,blank,gy1,white,blank,o1,blank,blank,o1,blank,
		blank,gy1,gy1,white,white,blank,o1,blank,o1,blank,
		blank,gy1,gy1,gy1,gy1,blank,o1,blank,o1,blank,
		blank,gy1,gy1,gy1,gy1,blank,o1,blank,o1,blank,
		blank,gy1,gy1,gy1,gy2,blank,o1,blank,o1,blank,
		blank,blank,gy1,gy2,blank,o1,blank,blank,o1,blank,
		blank,blank,blank,gy2,blank,blank,blank,o1,blank,blank
	};
	constexpr uint8_t au_off[] = {
		blank,blank,blank,gy2,blank,blank,blank,gy2,blank,blank,
		blank,blank,gy2,gy1,blank,gy2,blank,blank,gy2,blank,
		blank,gy2,gy2,gy1,gy1,blank,gy2,blank,gy2,blank,
		blank,gy2,gy2,gy2,gy2,blank,gy2,blank,gy2,blank,
		blank,gy2,gy2,gy2,gy2,blank,gy2,blank,gy2,blank,
		blank,gy2,gy2,gy2,gy2,blank,gy2,blank,gy2,blank,
		blank,blank,gy2,gy2,blank,gy2,blank,blank,gy2,blank,
		blank,blank,blank,gy2,blank,blank,blank,gy2,blank,blank
	};

	ImGuiIO &io = GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("SNES.ttf", 25.f);
	io.Fonts->AddFontFromFileTTF("SMW.ttf", 50.f);
	#define SMWfnt io.Fonts->Fonts[1]
	#define SNESfnt io.Fonts->Fonts[0]
	#define ENDfnt PopFont()
	#define MENUBAR 31

	SFML::UpdateFontTexture();
	while (window.isOpen()) {
		ImVec2 w = window.getSize();
		auto [width, height] = w;
		float wfactor = width / 1280.f;
		float hfactor = height / 720.f;
		while (const auto event = window.pollEvent()) {
			SFML::ProcessEvent(window, *event);
#define event(e) event->is<sf::Event::##e>()
			if (event(Closed)) {
				window.close();
			}
		}
		/*necessary variables and theme*/
		ImGuiStyle& theme = GetStyle();
		ImVec4 ogbtn = theme.Colors[ImGuiCol_Button]; //temp
		ImVec4 oghvr = theme.Colors[ImGuiCol_ButtonHovered]; //temp
		ImVec4 ogav = theme.Colors[ImGuiCol_ButtonActive]; //temp
		ImVec4 ogtxt = theme.Colors[ImGuiCol_Text]; //temp
		theme.FrameRounding = 2.0f;
		theme.Colors[ImGuiCol_WindowBg] = { 0.71f, 0.71f, 0.89f, 1.00f };
		theme.Colors[ImGuiCol_Border] = { 0.25f, 0.25f, 0.25f, 1.00f };
		//theme.FramePadding = { 5,10 };
		//theme.ItemSpacing = { 10,4 };
		theme.WindowTitleAlign = { 0.5f,0.5f };
		ImVec2 ogWP = theme.WindowPadding;
		w = window.getSize();
		width = w.x; height = w.y;
		wfactor = width / 1280;
		hfactor = height / 720;
		SFML::Update(window, dt.restart());
		if (io.KeysDown[GetKeyIndex(ImGuiKey_1)])csrcn = 0;
		if (io.KeysDown[GetKeyIndex(ImGuiKey_2)])csrcn = 1;
		if (io.KeysDown[GetKeyIndex(ImGuiKey_3)])csrcn = 2;
		//ShowStyleEditor(&theme);



		/*this is where the fun begins*/
		PushFont(SNESfnt);	
		SetNextWindowSize({ window.getSize() });
		SetNextWindowPos({ 0,0 });
		Begin("Main", 0, ImGuiWindowFlags_NoTitleBar bitor ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_MenuBar bitor ImGuiWindowFlags_NoBringToFrontOnFocus);
		if (BeginMenuBar()) {
			if(BeginMenu("File")){
				if (MenuItem("New	", "Ctrl+N")){}
				if (MenuItem("Save	", "Ctrl+S")){}
				if (MenuItem("Save As	", "Ctrl+Shift+S")){}
				if (MenuItem("Export	", "Ctrl+E")){}
				if (MenuItem("Repeat Last Export	", "Ctrl+Shift+E")){}
				EndMenu();
			}
			if (BeginMenu("Samples")) {
				if (MenuItem("Import Sample(s)	", "Ctrl+I")) {
					/*Add open file dialog file*/
				}
				if (MenuItem("Filter Mode	", "Ctrl+F", emu.f)) {
					emu.fON();
				}
				EndMenu();
			}
			EndMenuBar();
		}
		ENDfnt;
		SMWfnt->Scale = 0.22f;
		PushFont(SMWfnt);
		SetNextWindowSize({width*80/100.f,(height/3.f)});
		SetNextWindowPos({ 0,MENUBAR });
		Begin("Channels", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoScrollbar bitor ImGuiWindowFlags_NoCollapse);
		BeginChild("ChannelSep", { 100,0 }, ImGuiChildFlags_Border, ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoScrollbar);
		sf::Texture s1(sf::Vector2u(5, 4), false);  if (s1on)s1.update(m_on); else s1.update(m_off);
		sf::Texture h1(sf::Vector2u(10, 8), false); if (h1on)h1.update(au_on); else h1.update(au_off);
		sf::Texture s2(sf::Vector2u(5, 4), false);  if (s2on)s2.update(m_on); else s2.update(m_off);
		sf::Texture h2(sf::Vector2u(10, 8), false); if (h2on)h2.update(au_on); else h2.update(au_off);
		sf::Texture s3(sf::Vector2u(5, 4), false);  if (s3on)s3.update(m_on); else s3.update(m_off);
		sf::Texture h3(sf::Vector2u(10, 8), false); if (h3on)h3.update(au_on); else h3.update(au_off);
		sf::Texture s4(sf::Vector2u(5, 4), false);  if (s4on)s4.update(m_on); else s4.update(m_off);
		sf::Texture h4(sf::Vector2u(10, 8), false); if (h4on)h4.update(au_on); else h4.update(au_off);
		sf::Texture s5(sf::Vector2u(5, 4), false);  if (s5on)s5.update(m_on); else s5.update(m_off);
		sf::Texture h5(sf::Vector2u(10, 8), false); if (h5on)h5.update(au_on); else h5.update(au_off);
		sf::Texture s6(sf::Vector2u(5, 4), false);  if (s6on)s6.update(m_on); else s6.update(m_off);
		sf::Texture h6(sf::Vector2u(10, 8), false); if (h6on)h6.update(au_on); else h6.update(au_off);
		sf::Texture s7(sf::Vector2u(5, 4), false);  if (s7on)s7.update(m_on); else s7.update(m_off);
		sf::Texture h7(sf::Vector2u(10, 8), false); if (h7on)h7.update(au_on); else h7.update(au_off);
		sf::Texture s8(sf::Vector2u(5, 4), false);  if (s8on)s8.update(m_on); else s8.update(m_off);
		sf::Texture h8(sf::Vector2u(10, 8), false); if (h8on)h8.update(au_on); else h8.update(au_off);
		constexpr ImGuiChildFlags chcprop = ImGuiChildFlags_Border;
		constexpr ImGuiWindowFlags chwprop = ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoScrollbar bitor ImGuiWindowFlags_NoScrollWithMouse;
		theme.Colors[ImGuiCol_Button] = { 0,0,0,0 };
		theme.Colors[ImGuiCol_ButtonHovered] = { 0,0,0,0 };
		theme.Colors[ImGuiCol_ButtonActive] = { 0,0,0,0 };
		BeginChild("Channel 1", { 85, 50 }, chcprop, chwprop);
		Text("Channel 1");
		Separator();
		ImageButton("View Track", s1, { 20,16 });
		if (IsItemClicked()) {
			s1on = !s1on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h1, { 20,16 });
		if (IsItemClicked()) {
			h1on = !h1on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 2", { 85, 50 }, chcprop, chwprop);
		Text("Channel 2");
		Separator();
		ImageButton("View Track", s2, { 20,16 });
		if (IsItemClicked()) {
			s2on = !s2on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h2, { 20,16 });
		if (IsItemClicked()) {
			h2on = !h2on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 3", { 85, 50 }, chcprop, chwprop);
		Text("Channel 3");
		Separator();
		ImageButton("View Track", s3, { 20,16 });
		if (IsItemClicked()) {
			s3on = !s3on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h3, { 20,16 });
		if (IsItemClicked()) {
			h3on = !h3on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 4", { 85, 50 }, chcprop, chwprop);
		Text("Channel 4");
		Separator();
		ImageButton("View Track", s4, { 20,16 });
		if (IsItemClicked()) {
			s4on = !s4on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h4, { 20,16 });
		if (IsItemClicked()) {
			h4on = !h4on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 5", { 85, 50 }, chcprop, chwprop);
		Text("Channel 5");
		Separator();
		ImageButton("View Track", s5, { 20,16 });
		if (IsItemClicked()) {
			s5on = !s5on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h5, { 20,16 });
		if (IsItemClicked()) {
			h5on = !h5on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 6", { 85, 50 }, chcprop, chwprop);
		Text("Channel 6");
		Separator();
		ImageButton("View Track", s6, { 20,16 });
		if (IsItemClicked()) {
			s6on = !s6on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h6, { 20,16 });
		if (IsItemClicked()) {
			h6on = !h6on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 7", { 85, 50 }, chcprop, chwprop);
		Text("Channel 7");
		Separator();
		ImageButton("View Track", s7, { 20,16 });
		if (IsItemClicked()) {
			s7on = !s7on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h7, { 20,16 });
		if (IsItemClicked()) {
			h7on = !h7on;
			/*TODO: Enable Track Code*/
		}
		EndChild();

		BeginChild("Channel 8", { 85, 50 }, chcprop, chwprop);
		Text("Channel 8");
		Separator();
		ImageButton("View Track", s8, { 20,16 });
		if (IsItemClicked()) {
			s8on = !s8on;
			/*TODO: View Track Code*/
		}
		SameLine(0, -1);
		ImageButton("Enable Track", h8, { 20,16 });
		if (IsItemClicked()) {
			h8on = !h8on;
			/*TODO: Enable Track Code*/
		}
		EndChild();
		theme.Colors[ImGuiCol_Button] = ogbtn;
		theme.Colors[ImGuiCol_ButtonHovered] = oghvr;
		theme.Colors[ImGuiCol_ButtonActive] = ogav;
		EndChild();

		End();
		SetNextWindowSize({width*20/100.f, height/3.f});
		SetNextWindowPos({width,MENUBAR},0,{1,0});
		Begin("Songs", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoCollapse bitor ImGuiWindowFlags_NoScrollbar);
		/*songs*/
		End();
		SetNextWindowSize({ width * 20 / 100.f, height / 3.f });
		SetNextWindowPos({ width,MENUBAR+(height / 3.f)},0,{1,0});
		Begin("Samples", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoCollapse bitor ImGuiWindowFlags_NoScrollbar);
		/*samples*/
		End();
		SetNextWindowSize({ width * 20 / 100.f, height / 3.f });
		SetNextWindowPos({ width,MENUBAR + 2*(height / 3.f)-1 }, 0, { 1,0 });
		Begin("Instruments", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoCollapse bitor ImGuiWindowFlags_NoScrollbar);
		/*instruments*/
		End();

		ImVec2 oISPACING = theme.ItemSpacing;
		ImVec2 oFPAD = theme.FramePadding;
		theme.ItemSpacing = { 0,0 };
		theme.FramePadding = {2,2};
		theme.WindowPadding = ImVec2(0,0);
		int ogwbs = theme.WindowBorderSize; theme.WindowBorderSize = 0;
#define PTBAR 15
		SetNextWindowPos({ 0, (height/3.f)+MENUBAR+PTBAR });
		BeginChild("PianoRoll", { width * 80 / 100.f, (height * 2 / 3.f) - MENUBAR-PTBAR },0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoScrollbar bitor ImGuiWindowFlags_NoTitleBar);
		#define blacknote theme.Colors[ImGuiCol_Button] = { 0,0,0,1 };\
		theme.Colors[ImGuiCol_ButtonActive] = { 0.25f,0.25f,0.25f,1 };\
		theme.Colors[ImGuiCol_ButtonHovered] = { 0.5f,0.5f,0.5f,1 };\
		theme.Colors[ImGuiCol_Text] = { 1,1,1,1 }
		#define whitenote theme.Colors[ImGuiCol_Button] = { 1,1,1,1 };\
		theme.Colors[ImGuiCol_ButtonActive] = { 0.5f,0.5f,0.5f,1 };\
		theme.Colors[ImGuiCol_ButtonHovered] = { 0.75f,0.75f,0.75f,1 };\
		theme.Colors[ImGuiCol_Text] = {0,0,0,1}
		#define reset theme.Colors[ImGuiCol_Button] = ogbtn;\
		theme.Colors[ImGuiCol_ButtonHovered] = oghvr;\
		theme.Colors[ImGuiCol_ButtonActive] = ogav;\
		theme.Colors[ImGuiCol_Text] = ogtxt;\
		theme.ItemSpacing = oISPACING;\
		theme.FramePadding = oFPAD;\
		theme.WindowPadding = ogWP;\
		theme.WindowBorderSize = ogwbs

		std::string notenames[] = { "B", "A#", "A", "G#", "G", "F#", "F", "E", "D#", "D", "C#", "C" };
		std::string midiVALUES[87];
		int namesptr = 0;
		int notesnum = 7;
		int notesscale = 1;
		float notewidth = 85.f;
		/*Create MIDI numbers and put them in array*/
		for (int i = 0; i < 87; ++i) {
			midiVALUES[i] = notenames[namesptr++] + std::to_string(notesnum);
			if (namesptr >= 12) {
				namesptr = 0; --notesnum;
			}
		}
		/*Create the buttons (hopefully)*/
		for (int i = 0; i < 87; ++i) {
			std::string label = midiVALUES[i];
			for (char ch : midiVALUES[i]) {
				if (label.find("#")!=std::string::npos) {
					label = "	 " + midiVALUES[i];
					blacknote;
					notewidth = 70.f;
				}
				else {
					label = "		" + midiVALUES[i];
					whitenote;
					notewidth = 85.f;
				}
			}
			const char* flabel = label.c_str();
			Button(label.c_str(), {notewidth, height*0.025f*notesscale});
			if (IsItemActivated()) emu.note(nvoice, false , (unsigned)tSPC * (pow(2.0, (47 - i) / 12.0)), csrcn, vvoll[nvoice], vvolr[nvoice], cad1, cgad2); else if (IsItemDeactivated()) emu.endnote(0);
		}
		reset;
#define ptheight (height * 2 / 3.f) - MENUBAR
#define sprintf sprintf_s
		float ptx = 85.0f;
		float pty = (height / 3.f) + MENUBAR;
		float ptlength = 400.f;
		int ptcounter = 1;
		char ptnm[20] = "";
		sprintf(ptnm, "Pattern %i", ptcounter++);
		SetNextWindowPos({ ptx, pty }); ptx += ptlength;
		SetNextWindowSize({ ptlength*zoomlv*wfactor, ptheight });
		Begin(ptnm, 0, ImGuiWindowFlags_NoResize  bitor ImGuiWindowFlags_NoCollapse bitor ImGuiWindowFlags_NoMove);
		if (GetIO().MouseWheel == -1.f) {
			zoomlv--;
			if (zoomlv < 0) zoomlv = 0;
		}
		else if (GetIO().MouseWheel == 1.f) {
			zoomlv++;
			if (zoomlv > 6) zoomlv = 6;
		}
		End();

		End();
		//float ptx = 85.0f;
		//float pty = (height / 3.f) + MENUBAR;
		//int ptcounter = 1;
//#define ptheight (height * 2 / 3.f) - MENUBAR
//#define sprintf sprintf_s
//		SetNextWindowPos({ 85.0f,(height/3.f)+MENUBAR });
//		BeginChild("PatternsArea", { width * 80 / 100.f - (84.0f),(height * 2 / 3.f) - MENUBAR }, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoScrollbar bitor ImGuiWindowFlags_NoTitleBar bitor ImGuiWindowFlags_AlwaysHorizontalScrollbar bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoBringToFrontOnFocus);
//		
		//SetNextWindowPos({ ptx, pty }); ptx += 170.f;
		//SetNextWindowSize({ 170.f, ptheight });
		//char ptnm[20] = "";
		//sprintf(ptnm, "Pattern %i", ptcounter++);
		//Begin(ptnm, 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoCollapse);
		//End();
//		
//		EndChild();
		/*idk what goes in here lol*/

		/*this is where it ends :broken_heart:*/
		ENDfnt;
		End();
		window.clear();
		SFML::Render(window);
		window.display();
	}
	SFML::Shutdown();
	return 0;
}