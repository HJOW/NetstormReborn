// 최초 길이/선택 Mission은 유지하고 원본이 재조회하는 현재 값만 해당 위치에서 읽는다.
#include "o/RawFortMissionSave.h"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달하지 않은 경계는 요구하지 않으며 선택된 미연결 callback만 명시 실패한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Mission 저장의 실제 경계 미연결");return function;
}
// callback 전에 캡처한 DWORD를 원본 little endian의 지정 폭으로 쓴다.
void Word(std::uint32_t stream,std::uint32_t value,std::size_t width,const FortMissionSaveHooks& hooks) {
    std::array<std::uint8_t,4> bytes{}; //실제 일반 writer가 동기 호출 동안 빌리는 고정4BYTE다.
    // 하위 BYTE부터 기록하므로2BYTE 쓰기는 길이의 하위WORD만 보존한다.
    for (std::size_t byte=0;byte<bytes.size();++byte) bytes[byte]=static_cast<std::uint8_t>(value>>(byte*8));
    Need(hooks.write)(stream,std::span(bytes).first(width));
}
}
// 첫 write가 이름의 내용을 바꾸어도 포인터와 최초 길이를 바꾸거나 새 NUL을 검색하지 않는다.
void SaveFortMissionName(std::uint32_t stream,std::optional<std::span<const std::uint8_t>> name,const FortMissionSaveHooks& hooks) {
    static constexpr std::array<std::uint8_t,1> kEmptyName{}; //null 진단 복귀 뒤 실제 NUL한BYTE를 공급한다.
    if (!name) { Need(hooks.assertion)(FortMissionSaveAssertion::NullName);name=std::span(kEmptyName); }
    const auto source=*name;const auto terminator=std::find(source.begin(),source.end(),std::uint8_t{}); //NUL뒤 BYTE는 길이에 포함하지 않는다.
    if (terminator==source.end()) throw std::out_of_range("Mission 이름의 실제 NUL 물리 범위 누락");
    const auto length=static_cast<std::size_t>(terminator-source.begin()); //이 길이는 첫 일반쓰기 callback 뒤에도 유지한다.
    if (length>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("Mission 이름의32비트 원문 길이 범위");
    Word(stream,static_cast<std::uint32_t>(length),2,hooks);
    Need(hooks.write)(stream,source.first(length)); //길이0도 원본처럼 일반 writer를 한 번 호출한다.
}
// Guide현재포인터가 첫 쓰기 중 바뀌어도 이미 선택한 Mission의 flags를 읽는다.
void SaveFortMissionState(std::uint32_t stream,std::uint32_t mission,const FortMissionSaveHooks& hooks) {
    Word(stream,Need(hooks.field)(mission,0x58),4,hooks);
    Word(stream,Need(hooks.field)(mission,0x54),4,hooks);
}
// 이름 쓰기가 끝난 뒤에만 Guide현재포인터를 다시 읽으며 일반 Battle도 같은 경로를 사용한다.
void SaveFortMission(std::uint32_t stream,const FortMissionSaveHooks& hooks) {
    if (!Need(hooks.current)()) return;
    SaveFortMissionName(stream,Need(hooks.name)(),hooks);
    SaveFortMissionState(stream,Need(hooks.current)(),hooks);
}
}
