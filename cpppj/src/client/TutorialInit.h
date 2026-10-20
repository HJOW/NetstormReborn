// Tutorial Init의 실제 설정/이름·전역 쓰기/로드 요청·Header 순서를 공통 상태에 연결한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 한 미션의 원본 이름과 Init이 남긴 첫DWORD·지역 문자열을 보유한다.
struct TutorialInitState {
    std::string name; // 원본 객체+4의 이름이다. 정규화는 이 원문을 바꾸지 않고 지역 사본에서만 수행한다.
    std::uint32_t namePrefix{}; // +b0의 정규화한 첫DWORD다. 짧은 입력의 NUL 뒤는 제공한 지역 입력을 보존한다.
    std::array<char,80> scratch{}; // 원본 local_150의80BYTE이며 caller가 미사용 꼬리의 입력을 제공할 수 있다.
};
// Init이 읽고 쓰는 현재 전역의 참조를 제공하며 별도 사본을 만들지 않는다.
struct TutorialInitGlobals {
    std::uint32_t& tutorialNumber;std::uint32_t& missionNumber;std::uint32_t& tutorialSeen; // 5ca8e4/e0/f0의 실제 현재 전역이다.
    std::uint32_t& loginSubscriber;const std::uint32_t& initialSubscriber; // 540c74에54265c를 현재 시점에서 복사한다.
};
// 원본 Init에서 도달한 큰 효과를 현재 Client의 같은 저장소와 전환 큐에 연결한다.
struct TutorialInitHooks {
    std::function<void()> clearTemplates,resetOptions,header; // 실제 템플릿 정리/같은87BYTE/같은 기술 허용 표다.
    std::function<std::string(std::string_view,std::optional<std::string_view>)> get; // null은482c90,그 외는482ce0의 빈 값 fallback이다.
    std::function<void(std::string_view,std::string_view)> set; // 같은 미션 Config의 fileName 쓰기다.
    std::function<void(std::string_view)> requestFort; // 복사한256BYTE 안의 이름으로 다음 상태의 실제 로드를 요청한다.
};
// 004c3290 전체 제어다. 큰 템플릿/Config/로드/기술 Header 몸체는 같은 현재 Client 경계로 연결한다.
void InitializeTutorialMission(TutorialInitState& state,std::uint32_t& flags,std::uint32_t& demo,TutorialInitGlobals globals,const TutorialInitHooks& hooks);
}
