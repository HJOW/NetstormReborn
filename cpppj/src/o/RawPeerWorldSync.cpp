// 고정 서버 번호 범위와 현재 타입/종속 목록을 사용하는 원문 동기화 부모다.
#include "o/RawPeerWorldSync.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 호스트 물리 범위 실패와 원문의 정상 조건을 구별한다.
namespace { // 비정렬 종속 WORD와 선택된 장치의 현재 값을 읽는다.
std::uint16_t SyncWord(std::span<const std::uint8_t> bytes,std::size_t offset) { return static_cast<std::uint16_t>(bytes[offset]|std::uint16_t(bytes[offset+1])<<8); } // 지정 WORD만 little endian으로 읽는다.
template<class F> F SyncNeed(const F& hook) { if (!hook) throw std::logic_error("월드 동기화의 선택된 실제 장치 미연결");return hook; } // callback 교체 중에도 현재 callable의 수명을 보유한다.
constexpr std::uint32_t firstServer=15000,lastServer=23000; //004a7350의 두 순회는 현재 전체 capacity 대신 이 고정 양끝을 포함한다.
}
RawPeerWorldSync::RawPeerWorldSync(const SidPool& pool,std::span<const RiftTypeRecord> types,RawSquidTransmissionState& state,RawSquidTransmission& transmission,PeerWorldSyncHooks hooks)
    :pool_(pool),types_(types),state_(state),transmission_(transmission),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재 월드 동기화는10.78 전용"); } // CD004c15e0의 별도 SID 경계는 추후 판본 작업으로 남긴다.
// 자식 전송 뒤의 실제 sibling/child를 다시 읽는다. 잘못된 순환은 호스트 물리 안전 경계로 거부한다.
void RawPeerWorldSync::Children(Sid parent,std::uint32_t depth) const {
    if (depth>pool_.Capacity()) throw std::out_of_range("동기화 종속 깊이의 실제 Pool 범위");auto child=SyncWord(pool_.Slot(parent),6);std::uint32_t visited{};
    while (child) { // 같은 현재 부모의 child/sibling 사슬만 순회하며 임의 owner/free 관문을 추가하지 않는다.
        if (++visited>pool_.Capacity()) throw std::out_of_range("동기화 종속 sibling 순환");const Sid sid{child};
        if (child>=firstServer && child<=lastServer) {
            transmission_.SendTo(sid,std::bit_cast<std::int32_t>(state_.syncTarget),1);
            if (SyncWord(pool_.Slot(sid),6)) Children(sid,depth+1);
        } else if (child>=5 && child<firstServer && pool_.Slot(sid)[10]<70 && pool_.Slot(sid)[10]==SyncNeed(hooks_.sharedRegularType)()) transmission_.SendTo(sid,std::bit_cast<std::int32_t>(state_.syncTarget),1);
        child=SyncWord(pool_.Slot(sid),4);
    }
}
// 실제 세 단계와 전역 쓰기 순서를 보존한다. 마지막 순회는 프로세스 송신이 아니라 walker의 공간 재등록이다.
void RawPeerWorldSync::Synchronize(std::uint32_t owner) const {
    state_.clientSidAllowed=1;transmission_.SameOwnerMode(1);state_.syncTarget=owner;
    for (const bool surface:{true,false}) { // TYPE.flags1의 surface 비트가 있는 서버 객체를 먼저 보내고 나머지를 뒤에 보낸다.
        for (std::uint32_t number=firstServer;number<=lastServer;++number) { // 원문15000..23000을 각각 한 번 읽고 현재 free/TYPE 조건만 적용한다.
            const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=pool_.Slot(sid);if ((raw[11]&1U) || raw[10]<=69) continue;
            if (raw[10]>=types_.size()) throw std::out_of_range("동기화 현재 TYPE 범위");if (bool(types_[raw[10]].flags1&0x800U)!=surface) continue;
            transmission_.SendTo(sid,std::bit_cast<std::int32_t>(state_.syncTarget),1);Children(sid,0); // selector=-3의 일반+54는 fourth flags를 읽지 않는다. 기계어004ac2a6..004ac32e 대조.
        }
    }
    transmission_.SameOwnerMode(0);const auto limit=pool_.Capacity();std::uint32_t number=2;
    while (number<limit && (pool_.Slot(Sid{static_cast<std::uint16_t>(number)})[11]&1U)) ++number; // 처음 살아 있는 슬롯은 원문처럼2번에서 찾는다.
    for (;;) { // 현재 Pool 끝을 매번 확인하고 최초 capacity는 free 건너뛰기의 원문 상한으로만 사용한다.
        if (number<1 || number>=pool_.Capacity()) { state_.clientSidAllowed=0;return; }
        const Sid sid{static_cast<std::uint16_t>(number)};const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("동기화 walker의 현재 TYPE 범위");
        if (types_[type].flags2&0x10000U) { SyncNeed(hooks_.unpop)(sid,0);SyncNeed(hooks_.repop)(sid,0); }
        ++number;while (number<limit && (pool_.Slot(Sid{static_cast<std::uint16_t>(number)})[11]&1U)) ++number; // 두 가상 호출이 끝난 뒤 다음 현재 free 상태를 읽는다.
    }
}
}
