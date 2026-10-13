// 다이스/알터의 실제 최초 Pop과 플레이어 SID 등록·삭제 후처리 및 알터 파생 preDestroy다.
#pragma once
#include "o/RawAltarSacrifice.h"
#include "o/RawCarrierPostPop.h"

namespace netstorm::o {
// 실제 Player의 두 SID 쌍이다. 하나만 일치할 때 다른 쌍을 지우지 않는다.
struct DaisPlayerSlots {
    Sid first{},firstMirror{}; // patch+8c/+90, CD+84/+88: flags1 등록/삭제다.
    Sid second{},secondMirror{}; // patch+94/+98, CD+8c/+90: flags2 등록/삭제다.
    std::uint32_t homeIsland{}; // patch+78/CD+70: 새알터가현재홈섬에있는지비교하는번호다.
};
// 로딩/게임초기시각/원래플레이어 저장소를 실제 공통모듈과 공유한다.
struct DaisLifecycleState {
    double& gameTime; // patch005c85f0/CD00564db8의현재초기게임시각이다. Process clock.now와 다른 전역이다.
    CarrierPostPopState& loading; // 같은 DamageablePostPop의 loadingDepth 전역이다.
    std::array<DaisPlayerSlots,kPlayerCount+1> players{}; // owner0..8의현재Player 필드를저장한다.
};
// 파생 가상 공간과 아직 복원해야 할 장치/공통 Damageable 후처리 경계다.
struct DaisLifecycleHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 실제 SquidFrame.Set이다.
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 로딩중stage초기화의가상+48/+4c flags0이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 같은Factory의현재altarType/flags0이다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 새알터의현재다이스owner 가상지정이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 새알터의실제전체Pop이다. 자체후처리21예약도포함한다.
    std::function<bool()> reserveRegular; // 알터의독립로컬new0x28 성공여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 같은Host의로컬21 Regular다.
    std::function<Sid(float,float)> surface; // 같은현재지면의실제표면조회다.
    std::function<bool(Sid)> hasIsland; // 원본004acdf0/CD004acad0의현재섬번호존재조건이다.
    std::function<std::uint32_t(Sid)> islandNumber; // 원본004acda0/CD004acaa0의현재섬번호다.
    std::function<bool(std::string_view)> option; // 원본noShowWarnCantUpgrade 현재설정이다.
    std::function<void(std::string_view)> chat; // 원본WarnCantUpgradeAltarOffHomeIsland 요청이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 같은현재공통/파생가상삭제분배기다.
    std::function<void(Sid,std::uint32_t)> damageablePostDestroy; // 전체0044b9e0/CD00461b40이어야한다. 공통post만으로대체하지않는다.
};
// 모든 raw/등록/공통몸체는 같은월드에만연결한다. 미연결 파생Pop은여전히지원완료가아니다.
class RawDaisLifecycle {
public:
    // 상위상태와효과는이객체보다오래살아야하며모든필수callback이필요하다.
    RawDaisLifecycle(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const RawSquidNeighbors& footprint,const RawTempleRitual& temple,const RawAltarSacrifice& altar,
        const RawCarrierPostPop& postPop,const RawDamageablePreDestroy& preDestroy,
        DaisRegularState& daisState,AltarSacrificeState& altarState,DaisLifecycleState& state,DaisLifecycleHooks hooks);
    bool Handles(Sid sid) const; // 실제164/165최종VT의후처리만지원한다.
    void PostPop(Sid sid,std::uint32_t flags) const; // 실제Dais등록/Altar21예약→DamageablePostPop의순서다.
    void AltarPreDestroy(Sid altar,std::uint32_t flags) const; // 살아있는같은Dais가상삭제→주변추상Rune정리→Damageable이다.
    void PostDestroy(Sid sid,std::uint32_t flags) const; // DaisPlayer일치쌍정리/Altar직접위임→전체Damageable이다.
    const SidPool& Pool() const; // 실제PostPop/Destroy분배기의월드동일성검사다.
private:
    std::uint32_t Owner(Sid sid) const; // 현재raw ownerBYTE를효과뒤다시읽는다.
    void Player(std::uint32_t owner) const; // 현재owner0..8저장범위다.
    void DaisPostPop(Sid sid,std::uint32_t flags) const; // 실제165최초Pop과현재player등록/경고분기다.
    void WriteFrame(Sid sid,std::int32_t frame) const; // 로딩초기화의표시없는직접쓰기다.
    SidPool& pool_; // 실제현재raw풀이다.
    const SquidHash& hash_; // 실제주변추상Rune를읽는일반해시다.
    std::span<const RiftTypeRecord> types_; // 같은현재타입표다.
    const RawSquidNeighbors& footprint_; // 기존실제발자국/중심몸체다.
    const RawTempleRitual& temple_; // 기존현재Stage/Count몸체다.
    const RawAltarSacrifice& altar_; // 기존현재typedDais 조회다.
    const RawCarrierPostPop& postPop_; // 마지막전체Damageable→공통후처리다.
    const RawDamageablePreDestroy& preDestroy_; // 마지막전체Damageable→공통전처리다.
    DaisRegularState& daisState_; // 같은현재boss·suppress·player개수전역이다.
    AltarSacrificeState& altarState_; // 같은알터21현재저항지연이다.
    DaisLifecycleState& state_; // 현재SID쌍/초기시각/로딩전역이다.
    DaisLifecycleHooks hooks_; // 실제동일풀의생성/가상공간/UI/삭제효과다.
};
// 실제 Frame/Factory/Owner/로컬Host몸체를공유한다. Pop/공통파생삭제/UI는caller가실제연결한다.
DaisLifecycleHooks MakeDaisLifecycleHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,SquidFactory& factory,
    class SquidOwner& owner,DaisLifecycleHooks hooks);
}
