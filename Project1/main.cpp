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
	PMON = 0x2D,
	NON = 0x3D,
	EON = 0x4D,
	DIR = 0x5D, //0x67 (SIX SEVEN)
	ESA = 0x6D, //0x80
	EDL = 0x7D,
};
enum g_values {
	ADSRMODE = 128,
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

struct SPC700 : public sf::SoundStream
{
private:
	std::array<unsigned char, 0x10000> aram{};
	unsigned dpos = 0;
	unsigned vxkon = 0;
	unsigned pos = 0x200;
public:
	SPC_DSP dsp{};
	SPC_Filter filter{};
	std::array<SPC_DSP::sample_t, 32> buffer{};
	bool f = false;
	SPC700()
	{
		dsp.init(aram.data());
		initialize(2, 32000, CHANNELS);
		dsp.set_output(buffer.data(), buffer.size());
		dsp.reset();
		dsp.write(MVOLL, 127);
		dsp.write(MVOLR, 127);
		dsp.write(DIR, 0x67);
		dsp.write(FLG, 0x20);
		dsp.write(ESA, 0x80);
		dsp.write(EON, 0x00);
		dsp.write(NON, 0x00);
		dsp.write(PMON, 0x00);
		dsp.write(KON, 0);
		dsp.write(KOF, nKON);
	}
	bool onGetData(Chunk& data) override
	{
		bool ff = &f;
		dsp.set_output(buffer.data(), buffer.size());
		//for (int i = 0; i < 8; i++) {
		//	if (vxkon > 0) { dsp.write(vvoll(i), dsp.read(vvoll(i)) / vxkon); dsp.write(vvolr(i), dsp.read(vvolr(i)) / vxkon); }
		//	else continue;
		//}
		dsp.run(32 * (buffer.size()/2));
		if (ff) {
			filter.run(buffer.data(), buffer.size());
		}
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
		memcpy(aram.data() + pos, sample, len);
		aram.data()[0x6700 + dpos] = lobit(pos); dpos++;
		aram.data()[0x6700 + dpos] = hibit(pos); dpos++;
		aram.data()[0x6700 + dpos] = lobit(loop+pos); dpos++;
		aram.data()[0x6700 + dpos] = hibit(loop+pos); dpos++;
		pos += len;
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
};

using namespace ImGui;
SPC700 emu;

void testbutton() {
	ImGui::Button("		C");
	if (IsItemActivated()) {
		emu.note(0, false, 0x106e, 0, 0x7f, ADSRMODE + 10, 0xe0);
		//emu.note(1, 0, 0xa8c, 0, 0x7f, ADSRMODE + 10, 0xe0);
		std::bitset<8> kon1(emu.dsp.read(KON));
		print(kon1);
	}
	else if (IsItemDeactivated()) {
		emu.endnote(0);
		//emu.endnote(1);
		std::bitset<8> kof(emu.dsp.read(KON));
		print(kof);
	}
}

int main()
{
	emu.play();
	emu.newsample(c700sqwave, len(c700sqwave), 9);
	emu.newsample(BRR_SAWTOOTH, len(BRR_SAWTOOTH), 0);
	sf::RenderWindow window(sf::VideoMode({1280,720}), "SMPiano");
	ImGui::SFML::Init(window);
	sf::Clock dt;
	while (window.isOpen()) {
		/*necessary variables and theme*/
		ImVec2 w = window.getSize();
		auto [width, height] = w;
		ImGuiIO &io = GetIO();
		ImGuiStyle &theme = GetStyle();
		theme.FrameRounding = 2.0f;

		while (const auto event = window.pollEvent()) {
			SFML::ProcessEvent(window, *event);
#define event(e) event->is<sf::Event::##e>()
			if (event(Closed)) {
				window.close();
			}
		}
		SFML::Update(window, dt.restart());

		const uint8_t m_eyes_on[] = {
			0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff,
			0xffffffff, 0xffffffff, 0xff, 0xff,0, 0, 0xff, 0xff, 0xffffffff,0xffffffff
		};
		const uint8_t m_eyes_off[] = {
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			0xff,0xff,0xff,0xff,0,0,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0,0,0xff,0xff,0xff,0xff
		};
		sf::Texture see; see.update(m_eyes_on);
		bool s_on = true;

		/*this is where the fun begins*/
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
					emu.f = !emu.f;
				}
				EndMenu();
			}
			EndMenuBar();
		}
		SetNextWindowSize({width/1.2f, height/3.5f});
		SetNextWindowPos({ 0,20 });
		ImGuiChildFlags chcprop = ImGuiChildFlags_Border;
		ImGuiWindowFlags chwprop = ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoScrollbar;
		Begin("Channels", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_HorizontalScrollbar bitor ImGuiWindowFlags_AlwaysVerticalScrollbar);
		BeginChild("ChannelSep", { 100,400 }, ImGuiChildFlags_Border, ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_NoResize);
		BeginChild("Channel 1", { 0, 50 }, chcprop, chwprop);
		Text("Channel 1");
		Separator();
		ImageButton("See Track", see, {10,8});
		if (IsItemClicked) {
			see.update(m_eyes_off);
			/*TODO: show/dont show track code*/
		}
		EndChild();

		EndChild();

		End();
		SetNextWindowSize({width-(width/1.2f), (height/3)-20});
		SetNextWindowPos({ width/1.2f,20});
		Begin("Songs", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_AlwaysVerticalScrollbar bitor ImGuiWindowFlags_NoCollapse);
		/*songs*/
		End();
		SetNextWindowSize({width-(width/1.2f), height/3});
		SetNextWindowPos({ width/1.2f,height/3});
		Begin("Samples", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_AlwaysVerticalScrollbar bitor ImGuiWindowFlags_NoCollapse);
		/*samples*/
		End();
		SetNextWindowSize({width-(width/1.2f), height/3});
		SetNextWindowPos({ width/1.2f, height/1.5f});
		Begin("Instruments", 0, ImGuiWindowFlags_NoResize bitor ImGuiWindowFlags_NoMove bitor ImGuiWindowFlags_AlwaysVerticalScrollbar bitor ImGuiWindowFlags_NoCollapse);
		/*instruments*/
		End();


		End();
		/*this is where it ends :broken_heart:*/
		window.clear();
		SFML::Render(window);
		window.display();
	}
	SFML::Shutdown();
	return 0;
}