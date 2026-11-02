// 일반 MakeAi의 원문 설정/전체 객체 이전과 삭제를 같은 현재 메모리·기존 부모로 연결한다.
#include "o/RawMakeAi.h"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o { // host 주소나 복제된 게임 상태를 원문 DWORD로 가장하지 않는다.
namespace { // 선택된 효과의 지연 검증과 정렬 없는 원문 물리 접근이다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("MakeAi 실제 현재 부모 미연결");return hook; } // 도달하지 않은 분기는 불필요한 장치를 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) { // 실제 필드 폭만 조합한다.
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("MakeAi 읽기 물리 범위");std::uint32_t value{};
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(8*i); // 정렬/포인터 크기와 무관한 little endian이다.
    return value;
}
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { // 주변 미작성 BYTE를 건드리지 않는다.
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("MakeAi 쓰기 물리 범위");
    for (std::size_t i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 실제 DWORD 네 BYTE만 기록한다.
}
std::uint32_t Ftol(float value) { // 실제_ftol의 signed64 절삭 하위DWORD이며 indefinite 하위도0이다.
    if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
std::uint32_t Next(std::uint32_t index,std::uint32_t limit,const MakeAiObjectHooks& hooks) { //40eb40은 먼저증가하고 진입당시 limit안에서 free만 건너뛴다.
    ++index;while (index<limit && (Read(Need(hooks.slot)(index),11,1)&1U)) ++index;return index; // dead/void/buried에 새로운 관문을 추가하지 않는다.
}
bool Valid(std::uint32_t index,const MakeAiObjectHooks& hooks) { return index>=1 && index<Need(hooks.capacity)(); } // 본문의 현재 풀 상한은 iterator 고정 limit와 다르다.
std::array<float,2> Position(std::uint32_t index,const MakeAiObjectHooks& hooks) { const auto bytes=Need(hooks.slot)(index);return {std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18))}; } // 같은 현재 슬롯의 좌표를 callback 뒤 재조회한다.
}
// battle0이면 설정/저장소에 접근하지 않으며 Initialize 뒤에만 현재missionPost를 읽는다.
void MakeAi(std::uint32_t owner,std::span<std::uint8_t> settings,const MakeAiReads& reads,const MakeAiHooks& hooks) {
    if (!Need(reads.battle)()) return;if (settings.size()!=RawMissionAiSettings::kSize) throw std::invalid_argument("MakeAi 실제600BYTE 설정 필요");
    Need(hooks.initialize)(settings);
    if (!Need(reads.missionPost)()) { Put(settings,0x220,0);Put(settings,0x224,0x3f800000U);Put(settings,0x228,0x40000000U);Put(settings,0x230,3);Put(settings,0x234,6); }
    if (!owner) {
        const auto follower=Need(reads.localOwner)()+1U;if (Need(reads.brain)(follower)) return;
        Need(hooks.transferObjects)(follower);static_cast<void>(Need(hooks.create)(follower,settings));Need(hooks.clearObjects)(0x2000U);return;
    }
    const auto known=Need(reads.playerKnown)();settings[0x1e]=0;const auto sp=known ? Need(hooks.stormPower)(Need(reads.localOwner)()) : 0.0F;Put(settings,0x22c,Ftol(sp));
    static_cast<void>(Need(hooks.create)(owner,settings));
}
// local 첫region만 진입 때 캡처하고 owner/TYPE/flags/슬롯은 각 원문 위치에서 다시 읽는다.
void TransferMakeAiObjects(std::uint32_t owner,const MakeAiObjectHooks& h) {
    const auto limit=Need(h.capacity)();auto index=Next(1,limit,h);const auto localRegion=Read(Need(h.players)(),Need(h.localOwner)()*180U+0x78);
    while (Valid(index,h)) { // 원문 pool은 효과 뒤 재조회하며 fixed iterator 상한으로 free만 건너뛴다.
        const auto objectOwner=Read(Need(h.slot)(index),34,1);
        if ((objectOwner==Need(h.localOwner)() || !objectOwner) && !(Read(Need(h.slot)(index),11,1)&8U) && Need(h.flags)(index)) {
            const auto point=Position(index,h);
            if (Need(h.region)(point[0],point[1])!=localRegion) {
                const auto type=Read(Need(h.slot)(index),10,1);
                if (type!=Need(h.playerType)() && type!=Need(h.bridgeType)()) {
                    if (Read(Need(h.slot)(index),34,1)==Need(h.localOwner)()) { Need(h.decrementLocal)(type);Need(h.decrementBuilt)(Read(Need(h.slot)(index),10,1)); }
                    Need(h.setOwner)(index,owner);const auto flags=Need(h.flags)(index);
                    if (flags&0x200000U) { ++h.loadingDepth;if (h.loadingDepthChanged) h.loadingDepthChanged(h.loadingDepth);Need(h.setOwner)(index,owner);--h.loadingDepth;if (h.loadingDepthChanged) h.loadingDepthChanged(h.loadingDepth); }
                    if (flags&0x200U) { auto players=Need(h.players)();const auto position=Position(index,h);Put(players,owner*180U+0x78,Need(h.region)(position[0],position[1])); }
                }
            }
        }
        index=Next(index,limit,h);
    }
}
// 다음 후보는flags조회/가상삭제보다 앞서 찾으며 삭제가 슬롯을 바꿔도 이번 다음 번호는 유지한다.
void ClearMakeAiObjects(std::uint32_t mask,const MakeAiObjectHooks& h) {
    const auto limit=Need(h.capacity)();auto index=Next(1,limit,h);
    while (Valid(index,h)) { // 현재 상한은 매번 다시 읽고 다음 free 건너뛰기에는 진입 limit를 쓴다.
        const auto current=index;index=Next(index,limit,h);if (Need(h.flags)(current)&mask) Need(h.destroy)(current,0);
    }
}
}
