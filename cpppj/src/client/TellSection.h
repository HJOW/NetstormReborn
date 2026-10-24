// 원본4cf730의 파라미터 처리 뒤 현재 이름 기록과 세 단계 섹션 조회를 복원한다.
#pragma once
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::o { class ConfigInterface; } // 같은 장기 설정 등록부를 빌린다.
namespace netstorm::client { // Tell의 현재 이름과 원문 조회 순서를 공유한다.
struct TellSectionState { //005cab40의 첫 NUL 문자열을 같은 Client 수명에 보관한다.
    std::array<char,256> current{}; // 실패한 조회도 새 이름을 기록하며 NUL 뒤 바이트를 지우지 않는다.
};
struct TellSectionHooks { // 각 조회는 없는 섹션과 존재하는 빈 본문을 구별한다.
    std::function<void(std::string_view)> prior; // 이름이 대소문자 무시 비교에서 달라질 때만 global.priorTell을 쓴다.
    std::function<std::optional<std::string>(std::string_view,std::string_view)> named; // 등록된 접두 객체의 섹션만 읽으며 파일을 새로 적재하지 않는다.
    std::function<std::optional<std::string>(std::string_view)> mission,fallback; // 현재 미션과 매번 새로 적재하는 tell 설정을 순서대로 조회한다.
};
std::optional<std::string> FindTellSection(TellSectionState& state,std::string_view name,const TellSectionHooks& hooks); // 파라미터를 제거한 좁은 이름을 받아 원문 첫 점/전체 이름 조회와 이전 이름을 유지한다.
std::optional<std::string> FindTellSectionInConfig(TellSectionState& state,std::string_view name,o::ConfigInterface& config,
    std::function<std::optional<std::string>(std::string_view)> mission); // 실제 등록부/Config/파일 읽기를 연결하고 fallback 임시 객체를 반환 전에 반납한다.
}
