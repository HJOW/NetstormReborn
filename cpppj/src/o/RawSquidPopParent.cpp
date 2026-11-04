// 공통 Pop의 저장된 범위와 현재 raw 조회, 진단 복귀 뒤 실행을 같은 물리에서 처리한다.
#include "o/RawSquidPopParent.h"
#include "o/SquidPop.h"
#include "o/Squid.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 원문10.78의 현재 슬롯만 사용하며 호스트 포인터를 원본 주소로 해석하지 않는다.
namespace { // BYTE/WORD/DWORD와 원문 로그의 좁은 공통 경계를 보관한다.
constexpr std::uint32_t kBuildingMask=0x50444200U; // PE005424b8의 고정 초기 건물군이다.
constexpr std::uint32_t kInvalidIsland=127; // PE0050c038의 invalidIslandId이며 타입 번호와 다르다.
constexpr std::string_view kSource="\\Ns\\O\\Squid.cpp"; // 원문 Pop·SetIsland의 진단 소스다.
// 현재 실행 중인 callable이 연결 교체로 소멸하지 않도록 선택한 함수만 복사한다.
template<class F> F NeedPop(const F& function) { if (!function) throw std::logic_error("공통 Pop의 선택된 실제 장치 미연결");return function; }
// free/dead 진단 복귀도 같은 물리 슬롯을 사용하며 읽기 전용 보기의 저장소 자체는 가변 풀이다.
std::span<std::uint8_t> PopRaw(SidPool& pool,Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
// 정렬되지 않은 원문 DWORD를 낮은 바이트부터 조립한다.
std::uint32_t PopRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8); // 선택한 필드의 네 바이트만 읽는다.
    return value;
}
// 정확한 원문 저장 폭만 쓰며 이웃 payload를 유지한다.
void PopPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // WORD/DWORD의 낮은 바이트부터 저장한다.
}
// 위치 float의 저장 비트를 추가 변환 없이 읽는다.
float PopFloat(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(PopRead(bytes,offset)); }
// 원문 유한 입력의 ftol만 허용한다. 범위 밖 실제 메모리 접근은 호스트 오류로 남긴다.
std::int32_t PopInteger(float value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Pop 원문 정수 좌표 범위");
    return static_cast<std::int32_t>(value);
}
// imul/sub/inc의 하위 DWORD 감김을 보존하여 음수/0 발자국을 임의 양수로 보정하지 않는다.
std::int32_t PopStart(std::int32_t end,std::int32_t footprint) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(end)-std::bit_cast<std::uint32_t>(footprint)+1U); }
// 진단의 원래 파일/줄을 유지하며 정상 반환하면 호출 위치 다음에서 계속한다.
void PopAssert(const SquidPopParentHooks& hooks,std::string_view expression,std::uint32_t line,std::string_view file=kSource) { NeedPop(hooks.assertion)(expression,file,line); }
// TYPE+4 이름의 첫 NUL까지만 읽는다. 번역된 설명을 사용하지 않는다.
std::string PopName(const RiftTypeRecord& record) { return record.name.substr(0,record.name.find('\0')); }
// 이미 저장한 두 TYPE 이름과 SID로 원문 grid 교체 로그를 작성한다.
void PopGridLog(const SquidPopParentHooks& hooks,Sid sid,std::string_view name,Sid old,std::string_view oldName) {
    std::array<char,512> text{};
    const auto count=std::snprintf(text.data(),text.size(),"ERROR: gridSid %d( %s) replacing %d( %s)\n",int(sid.value),std::string(name).c_str(),int(old.value),std::string(oldName).c_str());
    if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::length_error("Pop grid 로그의 호스트 버퍼 범위");
    NeedPop(hooks.trace)(29,{text.data(),static_cast<std::size_t>(count)});
}
}

RawPopResult RunSquidPopParent(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,Sid sid,float x,float y,std::uint32_t flags,RawCheckingFlag checking,const SquidPopParentHooks& hooks) { // 실제004b02d0의 전체 외부 호출 순서를 따른다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || spots.size()!=65536) throw std::invalid_argument("현재 Pop 부모의 PATCH/전체 spot 필요");
    if (!sid.value || sid.value>=pool.Capacity()) return RawPopResult::Unchanged;
    auto raw=PopRaw(pool,sid);if (!(raw[11]&4U) || (raw[11]&8U)) return RawPopResult::Unchanged;
    if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("Pop의 유한 좌표 필요");
    if (x<=0 || y<=0 || x>=256 || y>=256) {
        if (!NeedPop(hooks.metamap)()) {
            const auto name=PopName(NeedPop(hooks.type)(raw[10]));std::array<char,512> text{};
            const auto count=std::snprintf(text.data(),text.size(),"Error %s popped to( %5.1f,%5.1f). Set to( 10,10)\n",name.c_str(),double(x),double(y));
            if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::length_error("Pop 좌표 로그의 호스트 버퍼 범위");
            NeedPop(hooks.trace)(29,{text.data(),static_cast<std::size_t>(count)});
        }
        x=y=10;
    }
    PopPut(raw,14,std::bit_cast<std::uint32_t>(x),4);PopPut(raw,18,std::bit_cast<std::uint32_t>(y),4);
    PopPut(raw,22,static_cast<std::uint32_t>(double(x)*16.0+0.5),2);PopPut(raw,24,static_cast<std::uint32_t>(double(y)*11.0+0.5),2);
    const auto& initial=NeedPop(hooks.type)(raw[10]);const auto genus=initial.flags2; // 좌표 저장 뒤 TYPE 포인터/그 genus만 보관한다.
    if ((genus&6U) && !(raw[40]&8U)) NeedPop(hooks.miniMap)(sid,genus);
    if (((genus&(kBuildingMask|0x400ffU)) || (NeedPop(hooks.type)(raw[10]).flags1&0x800U)) && !(raw[40]&8U) && !(raw[11]&8U)) {
        const auto right=PopInteger(x),bottom=PopInteger(y),left=PopStart(right,initial.footX),top=PopStart(bottom,initial.footY); // 외부 미니맵이 raw 좌표를 바꿔도 원문 인자 좌표 범위는 유지한다.
        if (!(left>0 && left<256 && top>0 && top<256)) PopAssert(hooks,"start.isValid()",0x6b7);
        for (std::int32_t cy=top;cy<=bottom;++cy) { // 진단 복귀 뒤에도 처음 저장한 행 범위를 유지한다.
            for (std::int32_t cx=left;cx<=right;++cx) { // 매 칸의 effective genus와 표면 여부는 현재 TYPE/좌표를 다시 읽는다.
                if (cx<0 || cy<0 || cx>=256 || cy>=256) throw std::out_of_range("Pop spot의 물리 지도 범위");
                const auto& current=NeedPop(hooks.type)(raw[10]);
                const auto effective=static_cast<std::uint8_t>(Squid::EffectiveGenus(current.flags2,PopFloat(raw,14),PopFloat(raw,18),current.footX,current.footY,cx,cy));
                if (effective) { const auto saved=spots[cy*256+cx];if (saved&effective) return RawPopResult::Overlap;spots[cy*256+cx]=static_cast<std::uint8_t>(saved|effective); }
                if ((genus&kBuildingMask) && (cx==left || cx==right) && cy!=top && cy!=bottom) {
                    const Sid surface{hash.Cell(0,cx,cy)};
                    if (surface.value && surface.value<pool.Capacity()) NeedPop(hooks.side)(cx,cy,PopFloat(raw,14),PopFloat(raw,18));
                }
                if (NeedPop(hooks.type)(raw[10]).flags1&0x800U) {
                    if (hash.Cell(0,cx,cy)) {
                        const Sid replaced{hash.Cell(0,cx,cy)}; // 두 grid 조회 뒤 저장한 후보는 진단이 지도를 바꿔도 유지한다.
                        if (checking) PopAssert(hooks,"false",0x6e8);
                        const auto name=PopName(NeedPop(hooks.type)(raw[10]));const auto oldName=PopName(NeedPop(hooks.type)(PopRaw(pool,replaced)[10]));
                        PopGridLog(hooks,sid,name,replaced,oldName);
                    }
                }
            }
        }
    }
    const auto levelGenus=NeedPop(hooks.type)(raw[10]).flags2;
    int level{};
    if (!(levelGenus&6U)) { const auto size=NeedPop(hooks.frameSize)(sid);level=SquidHash::ObjectLevel(levelGenus,size[0],size[1]); }
    raw[33]=static_cast<std::uint8_t>(level);
    auto& head=hash.Bucket(level,PopFloat(raw,14),PopFloat(raw,18));const auto next=head;
    if (next==sid.value) PopAssert(hooks,"n != getSid()",0x123,R"(c:\ns\o\squid.h)");
    PopPut(raw,4,next,2);head=sid.value; // 진단 뒤에도 선택한 버킷과 저장한 next를 사용한다.
    if (!raw[33]) {
        const auto islandGenus=NeedPop(hooks.type)(raw[10]).flags2; //004acd20은 현재 genus를 한 번 보관한다.
        if (!(islandGenus&0x1000006U)) PopAssert(hooks,"genus & (gISLAND|gBRIDGE|gISLAND3x3)",0x4b9);
        else if (!(islandGenus&4U)) PopPut(raw,8,kInvalidIsland,2); // 다리는 기존 WORD를 보존한다.
    }
    NeedPop(hooks.update)(sid,flags);
    if (!(raw[40]&0x80U)) { flags|=NeedPop(hooks.firstPop)(sid);raw[40]|=0x80; }
    NeedPop(hooks.activate)(sid,flags);
    if (raw[11]&8U) PopAssert(hooks,"!isContained()",0x71a);
    return RawPopResult::Registered;
}
}
