// 원본 Moviegump.cpp의 옵션·입력·팔레트·idle 순서를 실제 영상 장치와 분리한다.
#pragma once
#include "client/Screen.h"
#include <functional>
#include <optional>

namespace netstorm::client { // 원본 10.78 MovieGump의 장치 독립 재생 경계다.
struct MoviePlaybackState { // 같은 Gump가 소유하며 callback 뒤에도 현재 필드를 다시 읽는다.
    std::uint32_t autoKill{},freeze{},palettePending{}; // +a4/+a8/+ac: 종료시 삭제·게임 정지·새 팔레트 대기다.
    std::array<ScreenColor,256> palette{}; // +b8: Smacker RGB를 BGR0으로 바꾼 현재 영상 팔레트다.
    ScreenRect image{}; // +4b8: Gump 내부 원시 영상 사각형이다.
};
struct MoviePlaybackHooks { // 현재 디코더와 같은 공통 Gump/Renderer를 매 호출에 빌린다.
    std::function<bool()> atEnd,wait,paletteChanged; // 원문 가상54/SmackWait/SMK+68의 현재값이다.
    std::function<std::span<const std::uint8_t,768>()> palette; // 현재 Smacker RGB256색을 빌린다.
    std::function<void()> doFrame,nextFrame,end,kill; // 압축 해제·다음 프레임 준비·가상 종료·공통 지연 삭제다.
    std::function<void()> paletteLog; // 팔레트 복사 뒤 pending 쓰기 전의 원문 진단 로그다.
    std::function<ScreenPoint()> origin; //00464740의 현재 부모 누적 위치다.
    std::function<std::optional<ScreenRect>()> nextRect; //SmackToBufferRect의 다음 x/y/폭/높이를 exclusive 사각형으로 준다.
    std::function<void(ScreenRect,std::uint32_t)> invalidate; //00499370의 현재 원시 사각형과 flags4다.
    std::function<void(std::uint32_t)> invalidOption; // 원문은 잘못된 옵션을 로그만 남기고 상태를 유지한다.
};
//00483970: 옵션1/2는 입력이0인지에 따라 같은 DWORD를0/1로 쓴다.
void SetMoviePlaybackOption(MoviePlaybackState& state,std::uint32_t option,std::uint32_t value,const MoviePlaybackHooks& hooks);
//004839c0: ESC/Space는 누름만, 좌/우 버튼은 원문 마스크 조건대로 종료한다. 다른 입력은0이다.
std::uint32_t HandleMoviePlayback(MoviePlaybackState& state,std::uint32_t event,const MoviePlaybackHooks& hooks);
//00483a30: 실제 현재 RGB256색을 BGR0으로 바꾸고 팔레트 대기를1로 쓴다.
void UpdateMoviePlaybackPalette(MoviePlaybackState& state,std::span<const std::uint8_t,768> rgb,const MoviePlaybackHooks& hooks);
//00483a90: 종료/대기·팔레트·해제·dirty 목록·다음 준비의 순서를 보존한다. 시계와 디코더 속도는 장치가 담당한다.
void IdleMoviePlayback(MoviePlaybackState& state,std::uint32_t& dirtyOverride,const MoviePlaybackHooks& hooks);
}
