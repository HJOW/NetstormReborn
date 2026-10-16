// 초기 요새 자산의 실제 postPop 재정의를 raw SID와 공통 장부에 연결한다.
#pragma once
#include "o/RawCarrierPostPop.h"
#include <array>
#include <span>

namespace netstorm::o {
// 원본 Player의 신전 네 필드를 유지한다. 복사본을 읽지 않도록 실행 월드가 이 저장소를 공유한다.
struct AssetPostPopPlayer {
    // initialTemple: 초기 신전.
    // initialTempleCopy: 초기 신전 복사.
    // newTemple: 새 신전.
    // newTempleCopy: 새 신전 복사.
    std::uint32_t initialTemple{},initialTempleCopy{},newTemple{},newTempleCopy{};
};
// Gun/Residence/Geyser/Vortex 재정의가 사용하는 전역과 목록이다.
struct AssetPostPopState {
    // inBattle: 내부 전투 여부.
    // boss: boss 여부.
    // fortMode: Fort 모드 여부.
    // checkingForms: 검사 Form 여부.
    // residenceAnimation: 거주건물 애니메이션 여부.
    bool inBattle{},boss{true},fortMode{},checkingForms{},residenceAnimation{};
    // loadingDepth: 로딩 깊이.
    // templeCount: 신전 개수.
    // selected: 현재 선택한 객체·컨트롤의 번호 또는 토큰.
    std::uint32_t loadingDepth{},templeCount{},selected{};
    // localOwner: 로컬 소유자.
    std::uint8_t localOwner{1};
    // islandType: island 자산을 타입 표에서 찾는 번호.
    // stalagType: islandStalag 받침 자산을 타입 표에서 찾는 번호.
    // slotType: slot 자산을 타입 표에서 찾는 번호.
    std::uint32_t islandType{94},stalagType{95},slotType{133};
    // players: 플레이어.
    std::array<AssetPostPopPlayer,kPlayerCount+1> players{};
    // residences: 거주건물.
    // temples: 신전.
    // geysers: 가이저.
    SquidPostPopList residences,temples,geysers;
    SquidPostPopList outposts; // DAT0055a70c/CD005670e0: 신전/아웃포스트의 섬 소유권 재계산에 쓰는 별도 목록이다.
};
// 외부 몸체를 생략하지 않고 호출 순서대로 연결하는 명시 경계다. 필요한 경계가 없으면 사전 검사에서 거부한다.
struct AssetPostPopHooks {
    std::function<bool()> reserveRegular; // 원본 각각의 new(0x28) 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 실제 Regular 생성자에 event·SID·지연을 넘긴다.
    std::function<void(Sid)> createGunProcess; // new(0x38)·GunProcess 생성/부착 전체 경계다.
    std::function<void(Sid,std::uint32_t)> sunAviaryPostPop; // 실제0045ec50의 생성/부착/Damageable 전체 몸체다.
    std::function<void(Sid)> removeAnimation; // 썬 디스크 발사기의 현재 애니메이션 제거다.
    std::function<void(Sid)> checkForms; // 켜진 진단의 checkForms() assert를 검사한다.
    std::function<std::int32_t(std::int32_t)> gunFrame; // 현재 총 각도에서 실제 Gunanim 프레임을 조회한다.
    std::function<void(Sid,std::int32_t)> setGraph; // 004ad3e0/CD 004adbd0: BYTE 그래프 번호를 쓴다.
    std::function<Sid(float,float)> surface; // Residence의 현재 지면 SID 조회다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 가상 +0x74, Vortex/Residence 파생 분배를 포함한다.
    std::function<void(Sid)> notifySurface; // 연결을 재평가하는 004214a0/CD 00448c10이다.
    std::function<void(Sid)> startBlocker; // 비로딩 배리케이드의 0046ae80/CD 대응 몸체다.
    std::function<void(Sid,std::uint32_t)> geyserRegistration; // 00416720/CD 004d8670(sid,1)이다.
    std::function<std::vector<Sid>(float,float)> atPosition; // 실제 one-point Squid iterator 순서다.
    std::function<void(Sid,std::uint32_t)> unpop; // 가상 +0x48이다.
    std::function<void(Sid,std::uint32_t)> repop; // 가상 +0x4c이다.
    std::function<std::uint32_t(Sid,std::uint32_t)> typeDword; // 현재 타입의 +0x74/+0x130/+0x134를 읽는다.
    std::function<void(std::uint32_t)> playImpactSound; // 타입 +0x74 impactSound 토큰을 실제 Sound 재생에 넘긴다.
    std::function<void()> clearPlacement; // 선택된 Vortex 부류의 배치 커서를 취소한다.
    std::function<std::int32_t(float,float)> islandNumber; // 현재 지면의 섬 번호다.
    std::function<bool(std::int32_t)> islandExists; // 실제 islandList.exists 검사다.
    std::function<Sid(std::int32_t)> islandTemple; // CD의 getVortexSid==0 원본 assert 조회다.
    std::function<void(std::int32_t,Sid)> registerTemple; // 실제 영역 지도에 섬의 Vortex를 기록한다.
    std::function<std::int32_t(float,float)> regionNumber; // 패치는 실제 float 좌표, CD는 ftol/16의 정수 칸을 float로 넘겨 부호 byte를 읽는다.
    std::function<void(Sid,std::uint32_t)> destroy; // 같은 풀의 가상 삭제를 호출한다.
    std::function<bool(Sid,std::uint32_t)> hasContent; // 현재 Content 목록의 slotType/0x10 조회다.
    std::function<void(Sid,std::uint32_t)> createSlot; // RawContainedCreate(type,0,0x10,0x10,0) 연결이다.
    std::function<void()> refreshProduction; // Vortex 비로딩 slot 확인 뒤 0047d890/CD 004773a0이다.
    std::function<void(Sid,std::uint32_t)> sunFencePostPop; // 실제00457560의 이웃/연결선/Regular/Damageable 전체 몸체다.
};
// 실제 PE 가상 표 +0x20에 따라 초기 요새 건물의 전용 몸체를 분배한다.
class RawAssetPostPop {
public:
    // 같은 풀/공통 장부/Damageable와 상태를 받는다. 참조 대상은 이 객체보다 오래 살아야 한다.
    RawAssetPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPop& base,
        const RawCarrierPostPop& damageable,AssetPostPopState& state,AssetPostPopHooks hooks);
    // 실제 복원한 가상 표인지 조회한다. 타입 번호로 다른 파생 재정의를 대체하지 않는다.
    bool Handles(Sid sid) const;
    // 공간 쓰기 전에 현재 분기가 요구하는 외부 효과와 공통 장부를 확인한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // Gun·제네레이터·레지던스·배리케이드·가이저·신전 전체 재정의 순서를 실행한다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 실제9 Player BYTE를 공유한다. 현재 localOwner 포인터를 주면 외부 효과 후에도 그 DWORD를 다시 읽는다.
    // 기록은 PATCH0xb4/CD0xac 크기9개이며 이 모듈보다 오래 살아야 한다. 생략한 owner는 state.localOwner다.
    void BindPlayerRecords(std::span<std::uint8_t> records,const std::uint32_t* currentLocalOwner=nullptr);
    // SquidPostPop 구성 때 같은 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    // 두 PE 가상 주소를 실제 몸체 부류로 분류한다.
    std::uint32_t Kind(Sid sid) const;
    // 실패 가능한 두 Regular 확보를 각각 실행한다.
    void Regular(Sid sid,std::uint32_t event,float delay) const;
    // Gun 몸체의 네 분기·현재 raw 재조회·flags1 0x1000의 WORD/extra 쓰기다.
    void Gun(Sid sid,std::uint32_t flags,std::uint32_t kind) const;
    // 레지던스 최초 소유자·목록·예약·연결·패치의 잔여 프레임을 처리한다.
    void Residence(Sid sid,std::uint32_t flags) const;
    // 가이저 최초 비용 WORD·목록·주변 받침 neutral 프레임·Damageable을 처리한다.
    void Geyser(Sid sid,std::uint32_t flags) const;
    // 신전 공통 몸체 후 각 원소 wrapper의 extra/예약을 처리한다.
    void Vortex(Sid sid,std::uint32_t flags,std::uint32_t kind) const;
    // Damageable 뒤 Player·목록·영역·다른 신전 삭제·slot 생성을 처리한다.
    void VortexBase(Sid sid,std::uint32_t flags) const;
    // 현재 Player의 신전 네 DWORD를 실제 BYTE 저장소에서 읽는다. 미연결 독립 모듈은 기존 상태를 사용한다.
    std::uint32_t PlayerTemple(std::uint32_t owner,std::size_t field) const;
    // 초기/새 신전 및 각각의 복사 필드를 정확DWORD 폭으로 쓰고 다른 Player 바이트를 보존한다.
    void SetPlayerTemple(std::uint32_t owner,std::size_t field,std::uint32_t sid) const;
    // 실제9기록의 판본별 신전 필드 주소를 계산한다. field0..3은 initial/copy/new/copy 순서다.
    std::size_t PlayerTempleOffset(std::uint32_t owner,std::size_t field) const;
    // 현재 외부 battleOwner DWORD 또는 독립 모듈의 localOwner BYTE를 조회한다.
    std::uint32_t LocalOwner() const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // base_: 파생 처리에서 공통 동작을 위임하는 기본 객체.
    SquidPostPop& base_;
    // damageable_: 피해 가능한 객체의 HP·삭제·후처리 연결.
    const RawCarrierPostPop& damageable_;
    // state_: Gun/Residence/Geyser/Vortex 재정의가 사용하는 전역과 목록이다 (AssetPostPopState).
    AssetPostPopState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (AssetPostPopHooks).
    AssetPostPopHooks hooks_;
    // playerRecords_: Player 생성·AI·경제와 공유하는 현재9기록이다. 연결되면 state.players 사본을 쓰지 않는다.
    std::span<std::uint8_t> playerRecords_;
    // localOwner_: 효과 콜백이 변경할 수 있는 현재 원본 소유자 DWORD의 장기 주소다.
    const std::uint32_t* localOwner_{};
};
}
