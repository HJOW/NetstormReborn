// 초기 자산의 실제 Handle를 같은 raw 풀·프레임 지정기·게임 난수에 연결한다.
#pragma once
#include "o/RawAssetPostPop.h"
#include "o/RawFrameAdvance.h"
#include "o/RawSquidFinder.h"
#include "o/SpStore.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 레지던스·가이저·신전 Handle가 읽고 갱신하는 실제 전역이다. 신전 목록은 postPop 저장소 자체를 공유한다.
struct AssetRegularState {
    bool fortMode{}; // 신전의 편집 모드 역방향 프레임 진행을 선택한다.
    bool boss{true}; // 신전 이탈 사건의 3회/23회 대기를 선택한다.
    bool suppressLeaveRequest{}; // 신전 이탈 패킷 생성을 억제하는 원본 전역이다.
    bool checkingForms{}; // event4가 count4 이후 대기를 생략하는 진단 전역이다.
    bool themeMode{true}; // 독립 호출의 고정 옵션 입력이다. 실제 캠페인은 hooks.themeMode로 현재 설정을 읽는다.
    std::uint32_t decorationType{102}; // event1이 탐색하는 실제 장식 타입 전역이다. 두 PE 초기값은102다.
    std::uint32_t maxDecorationScan{}; // 원본 전역에 저장하는 한 번의 finder 최대 조사 개수다.
    std::uint32_t damageEvent{6699},constructionEvent{6700}; // Damageable의 폭발 연쇄/abstract 건설 재시도 이벤트 번호다.
    std::uint32_t constructionProcessType{30}; // abstract 건설 재시도가 찾는 부착 프로세스 번호다.
    std::uint32_t animationType{0x2000013}; // 신전 event4의 삭제 통지 대상 토큰이다. 두 PE 초기DWORD는0x2000013이다.
    std::uint32_t regionGeneration{}; // event4 통지 토큰의 상위 WORD를 만드는 전역이다.
};
// 아직 독립 하위 모듈로 연결해야 하는 효과다. 누락 시 해당 사건을 거부하며 무효 반환으로 대신하지 않는다.
struct AssetRegularHooks {
    std::function<std::uint32_t()> themeMode; // 레지던스의 일치 신전 조회 뒤 현재 theammode를 읽는다. 미연결 독립 호출은 state 값을 쓴다.
    std::function<void(Sid)> notifySurface; // 주민 변화 뒤 실제 004214a0 연결 재평가를 호출한다.
    std::function<Sid(float,float)> surface; // 레지던스 아래의 실제 표면 SID를 조회한다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 가상 소유자 지정의 실제 파생 분배를 호출한다.
    std::function<void(Sid)> residenceEvent2; // 레지던스 event2의 실제 00450a90/CD004e8cb0을 실행한다.
    std::function<void(Sid,std::uint32_t)> displayUpdate; // 가이저의 가상 +88 표시 갱신이다.
    std::function<bool(Sid,std::uint32_t)> ascendTick; // 신전 event0의 004db8e0/CD0044ee30 섬 자산 삭제 진행 몸체다.
    std::function<void(std::uint32_t)> playerLeavingNotice; // 신전 상태2 설정 뒤 실제 이름/채팅 알림을 표시한다.
    std::function<std::uint32_t(std::uint32_t)> countForeignOwnedIslands; // 신전 event3의 현재 플레이어와 다른 소유자 섬 개수 조회다. 반환은 원본처럼 버린다.
    std::function<std::uint32_t(float,float)> islandNumber; // 신전 이탈 패킷의 섬 번호 BYTE를 조회한다.
    std::function<void(Sid,std::uint32_t)> requestLeave; // type9/flag0/섬·신전·1 payload를 실제 송신 경로에 넘긴다.
    std::function<std::uint32_t(float,float)> regionNumber; // event4의 지역 번호를 조회한다.
    std::function<void(std::uint32_t,std::uint32_t)> removeAnimation; // animationType와 동일한 두 통지 토큰을 처리한다.
    std::function<void(float,float)> damageGround; // 피해 시 snap→Dude 통지→공유 난수100 소비→시각 효과의 실제00470ac0 사슬이다.
    std::function<std::int32_t(Sid,std::uint32_t,std::int32_t,std::uint32_t)> hitPoints; // 실제 가상 +98 피해/HP 처리다.
    std::function<Sid(Sid,std::uint32_t)> findAttached; // constructionProcessType의 현재 부착 프로세스를 조회한다.
    std::function<void(Sid,float)> startConstruction; // 건설 프로세스가 없는 abstract 객체의 실제 RawConstructionProcess::Start(parent,0)다.
};
// +5c 가상 표의 실제 주소로 몸체를 선택하고 공통 Damageable의 event 분배까지 복원한다.
class RawAssetRegular {
public:
    // 풀/해시/표/공유 난수/신전 목록은 이 객체보다 오래 살아야 한다. 지정기·진행기는 같은 풀이어야 한다.
    RawAssetRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<const std::int32_t> elements,
        const SquidFrame& setter,const RawFrameAdvance& advance,GameRandom& random,
        AssetPostPopState& assets,AssetRegularState& state,AssetRegularHooks hooks);
    // 레지던스/가이저/신전/Factory의 실제 가상 표를 확인한다.
    bool Handles(Sid sid) const;
    // 해당 부모의 실제 사건을 실행한다. 최초 float는 지연이 아닌 payload이며 시각 예약은 ProcessHost가 담당한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 이미 복원된 사제 등과 합성할 처리기다. 미복원 부모를 받아주는 기본 payload 처리는 넣지 않는다.
    RegularHandler Handler(RegularHandler fallback={}) const;
    // 공통 피해 연쇄/abstract 건설 재시도 분배를 다른 Damageable 파생에서 직접 재사용한다.
    float DamageableHandle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 어댑터가 다른 월드의 풀을 섞지 않게 확인한다.
    const SidPool& Pool() const;
private:
    // Factory의 실제 사건2 프레임 난수와 공통 Damageable 위임이다. 큰 upgrade 사건은 명시 미복원이다.
    float Factory(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 실제 가상 표에서 레지던스1/가이저2/신전3을 구한다.
    unsigned Kind(Sid sid) const;
    // 판본별 현재 프레임 DWORD/BYTE를 읽는다.
    std::int32_t CurrentFrame(Sid sid) const;
    // 레지던스 주민/장식/지면 테마 사건을 처리한다.
    float Residence(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 가이저의 실제 프레임과 남은 결정량별 반환값을 처리한다.
    float Geyser(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 신전 Ascend 정리/애니메이션/이탈/통지 사건을 처리한다.
    float Vortex(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    SidPool& pool_; // 실행 부모와 이웃을 다시 조회하는 단일 raw 풀이다.
    const SquidHash& hash_; // 레지던스 장식 finder의 실제 동적 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 비용/genus를 읽는 공유 타입 표다.
    std::span<const RiftTypeFrames> frames_; // 현재 프레임의 signed flags/글자 구간을 읽는 표다.
    std::span<const std::int32_t> elements_; // 원본 타입 +98의 현재 원소 값이다.
    const SquidFrame& setter_; // 실제 표시/해시/공간 효과를 보존하는 프레임 지정기다.
    const RawFrameAdvance& advance_; // 실제 한 번 감싸기 프레임 진행기다.
    GameRandom& random_; // 모든 자산 사건이 소비하는 같은 난수 상태다.
    AssetPostPopState& assets_; // postPop와 동일한 신전 목록이다.
    AssetRegularState& state_; // 현재 모드·통지·최대 finder 조사 전역이다.
    AssetRegularHooks hooks_; // 위에 명시한 실제 외부 효과 경계다.
};
}
