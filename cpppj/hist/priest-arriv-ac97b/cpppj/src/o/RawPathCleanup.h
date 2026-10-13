// PathProcess 삭제 통지의 공간 접두 뒤 실제 예약·운반 nugget·신전 의식·건설 helper 정리를 복원한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/RawSquidFinder.h"
#include <functional>

namespace netstorm::o {
class RawCarrierPreDestroy; // 기존 contained WORD 조건 조회를 재사용한다.
class RawPlayerPlacementAnchor; // 기존 실제 raw GraphFor를 재사용한다.
class RawConstructionPlace; // 기존 판본별 Refund를 재사용한다.
class SquidProcessHost; // 같은 Kernel의 실제 SharedRegular 예약을 재사용한다.
// 공간 접두 뒤에도 현재 값을 다시 읽는 cleanup 전역이다.
struct PathCleanupState {
    bool boss{true},checking{}; // 기존 ProcessHost와 같은 권한/패치 assert 상태다.
    std::uint32_t priestType{158},daisType{165},outpostType{126},nuggetType{154}; // 부모/근처 의식 장소/운반 반환 장소/contained kind의 현재 타입 번호다.
    std::uint32_t helperEvent{0x1a2c}; // 양판본 보조 건설물에 지연1을 요청하는 SharedRegular 이벤트 번호다.
};
// 수명/예약과 아직 별도로 복원할 신전 의식 하위 몸체다. 조회는 실제 기존 모듈을 공급한다.
struct PathCleanupHooks {
    std::function<bool()> reserveShared; // 원본 new(0x28)의 성공/실패다. 실패해도 인접 helper는0으로 지운다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 실제 같은 Host.AddSharedRegular(helper,event,1.0f)다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> contained; // RawCarrierPreDestroy::FindContained의 현재 mask/kind/index 조회다.
    std::function<std::uint32_t(Sid)> graphFor; // Graph pointer0 여부/동일성에 쓰는 기존 실제 raw GraphFor다.
    std::function<void(Sid,Sid,std::uint32_t)> ritual; // 신전00447e40/CD00481fc0. 실제 하위 구현 연결 전에는 실행 경계다.
    std::function<void(std::uint32_t,std::uint32_t)> refund; // 실제 RawConstructionPlace::Refund(type,owner)다.
    std::function<void(Sid,std::uint32_t)> destroy; // nugget/helper의 실제 가상+0x10(flags0)이다.
};
// 부모 공간 접두는 기존 RawPathProcess::OnFormDestroy가 수행하고 이 객체는 그 다음 몸체만 실행한다.
class RawPathCleanup {
public:
    // 풀/hash/타입/상태/연결 대상은 처리기보다 오래 살아야 한다. 모든 하위 효과는 필수다.
    RawPathCleanup(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,PathCleanupState& state,PathCleanupHooks hooks);
    // 패치0048aa00/CD0047f0a0의 공간 접두 이후 본문이다. 현재 runtime helper와 부모 raw 슬롯을 다시 읽는다.
    void AfterSpatial(RawPathRuntime& runtime,Sid parent) const;
    // 004263a0/CD004e3d70: 좌표±정수 radius를 절삭한 사각형에서 abstract/dead를 제외한 genus0x200 또는 outpost를 찾는다.
    Sid ReturnPlaceAt(float x,float y,std::int32_t radius=1) const;
    // 004b21d0/CD004eb7b0: 현재 타입의 genus로 finder 시작 단계를 고른 뒤 거리2의 첫 같은 타입을 반환한다.
    Sid NearbyType(float x,float y,std::uint32_t radius,std::uint32_t type) const;
    // 실제 월드 연결이 같은 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    // 판본별 슬롯의 현재 genus·owner·extra를 읽는다. helper의 free 상태를 자동으로 걸러내지 않는다.
    std::uint32_t Genus(Sid sid) const;
    std::uint8_t Owner(Sid sid) const;
    std::uint8_t Extra(Sid sid) const;
    // 원본 float+거의올림 bias의 __ftol 하위 DWORD를 보존한다.
    static std::int32_t Cell(float coordinate,bool biased);
    // 원본 CRT의 signed64 절삭 뒤 하위 DWORD만 돌려준다.
    static std::int32_t Truncate(double value);
    // 비정렬 DWORD 좌표를 해석한다.
    static float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset);
    const SidPool& pool_; // 부모/보조 객체/contained를 저장하는 유일한 raw 풀이다.
    const SquidHash& hash_; // 현재 실제 일반 finder의 네 단계 머리다.
    std::span<const RiftTypeRecord> types_; // 현재 helper 부류·반환 장소·신전 검색 속성이다.
    PathCleanupState& state_; // callback 뒤에도 현재 권한/타입/이벤트를 읽는다.
    PathCleanupHooks hooks_; // 같은 월드의 실제 조회/예약/환불/가상 삭제/의식이다.
};
// 기존 실제 contained·graph·공유예약·환불 몸체를 같은 풀에 연결하고 의식/삭제/할당 경계를 보존한다.
PathCleanupHooks MakePathCleanupHooks(const SidPool& pool,const RawCarrierPreDestroy& carrier,
    const RawPlayerPlacementAnchor& anchor,SquidProcessHost& host,RawConstructionPlace& place,PathCleanupHooks hooks);
}
