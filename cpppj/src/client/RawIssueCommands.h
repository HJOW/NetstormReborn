// 원문004d53a0의 초기 등록과 flash/desc/cheatorama 콜백을 실제 Client 장치에서 분리한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace netstorm::client { // 같은 명령표와 공유 치트 DWORD를 사용하는 초기 명령 계층이다.
// 원문이 선택한 효과만 실제 호출 위치에서 요구한다.
struct IssueCommandHooks {
    std::function<void(std::int32_t)> flash; //00470fa0에 원문 atol 밝기 또는0을 전달한다.
    std::function<void(std::string_view)> sound; //번개 생성 뒤 thunderQuietDistant.wav를 같은 음향 장치로 요청한다.
    std::function<void(std::int32_t,std::optional<std::string_view>,std::uint32_t)> description; //0042c040(-1,arg,0)의 디렉터리 설명 요청이다. MP 후순위 장치와 구별한다.
};
// 이름/함수/표시 여부의 원문 순서를 유지하며 중복 flash 두 번을 제거하지 않는다.
void RegisterIssueCommands(const std::function<void(std::string_view,std::uint32_t,std::uint32_t)>& registration);
// 알려진 세 콜백은 원문 결과0을 반환하고 다른 콜백은 nullopt로 다음 분배기에 넘긴다.
std::optional<std::uint32_t> InvokeIssueCommand(std::uint32_t callback,std::optional<std::string_view> argument,std::uint32_t& cheat,const IssueCommandHooks& hooks);
}
