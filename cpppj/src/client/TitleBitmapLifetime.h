// 원문4ab3e0/4ab370/4ab2d0의 같은8칸bitmap·현재번호·paint깊이를 빌린다.
#pragma once
#include "client/Screen.h"
#include <functional>
#include <exception>
#include <span>
#include <string_view>

namespace netstorm::client { // 시작 title과 일반 BackgroundGif의 소유 수명을 구별한다.
class TitleBitmapQuit final:public std::exception { //435b30의WM_QUIT/_exit를host Run의정상반환으로연결하는제어예외다.
public:
    const char* what() const noexcept override { return "Title bitmap WM_QUIT"; } //게임/파일오류와구별하고State41정상후반을합성하지않는다.
};
struct TitleBitmapGlobals { // 전역 사본 없이 Client와 현재 FrameTime을 빌린다.
    std::span<std::uint32_t,8> bitmaps; //005c7c24..40의 원문8칸이다. free 뒤0을 쓰지 않는다.
    std::int32_t& current; //005c7c20의 현재 선택이며 실행 첫 단계에서-1이다.
    std::uint32_t& paintDepth; //0054dc54의 WM_PAINT 선택 깊이다.
    const std::int32_t& width;const std::int32_t& height; //현재 원문 SCREENW/H이며 bitmap의 치수와 다르다.
    double& gameSeconds; //0055b4d0의 같은 실제 프레임 게임 시각이다.
};
struct TitleBitmapCopy { //4a1fe0/4a19d0에 전달하는 현재 raw 복사 인자다.
    std::int32_t x{},y{};std::uint32_t bitmap{};std::int32_t stride{};ScreenRect source{}; //호스트 이미지의 고유 치수로 교정하지 않는다.
};
struct TitleBitmapHooks { // 원문 큰 부모가 도달한 장치만 호출한다.
    std::function<void()> pump,lock,unlock,present,flip,frontLock,frontUnlock; //실제435b30/4a14e0/1550/46d0/16d0/17c0/17e0 경계다.
    std::function<std::uint32_t()> input; //452f00(local,1)의 현재 code만 반환하며 뗌은 기존 큐가 버린다.
    std::function<double()> game,wall; //460cd0과460d70의 서로 다른 매호출 시각이다.
    std::function<std::uint32_t(std::string_view,std::int32_t&,std::int32_t&)> load; //이름은4ab3e0 literal이며 같은공유출력치수를 쓴다.
    std::function<void(std::string_view)> palette; //title00/06.col의 clear1이며 서식/현재파일정책은 host가 연결한다.
    std::function<void(const TitleBitmapCopy&)> copy,frontCopy; //현재bitmap은 lock 뒤 다시 읽는다.
    std::function<void(ScreenRect,std::uint8_t)> fill; //검정 literal0이며 현재색표 검정으로 바꾸지 않는다.
    std::function<void(std::uint32_t)> release; //8칸 현재값을 앞에서부터 free한다.
    std::function<void(ScreenRect)> clip; //실행 앞의 실제 full viewport clip이다.
};
bool WaitTitleBitmap(std::int32_t milliseconds,const TitleBitmapHooks& hooks); //4ab370의 deadline 확인→pump→Pop이며 input시true다.
void RunTitleBitmaps(TitleBitmapGlobals globals,const TitleBitmapHooks& hooks); //실제큰부모이며예외를정상cleanup으로바꾸지않는다.
void RedrawTitleBitmap(TitleBitmapGlobals globals,const TitleBitmapHooks& hooks); //WM_PAINT의4ab2d0.현재-1은검정출력,그외전체SCREEN stride다.
void RequestInitialTitleBitmap(std::span<std::uint32_t,36> next); //4b3500의 next42/next+8c0만쓴다.
}
