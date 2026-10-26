// 원본004c8f60의 내장 설명 조회와 HtmlBody 생성/serial 반환을 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::o { class ConfigInterface; } // 현재 등록부/파일 조회의 실제 소유자다.
namespace netstorm::client { // 일반004cf730 팝업과 별개의 내장 HtmlBody 경로다.
// 원문 현재 Mission→매번 새 tell 설정→HtmlBody 생성 순서의 선택된 실제 장치다.
struct EmbeddedTellHooks {
    std::function<std::optional<std::string>(std::string_view)> mission,fallback; // 없는 미션과 존재하는 빈 섹션을 구별한다.
    std::function<void(std::string_view)> missing; // 두 조회 실패 때만 원문 경고를 보낸다.
    std::function<std::uint32_t()> construct; // 실제 new0x40a4/0046c9c0의 토큰이다.
    std::function<void(std::uint32_t,std::string_view)> setHtml; // 실제0046cc60의 기본 HTML 파서/다중 줄 자식 설치다.
    std::function<std::uint32_t(std::uint32_t)> serial; // 본문 소유권 반납 뒤 현재 부모+2c를 다시 읽는다.
};
std::uint32_t ShowEmbeddedTell(std::string_view name,const EmbeddedTellHooks& hooks); // 이름/param/현재 Tell 기록을 바꾸지 않고 원문 serial 또는 조회 실패0을 반환한다.
std::uint32_t ShowEmbeddedTellInConfig(std::string_view name,o::ConfigInterface& config,
    std::function<std::optional<std::string>(std::string_view)> mission,EmbeddedTellHooks hooks); // fallback 임시 Config를 UI 생성 전에 반납하며 등록된 tell 캐시를 재사용하지 않는다.
}
