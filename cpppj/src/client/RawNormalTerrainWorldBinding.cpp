// 양판본의 일반 wrapper와 섬별 후처리 호출 순서를 실제 완료 모듈로 조립한다.
#include "client/RawNormalTerrainWorldBinding.h"
#include "o/CanonDecoder.h"
#include "o/SquidOwner.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 기존 모듈이 검사하는 같은 Pool뿐 아니라 HP 장부도 같은 실제 월드인지 확인한다.
o::FortRandomPlacementHooks MakeNormalTerrainFarmWorldHooks(RawSurfaceWorld& world,const o::ChunkMap& chunks,
    const o::RawIslandRuntime& islands,o::RawCanonPlacementPipeline& placement,const o::SquidReward& hp) {
    const auto core=world.Core();
    if (&core.pool!=&islands.Pool() || &core.bookkeeping!=&hp.Bookkeeping())
        throw std::invalid_argument("일반 지형 Farm의 실제 섬/HP 장부 불일치");
    auto hooks=o::MakeFortRandomPlacementHooks(core.pool,placement,core.factory,hp,{});
    hooks.chunkRegion=[&chunks](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(chunks.At(x,y).island); };
    hooks.regionOwner=[&islands](std::int32_t region) { return islands.Owner(region); };
    hooks.setOwner=o::MakeOwnerDispatch(core.pool,core.bridgeState,[&owner=core.owner](o::Sid sid,std::uint32_t player) { owner.Set(sid,player); });
    hooks.unpop=[&world](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
    hooks.repop=[&world](o::Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    hooks.pop=[&world](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    return hooks;
}
// 완료된 단일 배치/군집과 실제 Damageable 후처리를 같은 월드에 명시 연결한다.
RawNormalTerrainWorldBinding::RawNormalTerrainWorldBinding(RawSurfaceWorld& world,const o::ChunkMap& chunks,
    const o::RawIslandRuntime& islands,o::GameRandom& random,std::uint32_t& crt,o::TerrainContextGlobals& contextGlobals,
    o::TerrainStampState& stampState,o::FortRandomPlacementState& placementState,o::FarmClusterState& farmState,
    std::uint32_t& puzzlePieceType,o::RawCanonPlacementPipeline& placement,const o::SquidReward& hp,
    const o::RawCarrierPostPop& carrier,NormalTerrainDiagnostics diagnostics)
    :world_(world),core_(world.Core()),chunks_(chunks),islands_(islands),random_(random),crt_(crt),contextGlobals_(contextGlobals),
    stampState_(stampState),placementState_(placementState),farmState_(farmState),puzzlePieceType_(puzzlePieceType),diagnostics_(std::move(diagnostics)) {
    if (&core_.pool!=&carrier.Pool() || !diagnostics_.context || !diagnostics_.stamp || !diagnostics_.region ||
        !diagnostics_.farm || !diagnostics_.postpass) throw std::invalid_argument("일반 지형 실제 후처리/필수 진단 누락");
    RequireStorage();
    const auto hooks=MakeNormalTerrainFarmWorldHooks(world_,chunks_,islands_,placement,hp);
    single_=std::make_unique<o::RawFortRandomPlacement>(core_.pool,core_.hash,core_.spots,core_.types,core_.frames,random_,placementState_,hooks);
    farms_=std::make_unique<o::RawFarmCluster>(core_.pool,chunks_,islands_,placementState_,farmState_,
        o::MakeFarmClusterPlacementHook(core_.pool,*single_,{{},diagnostics_.farm}));
    core_.postPop.SetCarrierPostPop(&carrier);
}
// 군도와 서로 다른 원본 전역을 묵시적인 정상 지형/지역 기본값으로 바꾸지 않는다.
void RawNormalTerrainWorldBinding::RequireNormal() const {
    if (farmState_.Archipelago()) throw std::logic_error("일반 전용 지형 요청에 군도 상태를 사용할 수 없습니다");
    RequireStorage();
}
// 현재 원본 모드의 선택과 분리하여 같은 실제 TYPE/지역/생성자 연결을 확인한다.
void RawNormalTerrainWorldBinding::RequireStorage() const {
    if (contextGlobals_.islandType!=placementState_.islandType || contextGlobals_.islandType!=stampState_.islandType ||
        contextGlobals_.edgeType!=stampState_.edgeType || contextGlobals_.noRegion!=stampState_.noRegion)
        throw std::invalid_argument("일반 지형 같은 타입/지역 전역 연결 오류");
    // 지면97의 실제0 슬롯/base 계약은 보존하며121의 실제 생성자를 임의0 슬롯으로 대신하지 않는다.
    for (const auto type:{contextGlobals_.islandType,placementState_.edgeFarmType})
        if (type>=core_.types.size() ||
            core_.types[type].constructorAddress!=o::TypeConstructorAddress(core_.pool.Edition(),type))
            throw std::invalid_argument("일반 지형 실제 타입 생성자 연결 누락");
}
// 원본 일반 context의 수명은 초기 도장과 정규화까지이며 군집 postpass 전에 끝난다.
o::Sid RawNormalTerrainWorldBinding::Generate(const NormalTerrainRequest& request) {
    if (request.branch!=NormalTerrainBranch::Normal) throw std::logic_error("군도/전체 부모를 일반 지형으로 실행할 수 없습니다");
    RequireNormal();
    if (request.bounds.left<1 || request.bounds.top<1 || request.bounds.right>15 || request.bounds.bottom>15 ||
        request.bounds.left>=request.bounds.right || request.bounds.top>=request.bounds.bottom)
        throw std::out_of_range("일반 지형 소비자의 유효 내부 청크 범위가 필요합니다");
    o::Sid last;
    {
        auto context=std::make_unique<o::RawTerrainContext>(core_.pool.Edition(),request.owner,request.selectedRegion,request.bounds,
            contextGlobals_,diagnostics_.context);
        o::RawTerrainGrowth growth(*context,chunks_,contextGlobals_);growth.RunNormal();
        last=StampContext(*context);
    }
    PostprocessFarms(request.selectedRegion);return last;
}
// context의 수명을 넘겨 보관하지 않고 원문 세 후처리를 같은 실제 World에 실행한다.
o::Sid RawNormalTerrainWorldBinding::StampContext(o::RawTerrainContext& context) {
    context.MarkEdges('x','=');o::TerrainStampHooks effects;
    effects.chunkRegion=[this](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(chunks_.At(x,y).island); };
    effects.assertion=diagnostics_.stamp;
    o::RawTerrainStamp stamp(core_.pool,core_.types,core_.frames,context.Map(),random_,crt_,stampState_,
        MakeTerrainStampWorldHooks(world_,stampState_,diagnostics_.region,std::move(effects)));
    stamp.Stamp('x');context.NormalizeAfterStamp();return context.Map().lastSid;
}
// Farm이 빌린 현재 두 전역과 root가 쓰는 owner가 다르면 연결 시점에 거부한다.
void RawNormalTerrainWorldBinding::BindGenerationInputs(TerrainGenerationInputs inputs) {
    if ((farmState_.sharedArchipelago && farmState_.sharedArchipelago!=&inputs.archipelago) ||
        (farmState_.sharedArchipelagoSpan && farmState_.sharedArchipelagoSpan!=&inputs.span))
        throw std::invalid_argument("지형 root와 군집의 같은 군도/확장 owner 불일치");
    farmState_.sharedArchipelago=&inputs.archipelago;farmState_.sharedArchipelagoSpan=&inputs.span;
    generation_=std::make_unique<TerrainGenerationInputs>(std::move(inputs));
}
// 후보는 constructor보다 먼저 캐시하고 PATCH layout/두 spacing은 constructor 뒤 현재값을 읽는다.
o::Sid RawNormalTerrainWorldBinding::GenerateOriginal(std::uint32_t owner,o::ChunkRect bounds,std::int32_t selectedRegion) {
    if (!generation_) throw std::logic_error("실제 지형 생성 상위의 현재 전역/장치 미연결");
    auto& input=*generation_;
    const bool candidate=input.enabled && input.battle && !input.mission && islands_.Count()==1;
    RequireStorage();o::Sid last;
    {
        auto context=std::make_unique<o::RawTerrainContext>(core_.pool.Edition(),owner,selectedRegion,bounds,
            contextGlobals_,diagnostics_.context);
        o::RawTerrainGrowth growth(*context,chunks_,contextGlobals_,input.growth);
        const bool cd=core_.pool.Edition()==o::OriginalEdition::Cd1072;
        if (!cd && !input.layout) throw std::logic_error("지형 root의 현재 layout 미연결");
        const bool archipelago=cd ? candidate : input.layout()==0 && candidate;
        if (archipelago) {
            const auto signedWord=[](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }; // 원문 signed 산술 앞의 DWORD 비트 해석이다.
            const auto bits=[](std::int32_t value) { return std::bit_cast<std::uint32_t>(value); }; // 덧셈/곱셈은32비트 감김으로 계산한다.
            std::int32_t extension;
            if (cd) extension=input.span; // CD는 기존 span을 쓰고 mapSize를 새로 조회하지 않는다.
            else {
                if (!input.spacing) throw std::logic_error("지형 root의 두 현재 spacing 조회 미연결");
                extension=signedWord(bits(input.spacing())-2U);
                const auto spacing=input.spacing();input.span=spacing==3 ? 1 : spacing==4 ? 2 : 3;
            }
            context->ArchipelagoMode()=1;
            const auto before=context->Chunks();
            context->SetBounds({signedWord(bits(before.left)-bits(extension)+1U),signedWord(bits(before.top)-bits(extension)+1U),
                signedWord(bits(before.right)+bits(extension)),signedWord(bits(before.bottom)+bits(extension))});
            const auto scatterWidth=bits(input.span)*2U+2U;
            growth.Scatter(signedWord(scatterWidth*scatterWidth*0x55U)/0x19);
            auto seed=growth.FirstChunkSeed()*0x1bbU;
            const auto chunks=context->Chunks();
            const auto area=(bits(chunks.bottom)-bits(chunks.top))*(bits(chunks.right)-bits(chunks.left))*0x100U;
            std::uint32_t target;
            if (input.percent==0) {
                const auto high=signedWord(bits(input.maximum)*area)/200,low=signedWord(bits(input.minimum)*area)/200;
                const auto first=growth.NextRange(seed,low,high),second=growth.NextRange(seed,low,high);
                target=bits(first)+bits(second);
            } else target=bits(signedWord(input.percent*area)/100);
            if (!input.logPercent) throw std::logic_error("지형 root의 실제 백분율 로그 장치 미연결");
            if (!area) throw std::domain_error("지형 root의 원문 백분율0 나눗셈");
            input.logPercent(signedWord(target*100U)/signedWord(area));
            growth.GrowRectangle(0,context->Chunks(),signedWord(target),seed);
            if (!input.subscriber) throw std::logic_error("지형 root의 현재 Player 장치 미연결");
            for (std::int32_t player=1;player<=8;++player) { // 각 반복에서 현재 Player base의subscriber를 다시 읽는다.
                if (!input.subscriber(player)) continue;
                if (!input.placement) throw std::logic_error("지형 root의 선택된 실제 배치 장치 미연결");
                const std::array<std::uint8_t,6> record{7,1,0,0,0,0}; // 원문0x107/WORD0의 정확한6BYTE이며 추가DWORD 상위16은 포함하지 않는다.
                const auto placement=input.placement(player,record);growth.ClearPlacement(placement);
            }
            growth.FixGaps(1);growth.RemoveSparse();
        } else growth.RunNormal();
        last=StampContext(*context);
        if (archipelago) input.archipelago=1;
    }
    PostprocessCurrentFarms(selectedRegion);return last;
}
// 기존 첫 청크 getter와 같은 부호 있는 지역 비교/순회 순서를 사용한다.
std::array<std::uint8_t,6> RawNormalTerrainWorldBinding::FirstRecord(std::int32_t region) const {
    const bool cd=core_.pool.Edition()==o::OriginalEdition::Cd1072;
    if (cd && !islands_.Exists(region)) diagnostics_.postpass(TerrainFarmPostpassAssertion::RecordMissingIsland);
    // Y청크가 바깥이며 각 행의 X청크를 처음부터 찾는다.
    for (std::int32_t y=0;y<chunks_.YLength();++y)
        // 원본은 처음 일치한 record6BYTE만 반환한다.
        for (std::int32_t x=0;x<chunks_.XLength();++x) {
            const auto& entry=chunks_.At(x,y);
            if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(entry.island))==region) return entry.record;
        }
    if (cd) diagnostics_.postpass(TerrainFarmPostpassAssertion::RecordNotFound);
    return {};
}
// 전체 패턴의 빈/누락 프레임을 기존 decoder가 실제로 건너뛴 뒤 유효한 칸만 센다.
std::uint32_t RawNormalTerrainWorldBinding::PatternCount(std::span<const std::uint8_t,6> record) const {
    if (puzzlePieceType_>=core_.frames.size()) throw std::out_of_range("일반 지형 현재 puzzlePiece 타입 범위 오류");
    const auto patterns=o::TerritoryPatterns();const auto pattern=static_cast<std::size_t>(record[0]&0x3fU);
    if (pattern>=patterns.size()) throw std::out_of_range("일반 지형 패턴 번호 범위 오류");
    o::CanonDecoder decoder(core_.frames[puzzlePieceType_],patterns[pattern],static_cast<std::int32_t>(record[0]>>6)*2,0,0,{},core_.pool.Edition(),diagnostics_.decoderDiagnostics);
    std::uint32_t count=0;
    // 원본 constructor의 최초 유효 칸을 포함하고 Advance가 끝날 때까지 센다.
    for (;decoder.Valid();decoder.Advance()) ++count;
    return count;
}
// 현재 섬 수와 현재 noRegion을 매 반복 읽으며 삭제된 항목을 별도 필터로 제거하지 않는다.
void RawNormalTerrainWorldBinding::PostprocessFarms(std::int32_t selectedRegion) {
    RequireNormal();
    PostprocessCurrentFarms(selectedRegion);
}
// root의 도장/context 해제 뒤 현재 군도 전역을 읽는 같은 군집 몸체를 실행한다.
void RawNormalTerrainWorldBinding::PostprocessCurrentFarms(std::int32_t selectedRegion) {
    RequireStorage();
    // 원본 상위처럼 count는 unsigned저장값을 signed DWORD로 해석하며 회마다 다시 읽는다.
    for (std::int32_t region=0;region<std::bit_cast<std::int32_t>(islands_.Count());++region) {
        if (selectedRegion!=contextGlobals_.noRegion && selectedRegion!=region) continue;
        const auto record=FirstRecord(region);
        if (!islands_.Exists(region)) diagnostics_.postpass(TerrainFarmPostpassAssertion::BoundsMissingIsland);
        const auto bounds=chunks_.Bounds(region);
        if (core_.pool.Edition()==o::OriginalEdition::Cd1072 && bounds.left==16 && bounds.top==16)
            diagnostics_.postpass(TerrainFarmPostpassAssertion::BoundsNotFound);
        const o::FortRandomRectangle cells{static_cast<float>(bounds.left*16),static_cast<float>(bounds.top*16),
            static_cast<float>(bounds.right*16),static_cast<float>(bounds.bottom*16)};
        farms_->Place(cells,region,PatternCount(record));
    }
}
// 모듈 내부의 복사/모형 풀이 아니라 실제 월드의 현재 풀이다.
const o::SidPool& RawNormalTerrainWorldBinding::Pool() const { return core_.pool; }
}
