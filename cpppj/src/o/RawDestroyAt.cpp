//0044c540/0044c790의 출력·필터·늦은 읽기와 원문 점 검색을 유지한다.
#include "o/RawDestroyAt.h"
#include "o/RawTextToken.h"
#include "o/ConfigInterface.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 같은 RawSquidFinder/월드 삭제에 조립하는 원문 부모다.
namespace { // 실제 선택된 장치와 원문 상수를 지역화한다.
constexpr std::uint32_t kDestroyGenus=0x7c77ef00U,kDestroyFlags=0x400000U,kBossAssertion=839; // 원문44c8a1/44c8b6/44c79f의 비교/호출 값이다.
// 호출되지 않는 하위는 요구하지 않고 선택된 효과의 누락만 실패한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("DestroyAt의 실제 장치 미연결");return function; }
// 원문의 int→float32→ftol→float32→ftol 저장을 DWORD 감김과 함께 적용한다.
std::int32_t PointCoordinate(std::int32_t value) {
    for (unsigned pass=0;pass<2;++pass) value=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(static_cast<float>(value)))); // 두 float 저장 사이의 signed DWORD 값을 보존한다.
    return value;
}
}
// 괄호는 문자열 첫 바이트에 있을 때만 건너뛰며 앞 공백을 새로 제거하지 않는다.
DestroyAtLocation ParseDestroyAtLocation(std::string_view source,const std::function<std::int32_t(std::string_view)>& lookup) {
    source=source.substr(0,source.find('\0'));const auto x=ReadRawTextToken(source,!source.empty() && source.front()=='(' ? 1U:0U,"x");
    const auto y=ReadRawTextToken(source,x.next,")"),tail=ReadRawTextToken(source,y.next,"#");
    const auto type=ReadRawTextToken(tail.text,0,"/"),owner=ReadRawTextToken(source,tail.next,"\n");
    return {ConfigParseLong(x.text),ConfigParseLong(y.text),std::bit_cast<std::uint32_t>(Need(lookup)(type.text)),ConfigParseLong(std::string_view(tail.text).substr(type.next)),std::bit_cast<std::uint32_t>(ConfigParseLong(owner.text))};
}
// radius는 해석하되 원문처럼 무시한다. 타입/분류/소유자 순서와 raw의 늦은 읽기를 보존한다.
void DestroyObjectsAt(std::string_view source,const DestroyAtHooks& hooks) {
    if (!Need(hooks.boss)()) { Need(hooks.assertion)(kBossAssertion);if (!Need(hooks.boss)()) return; }
    const auto location=Need(hooks.parse)(source);const auto x=PointCoordinate(location.x),y=PointCoordinate(location.y);
    auto sid=Need(hooks.begin)({x,y,x,y});
    for (;;) { // finder가 이미 반환한 현재 항목에 별도 free/dead/선택/소유자 자격을 추가하지 않는다.
        if (!sid || sid>=Need(hooks.capacity)()) return;
        const auto raw=Need(hooks.slot)(sid);if (raw.size()<35) throw std::out_of_range("DestroyAt의 현재 물리 슬롯 크기 오류");
        if ((!location.type || raw[10]==location.type) && (Need(hooks.genus)(sid)&kDestroyGenus) && (!location.owner || raw[34]==location.owner)) Need(hooks.destroy)(sid,kDestroyFlags);
        sid=Need(hooks.next)();
    }
}
}
