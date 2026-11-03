// 원문은 두 핸들을 닫지 않는다. 실제 자원 회수는 호출 어댑터의 현대 소유 경계다.
#include "client/ReadOnlyFileMap.h"
#include <stdexcept>

namespace netstorm::client { // 같은 원문 부모를 실제 W API와 독립 관측 장치가 공유한다.
namespace { // 호출 시 도달한 필수 장치만 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("읽기전용 매핑의 선택된 장치 미연결");return function; } // 미선택 진단/OS 장치는 미리 요구하지 않는다.
constexpr std::string_view kSource="\\Ns\\O\\Winutil.cpp"; // 원문 assertion의 실제 소스 문자열이다.
}
std::uintptr_t MapReadOnlyFile(std::string_view name,std::uintptr_t invalidFile,const ReadOnlyFileMapHooks& hooks) { // 첫NUL까지만 OS 이름으로 전달한다.
    const auto file=Need(hooks.createFile)(name.substr(0,name.find('\0')),0x80000000U,1,ReadOnlyMapSecurity{},3,0x80,0);
    if (file==invalidFile) Need(hooks.assertion)("fh != INVALID_HANDLE_VALUE",kSource,0x17e);
    const auto mapping=Need(hooks.createMapping)(file,0,2,0,0,0);
    if (!mapping) Need(hooks.assertion)("fm != NULL",kSource,0x181);
    const auto view=Need(hooks.mapView)(mapping,4,0,0,0);
    if (!view) Need(hooks.assertion)("p",kSource,0x184);
    return view;
}
}
