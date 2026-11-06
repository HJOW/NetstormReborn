// 원본004a7350/004a6d00의 참가자별 월드 동기화 순서를 같은 Pool/전송 전역에 적용한다.
#pragma once
#include "o/RawSquidTransmission.h"

namespace netstorm::o { // 네트워크 장치는 기존 전송기가 소유하고 이 부모는 현재 월드 순회만 수행한다.
struct PeerWorldSyncHooks {
    std::function<std::uint32_t()> sharedRegularType; //00541164의 현재 전체 DWORD를 매 클라이언트 Form 후보에서 읽는다.
    std::function<void(Sid,std::uint32_t)> unpop,repop; // walker의 현재 가상+48/+4c에 원문 인자0을 전달한다.
};
class RawPeerWorldSync {
public:
    RawPeerWorldSync(const SidPool& pool,std::span<const RiftTypeRecord> types,RawSquidTransmissionState& state,RawSquidTransmission& transmission,PeerWorldSyncHooks hooks); // 다섯 소유자는 동기 순회가 끝날 때까지 살아 있어야 한다.
    void Synchronize(std::uint32_t owner) const; // 지형→그 외 서버 객체/종속→walker 재등록을 수행한다. 예외 때 원문 전역을 임의 정상화하지 않는다.
private:
    void Children(Sid parent,std::uint32_t depth) const; // 현재 child+6/sibling+4를 재조회하며 서버 종속과 허용된 클라이언트 공유 Regular를 보낸다.
    const SidPool& pool_; // 실제 단일 SID 표이며 복사 snapshot으로 순회를 대체하지 않는다.
    std::span<const RiftTypeRecord> types_; // 같은 월드의 변경 가능한 TYPE 레코드를 읽기 참조한다.
    RawSquidTransmissionState& state_; // 실제 clientSidAllowed/syncTarget/압축 owner 캐시다.
    RawSquidTransmission& transmission_; // 현재 writer/대상 연결 관문/송신을 제공한다.
    PeerWorldSyncHooks hooks_; // 실제 현재 공유 TYPE와 walker 재등록 장치다.
};
}
