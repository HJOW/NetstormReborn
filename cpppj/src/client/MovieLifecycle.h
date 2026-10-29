// 원본 Moviegump.cpp의 시작/종료와 원시 영상 Draw를 같은 실제 장치 경계에 연결한다.
#pragma once
#include "client/MoviePlayback.h"

namespace netstorm::client { // SDK/Screen/Gump의 구체적인 소유권은 실제 조립 계층에 남긴다.
struct MovieLifecycleState { // 같은 MovieGump의 재생 판단과 원래 화면 팔레트를 함께 보관한다.
    MoviePlaybackState playback; // 현재 옵션/새 팔레트/원시 영상 사각형이다.
    std::array<ScreenColor,256> savedPalette{}; // +b4가 가리키는 시작 전0059afe0의 BGR0 팔레트1024BYTE다.
};
struct MovieLifecycleHooks { // 선택된 함수의 실제 현재 장치만 조회하며 빈 성공을 만들지 않는다.
    std::function<bool()> isOpen,hasBuffer,soundEnabled; // 현재 스트림/원시 버퍼와 원문0054daa0이다.
    std::function<void(std::uint32_t)> assertion; // 원문 assert의 줄 번호를 보고하고 복귀할 수 있다.
    std::function<void(std::span<ScreenColor,256>)> capturePalette; //004a2800의 현재0059afe0 BGR0 팔레트를 그대로 저장한다.
    std::function<void()> shutdownSound,muteSound,bindBuffer,unmuteSound,refreshCursor; // 원문 Sound/Smacker/커서의 순서 있는 효과다.
    std::function<ScreenPoint()> videoSize; // 현재 SMK의 unsigned 폭/높이 비트를 int32로 빌린다.
    std::function<std::array<float,2>()> windowSize; // 현재 Gump의 배율 적용 width/height다.
    std::function<void(bool)> idle,pauseUpdates,pauseGame,showCursor,redraw; // 원문 idle/두 정지 깊이/커서/가상 재그리기다.
    std::function<ScreenRect()> bounds,clip; // 현재 Gump의 절삭 경계와 Screen의 exclusive clip이다.
    std::function<void(ScreenRect,std::uint32_t)> clear,fill; // 종료의 잠금Clear와 Draw 내부Fill을 구별한다.
    std::function<void(std::span<ScreenColor,256>)> applyPalette; // 현재 팔레트를 즉시 적용한다. 원문004a2640의0/255번 입력BYTE 수정도 같은 배열에 남긴다.
    std::function<void(std::uint32_t)> restoreSound; // 소리 옵션이 비영일 때만 원문 음질3으로 장치를 다시 연다.
    std::function<std::uint32_t()> black; // 원문005a4058의 현재 검정색 번호다.
    std::function<void(ScreenPoint,ScreenPoint)> blit; // 현재 버퍼를 원래 폭/높이로 복사한다. 확대/투명 합성을 하지 않는다.
};
//00483680: 팔레트 보존→기존 음향 종료/음소거→버퍼 연결→중앙 사각형→idle/두 정지/커서 순서다.
void BeginMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& hooks);
//004837f0: 화면/팔레트/음향/idle/정지/커서를 원문 순서로 복원한다. Kill/Close는 별도 수명 경계다.
void EndMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& hooks);
//00483bf0: 현재 clip의 영상 바깥을 채우고 pending 팔레트를 적용한 뒤 원시 인덱스를 중앙에 복사한다.
void DrawMovieLifecycle(MovieLifecycleState& state,const MovieLifecycleHooks& hooks);
}
