// 저장 직전 재조회와 signed 분기, 청크의 미초기화 count 예약을 보존한다.
#include "o/RawFortObjectSave.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kInt64Boundary=9223372036854775808.0; // 원본 __ftol의 signed64 불확정값 경계다.
// 실제 선택한 장치가 없으면 앞선 부분 쓰기를 보존한 채 중단한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Fort 객체 저장의 실제 장치 누락");return function; }
// 비정렬 raw 값을 지정 폭으로 읽으며 물리 범위 밖은 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Fort 객체 저장의 raw 필드 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 지정 폭의 BYTE만 조합한다.
}
// 지정 DWORD만 little endian으로 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Fort 청크 저장의 stream 필드 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이웃 필드는 보존한다.
}
// DWORD 산술의 감김 뒤 같은 비트를 signed로 읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 현재 BYTE/WORD 값은 외부 Write 이전에 캡처한다.
void Write(std::uint32_t stream,std::uint32_t value,std::size_t width,const FortObjectSaveHooks& hooks) {
    const std::array<std::uint8_t,2> bytes{static_cast<std::uint8_t>(value),static_cast<std::uint8_t>(value>>8)};Need(hooks.write)(stream,std::span(bytes).first(width));
}
}
// 입력 float는 double에 정확히 표현되므로 이 변환에는 중간 x87 정밀도 반올림이 없다.
std::int32_t FortSaveCoordinate(float input) {
    const double value=input;if (!std::isfinite(value) || value>=kInt64Boundary || value< -kInt64Boundary) return 0;
    return Signed(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 초기 좌표는 고정하지만 뒤의 실제 virtual/TYPE/필드는 매 원본 소비 위치에서 읽는다.
std::uint32_t SaveFortObject(std::uint32_t stream,std::uint32_t sid,std::int32_t originX,std::int32_t originY,const FortObjectSaveHooks& hooks) {
    const auto raw=[&]() { return Need(hooks.form)(sid); }; // 같은 고정 raw 슬롯을 효과 뒤에도 다시 빌린다.
    const auto global=[&](FortObjectSaveGlobal key) { return Need(hooks.global)(key); }; // 각 조건의 현재 DWORD를 읽는다.
    const auto x=FortSaveCoordinate(std::bit_cast<float>(Read(raw(),14))),y=FortSaveCoordinate(std::bit_cast<float>(Read(raw(),18)));
    if (x>=Signed(static_cast<std::uint32_t>(originX)+16U) || y>=Signed(static_cast<std::uint32_t>(originY)+16U) || (Read(raw(),40,1)&9U)) return 0;
    if (global(FortObjectSaveGlobal::Battle) && !global(FortObjectSaveGlobal::Mission) && !global(FortObjectSaveGlobal::Editor)) {
        const auto flags=Need(hooks.flags2)(sid); // getter의 효과 뒤 현재 마스크를 다시 읽는다.
        if (!(flags&global(FortObjectSaveGlobal::BattleMask))) return 0;
    }
    const auto island=[&]() { const auto current=raw();return Need(hooks.island)(Read(current,14),Read(current,18)); }; // 원본 가상60 뒤의 현재 float 비트를 캡처한다.
    bool selected{};
    if (global(FortObjectSaveGlobal::Scope)==1) selected=Need(hooks.robust)(sid)!=0 && island()!=0;
    if (global(FortObjectSaveGlobal::Scope)==2) { if (!Need(hooks.robust)(sid) || island()) return 0; }
    else if (!selected) return 0;
    if (Read(raw(),10,1)==global(FortObjectSaveGlobal::LocalOnlyType) && Read(raw(),34,1)!=global(FortObjectSaveGlobal::LocalOwner)) return 0;
    const auto cx=Signed(static_cast<std::uint32_t>(x)-static_cast<std::uint32_t>(originX)),cy=Signed(static_cast<std::uint32_t>(y)-static_cast<std::uint32_t>(originY));
    if (cx<0 || cx>15) Need(hooks.assertion)(FortObjectSaveAssertion::XOffset);
    if (cy<0 || cy>15) Need(hooks.assertion)(FortObjectSaveAssertion::YOffset);
    Write(stream,(static_cast<std::uint32_t>(cx)<<4)|static_cast<std::uint8_t>(cy),1,hooks);
    const auto kind=Read(raw(),10,1);Write(stream,kind==global(FortObjectSaveGlobal::BridgeType) ? ~Read(raw(),34,1) : kind,1,hooks);
    if (Need(hooks.flags1)(sid)&0x20U) Write(stream,Read(raw(),36,1),1,hooks);
    if (Need(hooks.flags1)(sid)&0x1000U) Write(stream,Read(raw(),12,1),1,hooks);
    if (Need(hooks.flags1)(sid)&0x2000U) Write(stream,Read(raw(),12,2),2,hooks);
    if (Need(hooks.flags2)(sid)&4U) Write(stream,Read(raw(),36,1),1,hooks);
    if (!global(FortObjectSaveGlobal::Version) && Need(hooks.hasLevel)(sid)) Write(stream,Need(hooks.level)(sid),1,hooks);
    if (Signed(global(FortObjectSaveGlobal::Version))>1 && (Need(hooks.flags2)(sid)&0x4000U)) Write(stream,Need(hooks.level)(sid),1,hooks);
    if (Signed(global(FortObjectSaveGlobal::Version))>0) {
        const auto flags=Need(hooks.flags2)(sid); // getter의 효과 뒤 현재 마스크를 다시 읽는다.
        if (flags&global(FortObjectSaveGlobal::OwnerMask)) {
            if (Read(raw(),34,1)>8) Need(hooks.assertion)(FortObjectSaveAssertion::Owner);Write(stream,Read(raw(),34,1),1,hooks);
        }
    }
    if (Need(hooks.flags1)(sid)&0x20000U) Need(hooks.contents)(sid,stream);return 1;
}
// 청크 원점은 감긴 int<<4를 먼저 float32에 저장하고 원본 __ftol의 EAX를 보관한다.
void SaveFortChunk(std::uint32_t stream,std::int32_t chunkX,std::int32_t chunkY,const FortChunkSaveHooks& hooks) {
    const auto x=static_cast<std::uint32_t>(chunkX)<<4,y=static_cast<std::uint32_t>(chunkY)<<4;const FortChunkSaveArea area{static_cast<float>(Signed(x)),static_cast<float>(Signed(y)),static_cast<float>(Signed(x+15U)),static_cast<float>(Signed(y+15U))};
    const auto originX=FortSaveCoordinate(area.left),originY=FortSaveCoordinate(area.top);auto cursor=Need(hooks.begin)(area);
    Need(hooks.access)(stream,[](auto fields,const auto& memory) { const std::array<std::uint8_t,1> marker{99};WriteMemStream(fields,marker,1,memory); });
    std::uint32_t data{},offset{};Need(hooks.access)(stream,[&](auto fields,const auto&) { data=Read(fields,0);offset=Read(fields,4);const auto next=offset+2U;Put(fields,4,next);if (Signed(Read(fields,8))<Signed(next)) Put(fields,8,next); }); // count 예약은0을 쓰지 않는다.
    std::uint16_t count{};
    for (;;) { // 원본 현재 후보가 물리 범위를 벗어날 때만 count를 고정 주소에 쓴다.
        const auto candidate=Need(cursor.current)();if (!candidate || candidate>=Need(hooks.poolLimit)()) break;
        count=static_cast<std::uint16_t>(count+Need(hooks.object)(stream,candidate,originX,originY));Need(cursor.next)();
    }
    Need(hooks.access)(stream,[&](auto,const auto& memory) { auto buffer=Need(memory.buffer)(data);if (offset>buffer.size() || buffer.size()-offset<2) throw std::out_of_range("Fort 청크 countWORD의 고정 출력 범위");buffer[offset]=static_cast<std::uint8_t>(count);buffer[offset+1]=static_cast<std::uint8_t>(count>>8); });
}
}
