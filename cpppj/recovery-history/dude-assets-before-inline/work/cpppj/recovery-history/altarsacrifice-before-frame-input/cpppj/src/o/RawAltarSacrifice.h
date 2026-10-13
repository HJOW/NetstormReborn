// 알터20~24 사건과 다이스의 희생 완료·사제 해방·삭제 전처리를 같은 raw 월드에 연결한다.
#pragma once
#include "o/RawDaisRegular.h"
#include "o/RawDamageablePreDestroy.h"
#include "o/RawFrameAdvance.h"

namespace netstorm::o {
class SquidFactory; // 해방 사제의 기존 실제 생성자를 사용한다.
inline constexpr std::uint32_t kPatchAltarVtable=0x005076b8,kCdAltarVtable=0x00504200; // 실제164 최종 가상표의+5c는0044a5e0/CD00483800이며+80은0044a480/CD00483670이다.
// 원본의 조절 가능한 사제 저항 주기/확률 전역이다.
struct AltarSacrificeState {
    float struggleSeconds{3.0F}; // patch00532698/CD0052e344의 현재 지연이다.
    std::int32_t escapePercent{40}; // patch0053269c/CD0052e348: 0..100000 난수/1000과 비교한다.
};
// 파생 가상 공간/삭제 및 실제 UI·음향·난수 경계다. 빈 효과는 생성 단계에서 거부한다.
struct AltarSacrificeHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 같은 실제 SquidFrame의 지정이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 파생+10 분배다. Dais400000/포획사제8/altar48을 구분한다.
    std::function<bool()> reserveShared; // 매21회 new0x28 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 같은 Host의20/22 SharedRegular 예약이다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInt; // 현재 게임 전역의 실제 범위 난수다. 소비 순서를 유지한다.
    std::function<float(float,float)> randomFloat; // 현재 게임 전역의 실제 실수 난수다.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt; // 실제 위치 소리다. 저항 소리는 원본의 마지막 flags0을 공급한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> playGlobal; // priestFree 및 CD 추가 enemyPriestEsacping의 원본6인수다.
    std::function<void(Sid)> removeAnimation; // 원본004605d0/CD004547d0의 실제 애니메이션 삭제다.
    std::function<void(Sid,std::uint32_t)> unpop; // 알터 가상+48 flags0이다.
    std::function<void(Sid,std::uint32_t)> repop; // 알터 가상+4c flags0이다. plain frame 쓰기 이후 실행한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 기존 Factory로 현재 priestType을 flags0 생성한다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 해방 사제 가상+74의 owner0이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 실제 전체 사제+90 Pop이다.
    std::function<void(Sid,float,std::uint32_t)> escapeEffect; // 원본00429010/CD004e5c10의5.0/0 효과다.
    std::function<void(std::uint32_t)> refreshOwner; // Dais 전처리의현재1..8 순서 갱신이다.
    std::function<float(Sid,std::uint32_t,std::uint32_t,float)> damageable; // 나머지 알터 사건의 기존 Damageable 전체 fallback이다.
};
// 원본 알터 몸체와 다이스 파생 PreDestroy다. 완료 의미는 실제 하위 소비자 연결 여부로 따로 평가한다.
class RawAltarSacrifice {
public:
    // 모든 대상은 같은 풀을 공유하고 이 객체보다 오래 살아야 한다.
    RawAltarSacrifice(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        const RawSquidNeighbors& footprint,const RawRunePostPop& rune,const RawTempleRitual& temple,const RawCarrierPreDestroy& carrier,
        const RawDaisRegular& dais,const RawFrameAdvance& advance,SquidProcessHost& host,const RawDamageablePreDestroy& damageable,
        DaisRegularState& daisState,AltarSacrificeState& state,AltarSacrificeHooks hooks);
    bool Handles(Sid altar) const; // 실제 알터 타입/가상표만 분배한다.
    Sid DaisAt(Sid sid) const; // 기준점의 일반 typed Dais 조회다. 원본 buried/버킷 조건을 사용한다.
    std::uint32_t Count(Sid altar) const; // 같은 기준점의 Dais 가상 개수, 없으면0이다.
    float Handle(Sid altar,std::uint32_t event,std::uint32_t count,float payload) const; // 실제20/21/22/23/24의 지연/부작용 순서다.
    void EscapeFrame(Sid dais,std::uint32_t count) const; // 첫3회는 프레임 지정, 이후에는 결과를 버려도 실제 난수를 소비한다.
    void ReleaseFromAltar(Sid altar) const; // 패치 자동 해방→가상Unpop→현재P 첫번호 plain 쓰기→가상Repop이다.
    void ReleasePriest(Sid dais) const; // 포획 WORD복사/owner0/중심+.5 Pop/5초 효과/원본삭제/소리/604 취소다.
    void DaisPreDestroy(Sid dais,std::uint32_t flags) const; // 실제알터24 전체순회→완료→604취소→알터삭제→owner갱신→Damageable이다.
    const SidPool& Pool() const; // 후속 실제 세계의 동일 풀 검증에 쓴다.
private:
    std::int32_t Frame(Sid sid) const; // 패치signedDWORD/CD BYTE의 현재 값이다.
    std::int32_t Direction(Sid sid) const; // 현재 raw타입의 실제 코드 방향이다.
    std::int32_t First(Sid sid,std::uint8_t letter) const; // 현재 raw타입의 해당 글자 첫 번호다.
    void WriteFrame(Sid sid,std::int32_t value) const; // Release의 표시 없는 직접 프레임 필드쓰기다.
    std::array<float,2> Position(Sid sid) const; // 현재 float 좌표를 매효과 뒤 다시 읽는다.
    bool Valid(Sid sid) const; // 원본 isValid는 SID1..capacity-1만 본다.
    Sid ContainedPriest(Sid dais) const; // 원본 specialized 조회의kind WORD 조건을 기존 Carrier mask0/allowDeadfalse에 재사용한다.
    void FinishOnAltarForms(Sid dais,Sid altar) const; // 다음 링크를 먼저 저장하고 각 현재 Regular/Shared24를 실제 Host에서 읽는다.
    SidPool& pool_; // 유일한 raw 상태 저장소다.
    const SquidHash& hash_; // 기존 일반 해시와 버킷 순서다.
    std::span<const RiftTypeRecord> types_; // 기준점/발자국의 현재 타입 정보다.
    std::span<const RiftTypeFrames> frames_; // 현재 코드 방향과 첫번호/길이 표다.
    const RawSquidNeighbors& footprint_; // 기존 발자국/중심 몸체다.
    const RawRunePostPop& rune_; // 같은 typed AltarAt 몸체다.
    const RawTempleRitual& temple_; // 기존 강한 사제/개수 조회다.
    const RawCarrierPreDestroy& carrier_; // 기존 contained finder의kind WORD 조건이다.
    const RawDaisRegular& dais_; // 실제604취소와 희생 완료 몸체다.
    const RawFrameAdvance& advance_; // 기존 한 번 보정/실제 setter 몸체다.
    SquidProcessHost& host_; // 현재 알터24 Form의 실제 같은 Kernel이다.
    const RawDamageablePreDestroy& damageable_; // 마지막 실제 공통 피해가능 전처리다.
    DaisRegularState& daisState_; // 같은 boss/type/동맹/owner 전역이다.
    AltarSacrificeState& state_; // 현재 저항 조절 전역이다.
    AltarSacrificeHooks hooks_; // 실제 파생 공간/난수/장치/표시 효과다.
};
// Frame/Host/Factory/Damageable을 실제 같은 월드의 기존 모듈에 연결한다. 남은 가상 공간·UI·장치는 caller가 공급한다.
AltarSacrificeHooks MakeAltarSacrificeHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,SquidFactory& factory,
    const RawAssetRegular& assets,GameRandom& random,AltarSacrificeHooks hooks);
}
