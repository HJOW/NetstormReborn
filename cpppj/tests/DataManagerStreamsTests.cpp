// 독립 원본404행의 전체 메모리/경계 순서와 실제 템플릿 소비자를 검사한다.
#include "o/RawMemStream.h"
#include "o/RawDataManagerList.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12100100,kBuffers=0x12200100; // 독립 원문에 사용한 불투명 노드/버퍼 주소다.
constexpr std::size_t kNodeStride=76,kBufferStride=80,kBufferStart=332,kSourceStart=652; // 전체716BYTE 중 노드/버퍼/원문의 실제 위치다.
// 원문 입력/기대 바이트만 읽으며 제품의 결과를 계산하지 않는다.
std::vector<std::uint8_t> StreamBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 원문의 두16진 글자를 한 바이트로 옮긴다.
    for (std::size_t position=0;position<text.size();position+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(position,2)),nullptr,16)));return result;
}
}
// 실제 중첩 할당/쓰기/압축/소멸과 진단/힙 경계 뒤 재조회를 전체404행에 대조한다.
TEST_CASE(DataManagerStreams_Native404) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_STREAMS_FIXTURE).rows;CHECK(rows.size()==404);
    // 매 행은 다른 행의 결과와 독립적인 같은 전체 메모리를 사용한다.
    for (const auto& row:rows) {
        CHECK(row.size()==4);if (row.size()!=4) return;const auto input=Split(row[0],',');CHECK(input.size()==10);if (input.size()!=10) return;
        auto bytes=StreamBytes(row[1]);CHECK(bytes.size()==716);if (bytes.size()!=716) return;
        auto manager=std::span(bytes).subspan(8,12);const auto kind=input[0][0];const auto count=Number(input[2]),select=Number(input[3]),length=Number(input[8]),variant=Number(input[9]);std::string events;
        const auto selected=kNodes+static_cast<std::uint32_t>((kind=='P' && count ? count-1 : select)*kNodeStride)+8;
        const auto node=[&](std::uint32_t handle) { const auto offset=handle-kNodes-8;if (offset%kNodeStride || offset/kNodeStride>=4) throw std::out_of_range("원문 실제 노드 범위");return std::span(bytes).subspan(28+(offset/kNodeStride)*kNodeStride+8,60); }; // 같은 실제 노드만 빌린다.
        const auto buffer=[&](std::uint32_t handle) { const auto offset=handle-kBuffers-8;if (offset%kBufferStride || offset/kBufferStride>=4) throw std::out_of_range("원문 실제 버퍼 범위");return std::span(bytes).subspan(kBufferStart+(offset/kBufferStride)*kBufferStride+8,64); }; // free 뒤에도 관측용 바이트는 지우지 않는다.
        const auto event=[&](char type,std::uint32_t value) { if (!events.empty()) events+=';';events+=type;events+=','+std::to_string(value)+','+Hex(bytes); }; // 외부 효과 직전 전체716BYTE를 기록한다.
        MemStreamHooks memory;memory.buffer=buffer;
        memory.allocate=[&](std::uint32_t size) { event('A',size);if (variant==1) { Put(node(selected),12,size/2);Put(node(selected),4,0x87654321); }return kBuffers+3*static_cast<std::uint32_t>(kBufferStride)+8; }; // 원문 allocator의 명시 변이 입력이다.
        memory.release=[&](std::uint32_t handle) { event('F',handle);if (variant==2) { Put(node(selected),4,0x87654321);Put(node(selected),8,0x1234);Put(manager,8,0x456789ab); } }; // 원본 free 경계 뒤 재조회를 대조한다.
        memory.assertion=[&](MemStreamAssertion reason) { event('D',reason==MemStreamAssertion::ExistingData ? 0x14 : 0x24);if (kind=='W' && variant==1) Put(node(selected),4,5); };
        DataManagerListHooks list;list.lifetime.stream=node;list.lifetime.releaseBuffer=memory.release;list.lifetime.releaseStream=[&](auto handle) { event('N',handle); };list.compact=[&](auto handle) { CompactMemStream(node(handle),memory); };list.missingPredecessor=[] { throw std::logic_error("정상 원문404행에 없는 링크 진단"); };
        switch (kind) {
        case 'A':AllocateMemStream(node(selected),length,memory);break;
        case 'W':WriteMemStream(node(selected),variant==3 ? std::span<const std::uint8_t>(buffer(kBuffers+8)).subspan(1) : std::span<const std::uint8_t>(bytes).subspan(kSourceStart,64),length,memory);break;
        case 'C':CompactMemStream(node(selected),memory);break;
        case 'P':AppendDataManagerStream(manager,select ? kNodes+3*static_cast<std::uint32_t>(kNodeStride)+8 : 0,list);break;
        case 'R':RemoveDataManagerStream(manager,selected,list);break;
        default:throw std::logic_error("원문 입력의 동작 종류 오류");
        }
        CHECK(Hex(bytes)==row[2] && (events.empty() ? "-" : events)==row[3]);
    }
}
//0길이와 빈 목록은 원본이 실제로 접근하지 않는 버퍼/노드 소비자를 요구하지 않는다.
TEST_CASE(DataManagerStreams_PhysicalAndZeroContracts) {
    std::array<std::uint8_t,24> storage{};storage.fill(0xa5);const auto before=storage;
    CHECK(Throws([&] { AllocateMemStream(std::span(storage).first(15),0,{}); }) && storage==before);
    auto stream=std::span(storage).first(16);Put(stream,0,0);MemStreamHooks memory;memory.allocate=[](std::uint32_t size) { CHECK(!size);return 0U; };AllocateMemStream(stream,0,memory);
    CHECK(std::all_of(stream.begin(),stream.end(),[](auto byte) { return !byte; }) && storage[16]==0xa5);
    WriteMemStream(stream,{},0,{});CompactMemStream(stream,{});CHECK(Get(stream,4)==0 && Get(stream,8)==0);
    std::array<std::uint8_t,12> manager{};Put(manager,0,0x87654321);AppendDataManagerStream(manager,0x1234,{});CHECK(Get(manager,0)==0x87654321 && Get(manager,4)==0x1234 && Get(manager,8)==0x1234);
    Put(manager,4,0);CHECK(Throws([&] { RemoveDataManagerStream(manager,0,{}); }));
}
// 진단 뒤 수리된 같은 선행 노드를 다시 읽는 계약이며 원문404행의 통과 수와 구분한다.
TEST_CASE(DataManagerStreams_RemoveRequeriesRepairedPredecessor) {
    std::array<std::uint8_t,12> manager{};std::array<std::array<std::uint8_t,60>,3> nodes{};Put(manager,4,1);Put(manager,8,9);Put(nodes[1],4,17);Put(nodes[1],8,19);std::uint32_t released{},diagnosed{};
    DataManagerListHooks hooks;hooks.lifetime.stream=[&](auto handle) { return std::span(nodes.at(handle-1)); };hooks.lifetime.releaseStream=[&](auto handle) { released=handle; };
    hooks.missingPredecessor=[&] { ++diagnosed;Put(nodes[0],0x38,2);Put(nodes[1],0x38,3); };RemoveDataManagerStream(manager,2,hooks);
    CHECK(diagnosed==1 && released==2 && Get(manager,4)==1 && Get(manager,8)==9 && Get(nodes[0],0x38)==3 && Get(nodes[1],0x38)==3 && !Get(nodes[1],4) && !Get(nodes[1],8));
}
// 같은 실제 raw 섹션/노드가 공통 할당/추가/압축/제거/쓰기와 반복 재생성을 사용한다.
TEST_CASE(CampaignTemplates_CommonStreamsRecreateAndReleaseEmptyTail) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());const auto names=FortTemplate::SectionNames();const auto at=static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin());
    fort.sections[at].clear();fort.sections.back().clear();fort.sections.back().reserve(37);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"commonStreams");storage.Borrow(8,1);std::array<std::uint8_t,28> record{};record.fill(0xa7);InitializePlayerTemplateStateBytes(record);
    const auto first=storage.InitializeState(8,record).bytes;CHECK(first.size()==28 && std::equal(record.begin(),record.end(),first.begin()) && storage.CurrentSection(1)=="State" && storage.SectionOrder(1).back()=="State" && storage.Template(1).sections.back().capacity()==0);
    record[3]^=0x40;const auto second=storage.InitializeState(1,record).bytes;const auto order=storage.SectionOrder(8);CHECK(second.data()==storage.Template(8).Section("State").data() && std::equal(record.begin(),record.end(),second.begin()) && order.size()==35 && order.back()=="State" && std::count(order.begin(),order.end(),"State")==1 && storage.State(8).position==28);
    storage.SelectTerritory(1);CHECK(storage.SectionOrder(8)==order);storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
}
