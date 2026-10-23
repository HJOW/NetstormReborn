// Territory의 기존 비트·현재 소유권·생성 뒤 stream 재조회를 원본 순서로 보존한다.
#include "o/RawFortTerritorySave.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 선택한 경계에서만 실제 장치가 필요한지 검사한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Territory 저장의 실제 장치 누락");return function; }
// 비정렬 필드의 지정 폭을 little endian으로 읽고 물리 범위 밖은 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Territory 현재 필드의 물리 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 지정 폭의 바이트만 합친다.
}
// 원본 stream의 지정 DWORD만 써 주변 필드를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Territory stream DWORD 물리 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 낮은 BYTE부터 기록한다.
}
// 원본 MSVC char를 signed DWORD로 확장한다.
std::int32_t SignedByte(std::uint32_t value) { return std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(value)); }
}
// 일반/Editor 분기는 최초 special만 보관하며 noRegion과 manager는 실제 소비 위치에서 다시 읽는다.
void SnapshotFortTerritory(std::int32_t slot,const FortTerritorySaveHooks& hooks) {
    std::array<std::uint8_t,120> records{};Need(hooks.load)(slot,records);const bool special=Need(hooks.special)()!=0;std::uint32_t missing{};
    for (std::int32_t region=0;region<20;++region) { // 기존 record와 현재 지도 record를 같은0..19 순서로 처리한다.
        const auto at=static_cast<std::size_t>(region)*6;auto record=std::span<std::uint8_t,6>(records.data()+at,6);
        if (!special) {
            const auto island=Need(hooks.island)(slot>=1 && slot<=8 ? slot : 0,region);
            if (island!=Need(hooks.noRegion)() && (record[1]&1U)) { const auto current=Need(hooks.record)(island);std::copy(current.begin(),current.end(),record.begin()); }
        } else {
            const auto island=Need(hooks.anyIsland)(region);
            if (island==Need(hooks.noRegion)()) Need(hooks.seed)(record,missing++);
            else { const auto current=Need(hooks.record)(island);std::copy(current.begin(),current.end(),record.begin()); }
        }
    }
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortTerritorySaveAssertion::MissingManager);
    Need(hooks.create)(Need(hooks.manager)(slot));const auto stream=Need(hooks.current)(Need(hooks.manager)(slot));
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        if (std::bit_cast<std::int32_t>(Read(fields,12))<std::bit_cast<std::int32_t>(Read(fields,4)+120U)) Need(hooks.assertion)(FortTerritorySaveAssertion::WriteCapacity);
        const auto position=Read(fields,4);auto buffer=Need(memory.buffer)(Read(fields,0));if (position>buffer.size() || buffer.size()-position<records.size()) throw std::out_of_range("Territory 현재 출력120BYTE 물리 범위");
        std::copy(records.begin(),records.end(),buffer.begin()+position);const auto next=Read(fields,4)+120U;Put(fields,4,next);if (std::bit_cast<std::int32_t>(Read(fields,8))<std::bit_cast<std::int32_t>(next)) Put(fields,8,next);
    });
}
// count는 진입 때 보관하며 최초 일치에서 반환한다. 잘못된 큰 count는 실제 접근할 때만 거부한다.
std::int32_t FindFortTerritoryIsland(std::span<const std::uint8_t> islands,bool fort,std::int32_t owner,std::int32_t region) {
    const auto count=std::bit_cast<std::int32_t>(Read(islands,0));
    for (std::int32_t index=0;index<count;++index) { // 현재3204BYTE의 존재/소유/지역 필드만 읽는다.
        const auto at=4+static_cast<std::size_t>(index)*32;if (!Read(islands,at+4)) continue;const auto current=Read(islands,at+8);
        if (fort ? (current && current!=static_cast<std::uint32_t>(owner)) : (SignedByte(Read(islands,at+13,1))!=owner || current!=static_cast<std::uint32_t>(owner))) continue;
        if (SignedByte(Read(islands,at+12,1))==region) return index;
    }return 127;
}
// Editor 검색은 원래/현재owner를 읽거나 별도의 유효성 조건을 추가하지 않는다.
std::int32_t FindAnyFortTerritoryIsland(std::span<const std::uint8_t> islands,std::int32_t region) {
    const auto count=std::bit_cast<std::int32_t>(Read(islands,0));
    for (std::int32_t index=0;index<count;++index) { const auto at=4+static_cast<std::size_t>(index)*32;if (Read(islands,at+4) && SignedByte(Read(islands,at+12,1))==region) return index; } // 첫 존재 일치만 반환한다.
    return 127;
}
// 동일 ChunkMap의 실제 조회 순서이며 축약 IslandList나 Fort 파일의 과거 record를 사용하지 않는다.
std::array<std::uint8_t,6> FortTerritoryIslandRecord(const ChunkMap& chunks,std::int32_t island) {
    for (std::int32_t y=0;y<chunks.YLength();++y) { // 현재 y 행을 먼저 고정한다.
        for (std::int32_t x=0;x<chunks.XLength();++x) { const auto& entry=chunks.At(x,y);if (SignedByte(entry.island)==island) return entry.record; } // 같은 행의 x를 먼저 진행한다.
    }return {};
}
// 순번은 원본 인자의 하위 BYTE에서6비트만 쓰고 콜백 전에 기록한 필드도 되돌리지 않는다.
void SeedFortTerritoryRecord(std::span<std::uint8_t,6> record,std::uint32_t index,const std::function<std::uint32_t(std::uint32_t)>& random) {
    record[0]=static_cast<std::uint8_t>((record[0]&0xc0U)|(index&0x3fU));record[1]=static_cast<std::uint8_t>((record[1]&0xf7U)|3U);const auto value=Need(random)(254);record[3]=static_cast<std::uint8_t>(value+1U);
}
}
