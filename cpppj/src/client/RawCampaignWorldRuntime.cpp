// 기존 완료 몸체를 재작성하지 않고 캠페인 최초 설치의 실제 하위 경로로 연결한다.
#include "client/RawCampaignWorldRuntime.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 현재 값/배열을 복사하지 않고 같은 실제 설치 자료의 주소만 보관한다.
struct RawCampaignWorldRuntime::Impl {
    RawSurfaceWorld& world; // 동일 Pool/공간/Graph와 수명 경계의 소유자다.
    RawWorldCore core; // 실제 월드가 소유한 처리기/배열의 참조다.
    o::ChunkMap& chunks; // Builder와 object 영역 선택이 공유하는 실제 지도다.
    o::RawIslandRuntime& islands; // 실제3204BYTE 섬 기록과 Owner/Matches를 공유한다.
    o::RawPlayerWorld& player; // 실제9 Player 기록과 현재 두owner를 읽는다.
    RawNormalTerrainWorldBinding& normal; // 같은 두 난수/지도의 정상 생성 몸체다.
    RawCampaignOwnership& ownership; // 실제 신전/사제 분배 및 Player 첫 지역 쓰기를 담당한다.
    o::FortObjectLoadState& objectState; // 실제 중첩 깊이와 저장 객체 모드를 공유한다.
    RawCampaignInstallData data; // 원본 현재9 Template/stream/header/handle의 span이다.
    RawCampaignInstallEffects effects; // Core 밖의 필수 가상/UI/진단 경계다.
    std::unique_ptr<o::RawFortObjectLoad> objects; // 실제 저장172객체의 로더이며 결과는 같은 Pool에 쓴다.
    std::unique_ptr<o::RawFortWorldLoad> territory; // 실제120BYTE 읽기 몸체를 재사용한다.
    std::unique_ptr<o::RawCampaignWorldLoad> campaign; // 현재 순서/두owner를 보존하는 원본 미션 부모다.
    bool started{}; // 실패한 부분 설치의 재실행으로 중복 객체가 생기지 않게 한다.

    // 실제 저장소 동일성과 필수 경계를 최초 등록/쓰기 전에 검사한다.
    Impl(RawSurfaceWorld& source,o::ChunkMap& map,o::RawIslandRuntime& list,o::RawPlayerWorld& players,
        RawNormalTerrainWorldBinding& terrain,o::GameRandom& random,const o::SquidReward& hp,RawCampaignOwnership& owners,
        o::RawFortFinishOwner& finish,const o::RawContainedCreate& contained,o::FortObjectLoadState& state,
        o::CampaignWorldGlobals& globals,std::uint32_t& puzzlePiece,std::int32_t& noRegion,
        RawCampaignInstallData storage,RawCampaignInstallEffects hooks)
        :world(source),core(source.Core()),chunks(map),islands(list),player(players),normal(terrain),ownership(owners),
        objectState(state),data(storage),effects(std::move(hooks)) {
        if (&islands.Pool()!=&core.pool || &player.Pool()!=&core.pool || &normal.Pool()!=&core.pool ||
            &hp.Pool()!=&core.pool || &hp.Bookkeeping()!=&core.bookkeeping || &ownership.Vortex().Pool()!=&core.pool || &finish.Pool()!=&core.pool ||
            &contained.Pool()!=&core.pool || ownership.Vortex().Records().data()!=player.Records().data() ||
            (data.templates.size()!=9 && !data.templateAt) || (data.territories.size()!=9 && !data.territoryAt) || data.headers.size()!=180 || data.handle.empty() ||
            !effects.translateYou || !effects.setConfig || !effects.refreshWindowTitle || !effects.resetCamera ||
            !effects.builderAssertion || !effects.territoryAssertion || !effects.notifySurface || !effects.setState || !effects.virtualCount)
            throw std::invalid_argument("캠페인 설치의 동일 Core/Player/9 Template/실제 경계 누락");
        finish.BindPlayerRecords(player.Records());
        o::FortObjectLoadHooks objectHooks;
        objectHooks.notifySurface=effects.notifySurface;objectHooks.setState=effects.setState;objectHooks.virtualCount=effects.virtualCount;
        objectHooks.setOwner=[this](o::Sid sid,std::uint32_t owner) { ownership.SetOwner(sid,owner); };
        objectHooks.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        objectHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        objectHooks.chunkRegion=[this](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(chunks.At(x,y).island); };
        objectHooks.territoryAllowed=[this](std::int8_t region,std::uint32_t owner,std::int32_t section) { return islands.Matches(region,std::bit_cast<std::int32_t>(owner),section); };
        objectHooks.loadingDepthChanged=[this](std::uint32_t depth) { world.SetLoadingDepth(depth); };
        objectHooks=o::MakeFortFinishOwnerHooks(core.pool,finish,std::move(objectHooks));
        objectHooks=o::MakeFortObjectLoadHooks(core.pool,core.factory,hp,contained,std::move(objectHooks));
        objects=std::make_unique<o::RawFortObjectLoad>(core.pool,core.hash,core.types,core.frames,random,objectState,std::move(objectHooks));
        o::FortWorldLoadHooks territoryHooks;
        territoryHooks.templatePresent=[this](std::uint32_t slot) { CheckSlot(slot);const auto record=data.headers.subspan(slot*20,4);return record[0] || record[1] || record[2] || record[3]; };
        territoryHooks.territory=[this](std::uint32_t slot)->o::FortTerritoryStream& { CheckSlot(slot);auto& stream=data.territoryAt ? data.territoryAt(slot) : data.territories[slot];stream.position=0;return stream; };
        territoryHooks.currentTemplate=[this](std::uint32_t slot)->const o::FortTemplate& { return Template(slot); };
        territoryHooks.assertion=effects.territoryAssertion;territoryHooks.builderAssertion=effects.builderAssertion;
        territoryHooks.loadingDepthChanged=[this](std::uint32_t depth) { world.SetLoadingDepth(depth); };
        territoryHooks.generate=[this](const o::RawIslandBuilderState& builder,std::int32_t region) { Generate(builder,region); };
        territoryHooks.objects=[this](const o::FortTemplate& input,std::uint32_t slot) { objects->Load(input,static_cast<std::int32_t>(slot)); };
        // 캠페인 부모가 사용하지 않는 generic Fort 분기는 실제 미복원 경계로 남긴다.
        territoryHooks.money=territoryHooks.badges=territoryHooks.technology=[](const o::FortTemplate&,std::uint32_t) { throw std::logic_error("캠페인 설치에서 generic Fort 부모 후처리를 실행할 수 없습니다"); };
        territoryHooks.populateIsland=[](const o::RawIslandBuilderState&,std::int32_t) { throw std::logic_error("캠페인 설치의 alternate Fort populate 미연결"); };
        territoryHooks.rebuildGraph=[this](bool mode) { core.graph.Rebuild(mode); };
        territory=std::make_unique<o::RawFortWorldLoad>(core.pool,chunks,islands,core.frames,puzzlePiece,player.State().templateOwner,noRegion,objectState.loadingDepth,std::move(territoryHooks));
        o::CampaignWorldLoadHooks campaignHooks{effects.translateYou,effects.setConfig,effects.refreshWindowTitle,effects.builderAssertion,
            [this](const o::RawIslandBuilderState& builder,std::int32_t region) { Generate(builder,region); },
            [this](std::uint32_t slot)->const o::FortTemplate& { return Template(slot); },
            [this](const o::FortTemplate& input,std::uint32_t slot) { objects->Load(input,static_cast<std::int32_t>(slot)); },
            [this](bool mode) { core.graph.Rebuild(mode); },effects.resetCamera};
        campaign=std::make_unique<o::RawCampaignWorldLoad>(chunks,islands,core.frames,puzzlePiece,noRegion,*territory,player,globals,data.headers,data.handle,std::move(campaignHooks));
    }
    // 현재 Template 번호를 매 하위 호출 직전에 실제9칸 범위로 검사한다.
    void CheckSlot(std::uint32_t slot) const { if (slot>=9) throw std::out_of_range("캠페인 현재 Template의9칸 범위 오류"); }
    // 현재 Template 선택은 부모의 callback 뒤 변경된 owner도 반영한다.
    const o::FortTemplate& Template(std::uint32_t slot) const { CheckSlot(slot);return data.templateAt ? data.templateAt(slot) : data.templates[slot]; }
    // 원본 Builder의 실제 owner/bounds를 같은 일반 지형 소비자에 전달한다.
    void Generate(const o::RawIslandBuilderState& builder,std::int32_t region) { normal.Generate({builder.owner,builder.bounds,region,NormalTerrainBranch::Normal}); }
};
// 공급한 실제 모듈의 참조를 고정 주소 구현에 구성한다.
RawCampaignWorldRuntime::RawCampaignWorldRuntime(RawSurfaceWorld& world,o::ChunkMap& chunks,o::RawIslandRuntime& islands,
    o::RawPlayerWorld& player,RawNormalTerrainWorldBinding& normal,o::GameRandom& random,const o::SquidReward& hp,
    RawCampaignOwnership& ownership,o::RawFortFinishOwner& finish,const o::RawContainedCreate& contained,
    o::FortObjectLoadState& objects,o::CampaignWorldGlobals& globals,std::uint32_t& puzzlePiece,
    std::int32_t& noRegion,RawCampaignInstallData data,RawCampaignInstallEffects effects)
    :impl_(std::make_unique<Impl>(world,chunks,islands,player,normal,random,hp,ownership,finish,contained,objects,globals,puzzlePiece,noRegion,data,std::move(effects))) {}
// 이 연결 자체는 외부 Kernel/Player/PostPop 등록의 수명을 소유하지 않는다.
RawCampaignWorldRuntime::~RawCampaignWorldRuntime()=default;
// 부모의 실제 Graph(false) 한 번 이후에만 로딩 완료를 표시한다.
void RawCampaignWorldRuntime::Load() {
    auto& state=*impl_;if (state.started) throw std::logic_error("캠페인 동일 월드의 중복/실패 후 설치 재실행");
    state.started=true;state.world.BeginLoad();state.objectState.loadingDepth=1;state.campaign->Load();
    state.world.FinishLoadAfterGraph();state.objectState.loadingDepth=0;
}
// 참조를 비교할 실제 Pool을 반환한다.
const o::SidPool& RawCampaignWorldRuntime::Pool() const { return impl_->core.pool; }
// 실제 저장객체 loader를 제공하며 별도 사본을 만들지 않는다.
const o::RawFortObjectLoad& RawCampaignWorldRuntime::Objects() const { return *impl_->objects; }
}
