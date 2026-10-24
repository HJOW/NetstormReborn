// 이름 기록은 섹션 성공 여부보다 먼저 수행하며 실제 조회의 빈 본문을 보존한다.
#include "client/TellSection.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { //4cf730의 현재 이름/조회 부분 본체와 실제 Config 연결이다.
namespace { // 원문 C문자열과 선택된 외부 경계만 확인한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Tell 섹션의 선택된 실제 조회 미연결");return hook; } // 선택하지 않은 조회는 요구하지 않는다.
std::string Decode(std::string_view bytes) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); } // 설정의 UTF-8 경계에서만 원본 바이트를 변환한다.
}
std::optional<std::string> FindTellSection(TellSectionState& state,std::string_view name,const TellSectionHooks& hooks) {
    const std::string local(name.substr(0,name.find('\0'))); // 콜백이 원래 입력 저장소를 바꿔도 원문 지역256BYTE 복사본은 유지한다.
    if (local.size()>=state.current.size()) throw std::length_error("Tell 이름의 원본256BYTE 지역 범위 초과");
    const auto end=std::find(state.current.begin(),state.current.end(),'\0');if (end==state.current.end()) throw std::length_error("Tell 현재 이름의 원본 NUL 범위 초과");
    const std::string previous(state.current.begin(),end);if (o::AsciiLower(previous)!=o::AsciiLower(local)) Need(hooks.prior)(previous);
    std::copy(local.begin(),local.end(),state.current.begin());state.current[local.size()]=0;
    const auto dot=local.find('.');if (dot!=local.npos) if (auto body=Need(hooks.named)(std::string_view(local).substr(0,dot),std::string_view(local).substr(dot+1))) return body;
    if (auto body=Need(hooks.mission)(local)) return body;
    return Need(hooks.fallback)(local);
}
std::optional<std::string> FindTellSectionInConfig(TellSectionState& state,std::string_view name,o::ConfigInterface& config,
    std::function<std::optional<std::string>(std::string_view)> mission) {
    TellSectionHooks hooks;hooks.prior=[&](auto previous) { config.Set("global.priorTell",Decode(previous)); }; // 실제 global 객체에 기존 원문 이름을 기록한다.
    hooks.named=[&](auto prefix,auto section)->std::optional<std::string> { // 등록된 객체만 조회하며 없는 접두어의 파일 오류를 새로 만들지 않는다.
        const auto* object=config.Registry().FindByName(Decode(prefix));if (!object) return std::nullopt;const auto body=object->Section(Decode(section));return body ? std::optional<std::string>(std::string(*body)) : std::nullopt;
    };
    hooks.mission=[&](auto target)->std::optional<std::string> { return mission ? mission(Decode(target)) : std::nullopt; }; // 현재 미션 없음은 원문 NULL 관문이다.
    hooks.fallback=[&](auto target)->std::optional<std::string> { // 같은 이름의 기존 tell 객체와 구별되는 새 임시 설정을 적재한다.
        o::Config temporary(config.Registry(),"tell");const auto path=config.PathSpec("missionSpec","tell");if (config.Reader()) if (const auto bytes=config.Reader()(path)) temporary.LoadBytes(*bytes);
        const auto body=temporary.Section(Decode(target));return body ? std::optional<std::string>(std::string(*body)) : std::nullopt;
    };
    return FindTellSection(state,name,hooks);
}
}
