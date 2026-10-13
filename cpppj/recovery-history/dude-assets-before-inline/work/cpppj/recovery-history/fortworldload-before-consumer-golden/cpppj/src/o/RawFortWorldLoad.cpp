// 부모순서를변경하지않고각child가끝난뒤현재owner/Exists/원본깊이를다시읽는다.
#include "o/RawFortWorldLoad.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 동일World/Runtime/원본전역과필수child경계를부모효과전에확인한다.
RawFortWorldLoad::RawFortWorldLoad(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,
    std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePiece,std::uint32_t& localOwner,std::int32_t& noRegion,
    std::uint32_t& loadingDepth,FortWorldLoadHooks hooks)
    :pool_(pool),world_(world),islands_(islands),frames_(frames),puzzlePiece_(puzzlePiece),localOwner_(localOwner),noRegion_(noRegion),loadingDepth_(loadingDepth),hooks_(std::move(hooks)) {
    if (&islands.Pool()!=&pool || frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        !hooks_.templatePresent || !hooks_.territory || !hooks_.currentTemplate || !hooks_.assertion || !hooks_.loadingDepthChanged ||
        !hooks_.builderAssertion || !hooks_.generate || !hooks_.objects || !hooks_.money || !hooks_.badges || !hooks_.technology || !hooks_.populateIsland || !hooks_.rebuildGraph)
        throw std::invalid_argument("Fort World 실제자료/필수child/진단연결누락");
}
// 현재pointer의120BYTE를직접읽으며CD는진단/lookup앞에서깊이를즉시복구한다.
void RawFortWorldLoad::ReadTerritory(std::uint32_t templateSlot,std::span<std::uint8_t,120> output) const {
    if (pool_.Edition()==OriginalEdition::Cd1072) {
        ++loadingDepth_;hooks_.loadingDepthChanged(loadingDepth_);--loadingDepth_;hooks_.loadingDepthChanged(loadingDepth_);
    }
    if (!hooks_.templatePresent(templateSlot)) hooks_.assertion(FortWorldLoadAssertion::MissingTerritoryTemplate);
    auto& stream=hooks_.territory(templateSlot);const auto position=stream.position;
    if (position>stream.bytes.size() || stream.bytes.size()-position<output.size())
        throw std::out_of_range("Fort Territory 현재물리120BYTE누락");
    // 실제DWORD30개복사와동일한120BYTE만읽으며사용하지않는꼬리는보존한다.
    for (std::size_t index=0;index<output.size();++index) output[index]=stream.bytes[position+index];
    stream.position=position+120U;
    if (std::bit_cast<std::int32_t>(stream.end)<std::bit_cast<std::int32_t>(stream.position)) hooks_.assertion(FortWorldLoadAssertion::TerritoryPastEnd);
}
// 최초builder owner1/기본bounds와실제world주소를붙인뒤모든child를원본순서로소비한다.
void RawFortWorldLoad::Load(std::int32_t alternate) const {
    RawIslandBuilderState state;RawIslandBuilder builder(world_,world_,islands_,frames_,puzzlePiece_,state,hooks_.builderAssertion);
    std::array<std::uint8_t,120> records{};ReadTerritory(localOwner_,records);builder.PlaceAll(records,0,20);hooks_.generate(state,noRegion_);
    if (!alternate) {
        auto owner=localOwner_;hooks_.objects(hooks_.currentTemplate(owner),owner);
        owner=localOwner_;hooks_.money(hooks_.currentTemplate(owner),owner);
        owner=localOwner_;hooks_.badges(hooks_.currentTemplate(owner),owner);
        owner=localOwner_;hooks_.technology(hooks_.currentTemplate(owner),owner);
    } else if (islands_.Exists(0)) hooks_.populateIsland(state,0);
    hooks_.rebuildGraph(false);
}
// 현재모듈의풀참조만공개하며새Pool사본을만들지않는다.
const SidPool& RawFortWorldLoad::Pool() const { return pool_; }
// 같은풀의완료child를재사용하고기존Money/변환/전체populate경계는그대로보존한다.
FortWorldLoadHooks MakeFortWorldChildrenHooks(const SidPool& pool,const RawFortObjectLoad& objects,const RawFortMetaLoad& meta,
    std::function<void(const FortTemplate&,std::uint32_t,bool)> selectMeta,std::function<FortMetaStream&(const FortTemplate&,bool)> metaStream,FortWorldLoadHooks hooks) {
    if (&objects.Pool()!=&pool || &meta.Pool()!=&pool || !selectMeta || !metaStream) throw std::invalid_argument("Fort World child 실제풀/현재전역/섹션조회불일치");
    hooks.objects=[&objects](const FortTemplate& fort,std::uint32_t owner) { objects.Load(fort,std::bit_cast<std::int32_t>(owner),0,20); };
    hooks.badges=[&meta,selectMeta,metaStream](const FortTemplate& fort,std::uint32_t owner) { selectMeta(fort,owner,false);meta.LoadBadges(metaStream(fort,false)); };
    hooks.technology=[&meta,selectMeta,metaStream](const FortTemplate& fort,std::uint32_t owner) { selectMeta(fort,owner,true);meta.LoadTechnology(metaStream(fort,true)); };
    return hooks;
}
}
