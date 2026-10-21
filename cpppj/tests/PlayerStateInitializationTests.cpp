// 원본 빈 State의 보존 바이트/조건과 실제 섹션 재생성·현재 목록/버퍼 공유를 검사한다.
#include "o/RawPlayerTemplateState.h"
#include "client/CampaignTemplateStorage.h"
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원문의16진 바이트를 읽고 기대 결과를 계산하지 않는다.
std::vector<std::uint8_t> InitialStateBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 한 바이트씩 원문 순서대로 저장한다.
    for (std::size_t offset=0;offset<text.size();offset+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(offset,2)),nullptr,16)));
    return result;
}
// 실제 State 섹션의 파일 번호만 찾는다. 초기화 결과는 이 함수에서 만들지 않는다.
std::size_t InitialStateSection() { const auto names=FortTemplate::SectionNames();return static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin()); }
}
// 부모4bd550의 생성 조건·보존 스택/flags·28BYTE 쓰기와 기존 State 자연 반환을2560행에 대조한다.
TEST_CASE(PlayerStateInitialization_Native2560) {
    const auto rows=LoadFixture(NETSTORM_PLAYER_STATE_INITIALIZATION_FIXTURE).rows;CHECK(rows.size()==2560);
    // 각 행의 스택 미정 바이트는 원본 보장 초기값이 아닌 독립 입력이다.
    for (const auto& row:rows) {
        CHECK(row.size()==6);if (row.size()!=6) return;const auto input=Split(row[0],',');CHECK(input.size()==4);if (input.size()!=4) return;
        const auto seed=Number(input[0]),present=Number(input[2]),end=Number(input[3]);auto stack=InitialStateBytes(row[1]);const auto expectedStack=InitialStateBytes(row[2]),expectedNode=InitialStateBytes(row[3]);
        std::array<std::uint8_t,64> buffer{};
        // 같은 원문 버퍼의 생성 경계 입력을 준비하며 첫28BYTE 밖의 보존도 대조한다.
        for (std::size_t offset=0;offset<buffer.size();++offset) buffer[offset]=static_cast<std::uint8_t>(offset*13+seed);
        std::array<std::uint8_t,180> headers{};headers[20]=1;std::array<PlayerTemplateStateStream,9> streams{};streams[1]={buffer,17,end,present!=0};std::array<std::string,9> names{};std::uint32_t depth{};bool mission=true;std::string events="S"; // 같은 선택 경계의 pos0과 실제 데이터 입력이다.
        RawPlayerTemplateState body(OriginalEdition::Patch1078,headers,streams,names,depth,mission,[](auto) { throw std::logic_error("State 초기화 예상 밖 진단"); },[](auto) { throw std::logic_error("PATCH 빈 State의 CD 깊이 효과 금지"); });
        body.BindInitialization({[&](auto slot,std::span<std::uint8_t,28> output) { CHECK(slot==1);std::copy(stack.begin(),stack.end(),output.begin()); },
            [&](auto slot,std::span<const std::uint8_t,28> bytes)->PlayerTemplateStateStream& { CHECK(slot==1);events+='C';stack.assign(bytes.begin(),bytes.end());std::copy(bytes.begin(),bytes.end(),buffer.begin());streams[1]={buffer,28,28,true};return streams[1]; }});
        const auto result=body.Get(1);events+='E';CHECK(result.data()==buffer.data() && Hex(stack)==Hex(expectedStack) && Hex(buffer)==row[4] && events==row[5]);
        CHECK(streams[1].position==Get(expectedNode,4) && streams[1].end==Get(expectedNode,8) && streams[1].present);
    }
}
// 실제 빈 원시 섹션을 재생성하고 이전 노드/버퍼 대신 같은 새 State와 목록 끝을 공유한다.
TEST_CASE(CampaignTemplates_EmptyStateRecreatedWithCurrentBytesAndOrder) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    const auto at=InitialStateSection();fort.sections[at].clear();fort.sections[at].reserve(79);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"emptyState");storage.Borrow(8,1);
    const auto originalOrder=storage.SectionOrder(1);CHECK(originalOrder.size()==35 && originalOrder[at]=="State");std::array<std::uint8_t,28> seed{};seed.fill(0xa7);std::uint32_t depth{},initializations{};bool mission=true;
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("실제 State 생성 예상 밖 진단"); },[](auto) {});
    state.BindInitialization({[&](auto,std::span<std::uint8_t,28> bytes) { ++initializations;std::copy(seed.begin(),seed.end(),bytes.begin()); },[&](auto slot,std::span<const std::uint8_t,28> bytes)->PlayerTemplateStateStream& { return storage.InitializeState(slot,bytes); }});
    const auto first=state.Get(8);const auto order=storage.SectionOrder(1);CHECK(initializations==1 && order.size()==35 && order.back()=="State" && std::count(order.begin(),order.end(),"State")==1);
    CHECK(first[0]==0xa1 && first[1]==0xa7 && first[10]==0xa7 && first[18]==0xa7 && first[26]==0xa7 && first[4]==0 && first[17]==0 && first[24]==0);
    CHECK(first.size()==28 && first.data()==storage.Template(1).Section("State").data() && first.data()==storage.State(1).bytes.data() && storage.State(1).position==28 && storage.State(1).end==28 && storage.Template(1).sections[at].capacity()>=0x4000);
    CHECK(storage.CurrentSection(8)=="State");storage.SelectTerritory(1);CHECK(storage.CurrentSection(8)=="Territory" && storage.SectionOrder(8)==order);
    const auto second=state.Get(1);CHECK(initializations==1 && second.data()==first.data() && !storage.State(8).position && storage.SectionOrder(1)==order);storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
}
// 새 State 앞의 마지막 Territory 압축이 실제 버퍼를 교체하면 같은 지형 cursor도 새 버퍼/끝을 가리킨다.
TEST_CASE(CampaignTemplates_StateRecreationCompactsPreviousTerritoryTail) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    const auto names=FortTemplate::SectionNames();const auto territoryAt=static_cast<std::size_t>(std::find(names.begin(),names.end(),"Territory")-names.begin());fort.sections.resize(territoryAt+1);fort.sections[InitialStateSection()].clear();fort.sections[territoryAt].reserve(512);
    const auto before=fort.sections[territoryAt];CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"compactTail");storage.Borrow(8,1);std::array<std::uint8_t,28> record{};record.fill(0xa7);InitializePlayerTemplateStateBytes(record);
    const auto& state=storage.InitializeState(8,record);const auto& territory=storage.Territory(1);const auto order=storage.SectionOrder(1);
    CHECK(state.position==28 && territory.position==120 && territory.end==120 && order[order.size()-2]=="Territory" && order.back()=="State");
    CHECK(territory.bytes.data()==storage.Template(8).Section("Territory").data() && storage.Template(1).sections[territoryAt].capacity()==120 && std::equal(before.begin(),before.end(),territory.bytes.begin()));
    storage.SelectTerritory(8);CHECK(!territory.position && storage.SectionOrder(1)==order);storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
}
// 미연결 생성은 앞선pos0을 보존한 채 거부하고 생성 결과나CD 지원을 합성하지 않는다.
TEST_CASE(PlayerStateInitialization_RequiredEffectsAndEdition) {
    std::array<std::uint8_t,180> headers{};headers[20]=1;std::array<std::uint8_t,1> bytes{};std::array<PlayerTemplateStateStream,9> streams{};streams[1]={bytes,17,0,true};std::array<std::string,9> names{};std::uint32_t depth{};bool mission=true;
    RawPlayerTemplateState state(OriginalEdition::Patch1078,headers,streams,names,depth,mission,[](auto) {},[](auto) {});
    CHECK(Throws([&] { state.Get(1); }) && !streams[1].position);
    CHECK(Throws([&] { state.BindInitialization({}); }));
    RawPlayerTemplateState cd(OriginalEdition::Cd1072,headers,streams,names,depth,mission,[](auto) {},[](auto) {});
    CHECK(Throws([&] { cd.BindInitialization({[](auto,std::span<std::uint8_t,28>) {},[&](auto,std::span<const std::uint8_t,28>)->PlayerTemplateStateStream& { return streams[1]; }}); }));
}
