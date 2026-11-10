// Client의 실제 건설 상태와 그 raw 참조가 빌리는 장기 owner를 함께 유지한다.
#pragma once
#include "client/RawCampaignConstructionBindings.h"
#include "o/RawCanonPlacementPipeline.h" //현재게임배치의상태/부모를같은Client수명에보유한다.
#include "o/RawFortRandomPlacement.h" //단일난수배치의현재전역과실제본체를공유한다.
#include "o/RawGemReward.h" //오벨리스크TYPE와보상Caller가같은장기주소를읽는다.
#include "o/RawIslandRuntime.h" //신전/영역삭제가같은섬실행기와현재Player를공유한다.
#include "o/RawRegionPaint.h" //도장캐시/영향목록과실제전체투표를장기로보유한다.
#include "o/RawVortexOwner.h" //신전의실제Player/섬/투표가상소유자부모다.
#include "o/RawVortexAscend.h" // 신전 상승도 같은 장기 섬/난수/삭제를 사용한다.
#include <vector> // 중첩 상승 Tick의 실제 후보 버퍼를 각각 보유한다.
#include "o/RawVortexPreDestroy.h" //네신전의실제전체삭제전처리를공유한다.

namespace netstorm::client { // Session 상태를 복제하지 않고 같은 Client 저장소에 결합한다.
class ClientGameplayGlobals; // 현재 Player/카메라/미션 전역의 실제 owner다.
struct ClientBuildReplayGlobals; // 현재 재생 DWORD와 객체를 같은 수명으로 보관한다.
class ClientSpGlobals; //초기화/난수 소비 없이 같은 실제 SP 저장소를 보유한다.
class ClientPlayerRecords; //현재 Player base를 원문 선택 위치에서만 읽는 실제 할당 owner다.
class ClientRandomGlobals; //메뉴/SP/현재게임과같은실제난수한개를보유한다.
class ClientKencloudGlobals; //같은실제0059a8b0의수명을섬/영역본체보다오래유지한다.
struct RawSurfaceVortexDevices; // 장기 신전 정기가 같은 실제 Construction owner를 빌리는 장치다.
class Client; //현재 Config/시계/Renderer/Tell을 실제 Client 장치로 연결한다.
// Session/World 없이 소비할 수 있는 실제 현재 Client 장치다. 큰 게임 효과와 분리한다.
struct ClientConstructionDevices {
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; //현재 설정을 읽고 키가 없으면 전달된 원문 fallback을 유지한다.
    std::function<double()> wallSeconds; //원문 비결정적 시드의 같은 실제 벽시각이다.
    std::function<void()> refresh; //현재 Renderer의 실제 전체 화면 갱신이다.
    std::function<void()> negativeQuantity; //원문의 수량 진단과 복귀를 유지한다.
    std::function<void(o::PlayerGeyserBoundsAssertion)> boundsAssertion; //현재 Player/Bounds 진단의 실제 장치다.
    std::function<std::string(std::string_view,std::string_view)> format; //원래 비용 문구 서식의 문자열 입력/출력을 유지한다.
    std::function<void(std::string_view)> tell; //원문 표시 바이트를 현재 Tell 장치에서 UTF-8로 변환한다.
    o::RawProductionDeck::Fill allocationBytes; //스택Deck의원문미작성영역을현재실제힙입력으로채우며선택시점에만호출한다.
    std::function<void()> refreshProduction; //현재메뉴의실제02000003생산사건을같은Gump트리에보낸다.
    std::function<std::uint32_t()> themeMode; //현재theammode설정을원문기본1로선택위치마다읽는다.
};
ClientConstructionDevices MakeClientConstructionDevices(Client& client); //현재 Client만 참조하며 장치 조회/창 생성/게임 호출을 결합 시 실행하지 않는다.
// 현재 Session의 큰 효과만 짧게 빌리며 장기 body는 이 객체를 강하게 소유하지 않는다.
struct ClientConstructionRuntimeBindings {
    RawCampaignConstructionEffects effects; //실제 현재 설정/표시/비용 안내/진단의 콜백이다.
    RawCampaignConstructionGameplay gameplay; //실제 현재 Builder/사제/배치/건설 모듈의 콜백이다.
    std::function<std::shared_ptr<void>()> pin; //공개된 전체 Runtime을 선택 호출 동안만 보유한다. 생성/직접 로드는 호출자의 소유권을 따른다.
};
class ClientConstructionGlobals {
public:
    ClientConstructionGlobals()=default; // 새 Client의 건설 상태를 한 번 구성하며 실제 결합/게임 초기화는 따로 수행한다.
    ClientConstructionGlobals(const ClientConstructionGlobals&)=delete;ClientConstructionGlobals& operator=(const ClientConstructionGlobals&)=delete; // 같은 실제 전역 상태를 복제하지 않는다.
    ClientConstructionGlobals(ClientConstructionGlobals&&)=delete;ClientConstructionGlobals& operator=(ClientConstructionGlobals&&)=delete; // 현재 Session이 빌린 상태 주소를 이동하지 않는다.
    // 실제 소유자를 결합만 하며 World/Player/SP/난수/게임 상태를 초기화하지 않는다.
    void Bind(std::shared_ptr<RawSurfaceStorage> storage,std::shared_ptr<ClientGameplayGlobals> gameplay,std::shared_ptr<ClientBuildReplayGlobals> replay);
    bool Matches(const std::shared_ptr<RawSurfaceStorage>& storage,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::shared_ptr<ClientBuildReplayGlobals>& replay) const; // 다른 Client/전역을 섞기 전에 정체성만 확인한다.
    RawCampaignConstructionState& State(); // 같은 현재 건설 상태의 주소를 Session 교체 뒤에도 유지한다.
    const RawCampaignConstructionState& State() const; // 값 사본 없이 같은 장기 상태를 조회한다.
    std::uint32_t CurrentThemeMode() const; // 현재 실제 설정 장치를 호출하는 동안 보유하며 기본값 사본을 만들지 않는다.
    void BindDevices(ClientConstructionDevices devices); //필수 실제 장치를 검증해 현재 집합만 교체하며 게임 상태나 본체를 초기화하지 않는다.
    void ReleaseDevices(); //Client 최종 반납에서 함수 집합만 끊고 본체/현재 물리/State를 보존한다.
    void BindRuntime(const std::shared_ptr<ClientConstructionRuntimeBindings>& runtime); //실제 현재 대여만 약하게 공개하고 중복된 살아 있는 대여는 거부한다.
    void BindBacking(std::shared_ptr<ClientSpGlobals> sp,std::shared_ptr<ClientPlayerRecords> players,std::shared_ptr<std::array<std::uint8_t,87>> options); //최초메뉴전실제backing만보유하고SP/Player/Bounds를조회하지않는다.
    void ReplenishNuggets(); //현재권한/모드관문뒤같은장기Bounds와직접장기너겟배치를사용한다.
    o::CanonPlacementPipelineState& PlacementState(); //커서/요청과같은현재배치상태를빌리며Session사본을만들지않는다.
    o::PlayerPlacementAnchorState& PlacementAnchor(); //같은현재Graph/전체덱/기준점상태를공유한다.
    o::SquidPostPopList& PlacementAdditional(); //같은원문추가후보목록을다른조회기와공유한다.
    o::RawCanonPlacementPipeline& PlacementPipeline(); //현재장기Pool/공간/Canon/상태로완전한배치본체를한번구성한다.
    bool MayPlace(const o::CanonPlacementQuery& query); //실제현재Player/동맹/섬/모드를읽어전체배치판정을실행한다.
    void BindRandom(std::shared_ptr<ClientRandomGlobals> random); //같은실제난수owner만보유하고seed/난수소비를추가하지않는다.
    o::FortRandomPlacementState& RandomPlacementState(); //현재단일배치의TYPE/ownerOverride를모든NativeCaller가공유한다.
    o::FortFinishOwnerState& FinishOwnerState(); //섬/거주구역/반복배치의같은현재모드를공유한다.
    o::GemRewardState& GemState(); //배치/보상/변환의같은현재오벨리스크TYPE를공유한다.
    o::SquidPostPopList& ConstructionExcludedTypes(); //건설/메뉴/보상이같은현재TYPE제외목록을빌린다.
    o::RawGemReward& GemReward(); //현재Deck/난수/진단/실제생성·Pop·삭제를같은장기owner에조립한다.
    void BindRegionContext(std::shared_ptr<ClientKencloudGlobals> kencloud); //실제redrawowner만결합하며섬/Player/난수/캐시를초기화하지않는다.
    o::RegionPaintState& RegionPaintGlobals(); //Session교체후에도같은도장캐시/현재요청을빌린다.
    o::RegionOwnershipState& RegionVoteGlobals(); //같은영향목록/redraw/mapRevision을빌린다.
    o::RawIslandRuntime& Islands(); //현재3204BYTE/거주구역/Player를사용하는전체장기섬실행기다.
    o::RawRegionOwnership& Regions(); //같은실제두목록과현재TYPE/도장으로전체투표를한번조립한다.
    o::RawVortexOwner& VortexOwner(); //같은현재Player/섬/영역/생산장치의실제신전가상74다.
    o::RawVortexPreDestroy& VortexPre(); //현재두목록/섬/영역/실제Damageable/Player후처리를공유한다.
    o::RawVortexAscend& VortexAscend(); // 같은 실제 섬/난수/현재 noIsland 역할과 가상 삭제를 공유한다.
    o::VortexAscendState& VortexAscendGlobals(); // Session 생성/반납으로 초기화하지 않는 현재 역할 뷰다.
    std::int32_t IslandNumber(float x,float y); //원문4f530의편향/snap뒤현재hash0지역을조회한다.
    std::uint32_t PlaceRandomSingle(o::GameRandom& random,const o::FortRandomPlaceRequest& request); //원문의단일전체배치를같은장기Factory/HP/가상owner/Pop에연결한다.
    void PlaceBattleNuggets(o::FortRandomRectangle rectangle,std::uint32_t quantity); //원문의현재시각/지역seed와1000회반복단일배치를World없이조립한다.
    RawCampaignConstructionBindings& Body(std::shared_ptr<ClientSpGlobals> sp,std::shared_ptr<ClientPlayerRecords> players,std::shared_ptr<std::array<std::uint8_t,87>> options); //같은 장기 물리/현재 Player getter로 본체를 한 번 구성하며 World를 만들지 않는다.
    RawCampaignConstructionBindings& Body(); //이미 구성한 장기 본체를 빌리며 Session 반납 뒤에도 주소/상태를 보존한다.
private:
    o::RawCanonPlacementTerrain& RegionTerrain(); //실제현재타입/프레임/hash0를사용하는지역getter만구성한다.
    void PaintRegion(const o::RegionOwnershipPaint& request); //선택한도장마다현재terrain/edgeTYPE와같은난수/캐시를사용한다.
    void RefreshRegionProduction(); //현재장치집합/함수사본을호출끝까지보유하고실제생산사건을보낸다.
    void BindPlacementGlobals(); //최초장기결합에서원문초기값과현재전역주소만연결한다.
    std::shared_ptr<RawSurfaceStorage> storage_; // 상태의 TYPE/모드/Canon raw 참조보다 오래 유지한다.
    std::shared_ptr<ClientGameplayGlobals> gameplay_; // Player/카메라/미션 raw 참조의 실제 owner다.
    std::shared_ptr<ClientBuildReplayGlobals> replay_; // 선택적 재생 raw DWORD 참조의 수명을 보증한다.
    std::shared_ptr<ClientSpGlobals> sp_; //본체의 같은 실제 분산 SP 주소를 마지막 호출 뒤까지 보유한다.
    std::shared_ptr<ClientPlayerRecords> players_; //본체는 span을 고정하지 않고 이 owner의 현재base를 선택 위치에서 읽는다.
    std::shared_ptr<std::array<std::uint8_t,87>> options_; //같은87BYTE의 mapSize 주소를 본체보다 오래 유지한다.
    std::shared_ptr<ClientRandomGlobals> random_; //단일배치의전역프레임난수는지역좌표난수와구별하며재초기화하지않는다.
    std::shared_ptr<ClientKencloudGlobals> kencloud_; //섬/도장/투표가빌린같은실제redrawDWORD의owner다.
    std::weak_ptr<const ClientConstructionRuntimeBindings> runtime_; //큰 Session 효과를 반납 뒤 보유하지 않으며 새 실제 대여로 교체할 수 있다.
    std::shared_ptr<const ClientConstructionDevices> devices_; //선택된 호출은 자기 집합/함수 사본을 보유하고 다음 호출만 새 장치를 조회한다.
    RawCampaignConstructionState state_; //현재 값과 raw 참조는 본체보다 오래 유지하고 앞의 실제 backing보다 먼저 반납한다.
    o::CanonPlacementPipelineState placementState_; //원문미리보기/충돌/권한/거부값을커서와같은주소에서유지한다.
    o::PlayerPlacementAnchorState placementAnchor_; //현재기준점/Graph조회의실제장기상태다.
    o::SquidPostPopList placementAdditional_; //기본작업장목록과구별한원문추가목록이다.
    std::array<o::CanonPlacementIslandRegion,100> placementRegions_{}; //같은3204BYTE섬기록의현재제한/존재view다.
    bool placementBound_{}; //초기값을한번만설치하며Session재진입에서다시쓰지않는다.
    o::FortRandomPlacementState randomPlacementState_; //현재TYPE포인터와ownerOverride를장기로유지한다.
    o::FortFinishOwnerState finishOwnerState_; //섬/후처리가같은상태를빌리며임시성공/Player배열을만들지않는다.
    o::GemRewardState gemState_; //현재TYPE개수/오벨리스크TYPE를실제보상Caller도읽는다.
    o::SquidPostPopList constructionExcludedTypes_; //0052f7e0의현재BYTE항목목록이며추가SID목록과구별한다.
    o::RegionPaintState regionPaint_; //같은실제현재도장/변형캐시이며Session별사본을만들지않는다.
    o::RegionOwnershipState regionVotes_; //현재누적영향목록/공유두DWORD를같은주소에보관한다.
    o::CanonPlacementTerrainState regionGetterState_; //배치순회와별도인원문지역getter의sentinel상태다.
    std::unique_ptr<o::RawIslandRuntime> islands_; //backing/상태보다먼저반납하는실제장기섬실행기다.
    std::unique_ptr<o::RawCanonPlacementTerrain> regionTerrain_; //같은metadata/hash/현재타입보다먼저반납한다.
    std::unique_ptr<o::RawRegionOwnership> regions_; //현재두목록/도장상태의주소를고정한다.
    std::unique_ptr<o::RawVortexOwner> vortexOwner_; //같은Player/섬/투표를빌리며Session주소를보유하지않는다.
    o::VortexAscendState vortexAscendState_; // 현재 noIsland 역할을 장기 Storage에서 빌린다.
    std::vector<std::unique_ptr<std::uint8_t[]>> vortexCandidateBuffers_; // 각 중첩 Tick의 실제80BYTE 확보/반납을 별도로 보존한다.
    std::unique_ptr<o::RawVortexAscend> vortexAscend_; // 장기 섬/물리/난수/버퍼보다 먼저 반납한다.
    std::unique_ptr<o::RawVortexPreDestroy> vortexPre_; //동일실제Damageable/목록/현재Player를선택한다.
    std::unique_ptr<RawCampaignConstructionBindings> body_; //전체 건설/Bounds/최근 목록 본체를 Client 수명에 보유하고 State/backing보다 먼저 반납한다.
    std::unique_ptr<o::RawCanonPlacementPipeline> placementPipeline_; //같은상태/물리보다먼저반납하며World/Session을캡처하지않는다.
    std::unique_ptr<o::RawGemReward> gemReward_; //같은난수/제외목록/전역보다먼저반납하며Session주소를보유하지않는다.
};
RawSurfaceVortexDevices MakeClientVortexDevices(const std::shared_ptr<ClientConstructionGlobals>& owner); // 첫 메뉴/Session 전에 weak 장치만 만들고 실제 섬/후처리/게임은 실행하지 않는다.
}
