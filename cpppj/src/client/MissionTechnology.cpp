// 원본의 매회 목록 재조회·빈 항목 종료·대소문자 비교·255칸 제한을 보존한다.
#include "client/MissionTechnology.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 도달하지 않은 효과를 필수로 요구하지 않고 선택된 실제 효과의 누락은 즉시 드러낸다.
template<class Function> const Function& TechnologyEffect(const Function& effect) {
    if (!effect) throw std::logic_error("미션 기술 허용의 실제 조회/쓰기 연결 누락");return effect;
}
}
// 원본은 미등록 타입도-1인 채 쓰기 helper에 전달한다. 안전한 실제 저장소의 거부를 생략 성공으로 바꾸지 않는다.
void ApplyMissionTechnology(const MissionTechnologyHooks& hooks) {
    std::int32_t allowed=1;std::uint32_t index{};
    // 같은 현재 Config에서 인덱스별 항목을 다시 얻으며 첫 빈 항목 뒤는 읽지 않는다.
    for (;;) {
        const auto item=TechnologyEffect(hooks.item)(std::bit_cast<std::int32_t>(index));const auto visible=std::string_view(item).substr(0,item.find('\0'));if (visible.empty()) return;
        const auto lower=o::AsciiLower(visible);
        if (lower=="deny") allowed=0;
        else if (lower=="allow") allowed=1;
        else if (lower=="all") TechnologyEffect(hooks.allowAll)(allowed);
        else TechnologyEffect(hooks.allow)(TechnologyEffect(hooks.findType)(visible),allowed);
        ++index;
    }
}
// UTF-8 저장소 경계에서만 원본 좁은 문자로 변환하고 기존 ConfigListItem/TotalMade 몸체를 재사용한다.
void ApplyMissionTechnology(MissionScript& script,const o::RiftTypeTable& types,const o::RawTotalMade& totalMade) {
    MissionTechnologyHooks hooks;
    hooks.item=[&script](std::int32_t index) {
        const auto bytes=o::EncodeOriginalText(script.Get("techAllowed").value_or(""));
        const auto list=std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size());const auto visible=list.substr(0,list.find('\0'));
        if (visible.size()>=512) throw std::length_error("미션 기술 허용의 원본512BYTE 임시 버퍼 범위");
        return o::ConfigListItem(visible,index);
    };
    hooks.findType=[&types](std::string_view name) { return types.Find(name); };
    hooks.allowAll=[&totalMade](std::int32_t value) { totalMade.AllowAll(value); };
    hooks.allow=[&totalMade](std::int32_t type,std::int32_t value) { totalMade.Allow(type,value); };
    ApplyMissionTechnology(hooks);
}
}
