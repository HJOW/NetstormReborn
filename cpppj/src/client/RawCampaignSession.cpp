// 기존 원본 몸체의 단일 저장소 조립을 실제 클라이언트 세션이 소유한다.
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawCampaignGunQueries.h"
#include "o/RawPathRetarget.h"
#include "o/RawCarrierMining.h"
#include "o/RawGeyserMined.h"
#include "o/RawGeyserLifetime.h"
#include "o/RawCarrierEscape.h"
#include "o/RawPacketDispatch.h"
#include "client/AiTypeRequiredEnergy.h"
#include "o/RawGameRandomSeed.h"
#include "o/RawSquidFinder.h"
#include "o/RawConstructionArrival.h"
#include "o/RawConstructionProcess.h"
#include "o/RawManaBoltArrival.h"
#include "o/RawManaBoltRegular.h"
#include "o/RawBoardApproach.h"
#include <map>
#include "client/RawCursorPosition.h"
#include "client/RawCarrierDropPoint.h"
#include "o/RawProcessRemoval.h"
#include "o/RawSquidLevel.h"
#include "o/RawSquidSelection.h"
#include "o/RawCarrierRegular.h"
#include "o/RawContentPreviewProcess.h"
#include "o/RawNuggetPostPop.h"
#include "o/RawNuggetRegular.h"
#include "o/RawCarrierFalling.h"
#include "o/RawCarrierFallStart.h"
#include "o/RawCarrierTask.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawArcherTypeData.h"
#include "o/RawProjectileGeometry.h"
#include "o/RawFormPacket.h"
#include "o/RawFenceLifetime.h"
#include "client/RawCursorPlacement.h"
#include "o/RawContentParentNotice.h"
#include "o/RawPlayerDamageNotice.h"
#include "o/RawFenceTarget.h"
#include "o/RawFencePostPop.h"
#include "o/RawFenceNeighbors.h"
#include "o/RawFenceLine.h"
#include "o/RawFenceRegular.h"
#include "o/RawFenceMarkerClear.h"
#include "o/RawFenceCreate.h"
#include "o/RawFenceRebuild.h"
#include "o/RawGunAngle.h"
#include "o/RawRainVortexDraw.h"
#include "o/RawRegionOwner.h"
#include "o/RawSpStoreNoticeBindings.h"
#include "o/RawPlayerTechnology.h"
#include "o/RawAiCreate.h"
#include "o/RawPlayerColor.h"
#include "o/RawAiBrainInitialUpdate.h"
#include "o/RawAiDamage.h"
#include "o/RawPriestLookup.h"
#include "o/RawRunePostPop.h"
#include "o/RawVortexPreDestroy.h"
#include "o/RawBuriedPostPop.h"
#include "o/RawBuriedRegular.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawRainAviaryProcess.h"
#include "o/RawFlyerProcess.h"
#include "o/RawFlyerFrame.h"
#include "o/RawFlyerTarget.h"
#include "o/RawDamageableRelease.h"
#include "o/RawFlyerPreDestroy.h"
#include "client/RawShapeToBuffer.h"
#include "client/OwnerColorMaps.h"
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <new>
using namespace netstorm::o;
namespace netstorm::client {
namespace {
// 아직 미공급인 실제 하위는 선택 시 구체적 함수 인자형을 포함한 오류를 반환한다.
template<class Result,class... Args> std::function<Result(Args...)> SessionMissing() {
    return [](Args...)->Result { throw std::logic_error(std::string("실제 세션 하위 소비자 미연결: ")+__FUNCSIG__); };
}
// optional 경계를 성공으로 채우지 않고 실제 선택 시 요청을 보고한다.
template<class Result,class... Args> void FillMissing(std::function<Result(Args...)>& callback) { if (!callback) callback=SessionMissing<Result,Args...>(); }
// 동일 풀의 실제 예약/부착 실패를 즉시 보고한다.
void SessionCheck(bool value) { if (!value) throw std::logic_error("실제 캠페인 세션 예약/자료 조건 오류"); }
// 하나의 실제 클라이언트 소리 이름표만 사용한다.
SoundList& RequireSounds(const RawCampaignSessionServices& services) { if (!services.soundNames) throw std::invalid_argument("세션의 실제 SoundList 누락");return *services.soundNames; }
// 현재 원본 초기 다리/받침/미션 전역이다.
BridgeConnectState CampaignLinks() { BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;result.connectorType=155;result.noIslandType=157;result.battle=true;result.mission=true;return result; }
constexpr std::size_t kCampaignFactoryCapacity=500; // PATCH004fc990의push1f4→0044fb20이확보하는실제0055a4d0목록의DWORD수다.
constexpr std::size_t kPlayerFactoryCapacity=30; // 실제4910a0이9개 Player 각각 new78/용량1e로 초기화하는 공장 목록이다.
// 실제83개 VT에서+10/+14/+18이4af780/44b4b0/44b9e0인14개만 일반 Damageable 수명으로 연결한다.
constexpr std::array<std::uint32_t,14> kPlainDamageableTables{0x501ea0,0x509148,0x509930,0x5099e8,0x509aa0,0x50a820,0x50a8e8,0x50b6e8,0x50b7c0,0x50b888,0x50b9b8,0x50bad0,0x50e668,0x50e738};
// 보호 GameAssets의 실제 TYPE/프레임을 보관하며 검사 전용74 변경을 적용하지 않는다.
struct SessionAssets {
    const GameAssets& game; // 클라이언트가 소유하는 단일 자산이다.
    const BaseFileSystem& files; // 동일 보호 VFS 참조다.
    std::vector<RiftTypeRecord> types; // 실제 원본 전체 TYPE 자료다.
    std::vector<RiftTypeFrames> frames; // 같은 물리 블록의 실제 프레임 Run이다.
    // PATCH 판본과 actual 자료를 첫 모듈 생성 전에 확인한다.
    SessionAssets(const GameAssets& source,const BaseFileSystem& storage):game(source),files(storage),types(source.TypeTable().Types().begin(),source.TypeTable().Types().end()),frames(types.size(),RiftTypeFrames({})) {
        if (source.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("실제 세션의 CD판 조립 미연결");
        // 실제 로딩 번호70부터 프레임 표를 직접 채운다.
        for (const auto& asset:source.Types()) frames[70+asset.block]=asset.definition.FrameTable();
    }
};
// 원본 비정렬 DWORD를 지정 raw 필드에 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 원본 little endian의 네 바이트만 기록한다.
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 비정렬 DWORD를 같은 현재 바이트 저장소에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset+4>raw.size()) throw std::out_of_range("실제 세션 현재 DWORD 범위 오류");
    return static_cast<std::uint32_t>(raw[offset])|(static_cast<std::uint32_t>(raw[offset+1])<<8)|
        (static_cast<std::uint32_t>(raw[offset+2])<<16)|(static_cast<std::uint32_t>(raw[offset+3])<<24);
}
}
// 메뉴의 현재 원문/참조와 자식 우선 소멸 순서를 한 고정 주소에 보관한다.
struct RawCampaignObjectMenus::Impl {
    CampaignObjectMenuUi ui; // GUI·Chat·정적 문자열과 현재 외부 getter의 수명 소유자다.
    std::array<std::string,188> classes; // 실제GameAssets정의의TYPE+aa 보충문자열이며같은체인동안고정주소다.
    ObjectMenuInfoState infoState; // 같은현재로컬번호/편집/미션getter와117/118그림TYPE다.
    CarrierDropPointState dropState; // PE의 지도 한계256/실패점0,0이며 같은 위치 계산기가 직접 읽는다.
    CarrierMenuState carrierState; // 공통 객체 상태/실제 CD SP와 같은 참조를 읽고 PATCH는 현재 getter를 사용한다.
    std::unique_ptr<RawPlayerIdentity> identity; // 실제 Player9와 Client Chat 원문을 빌린다.
    std::unique_ptr<RawContainedLookup> lookup; // 같은 풀의 현재 이동 프로세스 종속 조회/가상 삭제 몸체다.
    std::unique_ptr<RawCarrierDropPoint> dropPoint; // 전체 MayPlace/현재 중심을 사용하는 실제 위치 몸체다.
    std::unique_ptr<RawPlayerMenu> player; // 현재 사제 상태/동맹/정적 asksend 문자열을 읽는 끝 부모다.
    std::unique_ptr<RawObjectMenu> object; // 같은 Player 메뉴와 실제 HP/회수 계산을 사용한다.
    std::unique_ptr<RawObjectMenuInfo> info; // 같은가상count/Identity/동맹/Player/포획몸체와controlled제목이다.
    std::unique_ptr<RawCarrierMenu> carrier; // 사제의 비가상 기본 호출이 참조하는 실제 Carrier 메뉴다.
    std::unique_ptr<RawPriestMenu> priest; // 같은 기술/SP/커서 취득과 위 전체 부모 체인을 사용한다.
    // 같은 현재 전역의 참조를 먼저 구성한다. 생성 중 메뉴/게임 효과를 실행하지 않는다.
    Impl(CampaignObjectMenuUi input,float& cdSp):ui(std::move(input)),carrierState{ui.objectState.localOwner,ui.objectState.editor,dropState.mapLimit,cdSp,ui.objectState.helpCommand} {}
};
// factory의 완성된 단일 주소만 넘겨받는다.
RawCampaignObjectMenus::RawCampaignObjectMenus(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
RawCampaignObjectMenus::~RawCampaignObjectMenus()=default; // 필드 역순으로 사제→Carrier→객체→Player→Identity를 해제한다.
// 원본 파생별 메뉴 작성만 위임하며 어떤 항목도 임의 활성화하지 않는다.
void RawCampaignObjectMenus::OpenPriest(Sid sid) const { impl_->priest->Open(sid); }
void RawCampaignObjectMenus::OpenCarrier(Sid sid) const { impl_->carrier->Open(sid); } // 실제 가상 메뉴가 Carrier인 caller가 선택한다.
void RawCampaignObjectMenus::OpenDamageable(Sid sid) const { impl_->object->OpenDamageable(sid); } // 실제 Damageable 회수 관문을 보존한다.
void RawCampaignObjectMenus::OpenObject(Sid sid) const { impl_->object->Open(sid); } // 기본 메뉴를 파생 기능의 대체 성공으로 쓰지 않는다.
// 임의제목입력대신같은현재객체와원본정보작성몸체를사용한다.
void RawCampaignObjectMenus::Summary(Sid sid) const { impl_->info->Summary(sid); }
// 객체없는타입메뉴도원본4인자와SID=-1계약을그대로전달한다.
void RawCampaignObjectMenus::Describe(std::uint32_t type,std::uint32_t count,std::uint32_t owner,std::uint32_t sid) const { impl_->info->Describe(type,count,owner,sid); }
// Open과 동일한 실제 세션 커서/현재 금액으로 건설 선택을 수행한다.
bool RawCampaignObjectMenus::SelectBuild(std::uint32_t type,Sid priest) const { return impl_->priest->SelectBuild(type,priest); }
// 정적 Player 버퍼는 메뉴 체인이 살아 있는 동안 같은 원본 토큰으로 조회한다.
std::string_view RawCampaignObjectMenus::TextParameter(std::uint32_t token) const { return impl_->player->TextParameter(token); }
// 실제 월드/상태와 모든 callback 대상을 같은 고정 주소에 소유한다.
struct RawCampaignSession::Impl {
    struct SelectionLink { RawUserSelection* selection{}; }; // 살아 있는 실제 선택 객체의 주소이며 약한 토큰으로만 빌린다.
    std::weak_ptr<SelectionLink> activeSelection; // UI 몸체가 먼저 파괴돼도 낙하가 해제된 주소를 호출하지 않는다.
    RawCampaignSessionServices services; // 실제 클라이언트의 선택 서비스이며 검사 관찰 기본값을 사용하지 않는다.

    SessionAssets assets; // 보호 archive의 현재 실제 자료다.
    PriestPlacementAssets metadata; // 실제 프레임/핫스폿/SHP이며 합성 그림 자료가 아니다.
    std::uint32_t terrainType{97}; // 실제 월드 생성과 영역 소유자 getter가 함께 읽는 isle 타입 전역이다.
    RawSurfaceWorld world; // 모든 생성/Pop/Graph가 사용하는 단일 raw 월드다.
    RawWorldCore core; // 같은 Pool/장부/해시의 참조만 유지한다.
    std::unique_ptr<RawCampaignCursor> cursor; // 같은 실제 Form/마나/위치/로컬 예측 몸체를 설치 뒤 소유한다.
    SoundView cursorView{}; // 마우스 위치의 실제 투영에도 현재 단일 카메라/화면 원점을 사용한다.
    CursorDropState cursorDrop{.lockDepth=1}; // 보호 PE5325c8의 초기 잠금1이며  실제 취득/배치/취소가 공유할 커서 상태이며 마나 토큰도 같은 저장소다.
    CursorManaGlobals cursorManaGlobals; // 현재 마나 종류/반경과 월드 좌표 제한 전역이다.
    std::unique_ptr<RawCursorMana> cursorMana; // Kernel을 먼저 비운 뒤 해제하는 실제 마나 프레임/수명 몸체다.
    EnergySupplyState energyState; // 마나 반경과 별개인 실제 제곱 반경 및 공급 전역이다.
    SquidPostPopList energySources{std::vector<std::uint32_t>(kEnergySourceCapacity),0}; // 4fcb80의 실제30칸 lastSource다.
    std::unique_ptr<RawEnergySupply> energySupply; // 현재 장부 공급원/동맹/타입을 직접 읽는 실제 배치 에너지다.
    std::vector<ConstructionBuilderType> builderTypes=std::vector<ConstructionBuilderType>(188); // 같은 실제 TYPE의 속도/theme/A..P 첫 프레임이다.
    std::map<std::uint16_t,ConstructionBuilderPath> builderPaths; // SID별 안정된 반환 view이며 계산 값 대신 같은 현재 경로를 읽는다.
    std::uint32_t builderApproachMask{0x50444200}; // 실제005424b8의 접근점 genus 전역이다.
    std::unique_ptr<RegularProcess> builderPendingRegular; // 원본 new28 성공부터 같은 Host 부착까지의 실제 예약 객체다.
    GameRandom random; // 게임 난수의 명시 초기 입력이다.
    ScrambledSpStore sp; // 같은 게임 난수를 사용하는 실제 패치 SP 저장소다.
    SquidRewardState rewardState; // 실제 HP getter의 장기 전역이다.
    SquidReward hp; // 같은 Pool/TYPE/장부로 최대HP와 실제HP를 읽는다.
    std::unique_ptr<RawSquidLevel> level; // 실제 base 가상80/84의 Factory 저장 상태를 처리한다.
    ChunkMap chunks; // 실제 Builder/성장/객체 선택의 지도다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> islandBytes{}; // 실제3204BYTE 섬 목록이다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(9*0xb4); // 부모/Owner/신전이 공유하는 실제9 기록이다.
    std::array<std::uint32_t,81> alliances{}; // 원본 방향별9×9 DWORD 표다. 실제 초기화 모듈이 현재 칸을 직접 쓴다.
    std::array<std::uint8_t,9> hiddenPlayers{}; // 같은 현재 Chat 숨김 BYTE9이며 동맹이나 active의 사본이 아니다.
    PlayerParticipationState participationState; // 독립 모듈의 값형 호환 상태이며 실제 세션은 현재 getter로만 읽는다.
    std::unique_ptr<RawPlayerParticipation> participation; // 같은 Player9/동맹/실제 brain 해제 몸체다.
    RawAiBrainMemory aiMemory; // AI 본체와17개 목록의 유일한 실제 소유 메모리다.
    std::unique_ptr<RawAiBrainConstructor> aiConstructor; // 같은 Pool/현재 TYPE/mana/섬/방향별 동맹의 실제 생성·소멸이다.
    std::unique_ptr<RawAiBrainInitialUpdate> aiInitial; // 같은 Player 기술 부모/contained 분류를 실제 본체에 기록한다.
    std::unique_ptr<RawAiCreate> aiCreate; // 원본 Color→기존brain→NoTemple→생성/부착/초기갱신 순서다.
    std::array<std::uint32_t,4> aiWorkshops{86,87,88,85}; // PATCH004709d0의 theme0/1/2/3이 읽는005411b0/b4/b8/ac의 실제 기본 타입이다.
    std::uint32_t aiExcludedType{133},aiMissionFallback{}; // 실제 sunWalker 분류 제외 번호와 현재 getter를 사용할 때 읽지 않는 값형 호환 저장소다.
    bool loaded{}; // 같은 실제 저장 부모와 이동 설치가 정상 종료했을 때만 AI 생성 요청을 허용한다.
    MissionOutcomeState outcomeState; // 미션 종료까지 유지하는43개 사건 잠금이며 입력 배열은 현재 원문 연결로 대체한다.
    std::unique_ptr<RawRegionOwner> regionOwner; // 동일 Pool/hash0에서 현재 지면 소유자 BYTE를 읽는다.
    std::unique_ptr<RawMissionOutcome> outcome; // 같은 Player9/동맹/사제/HP/영역의 현재 입력을 매 조회 읽는다.
    FortFinishOwnerState finishState; // 실제 미션 chaffMode의 자연 반환 조건이다.
    AssetPostPopState assetState; // 실제 신전/거주 구역/간헐천/아웃포스트 목록이다.
    RawPriestWorldState priestState; // 실제 사제/Carrier/보호막 장기 전역이다.
    RunePostPopState runeState; // PATCH005412e8/ec/f0의 현재Altar164/Dais165/Rune166이며 후속 실제 의식도 같은 저장소를 사용한다.
    PlayerWorldState playerState; // 현재 battleOwner와 templateOwner는 별도 상태다.
    FortObjectLoadState objectState; // 저장객체의 실제 미션/로딩 상태다.
    CampaignWorldGlobals globals; // 실제 로그인과 missionPost의 장기 전역이다.
    std::uint32_t revision{},ownerChanged{},vortexRevision{},piece{107},crt{}; // 원본 섬/신전/패턴/CRT 전역이다.
    std::uint32_t randomMode{}; // 실제 재시드에서 임시로 변경하는 현재 DWORD다.
    RawGameRandomSeed seed; // 같은 CRT/게임 난수와 실제 Unix 초 입력을 연결한다.
    std::int32_t noRegion{127}; // 정상 지형 생성의 실제 현재 지역없음 번호다.
    std::unique_ptr<RawIslandRuntime> islands; // 위3204BYTE와 동일 Pool을 공유한다.
    std::unique_ptr<RawPriestWorldBindings> priests; // 저장 사제의 실제 Owner/PostPop/보호막 분배기다.
    std::unique_ptr<RawPriestParticipation> priestParticipation; // 같은 현재live/Carrier/typedDais와기존ownerState의로그인종료조회다.
    std::unique_ptr<RawPlayerWorld> player; // 최초152 생성자/실제 PostPop/9기록을 연결한다.
    std::unique_ptr<RawPlayerRegular> playerRegular; // 현재 Player의 실제 사건1/2를 처리한다.
    std::unique_ptr<RawPlayerTemplateState> templateState; // 같은Template header/원본State28BYTE를 직접 읽는다.
    std::array<std::vector<std::uint8_t>,9> stateBytes; // 실제 선택한 State 섹션의 writable 원문이다.
    std::array<PlayerTemplateStateStream,9> stateStreams; // 같은 State cursor/end를 공유한다.
    std::array<std::string,9> stateNames; // 같은 현재 Template manager의 실제 이름이다.
    RegionPaintState paint; // 실제 지역 도장의 장기 상태다.
    RegionOwnershipState votes; // 실제 현재 신전/워크샾 투표 상태다.
    std::vector<std::uint32_t> themes; // 현재 TYPE의 같은 원소 번호다.
    std::vector<std::int32_t> elements; // 자산 Regular가 읽는 같은 원소 번호다.
    std::unique_ptr<RawCampaignOwnership> ownership; // 실제 Vortex/섬/Player/지역 연결이다.
    CarrierPostPopState carrierState; // Damageable의 실제 로딩 전역이다.
    std::unique_ptr<RawCarrierPostPop> carrier; // 같은 공통 장부와 실제 지면 소유자 경계다.
    AssetRegularState regularState; // 실제 예약 사건의 현재 전역이다.
    GunProcessState gunState; // 실제 Gun 초기 Form/시각/서버 전역이다.
    GunTargetState gunTargetState; // 실제 자기 동맹/검색의 현재 상태다.
    CursorRangeGlobals cursorRangeGlobals{cursorManaGlobals,gunTargetState}; // 마나와 총의 같은 좌표/픽셀/Wind 전역을 빌린다.
    std::unique_ptr<RawCursorRange> cursorRange; // 같은 World/현재 타입/Host에 연결한 원형·직선 사정거리 몸체다.
    std::vector<GunTargetType> gunTypes; // 보호 TYPE의 사거리/방향/높이 보충 자료다.
    std::vector<GunHitType> hitTypes; // 보호 TYPE의 HP/초와 별도 피해 보충 자료다.
    std::vector<float> firingIntervals; // 실제 타입별 발사 간격이다.
    std::unique_ptr<RawGunMode> gunMode; // 현재 raw 공격 방향을 매 검색마다 읽는다.
    std::unique_ptr<RawGunTarget> gunTarget; // 실제 finder/ray/필터의 같은 Core 인스턴스다.
    std::unique_ptr<RawGunHit> gunHit; // 실제 Q16 시간 압축과 현재 피해 getter다.
    PriestDamageState priestDamageState; // 실제 미션 레벨보정 차단/HP1 감시 권한이다.
    DamageableHitState generalDamageState{priestDamageState}; // 같은 레벨·현재 서버/로컬 소유자·주 섬의 일반 피해 전역이다.
    DamageableBurningState burningState; // 현재 nonzero damageEffect의 실제 생성 판정 전역이다.
    GunImpactState impactState; // 현재 충돌 시각 효과 억제/anim160 전역이다.
    std::vector<DudeNoticeTypeData> dudeSpeeds; // 실제 TYPE 이동속도의 현재 mutable 보충 표다.
    std::unique_ptr<RawPlayerDamageNotice> playerDamage; // 같은 Player9/모드의 실제 AI 조회와 피해 통지 래퍼다.
    std::unique_ptr<RawAiDamage> aiDamage; // 같은 AI 본체·목록에 실제 피해 대상/공격자 반응을 기록한다.
    std::unique_ptr<RawCampaignHitBindings> hits; // 같은 피해/감시/충돌/주변Dude/자산수명 처리기다.
    SoundList& soundNames; // 실제 보호막 파일명의 원본 이름표/opaque handle 조회다.
    SoundProcessState soundState; // 실제47 프로세스의 현재 frame/TYPE 중복 재생 전역이다.
    std::unique_ptr<SoundProcessSystem> sounds; // 같은 Host/Kernel의 실제 보호막 sound47 수명을 수행한다.
    std::vector<ArcherTypeData> archerTypes; // 원본 orientations/클러스터 수의 실제 보충 표다.
    std::vector<MissileTypeData> missileTypes; // 원본 투사체 사거리/방향/소리 입력이다.
    std::vector<ProjectileGeometryType> projectileTypes; // 실제 SHP 포구 WORD와 기본/물리 프레임 표다.
    ArcherAnimationState archerState; // 같은 Kernel의 게임 시각/경과다.
    MissileState missileState; // 같은 투사체 게임 시각이다.
    std::unique_ptr<RawProjectileGeometry> projectileGeometry; // 실제 포구/표적 치수/충돌 상자다.
    std::unique_ptr<RawFenceTarget> fenceTarget; // 같은 해시/배리어 끝점/현재 동맹의 실제 표적 코너 계산이다.
    std::unique_ptr<RawCampaignArcherBindings> combat; // 같은 Archer/Missile 생성/공간/Host를 소유한다.
    CrtRandom particleCrt{CrtRandom::Borrow(crt)}; // 실제 시드 초기화/지형이 소비한 현재 CRT DWORD를 함께 사용한다.
    std::array<std::uint32_t,188> shrapnelMapTokens{}; // 원본 일반 소유자 색표가 복원된144의 실제 토큰이다.
    OwnerColorMaps shrapnelOwnerColors; // 현재 원본 후보로 만든144의9×256 색표다.
    ShapeToBufferState shrapnelCaptureState; // 원본 fixed16/추가 TYPE 기준점/현재 소유자 색 선택이다.
    std::unique_ptr<RawShapeToBuffer> shrapnelCapture; // 같은 raw/보호 SHP의 실제 캡처이며 표시보다 오래 산다.
    std::unique_ptr<RawCampaignParticles> particles; // 같은 Host/Kernel/Renderer의 실제11/19/16 표시다.
    std::uint32_t archerCommand{0x030000b0}; // 보호 PE의 실제 입력 토큰이다.
    std::unique_ptr<RawCampaignAssetBindings> assetBindings; // 실제 저장 건물 후처리와 Gun/Regular다.
    CanonPlacementPipelineState gameplayPlacement; // 생성용 지형 판정과 구별한 실제 요청/커서의 현재 모드·방향 동맹이다.
    std::array<CanonPlacementIslandRegion,100> gameplayRegions{}; // 같은 원본32바이트 섬 기록의 현재 제한/존재 view다.
    std::unique_ptr<RawCanonPlacementPipeline> gameplayPipeline; // 같은 풀/표면/원본 자산으로 전체 실제 MayPlace를 실행한다.
    CanonPlacementPipelineState placement; // 실제 공간/기하/지형 허용 상태다.
    SquidPostPopList additional; // 기존 pipeline이 읽는 실제 추가 목록이다.
    ContainedFinderState contentFinder; // 기존 내용물 검색의 장기 전역이다.
    PriestMenuState priestMenu; // 원본 사제 메뉴의 타입/명령·특수 모드 전역을 같은 Session에서 소유한다.
    SquidPostPopList constructionExcludedTypes; // 0052f7e0의 초기 빈 TYPE 목록이며 additional의 SID 목록과 다르다.

    std::unique_ptr<RawCanonPlacementPipeline> pipeline; // 실제 Same Core MayPlace다.
    TerrainContextGlobals terrainGlobals; // 정상 문자 mapping의 실제 전역이다.
    TerrainStampState stamp; // 첫 생성 이후에도 유지되는 실제99cache다.
    FortRandomPlacementState randomPlacement; // 실제 지형/edgeFarm 배치 상태다.
    FarmClusterState farm; // 실제 정상 군집 모드다.
    std::unique_ptr<RawNormalTerrainWorldBinding> normal; // 실제 성장/도장/Normalize/군집 몸체다.
    std::unique_ptr<RawFortFinishOwner> finish; // 같은9 Player의 최초 지역을 읽는다.
    ContainedCreateState contentState; // 저장 내용물은 실제 Form6으로 생성한다.
    std::unique_ptr<ContentFormHost> contentForms; // ProcessForm과 다른 실제 Form6의 정렬 부착 몸체다.
    std::vector<CarrierRegularType> carrierRegularTypes; // 같은 실제 TYPE 정의의 기본 프레임/casttime이다.
    std::unique_ptr<RawContentPreviewProcess> contentPreviewProcess; // 같은 현재 시계/공유47타입의 일시 표시 수명이다.
    std::unique_ptr<ContentPreviewProcess> pendingContentPreview; // 원본 new18 성공 뒤 생성/부착까지의 단일 소유권이다.
    std::unique_ptr<RawCarrierRegular> carrierRegular; // 실제 내용물/자산/표시/기본Handle에 연결한 Carrier 부모다.
    std::unique_ptr<RawNuggetPostPop> nuggetPostPop; // 너겟의 실제 두 예약/난수/기본 처리다.
    std::unique_ptr<RegularProcess> pendingNuggetRegular; // 원본 new28과 생성자를 같은 실제 할당으로 연결한다.
    std::vector<std::int32_t> nuggetAFrames; // 같은188TYPE의A구간실제프레임수다.
    std::unique_ptr<RawNuggetRegular> nuggetRegular; // 기존너겟예약의왕복프레임/지면/낙하/삭제를소비한다.
    std::unique_ptr<RawCarrierFalling> carrierFalling; // 같은 표면/Player/권한에서 캐리어 낙하와 포획 사제 해방을 처리한다.
    std::unique_ptr<RawCarrierTask> carrierTask; // 실제 현재 Regular/SharedRegular task와 double 기한을 갱신한다.
    std::unique_ptr<RawCarrierFallStart> carrierFallStart; // 현재 이동 삭제/선택 해제/복귀 좌표/즉시 해방을 수행한다.
    CarrierTransferState carrierTransferState; // 미등록187 분기의 공유 주기/재시도/마지막 기증자와 현재 헤더다.
    std::unique_ptr<RawCarrierTransfer> carrierTransfer; // 실제 내용물 재부착/491/송신/이동 삭제와 연결한다.
    BoardTargetState boardTargetState; // 실제 월드 초기화 후의 실패점 두 float다.
    std::unique_ptr<RawBoardTarget> boardTarget; // 건설 Graph 순회와 다른 일반 접근점/변 중간점이다.
    std::unique_ptr<RawCarrierTarget> carrierTarget; // 같은 실제 Player AI/발자국/Path 시작과 귀환 선택을 공유한다.
    CarrierChoiceState carrierChoiceState; // 실제 후보의 경로 비교/현재 타입/임시 거리 전역이다.
    std::unique_ptr<RawCarrierChoice> carrierChoice; // 현재 Player+7c/AI/자산 목록과 같은 BFS를 사용한다.
    std::unique_ptr<RawPathRetarget> pathRetarget; //489ff0의 실제 대상 재확인을 현재 내용물/선택/이동에 연결한다.
    GeyserMinedState geyserMinedState; // 현재 권한/타입을 각 수신 직전 실제 전역에서 공급한다.
    FormPacketHeader miningHeader{0x5b,1,6}; //40ac20의 실제 초기 메시지/프로토콜/길이다.
    std::unique_ptr<RawGeyserMined> geyserMined; // 같은 난수/공간/가상 삭제/생성의 기존 원본 수량 몸체다.
    std::unique_ptr<RawGeyserLifetime> geyserLifetime; // 고갈한 지면의 고유 AI/목록/기본 프레임과 공통 수명이다.
    std::unique_ptr<RawCarrierMining> carrierMining; // 실제 너겟 생성 뒤 송신→로컬 수신을 실행한다.
    CarrierEscapeState carrierEscapeState; // 실제 겹친 방향표17DWORD다.
    std::unique_ptr<RawCarrierEscape> carrierEscape; // 현재 경로 깊이/공간/임시BFS/새이동을 연결한다.
    std::unique_ptr<RegularProcess> pendingCarrierTask; // 원본 new28과 생성자가 공유하는 하나의 확보 메모리다.
    std::unique_ptr<GumpFormHost> gumpForms; // 같은Pool의실제Form3와현재UI등록ID/부모삭제알림수명이다.
    std::unique_ptr<RawContentParentNotice> contentNotice; // 부모의 실제 현재 VT로 기본/파생 +24를 분배한다.
    bool contentSavedGame{}; // 이 생성자는 새 미션 경로이며005c85d4 저장 게임은 선택하지 않는다.
    std::array<std::int32_t,2> contentScreenOrigin{}; // 같은 GameWorld의 현재59a91c/920이며 SetParticleView에서 함께 공급한다.
    std::array<std::uint32_t,2> contentCamera{}; // 같은 scroll pixel을497490으로 변환한59a900/904 float 비트다.
    std::uint32_t contentBombBlocked{}; // 005453e8의 초기0이며 폭탄 시각 효과 억제 전역이다.
    std::unique_ptr<RawCarrierContentNotice> carrierContentNotice; // 실제 Walker/Carrier/사제의 가상+24 전체 몸체다.

    std::unique_ptr<RawContainedCreate> contained; // 실제 Factory/ContentFormHost의 저장 내용물 생성이다.
    ContentLifetimeState contentLifetimeState; // 같은 ContentForm의 커서·로컬 소유자·수입 명령 전역이다.
    std::uint32_t contentGumpType{3},contentEvent{0x02000031}; // PATCH0054107c/005427bc의 실제 초기값이다.
    std::unique_ptr<RawPlayerContentNotice> playerContent; // 현재 SP 저장소/같은 Player 체인/실제 UI를 조회한다.
    std::unique_ptr<RawCampaignLifetimeBindings> lifetime; // 공통 삭제와 ContentForm의 같은 풀 수명을 등록한다.
    std::unique_ptr<RawPlayerTechnology> technology; // 실제 시작/AI 갱신이 공유할 같은 Player 기술 체인이다.
    std::array<std::uint32_t,256> restrictions{}; // 실제 미션과 생산이 공유하는 금지DWORD 표이며 256번째 칸을 별도로 보존한다.
    std::unique_ptr<RawTotalMade> totalMade; // 같은 PostPop의 세 카운터와 위 금지 표를 읽는다.
    std::unique_ptr<RawProductionDeck> productionDeck; // 현재 Player+68의 실제 블록을 단독 소유한다.
    std::uint32_t fullDeck{},freeTechnologyType{153},templeProduct{133}; // PATCH0054db14/005412bc/0054126c의 현재 생산 전역이다.
    std::unique_ptr<RawCampaignProductionBindings> production; // 같은 현재 Factory/기술 내용물/허용 표를 조합한다.
    DamageablePreDestroyState damageablePreState; // 같은 사제 타입을 읽는 실제 공통 삭제 전역이다.
    DamageableReleaseState damageableReleaseState; // 해방 요청마다 같은 현재 전투 모드를 공급한다.
    DamageablePostDestroyState damageablePostState; // 삭제 후의 현재 전투/권한 전역이다.
    std::unique_ptr<RawDamageableRelease> damageableRelease; // 같은 풀/내용물/사제 실제 생성으로 해방한다.
    std::unique_ptr<RawDamageablePreDestroy> damageablePre; // 공통 삭제와 같은 깊이/Graph/장부를 사용한다.
    std::unique_ptr<RawFlyerPreDestroy> flyerPre; //144의 현재+8 WORD 대상 삭제 뒤 실제 Damageable Pre다.
    std::unique_ptr<RawDamageablePostDestroy> damageablePost; // 같은 실제 예약/다리/공통 후처리다.
    std::unique_ptr<RawVortexPreDestroy> vortexPre; // 실제 신전의 목록/섬/Player 정리를 담당한다.
    std::unique_ptr<RawBuriedPostPop> buriedPostPop; // 같은 boss/로딩 전역과 공통 장부를 사용하는 저장 buried 후처리다.
    std::unique_ptr<RawBuriedRegular> buriedRegular; // 같은 현재 Pool/Frame/공유 난수의 오벨리스크 사건 처리다.
    std::vector<float> fenceRanges; // 현재 불변 TYPE 표에서 읽은188개 실제 사거리다.
    std::uint32_t fenceShieldType{170}; //PATCH00541300의 실제 포스 필드(fenceShield) 타입이다.
    std::uint32_t fenceNodeType{90}; //PATCH005411c0의 별도 내부 fencemark 타입이다.
    std::unique_ptr<RawFenceNeighbors> fenceNeighbors; // 같은 해시/타입/현재 방향별 동맹의 실제 최근접 검색이다.
    std::unique_ptr<RawFenceLine> fenceLine; // 실제 기존 연결선 검색과 같은 월드의 가상 삭제다.
    std::unique_ptr<RawFencePostPop> fencePostPop; // 같은 공통 장부/이웃/연결선/Regular의 전체 펜스 접두다.
    std::unique_ptr<RawFenceRegular> fenceRegular; // 같은 현재 프레임/소유자/연결수의 실제 Regular 제어다.
    std::unique_ptr<RawFenceLifetime> fenceLifetime; // 실제 연결 정리/동맹 이웃 갱신/펜스 PreDestroy다.
    std::unique_ptr<RegularProcess> fencePendingRegular; // 원본 new28 성공 뒤 부착 전의 단일 실제 예약 객체다.
    std::unique_ptr<RawFenceMarkerClear> fenceMarkerClear; //같은 hash/가상 삭제의 기존 fencemark 제거다.
    std::unique_ptr<RawFenceMarkerCreate> fenceMarkerCreate; //같은 실제 중심/생성/통지의 내부 표식 생성이다.
    std::unique_ptr<RawFenceShieldCreate> fenceShieldCreate; //같은 현재TYPE/프레임의 화면 포스 필드 생성이다.
    std::unique_ptr<RawFenceRebuild> fenceRebuild; //같은 동맹 최근접/사거리/두 생성기로 연결을 재생성한다.
    std::unique_ptr<RawSunAviaryProcess> sunAviary; // 같은 현재 Kernel/시계/풀의 실제 훨리베이스 프로세스 몸체다.
    std::unique_ptr<RawRainAviaryProcess> rainAviary; // 같은 현재 Kernel/풀의 맨 오 워 풀 전용 상태 기계다.
    std::unique_ptr<RawFlyerProcess> flyer; // 같은 실제 Host의 공통 비행 생성/전송/홈 연결이다.
    std::unique_ptr<RawFlyerFrame> flyerFrame; // 같은 현재 시각/부모/표적/소리 장부의 실제 비행 Frame 제어다.
    std::unique_ptr<RawFlyerTarget> flyerTarget; // 같은 현재 풀/의존/실제 공간 검색의 표적 선택·경쟁·포위다.
    std::unique_ptr<RawSquidFinder> flyerFinder; // 한 비행체 표적 검색의 같은 일반 finder 커서를 보관한다.
    std::uint32_t flyerProcessEnd{63},flyerDependType{1}; // PATCH의 실제 프로세스 상한/의존 타입 전역이다.
    std::uint32_t windFlyerType{138}; // PATCH00541280의 현재 더스트 데빌 타입이다.
    std::unique_ptr<RawFlyerPostPop> flyerPostPop; // 맨 오 워의 실제 프로세스/그림자/압축 시각 후처리다.
    std::uint32_t flyerProcessType{31}; // PATCH005410ec의 현재 공통 비행 프로세스 타입이다.
    std::array<std::uint8_t,64> flyerPacket{};bool writingFlyer{}; // 실제0x39 상태 writer와 송신의 단일 버퍼/선택 표식이다.
    std::unique_ptr<FlyerProcess> pendingFlyer; // 기본 생성자의 부착 경계까지 단일 프로세스 소유권을 보관한다.
    std::uint32_t rainAviaryType{60},rainFlyerType{144}; // PATCH00541160/00541298의 현재 원본 타입 전역이다.
    std::array<std::uint8_t,64> rainAviaryPacket{};bool writingRainAviary{}; // 실제0x73 writer와 송신이 공유하는 버퍼/분배 표식이다.
    std::uint32_t sunAviaryType{32},sunFlyerType{73}; // PATCH005410f0/0054117c의 현재 원본 타입 전역이다.
    std::array<std::uint8_t,64> sunAviaryPacket{};bool writingSunAviary{}; // 현재 writer/송신이 공유하는 패킷과 분배 표식이다.
    std::array<FortTemplate,9> templates; // 실제 선택 자료는1번 Template에 둔다.
    std::array<FortTerritoryStream,9> streams; // 같은 Template의 실제120BYTE view다.
    std::array<std::uint8_t,180> headers{}; // 실제 Template9 header의 현재 바이트다.
    std::array<std::uint8_t,64> handle{}; // 현재 이름을 쓸 실제 저장소다.
    std::unique_ptr<RawCampaignWorldRuntime> runtime; // 위 실제 하위를 단일 캠페인 부모에 연결한다.
    PlayerPlacementAnchorState anchorState; // 현재 같은 Graph의 배치/도착 조회 전역이다.
    std::unique_ptr<RawPlayerPlacementAnchor> anchor; // 같은 hash0/장부를 직접 읽는 getter다.
    RawCampaignMotionState motionState; // 실제 Halt/Arrival/cleanup/BFS의 현재 상태다.
    std::uint32_t geyserType{122}; // 실제 현재 간헐천 자산 전역이다.
    bool snap{true},retrying{}; // 원본 사제 목적지 스냅/재시도 guard다.
    RawPathStartState start; // 같은 현재 시계/이동 타입을 참조한다.
    RawPathProcessState process; // 같은 Host의 경로 프로세스 전역이다.
    std::unique_ptr<RawCampaignMotionBindings> motion; // 실제 Start/BFS/도착/cleanup의 모듈 소유자다.
    ConstructionProcessState buildingState; // 같은 현재 게임 시각/권한/로컬 소유자의 실제30 전역이다.
    ConstructionEffectsState buildingEffectsState; // 완료 밀어내기가 현재 방향별 동맹을 읽는 저장소다.
    std::unique_ptr<RawConstructionEffects> buildingEffects; // 같은 해시/spot/가상 공간을 쓰는 원본 밀어내기 몸체다.
    std::unique_ptr<RawConstructionProcess> building; // 같은 Host의 실제30 시작/진행/완료를 소유한다.
    std::array<std::uint32_t,188> buildingMapTokens{}; // 원본 색표 초기화의 일반1/전용2/없는0 분류다.
    OwnerColorMaps buildingOwnerColors; // 일반 소유자 색 띠이며 다리는 별도 기존 분기로 취급한다.
    ShapeToBufferState buildingCaptureState; // 건설 현재 SHP의 원본 fixed16/추가 TYPE 기준점이다.
    std::unique_ptr<RawShapeToBuffer> buildingCapture; // 같은 실제 자산에서 건설 버퍼를 캡처한다.
    ConstructionBuilderState buildingMoverState; // 통지의 실제 운반 몸체가 유지하는 원본 상태이며 공급원은 같은 energySupply를 읽는다.
    std::unique_ptr<RawConstructionBuilder> buildingMover; // 같은 실제 기준점/23/39/현재 경로 시각의 통지용 Builder다.
    ConstructionArrivalState buildArrivalState; // 실제134의 예측 대기/서버 건설 시작 전역이다.
    std::unique_ptr<RawConstructionArrival> buildArrival; // 같은 풀/해시에서 건설 대상을 찾는다.
    ManaBoltArrivalState manaArrivalState; // 실제105의 도착 뒤 두 프로세스 종류/예약 간격이다.
    std::unique_ptr<RawManaBoltArrival> manaArrival; // 같은 풀에서 비행 도착의 실제 분기를 실행한다.
    ManaBoltRegularState manaRegularState; // 실제105 주기의 현재0.05/간격16 전역이다.
    std::unique_ptr<RawManaBoltRegular> manaRegular; // 같은 Target/Frame/현재 카메라/가상 공간을 연결한다.
    std::unique_ptr<RegularProcess> manaPendingRegular; // new28 성공 뒤 실제 부착까지 소유하는 예약이다.
    RawPathDataHeader pathHeader; // 실제 경로 writer가 사용하는 현재 프로토콜 header다.
    std::array<std::uint8_t,16384> pathBuffer{}; // 원본 경로 직렬화의 현재 송신 버퍼다.
    std::unique_ptr<RawBattleBroadcast> broadcast; // 동일 Player9와 실제 현재 연결 표를 사용하는 송신 분배기다.
    FormPacketHeaders formHeaders; // 원본 현재 감시/내용물/삭제 헤더 전역이다.
    FormDeletionStamp deletionStamp; // 실제 삭제 writer가 한 번 소비하는 최근 명령 표식이다.
    std::unique_ptr<RawFormPacket> formWriter; // 동일 Pool의 실제 가상+54/+58 직렬화 몸체다.
    std::array<std::uint8_t,2048> formBuffer{}; // 감시/내용물/삭제 요청이 공유하는 원본 크기의 현재 송신 버퍼다.
    RawCampaignConstructionState constructionState; // 실제 받침 확정/Notice 이력과 로컬 수량 전역의 장기 저장소다.
    SquidPostPopList recentGeysers; // 00595094/98의 전용 최근5목록이며 전체 assetState.geysers와 분리한다.
    std::uint8_t geyserMapSize{}; // 실제0052f7e6 초기BYTE0이며 PlayerBounds가 현재 참조로 읽는다.
    std::array<std::uint32_t,9> playerDirections{16448,4,0,6,2,7,3,5,1}; // 실제0053181c의 Player0까지 보존한 초기9DWORD다.
    std::unique_ptr<RawCampaignConstructionBindings> construction; // 같은 실제 HP/SP/장부/Player/지도에 첫 객체 전 설치한다.

    // 보호 자료를 읽기 전용으로 사용하며 모든 필수 후처리를 첫 생성 전에 등록한다.
    Impl(const GameAssets& game,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices input):
        services(std::move(input)),assets(game,files),metadata(assets.game),
        world(OriginalEdition::Patch1078,assets.types,assets.frames,metadata.shapes,CampaignLinks(),terrainType),core(world.Core()),
        sp(random,[this] { return seed.Reseed(); }),hp(core.pool,core.types,core.bookkeeping,rewardState,&sp),
        aiMemory(0x20000000,[this](std::span<std::uint8_t> bytes) {
            if (!services.ai.allocationBytes) throw std::logic_error("실제 Session AI 할당 입력 바이트 미연결");services.ai.allocationBytes(bytes);
        }),
        seed(random,crt,randomMode,services.unixSeconds),themes(core.types.size()),elements(core.types.size()),
        gunTypes(core.types.size()),hitTypes(core.types.size()),firingIntervals(core.types.size()),dudeSpeeds(core.types.size()),
        soundNames(RequireSounds(services)),archerTypes(core.types.size()),missileTypes(core.types.size()),projectileTypes(core.types.size()),
        start{core.processState,priestState.movement.dude,priestState.movement.teleport,priestState.movement.manaBolt,priestState.movement.priest,geyserType,snap,retrying},process{core.processState} {
        sp.BindTrackingOffset(priestState.regen.patch.sequence);sp.Initialize();
        level=std::make_unique<RawSquidLevel>(core.pool,core.types,SessionMissing<void,SquidLevelAssertion>());
        finishState.chaffMode=true;objectState.mission=true;objectState.chaffMode=true;
        playerState.playerKnown=1;assetState.boss=true;assetState.inBattle=true;assetState.localOwner=1;gunState.boss=true;
        broadcast=std::make_unique<RawBattleBroadcast>(core.pool.Edition(),players,services.battlePeers,assetState.localOwner,assetState.boss);
        // 별도 관찰/장치 송신이 없으면 실제 분배기를 사용한다. NULL 연결은 원본 조건에서 자연스럽게 생략한다.
        if (!services.assets.sendGunPacket) services.assets.sendGunPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        if (!services.pathPacket) services.pathPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        if (!services.formPacket) services.formPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        formWriter=std::make_unique<RawFormPacket>(core.pool,formHeaders,deletionStamp);
        // 실제 Form Pop이 정규화한 현재 상태 BYTE를 writer가 읽은 다음 같은 송신 분배기로 보낸다.
        if (!services.dependencyTransmit) services.dependencyTransmit=[this](Sid form,std::uint32_t) {
            const auto size=formWriter->WriteData(form,formBuffer); // 원본 반환 크기13/18이며 헤더의 선언 길이와 별개다.
            services.formPacket(std::span<const std::uint8_t>(formBuffer).first(size));
        };
        priestState.priests.priests.entries.resize(100);assetState.residences.entries.resize(100);
        assetState.temples.entries.resize(100);assetState.outposts.entries.resize(100);assetState.geysers.entries.resize(100);
        core.bookkeeping.factories.entries.resize(kCampaignFactoryCapacity); // 첫저장객체Pop전에예약하여원본Append의capacity0생략을방지한다.
        core.bookkeeping.providers.entries.resize(kEnergyProviderCapacity); // 원본4fcb60의256칸을 첫 실제 공급자 Pop 전에 확보한다.
        // 원본 Player의 첫 지역 sentinel은9기록에 직접 쓴다. 별도 playerRegions 배열을 사용하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            core.bookkeeping.ownerFactories[owner].entries.resize(kPlayerFactoryCapacity); // 첫 공장 Pop 이전에 확보하여 owner별 Append가 용량0 때문에 생략되지 않게 한다.
            Put(players,owner*0xb4+0x78,127);Put(players,owner*0xb4+0x54,1U<<owner);
            alliances[owner*9+owner]=1;
            gunTargetState.allyMasks[owner]=1U<<owner;gunState.allies[owner][owner]=true;
        }
        // 방향별 동맹/flag는 실제 현재 Player와 같은81DWORD를 각 총 판정 위치에서 읽는다.
        gunState.allianceRead=[this](std::uint32_t owner,std::uint32_t target) {
            if (owner>=9 || target>=9) throw std::out_of_range("실제 세션 현재 동맹 번호 오류");return alliances[owner*9+target];
        };
        gunTargetState.allyMaskRead=[this](std::uint32_t owner) {
            if (owner>=9) throw std::out_of_range("실제 세션 현재 동맹 mask 번호 오류");return Read(players,owner*0xb4+0x54);
        };
        gunTargetState.playerFlagsRead=[this](std::uint32_t owner) {
            if (owner>=9) throw std::out_of_range("실제 세션 현재 Player flag 번호 오류");return static_cast<std::uint32_t>(players[owner*0xb4+0x50]);
        };
        islands=std::make_unique<RawIslandRuntime>(core.pool,core.hash,islandBytes,finishState,assetState.residences,revision,ownerChanged,
            IslandRuntimeHooks{[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); },SessionMissing<void,IslandRuntimeAssertion>()});
        islands->Reset();
        auto priestEffects=services.priest;
        priestEffects.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        priestEffects.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        priestEffects.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        priestEffects.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        priestEffects.claimGround=[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); };
        // 사제 Owner의 004214a0도 동일한 실제 무상태 다리 진단 몸체다. 별도 AI 통지로 대체하지 않는다.
        if (!priestEffects.ownerNotice) priestEffects.ownerNotice=[this](Sid sid) { core.building.NotifySurface(sid); };
        priestEffects.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        priestEffects.dependentFound=SessionMissing<void>();priestEffects.reserveRegular=[] { return true; };
        priestEffects.shield.reserveSound=[] { return true; };
        sounds=std::make_unique<SoundProcessSystem>(core.processes,soundState,services.sound);
        priestEffects.shield.loadSound=[this](std::string_view name) { return soundNames.Lookup(name); };
        priestEffects.shield.attachSound=[this](Sid parent,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) {
            if (!sounds->Add(parent,sound,alternate,flags)) throw std::logic_error("실제 보호막 Sound47 부착 실패");
        };
        FillMissing(priestEffects.shield.noticeSound);FillMissing(priestEffects.shield.translate);FillMissing(priestEffects.shield.tell);
        // 4941f0 끝의 현재 좌표 priestFall.wav(flags0)를 같은 실제 위치 음향 장치로 전달한다.
        if (!priestEffects.fallSound) priestEffects.fallSound=[this](Sid,float x,float y) {
            if (!services.sound.playOnceAt) throw std::logic_error("사제 낙하 실제 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup("priestFall.wav"),0,0);
        };
        const auto otherWalkerFall=priestEffects.otherWalkerFall; // 별도 파생의 실제 외부 낙하 경계를 보존한다.
        priestEffects.otherWalkerFall=[this,otherWalkerFall](Sid sid) {
            if (carrierRegular && carrierRegular->Handles(sid)) { if (!carrierFallStart) throw std::logic_error("캐리어 낙하 시작 설치 전 호출");return carrierFallStart->Begin(sid); }
            if (!otherWalkerFall) throw std::logic_error("다른 실제 Walker 낙하 미연결");return otherWalkerFall(sid);
        };
        priestEffects.regen.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        FillMissing(priestEffects.regen.neutralAction);
        // 회전과 뒤의 읽기를 분리하여 중립 동작/측정이 바꾼 현재 SP 전역도 원본 시점에 조회한다.
        priestEffects.regen.patchAudit=[this] { sp.Rotate(); };
        priestEffects.regen.spReads=BindPriestRegenSpStore(sp,priestState.regen.patch);
        FillMissing(priestEffects.regen.measurement);
        priestEffects.regen.damageable=[this](Sid sid) { hits->Burning().Refresh(sid); };
        priests=std::make_unique<RawPriestWorldBindings>(RawPriestWorldCore{core.pool,core.hash,core.types,core.frames,core.spots,core.hash.Entries(0),hp,random,core.factory,core.owner,core.ownerMode,core.postPop,core.frame},priestState,std::move(priestEffects));
        priests->BindProcesses(core.processes);priests->SetAuthority(true);priests->SetLocalPlayer(1);
        // 첫객체전에같은실제Carrier/목록과현재타입참조만연결하며ownerState는여기서갱신하지않는다.
        priestParticipation=std::make_unique<RawPriestParticipation>(core.pool,core.hash,core.types,priests->Priests(),priests->Carrier(),
            PriestParticipationState{priestState.movement.priest,runeState.altarType,runeState.daisType});
        PlayerWorldHooks playerHooks;
        playerHooks.assertion=SessionMissing<void,PlayerWorldAssertion>();
        playerHooks.find=[this](float x,float y,std::uint32_t type) { RawSquidFinder finder(core.pool,core.hash,core.types);return finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},0,[this,type](Sid sid) { return core.pool.Slot(sid)[10]==type; }); };
        playerHooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        playerHooks.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };
        playerHooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        playerHooks.reserveRegular=[] { return true; };
        playerHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        player=std::make_unique<RawPlayerWorld>(core.pool,core.postPop,players,playerState,std::move(playerHooks));core.postPop.SetPlayerPostPop(player.get());
        templateState=std::make_unique<RawPlayerTemplateState>(core.pool.Edition(),headers,stateStreams,stateNames,
            objectState.loadingDepth,objectState.mission,SessionMissing<void,PlayerTemplateStateAssertion>(),[this](std::uint32_t depth) { world.SetLoadingDepth(depth); });
        auto playerRegularHooks=services.player;playerRegularHooks.assertion=SessionMissing<void,PlayerRegularAssertion>();
        FillMissing(playerRegularHooks.log);
        playerRegularHooks.reserveRegular=[] { return true; };
        playerRegularHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        playerRegularHooks.templateState=[this](std::uint32_t slot) { return templateState->Get(slot); };
        playerRegularHooks.templateChanged=[this](std::uint32_t slot) { templateState->Changed(slot); };
        playerRegular=std::make_unique<RawPlayerRegular>(*player,core.ownerMode,std::move(playerRegularHooks));
        auto ownerEffects=services.ownership;
        ownerEffects.otherOwner=[this](Sid sid,std::uint32_t owner) { priests->SetOwner(sid,owner); };
        FillMissing(ownerEffects.refreshProduction);FillMissing(ownerEffects.themeMode);
        ownerEffects.missingIsland=SessionMissing<void,Sid,std::int32_t,std::uint32_t>();
        ownership=std::make_unique<RawCampaignOwnership>(world,*islands,assetState,players,metadata.frames,themes,random,paint,votes,vortexRevision,97,121,std::move(ownerEffects));
        carrier=std::make_unique<RawCarrierPostPop>(core.pool,core.types,carrierState,
            MakeCarrierPostPopHooks(core.pool,core.postPop,[this](Sid sid) { static_cast<void>(priests->CheckCarrier(sid)); },[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); }));
        // 빈/내장 타입을 포함해 실제 TYPE+98의 판본 초기값과 파싱값을 같은 번호의 모든 소비자에게 전달한다.
        for (std::size_t type=0;type<core.types.size();++type) {
            gunTypes[type].theme=core.types[type].theme;themes[type]=std::bit_cast<std::uint32_t>(core.types[type].theme);elements[type]=core.types[type].theme;
            builderTypes[type].sourceFrameIndex=std::bit_cast<std::uint32_t>(core.types[type].theme);
            // TYPE+130의 실제16개 첫 번호이며 없는Run의-1도 보존한다.
            for (std::uint8_t index=0;index<16;++index) builderTypes[type].firstFrames[index]=core.frames[type].Run(static_cast<std::uint8_t>('A'+index)).first;
        }
        // 보호 .type의 실제 숫자/문자 Run을 같은 TYPE 번호의 보충 표에 연결한다.
        for (const auto& asset:assets.game.Types()) {
            const auto type=static_cast<std::uint32_t>(70+asset.block);auto& target=gunTypes[type];target.range=static_cast<float>(asset.definition.Number("range").value_or(0));
            target.heightExtension=static_cast<int>(asset.definition.Number("height").value_or(0));
            dudeSpeeds[type].speed=static_cast<float>(asset.definition.Number("speed").value_or(0));
            builderTypes[type].speed=dudeSpeeds[type].speed;
            // 원본 방향별 L/M/N/O 첫 프레임 표다. 없는Run의-1은 실제 검색 몸체가 처리한다.
            for (std::uint8_t direction=0;direction<4;++direction) target.directionFrames[direction]=core.frames[type].Run(static_cast<std::uint8_t>('L'+direction)).first;
            hitTypes[type].damagePerSecond=static_cast<float>(asset.definition.Number("hpPerSec").value_or(0));
            firingIntervals[type]=static_cast<float>(asset.definition.Number("delayBetweenShots").value_or(0));
            archerTypes[type]=ReadRawArcherTypeData(asset.definition);missileTypes[type].range=target.range;
            missileTypes[type].frameCount=archerTypes[type].directions;
            // 원본 TYPE의 실제 키는 fireSound이며 firingSound라는 별칭은 없다.
            const auto firingSound=asset.definition.String("fireSound"),impactSound=asset.definition.String("impactSound");
            missileTypes[type].firingSound=firingSound && !firingSound->empty() ? soundNames.Lookup(*firingSound) : 0;
            missileTypes[type].impactSound=impactSound && !impactSound->empty() ? soundNames.Lookup(*impactSound) : 0;
            auto& geometry=projectileTypes[type];geometry.defaultFrame=asset.definition.specialFrames.defaultFrame;
            geometry.range=target.range; // 방향 불일치 사정선 보정도 동일 TYPE의 현재 사정거리를 사용한다.
            geometry.frameCount=archerTypes[type].frameCount;geometry.directions=archerTypes[type].directions;geometry.heightExtension=target.heightExtension;
            // 각 물리 SHP header 마지막 두 WORD를 그대로 읽으며 표시 hotspot과 혼용하지 않는다.
            for (std::size_t frame=0;frame<core.shapes[type].frames.size();++frame) {
                const auto header=assets.game.Shapes().SquidHeader(asset.block,frame);
                geometry.shotOffsets.push_back({std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[32]|(std::uint16_t(header[33])<<8))),
                    std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[34]|(std::uint16_t(header[35])<<8)))});
            }
        }
        gunMode=std::make_unique<RawGunMode>(core.pool,core.frames,gunState.windArcherType);
        gunTarget=std::make_unique<RawGunTarget>(core.pool,core.hash,core.types,gunTypes,core.shapes,gunTargetState);
        EnergySupplyHooks energyHooks;energyHooks.providerEntry=EnergyProviderEntry;energyHooks.requirement=BindAiTypeRequiredEnergy(assets.game,core.types);
        EnergySupplyReads energyReads;energyReads.editor=[this] { return globals.editor; };energyReads.special=[this] { return playerState.special ? 1U : 0U; };energyReads.mission=[this] { return globals.missionPost ? 1U : 0U; };
        energySupply=std::make_unique<RawEnergySupply>(core.pool,core.types,core.bookkeeping.providers,energySources,alliances,cursorManaGlobals.mapLimit,energyState,std::move(energyHooks),std::move(energyReads));
        CursorManaHooks cursorManaHooks;BindCursorManaWorld(cursorManaHooks,world,[this](Sid sid) { return gunTarget->Center(sid); });
        cursorMana=std::make_unique<RawCursorMana>(core.pool,core.types,core.processState,cursorManaGlobals,cursorDrop,std::move(cursorManaHooks));
        CursorRangeHooks rangeHooks;BindCursorManaWorld(rangeHooks.world,world,[this](Sid sid) { return gunTarget->Center(sid); });
        rangeHooks.range=[this](std::uint32_t type) { if (type>=gunTypes.size()) throw std::out_of_range("현재 사정거리 TYPE 범위 오류");return gunTypes[type].range; };
        rangeHooks.firstContent=[this](Sid parent) { return priests->Carrier().FindContained(parent,1,0,0); };
        rangeHooks.mode=[this](Sid parent) { return gunMode->Get(parent); };
        rangeHooks.cannonDirection=[this](const GunTargetQuery& query,std::int32_t direction) { return gunTarget->CannonDirection(query,direction); };
        rangeHooks.footprint=[this](Sid parent) { return gunTarget->Footprint(parent,false); };
        rangeHooks.directionFrame=[this](Sid parent,std::int32_t direction) { return gunTarget->DirectionFrame(core.pool.Slot(parent)[10],direction); };
        rangeHooks.firingOffset=[this](std::uint32_t type,std::int32_t frame) {
            if (type<70 || type-70>=assets.game.Types().size() || frame<0) throw std::out_of_range("실제 사정거리 발사구 자산 범위 오류");
            const auto header=assets.game.Shapes().SquidHeader(assets.game.Types()[type-70].block,static_cast<std::uint32_t>(frame));
            return std::array{std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[32]|(std::uint16_t(header[33])<<8))),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[34]|(std::uint16_t(header[35])<<8)))};
        };
        rangeHooks.frameRun=[this](std::uint32_t type,std::uint32_t variant) { if (type>=core.frames.size() || variant>=16) throw std::out_of_range("실제 사정거리 Run 범위 오류");return core.frames[type].Run(static_cast<std::uint8_t>('A'+variant)); };
        rangeHooks.diagnostic=services.cursorRangeDiagnostic;
        cursorRange=std::make_unique<RawCursorRange>(core.pool,core.types,core.processState,cursorRangeGlobals,cursorDrop,std::move(rangeHooks));
        priestDamageState.levelCorrectionDisabled=globals.missionPost;priestDamageState.boss=assetState.boss;
        priestDamageState.levelCorrectionDisabledRead=[this] { return globals.missionPost; };
        generalDamageState.serverRead=[this] { return core.processState.server; };
        generalDamageState.localOwnerRead=[this] { return static_cast<std::uint8_t>(assetState.localOwner); };
        generalDamageState.primaryIslandRead=[this](std::uint8_t owner) { return std::bit_cast<std::int32_t>(Read(players,owner*0xb4U+0x78)); };
        auto hitEffects=services.hit;
        hitEffects.priest.defaultFrame=[this](Sid sid) { return projectileTypes[core.pool.Slot(sid)[10]].defaultFrame; };
        AiDamageHooks aiDamageEffects; // 현재 비행 홈과 실제 로그 출력만 외부 경계에 연결한다.
        aiDamageEffects.home=[this](Sid sid) {
            const auto raw=core.pool.Slot(sid);if (raw[11]&3U) return Sid{};
            const auto form=FindAttachedProcess(core.pool,sid,31);if (!form.value) return Sid{};
            auto* process=dynamic_cast<FlyerProcess*>(core.processes.FromForm(form));
            if (!process) throw std::logic_error("AI 피격 비행 홈의 실제 프로세스31 미연결");return process->Fields().Home();
        };
        aiDamageEffects.log=[this](std::uint32_t token,const AiDamageLog& value) {
            if (value.type>=core.types.size()) throw std::out_of_range("AI 피격 로그 현재 TYPE 범위 오류");
            const auto text=std::string("I hate ")+(value.more ? "MORE " : "")+core.types[value.type].name+" at ("+std::to_string(value.x)+","+std::to_string(value.y)+")\n";
            const auto raw=aiMemory.Bytes(token);if (!Read(raw,0xb00)) return;
            if (!services.player.log) throw std::logic_error("AI 피격 실제 로그 출력 소비자 미연결");
            services.player.log(29,"AI"+std::to_string(std::bit_cast<std::int32_t>(Read(raw,0x260)))+": "+text+"\n");
        };
        aiDamage=std::make_unique<RawAiDamage>(aiMemory,core.pool,core.types,std::move(aiDamageEffects));
        playerDamage=std::make_unique<RawPlayerDamageNotice>(players,core.ownerMode,
            [this](std::uint32_t token,Sid victim,Sid attacker) { aiDamage->Notice(token,victim,attacker); });
        // 실제 Player AI가 없는 자연 반환은 래퍼가 판단한다. 비영 AI의 미복원 몸체는 명시 오류를 유지한다.
        if (!hitEffects.priest.ownerDamageNotice) hitEffects.priest.ownerDamageNotice=
            [this](std::uint8_t owner,Sid victim,Sid attacker) { playerDamage->Notice(owner,victim,attacker); };
        if (!hitEffects.general.ownerDamageNotice) hitEffects.general.ownerDamageNotice=
            [this](std::uint8_t owner,Sid victim,Sid attacker) { playerDamage->Notice(owner,victim,attacker); };
        if (!hitEffects.general.realTime) hitEffects.general.realTime=services.sound.wallSeconds;
        if (!hitEffects.general.warningSound) hitEffects.general.warningSound=[this] {
            if (!services.sound.play) throw std::logic_error("실제 섬 피격 전역 음향 소비자 미연결");
            services.sound.play(soundNames.Lookup("islandUnderAttack.wav"),0,0,0,1,0);
        };
        if (!hitEffects.general.warningText) hitEffects.general.warningText=[this] {
            if (!services.priest.shield.tell) throw std::logic_error("실제 섬 피격 Tell 소비자 미연결");
            services.priest.shield.tell("islandUnderAttack");
        };
        FillMissing(hitEffects.general.realTime);FillMissing(hitEffects.general.warningEvent);
        // MessageBoard::Show의 현재00594fa4 억제 조건을 같은 미션 전역에서 읽고 실제 UI 삭제에 적용한다.
        hitEffects.general.warningEvent=[this,remove=std::move(hitEffects.general.warningEvent)] { if (!globals.missionPost) remove(); };
        FillMissing(hitEffects.burning.create);FillMissing(hitEffects.burning.bulfSound);
        FillMissing(hitEffects.dudes.createBurning);FillMissing(hitEffects.dudes.pathEndpoint);FillMissing(hitEffects.dudes.startPath);
        hits=std::make_unique<RawCampaignHitBindings>(world,*priests,hp,random,dudeSpeeds,assetState.residences,
            priestDamageState,generalDamageState,burningState,impactState,std::move(hitEffects));
        // 미복원 가상 피해는 실제 공격자/표적 TYPE를 기록해 다음 원본 복원 지점을 식별한다.
        gunHit=std::make_unique<RawGunHit>(core.pool,core.types,hitTypes,hits->DamageHooks(GunHitHooks{
            [this](Sid victim,std::int32_t,std::uint8_t,Sid attacker) {
                throw std::logic_error("실제 세션 총 피해 미복원: 공격 TYPE"+std::to_string(core.pool.Slot(attacker)[10])+" → 표적 TYPE"+std::to_string(core.pool.Slot(victim)[10]));
            },[this](Sid sid) { core.building.NotifySurface(sid); }}));
        projectileGeometry=std::make_unique<RawProjectileGeometry>(core.pool,core.types,projectileTypes,16.0F,11.0F,gunMode.get());
        fenceTarget=std::make_unique<RawFenceTarget>(core.pool,core.hash,core.types,projectileTypes,*gunTarget,gunTargetState,gunState);
        shrapnelMapTokens[144]=1;shrapnelOwnerColors=BuildStandardOwnerColorMaps(144,core.bridgeState.bridgeType);shrapnelCaptureState.ownerTypeMaps=shrapnelMapTokens;
        shrapnelCaptureState.addTypeHotspot=true; // 실제00542414의 PE 초기 DWORD는1이며 캡처 후 TYPE 기준점을 더한다.
        shrapnelCaptureState.ownerColors=[this](std::uint32_t type,std::uint32_t owner)->std::span<const std::uint8_t> {
            if (type!=144 || owner>=core.bridgeState.ownerColors.size()) throw std::out_of_range("파편 캡처 현재 타입/owner 색표 오류");
            const auto color=core.bridgeState.ownerColors[owner];if (color<0 || color>8) throw std::out_of_range("파편 캡처 현재 소유자 색 번호 오류");return shrapnelOwnerColors[static_cast<std::size_t>(color)];
        };
        shrapnelCapture=std::make_unique<RawShapeToBuffer>(core.pool,core.types,core.shapes,assets.game.Shapes(),shrapnelCaptureState);
        CampaignShrapnelHooks shrapnel;shrapnel.crt=&particleCrt;shrapnel.lifetime.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        shrapnel.lifetime.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        shrapnel.lifetime.depend=[this](Sid form,Sid dummy) { static_cast<void>(core.dependencies.Create(form,dummy)); };
        shrapnel.lifetime.capture=[this](Sid sid) { if (!Flyer(sid)) throw std::logic_error("다른 실제 파편 source의 전용 색표/추가 그림 미복원 TYPE"+std::to_string(core.pool.Slot(sid)[10]));return shrapnelCapture->Capture(sid); };
        shrapnel.destroyUnpaired=[this](Sid sid) { world.Destroy(sid,0); };
        particles=std::make_unique<RawCampaignParticles>(core,random,assets.game.Palette(),[this](float x,float y,std::string_view name) {
            if (!services.sound.playOnceAt) throw std::logic_error("캠페인 빔 실제 파일명 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),0,0);
        },std::move(shrapnel));
        auto combatEffects=services.archer;
        // 반더는 빔 생성 뒤 초기 source/target snapshot으로 피해/회피를 처리한다.
        combatEffects.beam.create=[this](Sid source,Sid target) { static_cast<void>(particles->AddBeam(source,target,std::string{"shortZap.wav"})); };
        combatEffects.beam.refresh=[this](Sid source) { gunHit->SetTime(source,core.processState.now); };
        combatEffects.beam.hit=[this](Sid source,Sid target) { gunHit->Hit(source,target,core.processState.now); };
        // 실제 SID의 현재 좌표와 같은 이름표/위치 재생 함수를 사용한다. 설치 전 world_ UI 포인터를 캡처하지 않는다.
        if (!combatEffects.animation.common.sound) combatEffects.animation.common.sound=[this](Sid sid,std::string_view name) {
            const auto raw=core.pool.Slot(sid); // 비정렬 원본 float 좌표를 읽는 현재 부모 슬롯이다.
            std::array<std::uint32_t,2> bits{}; // little endian x/y의 원본 DWORD 비트다.
            // 두 좌표의 각 네 바이트를 정확한 자리로 조립한다.
            for (std::size_t i=0;i<4;++i) { bits[0]|=std::uint32_t(raw[14+i])<<(8*i);bits[1]|=std::uint32_t(raw[18+i])<<(8*i); }
            services.sound.playOnceAt(std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),soundNames.Lookup(name),0,0);
        };
        if (!combatEffects.animation.common.targetPoint) combatEffects.animation.common.targetPoint=
            [this](Sid firing,Sid target) { return fenceTarget->Point(firing,target); };
        combatEffects.missile.muzzle=[this](Sid sid) { return projectileGeometry->Muzzle(sid); };
        // 현재 썬 캐논 프레임의 허용 방향과 실제 사정거리로 보정한 사정선 종점을 조회한다.
        combatEffects.missile.rayEnd=[this](std::array<float,2> from,std::array<float,2> to,Sid sid) { return projectileGeometry->CannonRayEnd(from,to,sid); };
        combatEffects.missile.hotspot=[this](Sid sid) { return projectileGeometry->Hotspot(sid); };
        combatEffects.missile.directionFrame=[this](std::array<float,2> from,std::array<float,2> to,std::uint32_t type) { return projectileGeometry->DirectionFrame(from,to,type); };
        combatEffects.missile.targetDimensions=[this](Sid sid) { return projectileGeometry->Dimensions(sid); };
        combatEffects.missile.targetBounds=[this](Sid sid,std::array<float,2> point) { return projectileGeometry->TargetBounds(sid,point); };
        combatEffects.missile.flightEffect=[this,other=std::move(combatEffects.missile.flightEffect)](std::uint32_t type,Sid parent) {
            if (type==11) { particles->Add(type,parent);return; }
            if (!other) throw std::logic_error("실제 비행 표시 효과 미복원 TYPE"+std::to_string(type));other(type,parent);
        };
        // 파생 음향 재정의가 없으면 같은 SoundPlayer의 실제 위치 한 번 재생 경계로 전달한다.
        const auto missileSound=[this](float x,float y,std::uint32_t token) {
            if (!services.sound.playOnceAt) throw std::logic_error("캠페인 투사체 실제 위치 음향 미연결");
            static_cast<void>(services.sound.playOnceAt(x,y,token,0,0));
        };
        if (!combatEffects.missile.firingSound) combatEffects.missile.firingSound=missileSound;
        if (!combatEffects.missile.impactSound) combatEffects.missile.impactSound=missileSound;
        combatEffects.missile=hits->MissileHooks(*gunHit,missileState,std::move(combatEffects.missile));
        combat=std::make_unique<RawCampaignArcherBindings>(world,*gunTarget,random,archerTypes,metadata.frames,missileTypes,archerState,missileState,std::move(combatEffects));
        auto assetEffects=services.assets;assetEffects.postPop.reserveRegular=[] { return true; };
        assetEffects.postPop.nuggetPostPop=[this](Sid sid,std::uint32_t flags) { if (!nuggetPostPop) throw std::logic_error("현재 너겟 PostPop 미설치");nuggetPostPop->PostPop(sid,flags); };
        // 거주 구역은 현재 일치 신전을 찾은 호출 위치에서 실제 theammode를 다시 읽는다.
        if (!assetEffects.regular.themeMode) assetEffects.regular.themeMode=services.ownership.themeMode;
        FillMissing(assetEffects.regular.themeMode);
        assetEffects.gun=MakeCampaignGunQueryHooks(core,*gunMode,*gunTarget,*gunHit,gunState,firingIntervals,std::move(assetEffects.gun));
        assetEffects.gun=MakeGunTargetHooks(core.pool,*gunTarget,std::move(assetEffects.gun));
        assetEffects.gun=combat->GunHooks(std::move(assetEffects.gun));
        assetEffects.factory.warnDuplicate=SessionMissing<void,Sid,std::int32_t,std::int32_t>();
        assetEffects.levelAssertion=SessionMissing<void,SquidLevelAssertion>();
        assetEffects.factory.regionOwnership=[this](float x,float y) { ownership->UpdateRegion(x,y); };
        assetEffects.postPop.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        assetEffects.postPop.gunFrame=[](std::int32_t frame) { return static_cast<std::int32_t>(PatchGunGraphFromFrame(frame)); }; // 실제00467610의 프레임 표 조회다.
        assetEffects.postPop.setGraph=[this](Sid sid,std::int32_t number) { core.pool.AllocatedBytes(sid)[30]=static_cast<std::uint8_t>(number); }; // 실제004ad3e0은 Graph BYTE만 쓴다.
        // PATCH0040d980은 현재float를절삭한 뒤16으로 나눈 청크의 실제 BYTE를 읽는다.
        assetEffects.postPop.regionNumber=[this](float x,float y) { return static_cast<std::int32_t>(chunks.At(static_cast<int>(x)/16,static_cast<int>(y)/16).island); };
        // 실제 TYPE 사운드 이름을 클라이언트의 단일 이름표에서 조회한다. TYPE번호를 토큰으로 대신하지 않는다.
        assetEffects.postPop.typeDword=[this](Sid sid,std::uint32_t offset) {
            if (offset!=0x74) throw std::logic_error("세션 TYPE 사운드 필드 미지원");
            const auto name=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("impactSound");
            return name && !name->empty() ? soundNames.Lookup(*name) : 0U;
        };
        FillMissing(assetEffects.postPop.playImpactSound);FillMissing(assetEffects.queueCommand);FillMissing(assetEffects.sendGunPacket);
        assetEffects.postPop.sunAviaryPostPop=[this](Sid sid,std::uint32_t flags) { sunAviary->PostPop(sid,flags); }; // 첫 실제 Pop 전에 아래에서 전체 몸체를 연결한다.
        assetEffects.postPop.rainAviaryPostPop=[this](Sid sid,std::uint32_t flags) { rainAviary->PostPop(sid,flags); }; // 같은 실제 TYPE143의 가상 후처리다.
        assetEffects.postPop.sunFencePostPop=[this](Sid sid,std::uint32_t flags) { fencePostPop->PostPop(sid,flags); }; // 공간 쓰기 전에 전체 펜스 후처리를 조립한다.
        assetBindings=std::make_unique<RawCampaignAssetBindings>(world,*carrier,assetState,random,*islands,players,playerState,RawCampaignAssetGlobals{regularState,gunState,elements,archerCommand},std::move(assetEffects));assetBindings->ActivatePostPop();
        RawWorldRuntimeHooks worldHooks;
        worldHooks.formTransmit=services.dependencyTransmit;
        FillMissing(worldHooks.formTransmit);
        worldHooks.assets=services.deletion;FillMissing(worldHooks.assets.selected);
        // 공통 삭제의 -1/-2/0/flags 요청은 실제+58을 실행한다. 다른 파생/선택 효과는 실제 상위 소비자로 전달한다.
        worldHooks.assets.emit=[this,other=services.deletion.emit](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::Transmit) {
                const auto size=formWriter->WriteDeletion(event.sid,event.flags,formBuffer); // 아직 반납 전인 같은 SID다.
                services.formPacket(std::span<const std::uint8_t>(formBuffer).first(size));return;
            }
            if (!other) throw std::logic_error("실제 세션의 다른 파생 삭제/선택 효과 미연결");
            other(event);
        };
        worldHooks.dependencyNotice=[this](Sid watcher,Sid watched,Sid form) {
            //110의 실제0048f0a0은 두 통지 인수를 무시하고 현재 가상+10(flags0)을 호출한다.
            if (VisualDummy(watcher)) { world.Destroy(watcher,0);return; }
            core.processes.NotifyDependency(watcher,watched,form);
        };
        InstallContents(worldHooks.assets);
        ConnectConstruction(); // 실제공통삭제후설치뒤같은noIsland VT의표면가상분배를첫객체전등록한다.
        ConnectDamageable(); // 같은 공통 삭제와 신전/사제 해방의 파생 하위를 첫 객체 전에 연결한다.
        ConnectCarrierRegular(); // 첫491예약/프레임 전에 같은 내용물 그림과 만료 삭제를 연결한다.
        ConnectSunAviary(); // 실제 타입32/flags8 부착과 현재48BYTE 필드를 같은 Kernel에 등록한다.
        ConnectRainAviary(); // 실제 타입60/flags8 부착과 별도 Frame/0x73 패킷을 구성한다.
        ConnectFlyer(); // TYPE144의 전체 후처리와 기본 표 부착→전용 표 쓰기의 실제 순서를 연결한다.
        ConnectFence(); // 실제 최근접/연결선 삭제/Regular를 첫 저장 펜스 Pop 전에 연결한다.
        BuriedPostPopHooks buried;buried.chooseType=SessionMissing<std::uint32_t,std::uint32_t>(); // 비로딩 최초 생성의 실제 보상 추첨은 별도 복원 전 명시 거부한다.
        buried.reserveRegular=[] { return true; }; // 실제 Host 할당 실패는 addRegular에서 예외로 전달한다.
        buried.addRegular=[this](Sid sid,std::uint32_t event,float delay) { SessionCheck(core.processes.AddRegular(sid,event,delay)!=nullptr); };
        buried.validateBase=[this](Sid sid,std::uint32_t flags,const RawGraphPop* projected) { core.postPop.ValidateBase(sid,flags,projected); };
        buried.base=[this](Sid sid,std::uint32_t flags) { core.postPop.PostPopBase(sid,flags); };
        buriedPostPop=std::make_unique<RawBuriedPostPop>(core.pool,assetState.boss,objectState.loadingDepth,std::move(buried));core.postPop.SetBuriedPostPop(buriedPostPop.get()); // 00462170의 실제00540bc4는 현재 boss이며 별도 server/전투 조건을 사용하지 않는다.
        worldHooks.assets=construction->DestroyHooks(lifetime->DestroyHooks(hits->DestroyHooks(std::move(worldHooks.assets))));
        worldHooks.destroy=construction->Destroy(lifetime->Destroy(hits->Destroy(services.destroy)));
        auto previousEffects=std::move(worldHooks.assets.emit); // 다른 기존 파생/전송/선택 처리기를 유지한다.
        worldHooks.assets.emit=[this,previous=std::move(previousEffects)](const SquidDestroyEvent& event) {
            if (geyserLifetime->Handles(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { geyserLifetime->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { geyserLifetime->PostDestroy(event.sid,event.flags);return; }
            }
            // 다이스 미리보기의 extra8은44afd0의 큰 일반 전처리를 건너뛴다. 실제 VT/타입/비트를 모두 확인한다.
            if (PreviewDais(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { damageablePre->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { DestroyPreviewDaisSlots(event.sid);damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            if (PlainDamageable(event.sid) || SunFence(event.sid) || Nugget(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { if (SunFence(event.sid)) fenceLifetime->PreDestroy(event.sid,event.flags);else damageablePre->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            if (PlainCarrierLifetime(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { priests->Carrier().PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            if (FenceCommon(event.sid) || VisualDummy(event.sid) || PriestForcefield(event.sid)) { // 실제 보호막도 +14/+18이 공통004b0950/004b0840이다.
                if (event.effect==SquidDestroyEffect::PreDestroy) { world.CommonDeletion().PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { world.CommonDeletion().PostDestroy(event.sid,event.flags);return; }
            }
            if (Flyer(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { flyerPre->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            if (Vortex(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { vortexPre->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            previous(event);
        };
        auto previousDestroy=std::move(worldHooks.destroy); // 실제 신전 VT의 공통+10만 추가 분배한다.
        worldHooks.destroy=[this,previous=std::move(previousDestroy)](Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) {
            if (geyserLifetime->Handles(sid) || Vortex(sid) || FenceCommon(sid) || Flyer(sid) || VisualDummy(sid) || PlainDamageable(sid) || SunFence(sid) || Nugget(sid) || PreviewDais(sid) || PriestForcefield(sid) || PlainCarrierLifetime(sid)) return core.virtualDestroy.Destroy(sid,flags,hooks);return previous(sid,flags,hooks);
        };
        buriedRegular=std::make_unique<RawBuriedRegular>(core.pool,core.frames,core.frame,random);
        auto actualRegular=hits->Handler(combat->Handler(assetBindings->Handler(buriedRegular->Handler(priests->RegularHandler([this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            if (playerRegular->Handles(sid)) return playerRegular->Handle(sid,event,count,payload);
            if (manaRegular && manaRegular->Handles(sid)) return manaRegular->Run(sid,event,count);
            if (nuggetRegular && nuggetRegular->Handles(sid)) return nuggetRegular->Handle(sid,event,count,payload);
            const auto raw=core.pool.Slot(sid);const bool priest=raw[10]==kPriestType && Read(raw,0)==kPatchPriestVtable;
            // 사제의 다섯 고유 사건은 파생 처리기가 담당한다. 나머지만 원본 기본 Carrier로 넘긴다.
            if (carrierRegular->Handles(sid) || (priest && (event<0x259U || event>0x25dU))) return carrierRegular->Handle(sid,event,count,payload);
            throw std::logic_error("캠페인 현재 Kernel의 다른 실제 Regular 부모 미연결: TYPE"+
                std::to_string(core.pool.Slot(sid)[10])+" event"+std::to_string(event));
        })))));
        actualRegular=fenceRegular->Handler(std::move(actualRegular)); //실제 가상 표가 맞는 펜스만 먼저 처리한다.
        // 실제 사건이 실패하면 당시 부모/사건/횟수를 남기고 같은 예외를 전파한다. 성공/상태 되돌림으로 대신하지 않는다.
        worldHooks.regular=[this,handler=std::move(actualRegular)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            try { return handler(sid,event,count,payload); }
            catch (...) { std::fprintf(stderr,"실제 Regular 실패 SID%u TYPE%u event%u count%u payload%08x\n",sid.value,sid.value<core.pool.Capacity() ? core.pool.Slot(sid)[10] : 0xffffU,event,count,std::bit_cast<std::uint32_t>(payload));throw; }
        };
        worldHooks.transmit=AviaryTransmission(assetBindings->Transmission());worldHooks.walkerFall=[this](Sid sid) { return priests->FallWalker(sid); };
        worldHooks.loadingDepth=[this](std::uint32_t depth) { objectState.loadingDepth=depth;carrierState.loadingDepth=depth;priests->SetLoadingDepth(depth);assetBindings->SetLoadingDepth(depth); };
        world.BindRuntime(std::move(worldHooks));
        placement.geometry=metadata.geometry;placement.placement.localPlayer=1;placement.permission.alliances.fill(1);
        pipeline=std::make_unique<RawCanonPlacementPipeline>(core.pool,core.hash,CanonPlacementPipelineData{core.types,metadata.frames,core.shapes,metadata.hotspots,core.spots,core.hash.Entries(0),{}},core.bookkeeping,additional,contentFinder,placement);
        NormalTerrainDiagnostics diagnostics{SessionMissing<void,TerrainContextAssertion>(),SessionMissing<void,TerrainStampAssertion>(),SessionMissing<void,SquidRegionAssert>(),SessionMissing<void,FarmClusterAssertion>(),SessionMissing<void,TerrainFarmPostpassAssertion>()};
        normal=std::make_unique<RawNormalTerrainWorldBinding>(world,chunks,*islands,random,crt,terrainGlobals,stamp,randomPlacement,farm,piece,*pipeline,hp,*carrier,std::move(diagnostics));
        finish=std::make_unique<RawFortFinishOwner>(core.pool,core.types,assetState.residences,finishState,FortFinishOwnerHooks{SessionMissing<FortRandomRectangle,std::int32_t>(),SessionMissing<double>(),SessionMissing<std::uint32_t,GameRandom&,const FortRandomPlaceRequest&>(),SessionMissing<void>()});
        templates[1]=std::move(fort);
        const auto state=templates[1].Section("State");stateBytes[1].assign(state.begin(),state.end());
        stateStreams[1]={stateBytes[1],0,static_cast<std::uint32_t>(stateBytes[1].size()),!stateBytes[1].empty()};stateNames[1]=std::string(mission);
        streams[1]={templates[1].territory,0,static_cast<std::uint32_t>(templates[1].territory.size())};headers[20]=1;
        runtime=MakeRuntime();
        regionOwner=std::make_unique<RawRegionOwner>(core.pool,core.hash.Entries(0),terrainType);
        auto& ui=services.missionOutcome; // 선택되지 않은 UI를 기본 성공으로 채우지 않고 실제 Check에서 오류를 유지한다.
        FillMissing(ui.demo);FillMissing(ui.tellBlocked);FillMissing(ui.section);FillMissing(ui.closeTransient);FillMissing(ui.onSection);
        MissionOutcomeHooks outcomeHooks; // 현재 원본 조건 계산은 기존 RawMissionOutcome에 맡긴다.
        outcomeHooks.regionOwner=[this](float x,float y) { return regionOwner->At(x,y); };
        outcomeHooks.section=ui.section;outcomeHooks.closeTransientGumps=ui.closeTransient;outcomeHooks.onSection=ui.onSection;
        outcomeHooks.invalidPlayerAssert=SessionMissing<void,std::uint32_t>();
        outcome=std::make_unique<RawMissionOutcome>(core.pool,priests->Priests(),outcomeState,
            MakeMissionOutcomeHooks(core.pool,hp,std::move(outcomeHooks)));
        outcome->BindCurrentInputs({players,alliances,[this] { return player->State().battleOwner; },ui.demo,ui.tellBlocked});
        ConnectAi();
    }
    // 전체 최초 펜스 몸체를 같은 현재 해시/TYPE/동맹/가상 삭제/공통 장부에 연결한다.
    void ConnectFence() {
        fenceRanges.resize(gunTypes.size());
        for (std::size_t type=0;type<gunTypes.size();++type) fenceRanges[type]=gunTypes[type].range; // 로드가 끝난 같은 불변 TYPE 사거리를 공급한다.
        fenceNeighbors=std::make_unique<RawFenceNeighbors>(core.pool,core.hash,core.types,fenceRanges,alliances);
        fenceLine=std::make_unique<RawFenceLine>(core.pool,core.hash,core.types,core.frames,fenceShieldType,
            [this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); });
        FencePostPopHooks hooks;
        hooks.theme=[this](Sid sid) { return themes.at(core.pool.Slot(sid)[10]); };
        hooks.neighbor=[this](Sid sid,std::uint32_t direction) { return fenceNeighbors->Neighbor(sid,direction); };
        hooks.reserveRegular=[] { return true; }; // 실제 Host의 할당 실패는 다음 AddRegular가 예외로 전달한다.
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        hooks.clearLine=[this](std::array<float,4> endpoints) { fenceLine->Clear(endpoints); };
        hooks.connections=[this](Sid sid) { return fenceNeighbors->Connections(sid); };
        hooks.validateBase=[this](Sid sid,std::uint32_t flags,const RawGraphPop* projected) { core.postPop.ValidateBase(sid,flags,projected); };
        hooks.damageable=[this](Sid sid,std::uint32_t flags) { carrier->DamageablePostPop(sid,flags); };
        fencePostPop=std::make_unique<RawFencePostPop>(core.pool,std::move(hooks));
        FenceCreateHooks create;
        create.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        create.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        create.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); }; //두 실제 생성 가상 표의+74는 공통004adf00이다.
        create.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        fenceMarkerCreate=std::make_unique<RawFenceMarkerCreate>(core.pool,*gunTarget,fenceNodeType,create);
        fenceShieldCreate=std::make_unique<RawFenceShieldCreate>(core.pool,core.frames,fenceShieldType,std::move(create));
        FenceRebuildHooks rebuilding;
        rebuilding.neighbor=[this](Sid sid,std::uint32_t direction) { return fenceNeighbors->Neighbor(sid,direction,true); };
        rebuilding.withinRange=[this](Sid sid,std::array<float,4> points) { return fenceNeighbors->WithinRange(sid,points); };
        rebuilding.markers=[this](Sid sid,Sid other) { fenceMarkerCreate->Create(sid,other); };
        rebuilding.shields=[this](std::array<float,4> points,std::uint32_t a,std::uint32_t b,std::int32_t first,std::int32_t second) { fenceShieldCreate->Create(points,a,b,first,second); };
        fenceRebuild=std::make_unique<RawFenceRebuild>(core.pool,std::move(rebuilding));
        FenceRegularHooks regular;
        regular.neighbor=[this](Sid sid,std::uint32_t direction) { return fenceNeighbors->Neighbor(sid,direction); };
        fenceMarkerClear=std::make_unique<RawFenceMarkerClear>(core.pool,core.hash,core.types,*gunTarget,fenceNodeType,[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); });
        regular.clearMarkers=[this](Sid sid,Sid other) { fenceMarkerClear->Clear(sid,other); };
        regular.clearLine=[this](std::array<float,4> endpoints) { fenceLine->Clear(endpoints); };
        regular.connections=[this](Sid sid) { return fenceNeighbors->Connections(sid); };
        regular.reserveRegular=[] { return true; }; //실제 Host 할당 실패는 다음 AddRegular에서 예외로 보존한다.
        regular.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        regular.refreshNeighbors=[this](Sid sid) { fenceLifetime->Refresh(sid); };
        regular.rebuild=[this](Sid sid) { fenceRebuild->Rebuild(sid); };
        regular.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
        fenceRegular=std::make_unique<RawFenceRegular>(core.pool,core.frames,core.frame,std::move(regular));
        FenceLifetimeHooks lifetimeHooks;
        lifetimeHooks.neighbor=[this](Sid sid,std::uint32_t direction,bool allied) { return fenceNeighbors->Neighbor(sid,direction,allied); };
        lifetimeHooks.clearMarkers=[this](Sid sid,Sid other) { fenceMarkerClear->Clear(sid,other); };
        lifetimeHooks.clearLine=[this](std::array<float,4> points,std::uint32_t,std::uint32_t,Sid,Sid) { fenceLine->Clear(points); }; // 실제455b20(mode0)은 두 owner/SID를 사용하지 않는다.
        lifetimeHooks.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };lifetimeHooks.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        lifetimeHooks.rebuild=[this](Sid sid) { fenceRebuild->Rebuild(sid); };
        lifetimeHooks.reserveRegular=[this] { try { fencePendingRegular=std::make_unique<RegularProcess>(2,0.10000000149011612f);return true; }catch (const std::bad_alloc&) { return false; } };
        lifetimeHooks.addRegular=[this](Sid sid,std::uint32_t event,float) { if (!fencePendingRegular) throw std::logic_error("펜스 실제 예약 객체 누락");auto* process=dynamic_cast<RegularProcess*>(core.processes.Attach(std::move(fencePendingRegular),46,sid,0x50));if (!process) throw std::logic_error("펜스 Regular 부모 부착 실패");process->ResetTask(event,core.processState.now); };
        lifetimeHooks.damageable=[this](Sid sid,std::uint32_t flags) { damageablePre->PreDestroy(sid,flags); };fenceLifetime=std::make_unique<RawFenceLifetime>(core.pool,std::move(lifetimeHooks));
    }
    // 가상 주소를 호출하지 않고 검증한 일반 Damageable의 실제 표만 선택한다.
    bool PlainDamageable(Sid sid) const {
        const auto table=Read(core.pool.Slot(sid),0);
        // 현재 VT를 매 호출 다시 읽고 주소가 같은 순수 수명 몸체만 공유한다.
        for (const auto known:kPlainDamageableTables) if (known==table) return true;return false;
    }
    // 실제507950/+10=4af780이며 extra8일 때44afd0의선행 일반 효과가 없다. 살아 있는 다이스 전체로 확장하지 않는다.
    bool PreviewDais(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==runeState.daisType && Read(raw,0)==0x00507950 && (raw[40]&8)!=0; }
    // 449580/기존 RawDaisLifecycle.PostDestroy의 두 SID 쌍을 같은 실제9×b4 Player 저장소에 연결한다.
    void DestroyPreviewDaisSlots(Sid sid) {
        const auto owner=core.pool.Slot(sid)[34];if (!owner) return;
        if (owner>8) throw std::out_of_range("다이스 미리보기 현재 Player 범위 오류");
        auto record=std::span(players).subspan(static_cast<std::size_t>(owner)*180,180);
        // 원본은 second94/98을 먼저, first8c/90을 다음에 처리하며 일치하지 않는 쌍은 보존한다.
        for (const std::size_t offset:{0x94U,0x8cU}) if (Read(record,offset)==sid.value) { Put(record,offset,0);Put(record,offset+4,Read(record,offset)); }
    }
    // 썬 바리케이드의 특수457990 전처리는 다른 일반 Damageable에 적용하지 않는다.
    bool SunFence(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==136 && Read(raw,0)==0x509470; }
    // 보호50dc30의+14는484f80→44b4b0 thunk이며+18/+10도기존Damageable/공통삭제다. 메뉴 범위와는 별개다.
    bool Nugget(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==154 && Read(raw,0)==0x50dc30U; }
    // 실제 사제 보호막167/507770의 공통 수명만 추가한다. 펜스 보호막170과 구별한다.
    bool PriestForcefield(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==167 && Read(raw,0)==0x507770U; }
    // 다섯 직접 Carrier의 +14=426890/+18=44b9e0이다. 5040c0은 별도42ae80이므로 포함하지 않는다.
    bool PlainCarrierLifetime(Sid sid) const { return carrierRegular && carrierRegular->Handles(sid) && Read(core.pool.Slot(sid),0)!=0x5040c0U; }
    // 실제 PE의 신전 네 가상 표를 판정하며 다른 자산을 타입 이름만으로 허용하지 않는다.
    bool Vortex(Sid sid) const {
        const auto table=Read(core.pool.Slot(sid),0);return table==0x005196f0 || table==0x005197a8 || table==0x00519860 || table==0x00519918;
    }
    //실제 표식90/포스필드170의+10은 이미 복원한 공통004af780이며 다른 펜스 파생은 포함하지 않는다.
    bool FenceCommon(Sid sid) const {
        const auto raw=core.pool.Slot(sid);const auto table=Read(raw,0);return (raw[10]==90 && table==0x005092d0) || (raw[10]==170 && table==0x00509388);
    }
    //144의 전용 Pre/Damageable Post가 확인된 실제 가상 표만 선택한다.
    bool Flyer(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==144 && Read(raw,0)==0x0050a388; }
    //110의 실제 공통 Pre/Post/Pop 가상 표이며 다른 자산에 일반화하지 않는다.
    bool VisualDummy(Sid sid) const { const auto raw=core.pool.Slot(sid);return raw[10]==110 && Read(raw,0)==0x0050eb98; }
    // 같은 현재 원문/Host의 생성·프레임·QA·패킷을 연결한다. 미복원 BaseFlyer 하위는 선택 시 거부한다.
    void ConnectSunAviary() {
        SunAviaryProcessHooks hooks;hooks.findProcess=[this](Sid parent,std::uint32_t type) { return FindAttachedProcess(core.pool,parent,type); };
        hooks.allocate=[](std::size_t size)->std::optional<SunAviaryProcessFields> {
            if (size!=0x30) throw std::invalid_argument("훨리베이스 실제 할당 크기 오류");auto* bytes=std::malloc(size);if (!bytes) return std::nullopt;
            SunAviaryProcessFields fields;std::memcpy(fields.bytes.data(),bytes,size);std::free(bytes);return fields; // 현재 CRT 할당의 미정 바이트를 지우지 않고 원문 입력으로 복사한다.
        };
        hooks.attach=[this](std::optional<SunAviaryProcessFields> fields,Sid parent,std::uint32_t type,std::uint32_t flags) {
            if (!fields) throw std::bad_alloc(); // 원본 NULL inner 부착은 주소0 쓰기 경로라 정상 성공으로 처리하지 않는다.
            static_cast<void>(core.processes.Attach(std::make_unique<SunAviaryProcess>(*sunAviary,std::move(*fields)),type,parent,flags));
        };
        hooks.damageable=[this](Sid parent,std::uint32_t flags) { carrier->DamageablePostPop(parent,flags); };hooks.assertion=SessionMissing<void,std::uint32_t>();
        hooks.setFrame=[this](Sid sid,std::int32_t frame) { core.frame.Set(sid,frame,0); };
        hooks.sound=[this](float x,float y,std::string_view name) { if (!services.sound.playOnceAt) throw std::logic_error("훨리베이스 실제 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),0,0); };
        hooks.checkFrame=[this](Sid sid,std::int32_t frame,std::uint32_t line) {
            const auto type=core.pool.Slot(sid)[10];if (type>=core.frames.size() || type>=core.types.size()) throw std::out_of_range("훨리베이스 현재 프레임 타입 오류");
            const auto count=core.frames[type].Codes().size();const auto multiplier=(core.types[type].flags1&0x440000U) ? 2U : 1U;
            if (frame<0 || static_cast<std::size_t>(frame)>=count*multiplier) SessionMissing<void,std::uint32_t>()(line); // 선택된 개발자 범위 진단을 실제 타입 자료로 확인한다.
        };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };hooks.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };hooks.bindFlyer=[this](Sid sid,Sid home) { flyer->Bind(sid,home); };
        hooks.refreshForm=[this](SunAviaryProcessFields& fields) {
            auto* current=dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(fields.Form()));if (!current || &current->Fields()!=&fields) throw std::logic_error("훨리베이스 현재 Host/raw 이미지 불일치");core.processes.RefreshForm(*current);
        };
        sunAviary=std::make_unique<RawSunAviaryProcess>(core.pool,SunAviaryProcessGlobals{core.processState.server,globals.editor,gunState.checking,core.processState.now,sunAviaryType,sunFlyerType},std::move(hooks));
    }
    // 같은 현재 원문/Host의 생성·프레임·QA·패킷을 연결한다. 미복원 BaseFlyer 하위는 선택 시 거부한다.
    void ConnectRainAviary() {
        RainAviaryProcessHooks hooks;hooks.findProcess=[this](Sid parent,std::uint32_t type) { return FindAttachedProcess(core.pool,parent,type); };
        hooks.allocate=[](std::size_t size)->std::optional<RainAviaryProcessFields> {
            if (size!=0x30) throw std::invalid_argument("맨 오 워 풀 실제 할당 크기 오류");auto* bytes=std::malloc(size);if (!bytes) return std::nullopt;
            RainAviaryProcessFields fields;std::memcpy(fields.bytes.data(),bytes,size);std::free(bytes);return fields; // 현재 CRT 할당의 미정 바이트를 지우지 않고 원문 입력으로 복사한다.
        };
        hooks.attach=[this](std::optional<RainAviaryProcessFields> fields,Sid parent,std::uint32_t type,std::uint32_t flags) {
            if (!fields) throw std::bad_alloc(); // 원본 NULL inner 부착은 주소0 쓰기 경로라 정상 성공으로 처리하지 않는다.
            static_cast<void>(core.processes.Attach(std::make_unique<RainAviaryProcess>(*rainAviary,std::move(*fields)),type,parent,flags));
        };
        hooks.damageable=[this](Sid parent,std::uint32_t flags) { carrier->DamageablePostPop(parent,flags); };hooks.assertion=SessionMissing<void,std::uint32_t>();
        hooks.setFrame=[this](Sid sid,std::int32_t frame) { core.frame.Set(sid,frame,0); };
        hooks.sound=[this](float x,float y,std::string_view name) { if (!services.sound.playOnceAt) throw std::logic_error("맨 오 워 풀 실제 위치 음향 미연결");services.sound.playOnceAt(x,y,soundNames.Lookup(name),0,0); };
        hooks.checkFrame=[this](Sid sid,std::int32_t frame,std::uint32_t line) {
            const auto type=core.pool.Slot(sid)[10];if (type>=core.frames.size() || type>=core.types.size()) throw std::out_of_range("맨 오 워 풀 현재 프레임 타입 오류");
            const auto count=core.frames[type].Codes().size();const auto multiplier=(core.types[type].flags1&0x440000U) ? 2U : 1U;
            if (frame<0 || static_cast<std::size_t>(frame)>=count*multiplier) SessionMissing<void,std::uint32_t>()(line); // 선택된 개발자 범위 진단을 실제 타입 자료로 확인한다.
        };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };hooks.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };hooks.bindFlyer=[this](Sid sid,Sid home) { flyer->Bind(sid,home); };
        rainAviary=std::make_unique<RawRainAviaryProcess>(core.pool,RainAviaryProcessGlobals{core.ownerMode.fort,gunTargetState.friendlyFire,core.processState.server,globals.editor,gunState.checking,core.processState.now,rainAviaryType,rainFlyerType},std::move(hooks));
    }
    // 같은 현재 Host에 기본 비행 생성자와 맨 오 워 전체 Pop을 연결한다. 비행 Frame/의존 해제는 선택 시 명시 오류다.
    void ConnectFlyer() {
        FlyerProcessHooks hooks;hooks.assertion=SessionMissing<void,std::uint32_t>();
        hooks.findProcess=[this](Sid sid,std::uint32_t type) { return FindAttachedProcess(core.pool,sid,type); };
        hooks.fields=[this](Sid form)->FlyerProcessFields& { auto* process=dynamic_cast<FlyerProcess*>(core.processes.FromForm(form));if (!process) throw std::logic_error("비행 Form의 현재 PID/원문 미연결");return process->Fields(); };
        hooks.attach=[this](FlyerProcessFields& fields,Sid parent,std::uint32_t type,std::uint32_t flags) {
            if (!pendingFlyer || &pendingFlyer->Fields()!=&fields) throw std::logic_error("비행 생성자의 현재 원문/소유권 불일치");
            if (!core.processes.Attach(std::move(pendingFlyer),type,parent,flags)) throw std::logic_error("비행 생성자의 free/dead 부모 부착 거부");
        };
        hooks.frame=[this](FlyerProcessFields& fields) { flyerFrame->Frame(fields); };
        hooks.notify=[this](FlyerProcessFields& fields,std::uint32_t message,std::uint32_t a,std::uint32_t b) { flyerFrame->Notify(fields,message,a,b); };hooks.parentDying=[this](FlyerProcessFields& fields) { flyerFrame->ParentDying(fields); };
        flyer=std::make_unique<RawFlyerProcess>(core.pool,core.types,FlyerProcessGlobals{core.processState.server,core.processState.now,flyerProcessType},std::move(hooks));
        ConnectFlyerFrame(); // 최초 Pop/Kernel보다 먼저 현재 같은 Frame 소비자를 설치한다.
        FlyerPostPopHooks post;post.allocateRainProcess=[this] {
            auto* allocation=std::malloc(0x70);if (!allocation) return false;FlyerProcessFields fields;std::memcpy(fields.bytes.data(),allocation,0x70);std::free(allocation);
            if (pendingFlyer) throw std::logic_error("비행 생성자 부착의 예상 밖 재진입");pendingFlyer=std::make_unique<FlyerProcess>(*flyer,std::move(fields));return true;
        };
        post.createRainProcess=[this](Sid parent,Sid home,std::uint32_t speed,float parameter) {
            if (!pendingFlyer) throw std::logic_error("비행 생성자의 실제 할당 누락");auto* current=pendingFlyer.get();
            flyer->Construct(current->Fields(),parent,home,speed,parameter);RawFlyerProcess::SpecializeRain(current->Fields()); // 실제 부착/첫 패킷 뒤에 전용 표를 쓴다.
        };
        post.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };post.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        post.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };post.initializeUnit=[this](Sid sid) { gunHit->SetTime(sid,core.processState.now); };
        post.shadowOffset=[this](Sid sid) { if (Read(core.pool.Slot(sid),0)!=0x50a388) throw std::logic_error("다른 비행체 그림자 offset 미복원");return std::array<float,2>{0.5f,0.5f}; }; // 실제0045faa0의 두 상수다.
        post.damageable=[this](Sid sid,std::uint32_t flags) { carrier->DamageablePostPop(sid,flags); };
        std::vector<FlyerPostPopType> details(core.types.size()); // 실제 불변 TYPE의 speed/전체 물리 프레임 수다.
        for (const auto& asset:assets.game.Types()) { const auto type=70+asset.block;details[type].speed=std::bit_cast<std::uint32_t>(static_cast<float>(asset.definition.Number("speed").value_or(0)));details[type].frames=static_cast<std::int32_t>(core.frames[type].Codes().size()); } // 원본 같은 번호의 표만 보충한다.
        flyerPostPop=std::make_unique<RawFlyerPostPop>(core.pool,core.types,details,core.processState.server,std::move(post));core.postPop.SetFlyerPostPop(flyerPostPop.get());
    }
    // 원본 제어/프레임·표적 검색을 같은 실제 큰 소비자에 연결하고 미복원 무기는 선택 시 구체적으로 거부한다.
    void ConnectFlyerFrame() {
        FlyerFrameHooks hooks;hooks.assertion=SessionMissing<void,std::uint32_t>();
        hooks.robust=[this](FlyerProcessFields& fields) {
            const auto form=fields.Form(),parent=fields.Parent();if ((form.value && form.value>=core.pool.Capacity()) || (parent.value && parent.value>=core.pool.Capacity())) throw std::logic_error("비행 Baseprocess0041be80의 Form/부모 원본 진단");return true; // 원문은 범위 진단 후1을 반환한다.
        };
        hooks.sidRobust=[this](Sid sid,bool live) { if (!sid.value || sid.value==0xffff) return false;if (live && gunState.checking) throw std::logic_error("비행 개발자 SID 상세진단004ac040 미복원");return true; }; // checking0의004ac140은 추가 raw 검사를 하지 않는다.
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { static_cast<void>(world.Destroy(sid,flags)); };
        hooks.finish=[](FlyerProcessFields&) { throw std::logic_error("비행 종료0045bc30/004605f0 미복원"); };
        hooks.otherFrame=[](FlyerProcessFields&)->std::int32_t { throw std::logic_error("다른 플라이어 outer+8 Frame 미복원"); };
        hooks.offset=[this](Sid sid,bool) { if (Read(core.pool.Slot(sid),0)!=0x50a388) throw std::logic_error("다른 플라이어 이동 offset 미복원");return std::array<float,2>{1,0}; }; // 실제 맨 오 워0045fa80의+cc/+d0은1/0이다.
        hooks.scatter=[this](Sid target,Sid flyer) { return flyerTarget->Scatter(target,flyer); };
        hooks.refreshTime=[this](Sid sid) { gunHit->SetTime(sid,core.processState.now); };hooks.hit=[this](Sid sid,Sid target) { gunHit->Hit(sid,target,core.processState.now); };
        hooks.removeSound=[this](Sid sid,std::uint32_t type) {
            const auto form=FindAttachedProcess(core.pool,sid,type);if (form.value && (core.processState.server || (form.value>4 && form.value<core.pool.Layout().serverFirst))) static_cast<void>(world.Destroy(form,0)); // 원본004b1a90(0,1)은 첫 일치 하나를 실제 삭제한다.
        };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };hooks.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        hooks.movedCell=[this](Sid sid) { if (Read(core.pool.Slot(sid),0)!=0x50a388) throw std::logic_error("다른 비행체 칸 이동+d4 미복원"); }; // TYPE144의 실제+d4=004437f0은 ret인 빈 몸체다.
        hooks.checkForms=[this](Sid sid) { return flyerFrame->CheckForms(sid); };
        hooks.mayTarget=[this](Sid sid,Sid target) {
            const auto raw=core.pool.Slot(sid);const auto center=gunTarget->Center(sid);const GunTargetQuery query{std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),center[0],center[1],raw[10],std::bit_cast<std::int32_t>(gunMode->Get(sid)),raw[34],std::bit_cast<std::int32_t>(Read(raw,36))};return gunTarget->MayTarget(query,target);
        };
        hooks.closerCount=[this](Sid target,Sid flyer) { return flyerTarget->CloserCount(target,flyer); };
        hooks.capacity=[this](Sid sid) { return flyerTarget->Capacity(sid); };
        hooks.findTarget=[this](FlyerProcessFields& fields) { flyerTarget->Find(fields); };
        hooks.addDepend=[this](Sid target,Sid form) { static_cast<void>(core.dependencies.Create(target,form)); };hooks.removeDepend=[this](Sid form,Sid target) { core.dependencies.Remove(form,target,0); };
        hooks.refreshForm=[this](FlyerProcessFields& fields) { auto* process=dynamic_cast<FlyerProcess*>(core.processes.FromForm(fields.Form()));if (!process || &process->Fields()!=&fields) throw std::logic_error("비행 Frame 현재 Host/raw 이미지 불일치");core.processes.RefreshForm(*process); };
        hooks.returnDelay=[](FlyerProcessFields&) { return 10000000.0; }; // 실제0045c2e0이 읽는00509ce0 float의 정확한 값이다.
        hooks.command=SessionMissing<void,Sid,std::uint32_t>();
        hooks.sunProcess=[this](Sid sid,std::uint32_t type)->SunAviaryProcessFields& { const auto form=FindAttachedProcess(core.pool,sid,type);auto* process=form.value ? dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(form)) : nullptr;if (!process) throw std::logic_error("착륙 대상의 실제 훨리베이스 프로세스 미연결");return process->Fields(); };
        hooks.refreshSun=[this](SunAviaryProcessFields& fields) { auto* process=dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(fields.Form()));if (!process || &process->Fields()!=&fields) throw std::logic_error("착륙 훨리베이스 Host/raw 이미지 불일치");core.processes.RefreshForm(*process); };
        hooks.homeNotice=[this](Sid home,std::uint32_t,std::uint32_t) {
            if (!core.processState.server) return;if (Read(core.pool.Slot(home),0)!=0x5099e8) throw std::logic_error("다른 실제 플라이어 홈 가상 통지 미복원");
            const auto form=FindAttachedProcess(core.pool,home,rainAviaryType);auto* process=form.value ? dynamic_cast<RainAviaryProcess*>(core.processes.FromForm(form)) : nullptr;
            if (!process) throw std::logic_error("맨 오 워 홈 통지의 현재 프로세스 NULL 원본 경계");
            rainAviary->FlyerDied(process->Fields(),random,[this](RainAviaryProcessFields& fields) {
                auto* current=dynamic_cast<RainAviaryProcess*>(core.processes.FromForm(fields.Form()));if (!current || &current->Fields()!=&fields) throw std::logic_error("맨 오 워 홈 통지의 Host/raw 이미지 불일치");core.processes.RefreshForm(*current);
            });
        };
        hooks.runCount=[this](Sid sid) { const auto raw=core.pool.Slot(sid);const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));const auto& table=core.frames[raw[10]];if (frame<0 || static_cast<std::size_t>(frame)>=table.Codes().size()) throw std::logic_error("플라이어 현재 글자 프레임 정상 범위 밖");return table.Run(table.Codes()[static_cast<std::size_t>(frame)].side).count; };
        hooks.launch=[this](Sid source,Sid target,std::uint32_t filename) {
            if (filename) throw std::logic_error("맨 오 워 빔의 비영 파일명 주소 미복원");static_cast<void>(particles->AddBeam(source,target,std::nullopt));
        };
        hooks.sound=[this](float x,float y,std::uint32_t token) { if (!services.sound.playOnceAt) throw std::logic_error("맨 오 워 실제 발사 소리 미연결");services.sound.playOnceAt(x,y,token,0,0); };
        std::vector<FlyerFrameType> details(core.types.size()); // 같은 이미 파싱한 TYPE 사거리와 실제 사운드 이름표 토큰이다.
        for (std::size_t type=0;type<details.size();++type) details[type]={gunTypes[type].range,missileTypes[type].firingSound}; // 현재 단일 TYPE 번호를 유지한다.
        flyerFrame=std::make_unique<RawFlyerFrame>(core.pool,core.types,details,std::span(soundState.typeFrames).first(core.types.size()),FlyerFrameGlobals{core.processState.server,assetState.boss,gunState.checking,core.processState.now,soundState.frame,sunFlyerType,windFlyerType,rainFlyerType,sunAviaryType,soundState.processType},std::move(hooks));
        ConnectFlyerTarget(); // 첫 Kernel 프레임 이전에 같은 표적 선택 본체를 설치한다.
    }
    // 원본의 일반 finder/의존 watcher/현재 Host 부모/표적 문맥을 동일 월드에 연결한다.
    void ConnectFlyerTarget() {
        FlyerTargetHooks hooks;hooks.assertion=SessionMissing<void,std::uint32_t>();
        hooks.checkForms=[this](Sid sid) { return flyerFrame->CheckForms(sid); };
        hooks.offset=[this](Sid sid) { if (Read(core.pool.Slot(sid),0)!=0x50a388) throw std::logic_error("다른 플라이어 검색 offset 미복원");return std::array<float,2>{1,0}; }; // 실제 맨 오 워0045fa80의+cc다.
        hooks.begin=[this](SquidSearchArea area,std::uint32_t flags) { flyerFinder=std::make_unique<RawSquidFinder>(core.pool,core.hash,core.types);return flyerFinder->Begin(area,flags); };
        hooks.next=[this] { if (!flyerFinder) throw std::logic_error("비행 표적 일반 finder 커서 누락");return flyerFinder->Next(); };
        hooks.mayTarget=[this](Sid sid,Sid target) { const auto raw=core.pool.Slot(sid);const auto center=gunTarget->Center(sid);const GunTargetQuery query{std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),center[0],center[1],raw[10],std::bit_cast<std::int32_t>(gunMode->Get(sid)),raw[34],std::bit_cast<std::int32_t>(Read(raw,36))};return gunTarget->MayTarget(query,target); };
        hooks.watcher=[this](Sid form) { return core.dependencies.Watcher(form); };
        hooks.processParent=[this](Sid form) { const auto* process=core.processes.FromForm(form);if (!process) throw std::logic_error("비행 경쟁의 현재 Host 프로세스 조회 실패");return process->Parent(); };
        hooks.returnDelay=[](FlyerProcessFields&) { return 10000000.0; }; // 현재 맨 오 워 outer+c의 실제0045c2e0이다.
        hooks.command=SessionMissing<void,Sid,std::uint32_t>();
        std::vector<float> ranges(core.types.size()); // 파싱한 같은188TYPE의 실제 사거리다.
        for (std::size_t type=0;type<ranges.size();++type) ranges[type]=gunTypes[type].range; // 별도 추정 사거리를 만들지 않는다.
        flyerTarget=std::make_unique<RawFlyerTarget>(core.pool,core.types,ranges,FlyerTargetGlobals{core.processState.server,gunState.checking,core.processState.now,sunFlyerType,flyerProcessEnd,flyerDependType},std::move(hooks));
    }
    // 타입31/32/60의 서로 다른 writer와 같은 실제 송신기를 연결한다.
    ProcessFormTransmission AviaryTransmission(ProcessFormTransmission fallback) {
        ProcessFormTransmission hooks;
        hooks.writeData=[this,fallback](SquidProcess& process,bool parentChanged) { auto* flying=dynamic_cast<FlyerProcess*>(&process);writingFlyer=flying!=nullptr;auto* rain=dynamic_cast<RainAviaryProcess*>(&process);writingRainAviary=rain!=nullptr;auto* current=dynamic_cast<SunAviaryProcess*>(&process);writingSunAviary=current!=nullptr;if (flying) return static_cast<std::int32_t>(flying->WritePacket(flyerPacket,parentChanged));if (rain) return static_cast<std::int32_t>(rain->WritePacket(rainAviaryPacket,parentChanged));if (current) return static_cast<std::int32_t>(current->WritePacket(sunAviaryPacket,parentChanged));if (!fallback.writeData) throw std::logic_error("다른 실제 프로세스 writer 미연결");return fallback.writeData(process,parentChanged); };
        hooks.transmit=[this,fallback](std::int32_t length) { if (writingFlyer) { if (length<0 || static_cast<std::size_t>(length)>flyerPacket.size()) throw std::out_of_range("비행 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(flyerPacket).first(static_cast<std::size_t>(length)));return; }if (writingRainAviary) { if (length<0 || static_cast<std::size_t>(length)>rainAviaryPacket.size()) throw std::out_of_range("맨 오 워 풀 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(rainAviaryPacket).first(static_cast<std::size_t>(length)));return; }if (writingSunAviary) { if (length<0 || static_cast<std::size_t>(length)>sunAviaryPacket.size()) throw std::out_of_range("훨리베이스 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(sunAviaryPacket).first(static_cast<std::size_t>(length)));return; }if (!fallback.transmit) throw std::logic_error("다른 실제 프로세스 송신 미연결");fallback.transmit(length); };return hooks;
    }
    // 실제 Carrier 내용물 그림/표시/타이머를 현재 Session의 한 풀·시계·공유 타입에 연결한다.
    void ConnectCarrierRegular() {
        NuggetPostPopHooks nugget;
        nugget.hasRegular=[this](Sid sid,std::uint32_t event) { return core.processes.FindEvent(sid,event)!=nullptr; };
        nugget.allocate=[this] {
            if (pendingNuggetRegular) throw std::logic_error("너겟 Regular 확보 메모리 중복");
            try { pendingNuggetRegular=std::make_unique<RegularProcess>(0,0.0F); }catch (const std::bad_alloc&) { return false; }
            return true;
        };
        // 기존455930 복원과 같은LCG상위16비트/65536 및넓은간격을 사용하고 여기서만float저장한다.
        nugget.random=[this](float low,float high) { return static_cast<float>(static_cast<double>(random.Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low); };
        nugget.construct=[this](Sid sid,std::uint32_t event,float payload) {
            if (!pendingNuggetRegular) throw std::logic_error("너겟 Regular 생성 전 확보 메모리 없음");
            auto* reserved=pendingNuggetRegular.get();std::destroy_at(reserved);std::construct_at(reserved,event,payload); // 확보한 객체를 같은 주소에서 실제 인자로 생성하며 두 번째 할당을 하지 않는다.
            auto* attached=static_cast<RegularProcess*>(core.processes.Attach(std::move(pendingNuggetRegular),46,sid,0x50));if (attached) attached->ResetTask(event,core.processState.now);
        };
        nugget.damageable=[this](Sid sid,std::uint32_t flags) { carrier->DamageablePostPop(sid,flags); };
        NuggetRegularHooks nuggetEvents;nuggetEvents.allocate=nugget.allocate;nuggetEvents.construct=nugget.construct; // 같은확보메모리/Host를PostPop과왕복끝점에서공유한다.
        nuggetEvents.randomInteger=[this](std::int32_t low,std::int32_t high) { if (high<=low) throw std::invalid_argument("너겟정수난수구간오류");return low+static_cast<std::int32_t>(random.Next(static_cast<std::uint32_t>(high-low))); };
        nuggetEvents.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); };
        nuggetEvents.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        nuggetEvents.destroy=[this](Sid sid,std::uint32_t flags) { static_cast<void>(world.Destroy(sid,flags)); };
        nuggetEvents.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        nuggetEvents.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        nuggetEvents.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
        nuggetAFrames.resize(core.types.size());
        // 원본TYPE+170과같은A구간개수를현재보호프레임표에서보충한다.
        for (std::size_t type=0;type<nuggetAFrames.size();++type) nuggetAFrames[type]=core.frames[type].Run('A').count;
        nuggetRegular=std::make_unique<RawNuggetRegular>(core.pool,nuggetAFrames,core.spots,assetState.boss,std::move(nuggetEvents));
        nuggetPostPop=std::make_unique<RawNuggetPostPop>(core.pool,std::move(nugget));
        carrierRegularTypes.resize(core.types.size());
        // 원본 타입 로더의 기본0과 실제 자산 정의의118/1b8을 같은 번호로 보충한다.
        for (std::size_t index=0;index<assets.game.Types().size();++index) {
            const auto& definition=assets.game.Types()[index].definition;const auto cast=definition.Number("casttime").value_or(0.0);
            if (!std::isfinite(cast) || cast<std::numeric_limits<std::int32_t>::min() || cast>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("실제 자산 casttime의 정수 범위 오류");
            carrierRegularTypes.at(index+70)={definition.specialFrames.defaultFrame,static_cast<std::int32_t>(cast)};
        }
        ContentPreviewHooks preview;
        preview.attach=[this](ContentPreviewFields& fields,Sid parent,std::uint32_t type,std::uint32_t flags) {
            if (!pendingContentPreview || &pendingContentPreview->Fields()!=&fields) throw std::logic_error("내용물 타이머의 실제 확보 메모리 불일치");
            static_cast<void>(core.processes.Attach(std::move(pendingContentPreview),type,parent,flags));
        };
        preview.destroy=[this](Sid sid,std::uint32_t flags) { static_cast<void>(world.Destroy(sid,flags)); };
        preview.assertion=SessionMissing<void>();
        contentPreviewProcess=std::make_unique<RawContentPreviewProcess>(core.pool,core.processState.now,soundState.processType,std::move(preview));
        CarrierRegularHooks hooks;
        hooks.find=[this](Sid parent,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { return priests->Carrier().FindContained(parent,mask,kind,index); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        // 실제4ac220은 하위WORD 저장 뒤 현재 타입이 다리일 때만 기존 표면 통지를 호출한다.
        hooks.setWord=[this](Sid sid,std::uint32_t value) { auto raw=core.pool.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(value);raw[13]=static_cast<std::uint8_t>(value>>8);if (raw[10]==core.bridgeState.bridgeType) core.building.NotifySurface(sid); };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        hooks.allocateTimer=[this] {
            if (pendingContentPreview) throw std::logic_error("내용물 타이머 확보 메모리의 중복 소유권");
            try { pendingContentPreview=std::make_unique<ContentPreviewProcess>(*contentPreviewProcess,ContentPreviewFields{}); }
            catch (const std::bad_alloc&) { return false; }
            return true;
        };
        hooks.constructTimer=[this](Sid parent,float duration) {
            if (!pendingContentPreview) throw std::logic_error("내용물 타이머 생성 전 실제 메모리 미확보");
            contentPreviewProcess->Construct(pendingContentPreview->Fields(),parent,duration);
        };
        hooks.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        CarrierFallingHooks falling;falling.find=hooks.find;falling.create=hooks.create;falling.setWord=hooks.setWord;falling.setOwner=hooks.setOwner;falling.pop=hooks.pop;
        falling.findKind=[this](Sid parent,std::uint32_t kind) {
            RawContainedFinder finder(core.pool,contentFinder);
            // 4b26f0(parent,kind,0,0)은 mask 없이 WORD18이 같은 첫 Form6을 반환한다.
            for (auto sid=finder.Begin(parent);sid.value;sid=finder.Next()) { const auto raw=core.pool.Slot(sid);if ((std::uint32_t(raw[18])|(std::uint32_t(raw[19])<<8))==kind) return sid; }
            return Sid{};
        };
        falling.destroy=[this](Sid sid,std::uint32_t flags) { static_cast<void>(world.Destroy(sid,flags)); };
        falling.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        falling.fallWalker=[this](Sid sid) { return priests->FallWalker(sid); };
        falling.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Carrier.cpp 낙하 진단 "+std::to_string(line)); };
        carrierFalling=std::make_unique<RawCarrierFalling>(core.pool,core.types,core.frames,core.hash.Entries(0),players,assetState.boss,gunState.checking,priestState.movement.priest,cursorManaGlobals.mapLimit,std::move(falling));
        CarrierTaskHooks task;task.find=[this](Sid sid) { return core.processes.FindEventMasked(sid,0xfff0,0x5100); };
        task.allocate=[this] { if (pendingCarrierTask) throw std::logic_error("캐리어 task 확보 메모리 중복");try { pendingCarrierTask=std::make_unique<RegularProcess>(0,0.0F); }catch (const std::bad_alloc&) { return false; }return true; };
        task.construct=[this](Sid sid,std::uint32_t event,bool shared) {
            if (!pendingCarrierTask) throw std::logic_error("캐리어 task 생성 전 확보 메모리 없음");auto* reserved=pendingCarrierTask.get();std::destroy_at(reserved);std::construct_at(reserved,event,5.0F);
            return static_cast<RegularProcess*>(core.processes.Attach(std::move(pendingCarrierTask),shared ? 61U : 46U,sid,0x50));
        };
        task.assertion=[] { throw std::logic_error("원본 Carrier.cpp407 낙하 task 교체 진단"); };
        carrierTask=std::make_unique<RawCarrierTask>(core.pool,core.processState.now,std::move(task));
        CarrierFallStartHooks begin;begin.hasRegular=[this](Sid sid,std::uint32_t event) { return core.processes.FindEvent(sid,event)!=nullptr; };
        begin.movementType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
        begin.removeMovement=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { static_cast<void>(world.Destroy(target,flags)); });static_cast<void>(lookup.Find(sid,type,0,true)); };
        begin.clearSelection=[this] { auto link=activeSelection.lock();if (!link || !link->selection) throw std::logic_error("낙하 선택 해제의 현재 UI 수명 미연결");static_cast<void>(link->selection->Select(0,0,1)); };
        begin.findKind=[this](Sid parent,std::uint32_t kind) { RawContainedFinder finder(core.pool,contentFinder);
            // 실제4b26f0의 mask 없는 첫 kind 조회다.
            for (auto sid=finder.Begin(parent);sid.value;sid=finder.Next()) { const auto raw=core.pool.Slot(sid);if ((std::uint32_t(raw[18])|(std::uint32_t(raw[19])<<8))==kind) return sid; }return Sid{};
        };
        begin.releasePriest=[this](Sid sid) { carrierFalling->ReleasePriest(sid); };begin.task=[this](Sid sid,std::uint32_t event,float delay) { carrierTask->Set(sid,event,delay); };
        carrierFallStart=std::make_unique<RawCarrierFallStart>(core.pool,players,assetState.boss,priestState.movement.priest,std::move(begin));
        hooks.needsEscape=[this](float x,float y) { return carrierFalling->NeedsEscape(x,y)!=0; };
        hooks.movementType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
        hooks.hasPath=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { static_cast<void>(world.Destroy(target,flags)); });return lookup.Find(sid,type).value!=0; };
        hooks.escape=[this](Sid sid,float delay,std::uint32_t event) { if (!services.motion.carrier.escape) throw std::logic_error("현재 Carrier 회피429010 미연결");static_cast<void>(services.motion.carrier.escape(sid,delay,event)); };
        hooks.falling=[this](Sid sid,std::uint32_t count) { return carrierFalling->Fall(sid,count); };
        hooks.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
        carrierRegular=std::make_unique<RawCarrierRegular>(core.pool,core.types,carrierRegularTypes,assetState.boss,std::move(hooks));
    }
    // 실제 Damageable 해방/공통 삭제와 신전 Pre를 같은 현재 월드에 조립한다.
    void ConnectDamageable() {
        DamageableReleaseHooks release;release.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        release.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        release.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };release.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        release=MakePriestSpawnHooks(core.pool,priests->Spawn(),std::move(release));
        damageableRelease=std::make_unique<RawDamageableRelease>(core.pool,core.types,core.spots,contentFinder,damageableReleaseState,std::move(release));
        auto pre=services.damageablePre;
        if (!pre.collapse) pre.collapse=[this](Sid sid) { static_cast<void>(particles->AddShrapnel(sid,ShrapnelKind::Collapse)); };
        if (!pre.explosion) pre.explosion=[this](Sid sid) {
            if (impactState.suppressedTime==0 && !impactState.suppressed && core.kernel.Size()<30001) {
                const auto raw=core.pool.Slot(sid);const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));const auto foot=static_cast<float>(core.types[raw[10]].footX);
                hits->Impact().Visual(static_cast<float>(static_cast<double>(x)-static_cast<double>(foot)*.5),y,1,std::bit_cast<float>(0x3d23d70aU));
            }
            static_cast<void>(particles->AddShrapnel(sid,ShrapnelKind::Explosion));
        };
        if (!pre.sound) pre.sound=[this](const DamageableSoundEvent& event) {
            if (event.sound==DamageableSound::PriestFree) { if (!services.sound.play) throw std::logic_error("실제 priestFree 전역 소리 미연결");services.sound.play(soundNames.Lookup("priestFree.wav"),0,0,0,1,0);return; }
            if (!services.sound.playOnceAt) throw std::logic_error("실제 붕괴/폭발 위치 소리 미연결");services.sound.playOnceAt(event.x,event.y,soundNames.Lookup(event.sound==DamageableSound::Collapse ? "collapse.wav" : "explosion.wav"),0,0);
        };
        pre.validateBase=[this](Sid sid,std::uint32_t flags) { world.CommonDeletion().ValidatePre(sid,flags); };
        pre.base=[this](Sid sid,std::uint32_t flags) { world.CommonDeletion().PreDestroy(sid,flags); };
        pre.releaseContained=[this](Sid sid) { damageableReleaseState.battle=core.ownerMode.battle;damageableRelease->Release(sid); };
        damageablePre=std::make_unique<RawDamageablePreDestroy>(core.pool,core.spots,contentFinder,damageablePreState,std::move(pre));priests->BindDamageable(*damageablePre);
        flyerPre=std::make_unique<RawFlyerPreDestroy>(core.pool,FlyerPreDestroyHooks{[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); },[this](Sid sid,std::uint32_t flags) { damageablePre->PreDestroy(sid,flags); }});
        auto post=services.damageablePost;FillMissing(post.dropDude);FillMissing(post.dropIsland);
        post.regular=[this](Sid sid,std::uint32_t event,float payload,std::uint32_t count) { auto* process=core.processes.AddRegular(sid,event,payload);if (!process) throw std::bad_alloc();process->SetInitialCount(count); };
        post.hasBridgeFall=[this](Sid sid) { return HasScheduledBridgeFall(core.processes,sid); };post.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        post.displayUpdate=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };post.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        post.base=[this](Sid sid,std::uint32_t flags) { world.CommonDeletion().PostDestroy(sid,flags); };
        damageablePost=std::make_unique<RawDamageablePostDestroy>(core.pool,core.hash,core.types,core.frames,hp,core.neighbors,damageablePostState,std::move(post));
        VortexPreDestroyHooks vortex;vortex.islandNumber=[this](float x,float y) { return ownership->IslandNumber(x,y); };vortex.islandExists=[this](std::int32_t island) { return islands->Exists(island)!=0; };
        vortex.islandTemple=[this](std::int32_t island) { return islands->Temple(island); };
        vortex.clearIslandTemple=[this](std::int32_t island) { if (!islands->Exists(island)) throw std::logic_error("신전 삭제의 실제 섬 부재 진단");islands->Temple(island,0);services.ownership.refreshProduction(); };
        vortex.islandOwner=[this](std::int32_t island,std::uint32_t owner) { islands->Owner(island,owner); };vortex.regionOwnership=[this](float x,float y) { ownership->UpdateRegion(x,y); };
        vortex.damageablePre=[this](Sid sid,std::uint32_t flags) { damageablePre->PreDestroy(sid,flags); };vortex.refreshProduction=services.ownership.refreshProduction;
        vortex.assertion=SessionMissing<void,std::uint32_t>();
        vortexPre=std::make_unique<RawVortexPreDestroy>(core.pool,players,assetState.temples,assetState.residences,assetState.outposts,objectState.loadingDepth,std::move(vortex));
        GeyserLifetimeHooks geyser;
        geyser.defaultFrame=[this](Sid sid) { const auto type=core.pool.Slot(sid)[10];if (type<70 || type-70>=assets.game.Types().size()) throw std::out_of_range("간헐천 기본 프레임 TYPE 오류");return static_cast<std::uint32_t>(assets.game.Types()[type-70].definition.specialFrames.defaultFrame); };
        geyser.update=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        geyser.registerAi=[this](Sid sid,std::uint32_t state) { RawGeyserAiRegistration(core.pool.Edition(),players).Register(sid.value,state); };
        geyser.damageablePre=[this](Sid sid,std::uint32_t flags) { damageablePre->PreDestroy(sid,flags); };
        geyser.damageablePost=[this](Sid sid,std::uint32_t flags) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(sid,flags); };
        geyser.dropIsland=[this](float x,float y,std::uint32_t flags,std::uint32_t owner) { PrepareBuilding(core.processState.now);buildingEffects->DropIsland(x,y,flags,owner); };
        geyserLifetime=std::make_unique<RawGeyserLifetime>(core.pool,assetState.geysers,globals.editor,std::move(geyser));
    }
    // 각 명시 요청에서 현재 실제 소유자/서버/권한/요새/타입만 공급한다. Player/자원/지도는 복사하지 않는다.
    void PrepareConstruction() {
        constructionState.place.localPlayer=constructionState.notice.localPlayer=playerState.battleOwner;
        constructionState.request.editor=core.ownerMode.fort;constructionState.request.localPlayer=playerState.battleOwner;
        constructionState.request.displaceGenus=buildingState.pushGenus;constructionState.request.patternTypes=constructionState.confirm.patternTypes;
        constructionState.command.localPlayer=playerState.battleOwner;constructionState.command.chargeBlocked=constructionState.place.chargeBlocked;
        constructionState.place.server=core.processState.server;constructionState.place.battleShown=playerState.playerKnown!=0;
        constructionState.place.authority=constructionState.clear.authority=constructionState.notice.authority=constructionState.geyser.boss=assetState.boss;
        constructionState.geyser.fort=core.ownerMode.fort;
        constructionState.place.noIslandType=constructionState.geyser.noIslandType=core.bridgeState.noIslandType;
        constructionState.place.bridgeType=constructionState.clear.bridgeType=core.bridgeState.bridgeType;
        constructionState.confirm.patternTypes[0]=piece;constructionState.confirm.patternTypes[1]=core.bridgeState.bridgeType;
        constructionState.confirm.patternTypes[2]=core.bridgeState.islandType;constructionState.confirm.patternTypes[3]=core.bridgeState.noIslandType;
        constructionState.clear.patternTypes=constructionState.place.patternTypes=constructionState.request.patternTypes=constructionState.confirm.patternTypes;
    }
    // 수명/자산의 실제 파생 동작을 설치한 뒤 첫 객체 생성 전에 받침/로컬 간헐천 연결을 한 번 구성한다.
    void ConnectConstruction() {
        if (construction) throw std::logic_error("실제 Session 받침/간헐천 중복 연결");
        recentGeysers.entries.resize(5);PrepareConstruction();
        auto effects=services.construction; // 미선택 외부 경계는 실제 호출 때 오류로 드러내며 성공 관찰로 채우지 않는다.
        FillMissing(effects.configInt);FillMissing(effects.wallSeconds);FillMissing(effects.refresh);
        FillMissing(effects.negativeQuantity);FillMissing(effects.boundsAssertion);
        RawCampaignConstructionGameplay gameplay; // 첫 객체 설치 뒤 선택될 현재 원본 하위만 지연 연결한다.
        gameplay.request.mayPlace=[this](const CanonPlacementQuery& query) { return MayPlaceGameplay(query); };
        gameplay.request.displace=[this](float x,float y,std::uint32_t type,std::uint32_t owner) { if (!buildingEffects) throw std::logic_error("요청의 실제 건설 효과 미설치");PrepareBuilding(core.processState.now);return buildingEffects->Displace(x,y,type,owner); };
        gameplay.request.cancel=[this](float x,float y,std::uint32_t flags,std::uint32_t owner) { if (!buildingEffects) throw std::logic_error("요청 실패의 실제 섬 정리 미설치");PrepareBuilding(core.processState.now);buildingEffects->DropIsland(x,y,flags,owner); };
        gameplay.confirm.findBuilder=[this](std::uint32_t owner,std::uint32_t type,float x,float y) {
            if (!anchor) throw std::logic_error("일반 확정의 실제 기준점 미설치");const auto value=anchor->Locate(owner,type,x,y);
            if (value>=core.pool.Capacity()) throw std::out_of_range("일반 확정 기준점 SID 범위 오류");return Sid{static_cast<std::uint16_t>(value)};
        };
        gameplay.notice.moveBuilder=[this](std::uint32_t owner,Sid target,std::uint32_t low,std::uint32_t high) { if (!buildingMover) throw std::logic_error("일반 통지의 실제 Builder 미설치");static_cast<void>(buildingMover->Move(owner,target,low,high)); };
        gameplay.notice.findPriest=[this](std::uint32_t owner) { return Sid{static_cast<std::uint16_t>(RawPriestLookup(core.pool,priestState.priests).Find(owner))}; };
        gameplay.notice.containedType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
        gameplay.notice.removeContained=[this](Sid parent,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { world.Destroy(target,flags); });static_cast<void>(lookup.Find(parent,type,0,true)); };
        gameplay.notice.immobile=[this](Sid sid) { return priests->PriestState().Immobile(sid); };
        gameplay.notice.approach=[this](Sid sid,float x,float y,std::uint32_t flags) {
            if (!anchor) throw std::logic_error("일반 통지 접근점의 실제 Graph 미설치");const auto footprint=gunTarget->Footprint(sid,false);auto rect=std::array<float,4>{footprint.left,footprint.top,footprint.right,footprint.bottom};
            RawBoardApproach approach([this](float px,float py) { return anchor->GraphAt(px,py); });return approach.Find(rect,x,y,flags);
        };
        gameplay.notice.tryMove=[this](Sid priest,float x,float y,Sid target) { if (!motion) throw std::logic_error("일반 통지의 실제54 미설치");return motion->Paths().StartPriest({priest,{x,y},0,target.value,0,0,0})!=nullptr; };
        gameplay.notice.start=[this](Sid parent,std::uint32_t bits) { if (!building) throw std::logic_error("일반 통지의 실제30 미설치");PrepareBuilding(core.processState.now);building->Start(parent,std::bit_cast<float>(bits)); };
        construction=std::make_unique<RawCampaignConstructionBindings>(world,hp,sp,rewardState,metadata.frames,chunks,players,*broadcast,constructionState,
            PlayerGeyserBoundsState{core.ownerMode.battle,objectState.mission,core.ownerMode.challenge,geyserMapSize,piece,playerDirections},
            recentGeysers,assetState.geysers,std::move(effects),std::move(gameplay));
    }
    // 실제 AI 계산/색 표/섬 프레임은 Session이 연결하고 언어/채팅/표시 통지는 외부 소비자에 남긴다.
    void ConnectAi() {
        auto hooks=services.ai.participation; // brain 수명은 외부 관찰 callback에 맡기지 않는다.
        hooks.destructBrain=[this](std::uint32_t token) { aiConstructor->Destruct(token); };
        hooks.freeBrain=[this](std::uint32_t token) { aiConstructor->FreeBody(token); };
        participation=std::make_unique<RawPlayerParticipation>(core.pool.Edition(),players,alliances,hiddenPlayers,participationState,std::move(hooks));
        participation->BindCurrentInputs({[this] { return playerState.battleOwner; },[this] { return playerState.playerKnown; },
            [this] { return core.ownerMode.battle; },[this] { return core.ownerMode.challenge; },[this] {
                if (!services.ai.localTransport) throw std::logic_error("실제 Session 로그인 로컬 전송 조회 미연결");return services.ai.localTransport();
            }});
        auto energy=BindAiTypeRequiredEnergy(assets.game,core.types); // 생성자/초기갱신이 같은 실제 AST와 현재188TYPE를 읽는다.
        AiBrainConstructorHooks constructorHooks;
        constructorHooks.priest=[this](std::uint32_t owner) { return Sid{static_cast<std::uint16_t>(RawPriestLookup(core.pool,priestState.priests).Find(owner))}; };
        constructorHooks.regionAt=[this](float x,float y) { return ownership->IslandNumber(x,y); };
        constructorHooks.requiredEnergy=energy;
        aiConstructor=std::make_unique<RawAiBrainConstructor>(aiMemory,*player,*participation,*islands,alliances,core.types,
            AiBrainConstructorGlobals{playerState.battleOwner,noRegion,[this] { return static_cast<std::uint32_t>(core.bookkeeping.invalidGraph); }},std::move(constructorHooks));
        AiBrainInitialUpdateHooks initialHooks;
        initialHooks.defaultName=[this] {
            if (!services.ai.defaultName) throw std::logic_error("실제 Session defaultAiName 현재 언어 조회 미연결");return services.ai.defaultName();
        };
        initialHooks.requiredEnergy=std::move(energy);
        aiInitial=std::make_unique<RawAiBrainInitialUpdate>(aiMemory,*player,*technology,contentFinder,core.types,
            AiBrainInitialUpdateGlobals{aiMissionFallback,aiExcludedType,aiWorkshops,[this] { return globals.missionPost ? 1U : 0U; }},std::move(initialHooks));
        AiCreateHooks createHooks;
        createHooks.changeColor=[this](std::uint32_t owner,std::uint32_t color) {
            ChangePlayerColor(core.pool,core.bridgeState,owner,color,[this](Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); }); // 같은 실제 색 표/프레임/표시 갱신을 실행한다.
            if (services.ai.changeColor) services.ai.changeColor(owner,color); // 외부 화면이 필요할 때만 완료 통지를 받으며 게임 계산을 대신하지 않는다.
        };
        createHooks.firstRegion=[this](std::uint32_t owner) {
            if (owner>=9) throw std::out_of_range("실제 Session AI 첫 지역 owner 범위 오류");return Read(players,owner*0xb4+0x78);
        };
        createHooks.islandTemple=[this](std::uint32_t region) { return islands->Temple(std::bit_cast<std::int32_t>(region)); };
        createHooks.destroyTemple=[this](std::uint32_t sid,std::uint32_t flags) {
            if (sid>65535 || sid>=core.pool.Capacity()) throw std::out_of_range("실제 Session AI 신전 SID 범위 오류");world.Destroy(Sid{static_cast<std::uint16_t>(sid)},flags);
        };
        createHooks.construct=[this](std::uint32_t owner,std::span<const std::uint8_t> settings,std::size_t bytes) { return aiConstructor->Construct(owner,settings,bytes); };
        createHooks.initialUpdate=[this](std::uint32_t token,std::span<const std::uint8_t> settings) { aiInitial->Update(token,settings); };
        aiCreate=std::make_unique<RawAiCreate>(core.pool.Edition(),*participation,std::move(createHooks));
    }
    // 첫 실제 객체 이전에 같은 공통 삭제/내용물/Player 통지/기술 모듈을 한 번 연결한다.
    void InstallContents(const SquidDestroyHooks& deletion) {
        gumpForms=std::make_unique<GumpFormHost>(core.pool,core.factory,core.types,core.destroy,
            [this](std::uint32_t owner,Sid parent) { if (!services.gumpNotice) throw std::logic_error("현재Session GumpForm UI통지미연결");services.gumpNotice(owner,parent); },core.forms.Hooks());
        core.forms.Bind(*gumpForms); // 실제월드삭제의혼합Form체인이같은호스트로돌아온다.
        FillMissing(services.unitLost);
        world.BindDeletion(hp,deletion.selected,MakeCampaignDeletionEffects(deletion,services.unitLost));
        world.DeletionState().silentType=147; // PATCH005412a4의 실제 초기 타입이다.
        auto playerHooks=services.playerContent;
        playerHooks.money=[this](std::uint32_t owner) { return std::bit_cast<float>(sp.Get(owner)); };
        FillMissing(playerHooks.gump);FillMissing(playerHooks.handle);
        const auto reads=BindSpStoreNotice(sp); // 모든 조회기는 초기값 대신 현재 같은 저장소를 빌린다.
        auto& patch=priestState.regen.patch;
        playerContent=std::make_unique<RawPlayerContentNotice>(core.pool,PlayerContentNoticeState{
            reads.cursorA,reads.cursorB,patch.changedSample,patch.sample,patch.remaining,
            PlayerContentNoticeRead<std::uint32_t>::Bind([this] { return assetState.inBattle ? 1U : 0U; }),
            assetState.localOwner,reads.firstSpPool,reads.firstPermutation,contentGumpType,contentEvent},std::move(playerHooks));
        contentNotice=std::make_unique<RawContentParentNotice>(core.pool,[this](Sid parent,Sid form,std::uint32_t reason) {
            lifetime->ParentNotice(parent,form,reason);
        });
        contentForms=std::make_unique<ContentFormHost>(core.pool,core.types,
            [this](Sid parent,Sid form,std::uint32_t reason) { contentNotice->Notice(parent,form,reason); },services.dependencyTransmit);
        auto contentEffects=services.contentLifetime;
        contentEffects.cancelCursor=[this] {
            if (!cursor) throw std::logic_error("현재 ContentForm 커서 수명 미설치");cursor->Cancel();
        };
        contentLifetimeState.currentCursor=[this] { return cursorDrop.preview; };
        FillMissing(contentEffects.localNotice);FillMissing(contentEffects.remoteNotice);
        FillMissing(contentEffects.wallet);FillMissing(contentEffects.addSp);FillMissing(contentEffects.sound);
        contentLifetimeState.localOwner=assetState.localOwner;contentLifetimeState.boss=assetState.boss;
        contentLifetimeState.notice=0x020000b4; // PATCH005429c4의 실제 수입 명령 초기값이다.
        auto carrierEffects=services.carrierContent;
        carrierEffects.center=[this](Sid parent) { return gunTarget->Center(parent); };
        carrierEffects.checkCarrier=[this](Sid parent) {
            StartSelectionContentPreview(core.pool,parent,objectState.loadingDepth,[this](Sid sid,std::uint32_t event,float payload) { static_cast<void>(core.processes.AddRegular(sid,event,payload)); },SessionMissing<void>());
        };
        carrierEffects.typeAt=[this](float x,float y,std::uint32_t type) { return world.Find(x,y,type); };
        carrierEffects.genusAt=[this](float x,float y,std::uint32_t genus) { return damageablePost->GenusAt(x,y,genus); }; // 같은 실제4b2070/일반 finder를 사용한다.
        if (!carrierEffects.freeSound) carrierEffects.freeSound=[this](std::string_view name) {
            if (!services.sound.play) throw std::logic_error("사제 해방 실제 전역 음향 미연결");services.sound.play(soundNames.Lookup(name),0,0,0,1,0); // 4a9cb0의 원본6인자다.
        };
        carrierContentNotice=std::make_unique<RawCarrierContentNotice>(core.pool,core.types,
            CarrierContentNoticeState{contentSavedGame,objectState.loadingDepth,contentBombBlocked,playerState.battleOwner,priestState.movement.priest,contentScreenOrigin[0],contentScreenOrigin[1],priestState.movement.rainWalker},std::move(carrierEffects));
        auto otherParentNotice=services.contentNotice;
        ContentFormNotice parentNotice=[this,other=std::move(otherParentNotice)](Sid parent,Sid form,std::uint32_t reason) {
            if (carrierContentNotice->Handles(parent)) { carrierContentNotice->Notice(parent,form,reason);return; }
            if (!other) throw std::logic_error("다른 캠페인 내용물 실제 부모 통지 미연결");other(parent,form,reason);
        };
        lifetime=std::make_unique<RawCampaignLifetimeBindings>(world,*contentForms,contentLifetimeState,
            std::move(contentEffects),hp,*playerContent,std::move(parentNotice));
        ContainedCreateHooks createHooks;createHooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        contained=std::make_unique<RawContainedCreate>(core.pool,contentState,contentForms->BindCreate(core.factory,std::move(createHooks)));
        PlayerTechnologyHooks techHooks;techHooks.assertion=SessionMissing<void,PlayerTechnologyAssertion>();
        techHooks.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        technology=std::make_unique<RawPlayerTechnology>(*player,priests->Carrier(),*contained,core.types,
            BindPlayerTechnologyTypes(assets.game.TypeTable(),std::move(techHooks)));
        totalMade=std::make_unique<RawTotalMade>(core.pool.Edition(),core.types,core.bookkeeping,restrictions,
            SessionMissing<void,TotalMadeAssertion>());
        productionDeck=std::make_unique<RawProductionDeck>(*player,[this](std::span<std::uint8_t> bytes) {
            if (!services.production.allocationBytes) throw std::logic_error("실제 Session 생산 Deck 할당 입력 미연결");
            services.production.allocationBytes(bytes);
        },SessionMissing<void,ProductionDeckAssertion>());
        production=std::make_unique<RawCampaignProductionBindings>(core,*player,priests->Carrier(),*contained,*productionDeck,*totalMade,contentFinder,
            CampaignProductionState{objectState.mission,fullDeck,freeTechnologyType,templeProduct},services.production.effects);
    }
    // 외부 config/UI는 현재 클라이언트 서비스를 사용하고 게임 계산은 동일 원본 모듈을 사용한다.
    std::unique_ptr<RawCampaignWorldRuntime> MakeRuntime(std::size_t templateCount=9) {
        auto hooks=services.install;
        FillMissing(hooks.translateYou);FillMissing(hooks.setConfig);
        FillMissing(hooks.refreshWindowTitle);FillMissing(hooks.resetCamera);
        hooks.builderAssertion=SessionMissing<void,RawIslandBuilderAssertion>();hooks.territoryAssertion=SessionMissing<void,FortWorldLoadAssertion>();
        hooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        hooks.setState=[this](Sid sid,std::uint32_t value) { assetBindings->SetState(sid,value); };
        hooks.virtualCount=[this](Sid sid) { return static_cast<std::int32_t>(level->Get(sid)); };
        return std::make_unique<RawCampaignWorldRuntime>(world,chunks,*islands,*player,*normal,random,hp,*ownership,*finish,*contained,objectState,globals,piece,noRegion,RawCampaignInstallData{std::span<const FortTemplate>(templates).first(templateCount),streams,headers,handle},std::move(hooks));
    }
    // 공개 운반 어댑터와 일반 배치 통지가 같은 현재 원본 Builder를 조립한다.
    std::unique_ptr<RawConstructionBuilder> MakeBuilder(ConstructionBuilderState& state,ConstructionBuilderHooks hooks) {
    auto* current=this;if (!current->motion) throw std::logic_error("미설치 Session의 건설 운반 연결");
    hooks=MakeConstructionBuilderHooks(current->core.pool,*current->anchor,current->core.factory,current->core.owner,std::move(hooks));
    hooks.pop=[current](Sid sid,float x,float y,std::uint32_t flags) { current->world.Pop(sid,x,y,flags); };
    hooks.notifySurface=[current](Sid sid) { current->core.building.NotifySurface(sid); };
    hooks.approach=[current](const std::array<float,4>& rectangle,float x,float y,std::uint32_t flags) { auto mutableRectangle=rectangle;RawBoardApproach approach([current](float px,float py) { return current->anchor->GraphAt(px,py); });return approach.Find(mutableRectangle,x,y,flags); };
    hooks.move=[current](Sid sid,float x,float y)->ConstructionBuilderPath* {
        auto* path=current->motion->Paths().StartConstruction({sid,{x,y},0,0,0,0,0});if (!path) return nullptr;const auto pid=current->core.kernel.Find(*path);
        auto& view=current->builderPaths[sid.value];
        view.readArrival=[current,path,pid] { if (current->core.kernel.Get(pid)!=path) throw std::logic_error("건설 운반의 실제 경로 수명 종료");const auto& runtime=path->Runtime();if (runtime.Count()<1) throw std::logic_error("건설 운반의 실제 경로 꼭짓점0");return runtime.Time(runtime.Count()-1); };
        view.prepare=[current,path,pid,sid](float speed,std::uint32_t flags) { if (flags || current->core.kernel.Get(pid)!=path) throw std::logic_error("건설 운반 Prepare의 실제 경로/flags 오류");const auto& definition=current->assets.game.Types()[current->core.pool.Slot(sid)[10]-70].definition;path->Prepare(speed,static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value())); };
        return &view;
    };
    hooks.speed=[](ConstructionBuilderPath& path,float speed,std::uint32_t flags) { if (!path.prepare) throw std::logic_error("건설 운반의 실제 Prepare view 미연결");path.prepare(speed,flags); };
    hooks.reserveTimer=[current] { try { current->builderPendingRegular=std::make_unique<RegularProcess>(0,0.05f);return true; }catch (const std::bad_alloc&) { return false; } };
    hooks.addTimer=[current](Sid sid,std::uint32_t event,float payload) {
        if (!current->builderPendingRegular || payload!=0.05f) throw std::logic_error("건설 운반의 실제 예약 객체/payload 오류");
        auto* process=dynamic_cast<RegularProcess*>(current->core.processes.Attach(std::move(current->builderPendingRegular),46,sid,0x50));if (!process) throw std::logic_error("건설 운반의 실제 Regular 부착 실패");process->ResetTask(event,current->core.processState.now);
    };
    ConstructionBuilderReads reads;reads.builderType=[current] { return current->start.teleportType; };reads.carrierType=[current] { return current->start.manaBoltType; };reads.bridgeType=[current] { return current->core.bridgeState.bridgeType; };reads.approachMask=[current] { return current->builderApproachMask; };reads.timerPayload=[] { return 0.05f; };reads.now=[current] { return current->core.processState.now; };
    return std::make_unique<RawConstructionBuilder>(current->core.pool,current->core.types,current->builderTypes,state,std::move(hooks),current->energySupply.get(),std::move(reads));

    }
    // 생성용 임의 동맹 표와 구별한 현재 게임 배치 파이프라인을 한 번 구성한다.
    void ConnectGameplayPlacement() {
        if (gameplayPipeline) throw std::logic_error("실제 게임 배치 중복 연결");gameplayPlacement.geometry=metadata.geometry;
        gameplayPlacement.permission.useAlliances=1; // 실제540cb0의 PE 초기 DWORD는1이다. 이후에도 같은 장기 상태를 읽는다.
        gameplayPipeline=std::make_unique<RawCanonPlacementPipeline>(core.pool,core.hash,CanonPlacementPipelineData{core.types,metadata.frames,core.shapes,metadata.hotspots,core.spots,core.hash.Entries(0),gameplayRegions},core.bookkeeping,additional,contentFinder,gameplayPlacement);
    }
    // 현재 Player/실제 방향 동맹/Graph와 IslandList를 요청 직전에 원본 소비 위치로 공급한다.
    bool MayPlaceGameplay(const CanonPlacementQuery& query) {
        if (!gameplayPipeline) throw std::logic_error("게임 요청의 전체 MayPlace 미설치");
        gameplayPlacement.placement.localPlayer=std::bit_cast<std::int32_t>(playerState.battleOwner);gameplayPlacement.permission.editor=globals.editor;
        gameplayPlacement.permission.graphReady=anchorState.graphReady;gameplayPlacement.permission.alliances=alliances;gameplayPlacement.anchor=anchorState;
        gameplayPlacement.collision.clientMode=core.processState.server ? 1U : 0U;gameplayPlacement.collision.ignoredGenus=buildingState.pushGenus;
        gameplayPlacement.geometry.patternTypes=constructionState.confirm.patternTypes;gameplayPlacement.relations.islandCount=islands->Count();
        // 현재 원본 섬의 제한/존재 DWORD만 읽고 Player/owner를 새로 만들지 않는다.
        for (std::size_t index=0;index<gameplayRegions.size();++index) gameplayRegions[index]={Read(islandBytes,4+index*32),Read(islandBytes,8+index*32)};
        return gameplayPipeline->MayPlace(query);
    }
    // 같은 현재 커서/에너지/배치/수명/건설을 한 번 조립한다. UI 소비자는 이후 실제 장치가 연결한다.
    void ConnectCursor() {
        if (cursor) throw std::logic_error("실제 캠페인 커서 중복 연결");CampaignCursorGame game;
        game.prepare=[this] { PrepareConstruction();gameplayPlacement.geometry.patternTypes=constructionState.confirm.patternTypes;cursorDrop.localPlayer=playerState.battleOwner;cursorDrop.battle=core.ownerMode.battle ? 1U : 0U;cursorDrop.blocked=globals.missionPost; };
        game.lifecycle.source=[this](Sid form) { return SquidForms(core.pool).Parent(form); };
        game.lifecycle.sourceNotice=[this](Sid parent,Sid form,std::uint32_t reason) { contentNotice->Notice(parent,form,reason); };
        game.lifecycle.detach=[this](Sid sid,std::uint32_t flags) {
            if (core.pool.Slot(sid)[10]==6) { if (flags) throw std::logic_error("커서 ContentForm 분리 flags0 계약");contentForms->Unpop(sid); }
            else world.Unpop(sid,flags);
        };
        game.lifecycle.unpop=[this](Sid sid,std::uint32_t flags) {
            const auto raw=core.pool.Slot(sid);
            if (raw[10]==6) { SquidForms(core.pool).Pop(sid,flags);return; }
            world.Pop(sid,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),flags);
        }; // 원본 가상+4c는 void 해제/복귀 Pop이며 +48 분리와 구별한다.
        game.lifecycle.destroy=[this](Sid sid,std::uint32_t flags) { if (!world.Destroy(sid,flags)) throw std::logic_error("커서 실제 가상 삭제 거부"); };
        game.lifecycle.addLocalSp=[this](float delta) { hp.AddSp(playerState.battleOwner,delta); };
        game.lifecycle.sound=[this](std::string_view name) {
            if (!services.sound.play) throw std::logic_error("커서 실제 소리 장치 미연결");services.sound.play(soundNames.Lookup(name),0,0,0,0,0);
        };
        game.playDrop=[play=game.lifecycle.sound] { play("droppiece.wav"); };
        BindCursorManaLifecycle(game.lifecycle,core.processes,core.kernel,*cursorMana);BindCursorRangeLifecycle(game.lifecycle,core.processes,*cursorRange);
        game.placement.describe=[this](std::string& text,float x,float y,std::uint32_t type,std::uint32_t owner) { return energySupply->Evaluate(text,x,y,type,owner); };
        game.placement.source=game.lifecycle.source;
        game.placement.region=[this](Sid sid) { const auto graph=anchor->GraphFor(sid.value);return graph==254 ? 0 : static_cast<std::int32_t>(graph+1); }; // 원본 GraphRecord 포인터의 NULL/같음만 보존하는 토큰이다.
        game.placement.mayPlace=[this](const CanonPlacementQuery& query) { const auto result=MayPlaceGameplay(query);cursor->Placement().obstruction=gameplayPlacement.placement.blockedRelation;return result ? 1U : 0U; };
        game.placement.processEvent=game.lifecycle.processEvent;
        game.placement.project=[this](float x,float y) { const auto point=SoundToScreen(cursorView,x,y);return ScreenPoint{point.x,point.y}; };
        game.removeRange=[this](Sid parent,std::uint32_t type) {
            RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { if (!world.Destroy(target,flags)) throw std::logic_error("회전 사정거리 실제 가상 삭제 거부"); });
            static_cast<void>(lookup.Find(parent,type,0,true));
        };
        game.moveBuilder=[this](std::uint32_t owner,Sid target,std::uint32_t low,std::uint32_t high) { static_cast<void>(buildingMover->Move(owner,target,low,high)); };
        cursor=std::make_unique<RawCampaignCursor>(CampaignCursorData{world,assets.game,gameplayPlacement.geometry,metadata.frames,*contentForms,*contained,hp,random,*construction,cursorDrop,gameplayPlacement.preview.blocked,sp,playerState.playerKnown,gunTargetState.windArcherType,cursorRangeGlobals.processType,contentCamera},std::move(game));
        std::array<ScreenColor,256> palette; // 현재 파일 팔레트를 원본 논리 검정/흰색 끝점으로 준비한다.
        // RGB와 예약 BYTE의 현재 파일 값을 그대로 공급한다.
        for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.blue,color.green,color.red,assets.game.Palette().Reserved(static_cast<std::uint8_t>(index)).value_or(0)}; }
        palette[0]={0,0,0,0};palette[255]={255,255,255,0};cursor->SetPalette(palette);cursor->SetView(cursorView);
    }
    // 같은 Session 전역을 건설 시작/프레임 직전에 갱신한다. 별도 건설 시계를 만들지 않는다.
    void PrepareBuilding(double now) {
        buildingState.now=now;buildingState.boss=assetState.boss;buildingState.localPlayer=playerState.battleOwner;
        buildingEffectsState.alliances=alliances;buildingEffectsState.server=core.processState.server;
    }
    // 원본30/17을 같은 Host/표시/최대 HP/음향/가상 공간에 연결한다.
    void ConnectBuilding() {
        if (building) throw std::logic_error("실제 건설 프로세스 중복 연결");
        buildingOwnerColors=BuildStandardOwnerColorMaps(0,core.bridgeState.bridgeType);buildingCaptureState.addTypeHotspot=true;
        // 실제43be30의 일반 색표 등록 대상이다. 다리/connector는 전용 처리 전까지 명시 거부한다.
        for (const auto type:std::array<std::uint32_t,12>{82,155,158,164,135,133,156,73,170,146,144,126}) buildingMapTokens[type]=1;
        // 실제43be30/43bc40의 전용 색표 대상은 일반 색표로 대신하지 않는다.
        for (const auto type:std::array<std::uint32_t,7>{82,155,111,141,137,94,97}) buildingMapTokens[type]=2;
        buildingCaptureState.ownerTypeMaps=buildingMapTokens;
        buildingCaptureState.ownerColors=[this](std::uint32_t type,std::uint32_t owner)->std::span<const std::uint8_t> {
            if (buildingMapTokens.at(type)!=1 || owner>=core.bridgeState.ownerColors.size()) throw std::logic_error("건설 전용 소유자 색표 미복원");
            const auto color=core.bridgeState.ownerColors[owner];if (color<0 || color>8) throw std::out_of_range("건설 현재 소유자 색 번호 오류");return buildingOwnerColors[static_cast<std::size_t>(color)];
        };
        buildingCapture=std::make_unique<RawShapeToBuffer>(core.pool,core.types,core.shapes,assets.game.Shapes(),buildingCaptureState);
        particles->ConnectConstruction([this](Sid sid) { return buildingCapture->Capture(sid); },[this](Sid sid) { return hp.MaxHitPoints(sid); });
        ConstructionEffectsHooks effects; // 밀어내기에 필요한 현재 이동 종류/내용물 삭제/가상 공간이다.
        effects.pathType=[this](std::uint32_t type) { return SelectMovementProcessType(core.pool.Edition(),core.types,priestState.movement,type); };
        effects.clearPath=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { world.Destroy(target,flags); });static_cast<void>(lookup.Find(sid,type,0,true)); };
        effects.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };effects.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        effects.fall=SessionMissing<void,Sid>();effects.sound=SessionMissing<void,float,float>();effects.destroy=SessionMissing<void,Sid,std::uint32_t>();effects.notify=SessionMissing<void,Sid>();effects.broadcast=SessionMissing<void,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>();
        buildingEffects=std::make_unique<RawConstructionEffects>(core.pool,core.hash,core.types,core.spots,buildingEffectsState,std::move(effects));
        ConstructionProcessHooks hooks; // 실제 Host/vector가 확보를 수행하며 실패를 예외로 보고한다.
        hooks.reserve=[] { return true; };hooks.reserveSound=[] { return true; };
        hooks.effect=[this](Sid sid,bool instant) { static_cast<void>(particles->AddConstruction(sid,instant)); };
        hooks.loadSound=[this](std::string_view name) { return soundNames.Lookup(name); };
        hooks.attachSound=[this](Sid sid,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) { SessionCheck(sounds->Add(sid,sound,alternate,flags)!=nullptr); };
        hooks.refresh=[this] { if (!services.construction.refresh) throw std::logic_error("건설 실제 화면 갱신 요청 미연결");services.construction.refresh(); };
        hooks.maxHitPoints=[this](Sid sid) { return hp.MaxHitPoints(sid); };hooks.updateDisplay=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        hooks.removeContained=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { world.Destroy(target,flags); });static_cast<void>(lookup.Find(sid,type,0,true)); };
        hooks.playAt=[this](float x,float y,std::string_view name,std::uint32_t loop) { services.sound.playOnceAt(x,y,soundNames.Lookup(name),loop,0); };
        hooks.readySound=[this](std::uint32_t type) { if (type<70 || type-70>=assets.game.Types().size()) throw std::out_of_range("건설 준비 소리 TYPE 오류");const auto name=assets.game.Types()[type-70].definition.String("buildDoneSound");return name && !name->empty() ? soundNames.Lookup(*name) : 0U; };
        hooks.play=[this](std::uint32_t sound) { services.sound.play(sound,0,0,0,0,0); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };hooks.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        hooks.push=[this](float x,float y,std::uint32_t type,std::uint32_t owner) { static_cast<void>(buildingEffects->Displace(x,y,type,owner)); };
        building=std::make_unique<RawConstructionProcess>(core.pool,core.types,buildingState,std::move(hooks),&core.processes);
    }
    // 송신 효과 뒤 또는 외부 수신 직전에 실제 현재 전역을 같은 기존 handler에 공급한다.
    void ReceiveGeyserMined(std::span<const std::uint8_t> packet) {
        geyserMinedState.boss=assetState.boss;geyserMinedState.checking=gunState.checking;geyserMinedState.nuggetType=motionState.carrier.nuggetType;
        geyserMined->HandlePacket(packet);
    }
    // 설치를 마친 동일 Graph/Host에 실제 사제 경로 모듈을 한 번 연결한다.
    void ConnectMotion() {
        if (motion) throw std::logic_error("실제 세션 경로 중복 연결");
        ConstructionArrivalHooks buildHooks;
        buildHooks.start=[this](Sid sid,float duration) { PrepareBuilding(core.processState.now);building->Start(sid,duration); };
        buildHooks.removeContained=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { world.Destroy(target,flags); });static_cast<void>(lookup.Find(sid,type,0,true)); };
        buildHooks.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        buildArrival=std::make_unique<RawConstructionArrival>(core.pool,core.hash,core.types,buildArrivalState,std::move(buildHooks));
        ManaBoltArrivalHooks manaHooks;
        manaHooks.sound=[this](float x,float y,std::string_view name) { services.sound.playOnceAt(x,y,soundNames.Lookup(name),0,0); };
        manaHooks.removeProcesses=[this](Sid sid,std::uint32_t type) { RemoveAttachedProcesses(core.processes,sid,type); };
        manaHooks.reserveRegular=[this] { try { manaPendingRegular=std::make_unique<RegularProcess>(1,manaArrivalState.delay);return true; }catch (const std::bad_alloc&) { return false; } };
        manaHooks.addRegular=[this](Sid sid,std::uint32_t event,float delay) { if (!manaPendingRegular || delay!=manaArrivalState.delay) throw std::logic_error("마나 볼트 Regular 예약 누락");auto* regular=static_cast<RegularProcess*>(core.processes.Attach(std::move(manaPendingRegular),manaArrivalState.regularType,sid,0x50));regular->ResetTask(event,core.processState.now); };
        manaHooks.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        manaArrival=std::make_unique<RawManaBoltArrival>(core.pool,core.hash,core.types,manaArrivalState,std::move(manaHooks));
        ManaBoltRegularHooks regularHooks;
        regularHooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); };
        regularHooks.sound=[this](float x,float y,std::string_view name) { services.sound.playOnceAt(x,y,soundNames.Lookup(name),0,0); };
        regularHooks.project=[this](float x,float y) { const auto point=SoundToScreen({contentScreenOrigin[0],contentScreenOrigin[1]},x,y);return std::array<std::int32_t,2>{point.x,point.y}; };
        regularHooks.inverse=[this](std::int32_t x,std::int32_t y) { return RawCursorPosition::ScreenToWorld(x,y,contentCamera); };
        regularHooks.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        regularHooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        regularHooks.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        manaRegular=std::make_unique<RawManaBoltRegular>(core.pool,core.types,core.frames,core.shapes,*manaArrival,manaRegularState,std::move(regularHooks));
        motionState.halt.battle=true;anchorState.graphReady=1;anchorState.fenceType=90;anchorState.windArcherType=140;
        anchor=std::make_unique<RawPlayerPlacementAnchor>(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,contentFinder,anchorState);
        auto effects=services.motion;
        effects.path.startBlocked=[this](RawPathPoint point,std::uint32_t type,std::uint32_t owner) { PrepareBuilding(core.processState.now);static_cast<void>(buildingEffects->Displace(point.x,point.y,type,owner)); };
        effects.path.arrivalBlocked=[this](Sid,RawPathPoint point,std::uint32_t type,std::uint32_t owner) { PrepareBuilding(core.processState.now);static_cast<void>(buildingEffects->Displace(point.x,point.y,type,owner)); };
        effects.path.constructionArrival=[this](Sid sid,std::uint32_t,std::uint32_t) { const auto type=core.pool.Slot(sid)[10];if (type==134) return buildArrival->Arrive(sid);if (type==105) return manaArrival->Arrive(sid);throw std::logic_error("실제 건설 도착 타입 오류"); };
        if (!effects.cleanup.reserveShared) effects.cleanup.reserveShared=[] { return true; };
        FillMissing(effects.cleanup.ritual);FillMissing(effects.cleanup.refund);
        FillMissing(effects.halt.carrierPickup);
        effects.carrier.genusAt=[this](float x,float y,std::uint32_t genus) {
            RawSquidFinder finder(core.pool,core.hash,core.types);
            // 실제 일반 getter의 첫 현재 후보/표면 생략 순서를 사용한다.
            for (auto sid=finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},(genus&6)==0 ? 1U : 0U);sid.value;sid=finder.Next())
                if (core.types[core.pool.Slot(sid)[10]].flags2&genus) return sid;
            return Sid{};
        };
        FillMissing(effects.carrier.cdPriestPickupPriest);FillMissing(effects.carrier.pickup);
        boardTarget=std::make_unique<RawBoardTarget>(boardTargetState,[](std::uint32_t line) { throw std::logic_error("원본 Boardcoord.cpp 대상 접근 진단 "+std::to_string(line)); });
        CarrierChoiceHooks choice;
        choice.movement=[this](Sid sid) {
            const auto kind=core.pool.Slot(sid)[10];if (kind<70 || kind-70>=assets.game.Types().size()) return std::array<float,2>{};
            const auto& definition=assets.game.Types()[kind-70].definition;return std::array<float,2>{static_cast<float>(definition.Number("speed").value_or(0)),static_cast<float>(definition.Number("turningSpeed").value_or(0))};
        };
        choice.footprint=[this](Sid sid) { return core.neighbors.Footprint(sid); };
        choice.search=[this](RawPathRuntime& temporary,Sid parent,RawPathPoint source,RawPathPoint destination,std::uint32_t flags) { if (!motion) throw std::logic_error("현재 이동 설치 전 운반 후보 탐색");static_cast<void>(motion->Paths().SearchCandidate(temporary,parent,source,destination,flags)); };
        choice.validReturn=[this](Sid parent,Sid candidate) { if (!motion) throw std::logic_error("현재 이동 설치 전 귀환 후보 검사");return motion->CarrierArrival().ValidReturn(parent,candidate); };
        choice.validGeyser=[this](Sid parent,Sid candidate) { if (!motion) throw std::logic_error("현재 이동 설치 전 간헐천 후보 검사");return motion->CarrierArrival().ValidGeyser(parent,candidate); };
        choice.graphFor=[this](Sid sid) { return anchor->GraphFor(sid.value); };
        choice.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Carrier.cpp 후보 선택 진단 "+std::to_string(line)); };
        const auto movement=choice.movement; // 같은 현재 TYPE 속성 getter를 회피 후보 평가에도 사용한다.
        carrierChoice=std::make_unique<RawCarrierChoice>(core.pool,core.types,players,assetState.outposts,assetState.geysers,core.processState.now,gunState.checking,*boardTarget,carrierChoiceState,std::move(choice));
        effects.carrier.chooseReturn=[this](Sid sid) { return carrierChoice->ChooseReturn(sid); };
        effects.carrier.chooseGeyser=[this](Sid sid) { return carrierChoice->ChooseGeyser(sid); };
        effects.carrier.currentControlled=[this](std::uint32_t owner) { return participation->Brain(owner)!=0; };
        CarrierTargetHooks target;
        target.footprint=[this](Sid sid) { return core.neighbors.Footprint(sid); };
        target.start=[this](RawPathStartRequest request)->CarrierPathResult {
            if (!motion) throw std::logic_error("현재 이동 설치 전 운반 대상 시작");const auto kind=core.pool.Slot(request.parent)[10];RawPathProcess* path{}; // 실제 시작의 반환 포인터를 호스트 폭으로 보관한다.
            if (kind==priestState.movement.priest) path=motion->Paths().StartPriest(request);
            else if (kind==133) path=motion->Paths().StartCarrier(request);
            else if (kind==priestState.movement.teleport || kind==priestState.movement.manaBolt) path=motion->Paths().StartConstruction(request);
            else throw std::logic_error("다른 실제 운반 경로38/41/42 미복원 경계");return reinterpret_cast<CarrierPathResult>(path);
        };
        const auto startEscape=target.start; // 같은489080 분배/실제BFS/Host/직렬화 시작이다.
        target.chooseReturn=effects.carrier.chooseReturn;
        carrierTarget=std::make_unique<RawCarrierTarget>(core.pool,core.types,players,assetState.boss,*boardTarget,std::move(target));
        effects.carrier.goReturn=[this](Sid sid,Sid place) { static_cast<void>(carrierTarget->GoReturn(sid,place)); };
        effects.carrier.goGeyser=[this](Sid sid,Sid geyser) { static_cast<void>(carrierTarget->GoGeyser(sid,geyser)); };
        effects.carrierGoHome=[this](Sid sid) { static_cast<void>(carrierTarget->GoHome(sid)); };
        CarrierEscapeHooks escape;
        escape.path=[this](Sid sid) { if (!motion) throw std::logic_error("현재 이동 설치 전 회피 깊이 조회");return motion->Paths().CurrentRuntime(sid); };
        escape.random=[this](std::int32_t low,std::int32_t high) { if (high<=low) throw std::invalid_argument("회피 난수 구간 오류");return low+static_cast<std::int32_t>(random.Next(static_cast<std::uint32_t>(high-low))); };
        escape.start=[startEscape](Sid sid,RawPathPoint destination) { const auto result=startEscape({sid,destination,0,0,0,0,0});return result ? &reinterpret_cast<RawPathProcess*>(result)->Runtime() : nullptr; };
        escape.blocker=[genusAt=effects.carrier.genusAt](RawPathPoint point) { return genusAt(point.x,point.y,0x10000); };
        escape.search=[this](RawPathRuntime& temporary,Sid parent,RawPathPoint source,RawPathPoint destination,std::uint32_t flags) { if (!motion) throw std::logic_error("현재 이동 설치 전 회피 탐색");static_cast<void>(motion->Paths().SearchCandidate(temporary,parent,source,destination,flags)); };
        escape.movement=movement;escape.valid=[this](RawPathPoint point) { return carrierFalling->ValidPoint(point.x,point.y); };
        escape.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Carrier.cpp 회피 깊이 진단 "+std::to_string(line)); };
        carrierEscape=std::make_unique<RawCarrierEscape>(core.pool,core.hash,core.types,core.spots,core.processState.now,gunState.checking,carrierChoiceState.cardinalTypes,carrierEscapeState,std::move(escape));
        services.motion.carrier.escape=[this](Sid sid,float distance,std::uint32_t event) { return carrierEscape->Run(sid,distance,event); };
        effects.carrier.escape=services.motion.carrier.escape;
        PathRetargetHooks retarget;
        retarget.find=[this](Sid sid,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { return priests->Carrier().FindContained(sid,mask,type,index); };
        retarget.chooseReturn=effects.carrier.chooseReturn;retarget.chooseGeyser=effects.carrier.chooseGeyser;
        retarget.goReturn=effects.carrier.goReturn;retarget.goGeyser=effects.carrier.goGeyser;
        pathRetarget=std::make_unique<RawPathRetarget>(core.pool,motionState.carrier.nuggetType,std::move(retarget));
        effects.path.retarget=[this](RawPathRuntime& runtime,Sid initialParent) { pathRetarget->Update(runtime,initialParent); };
        GeyserMinedHooks mined;
        mined.sound=[this](float x,float y,std::string_view name,std::uint32_t flags) { services.sound.playOnceAt(x,y,soundNames.Lookup(name),flags,0); };
        mined.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        mined.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        mined.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        geyserMined=std::make_unique<RawGeyserMined>(core.pool,core.hash,core.types,random,geyserMinedState,std::move(mined));
        CarrierMiningHooks mining;
        mining.broadcast=[this](std::span<const std::uint8_t> packet) { broadcast->Send(packet); };
        mining.receive=[this](std::span<const std::uint8_t> packet) { ReceiveGeyserMined(packet); };
        carrierMining=std::make_unique<RawCarrierMining>(core.pool,miningHeader,std::move(mining));
        effects.carrier.minedPacket=[this](Sid geyser) { carrierMining->Notify(geyser); };
        FillMissing(effects.carrier.cdAddSp);FillMissing(effects.carrier.cdOwnerPacket);FillMissing(effects.carrier.playAt);FillMissing(effects.carrier.robust);
        CarrierTransferHooks transfer;
        transfer.find=[this](Sid parent,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { return priests->Carrier().FindContained(parent,mask,kind,index); };
        transfer.detach=[this](Sid form,std::uint32_t flags) { if (flags) throw std::logic_error("너겟 전달의 실제 Unpop flags 오류");contentForms->Unpop(form); };
        transfer.attach=[this](Sid form,Sid parent,std::uint32_t flags) { contentForms->Attach(form,parent,flags); };
        transfer.preview=[this](Sid parent) { StartSelectionContentPreview(core.pool,parent,objectState.loadingDepth,[this](Sid sid,std::uint32_t event,float payload) { static_cast<void>(core.processes.AddRegular(sid,event,payload)); },SessionMissing<void>()); };
        transfer.broadcast=[this](std::span<const std::uint8_t> packet) { broadcast->Send(packet); };
        transfer.movementType=[this](std::uint32_t type) { return priests->MovementProcessType(type); };
        transfer.removeMovement=[this](Sid sid,std::uint32_t type) { RawContainedLookup lookup(core.pool,[this](Sid target,std::uint32_t flags) { static_cast<void>(world.Destroy(target,flags)); });static_cast<void>(lookup.Find(sid,type,0,true)); };
        transfer.chooseGeyser=effects.carrier.chooseGeyser;transfer.goGeyser=effects.carrier.goGeyser;
        transfer.goHome=effects.carrierGoHome;FillMissing(transfer.goHome);
        transfer.commonHalt=[this](Sid sid) { if (!motion) throw std::logic_error("현재 이동 연결 전 고갈된 캐리어 정지");return motion->CommonHalt(sid); };
        carrierTransfer=std::make_unique<RawCarrierTransfer>(core.pool,core.hash,core.types,assetState.boss,globals.editor,assetState.localOwner,carrierTransferState,std::move(transfer));
        effects.halt.nuggetTransfer=[this](Sid sid) { return carrierTransfer->Transfer(sid); };
        FillMissing(effects.priest.neutralReturn);
        effects.path.metadata=[this](Sid sid) {
            const auto& definition=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition;
            return RawPathStartType{static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame};
        };
        effects.path.soundId=[this](Sid sid) {
            const auto sound=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("stepSound");
            return sound && !sound->empty() ? soundNames.Lookup(*sound) : 0U;
        };
        FillMissing(effects.path.stepSound);
        motion=std::make_unique<RawCampaignMotionBindings>(world,*priests,*islands,*contained,*anchor,hp,metadata.frames,motionState,start,process,std::move(effects));
        FillMissing(services.pathPacket);
        auto pathTx=motion->Paths().Transmission(pathHeader,pathBuffer,[this](std::int32_t length) {
            if (length<=0 || static_cast<std::size_t>(length)>pathBuffer.size()) throw std::logic_error("세션 Path 송신 길이 오류");
            services.pathPacket(std::span<const std::uint8_t>(pathBuffer).first(static_cast<std::size_t>(length)));
        });
        core.processes.SetFormTransmit(AviaryTransmission(assetBindings->Transmission(std::move(pathTx))));
    }
    // 외부 콜백이 참조하는 모듈을 해제하기 전에 현재 Kernel을 비운다.
    ~Impl() {
        // 다른 AI의 손상/외부 예외가 나더라도 나머지 실제 brain과 Kernel 정리를 계속하고 오류를 남긴다.
        for (std::uint32_t owner=1;owner<9;++owner) if (participation) {
            try { participation->RemoveBrain(owner); }
            catch (const std::exception& error) { std::fprintf(stderr,"실제 Session AI 정리 실패 owner%u: %s\n",owner,error.what()); }
            catch (...) { std::fprintf(stderr,"실제 Session AI 정리 실패 owner%u: 알 수 없는 예외\n",owner); }
        }
        try { world.ClearProcesses(); }
        catch (const std::exception& error) { std::fprintf(stderr,"실제 Session Kernel 정리 실패: %s\n",error.what()); }
        catch (...) { std::fprintf(stderr,"실제 Session Kernel 정리 실패: 알 수 없는 예외\n"); }
        core.postPop.SetPlayerPostPop(nullptr);
        core.postPop.SetBuriedPostPop(nullptr); // 장기 모듈 해제 뒤 파생 포인터를 남기지 않는다.
        core.postPop.SetFlyerPostPop(nullptr); // 비행 장기 몸체 해제 뒤 공통 분배에 포인터를 남기지 않는다.
    }

};
// 현재 실제 자산/Template/서비스를 보관하고 모든 하위 모듈을 구성한다.
RawCampaignSession::RawCampaignSession(const GameAssets& assets,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices services)
    :impl_(std::make_unique<Impl>(assets,files,std::move(fort),std::move(mission),std::move(services))) {}
// Impl이 Kernel을 비운 뒤 실제 모듈을 해제한다.
RawCampaignSession::~RawCampaignSession()=default;
// 동일 원본 부모를 실행한다.
void RawCampaignSession::Load() { impl_->runtime->Load();impl_->ConnectBuilding();impl_->ConnectMotion();impl_->buildingMover=impl_->MakeBuilder(impl_->buildingMoverState,{});impl_->ConnectGameplayPlacement();impl_->ConnectCursor();impl_->loaded=true; }
// UI의 현재 크기/변경 전역을 빌리며 게임 소유자·모드는 실제 세션에서 다시 읽는다.
void RawCampaignSession::ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const {
    state.localPlayer=impl_->assetState.localOwner;state.fort=impl_->core.ownerMode.fort;
    state.battle=impl_->core.ownerMode.battle;state.challenge=impl_->core.ownerMode.challenge;
    CameraHome(impl_->core.pool,impl_->core.types,state,std::move(apply)).Reset();
}
// 같은 원본 건설 시작을 후속 배치/룬 부모와 공유한다.
void RawCampaignSession::StartConstruction(Sid parent,float duration) {
    if (!impl_->loaded || !impl_->building) throw std::logic_error("미설치 Session의 실제 건설 시작");
    impl_->PrepareBuilding(impl_->core.processState.now);impl_->building->Start(parent,duration);
}
// 현재 같은 시각을 공격/소리/실제 Kernel에 공급한다.
void RawCampaignSession::RunFrame(FrameTime time,bool paused) { impl_->PrepareBuilding(time.game);impl_->assetBindings->PrepareFrame(time.game);impl_->combat->PrepareFrame(time);impl_->particles->PrepareFrame(time.game);impl_->soundState.frame=static_cast<std::uint32_t>(time.number);impl_->world.RunFrame(time,paused); }
// 해상도/카메라 변경과 동일 위치 효과음의 view를 process11에도 전달한다.
void RawCampaignSession::SetParticleView(SoundView view,SurfaceDisplayHooks display) {
    impl_->contentScreenOrigin={view.cameraX,view.cameraY};
    impl_->cursorView=view;if (impl_->cursor) impl_->cursor->SetView(view);impl_->contentCamera=RawCursorPosition::CameraFromPixels(view.cameraX,view.cameraY);
    impl_->particles->SetView(view,std::move(display));
}
// 현재 단일 raw 표시 목록에서 SHP와 같은 정렬 항목을 만든다.
std::vector<RenderSprite> RawCampaignSession::ParticleSprites(ScreenRect clip) const { return impl_->particles->Sprites(clip); }
// 실제 표시 소유자이며 별도 검사용 모형이 아니다.
const RawCampaignParticles& RawCampaignSession::Particles() const { return *impl_->particles; }
// 원본 미션 게이트가 지정한 순서에 호출하며 Session Kernel 프레임에서는 자동 실행하지 않는다.
void RawCampaignSession::CheckMissionOutcome() { impl_->outcome->Check(); }
// 실제 worldLoad의 loginSubscriber 복사 대상과 같은 장기 전역을 매번 읽는다.
std::uint32_t RawCampaignSession::CurrentSubscriber() const { return impl_->globals.mySubscriber; }
// 로그인/기술/AI가 공유하는 현재 원본 로컬 Player 번호다.
std::uint32_t RawCampaignSession::LocalOwner() const { return impl_->playerState.battleOwner; }
// 실제 Player 생성/현재 기술/참여가 같은 DWORD를 읽는다.
std::uint32_t RawCampaignSession::PlayerKnown() const { return impl_->playerState.playerKnown; }
// 원본 로더의 현재 편집 조건과 같은 저장소를 직접 읽는다.
std::uint32_t RawCampaignSession::Editor() const { return impl_->globals.editor; }
// Get은 같은 분산배열의 값 비트를 모으고 float로 재해석하며 금액 변환/가산을 하지 않는다.
float RawCampaignSession::Sp(std::uint32_t owner) const { return std::bit_cast<float>(impl_->sp.Get(owner)); }
// Set의 실제 난수 잡음 쓰기도 같은 저장소/게임 난수를 사용한다.
void RawCampaignSession::SetSp(std::uint32_t owner,float value) { impl_->sp.Set(owner,std::bit_cast<std::uint32_t>(value)); }
// 후속 소비자는 현재 같은 원문 배열의 주소와 offset을 검사할 수 있다.
const ScrambledSpStore& RawCampaignSession::SpStore() const { return impl_->sp; }
// 원본 로그인/AI 초기화와 승패 판정은 같은9 Player 바이트를 공유한다.
std::span<std::uint8_t> RawCampaignSession::PlayerRecords() { return impl_->players; }
// 방향별 DWORD를 그대로 공유하여 한쪽 변경을 대칭화하거나 bool로 줄이지 않는다.
std::span<std::uint32_t> RawCampaignSession::Alliances() { return impl_->alliances; }
// 같은 현재 Player 참여/동맹/실제 AI 소유자 수명을 후속 로그인 부모에 빌린다.
RawPlayerParticipation& RawCampaignSession::Participation() { return *impl_->participation; }
// 채팅의 실제 현재 숨김표를 빌리며 표시 bool 사본을 만들지 않는다.
std::span<std::uint8_t> RawCampaignSession::HiddenPlayers() { return impl_->hiddenPlayers; }
// Stage3 원본의1..8 방문 계약을 지키며 설치 전 요청을 명시 거부한다.
std::uint32_t RawCampaignSession::CreateAi(std::uint32_t owner,std::span<const std::uint8_t> settings) {
    if (!impl_->loaded || owner<1 || owner>=9) throw std::logic_error("실제 Session 설치 후 owner1..8 AI 생성만 허용");
    return impl_->aiCreate->Create(owner,settings);
}
// 명시 해제의 오류는 caller에 전파하고 destructor의 마지막 정리와 구분한다.
void RawCampaignSession::RemoveAi(std::uint32_t owner) { impl_->participation->RemoveBrain(owner); }
// 후속 실제 전략은 현재 Player 토큰으로 이 유일한 본체/목록 저장소를 조회해야 한다.
const RawAiBrainMemory& RawCampaignSession::AiMemory() const { return impl_->aiMemory; }
// 기존사제목록의현재owner/dead와같은풀의실제포획원소유자를명시조회한다.
std::uint32_t RawCampaignSession::FindPriestParticipation(std::uint32_t owner) const { return impl_->priestParticipation->Find(owner); }
// 원본로그인종료타이머소비자가선택한위치에서만기존ownerState1..8을갱신한다.
void RawCampaignSession::RefreshPriestParticipation(std::uint32_t owner) { impl_->priestParticipation->Refresh(owner); }
// 실제MissionOutcome/PostPop과동일한저장소인지후속소비자가확인한다.
const RawPriestParticipation& RawCampaignSession::PriestParticipation() const { return *impl_->priestParticipation; }
// 배치 가능 판정을 통과한 서버 요청을 현재 원본 모드와 같은 일반 확정/통지에 전달한다.
bool RawCampaignSession::ConfirmConstruction(const ConstructionConfirmRequest& request) {
    if (!impl_->loaded) throw std::logic_error("미설치 Session의 일반 건설 확정");impl_->PrepareConstruction();return impl_->construction->Confirm(request);
}
// 현재 실제 공간/권한/지형 파이프라인을 커서와 서버 요청에서 공유한다.
bool RawCampaignSession::MayPlaceConstruction(const CanonPlacementQuery& query) { impl_->PrepareConstruction();return impl_->MayPlaceGameplay(query); }
// 실제 서버 요청 전체를 현재 모드로 실행한다. 일반 Confirm으로 바로 건너뛰지 않는다.
bool RawCampaignSession::RequestConstruction(const ConstructionConfirmRequest& request) { if (!impl_->loaded) throw std::logic_error("미설치 Session의 건설 요청");impl_->PrepareConstruction();return impl_->construction->Request(request); }
// 기존 실제 패킷 분배기/로컬 전송이 같은 서버 수신 몸체를 호출한다.
void RawCampaignSession::RegisterConstructionCommands(RawPacketDispatch& dispatch) { if (!impl_->loaded) throw std::logic_error("미설치 Session의 건설 수신 등록");impl_->construction->Register(dispatch,[this] { impl_->PrepareConstruction(); }); }
// 실제 초기 등록과 같은 버전/정책/빈 observer이며 수신 시점의 현재 권한/타입을 읽는다.
void RawCampaignSession::RegisterCarrierCommands(RawPacketDispatch& dispatch) {
    if (!impl_->loaded) throw std::logic_error("미설치 Session의 채취 수신 등록");RegisterGeyserMined(dispatch,*impl_->geyserMined);
    dispatch.Register(0x5b,[this](std::uint32_t,PacketConnection&,std::span<const std::uint8_t> packet) { impl_->ReceiveGeyserMined(packet); });
}
// 원본 부모 설치 전의 임의 공간 쓰기를 거부하며 같은 현재 noIsland 전역으로 요청을 구성한다.
bool RawCampaignSession::ConfirmSupport(float x,float y) {
    if (!impl_->loaded) throw std::logic_error("실제 Session 설치 전 받침 확정 요청");
    impl_->PrepareConstruction();return impl_->construction->ConfirmSupport({impl_->core.bridgeState.noIslandType,x,y,0,0,0,0,0,0,0});
}
// 로그인/미션 단계의 자동 호출을 추가하지 않고 실제 부모가 요청할 때만 현재 raw 기록으로 생성한다.
void RawCampaignSession::GenerateLocalGeysers() {
    if (!impl_->loaded) throw std::logic_error("실제 Session 설치 전 로컬 간헐천 생성 요청");
    impl_->PrepareConstruction();impl_->construction->GenerateLocalGeysers();
}
// 초기 객체 이전에 설치한 같은 연결부를 제공하며 게임 요청은 명시 Session API를 사용한다.
const RawCampaignConstructionBindings& RawCampaignSession::Construction() const { return *impl_->construction; }
// 동일 Pool/Player 기록과 현재 Factory 목록을 읽으며 자동 재계산을 추가하지 않는다.
const RawCampaignProductionBindings& RawCampaignSession::Production() const { return *impl_->production; }
// 현재 세 카운터와 생산이 사용하는 같은 금지DWORD 표를 빌린다.
RawTotalMade& RawCampaignSession::TotalMade() { return *impl_->totalMade; }
// 같은 PlayerWorld 생성자/Owner/직접frame0/Pop으로 실제 기술 부모 기록을 설치한다.
Sid RawCampaignSession::CreatePlayer(std::int32_t owner) { if (!impl_->loaded) throw std::logic_error("미설치 Session의 Player 생성");return impl_->player->Create(owner); }
// 원본 시작/AI 기술 처리가 사용하는 같은 현재 부모/실제 contained 생성 경로다.
void RawCampaignSession::AddTechnology(std::int32_t owner,std::string_view list) { impl_->technology->AddList(owner,list); }
// 제거 통지는 현재 Player SP 접두와 직접 Gump 가상 처리까지 같은 경로를 사용한다.
void RawCampaignSession::ClearTechnology(std::int32_t owner) { impl_->technology->Clear(owner); }
// 상태 사본 없이 현재 raw 부모 체인에서 읽는다.
Sid RawCampaignSession::FindTechnology(std::int32_t owner,std::int32_t type) const { return impl_->technology->Find(owner,type); }
// 실제 내용물 생성/부착/현재 부모 통지의 같은 장기 몸체를 명령 소비자에게 제공한다.
Sid RawCampaignSession::CreateContained(Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attachFlags,std::uint32_t quality) {
    if (!impl_->loaded || !impl_->contained) throw std::logic_error("설치 전 실제 내용물 생성 요청");
    return impl_->contained->Create(parent,type,argument,flags,attachFlags,quality);
}
// 실제 동일 Host54/BFS 경로를 시작하며 원본 재귀의 외부nullptr 뒤 현재 경로도 확인한다.
bool RawCampaignSession::MovePriest(Sid sid,RawPathPoint destination) {
    if (!impl_->motion) throw std::logic_error("실제 세션 설치 전 사제 명령");
    auto* path=impl_->motion->Paths().StartPriest({sid,destination,1,0,0,0,0});
    return path!=nullptr || impl_->motion->Paths().CurrentPath(sid)!=nullptr;
}
// Draw의 조건부 소리도 기존 세션의 단일 실제 이름표와 장치 경계를 사용한다.
void RawCampaignSession::DrawRainTemple(Sid sid,const std::function<void(std::int32_t)>& frame) const {
    const auto raw=impl_->core.pool.Slot(sid);
    DrawRainVortex(impl_->core.pool.Edition(),raw,impl_->core.frames[raw[10]],{frame,[this] {
        if (!impl_->services.sound.play) throw std::logic_error("레인 신전 상승음 실제 재생 미연결");
        impl_->services.sound.play(impl_->soundNames.Lookup("templeAscend.wav"),0,0,0,0,0);
    }});
}
// 실제 설치된 Session의 게임 계산을 연결하고 UI의 실제 번역/작성/커서 소비자는 보존한다.
std::unique_ptr<RawPriestMenu> RawCampaignSession::MakePriestMenu(PriestMenuHooks hooks) {
    if (!impl_->loaded) throw std::logic_error("설치 전 실제 Session 사제 메뉴 요청");
    auto* current=impl_.get(); // 반환 메뉴가 빌리는 고정 주소이며 Session의 수명 안에서만 유효하다.
    auto& state=current->priestMenu;
    state.localOwnerRead=[current] { return current->player->State().battleOwner; };
    state.workshopModeRead=[current] { return current->core.ownerMode.battle; };
    state.editorRead=[current] { return current->globals.editor!=0; };
    state.chargeBlockedRead=[current] { return current->globals.missionPost; };
    BindPriestMenuWorld(hooks,current->core.pool,current->priests->PriestState(),current->core.processes,
        *current->player,current->priests->Carrier(),current->hp,*current->contained);
    if (!hooks.takePreview) current->cursor->Bind(hooks); // 실제 UI가 별도 취득 소비자를 주지 않으면 같은 세션 커서를 사용한다.
    hooks.allowed=[current](std::uint32_t type) { return current->totalMade->Allowed(static_cast<std::int32_t>(type)); };
    hooks.money=[current](std::uint32_t owner) { return std::bit_cast<float>(current->sp.Get(owner)); };
    return std::make_unique<RawPriestMenu>(current->core.pool,current->core.types,current->core.frames,current->players,
        current->constructionExcludedTypes,current->contentFinder,state,std::move(hooks));
}
// 원본 사제/Carrier/회수/공통 객체/Player 몸체를 같은 현재 월드와 실제 Client 경계로 조립한다.
std::unique_ptr<RawCampaignObjectMenus> RawCampaignSession::MakeObjectMenus(CampaignObjectMenuUi ui) {
    if (!impl_->loaded) throw std::logic_error("설치 전 실제 Session 객체 메뉴 체인 요청");
    auto* current=impl_.get(); // 반환 체인의 모든 게임 getter가 빌리는 고정 주소다.
    auto menus=std::make_unique<RawCampaignObjectMenus::Impl>(std::move(ui),current->rewardState.cdLocalSp);auto* chain=menus.get();
    auto& inputs=chain->ui; // 이 주소는 반환 객체의 수명 동안 바뀌지 않는다.
    inputs.objectState.localOwnerRead=[current] { return current->playerState.battleOwner; };
    inputs.objectState.editorRead=[current] { return current->globals.editor!=0; };
    inputs.objectState.specialRead=[current] { return current->playerState.special; };
    inputs.objectState.chargeBlockedRead=[current] { return current->globals.missionPost; };
    auto reads=inputs.identity; // 주소/실패 이름/선택적 metadata는 현재 Client의 실제 입력을 유지한다.
    reads.localOwner=inputs.objectState.localOwnerRead;reads.subscriber=[current] { return current->globals.mySubscriber; };
    reads.battle=[current] { return current->core.ownerMode.battle; };reads.editor=inputs.objectState.editorRead;
    reads.metamap=[current] { return current->core.ownerMode.challenge; };
    reads.name=[current] { const auto end=std::find(current->handle.begin(),current->handle.end(),std::uint8_t{});if (end==current->handle.end()) throw std::logic_error("현재 Session 이름 NUL 누락");return std::string_view(reinterpret_cast<const char*>(current->handle.data()),static_cast<std::size_t>(end-current->handle.begin())); };
    chain->identity=std::make_unique<RawPlayerIdentity>(current->core.pool.Edition(),current->players,inputs.meta,inputs.chat,std::move(reads));
    auto player=inputs.player; // 실제 Chat/허용/통신/서식은 caller의 동일 Client 경계다.
    player.mission=[current] { return current->globals.missionPost; };player.fort=[current] { return current->core.bridgeState.mission; }; // Player 메뉴의 fort 경계는00594fa0이며 inFortMode00594fb8과 다르다.
    player.money=[current](std::uint32_t owner) { return std::bit_cast<float>(current->sp.Get(owner)); };
    chain->player=std::make_unique<RawPlayerMenu>(*chain->identity,current->alliances,current->hiddenPlayers,current->priestState.priests.ownerState,inputs.playerState,std::move(player));
    auto object=inputs.object; // GUI/설정/문자열 토큰은 유지하며 게임 계산만 현재 실제 몸체로 교체한다.
    object.hasHitPoints=[current](Sid sid) { return current->hp.HasHitPoints(sid); };
    const auto derivedCount=object.count; // 원본83VT 중 두 파생 가상80만 외부 실제 분배기를 요구한다.
    object.count=[current,derivedCount](Sid sid) {
        const auto raw=current->core.pool.Slot(sid);const auto type=static_cast<std::size_t>(raw[10]);
        if (type>=current->core.types.size() || Read(raw,0)!=current->core.factory.ConstructorVtable(current->core.types[type])) throw std::logic_error("현재 객체 메뉴 가상80 VT 미연결");
        const auto table=Read(raw,0); // 보호 PE의 나머지81 생성 VT는 모두 실제 base004ad0f0을 가리킨다.
        if (table==0x5076b8U || table==0x507950U) { if (!derivedCount) throw std::logic_error("객체 메뉴 파생 가상80 미연결");return derivedCount(sid); }
        return current->level->Get(sid);
    };
    object.salvageRefund=[current](Sid sid,std::uint32_t type,std::uint32_t count) { return current->hp.SalvageRefund(sid,type,std::bit_cast<std::int32_t>(count)); };
    BindObjectMenuIdentity(object,*chain->identity);BindObjectPlayerMenu(object,*chain->player);
    const auto actualCount=object.count; // 정보와회수는같은현재VT검증/실제가상80을공유한다.
    chain->object=std::make_unique<RawObjectMenu>(current->core.pool,current->core.types,inputs.objectState,std::move(object));
    // 기존RiftTypeRecord에서빠진+aa문자열만실제정의에서보충하며TYPE/flags/theme를복제하지않는다.
    for (const auto& asset:current->assets.game.Types()) if (const auto value=asset.definition.String("class")) {
        const auto text=value->substr(0,value->find('\0'));if (text.size()>23) throw std::length_error("현재객체메뉴 class23바이트초과");chain->classes[70+asset.block]=text;
    }
    chain->infoState.localOwner=inputs.objectState.localOwnerRead;chain->infoState.editor=[current] { return current->globals.editor; };chain->infoState.missionPost=[current] { return current->globals.missionPost; };
    auto info=inputs.info;info.count=actualCount;info.ownerName=[chain](std::uint32_t owner) { return chain->identity->NameForOwner(owner); };
    info.alliance=[current](std::uint32_t owner,std::uint32_t local) { if (owner>=9 || local>=9) throw std::out_of_range("현재객체메뉴 동맹범위오류");return current->alliances[owner*9+local]; };
    info.groundImmobile=[current](Sid sid) { return current->priests->PriestState().GroundImmobile(sid); };
    info.ownerStatus=[current](std::uint32_t owner) { if (owner>=9) throw std::out_of_range("현재객체메뉴 Player범위오류");return current->players[owner*180+0x50]; };
    chain->info=std::make_unique<RawObjectMenuInfo>(current->core.pool,current->core.types,chain->classes,chain->infoState,std::move(info));
    CarrierDropPointHooks point;point.mayPlace=[current](const CanonPlacementQuery& query) { current->PrepareConstruction();return current->MayPlaceGameplay(query); };
    point.center=[current](Sid sid) { return current->gunTarget->Center(sid); };
    chain->dropPoint=std::make_unique<RawCarrierDropPoint>(current->core.pool,current->core.types,chain->dropState,std::move(point));
    chain->lookup=std::make_unique<RawContainedLookup>(current->core.pool,[current](Sid sid,std::uint32_t flags) { if (!current->world.Destroy(sid,flags)) throw std::logic_error("현재 메뉴 종속 가상 삭제 거부"); });
    auto carrier=inputs.carrier; // 현재 이동/Host/위치/SP를 같은 월드에 연결한다.
    BindCarrierMenuWorld(carrier,current->core.pool,current->priests->PriestState(),current->core.processes,*chain->lookup,current->hp,
        [current](std::uint32_t type) { return current->priests->MovementProcessType(type); },[current](Sid sid) { return current->core.processes.FromForm(sid)!=nullptr; });
    BindCarrierMenuDropPoint(carrier,current->core.pool,*chain->dropPoint);BindCarrierMenuBase(carrier,*chain->object);
    carrier.money=[current](std::uint32_t owner) { return std::bit_cast<float>(current->sp.Get(owner)); };
    chain->carrierState.localOwnerRead=inputs.objectState.localOwnerRead;chain->carrierState.editorRead=inputs.objectState.editorRead;
    chain->carrier=std::make_unique<RawCarrierMenu>(current->core.pool,current->core.types,current->contentFinder,chain->carrierState,std::move(carrier));
    auto priest=inputs.priest;priest.takePreview={}; // 검사/별도 취득 callback 대신 항상 같은 세션의 실제 커서를 사용한다.
    BindPriestMenuCarrier(priest,*chain->carrier);chain->priest=MakePriestMenu(std::move(priest));
    return std::unique_ptr<RawCampaignObjectMenus>(new RawCampaignObjectMenus(std::move(menus)));
}
// 공통Open의게임조회만현재Session으로교체한다. GUI몸체/가상+a8/+ac의대체성공은추가하지않는다.
std::unique_ptr<RawObjectMenuOpen> RawCampaignSession::MakeObjectMenuOpen(ObjectMenuOpenHooks hooks) {
    if (!impl_->loaded) throw std::logic_error("설치 전 실제 Session 공통 객체 메뉴 요청");
    auto* current=impl_.get(); // 반환몸체가빌리는현재게임의고정주소다.
    hooks.altar=[current](Sid sid) {
        const auto raw=current->core.pool.Slot(sid);return current->world.Find(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),current->runeState.altarType);
    };
    hooks.project=[current](float x,float y) { const auto point=SoundToScreen(current->cursorView,x,y);return std::array<std::int32_t,2>{point.x,point.y}; };
    hooks.content=[current](Sid sid) { return current->priests->Carrier().FindContained(sid,1,0,0); };
    hooks.range=[current](std::uint32_t type) { if (type>=current->gunTypes.size()) throw std::out_of_range("객체 메뉴 현재 사정거리TYPE 범위 오류");return current->gunTypes[type].range; };
    return std::make_unique<RawObjectMenuOpen>(current->core.pool,current->core.types,std::move(hooks));
}
// 초기설치이전에는Form생성/부착을노출하지않는다. 별도UI전용풀을만들지않는다.
GumpFormHost& RawCampaignSession::GumpForms() { if (!impl_->loaded || !impl_->gumpForms) throw std::logic_error("설치 전 실제 Session GumpForm 요청");return *impl_->gumpForms; }
// 큰 UI/현재 관문을 보존하고 같은 월드가 이미 소유하는 게임 계산·표시 프로세스만 연결한다.
std::unique_ptr<RawUserSelection> RawCampaignSession::MakeUserSelection(UserSelectionState state,UserSelectionHooks hooks) {
    if (!impl_->loaded) throw std::logic_error("설치 전 실제 Session 선택 요청");auto* current=impl_.get(); // 반환 몸체가 빌리는 같은 장기 Session이다.
    hooks.localOwner=[current] { return current->playerState.battleOwner; };hooks.editor=[current] { return current->globals.editor; };
    hooks.dais=[current] { return current->runeState.daisType; };hooks.manaProcess=[current] { return current->cursorManaGlobals.processType; };hooks.rangeProcess=[current] { return current->cursorRangeGlobals.processType; };
    hooks.refresh=[current](Sid sid) { current->core.display.Update(current->core.pool.Slot(sid)); };
    const auto marks=std::make_shared<RawSquidSelection>(current->core.pool,hooks.refresh); // 실제+7c의 표시 전 비트와 반환 몸체의 수명을 함께 유지한다.
    hooks.selectNotice=[marks](Sid sid,std::uint32_t on) { marks->Mark(sid,on); };
    hooks.processNotice=[current](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* process=current->core.processes.FromForm(form);if (!process) throw std::logic_error("선택의 실제 ProcessForm/Kernel 미일치");process->Notify(event,info,value); };
    hooks.showMana=[current](Sid parent,std::uint32_t form) { if (form>65535U) throw std::out_of_range("선택 마나 Form WORD 오류");static_cast<void>(CreateCursorManaProcess(current->core.processes,*current->cursorMana,parent,Sid{static_cast<std::uint16_t>(form)})); };
    hooks.showRange=[current](Sid parent,std::uint32_t form) { if (form>65535U) throw std::out_of_range("선택 사정거리 Form WORD 오류");static_cast<void>(CreateCursorRangeProcess(current->core.processes,*current->cursorRange,parent,Sid{static_cast<std::uint16_t>(form)})); };
    hooks.rangeType=[current](std::uint32_t type) { if (type>=current->core.types.size()) throw std::out_of_range("선택 사정거리 TYPE 오류");return SelectionUsesRange(current->core.types[type]) ? 1U : 0U; };
    hooks.content=[current](Sid parent) { return current->priests->Carrier().FindContained(parent,1,0,0); };
    hooks.altar=[current](Sid parent) { const auto raw=current->core.pool.Slot(parent);return current->world.Find(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),current->runeState.altarType); };
    hooks.moving=[current](Sid parent) { return current->core.processes.FindEvent(parent,0x493)!=nullptr ? 1U : 0U; };
    const auto previewAssertion=hooks.previewAssertion; // 서로 다른 원본 진단을 selectedSid 진단으로 합치지 않는다.
    hooks.movementPreview=[current,previewAssertion](Sid parent) { StartSelectionContentPreview(current->core.pool,parent,current->objectState.loadingDepth,[current](Sid sid,std::uint32_t event,float payload) { static_cast<void>(current->core.processes.AddRegular(sid,event,payload)); },previewAssertion); };
    hooks.sound=[current](std::uint32_t x,std::uint32_t y,std::string_view name,std::uint32_t flags) { if (!current->services.sound.playOnceAt) throw std::logic_error("선택 실제 위치 소리 미연결");current->services.sound.playOnceAt(std::bit_cast<float>(x),std::bit_cast<float>(y),current->soundNames.Lookup(name),flags,0); };
    auto selection=std::make_unique<RawUserSelection>(current->core.pool,current->core.types,state,std::move(hooks));auto link=std::make_shared<Impl::SelectionLink>();link->selection=selection.get();selection->sessionLifetime_=link;current->activeSelection=link;return selection;
}
// 기본 번호/특수 모드는 같은 장기 상태를 빌리며 getter를 새 캐시로 바꾸지 않는다.
PriestMenuState& RawCampaignSession::PriestMenuSettings() { return impl_->priestMenu; }
// 원본 TYPE 제외 목록을 현재 값으로 빌린다. 다른 SID/생산 금지 표와 합치지 않는다.
SquidPostPopList& RawCampaignSession::ConstructionExcludedTypes() { return impl_->constructionExcludedTypes; }
// UI와 실제 마나 프로세스가 동일한 현재 커서 토큰을 공유한다.
CursorDropState& RawCampaignSession::CursorDropSettings() { return impl_->cursorDrop; }
// 실제 설치 이후 같은 커서를 빌리며 새 몸체나 상태를 만들지 않는다.
RawCampaignCursor& RawCampaignSession::Cursor() { if (!impl_->loaded || !impl_->cursor) throw std::logic_error("설치 전 실제 커서 요청");return *impl_->cursor; }
// 이후 커서/맵 명령도 프레임이 읽는 같은 전역을 갱신한다.
CursorManaGlobals& RawCampaignSession::CursorManaSettings() { return impl_->cursorManaGlobals; }
// 실제 Session 소유의 생성·표시·공간·가상 삭제·중심과 현재 Host/Kernel을 조립한다.
void RawCampaignSession::BindCursorMana(CursorLifecycleHooks& hooks) {
    if (!impl_->loaded) throw std::logic_error("미설치 Session의 커서 마나 연결");
    BindCursorManaLifecycle(hooks,impl_->core.processes,impl_->core.kernel,*impl_->cursorMana);
}
// 마나와 같은 실제 맵/현재 총 전역을 읽는 장기 상태다.
CursorRangeGlobals& RawCampaignSession::CursorRangeSettings() { return impl_->cursorRangeGlobals; }
// 설치/소유권 확인 뒤 실제 두 프로세스와 공통 현재 Kernel 분배기를 연결한다.
void RawCampaignSession::BindCursorRange(CursorLifecycleHooks& hooks) {
    BindCursorMana(hooks);BindCursorRangeLifecycle(hooks,impl_->core.processes,*impl_->cursorRange);
}
// 실제 동일 장부의 공급원과 현재 TYPE/동맹을 조회한다.
const RawEnergySupply& RawCampaignSession::EnergySupply() const { return *impl_->energySupply; }
// 호출자가 바꾼 원본 공급 설정을 다음 실제 조회에서 읽는다.
EnergySupplyState& RawCampaignSession::EnergySettings() { return impl_->energyState; }
// 커서 조회 뒤 생산/Builder도 실제 같은 선택 목록을 소비하게 한다.
SquidPostPopList& RawCampaignSession::EnergySources() { return impl_->energySources; }
// 도달한 실제 에너지 판정만 연결하며 다른 GUI/공간 효과를 기본 성공으로 만들지 않는다.
void RawCampaignSession::BindCursorEnergy(CursorPlacementHooks& hooks) {
    if (!impl_->loaded) throw std::logic_error("미설치 Session의 커서 에너지 연결");
    hooks.describe=[this](std::string& text,float x,float y,std::uint32_t type,std::uint32_t owner) { return impl_->energySupply->Evaluate(text,x,y,type,owner); };
}
// 실제 원문 하위들을 조립하며 경로/접근점을 독립 직선 계산으로 대체하지 않는다.
std::unique_ptr<RawConstructionBuilder> RawCampaignSession::MakeConstructionBuilder(ConstructionBuilderState& state,ConstructionBuilderHooks hooks) {
    if (!impl_->loaded) throw std::logic_error("미설치 Session의 건설 운반 연결");return impl_->MakeBuilder(state,std::move(hooks));
}
// 화면과 입력은 같은 살아 있는 상태를 읽는다.
RawPathProcess* RawCampaignSession::StartConstructionMovement(RawPathStartRequest request) {
    if (!impl_->loaded || !impl_->motion) throw std::logic_error("미설치 Session의 건설 이동");return impl_->motion->Paths().StartConstruction(request);
}
// 같은 원본489080과 현재40 클래스의 생성/부모 통지를 사용한다.
RawPathProcess* RawCampaignSession::StartCarrierMovement(RawPathStartRequest request) { if (!impl_->loaded || !impl_->motion) throw std::logic_error("미설치 Session의 Carrier 이동");return impl_->motion->Paths().StartCarrier(request); }
// 원본 네트워크/내부 명령 소비자는 같은 실제 처리기를 사용한다.
const RawCarrierTransfer& RawCampaignSession::CarrierTransfer() const { if (!impl_->loaded || !impl_->carrierTransfer) throw std::logic_error("미설치 Session의 캐리어 전달");return *impl_->carrierTransfer; }
// 외부 명령과 같은 운반 도착 몸체가 실제 하나의 대상 이동 처리기를 사용한다.
const RawCarrierTarget& RawCampaignSession::CarrierTarget() const { if (!impl_->loaded || !impl_->carrierTarget) throw std::logic_error("미설치 Session의 운반 대상 이동");return *impl_->carrierTarget; }
// 기존CarrierRegular의지연호출과부모도착도이몸체를공유한다.
std::uint32_t RawCampaignSession::EscapeCarrier(Sid parent,float distance,std::uint32_t event) { if (!impl_->loaded || !impl_->carrierEscape) throw std::logic_error("미설치 Session의 Carrier 회피");return impl_->carrierEscape->Run(parent,distance,event); }
// 외부 요청과 도착/전달이 같은 실제 후보 선택기를 공유한다.
const RawCarrierChoice& RawCampaignSession::CarrierChoice() const { if (!impl_->loaded || !impl_->carrierChoice) throw std::logic_error("미설치 Session의 운반 후보 선택");return *impl_->carrierChoice; }
// 화면과 입력은 같은 살아 있는 상태를 읽는다.
RawSurfaceWorld& RawCampaignSession::World() { return impl_->world; }
RawWorldCore RawCampaignSession::Core() { return impl_->core; }
const SquidReward& RawCampaignSession::HitPoints() const { return impl_->hp; }
const RawPriestWorldBindings& RawCampaignSession::Priests() const { return *impl_->priests; }
}
