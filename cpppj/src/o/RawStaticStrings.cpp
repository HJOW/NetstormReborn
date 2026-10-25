// 현재 전역 재조회와 한 store 이전의 부분 효과를 유지한다. 예약 폭은 출력 절단 한도가 아니다.
#include "o/RawStaticStrings.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { // 고리 순서는 원문 본체가 소유하고 장치는 같은 실제 물리만 제공한다.
namespace { // 선택하지 않은 CRT/진단을 선행 요구하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("임시 문자열 실제 장치 미연결");return function; } // 도달한 장치만 요구한다.
void Store(StaticStringHooks const& hooks,std::uint32_t address,std::uint32_t width,std::uint8_t value) {
    auto bytes=Need(hooks.memory)(address,width);if (bytes.size()<width) throw std::out_of_range("임시 문자열 원문 store 물리 범위");std::fill_n(bytes.begin(),width,value);
} // 한 DWORD/BYTE 전체 폭을 확인한 뒤 해당 바이트만 작성한다.
}
void InitializeStaticStrings(std::uint32_t size,StaticStringGlobals globals,const StaticStringHooks& hooks) {
    if (globals.base) Need(hooks.assertion)(17);
    globals.size=size;const auto allocated=Need(hooks.allocate)(size);const auto count=globals.size;globals.base=allocated; // 원문은 new 복귀 뒤 현재 size를 읽고 반환주소를 base에 쓴다.
    std::uint32_t destination=allocated;
    for (std::uint32_t word=0;word<(count>>2);++word,destination+=4) Store(hooks,destination,4,0); // rep stosd의 전체 폭/부분 효과다.
    for (std::uint32_t byte=0;byte<(count&3U);++byte,++destination) Store(hooks,destination,1,0); // 마지막 나머지만 BYTE로 쓴다.
    const auto base=globals.base;globals.current=base;Store(hooks,base,1,0xd5);globals.current=globals.current+1U; // last는 이 부모가 쓰지 않는다.
}
std::uint32_t ReserveStaticString(std::uint32_t size,StaticStringGlobals globals,const StaticStringHooks& hooks) {
    auto base=globals.base;if (!base) { InitializeStaticStrings(8192,globals,hooks);base=globals.base; }
    const auto end=globals.size+base;auto start=globals.current; // 두 주소 합은 원문 DWORD로 감긴다.
    if (start+size+1U>=end) { globals.current=base;Store(hooks,base,1,0xd5);start=globals.current+1U; }
    globals.last=start;globals.current=start+size;Store(hooks,globals.current,1,0xd5);++globals.current;return globals.last;
}
std::uint32_t FormatStaticString(std::uint32_t size,std::uint32_t pattern,StaticStringGlobals globals,const StaticStringHooks& hooks) {
    const auto result=ReserveStaticString(size,globals,hooks);if (result!=pattern) Need(hooks.format)(result,pattern);return result; // CRT가 last/current를 바꿔도 캡처한 반환주소를 유지한다.
}
}
