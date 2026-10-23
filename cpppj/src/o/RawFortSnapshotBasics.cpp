// 현재 선택과 별개인 stream을 보존하며 원본 인라인 MemStream 쓰기와 재조회 순서를 복원한다.
#include "o/RawFortSnapshotBasics.h"
#include "o/RiftType.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달한 외부 경계가 빠지면 앞선 원본 효과를 유지한 채 실패한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Fort snapshot 실제 경계 미연결");return function; }
// 비정렬 원본 DWORD를 읽으며 물리 범위를 검사한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Fort snapshot DWORD 물리 범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; // 원본 little endian의 네 바이트만 읽는다.
}
// 같은 stream의 지정 DWORD만 바꾸어 다른 필드를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Fort snapshot DWORD 쓰기 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 낮은 바이트부터 원본 폭으로 쓴다.
}
// 원본 인라인 BYTE/DWORD 쓰기는 진단 후 현재 data/pos를 다시 읽고 pos/end만 진행한다.
void Append(std::uint32_t stream,std::uint32_t value,std::uint32_t width,const FortSnapshotBasicsHooks& hooks) {
    Need(hooks.access)(stream,[&](auto fields,const auto& memory) {
        if (std::bit_cast<std::int32_t>(Read(fields,12))<std::bit_cast<std::int32_t>(Read(fields,4)+width)) Need(hooks.assertion)(FortSnapshotAssertion::WriteCapacity);
        auto buffer=Need(memory.buffer)(Read(fields,0));const auto position=Read(fields,4); // 용량 진단이 바꾼 현재 필드로 실제 쓰기 주소를 결정한다.
        if (position>buffer.size() || width>buffer.size()-position) throw std::out_of_range("Fort snapshot 실제 인라인 버퍼 범위");
        for (std::uint32_t byte=0;byte<width;++byte) buffer[position+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이미 캡처한 원본 값만 지정 폭으로 쓴다.
        const auto next=Read(fields,4)+width;Put(fields,4,next);if (std::bit_cast<std::int32_t>(Read(fields,8))<std::bit_cast<std::int32_t>(next)) Put(fields,8,next);
    });
}
// 같은 캡처 덱의 현재 signed count를 반복 조건마다 조회한다.
std::int32_t DeckCount(std::uint32_t token,const FortSnapshotBasicsHooks& hooks) { return std::bit_cast<std::int32_t>(Read(Need(hooks.deck)(token),0x14c)); }
}
// 이름 저장을 시작한 stream은 State 조회가 current를 바꾸어도 교체하지 않는다.
void SnapshotFortTypeNames(std::int32_t slot,const std::uint32_t& templateOwner,const std::int32_t& typeCount,const FortSnapshotBasicsHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotAssertion::MissingNamesTemplate);
    if (std::bit_cast<std::uint32_t>(slot)!=templateOwner) Need(hooks.assertion)(FortSnapshotAssertion::NamesNotMine);
    const auto stream=Need(hooks.create)(Need(hooks.manager)(slot),"TypeNames");const auto state=Need(hooks.state)(slot);
    if (state.size()<18) throw std::out_of_range("Fort snapshot의 현재 State+17 누락");if (!state[17]) return;
    const std::array<std::uint8_t,1> count{static_cast<std::uint8_t>(typeCount)}; // count는 낮은 BYTE만 저장하고 반복 조건은 현재 signed DWORD다.
    Need(hooks.write)(stream,count);
    for (std::int32_t index=0;index<typeCount;++index) { // Name 조회/진단 뒤 달라진 현재 타입 수를 다음 반복에 반영한다.
        const auto name=Need(hooks.typeName)(index);Append(stream,RiftTypeTable::NameHash(name.substr(0,name.find('\0'))),4,hooks);
    }
}
// owner는 고정 this 슬롯으로 정하고 Refresh/Get 후 현재 manager로 새 Deck 섹션을 만든다.
void SnapshotFortDeck(std::int32_t slot,const FortSnapshotBasicsHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotAssertion::MissingDeckTemplate);
    const auto owner=slot>=1 && slot<=8 ? static_cast<std::uint32_t>(slot) : 0U; // 원문은 Template 배열 밖/0 슬롯을 Player0으로 변환한다.
    Need(hooks.refreshDeck)(owner);const auto token=Need(hooks.getDeck)(owner);const auto stream=Need(hooks.create)(Need(hooks.manager)(slot),"Deck");
    const std::array<std::uint8_t,1> count{static_cast<std::uint8_t>(DeckCount(token,hooks))};Need(hooks.write)(stream,count);
    for (std::int32_t index=0;index<DeckCount(token,hooks);++index) { // 헤더/용량 진단 뒤 현재 count를 다시 읽으며 다섯째 런타임 BYTE는 저장하지 않는다.
        for (std::size_t byte=0;byte<4;++byte) { // 원본은 각 BYTE를 읽은 다음 그 쓰기의 용량 진단을 수행한다.
            const auto source=Need(hooks.deck)(token);const auto offset=static_cast<std::size_t>(index)*5+byte;if (offset>=source.size()) throw std::out_of_range("Fort snapshot Deck 실제 항목 범위");
            Append(stream,source[offset],1,hooks);
        }
    }
}
}
