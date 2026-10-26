// 실제Place반환지역을그뒤모든하위에전달하며현재owner는마지막저장직전에읽는다.
#include "o/RawIslandAdd.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원문의최소월드변경부모를기존몸체에연결한다.
RawIslandAdd::RawIslandAdd(ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& piece,RawIslandBuilderState& state,const bool& fort,RawIslandPopulation& population,std::function<void(RawIslandBuilderAssertion)> assertion,IslandAddHooks hooks)
    :world_(world),islands_(islands),frames_(frames),piece_(piece),state_(state),fort_(fort),population_(population),assertion_(std::move(assertion)),hooks_(std::move(hooks)) {
    if (&islands.Pool()!=&population.Pool()) throw std::invalid_argument("실제섬추가의Pool불일치");
}
std::uint32_t RawIslandAdd::Add(std::span<const std::uint8_t,6> record,std::uint32_t territory) const {
    if (!fort_) { if (!hooks_.fortAssertion) throw std::logic_error("섬추가Fort진단미연결");hooks_.fortAssertion(); }
    RawIslandBuilder builder(world_,world_,islands_,frames_,piece_,state_,assertion_);const auto region=builder.Place(record,territory,false);
    if (!hooks_.generate) throw std::logic_error("섬추가실제지형생성미연결");hooks_.generate(state_,static_cast<std::int32_t>(region));population_.Populate(static_cast<std::int32_t>(region));
    if (!hooks_.graph) throw std::logic_error("섬추가실제Graph미연결");hooks_.graph(false);if (!hooks_.territory) throw std::logic_error("섬추가실제Territory저장미연결");hooks_.territory(state_.owner);return region;
} //원문void부모의관측편의용반환은실제Place가돌려준지역이며새로합성하지않는다.
bool RawIslandAdd::TryDrop(const ChunkMap& preview,std::span<std::uint8_t,6> record) const {
    if (!CanPlaceIsland(OriginalEdition::Patch1078,preview,frames_,piece_,state_,record,assertion_)) return false;
    record[1]&=0xfdU;state_.bounds=DefaultChunkRect();state_.owner=1; //0046da20은현재템플릿owner를보존하지않고실제기본bounds와owner1을쓴다.
    static_cast<void>(Add(record,islands_.Count()));return true;
} //world변경/지형/생성/Graph/저장중실패하면앞선실제효과를보존하고예외를전달한다.
}
