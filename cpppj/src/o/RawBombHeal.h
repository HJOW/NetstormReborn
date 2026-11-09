// 원문41ebb0/41ebd0의 힐 주문 자격과 현재 대상 상태 복원을 처리한다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include "o/SquidFrame.h" //힐 버스트도 같은 실제 프레임 지정기를 사용한다.
#include "o/SquidProcess.h" //종료 전 실제 event2 Regular 생성/부착을 공유한다.
#include <functional>

namespace netstorm::o { // 패킷/정기 예약과 분리된 실제 대상 가상 함수 본문이다.
//41f9d0의 실제 버스트 정기다. 호출자는 현재 힐 버스트 VT만 분배하며 같은 본문을 공유하는 미티어는 별도 복원 대상이다.
float HandleHealBurstRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,SquidProcessHost& host,const std::function<void(Sid)>& refresh,const std::function<void(Sid,std::uint32_t)>& destroy,Sid sid,std::uint32_t count,float payload);
struct BombHealHooks { // 큰 효과와 현재 전역만 같은 실제 Runtime에서 빌린다.
    std::function<bool(Sid)> hasHitPoints; //004adc60의 현재 HP 보유 여부이며 현재HP 수치 관문이 아니다.
    std::function<std::int32_t(Sid)> maximumHitPoints; //004adcb0 호출 전에 실제 현재HP를 저장한다.
    std::function<std::uint32_t()> fenceType,burstType; //00541278/00541318의 현재 전체 TYPE DWORD다.
    std::function<void(Sid)> fence; //457890의 현재 썬 바리케이드 재연결 부모다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> unpop,repop; //현재 가상48(0)/4c(40)이며 두 VT는 각각 다시 읽는다.
    std::function<void(Sid,std::uint32_t)> refresh; //현재 가상88의 실제 표시/공간 갱신이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530(현재 힐 버스트 TYPE,2)의 같은 실제 공장이다.
    std::function<void(Sid,std::uint32_t,float,float,std::uint32_t)> pop; //생성 뒤 현재 대상 중심과 새 객체의 현재 가상90을 사용한다.
};
class RawBombHeal { //같은 풀/현재TYPE를 읽으며 독립 회복 수치나 임시 World를 만들지 않는다.
public:
    RawBombHeal(SidPool& pool,std::span<const RiftTypeRecord> types,BombHealHooks hooks); //PATCH의 실제188TYPE와 같은 물리 풀을 빌린다.
    std::uint32_t MayTarget(Sid target) const; //41ebb0의 실제81574204 마스크를0/1로 바꾸지 않고 반환한다.
    void Apply(Sid parent,Sid target) const; //QA 제외→HP 평균→상태 해제→펜스/재배치→효과 생성의 순서다.
    const SidPool& Pool() const; //상위 조립 시 다른 저장소 혼용 여부를 확인한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; //비정렬 원문 필드의 지정 폭만 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; //외부 효과 뒤 현재 raw TYPE를 다시 조회한다.
    std::array<float,2> Center(Sid sid) const; //생성 뒤 실제4adea0 발자국 중심을float에 저장한다.
    SidPool& pool_; //현재HP/extra를 직접 쓰는 같은 물리50BYTE 저장소다.
    std::span<const RiftTypeRecord> types_; //HP 자격/표시/중심에 쓰는 실제 현재 TYPE 자료다.
    BombHealHooks hooks_; //선택한 큰 하위만 요구하며 누락을 빈 성공으로 숨기지 않는다.
};
}
