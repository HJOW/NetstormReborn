// 실제 Client/Win32 Screen 장치에서 전체 팔레트와 장기 구름의 수명을 검사하는 명시 GUI 실행 도구다.
#include "client/ClientMain.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { // 동결한 실제 제품 API와 현재 장치만 관측하며 자연 플레이 증거로 표시하지 않는다.
using namespace netstorm::client;
void Require(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); } // 실제 경계 불일치를 종료1로 남긴다.
// 실제 Screen의256색 RGB를 읽기만 하며 장치 변경 전후의 같은 값을 비교한다.
std::uint64_t PaletteRgb(Client& client) {
    auto& screen=client.GetScreen();std::uint64_t hash=1469598103934665603ULL;
    for (const auto& color:screen.Palette()) { for (const auto value:{color.blue,color.green,color.red}) { hash^=value;hash*=1099511628211ULL; } } // 실제 현재256색의 RGB 전체를 원시 순서로 식별한다.
    return hash;
}
// 같은 호출 직후의 캐시/난수/장치와 실제 팔레트를 TSV에 즉시 보존한다.
void Observe(std::ofstream& report,Client& client,const char* tag) {
    auto cloud=client.KencloudGlobals();const auto globals=cloud->Globals();report<<tag<<'\t'<<globals.cache<<'\t'<<globals.end<<'\t'<<globals.phase<<'\t'<<cloud->Allocations()<<'\t'<<cloud->RendererToken()<<'\t'<<cloud->WorldRedraw()<<'\t'<<client.RandomGlobals()->Game().State()<<'\t'<<client.GetScreen().Width()<<'\t'<<client.GetScreen().Height()<<'\t'<<client.GetScreen().PaletteLoaded()<<'\t'<<PaletteRgb(client)<<'\n';report.flush(); // 앞선 효과를 오류 뒤에도 보존한다.
}
}

// 명시한 실제 Client 장치 순서를 실행하고 실패를 종료 코드와 원시 로그에 남긴다.
int main(int argc,char** argv) {
    try {
        if (argc!=3) throw std::invalid_argument("사용법: netstorm_cloud_probe 게임폴더 보고서.tsv");std::ofstream report(argv[2],std::ios::binary);if (!report) throw std::runtime_error("구름 실제 장치 보고서 열기 실패");
        ClientOptions options;options.gameDirectory=argv[1];options.forceWindow=true;options.noAudio=true;options.frameLimit=2;options.settings="SCREENW=640;SCREENH=480;maxFPS=40;sleepPerLoop=10;autoDemo=0;tellTips=0;currentLanguage=english;cloudSpeed=2;cloudFury=10";Client client(std::move(options));
        client.ready=[&](Client& active) {
            Require(active.SpGlobals()->Store().Initialized(),"실제 공통 SP 시작 누락");auto cloud=active.KencloudGlobals();Observe(report,active,"ready");Require(!cloud->Globals().cache && cloud->RendererToken(),"실제 최초 구름/Renderer 상태 불일치");
            const auto change=[&](const std::string& value) { const std::span<const std::uint8_t> bytes{reinterpret_cast<const std::uint8_t*>(value.c_str()),value.size()+1};active.ChangeFortPalette(bytes); }; // 같은 실제 NUL 입력으로 원문 부모를 호출한다.
            change("Gifcloud");Observe(report,active,"first");const auto cache=cloud->Globals().cache,rng=active.RandomGlobals()->Game().State(),phase=cloud->Globals().phase;const auto palette=PaletteRgb(active);Require(cache && cloud->Allocations()==1 && phase==2 && cloud->WorldRedraw()==1,"실제 첫 팔레트/구름 전환 불일치");
            change("GIFCLOUD");Observe(report,active,"same");Require(cloud->Globals().cache==cache && active.RandomGlobals()->Game().State()==rng && cloud->Globals().phase==phase && PaletteRgb(active)==palette,"같은 이름 자연 반환 불일치");
            change("BULF");Observe(report,active,"second");Require(cloud->Globals().cache!=cache && cloud->Allocations()==1 && cloud->Globals().phase==4,"실제 다른 팔레트 반납/재생성 불일치");const auto retained=cloud->Globals().cache,current=active.RandomGlobals()->Game().State(),renderer=cloud->RendererToken();
            active.ChangeResolution(800,600);Observe(report,active,"resolution");Require(cloud->Globals().cache==retained && active.RandomGlobals()->Game().State()==current && cloud->RendererToken()!=renderer,"해상도 재생성의 같은 구름/난수 수명 불일치");
            change("Gifcloud");Observe(report,active,"third");Require(cloud->Allocations()==1 && cloud->Globals().cache!=retained && cloud->Globals().phase==6 && active.GetScreen().PaletteLoaded()==1,"새 Screen의 실제 전체 팔레트 전환 불일치");active.ShowScene();
        };
        const auto code=client.Run();Require(code==0,"실제 Client 종료 코드 불일치");Observe(report,client,"finished");std::cout<<"실제 구름/Screen 팔레트 검사 통과\n";return 0;
    }catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; } // 실제 실패를 빈 성공으로 바꾸지 않는다.
}
