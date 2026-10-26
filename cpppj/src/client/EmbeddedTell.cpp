// 내장 설명은 빈 섹션도 실제 HtmlBody를 생성하며 일반 팝업의 이력/파라미터를 건드리지 않는다.
#include "client/EmbeddedTell.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <new>
#include <stdexcept>

namespace netstorm::client { // 같은 Config/현재 미션을 빌리는 내장 설명 부모다.
namespace { // 실제 선택한 장치만 연결을 요구한다.
template<class T> const T& EmbeddedNeed(const T& value) { if (!value) throw std::logic_error("내장 TELL의 실제 선택된 장치 미연결");return value; } // 도달하지 않는 fallback/경고는 요구하지 않는다.
std::string EmbeddedDecode(std::string_view value) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 원문 좁은 키를 실제 UTF-8 Config 경계에서만 변환한다.
}
// 원문은 첫 점의 등록 객체/param 파싱/BLANK/팝업 생략을 검사하지 않는다.
std::uint32_t ShowEmbeddedTell(std::string_view name,const EmbeddedTellHooks& hooks) {
    name=name.substr(0,name.find('\0'));auto body=hooks.mission ? hooks.mission(name) : std::nullopt;
    if (!body) body=EmbeddedNeed(hooks.fallback)(name);
    if (!body) { EmbeddedNeed(hooks.missing)(name);return 0; }
    const auto token=EmbeddedNeed(hooks.construct)();if (!token) throw std::bad_alloc();
    EmbeddedNeed(hooks.setHtml)(token,*body);body.reset();return EmbeddedNeed(hooks.serial)(token);
}
// 실제 파일은 현재 missionSpec에서 tell 키로 매번 읽고 지역 Config를 먼저 소멸한다.
std::uint32_t ShowEmbeddedTellInConfig(std::string_view name,o::ConfigInterface& config,
    std::function<std::optional<std::string>(std::string_view)> mission,EmbeddedTellHooks hooks) {
    hooks.mission=mission ? std::function<std::optional<std::string>(std::string_view)>([mission=std::move(mission)](auto key) { return mission(EmbeddedDecode(key)); }) : decltype(hooks.mission){};
    hooks.fallback=[&config](auto key)->std::optional<std::string> {
        o::Config temporary(config.Registry(),"tell");const auto path=config.PathSpec("missionSpec","tell");if (config.Reader()) if (const auto bytes=config.Reader()(path)) temporary.LoadBytes(*bytes);
        const auto section=temporary.Section(EmbeddedDecode(key));return section ? std::optional<std::string>(std::string(*section)) : std::nullopt;
    };
    return ShowEmbeddedTell(name,hooks);
}
}
