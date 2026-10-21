// 최소 확장/생성/이름 교체의 독립286행과 실제 새 현재 State/목록 순서를 검사한다.
#include "o/RawDataManagerResize.h"
#include "o/RawPlayerTemplateState.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12100100,kBuffers=0x12200100; // 최초 원문에 사용한 불투명 노드/버퍼 주소다.
constexpr std::size_t kNodeStride=76,kBufferStride=80,kBufferStart=332,kNamesStart=732; // 전체796BYTE 중 각 자료의 실제 위치다.
// 원문의 입력/기대 바이트를 읽되 제품의 결과를 계산하지 않는다.
std::vector<std::uint8_t> ResizeBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 두16진 글자를 같은 원문 순서의 한 바이트로 변환한다.
    for (std::size_t position=0;position<text.size();position+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(position,2)),nullptr,16)));return result;
}
// 실제 이름 인자의 첫NUL까지 같은 메모리를 빌린다.
std::string_view ResizeName(std::span<const std::uint8_t> bytes,std::size_t position) {
    const auto tail=bytes.subspan(position);const auto end=std::find(tail.begin(),tail.end(),std::uint8_t{});if (end==tail.end()) throw std::out_of_range("원문 이름 NUL 누락");return {reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin())};
}
// State의 정해진 파일 섹션 번호만 찾는다.
std::size_t ResizeStateSection() { const auto names=FortTemplate::SectionNames();return static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin()); }
}
// 부모/하위 열 본문의 전체796BYTE·반환 핸들·각 경계 상태를 독립286행에 대조한다.
TEST_CASE(DataManagerResize_Native286) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_RESIZE_FIXTURE).rows;CHECK(rows.size()==286);
    // 각 행은 다른 행의 결과와 독립적인 같은 원본 입력을 적용한다.
    for (const auto& row:rows) {
        CHECK(row.size()==5);if (row.size()!=5) return;const auto input=Split(row[0],',');CHECK(input.size()==7);if (input.size()!=7) return;
        auto bytes=ResizeBytes(row[1]);CHECK(bytes.size()==796);if (bytes.size()!=796) return;auto manager=std::span(bytes).subspan(8,12);
        const auto kind=input[0][0];const auto position=Number(input[2]),nameKind=Number(input[5]),variant=Number(input[6]);const auto minimum=static_cast<std::int32_t>(std::stoll(input[4]));std::uint32_t allocations{};std::string events;
        const auto original=kNodes+position*static_cast<std::uint32_t>(kNodeStride)+8,temporary=kNodes+3*static_cast<std::uint32_t>(kNodeStride)+8;
        const auto node=[&](std::uint32_t handle) { const auto offset=handle-kNodes-8;if (offset%kNodeStride || offset/kNodeStride>=4) throw std::out_of_range("확장 원문 노드 범위");return std::span(bytes).subspan(28+(offset/kNodeStride)*kNodeStride+8,60); }; // 같은 raw 노드만 조회한다.
        const auto buffer=[&](std::uint32_t handle) { const auto offset=handle-kBuffers-8;if (offset%kBufferStride || offset/kBufferStride>=5) throw std::out_of_range("확장 원문 버퍼 범위");return std::span(bytes).subspan(kBufferStart+(offset/kBufferStride)*kBufferStride+8,64); }; // 반납한 관측용 바이트도 지우지 않는다.
        const auto event=[&](char type,std::string value) { if (!events.empty()) events+=';';events+=type;events+=','+value+','+Hex(bytes); }; // 외부 효과 전 현재796BYTE를 기록한다.
        DataManagerResizeHooks hooks;hooks.selection.stream=node;
        hooks.selection.compare=[&](std::string_view left,std::string_view right) { event('Q',Hex({reinterpret_cast<const std::uint8_t*>(left.data()),left.size()})+'/'+Hex({reinterpret_cast<const std::uint8_t*>(right.data()),right.size()}));return CompareDataManagerSectionNames(left,right); };
        hooks.memory.buffer=buffer;
        hooks.memory.allocate=[&](std::uint32_t size) {
            event('A',std::to_string(size));const auto pointer=kBuffers+(3+allocations)*static_cast<std::uint32_t>(kBufferStride)+8;
            if (variant==1 && kind=='G' && !allocations) { Put(node(original),0,kBuffers+((position+1)%3)*static_cast<std::uint32_t>(kBufferStride)+8);Put(node(original),8,2);Put(manager,8,0xdead1234); }
            if (variant==3 && !allocations) Put(node(temporary),12,size+1);++allocations;return pointer;
        }; // 원본 allocator의 명시 입력만 적용해 생성 뒤 data/end/alloc 재조회를 검사한다.
        hooks.memory.release=[&](std::uint32_t pointer) { event('F',std::to_string(pointer));if (variant==2) Put(manager,8,0xdead5678); };
        hooks.memory.assertion=[](auto) { throw std::logic_error("정상 확장 원문에 없는 MemStream 진단"); };
        hooks.list.lifetime.stream=node;hooks.list.lifetime.releaseBuffer=hooks.memory.release;hooks.list.lifetime.releaseStream=[&](auto handle) { event('N',std::to_string(handle));if (variant==2) Put(manager,8,0xdead5678); };
        hooks.list.compact=[&](auto handle) { CompactMemStream(node(handle),hooks.memory); };hooks.list.missingPredecessor=[] { throw std::logic_error("정상 확장 원문에 없는 링크 진단"); };
        hooks.allocateStream=[&](auto size) { event('S',std::to_string(size));CHECK(size==60);return temporary; };hooks.missingSource=[] { throw std::logic_error("정상 확장 원문에 없는 원문 누락"); };
        const auto desired=ResizeName(bytes,kNamesStart),prior=ResizeName(bytes,kNamesStart+32);std::uint32_t result{};
        if (kind=='G') result=EnsureDataManagerStreamSize(manager,desired,minimum,hooks);
        else if (kind=='R') result=RenameDataManagerStream(manager,desired,prior,hooks);
        else result=ConstructDataManagerStream(temporary,nameKind==2 ? std::nullopt : std::optional<std::string_view>(nameKind==1 ? prior : desired),minimum,hooks);
        CHECK(result==Number(row[3]) && Hex(bytes)==row[2] && (events.empty() ? "-" : events)==row[4]);
    }
}
// 실제 getter는 임시 섹션 부모가 만든 새 현재 stream을 받고 분리 할당/중복 복사를 호출하지 않는다.
TEST_CASE(CampaignTemplates_MinimumStateUsesTemporarySectionAndSharedData) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());fort.sections[ResizeStateSection()]={0x12,0x34,0x56};CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"minimumState");storage.Borrow(8,1);
    std::uint32_t depth{},ensured{};bool mission=true;RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("최소 State 예상 밖 진단"); },[](auto) {},[](auto,auto)->std::span<std::uint8_t> { throw std::logic_error("실제 최소 확장에서 분리 할당 호출 금지"); },[](auto) { throw std::logic_error("실제 최소 확장에서 분리 복사 완료 호출 금지"); });
    state.BindMinimumSize([&](auto slot,std::int32_t minimum)->PlayerTemplateStateStream& { ++ensured;CHECK(minimum==28);return storage.EnsureStateSize(slot,minimum); });
    const auto first=state.Get(8);const auto order=storage.SectionOrder(1);CHECK(ensured==1 && first.size()==28 && first[0]==0x12 && first[1]==0x34 && first[2]==0x56 && std::all_of(first.begin()+3,first.end(),[](auto byte) { return !byte; }));
    CHECK(first.data()==storage.Template(1).Section("State").data() && first.data()==storage.State(1).bytes.data() && order.size()==35 && order.back()=="State" && std::count(order.begin(),order.end(),"State")==1 && storage.State(1).position==28 && storage.State(1).end==28);
    const auto second=state.Get(1);CHECK(second.data()==first.data() && ensured==1 && !storage.State(8).position && storage.SectionOrder(8)==order);storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
}
// State가 마지막이고 원래 큰 할당을 가진 경우에도 원문 복사를 마친 뒤 tail 압축/원래 State 반납을 수행한다.
TEST_CASE(CampaignTemplates_MinimumStateTailCopiesBeforeCompaction) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CampaignTemplateStorage storage;storage.Install(1,FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable()),"tailMinimum");std::array<std::uint8_t,28> initial{};initial.fill(0xa7);InitializePlayerTemplateStateBytes(initial);storage.InitializeState(1,initial);
    auto& bytes=storage.Template(1).sections[ResizeStateSection()];bytes.resize(3);bytes[0]=0x12;bytes[1]=0x34;bytes[2]=0x56;storage.State(1)={bytes,0,3,true};CHECK(storage.SectionOrder(1).back()=="State" && bytes.capacity()>=0x4000);
    const auto& current=storage.EnsureStateSize(1,28);const auto order=storage.SectionOrder(1);CHECK(current.bytes.size()==28 && current.position==28 && current.end==28 && current.bytes[0]==0x12 && current.bytes[1]==0x34 && current.bytes[2]==0x56 && std::all_of(current.bytes.begin()+3,current.bytes.end(),[](auto byte) { return !byte; }));
    CHECK(order.size()==35 && order.back()=="State" && std::count(order.begin(),order.end(),"State")==1 && current.bytes.data()==storage.Template(1).Section("State").data());storage.ClearAll();CHECK(!storage.OwnedImages());
}
// signed 최소 크기와 충분한 원문은 할당/순서/cursor를 보존하며 CD 또는 빈 부모 연결은 거부한다.
TEST_CASE(DataManagerResize_SufficientStateAndRequiredBinding) {
    FortTemplate fort;fort.sections.resize(ResizeStateSection()+1);fort.sections[ResizeStateSection()].assign(32,0xa7);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"sufficient");storage.State(1).position=17;const auto* pointer=storage.State(1).bytes.data();const auto order=storage.SectionOrder(1);
    // 실제 signed 부모의 음수/0/이미 충분한 최소 경계를 같은 저장소에 적용한다.
    for (const auto minimum:{-1,0,1,28,32}) { const auto& state=storage.EnsureStateSize(1,minimum);CHECK(state.bytes.data()==pointer && state.position==17 && state.end==32 && storage.SectionOrder(1)==order); }
    std::array<PlayerTemplateStateStream,9> streams{};std::array<std::string,9> names{};std::uint32_t depth{};bool mission=true;RawPlayerTemplateState cd(OriginalEdition::Cd1072,storage.Headers(),streams,names,depth,mission,[](auto) {},[](auto) {});
    CHECK(Throws([&] { cd.BindMinimumSize([&](auto,std::int32_t)->PlayerTemplateStateStream& { return streams[1]; }); }));RawPlayerTemplateState patch(OriginalEdition::Patch1078,storage.Headers(),streams,names,depth,mission,[](auto) {},[](auto) {});CHECK(Throws([&] { patch.BindMinimumSize({}); }));storage.ClearAll();
}
