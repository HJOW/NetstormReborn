// 원문 SetLanguage의 현재 번호 비교→설정→LanguageChanged 큐→현재 카운터 증가다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { //HTML·시나리오와 현재 Client가 같은 언어 요청을 공유한다.
inline constexpr std::uint32_t kSetLanguageCommand=0x020000a5U,kLanguageChangedCommand=0x020000a6U; //보호542988/54298c의 이름표 초기 명령이다.
struct LanguageRequestHooks { //선택된 요청의 실제 설정/현재 사건/입력 큐 경계다.
    std::function<void(std::string_view,std::string_view)> set; //동기 Config 변경이 현재 언어 번호/용어표를 실제 갱신한다.
    std::function<std::uint32_t()> changed; //설정 callback 복귀 뒤 현재 LanguageChanged를 읽는다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; //원문452fe0이며 직접 GUI 호출로 대체하지 않는다.
};
void RequestOriginalLanguage(std::uint32_t requested,const std::int32_t& current,std::uint32_t& refresh,const LanguageRequestHooks& hooks); //004dea70의 앞선 부분효과와DWORD wrap을 유지한다.
bool DispatchSetLanguage(const InputEvent& event,const std::function<void(std::uint32_t)>& request); //일반 입력의 release 관문 뒤 두 번째 DWORD를 전달한다.
bool DispatchLanguageChanged(const InputEvent& event,bool alreadyRouted,const std::function<void(const InputEvent&)>& route); //원문 Gump 경로의 알림을 중복 없이 같은 실제 트리에 전달한다.
}
