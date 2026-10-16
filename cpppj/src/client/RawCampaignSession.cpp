// 기존 원본 몸체의 단일 저장소 조립을 실제 클라이언트 세션이 소유한다.
#include "client/RawCampaignSession.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawCampaignGunQueries.h"
#include "client/AiTypeRequiredEnergy.h"
#include "o/RawGameRandomSeed.h"
#include "o/RawSquidFinder.h"
#include "o/RawSquidLevel.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawArcherTypeData.h"
#include "o/RawProjectileGeometry.h"
#include "o/RawFormPacket.h"
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
#include "o/RawPriestLookup.h"
#include "o/RawRunePostPop.h"
#include "o/RawVortexPreDestroy.h"
#include "o/RawBuriedPostPop.h"
#include "o/RawBuriedRegular.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawRainAviaryProcess.h"
#include "o/RawFlyerProcess.h"
#include "o/RawFlyerFrame.h"
#include "o/RawDamageableRelease.h"
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
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
// 실제 월드/상태와 모든 callback 대상을 같은 고정 주소에 소유한다.
struct RawCampaignSession::Impl {
    RawCampaignSessionServices services; // 실제 클라이언트의 선택 서비스이며 검사 관찰 기본값을 사용하지 않는다.

    SessionAssets assets; // 보호 archive의 현재 실제 자료다.
    PriestPlacementAssets metadata; // 실제 프레임/핫스폿/SHP이며 합성 그림 자료가 아니다.
    std::uint32_t terrainType{97}; // 실제 월드 생성과 영역 소유자 getter가 함께 읽는 isle 타입 전역이다.
    RawSurfaceWorld world; // 모든 생성/Pop/Graph가 사용하는 단일 raw 월드다.
    RawWorldCore core; // 같은 Pool/장부/해시의 참조만 유지한다.
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
    std::vector<GunTargetType> gunTypes; // 보호 TYPE의 사거리/방향/높이 보충 자료다.
    std::vector<GunHitType> hitTypes; // 보호 TYPE의 HP/초와 별도 피해 보충 자료다.
    std::vector<float> firingIntervals; // 실제 타입별 발사 간격이다.
    std::unique_ptr<RawGunMode> gunMode; // 현재 raw 공격 방향을 매 검색마다 읽는다.
    std::unique_ptr<RawGunTarget> gunTarget; // 실제 finder/ray/필터의 같은 Core 인스턴스다.
    std::unique_ptr<RawGunHit> gunHit; // 실제 Q16 시간 압축과 현재 피해 getter다.
    PriestDamageState priestDamageState; // 실제 미션 레벨보정 차단/HP1 감시 권한이다.
    DamageableBurningState burningState; // 현재 nonzero damageEffect의 실제 생성 판정 전역이다.
    GunImpactState impactState; // 현재 충돌 시각 효과 억제/anim160 전역이다.
    std::vector<DudeNoticeTypeData> dudeSpeeds; // 실제 TYPE 이동속도의 현재 mutable 보충 표다.
    std::unique_ptr<RawPlayerDamageNotice> playerDamage; // 같은 Player9/모드의 실제 AI 조회와 피해 통지 래퍼다.
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
    std::uint32_t archerCommand{0x030000b0}; // 보호 PE의 실제 입력 토큰이다.
    std::unique_ptr<RawCampaignAssetBindings> assetBindings; // 실제 저장 건물 후처리와 Gun/Regular다.
    CanonPlacementPipelineState placement; // 실제 공간/기하/지형 허용 상태다.
    SquidPostPopList additional; // 기존 pipeline이 읽는 실제 추가 목록이다.
    ContainedFinderState contentFinder; // 기존 내용물 검색의 장기 전역이다.
    std::unique_ptr<RawCanonPlacementPipeline> pipeline; // 실제 Same Core MayPlace다.
    TerrainContextGlobals terrainGlobals; // 정상 문자 mapping의 실제 전역이다.
    TerrainStampState stamp; // 첫 생성 이후에도 유지되는 실제99cache다.
    FortRandomPlacementState randomPlacement; // 실제 지형/edgeFarm 배치 상태다.
    FarmClusterState farm; // 실제 정상 군집 모드다.
    std::unique_ptr<RawNormalTerrainWorldBinding> normal; // 실제 성장/도장/Normalize/군집 몸체다.
    std::unique_ptr<RawFortFinishOwner> finish; // 같은9 Player의 최초 지역을 읽는다.
    ContainedCreateState contentState; // 저장 내용물은 실제 Form6으로 생성한다.
    std::unique_ptr<ContentFormHost> contentForms; // ProcessForm과 다른 실제 Form6의 정렬 부착 몸체다.
    std::unique_ptr<RawContentParentNotice> contentNotice; // 부모의 실제 현재 VT로 기본/파생 +24를 분배한다.
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
    std::unique_ptr<RawFenceMarkerClear> fenceMarkerClear; //같은 hash/가상 삭제의 기존 fencemark 제거다.
    std::unique_ptr<RawFenceMarkerCreate> fenceMarkerCreate; //같은 실제 중심/생성/통지의 내부 표식 생성이다.
    std::unique_ptr<RawFenceShieldCreate> fenceShieldCreate; //같은 현재TYPE/프레임의 화면 포스 필드 생성이다.
    std::unique_ptr<RawFenceRebuild> fenceRebuild; //같은 동맹 최근접/사거리/두 생성기로 연결을 재생성한다.
    std::unique_ptr<RawSunAviaryProcess> sunAviary; // 같은 현재 Kernel/시계/풀의 실제 훨리베이스 프로세스 몸체다.
    std::unique_ptr<RawRainAviaryProcess> rainAviary; // 같은 현재 Kernel/풀의 맨 오 워 풀 전용 상태 기계다.
    std::unique_ptr<RawFlyerProcess> flyer; // 같은 실제 Host의 공통 비행 생성/전송/홈 연결이다.
    std::unique_ptr<RawFlyerFrame> flyerFrame; // 같은 현재 시각/부모/표적/소리 장부의 실제 비행 Frame 제어다.
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
        // 원본 Player의 첫 지역 sentinel은9기록에 직접 쓴다. 별도 playerRegions 배열을 사용하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) {
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
        FillMissing(priestEffects.fallSound);FillMissing(priestEffects.otherWalkerFall);
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
        }
        // 보호 .type의 실제 숫자/문자 Run을 같은 TYPE 번호의 보충 표에 연결한다.
        for (const auto& asset:assets.game.Types()) {
            const auto type=static_cast<std::uint32_t>(70+asset.block);auto& target=gunTypes[type];target.range=static_cast<float>(asset.definition.Number("range").value_or(0));
            target.heightExtension=static_cast<int>(asset.definition.Number("height").value_or(0));
            dudeSpeeds[type].speed=static_cast<float>(asset.definition.Number("speed").value_or(0));
            // 원본 방향별 L/M/N/O 첫 프레임 표다. 없는Run의-1은 실제 검색 몸체가 처리한다.
            for (std::uint8_t direction=0;direction<4;++direction) target.directionFrames[direction]=core.frames[type].Run(static_cast<std::uint8_t>('L'+direction)).first;
            hitTypes[type].damagePerSecond=static_cast<float>(asset.definition.Number("hpPerSec").value_or(0));
            firingIntervals[type]=static_cast<float>(asset.definition.Number("delayBetweenShots").value_or(0));
            archerTypes[type]=ReadRawArcherTypeData(asset.definition);missileTypes[type].range=target.range;
            missileTypes[type].frameCount=archerTypes[type].directions;
            const auto firingSound=asset.definition.String("firingSound"),impactSound=asset.definition.String("impactSound");
            missileTypes[type].firingSound=firingSound && !firingSound->empty() ? soundNames.Lookup(*firingSound) : 0;
            missileTypes[type].impactSound=impactSound && !impactSound->empty() ? soundNames.Lookup(*impactSound) : 0;
            auto& geometry=projectileTypes[type];geometry.defaultFrame=asset.definition.specialFrames.defaultFrame;
            geometry.frameCount=archerTypes[type].frameCount;geometry.heightExtension=target.heightExtension;
            // 각 물리 SHP header 마지막 두 WORD를 그대로 읽으며 표시 hotspot과 혼용하지 않는다.
            for (std::size_t frame=0;frame<core.shapes[type].frames.size();++frame) {
                const auto header=assets.game.Shapes().SquidHeader(asset.block,frame);
                geometry.shotOffsets.push_back({std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[32]|(std::uint16_t(header[33])<<8))),
                    std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[34]|(std::uint16_t(header[35])<<8)))});
            }
        }
        gunMode=std::make_unique<RawGunMode>(core.pool,core.frames,gunState.windArcherType);
        gunTarget=std::make_unique<RawGunTarget>(core.pool,core.hash,core.types,gunTypes,core.shapes,gunTargetState);
        priestDamageState.levelCorrectionDisabled=globals.missionPost;priestDamageState.boss=assetState.boss;
        auto hitEffects=services.hit;
        hitEffects.priest.defaultFrame=[this](Sid sid) { return projectileTypes[core.pool.Slot(sid)[10]].defaultFrame; };
        playerDamage=std::make_unique<RawPlayerDamageNotice>(players,core.ownerMode);
        // 실제 Player AI가 없는 자연 반환은 래퍼가 판단한다. 비영 AI의 미복원 몸체는 명시 오류를 유지한다.
        if (!hitEffects.priest.ownerDamageNotice) hitEffects.priest.ownerDamageNotice=
            [this](std::uint8_t owner,Sid victim,Sid attacker) { playerDamage->Notice(owner,victim,attacker); };
        FillMissing(hitEffects.burning.create);FillMissing(hitEffects.burning.bulfSound);
        FillMissing(hitEffects.dudes.createBurning);FillMissing(hitEffects.dudes.pathEndpoint);FillMissing(hitEffects.dudes.startPath);
        hits=std::make_unique<RawCampaignHitBindings>(world,*priests,hp,random,dudeSpeeds,assetState.residences,
            priestDamageState,burningState,impactState,std::move(hitEffects));
        gunHit=std::make_unique<RawGunHit>(core.pool,core.types,hitTypes,hits->DamageHooks(GunHitHooks{SessionMissing<void,Sid,std::int32_t,std::uint8_t,Sid>(),[this](Sid sid) { core.building.NotifySurface(sid); }}));
        projectileGeometry=std::make_unique<RawProjectileGeometry>(core.pool,core.types,projectileTypes);
        fenceTarget=std::make_unique<RawFenceTarget>(core.pool,core.hash,core.types,projectileTypes,*gunTarget,gunTargetState,gunState);
        auto combatEffects=services.archer;
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
        combatEffects.missile.muzzle=[this](Sid sid) { return projectileGeometry->ArcherMuzzle(sid); };
        combatEffects.missile.targetDimensions=[this](Sid sid) { return projectileGeometry->Dimensions(sid); };
        combatEffects.missile.targetBounds=[this](Sid sid,std::array<float,2> point) { return projectileGeometry->TargetBounds(sid,point); };
        FillMissing(combatEffects.missile.firingSound);FillMissing(combatEffects.missile.impactSound);
        combatEffects.missile=hits->MissileHooks(*gunHit,missileState,std::move(combatEffects.missile));
        combat=std::make_unique<RawCampaignArcherBindings>(world,*gunTarget,random,archerTypes,missileTypes,archerState,missileState,std::move(combatEffects));
        auto assetEffects=services.assets;assetEffects.postPop.reserveRegular=[] { return true; };
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
        InstallContents(worldHooks.assets);
        ConnectConstruction(); // 실제공통삭제후설치뒤같은noIsland VT의표면가상분배를첫객체전등록한다.
        ConnectDamageable(); // 같은 공통 삭제와 신전/사제 해방의 파생 하위를 첫 객체 전에 연결한다.
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
            if (FenceCommon(event.sid)) { //두 실제 가상 표의+14/+18은 공통004b0950/004b0840이다.
                if (event.effect==SquidDestroyEffect::PreDestroy) { world.CommonDeletion().PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { world.CommonDeletion().PostDestroy(event.sid,event.flags);return; }
            }
            if (Vortex(event.sid)) {
                if (event.effect==SquidDestroyEffect::PreDestroy) { vortexPre->PreDestroy(event.sid,event.flags);return; }
                if (event.effect==SquidDestroyEffect::PostDestroy) { damageablePostState.battle=core.ownerMode.battle;damageablePostState.boss=assetState.boss;damageablePost->PostDestroy(event.sid,event.flags);return; }
            }
            previous(event);
        };
        auto previousDestroy=std::move(worldHooks.destroy); // 실제 신전 VT의 공통+10만 추가 분배한다.
        worldHooks.destroy=[this,previous=std::move(previousDestroy)](Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) {
            if (Vortex(sid) || FenceCommon(sid)) return core.virtualDestroy.Destroy(sid,flags,hooks);return previous(sid,flags,hooks);
        };
        buriedRegular=std::make_unique<RawBuriedRegular>(core.pool,core.frames,core.frame,random);
        auto actualRegular=hits->Handler(combat->Handler(assetBindings->Handler(buriedRegular->Handler(priests->RegularHandler([this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            if (playerRegular->Handles(sid)) return playerRegular->Handle(sid,event,count,payload);
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
        regular.refreshNeighbors=[](Sid) { throw std::logic_error("썬 바리케이드004578f0 이웃 갱신 미복원"); };
        regular.rebuild=[this](Sid sid) { fenceRebuild->Rebuild(sid); };
        regular.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return assetBindings->Regular().DamageableHandle(sid,event,count,payload); };
        fenceRegular=std::make_unique<RawFenceRegular>(core.pool,core.frames,core.frame,std::move(regular));
    }
    // 실제 PE의 신전 네 가상 표를 판정하며 다른 자산을 타입 이름만으로 허용하지 않는다.
    bool Vortex(Sid sid) const {
        const auto table=Read(core.pool.Slot(sid),0);return table==0x005196f0 || table==0x005197a8 || table==0x00519860 || table==0x00519918;
    }
    //실제 표식90/포스필드170의+10은 이미 복원한 공통004af780이며 다른 펜스 파생은 포함하지 않는다.
    bool FenceCommon(Sid sid) const {
        const auto raw=core.pool.Slot(sid);const auto table=Read(raw,0);return (raw[10]==90 && table==0x005092d0) || (raw[10]==170 && table==0x00509388);
    }
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
    // 원본 제어/프레임은 실제 큰 소비자에 연결하고 아직 미복원인 표적 검색/포위/무기는 선택 시 구체적으로 거부한다.
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
        hooks.scatter=[](Sid,Sid)->std::array<float,2> { throw std::logic_error("플라이어 포위 오프셋0045bb80 미복원"); };
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
        hooks.closerCount=[](Sid,Sid)->std::int32_t { throw std::logic_error("플라이어 거리별 경쟁수0045b840 미복원"); };
        hooks.capacity=[this](Sid sid) { if (!sid.value) return 3;const auto raw=core.pool.Slot(sid);const auto& type=core.types[raw[10]];return !(raw[11]&2U) && type.footX==1 && type.footY==1 ? 1 : 3; }; // 실제0045b9c0은1x1 live/dead 구별 후1 또는3이다.
        hooks.findTarget=[](FlyerProcessFields&) { throw std::logic_error("플라이어 표적 검색0045c4b0 미복원"); };
        hooks.addDepend=[this](Sid target,Sid form) { static_cast<void>(core.dependencies.Create(target,form)); };hooks.removeDepend=[this](Sid form,Sid target) { core.dependencies.Remove(form,target,0); };
        hooks.refreshForm=[this](FlyerProcessFields& fields) { auto* process=dynamic_cast<FlyerProcess*>(core.processes.FromForm(fields.Form()));if (!process || &process->Fields()!=&fields) throw std::logic_error("비행 Frame 현재 Host/raw 이미지 불일치");core.processes.RefreshForm(*process); };
        hooks.returnDelay=[](FlyerProcessFields&) { return 10000000.0; }; // 실제0045c2e0이 읽는00509ce0 float의 정확한 값이다.
        hooks.command=SessionMissing<void,Sid,std::uint32_t>();
        hooks.sunProcess=[this](Sid sid,std::uint32_t type)->SunAviaryProcessFields& { const auto form=FindAttachedProcess(core.pool,sid,type);auto* process=form.value ? dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(form)) : nullptr;if (!process) throw std::logic_error("착륙 대상의 실제 훨리베이스 프로세스 미연결");return process->Fields(); };
        hooks.refreshSun=[this](SunAviaryProcessFields& fields) { auto* process=dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(fields.Form()));if (!process || &process->Fields()!=&fields) throw std::logic_error("착륙 훨리베이스 Host/raw 이미지 불일치");core.processes.RefreshForm(*process); };
        hooks.homeNotice=SessionMissing<void,Sid,std::uint32_t,std::uint32_t>();
        hooks.runCount=[this](Sid sid) { const auto raw=core.pool.Slot(sid);const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));const auto& table=core.frames[raw[10]];if (frame<0 || static_cast<std::size_t>(frame)>=table.Codes().size()) throw std::logic_error("플라이어 현재 글자 프레임 정상 범위 밖");return table.Run(table.Codes()[static_cast<std::size_t>(frame)].side).count; };
        hooks.launch=[](Sid,Sid,std::uint32_t) { throw std::logic_error("맨 오 워 무기 생성00472690 미복원"); };
        hooks.sound=[this](float x,float y,std::uint32_t token) { if (!services.sound.playOnceAt) throw std::logic_error("맨 오 워 실제 발사 소리 미연결");services.sound.playOnceAt(x,y,token,0,0); };
        std::vector<FlyerFrameType> details(core.types.size()); // 같은 이미 파싱한 TYPE 사거리와 실제 사운드 이름표 토큰이다.
        for (std::size_t type=0;type<details.size();++type) details[type]={gunTypes[type].range,missileTypes[type].firingSound}; // 현재 단일 TYPE 번호를 유지한다.
        flyerFrame=std::make_unique<RawFlyerFrame>(core.pool,core.types,details,std::span(soundState.typeFrames).first(core.types.size()),FlyerFrameGlobals{core.processState.server,assetState.boss,gunState.checking,core.processState.now,soundState.frame,sunFlyerType,windFlyerType,rainFlyerType,sunAviaryType,soundState.processType},std::move(hooks));
    }
    // 타입31/32/60의 서로 다른 writer와 같은 실제 송신기를 연결한다.
    ProcessFormTransmission AviaryTransmission(ProcessFormTransmission fallback) {
        ProcessFormTransmission hooks;
        hooks.writeData=[this,fallback](SquidProcess& process,bool parentChanged) { auto* flying=dynamic_cast<FlyerProcess*>(&process);writingFlyer=flying!=nullptr;auto* rain=dynamic_cast<RainAviaryProcess*>(&process);writingRainAviary=rain!=nullptr;auto* current=dynamic_cast<SunAviaryProcess*>(&process);writingSunAviary=current!=nullptr;if (flying) return static_cast<std::int32_t>(flying->WritePacket(flyerPacket,parentChanged));if (rain) return static_cast<std::int32_t>(rain->WritePacket(rainAviaryPacket,parentChanged));if (current) return static_cast<std::int32_t>(current->WritePacket(sunAviaryPacket,parentChanged));if (!fallback.writeData) throw std::logic_error("다른 실제 프로세스 writer 미연결");return fallback.writeData(process,parentChanged); };
        hooks.transmit=[this,fallback](std::int32_t length) { if (writingFlyer) { if (length<0 || static_cast<std::size_t>(length)>flyerPacket.size()) throw std::out_of_range("비행 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(flyerPacket).first(static_cast<std::size_t>(length)));return; }if (writingRainAviary) { if (length<0 || static_cast<std::size_t>(length)>rainAviaryPacket.size()) throw std::out_of_range("맨 오 워 풀 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(rainAviaryPacket).first(static_cast<std::size_t>(length)));return; }if (writingSunAviary) { if (length<0 || static_cast<std::size_t>(length)>sunAviaryPacket.size()) throw std::out_of_range("훨리베이스 실제 패킷 길이 오류");services.formPacket(std::span<const std::uint8_t>(sunAviaryPacket).first(static_cast<std::size_t>(length)));return; }if (!fallback.transmit) throw std::logic_error("다른 실제 프로세스 송신 미연결");fallback.transmit(length); };return hooks;
    }
    // 실제 Damageable 해방/공통 삭제와 신전 Pre를 같은 현재 월드에 조립한다.
    void ConnectDamageable() {
        DamageableReleaseHooks release;release.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        release.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        release.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };release.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        release=MakePriestSpawnHooks(core.pool,priests->Spawn(),std::move(release));
        damageableRelease=std::make_unique<RawDamageableRelease>(core.pool,core.types,core.spots,contentFinder,damageableReleaseState,std::move(release));
        auto pre=services.damageablePre;FillMissing(pre.collapse);FillMissing(pre.explosion);FillMissing(pre.sound);
        pre.validateBase=[this](Sid sid,std::uint32_t flags) { world.CommonDeletion().ValidatePre(sid,flags); };
        pre.base=[this](Sid sid,std::uint32_t flags) { world.CommonDeletion().PreDestroy(sid,flags); };
        pre.releaseContained=[this](Sid sid) { damageableReleaseState.battle=core.ownerMode.battle;damageableRelease->Release(sid); };
        damageablePre=std::make_unique<RawDamageablePreDestroy>(core.pool,core.spots,contentFinder,damageablePreState,std::move(pre));priests->BindDamageable(*damageablePre);
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
    }
    // 각 명시 요청에서 현재 실제 소유자/서버/권한/요새/타입만 공급한다. Player/자원/지도는 복사하지 않는다.
    void PrepareConstruction() {
        constructionState.place.localPlayer=constructionState.notice.localPlayer=assetState.localOwner;
        constructionState.place.server=core.processState.server;
        constructionState.place.authority=constructionState.clear.authority=constructionState.notice.authority=constructionState.geyser.boss=assetState.boss;
        constructionState.geyser.fort=core.ownerMode.fort;
        constructionState.place.noIslandType=constructionState.geyser.noIslandType=core.bridgeState.noIslandType;
        constructionState.place.bridgeType=constructionState.clear.bridgeType=core.bridgeState.bridgeType;
        constructionState.confirm.patternTypes[0]=piece;constructionState.confirm.patternTypes[1]=core.bridgeState.bridgeType;
        constructionState.confirm.patternTypes[2]=core.bridgeState.islandType;constructionState.confirm.patternTypes[3]=core.bridgeState.noIslandType;
        constructionState.clear.patternTypes=constructionState.place.patternTypes=constructionState.confirm.patternTypes;
    }
    // 수명/자산의 실제 파생 동작을 설치한 뒤 첫 객체 생성 전에 받침/로컬 간헐천 연결을 한 번 구성한다.
    void ConnectConstruction() {
        if (construction) throw std::logic_error("실제 Session 받침/간헐천 중복 연결");
        recentGeysers.entries.resize(5);PrepareConstruction();
        auto effects=services.construction; // 미선택 외부 경계는 실제 호출 때 오류로 드러내며 성공 관찰로 채우지 않는다.
        FillMissing(effects.configInt);FillMissing(effects.wallSeconds);FillMissing(effects.refresh);
        FillMissing(effects.negativeQuantity);FillMissing(effects.boundsAssertion);
        construction=std::make_unique<RawCampaignConstructionBindings>(world,hp,sp,rewardState,metadata.frames,chunks,players,*broadcast,constructionState,
            PlayerGeyserBoundsState{core.ownerMode.battle,objectState.mission,core.ownerMode.challenge,geyserMapSize,piece,playerDirections},
            recentGeysers,assetState.geysers,std::move(effects));
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
        FillMissing(contentEffects.cancelCursor);FillMissing(contentEffects.localNotice);FillMissing(contentEffects.remoteNotice);
        FillMissing(contentEffects.wallet);FillMissing(contentEffects.addSp);FillMissing(contentEffects.sound);
        contentLifetimeState.localOwner=assetState.localOwner;contentLifetimeState.boss=assetState.boss;
        contentLifetimeState.notice=0x020000b4; // PATCH005429c4의 실제 수입 명령 초기값이다.
        lifetime=std::make_unique<RawCampaignLifetimeBindings>(world,*contentForms,contentLifetimeState,
            std::move(contentEffects),hp,*playerContent,services.contentNotice);
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
    // 설치를 마친 동일 Graph/Host에 실제 사제 경로 모듈을 한 번 연결한다.
    void ConnectMotion() {
        if (motion) throw std::logic_error("실제 세션 경로 중복 연결");
        motionState.halt.battle=true;anchorState.graphReady=1;anchorState.fenceType=90;anchorState.windArcherType=140;
        anchor=std::make_unique<RawPlayerPlacementAnchor>(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,contentFinder,anchorState);
        auto effects=services.motion;
        if (!effects.cleanup.reserveShared) effects.cleanup.reserveShared=[] { return true; };
        FillMissing(effects.cleanup.ritual);FillMissing(effects.cleanup.refund);
        FillMissing(effects.halt.carrierPickup);FillMissing(effects.halt.nuggetTransfer);
        effects.carrier.genusAt=[this](float x,float y,std::uint32_t genus) {
            RawSquidFinder finder(core.pool,core.hash,core.types);
            // 실제 일반 getter의 첫 현재 후보/표면 생략 순서를 사용한다.
            for (auto sid=finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},(genus&6)==0 ? 1U : 0U);sid.value;sid=finder.Next())
                if (core.types[core.pool.Slot(sid)[10]].flags2&genus) return sid;
            return Sid{};
        };
        FillMissing(effects.carrier.cdPriestPickupPriest);FillMissing(effects.carrier.pickup);FillMissing(effects.carrier.escape);
        FillMissing(effects.carrier.chooseReturn);FillMissing(effects.carrier.chooseGeyser);FillMissing(effects.carrier.goReturn);FillMissing(effects.carrier.goGeyser);
        FillMissing(effects.carrier.minedPacket);FillMissing(effects.carrier.cdAddSp);FillMissing(effects.carrier.cdOwnerPacket);FillMissing(effects.carrier.playAt);FillMissing(effects.carrier.robust);
        FillMissing(effects.priest.neutralReturn);
        effects.path.metadata=[this](Sid sid) {
            const auto& definition=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition;
            return RawPathStartType{static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame};
        };
        effects.path.soundId=[this](Sid sid) {
            const auto sound=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("stepSound");
            return sound && !sound->empty() ? soundNames.Lookup(*sound) : 0U;
        };
        FillMissing(effects.path.stepSound);FillMissing(effects.path.startBlocked);FillMissing(effects.path.arrivalBlocked);FillMissing(effects.path.retarget);
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
void RawCampaignSession::Load() { impl_->runtime->Load();impl_->ConnectMotion();impl_->loaded=true; }
// UI의 현재 크기/변경 전역을 빌리며 게임 소유자·모드는 실제 세션에서 다시 읽는다.
void RawCampaignSession::ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const {
    state.localPlayer=impl_->assetState.localOwner;state.fort=impl_->core.ownerMode.fort;
    state.battle=impl_->core.ownerMode.battle;state.challenge=impl_->core.ownerMode.challenge;
    CameraHome(impl_->core.pool,impl_->core.types,state,std::move(apply)).Reset();
}
// 현재 같은 시각을 공격/소리/실제 Kernel에 공급한다.
void RawCampaignSession::RunFrame(FrameTime time,bool paused) { impl_->assetBindings->PrepareFrame(time.game);impl_->combat->PrepareFrame(time);impl_->soundState.frame=static_cast<std::uint32_t>(time.number);impl_->world.RunFrame(time,paused); }
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
// 원본 시작/AI 기술 처리가 사용하는 같은 현재 부모/실제 contained 생성 경로다.
void RawCampaignSession::AddTechnology(std::int32_t owner,std::string_view list) { impl_->technology->AddList(owner,list); }
// 제거 통지는 현재 Player SP 접두와 직접 Gump 가상 처리까지 같은 경로를 사용한다.
void RawCampaignSession::ClearTechnology(std::int32_t owner) { impl_->technology->Clear(owner); }
// 상태 사본 없이 현재 raw 부모 체인에서 읽는다.
Sid RawCampaignSession::FindTechnology(std::int32_t owner,std::int32_t type) const { return impl_->technology->Find(owner,type); }
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
// 화면과 입력은 같은 살아 있는 상태를 읽는다.
RawSurfaceWorld& RawCampaignSession::World() { return impl_->world; }
RawWorldCore RawCampaignSession::Core() { return impl_->core; }
const SquidReward& RawCampaignSession::HitPoints() const { return impl_->hp; }
const RawPriestWorldBindings& RawCampaignSession::Priests() const { return *impl_->priests; }
}
