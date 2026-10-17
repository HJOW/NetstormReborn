// 건설용 사제와 자원 운반 객체를 작업장/공급원에서 생성해 목표로 보낸다.
#pragma once
#include "o/RawConstructionLocal.h"
#include "o/RawConstructionSelection.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
class RawEnergySupply; // 실제 Session의 단일 에너지 판정/정렬/선택 소비를 참조한다.
// 원본 타입 +0xe0/+0x98/+0x130의 이 흐름 전용 메타 자료다. 기본 타입 표를 확장하지 않는다.
struct ConstructionBuilderType {
    float speed{}; // 첫 건설 사제의 도착 시각을 조정할 때 읽는 현재 타입 속도다.
    std::uint32_t sourceFrameIndex{}; // 공급원 타입 +0x98: 운반 객체의 첫 프레임 표 인덱스다.
    std::array<std::int32_t,16> firstFrames{}; // 타입 +0x130의 프레임 DWORD 16개다. CD는 최종 BYTE만 쓴다.
};
// 경로 프로세스의 현재 단계와 단계별 도착 시각을 호스트 자료로 표현한다.
struct ConstructionBuilderPath {
    std::uint32_t currentStep{}; // 원본 프로세스 +0x10의 현재 시각 인덱스다.
    std::vector<double> arrivalTimes; // 패치 +0x9c50 / CD +0x12d0의 시각 배열이다.
};
// 생성/자원 선택 경계 뒤에도 다시 읽는 전역과 공유 공급원 목록이다.
struct ConstructionBuilderState {
    std::uint32_t builderType{}; // 00541270 / CD 0051cb5c: 건설용 객체 타입. 호출자가 실제 전역을 공급한다.
    std::uint32_t carrierType{}; // 005411fc / CD 0051cae8: 자원 운반 객체 타입. 호출자가 실제 전역을 공급한다.
    std::uint32_t bridgeType{82}; // 005411a0 / CD 0051ca8c: 단어 쓰기 helper가 읽는 현재 다리 타입이다.
    std::uint32_t approachMask{}; // 005424b8 / CD 005395cc: 기준점 genus와 겹치면 mask|1로 접근한다.
    float timerPayload{0.05F}; // 00532750 / CD 0052da30: event0으로 예약하는 운반 객체 Regular의 현재 float payload다.
    double now{}; // 0055b4d0 / CD 005484a8: 속도 조정의 기준 게임 시각이다.
    SquidPostPopList sources; // 00592f4c/54 / CD 대응 전역: 선택/정렬 뒤 소비하는 공유 DWORD 목록이다.
};
// 경로·자원 선택·가상 공간·할당의 하위 효과를 명시적으로 연결한다.
struct ConstructionBuilderHooks {
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> anchor; // 실제 작업장/그래프 조회다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // factory(type,2): 클라이언트 영역의 초기화된 객체다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 가상 +0x74. 최초 요청 DWORD/운반 기준점 BYTE를 구분한다.
    std::function<ConstructionNoticePoint(const std::array<float,4>&,float,float,std::uint32_t)> approach; // 기준점 사각형(left/top/right/bottom)의 접근점 조회다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 가상 +0x90. 첫 객체는 0x10, 운반 객체는 0이다.
    std::function<ConstructionBuilderPath*(Sid,float,float)> move; // 실제 경로 시작 경계. 실패는 null, 반환 객체는 Move 완료까지 유효해야 한다.
    std::function<void(ConstructionBuilderPath&,float,std::uint32_t)> speed; // 경로 가상 +0x30에 속도/0을 전달한다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> chooseSources; // Mana 요구 계산. 공유 sources를 준비한다.
    std::function<void(float,float)> sortSources; // 현재 목표 좌표에서 공유 sources의 우선순위를 정한다.
    std::function<void(Sid)> notifySurface; // 단어를 쓴 운반 객체가 현재 bridgeType이면 표면 알림을 요청한다.
    std::function<bool()> reserveTimer; // 원본 new(0x28)의 성공 여부다. 실패해도 이동은 계속한다.
    std::function<void(Sid,std::uint32_t,float)> addTimer; // 확보 성공 뒤 현재 이벤트로 Regular를 만든다.
};
// 실제 Session은 같은 타입/시계 전역을 읽고, 독립 원문 검사는 기존 값형 State를 사용한다.
struct ConstructionBuilderReads {
    std::function<std::uint32_t()> builderType,carrierType,bridgeType,approachMask; // 현재541270/5411fc/5411a0/5424b8이다.
    std::function<float()> timerPayload; // 원본532750의 예약 payload다.
    std::function<double()> now; // 같은55b4d0 게임 시각이며 사본을 매 프레임 복사하지 않는다.
};
// 00443000 / CD 004d1120 전체의 생성·배치·경로·공급원 소비 순서를 복원한다.
class RawConstructionBuilder {
public:
    // 풀·타입·메타·상태와 훅의 참조 대상은 객체보다 오래 살아야 한다.
    RawConstructionBuilder(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const ConstructionBuilderType> metadata,
        ConstructionBuilderState& state,ConstructionBuilderHooks hooks,const RawEnergySupply* supply=nullptr,ConstructionBuilderReads reads={});
    // 숨은 double 결과 포인터를 반환값으로 표현한다. timeLow/High는 속도 조정 목표 시각의 원래 비트다.
    // 기준점/첫 경로가 없으면 5.0이며, 실패 이전에 생성한 객체를 되돌리지 않는다.
    double Move(std::uint32_t player,Sid target,std::uint32_t timeLow,std::uint32_t timeHigh) const;
    // 00473570 / CD 0048aaf0: 첫 0이 아닌 공급원 DWORD를 0으로 바꾸어 소비한다. 없으면 0이다.
    std::uint32_t TakeSource() const;
    // 어댑터에서 실제 월드가 같은지 검사한다.
    const SidPool& Pool() const;
private:
    // 실제 접근 전에 DWORD SID가 풀 범위 안인지 확인한다.
    Sid CheckedSid(std::uint32_t sid) const;
    // 현재 raw 타입의 발자국/genus 자료를 조회한다.
    const RiftTypeRecord& Type(Sid sid) const;
    // 현재 raw 타입의 속도/프레임 표를 조회한다.
    const ConstructionBuilderType& Metadata(Sid sid) const;
    // 현재 좌표/발자국으로 left/top/right/bottom 사각형을 만든다.
    std::array<float,4> Footprint(Sid sid) const;
    // 목표 발자국의 위쪽 가운데를 원본 방식으로 반올림한다.
    ConstructionNoticePoint Destination(Sid sid) const;
    // 공급원 좌표에서 발자국 크기의 절반을 뺀다.
    ConstructionNoticePoint Center(Sid sid) const;
    // 현재 경로 단계의 도착 시각을 읽는다.
    double Arrival(const ConstructionBuilderPath& path) const;
    // 플래그 2로 생성하고 실제 풀의 새 SID인지 확인한다.
    Sid Create(std::uint32_t type) const;
    SidPool& pool_; // 객체 생성과 운반 단어/프레임을 쓰는 실제 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 객체 타입의 발자국/genus를 재조회한다.
    std::span<const ConstructionBuilderType> metadata_; // 현재 타입의 속도/프레임 표를 재조회한다.
    ConstructionBuilderState& state_; // 경계가 바꾼 현재 전역과 공급원 목록을 공유한다.
    ConstructionBuilderHooks hooks_; // 아직 복원하지 않은 하위 효과다.
    const RawEnergySupply* supply_{}; // 연결됐으면 별도 state.sources/선택 훅 대신 같은 실제 마지막 목록을 사용한다.
    ConstructionBuilderReads reads_; // 실제 전역 조회 위치에서만 호출하는 현재 getter다.
};
// 같은 풀의 실제 기준점/factory/일반 소유자 지정과 선택적으로 Regular/Kernel를 연결한다.
ConstructionBuilderHooks MakeConstructionBuilderHooks(const SidPool& pool,const RawPlayerPlacementAnchor& anchor,
    SquidFactory& factory,SquidOwner& owner,ConstructionBuilderHooks hooks,SquidProcessHost* process=nullptr);
// double 반환값을 사용하지 않는 로컬 배치/통지 경계를 원본 전체 Move에 연결한다.
ConstructionLocalHooks MakeConstructionBuilderLocalHooks(const SidPool& pool,const RawConstructionBuilder& builder,ConstructionLocalHooks hooks);
ConstructionNoticeHooks MakeConstructionBuilderNoticeHooks(const SidPool& pool,const RawConstructionBuilder& builder,ConstructionNoticeHooks hooks);
}
