// 현재 후보/타입 재조회와 원본 폭별 쓰기, 고정된 목록 개수 위치를 보존한다.
#include "o/RawFortContentSave.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace netstorm::o {
namespace {
// 실제 선택한 효과가 없으면 앞선 부분 쓰기를 보존한 채 해당 지점에서 실패한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Fort 내용물 저장의 실제 경계 미연결");return function; }
// 비정렬 BYTE/WORD/DWORD를 읽는다. 원본 물리 슬롯 밖은 성공으로 처리하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("Fort 내용물 저장의 원문 필드 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; // 지정 폭의 낮은 바이트부터 조합한다.
}
// 같은 stream의 지정 DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Fort 내용물 저장의 stream 필드 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접 필드는 보존한다.
}
// 원본 header가 예약한 당시의 data+offset을 고정한다. 이후 현재 data 변경과 별개다.
struct CountPosition {
    std::uint32_t data{},offset{}; // 실제 포인터를 자르지 않는 버퍼 토큰과 그 안의 개수 BYTE 위치다.
};
// 원문 header는 용량 진단 없이 pos/end를 먼저 진행한 다음 보관한 주소에0을 쓴다.
CountPosition ReserveCount(std::uint32_t stream,const FortContentSaveHooks& hooks) {
    CountPosition count;
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        count={Read(fields,0),Read(fields,4)};const auto next=count.offset+1;Put(fields,4,next);if (std::bit_cast<std::int32_t>(Read(fields,8))<std::bit_cast<std::int32_t>(next)) Put(fields,8,next);
        auto buffer=Need(memory.buffer)(count.data);if (count.offset>=buffer.size()) throw std::out_of_range("Fort 내용물 저장의 개수 BYTE 물리 범위");buffer[count.offset]=0;
    });return count;
}
// robust 통과 직후 원래 개수 위치를 BYTE로 증가시킨다. 256번째 항목은0으로 감긴다.
void Increment(CountPosition count,std::uint32_t stream,const FortContentSaveHooks& hooks) {
    Need(hooks.access)(stream,[&](auto,const auto& memory) { auto buffer=Need(memory.buffer)(count.data);if (count.offset>=buffer.size()) throw std::out_of_range("Fort 내용물 저장의 해제된 개수 위치");++buffer[count.offset]; });
}
// 현재 data/pos는 용량 진단 뒤 다시 읽고 이미 캡처한 값만 원본 BYTE/WORD 폭으로 쓴다.
void Append(std::uint32_t value,std::uint32_t width,std::uint32_t stream,const FortContentSaveHooks& hooks) {
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        if (std::bit_cast<std::int32_t>(Read(fields,12))<std::bit_cast<std::int32_t>(Read(fields,4)+width)) Need(hooks.assertion)(FortContentSaveAssertion::WriteCapacity);
        auto buffer=Need(memory.buffer)(Read(fields,0));const auto position=Read(fields,4);if (position>buffer.size() || width>buffer.size()-position) throw std::out_of_range("Fort 내용물 저장의 실제 출력 범위");
        for (std::uint32_t byte=0;byte<width;++byte) buffer[position+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // BYTE/WORD를 little endian으로 쓴다.
        const auto next=Read(fields,4)+width;Put(fields,4,next);if (std::bit_cast<std::int32_t>(Read(fields,8))<std::bit_cast<std::int32_t>(next)) Put(fields,8,next);
    });
}
// 원본의 재귀별 finder/고정 부모/개수 위치를 동적 프레임으로 보관한다.
struct SaveFrame {
    std::uint32_t parent{}; // 현재 타입 fallback에 사용하는 진입 당시 부모다.
    CountPosition count; // 이 목록의 진입 당시 개수 BYTE 주소다.
    FortContentSaveCursor cursor; // 자식이 끝날 때까지 살아 있는 독립 현재/다음 검색기다.
};
}
// 명시 프레임은 원본의 깊이 우선 순서와 자식 복귀 뒤 부모 Next 호출을 보존한다.
void SaveFortContents(std::uint32_t parent,std::uint32_t stream,const FortContentSaveHooks& hooks) {
    std::vector<SaveFrame> frames;std::unordered_set<std::uint32_t> active;
    const auto enter=[&](std::uint32_t selected) { // 순환 목록을 원본의 스택 소진 대신 물리 손상으로 보고한다.
        if (!active.insert(selected).second) throw std::logic_error("Fort 내용물 저장의 순환 부모");const auto count=ReserveCount(stream,hooks);auto cursor=Need(hooks.begin)(selected);frames.push_back({selected,count,std::move(cursor)});
    };
    enter(parent);
    while (!frames.empty()) { // 현재 자식 목록이 끝나면 부모의 보존된 next부터 계속한다.
        auto& frame=frames.back();const auto candidate=Need(frame.cursor.current)();
        if (!candidate || candidate>=Need(hooks.poolLimit)()) { active.erase(frame.parent);frames.pop_back();if (!frames.empty()) Need(frames.back().cursor.next)();continue; }
        if (Need(hooks.robust)(candidate)) {
            Increment(frame.count,stream,hooks);const auto kind=Read(Need(hooks.form)(Need(frame.cursor.current)()),0x12,2);const auto flags=Need(hooks.type)(kind).flags1;
            Append(kind,1,stream,hooks);
            if (flags&0x1000U) Append(Read(Need(hooks.form)(Need(frame.cursor.current)()),0xc,1),1,stream,hooks);
            if (flags&0x2000U) Append(Read(Need(hooks.form)(Need(frame.cursor.current)()),0xc,2),2,stream,hooks);
            auto required=Need(hooks.type)(kind).contentListFlags; // 두 번째 조회도 첫 kind WORD를 사용하고 현재 metadata를 다시 읽는다.
            if (!required) required=Need(hooks.parentType)(frame.parent).containerListFlags;
            if (!required) Append(Read(Need(hooks.form)(Need(frame.cursor.current)()),0x14,1),1,stream,hooks);
            else if ((Read(Need(hooks.form)(Need(frame.cursor.current)()),0x14,2)&required)!=required) Need(hooks.assertion)(FortContentSaveAssertion::ListFlags);
            if (flags&0x20000U) { const auto nested=Need(frame.cursor.current)();enter(nested);continue; }
        }
        Need(frame.cursor.next)();
    }
}
}
