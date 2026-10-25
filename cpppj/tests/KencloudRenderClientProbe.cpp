// 같은 실제 Client/Screen 캐시의 전체·부분 표시를 모든 픽셀과 상태로 명시 검사한다.
#include "client/ClientMain.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRandomGlobals.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <stdexcept>

namespace { // API 장치 검사를 자연 사용자 플레이와 구별한다.
using namespace netstorm::client;
constexpr std::uint8_t kUntouched=42; // 복사하지 않은 실제 화면 픽셀의 명시 초기 입력이다.
void Require(bool value,const char* text) { if (!value) throw std::runtime_error(text); } // 실제 첫 차이를 정상 성공으로 숨기지 않는다.
void Fill(Screen& screen) { auto* pixels=screen.Lock();std::fill_n(pixels,std::size_t(screen.Pitch())*screen.Height(),kUntouched);screen.Unlock(); } // 같은 실제 DIB에만 초기 표면 입력을 쓴다.
std::uint64_t Pixels(Screen& screen) { auto* pixels=screen.Lock();std::uint64_t hash=14695981039346656037ULL;for (std::size_t index=0;index<std::size_t(screen.Pitch())*screen.Height();++index) { hash^=pixels[index];hash*=1099511628211ULL; }screen.Unlock();return hash; } // 실제 잠근 표면 전체를 식별한다.
bool Same(ScreenRect first,ScreenRect second) { return first.left==second.left && first.top==second.top && first.right==second.right && first.bottom==second.bottom; } // 네 exclusive clip 값을 비교한다.
void Verify(Client& client,const std::array<std::uint32_t,6>& area,bool partial,const std::array<std::uint32_t,512>& rows) {
    auto& screen=client.GetScreen();auto cloud=client.KencloudGlobals();const auto source=cloud->Memory(cloud->Globals().cache);const auto* pixels=screen.Lock();std::size_t different{};
    for (int y=0;y<screen.Height();++y) for (int x=0;x<screen.Width();++x) { // 원시 타일 함수 대신 독립적인 각 대상 픽셀의 source 좌표로 비교한다.
        const auto sx=(std::uint32_t(x)+area[4])%512U,sy=(std::uint32_t(y)+area[5])%512U;const auto inside=std::uint32_t(x)>=area[0] && std::uint32_t(x)<area[2] && std::uint32_t(y)>=area[1] && std::uint32_t(y)<area[3];const auto expected=inside && (!partial || rows[sy]) ? source[sy*512U+sx] : kUntouched;different+=pixels[std::size_t(y)*screen.Pitch()+x]!=expected;
    }
    screen.Unlock();Require(!different,"실제 구름 Screen 픽셀 불일치");
}
void Observe(std::ofstream& output,Client& client,const char* tag) {
    auto cloud=client.KencloudGlobals();const auto g=cloud->Globals();const auto state=cloud->State();output<<tag<<'\t'<<g.cache<<'\t'<<g.phase<<'\t'<<g.allDirty<<'\t'<<g.partialDirty<<'\t'<<std::count_if(g.dirtyRows.begin(),g.dirtyRows.end(),[](auto value) { return value!=0; })<<'\t'<<std::count_if(state.begin(),state.begin()+512,[](auto value) { return value!=0; })<<'\t'<<client.RandomGlobals()->Game().State()<<'\t'<<Pixels(client.GetScreen())<<'\t'<<client.GetScreen().Width()<<'\t'<<client.GetScreen().Height()<<'\n';output.flush(); // 중단 뒤에도 앞선 효과를 보존한다.
}
}

// 명시 장치 입력으로 같은 실제 생성/전체 표시/행 표시/정리/장치 교체를 실행한다.
int main(int argc,char** argv) {
    try {
        if (argc!=3) throw std::invalid_argument("사용법: netstorm_cloud_render_probe 게임폴더 출력폴더");const std::filesystem::path directory=argv[2];std::ofstream output(directory/"device.tsv",std::ios::binary);if (!output) throw std::runtime_error("구름 표시 보고서 열기 실패");
        ClientOptions options;options.gameDirectory=argv[1];options.forceWindow=true;options.noAudio=true;options.frameLimit=2;options.settings="SCREENW=640;SCREENH=480;maxFPS=40;sleepPerLoop=10;autoDemo=0;tellTips=0;cloudSpeed=2;cloudFury=10;CloudFileSpec=d/Gifcloud.gif";Client client(std::move(options));
        client.ready=[&](Client& active) {
            auto cloud=active.KencloudGlobals();auto& screen=active.GetScreen();cloud->Build(active.Files(),active.Configuration(),active.RandomGlobals()->Game());const auto rng=active.RandomGlobals()->Game().State(),cache=cloud->Globals().cache;Fill(screen);Observe(output,active,"built");
            const std::array<std::uint32_t,6> area{11,13,629,477,3,7};const ScreenRect previous{3,5,620,470};screen.SetClip(previous.left,previous.top,previous.right,previous.bottom);std::array<std::uint32_t,512> rows{};
            const auto draw=[&](const auto& region) { cloud->Draw(active.GetScreen(),region,active.Files(),active.Configuration(),active.RandomGlobals()->Game()); }; // 실제 같은 생존 cache를 같은 Screen에 전달한다.
            draw(area);Verify(active,area,false,rows);Require(Same(screen.Clip(),previous) && cloud->Globals().partialDirty==1,"실제 전체 표시/clip 복구 불일치");active.Capture(directory/"full.bmp");Observe(output,active,"full");cloud->CleanDirty();draw(area);cloud->CleanDirty();Require(std::none_of(cloud->Globals().dirtyRows.begin(),cloud->Globals().dirtyRows.end(),[](auto value) { return value!=0; }),"실제 이전 행 정리 불일치");Observe(output,active,"drained");
            cloud->Update(active.Files(),active.Configuration(),active.RandomGlobals()->Game());std::copy_n(cloud->Globals().dirtyRows.begin(),512,rows.begin());Fill(screen);draw(area);Verify(active,area,true,rows);Require(Same(screen.Clip(),previous),"실제 부분 표시 clip 복구 불일치");active.Capture(directory/"partial.bmp");Observe(output,active,"partial");cloud->CleanDirty();Require(std::none_of(cloud->Globals().dirtyRows.begin(),cloud->Globals().dirtyRows.end(),[](auto value) { return value!=0; }),"실제 부분 표시 행 정리 불일치");Observe(output,active,"cleaned");
            const auto pixels=Pixels(screen);draw(std::array<std::uint32_t,6>{0,0,65536,65536,0,0});Require(Pixels(screen)==pixels && Same(screen.Clip(),previous),"실제 곱0 자연 생략 불일치");Observe(output,active,"zero");
            bool unsupported=false;try { cloud->Draw(screen,{4,6,12,14,0,0},active.Files(),active.Configuration(),active.RandomGlobals()->Game(),true); }catch (const std::logic_error&) { unsupported=true; }Require(unsupported && Same(screen.Clip(),{4,6,12,14}) && Pixels(screen)==pixels,"별도 시차 잠금 경계/선행 clip 효과 불일치");screen.SetClip(previous.left,previous.top,previous.right,previous.bottom);Observe(output,active,"alternate-prefix");
            active.ChangeResolution(800,600);Require(cloud->Globals().cache==cache && active.RandomGlobals()->Game().State()==rng,"실제 표시/해상도 교체의 cache/난수 보존 불일치");const std::array<std::uint32_t,6> wide{17,19,799,599,511,512};cloud->MarkDirty();Fill(active.GetScreen());draw(wide);Verify(active,wide,false,rows);active.Capture(directory/"resized.bmp");Observe(output,active,"resized");cloud->CleanDirty();
            Require(active.RandomGlobals()->Game().State()==rng && cloud->Globals().cache==cache && cloud->Allocations()==1,"실제 표시가 별도 난수/캐시를 생성함");active.GetScreen().Update({0,0,800,600});
        };
        const auto code=client.Run();Require(code==0,"실제 구름 표시 Client 종료 실패");Observe(output,client,"finished");std::cout<<"실제 구름 표시 픽셀 검사 통과\n";return 0;
    }catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; } // 실패를 원시 종료 코드에 보존한다.
}
