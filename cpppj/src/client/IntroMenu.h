// 원본 Intro 존재 감지와 PlayIntro 일반 입력을 실제 Native 영상 상태에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include "o/RawLocalMissionState.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { // 메인 메뉴 표시와 일반 입력이 같은 감지/요청 부모를 사용한다.
inline constexpr std::uint32_t kPlayIntroCommand=0x020000d8U; //00543248의 PlayIntro→00542a54 초기 명령 DWORD다.
std::uint32_t DetectIntroMovie(std::uint32_t& available,const std::function<std::string()>& path,const std::function<std::uint32_t(std::string_view)>& probe); //00435de0: 0쓰기→현재 introMovie→비어 있지 않으면 존재 조회→비영이면1쓰기다.
bool DispatchPlayIntroInput(const InputEvent& event,o::LocalMissionStateStorage state); //004d8xxx→004b3520: release는 소비만 하고 나머지는 next39/next+8c=1을 쓴다.
}
