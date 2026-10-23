// 전체 Objects 부모의 Temp 버전BYTE쓰기·현재manager재조회·조건부rename/Chaff를 복원한다.
#include "o/RawFortObjectsSave.h"
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::o {
namespace {
// 선택되지 않은 하위는 요구하지 않고 도달한 미연결 경계만 명시 실패시킨다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Objects 저장 부모의 실제 경계 미연결");return function; }
// 실제 비정렬 DWORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Objects stream DWORD 물리 범위");std::uint32_t value{};
    // 해당 네 BYTE만 합쳐 주변 필드를 읽지 않는다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value;
}
// 원본16BYTE stream의 지정 DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Objects stream DWORD 쓰기 범위");
    // 낮은 BYTE부터 원본 폭을 보존한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// version은 진단 전에 캡처하며 진단 복귀 뒤의 현재 data/pos로 실제 BYTE만 쓴다.
void Version(std::uint32_t stream,std::uint8_t value,const FortObjectsSaveHooks& hooks) {
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        if (std::bit_cast<std::int32_t>(Read(fields,12))<std::bit_cast<std::int32_t>(Read(fields,4)+1U)) Need(hooks.assertion)(FortObjectsSaveAssertion::WriteCapacity);
        const auto position=Read(fields,4);auto bytes=Need(memory.buffer)(Read(fields,0));if (position>=bytes.size()) throw std::out_of_range("Objects version 출력 BYTE 물리 범위");
        bytes[position]=value;const auto next=Read(fields,4)+1U;Put(fields,4,next);if (std::bit_cast<std::int32_t>(Read(fields,8))<std::bit_cast<std::int32_t>(next)) Put(fields,8,next);
    });
}
// Temp의포인터·논리end가모두비영일때만rename한다.pos나논리자료길이1이상을추가조건으로쓰지않는다.
bool HasData(std::uint32_t stream,const FortObjectsSaveHooks& hooks) {
    bool result{};Need(hooks.access)(stream,[&](auto fields,const auto&) { result=Read(fields,0)!=0 && Read(fields,8)!=0; });return result;
}
}
// 진단후의현재manager와낮은versionBYTE를사용하며owner는고정this슬롯에서만clamp한다.
void SaveFortObjects(std::int32_t slot,FortObjectsSaveGlobals globals,const FortObjectsSaveHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortObjectsSaveAssertion::MissingTemplate);
    if (std::bit_cast<std::uint32_t>(slot)!=globals.templateOwner) Need(hooks.assertion)(FortObjectsSaveAssertion::NotMine);
    globals.version=2;
    // 원본은존재하지않는Terr를먼저생성한뒤각영역마다새Temp를만든다.
    for (std::int32_t region=0;region<20;++region) {
        const auto name=std::string("Terr")+(region<10 ? "0" : "")+std::to_string(region);
        if (!Need(hooks.find)(Need(hooks.manager)(slot),name)) Need(hooks.create)(Need(hooks.manager)(slot),name);
        const auto stream=Need(hooks.create)(Need(hooks.manager)(slot),"Temp");Version(stream,static_cast<std::uint8_t>(globals.version),hooks);
        Need(hooks.territory)(stream,slot>=1 && slot<=8 ? static_cast<std::uint32_t>(slot) : 0U,region);
        const auto selected=Need(hooks.select)(Need(hooks.manager)(slot),"Temp",1);
        if (HasData(selected,hooks)) Need(hooks.rename)(Need(hooks.manager)(slot),name,"Temp");
    }
    if (Need(globals.fort)()) {
        const auto stream=Need(hooks.create)(Need(hooks.manager)(slot),"Chaff");Version(stream,static_cast<std::uint8_t>(globals.version),hooks);Need(hooks.chaff)(stream);
    }
}
}
