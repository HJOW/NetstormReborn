// 원본 창 재사용 관문과 현재 이력/팝업/버튼을 읽는 순서를 그대로 유지한다.
#include "client/QuickHelpControl.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kHelp=0x02000017,kRequest=0x02000057,kLink=0x02000025,kButton=0x0200002c; // 실제 PE의 Help/요청/링크/버튼 명령이다.
constexpr std::uint32_t kCommandMask=0x03ffffff,kRelease=0x80000000,kLeft=0x10000,kRight=0x20000; // 원본 사건 종류와 키 해제 비트다.
// 실행한 분기의 실제 경계가 없으면 미연결 상태를 명시한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("Quick Help 제어 실제 경계 누락");return value; }
// C 문자열의 첫NUL 뒤를 요청/이력으로 해석하지 않는다.
std::string_view CText(std::string_view text) { return text.substr(0,text.find('\0')); }
// 실제 기본/자식 저장소가 없는 토큰은 원본 포인터 접근의 호스트 계약 위반이다.
GumpCreateNode& Node(const std::function<GumpCreateNode*(std::uint32_t)>& lookup,std::uint32_t token) {
    auto* result=Need(lookup)(token);if (!result) throw std::out_of_range("Quick Help 현재 Gump 토큰 누락");return *result;
}
// 원본 _ftol의 signed64 절삭 후 하위 DWORD다. NaN/범위 밖 indefinite의 하위 값은0이다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// null은 전체 버퍼를 지우지만 비null 빈 문자열은 첫 바이트만 지운다.
void QuickHelpPopupState::Mark(std::uint32_t code,std::optional<std::string_view> text) {
    if (text && CText(*text).size()>=argument.size()) throw std::length_error("Quick Help 팝업 인자80바이트 초과");
    command=code;if (!text) { argument.fill(0);return; }
    const auto source=CText(*text);std::copy(source.begin(),source.end(),argument.begin());argument[source.size()]=0;
}
// 공용 저장소의 현재 문자열을 읽으며 이전 길이/복사본을 캐시하지 않는다.
std::string_view QuickHelpPopupState::Argument() const {
    const auto end=std::find(argument.begin(),argument.end(),0);if (end==argument.end()) throw std::length_error("Quick Help 팝업 인자의 NUL 누락");
    return {argument.data(),static_cast<std::size_t>(end-argument.begin())};
}
// 원본 CRT는 ASCII C locale에서 현재 머리하고만 대소문자를 무시해 비교한다.
void QuickHelpHistory::Push(std::string_view topic) {
    topic=CText(topic);if (!topics_.empty() && o::AsciiLower(topics_.back())==o::AsciiLower(topic)) return;topics_.emplace_back(topic);
}
// 마지막 노드를 제거한 뒤 다음 노드가 없을 때만 null을 반환한다.
std::optional<std::string> QuickHelpHistory::Back() {
    if (topics_.empty()) return std::nullopt;topics_.pop_back();if (topics_.empty()) return std::nullopt;return topics_.back();
}
// 읽기 전용 span은 다음 Push/Back 전에만 유효하다.
std::span<const std::string> QuickHelpHistory::Topics() const { return topics_; }
// 팝업 닫기와 두 등록 조회는 모달 관문보다 먼저 일어난다.
std::uint32_t OpenQuickHelpWindow(const QuickHelpOpenState& state,const QuickHelpOpenHooks& hooks,
    std::uint32_t topic,std::int32_t mode,std::uint32_t learnType) {
    Need(hooks.closePopups)();auto help=Need(hooks.lookup)(state.helpId);const auto alternate=Need(hooks.lookup)(state.alternateId);
    if (Need(hooks.noModal)() || Need(hooks.currentModal)()==help || Need(hooks.currentModal)()==alternate) {
        if (!help) {
            help=Need(hooks.construct)(learnType);if (!help) throw std::bad_alloc();
            const auto& node=Node(hooks.node,help);const float x=static_cast<float>((static_cast<double>(state.areaWidth)-node.baseWidth)*0.5+state.areaLeft);
            Need(hooks.attach)(help,x,40.0f,state.root,0);Need(hooks.addDepth)(help,Node(hooks.node,help).depth-76U);
        }
        Need(hooks.modal)(help,1);Need(hooks.handle)(help,mode==0 ? kRequest : kHelp,0,topic);
    }
    return help;
}
// 마우스 좌표 변환·바깥 판정·일반 키·명령 순서로 원본의 분기를 유지한다.
std::uint32_t DispatchQuickHelpWindow(QuickHelpWindowNode& window,InputEvent event,QuickHelpHistory& history,
    QuickHelpPopupState& popup,const QuickHelpEventHooks& hooks) {
    const auto point=Need(hooks.local)(event);const auto code=event.code&kCommandMask;const bool down=(event.code&kRelease)==0;
    const auto first=std::bit_cast<std::uint32_t>(event.x),second=std::bit_cast<std::uint32_t>(event.y); // 좌표 칸에 저장된 원본 인자 DWORD다.
    const auto close=[&]() { Need(hooks.kill)();return 1U; }; // 원본 지연 삭제 뒤에는 window 필드를 다시 읽지 않는다.
    if ((!(point[0]>=0.0f) || point[0]>=window.width || !(point[1]>=0.0f) || point[1]>=window.height) && down && code==kLeft) return close();
    if (!(event.code&0x02000000U) && code!=kLeft && code!=kRight) {
        if (down) {
            if (code==0x210000U || code==0x220000U) { const float height=Need(hooks.pageHeight)();Need(hooks.scroll)(Truncate(code==0x210000U ? -height : height));return 1; }
            if (code==0x260000U || code==0x280000U) { Need(hooks.scroll)(code==0x260000U ? -10 : 10);return 1; }
        }
        return Need(hooks.base)(event);
    }
    if (code==kHelp || code==kRequest) {
        if (event.y<256 && second) {
            auto topic=Need(hooks.typeName)(second);topic=std::string(CText(topic));topic+=CText(Need(hooks.translate)("Type"));
            if (topic.size()>=80) throw std::length_error("Quick Help TYPE 주제80바이트 초과");Need(hooks.request)(second,topic);
        } else { const auto topic=Need(hooks.text)(second ? second : first);Need(hooks.request)(0,CText(topic)); }
        return 1;
    }
    if (code==kLink) { if (second) { const auto topic=Need(hooks.text)(second);Need(hooks.link)(CText(topic)); }return 1; }
    if (code==kButton) {
        const auto matches=[&](std::uint32_t token) { return token && Node(hooks.node,token).serial==first; }; // 등록 ID가 아닌 원본 자식+2c와 비교한다.
        if (matches(window.cancelButton)) return close();
        if (matches(window.learnButton)) { if (window.learnType) Need(hooks.learn)(window.learnType);return close(); }
        if (matches(window.backButton)) {
            if (!history.Topics().empty()) {
                const auto topic=history.Back();const auto previous=popup.command;
                if (topic) { Need(hooks.link)(*topic);return 1; }
                Need(hooks.kill)();if (previous) { const auto argument=popup.Argument();Need(hooks.queue)(previous,argument,argument); }return 1;
            }
            return close();
        }
    }
    return Need(hooks.base)(event);
}
}
