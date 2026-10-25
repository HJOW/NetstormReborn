// Damageable 삭제 후의 실제 드롭·섬 정리·주변 HP 예약·다리 약화와 공통 post를 연결한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/RawSquidNeighbors.h"
#include "o/SquidReward.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 실제 PostDestroy와 드롭 helper가 매 호출 읽는 원본 전역이다.
struct DamageablePostDestroyState {
    bool battle{}; // 00594fbc/CD00540a20: 주변 HP 피해와 다리 후처리를 허용한다.
    bool boss{true}; // 00540bc4/CD00540a2c: 금 간 다리의 실제 가상 삭제 권한이다.
    std::int32_t dudeCount{}; // 0055a4f8/CD005b72c0:20보다 크면 생성 수를 절반으로 줄인다.
    std::uint32_t silentSurfaceType{147}; // rainBlocker: 일반 삭제의 섬 정리에서 제외한다.
    std::uint32_t damageEvent{6699}; // 원본 Regular event이며 packed 피해/owner는 별도 count에 쓴다.
    std::uint32_t bridgeCrackLife{5}; // 0052f960/CD0051f05c: 약화가 쓰는 life는 이 값−1이다.
    std::array<std::uint32_t,5> excludedDropTypes{133,156,144,138,187}; // patch5개/CD앞4개만 제외하는 현재 타입 전역이다.
    std::uint32_t specialDropType{135}; // 현재 이 타입이면 일반 수량 대신3개를 생성한다.
    const bool* sharedBoss{}; // 원문 다리 후처리가 읽는 같은 Client boss를 빌린다.
    const bool& Boss() const { return sharedBoss ? *sharedBoss : boss; } // 드롭·섬·주변 피해 효과 뒤 현재 권한을 재조회한다.
};
// 해당 하위 몸체가 필요한 경우 성공값/noop으로 바꾸지 않는 필수 효과 연결이다.
struct DamageablePostDestroyHooks {
    std::function<void(Sid,std::int32_t,bool)> dropDude; // 00450c20/CD004e8e00 또는 random=true의00450b30/CD004e8d20 전체 생성 경계다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> dropIsland; // 기존 RawConstructionEffects.DropIsland를 같은 월드에 연결한다.
    std::function<void(Sid,std::uint32_t,float,std::uint32_t)> regular; // 실제 AddRegular(event,payload0) 직후 +20 count에 packed값을 직접 쓴다.
    std::function<bool(Sid)> hasBridgeFall; // 실제 Host의 HasScheduledBridgeFall(0x2692) 조회다.
    std::function<void(Sid)> notifySurface; // 약화 WORD쓰기 직후004214a0/CD00448c10의 실제 공간 통지다.
    std::function<void(Sid)> displayUpdate; // 프레임이 바뀌면 실제 vtable+88을 호출한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 금 간 다리의 실제 가상+10 삭제다.
    std::function<void(Sid,std::uint32_t)> base; // 마지막004b0840/CD004adbe0의 공통 postDestroy다.
};
// HP·중심·일반 탐색·기존 금 간 프레임 검색을 재사용하는 실제 부모 몸체다.
class RawDamageablePostDestroy {
public:
    // 타입/프레임 metadata는 연결 뒤 고정이고 raw 타입·상태·좌표/전역은 각 효과 뒤 다시 읽는다.
    // pool/hash/HP/중심/효과는 같은 월드에 속하며 이 객체보다 오래 살아야 한다.
    RawDamageablePostDestroy(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,const SquidReward& hp,const RawSquidNeighbors& footprint,
        const DamageablePostDestroyState& state,DamageablePostDestroyHooks hooks);
    // 0044b9e0/CD00461b40: 첫 드롭→섬 정리→HP Regular→다리 후처리→공통 post 순서다.
    void PostDestroy(Sid sid,std::uint32_t flags) const;
    // 0044b830/CD00461970: 일반/종류별 수량·제외 타입·현재20명 경계 뒤 실제 하위 생성을 요청한다.
    // 실제 PostDestroy는 random=true를 넘기며 decompiled C의 생략된 셋째 인자를 보정한다.
    void Drops(Sid sid,std::uint32_t genus,bool random) const;
    // 실제 Dais/Altar 어댑터가 같은 풀인지 확인한다.
    const SidPool& Pool() const;
    Sid GenusAt(float x,float y,std::uint32_t genus) const; // plain 절삭한 한 칸의 첫 일반 finder 후보를 조회한다.
    void Weaken(Sid sid) const; // 실제421db0을 더스트 데빌/삭제 파생이 같은 월드에서 공유한다.
private:
    // 현재 유효 SID·타입을 읽고 free/dead를 임의 필터로 추가하지 않는다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    const RiftTypeRecord& Type(Sid sid) const; // 현재 raw 타입 번호의 같은 metadata다.
    std::uint32_t Genus(Sid sid) const; // 현재 TYPE+ec이다.
    bool Ordinary(Sid sid) const; // 원본 extra&9가0인지 확인한다.
    std::array<float,2> Position(Sid sid) const; // 현재 raw float 좌표의 비트다.
    SquidSearchArea Area(Sid sid,std::int32_t radius) const; // 현재 발자국 중심의 거의올림→inclusive 정수 반경이다.
    std::uint8_t FrameFlags(Sid sid) const; // 현재 raw DWORD/BYTE 프레임의 코드 flags다.
    SidPool& pool_; // 실제 부모/후보를 읽고 다리 WORD/프레임을 쓰는 단일 SID 풀이다.
    const SquidHash& hash_; // 현재 일반 finder의4단계 버킷과 raw next 체인이다.
    std::span<const RiftTypeRecord> types_; // 현재 타입 번호로 조회하는 고정 metadata다.
    std::span<const RiftTypeFrames> frames_; // 기존 Bridge::CrackedFrame 검색에 공급하는 같은 코드 표다.
    const SquidReward& hp_; // 이미 복원한 HasHitPoints/MaxHitPoints 몸체다.
    const RawSquidNeighbors& footprint_; // 이미 복원한004adea0/CD004aef30 중심 몸체다.
    const DamageablePostDestroyState& state_; // 원본 현재 전역을 별도 값형 사본으로 만들지 않는다.
    DamageablePostDestroyHooks hooks_; // 원본 하위 생성/예약/가상 삭제/공통 후처리를 공급한다.
};
}
