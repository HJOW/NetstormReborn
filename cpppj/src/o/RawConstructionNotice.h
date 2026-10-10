// 건설 배치 통지(메시지 0x3f, 004441b0 / CD 004d1d00)의 정리·배치·사제 처리 순서를 복원한다.
#pragma once
#include "o/RawConstructionClear.h"
#include "o/RawConstructionConfirm.h"
#include "o/RawConstructionPlace.h"
#include "o/RawPriestState.h"

namespace netstorm::o {
// 배치 이력과 사제 접근 지점의 단정도 좌표다. 이력은 좌표 비트를 그대로 보존한다.
struct ConstructionNoticePoint { float x{},y{}; };
// 통지 처리기가 읽는 전역이다. 주소는 패치 / CD 순서다.
struct ConstructionNoticeState {
    std::uint32_t localPlayer{}; // 00540c70 / 0050f6c8: 통지의 플레이어 바이트와 DWORD로 비교한다.
    bool authority{true};       // 00540bc4 / 00540a2c: 권한 쪽에서만 자동 건설 시작 또는 소유 사제 이동을 처리한다.
    bool moveLocalBuilder{};    // 00544d10 / 00540c4c: 켜져 있으면 로컬 플레이어의 건설 사제도 다시 이동시킨다.
    std::uint32_t daisType{165}; // 005412ec / 0051cbd8: HP 플래그가 없어도 abstract로 놓는 타입이다.
    std::uint32_t priestType{158}; // 005412d0(패치): 사제의 contained 타입 조회에 넘긴다.
    std::array<ConstructionNoticePoint,5> history{}; // 00558ea4 / 00589150: 가장 최근 배치부터 다섯 좌표.
    std::uint32_t historyCounter{}; // 00558e78 / 0053fc40: 로컬 이력이 갱신되면 0으로 초기화한다.
};
// 처리기 밖의 효과다. 공통 경계는 모두 필수이며 contained 두 경계는 패치판에서만 필요하다.
struct ConstructionNoticeHooks {
    // 00441fd0 / 004cff80: 통지가 놓을 칸의 로컬 예측 조각을 먼저 지운다.
    std::function<void(const ConstructionClearRequest&)> clear;
    // 00442c80 / 004d0ca0: 계산한 abstract와 통지의 SID·품질로 조각을 놓는다.
    std::function<void(const ConstructionPlaceRequest&)> place;
    // 00443000 / 004d1120: 플레이어, 첫 SID, double 시각의 하위/상위 DWORD로 건설 사제 이동을 요청한다.
    std::function<void(std::uint32_t,Sid,std::uint32_t,std::uint32_t)> moveBuilder;
    // 004921f0 / 0040d940: 플레이어가 소유한 사제의 SID를 읽는다. 0번 슬롯도 원본처럼 검사할 수 있다.
    std::function<Sid(std::uint32_t)> findPriest;
    // 00488fe0(패치): priestType에 대응하는 contained 타입을 읽는다.
    std::function<std::uint32_t(std::uint32_t)> containedType;
    // 004b1a90(패치): 두 번째 소유 사제 조회의 SID에서 해당 타입의 첫 contained 객체를 삭제한다(원본 인자 0, 1).
    std::function<void(Sid,std::uint32_t)> removeContained;
    // 00427030 / 004e5150: 사제가 현재 이동 불가인지 묻는다.
    std::function<bool(Sid)> immobile;
    // 004ade70→0041db70 / 004aeec0→00440400: 객체 발자국(인자 0)에서 현재 사제 좌표에 가까운 접근점을 찾는다.
    // 플래그는 첫 시도 7, 재시도 3이다. 좌표 거의 올림은 이 경계 뒤에서 처리기 자신이 수행한다.
    std::function<ConstructionNoticePoint(Sid,float,float,std::uint32_t)> approach;
    // 00489080 / 0047d150: 거의 올림한 좌표로 사제 이동을 시도한다. 나머지 원본 인자는 모두 0이다.
    std::function<bool(Sid,float,float,Sid)> tryMove;
    // 00442a40 / 004d1bc0: 이동 실패 시 첫 조각의 현재 타입·소유자 비용을 환불한다.
    std::function<void(std::uint32_t,std::uint32_t)> refund;
    // 가상 +0x10: 환불 뒤 첫 조각을 플래그 0으로 삭제한다.
    std::function<void(Sid,std::uint32_t)> destroy;
    // 00443fe0 / 004d06a0: 자동 건설 대상의 첫 SID로 건설을 시작한다(둘째 인자 0).
    std::function<void(Sid,std::uint32_t)> start;
    // 004da650 / 0041eac0: 정상 처리 끝에 한 번 갱신을 요청한다.
    std::function<void()> refresh;
};
// 메시지 수신과 서버 확정의 직접 처리가 함께 사용하는 통지 실행 계층이다.
class RawConstructionNotice {
public:
    // 풀·타입 표·상태는 더 오래 살아야 한다. 누락된 필수 경계는 첫 효과 전에 거부한다.
    RawConstructionNotice(const SidPool& pool,std::span<const RiftTypeRecord> types,ConstructionNoticeState& state,ConstructionNoticeHooks hooks);
    // 정리 → abstract/사제 분기·로컬 이력 → 배치 → 사제 이동 또는 건설 시작/환불·삭제 → 갱신 순서다.
    // 빈 SID 목록·19개 초과·풀 밖 SID·없는 타입은 효과 없이 거부한다. 원본 발신자 인자 둘은 사용하지 않는다.
    void Handle(const ConstructionNotice& notice) const;
    // 연결할 실제 모듈이 같은 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    // 객체 타입의 flags2(genus)를 현재 raw 슬롯에서 읽는다.
    std::uint32_t Genus(Sid sid) const;
    // 현재 사제 좌표로 접근점을 구해 거의 올림한 다음 이동을 시도한다.
    bool Approach(Sid priest,Sid object,std::uint32_t flags) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    ConstructionNoticeState& state_;
    ConstructionNoticeHooks hooks_;
};
// 정리·배치·환불을 같은 풀의 실제 모듈로 연결한다. priest가 있으면 이동 불가 조회도 실제 모듈로 연결한다.
ConstructionNoticeHooks MakeConstructionNoticeHooks(const SidPool& pool,const RawConstructionClear& clear,RawConstructionPlace& place,
    ConstructionNoticeHooks hooks,const RawPriestState* priest=nullptr);
// 서버 확정의 직접 처리 경계를 같은 풀의 실제 통지 처리기로 연결한다.
ConstructionConfirmHooks MakeConstructionNoticeConfirmHooks(const SidPool& pool,const RawConstructionNotice& notice,ConstructionConfirmHooks hooks);
}
