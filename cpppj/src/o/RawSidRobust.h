// 원문004ac040/004ac140의현재Pool·원시진단·선택assert뒤반환을복원한다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // SID 슬롯 번호에는 세대 비트를 더하지 않으며 호스트 객체ID와 구별한다.
enum class SidRobustAddressMode { Original32,HostPointer }; // Original32는DWORD주소감김,HostPointer는실제현재Pool포인터폭을사용한다.
struct RawSidRobustDiagnostics { // 같은Client의원문두DWORD이며Session교체/검사호출로새owner를만들지않는다.
    std::uint32_t lastRobust{}; //005c8450의최근실패bit이며실제ac040진입때만0으로쓴다.
    std::uint32_t type{}; //005c7c48이며주소범위검사보다먼저현재object+10BYTE를넓혀쓴다.
};
struct SidRobustHooks { // 미선택checking/Pool/메모리/진단장치는선행요구하지않는다.
    SidRobustAddressMode addressMode{SidRobustAddressMode::Original32}; // 보호PE토큰과실제64비트Pool주소를명시구별한다.
    std::function<std::uintptr_t()> poolBase; // 현재005c8464토큰이다. ac140과ac040은각자필요한시점에읽는다.
    std::function<std::uint32_t()> capacity; // 현재005c847cDWORD이며ac040의아래범위통과뒤에만읽는다.
    std::function<std::span<const std::uint8_t>(std::uintptr_t,std::size_t)> memory; // 선택된주소/폭만빌린다. 풀밖·없는물리는그접근에서거부한다.
    std::function<std::uint32_t()> checking; //005e4794전체DWORD현재값을부모의유효SID/live분기에서만읽는다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //실패식과0x218이다. 정상복귀하면부모는SID범위가정상인1을반환한다.
};
bool RawSquidIsRobust(std::uintptr_t object,std::int32_t live,RawSidRobustDiagnostics& diagnostics,const SidRobustHooks& hooks); //ac040의원시필드검사다.정렬/세대/void/TYPE표상한관문을합성하지않는다.
bool RawIsSidRobust(std::int32_t sid,std::int32_t live,RawSidRobustDiagnostics& diagnostics,const SidRobustHooks& hooks); //ac140의signed1..65534판정과선택된checking진단부모다.
}
