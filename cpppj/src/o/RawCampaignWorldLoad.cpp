// 실제 캠페인 부모는 각 하위 호출 뒤 현재 Template/battle owner와handle을 다시 읽는다.
#include "o/RawCampaignWorldLoad.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 같은 완료 자료와 명시 필수 하위 경계를 실제 최초쓰기 전에 확인한다.
RawCampaignWorldLoad::RawCampaignWorldLoad(ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,
    std::uint32_t& puzzlePiece,std::int32_t& noRegion,const RawFortWorldLoad& territory,RawPlayerWorld& player,
    CampaignWorldGlobals& globals,std::span<std::uint8_t> templateHeaders,std::span<std::uint8_t> currentHandle,CampaignWorldLoadHooks hooks)
    :world_(world),islands_(islands),frames_(frames),puzzlePiece_(puzzlePiece),noRegion_(noRegion),territory_(territory),player_(player),globals_(globals),templates_(templateHeaders),handle_(currentHandle),hooks_(std::move(hooks)) {
    if (&islands.Pool()!=&player.Pool() || &territory.Pool()!=&player.Pool() || frames.size()!=(player.Pool().Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        templates_.size()!=180 || handle_.empty() || !hooks_.translateYou || !hooks_.setConfig || !hooks_.refreshWindowTitle || !hooks_.builderAssertion || !hooks_.generate || !hooks_.currentTemplate || !hooks_.objects || !hooks_.rebuildGraph || !hooks_.resetCamera)
        throw std::invalid_argument("실제 캠페인 부모의 동일 Pool/현재 저장소/필수 하위 경계 누락");
}
// 원본 두strcpy 형태는 같은 NUL BYTE까지 쓰며 나머지 stale 메모리를 보존한다.
void RawCampaignWorldLoad::CopyName(std::span<std::uint8_t> target,std::span<const std::uint8_t> source) {
    const auto end=std::find(source.begin(),source.end(),std::uint8_t{}); // 원본 좁은문자열의 첫 NUL까지만 읽는다.
    if (end==source.end()) throw std::out_of_range("캠페인 실제 NUL 문자열 물리바이트 누락");
    const auto count=static_cast<std::size_t>(end-source.begin())+1; // 원본이 마지막에 저장하는 NUL을 포함한다.
    if (count>target.size()) throw std::out_of_range("캠페인 실제 이름 대상 물리바이트 범위 초과");std::copy_n(source.begin(),count,target.begin());
}
// 현재 두owner를 합치지 않고 실제battleOwner 기록만 선택한다.
std::span<std::uint8_t> RawCampaignWorldLoad::CurrentPlayer() const {
    const auto owner=player_.State().battleOwner; // 부모의 각 현재 Player 접근 직전에 다시 읽는다.
    if (owner>=9) throw std::out_of_range("캠페인 현재battleOwner의 실제9기록 범위 오류");
    const auto stride=Pool().Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU;return player_.Records().subspan(owner*stride,stride);
}
// 실제 selected mission1에서는923e0/40daf0의 두조건이 모두 false이므로 자연 반환한다.
void RawCampaignWorldLoad::FinishMissionPriests() const {
    if (!globals_.missionPost) throw std::logic_error("캠페인 비미션 Technology/Priest 후처리 전체 미복원");
}
// 본문순서를 보존하고 기존 하위 body를 다시 구현하지 않는다.
void RawCampaignWorldLoad::Load() const {
    globals_.mySubscriber=globals_.loginSubscriber;const auto owner=player_.State().templateOwner;
    if (owner>=9) throw std::out_of_range("캠페인 현재Template owner의 실제9기록 범위 오류");
    auto header=templates_.subspan(owner*20+8,4); // 최초현재Template의+8 DWORD만1로 저장한다.
    header[0]=1;header[1]=header[2]=header[3]=0;CopyName(handle_,hooks_.translateYou());
    const auto handleEnd=std::find(handle_.begin(),handle_.end(),std::uint8_t{}); // config에전달하는현재NUL포함handle이다.
    hooks_.setConfig("fort.mySubHandle",handle_.first(static_cast<std::size_t>(handleEnd-handle_.begin())+1));hooks_.refreshWindowTitle();
    player_.Create(std::bit_cast<std::int32_t>(player_.State().battleOwner));
    RawIslandBuilderState state;RawIslandBuilder builder(world_,world_,islands_,frames_,puzzlePiece_,state,hooks_.builderAssertion);
    if (globals_.editor) { auto raw=CurrentPlayer();const auto field=Pool().Edition()==OriginalEdition::Patch1078 ? 0x78U : 0x70U;std::fill_n(raw.begin()+field,4,std::uint8_t{}); }
    std::array<std::uint8_t,120> records{};territory_.ReadTerritory(player_.State().templateOwner,records);builder.PlaceAll(records,0,20);hooks_.generate(state,noRegion_);
    const auto current=player_.State().templateOwner;hooks_.objects(hooks_.currentTemplate(current),current);hooks_.rebuildGraph(false);FinishMissionPriests();hooks_.resetCamera();
    CopyName(CurrentPlayer().subspan(0xc),handle_);
}
// 실제 Player와 같은 pool 참조를 공개하며 풀/Player 사본을 만들지 않는다.
const SidPool& RawCampaignWorldLoad::Pool() const { return player_.Pool(); }
}
