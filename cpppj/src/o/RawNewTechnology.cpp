// 중복 기술도 저장/안내를 실행하고 각 외부 효과 뒤의 현재 getter를 원문 위치에서 읽는다.
#include "o/RawNewTechnology.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 기술 추가/저장/안내를 한 부모로 묶되 child의 게임 계산을 대신하지 않는다.
namespace { // 원문에서 실제 도달한 외부 경계만 필수로 요구한다.
constexpr std::string_view kPicture="<!`\"%d.*`\">"; //원문 지역256BYTE 버퍼의 고정 그림 서식이다.
template<class Function> const Function& Need(const Function& function) { // 선택된 경계가 없으면 앞선 부분효과를 유지하고 중단한다.
    if (!function) throw std::logic_error("기술 획득의 실제 선택 장치 미연결");return function;
}
// 기존 C 문자열의 첫NUL 뒤를 Config 값으로 추가하지 않는다.
std::string_view Text(std::string_view value) { return value.substr(0,value.find('\0')); }
}
// 첫 getter의 버린 반환과 조건부 세 번째 getter까지 원문의 callback 순서를 유지한다.
void AcquireNewTechnology(OriginalEdition edition,std::uint32_t type,const NewTechnologyHooks& hooks) {
    if (edition!=OriginalEdition::Patch1078) throw std::invalid_argument("기술 획득 부모는 실제10.78 경계가 필요합니다");
    static_cast<void>(Need(hooks.technologyParent)());
    const auto parent=Need(hooks.technologyParent)();
    if (!Need(hooks.find)(parent,4,type,0)) Need(hooks.create)(Need(hooks.technologyParent)(),type,0,4,0x10,0);
    Need(hooks.snapshot)();
    const auto record=Need(hooks.type)(type);const auto name=Need(hooks.name)(record);Need(hooks.configure)("global.newTechName",Text(name));
    const auto picture=Need(hooks.format)(kPicture,std::bit_cast<std::int32_t>(type));Need(hooks.configure)("global.newTechPic",Text(picture));
    Need(hooks.tell)("NewTech");
}
}
