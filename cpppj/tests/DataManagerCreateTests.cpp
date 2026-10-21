// 섹션 생성/복제·대입의 독립220행과 실제 State 생성 부모 연결을 검사한다.
#include "o/RawDataManagerResize.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12100100,kBuffers=0x12200100,kLarge=0x12210000; // 최초 관측의 불투명 포인터이며 호스트 주소로 사용하지 않는다.
constexpr std::size_t kNodeStart=56,kNodeStride=76,kBufferStart=436,kBufferStride=80,kLargeStart=916,kNamesStart=17316; // 전체17380BYTE에 있는 실제 자료의 위치다.
// 같은 바이트의 반복 표기도 빠짐없이 전체 메모리로 복원하며 제품 결과를 계산하지 않는다.
std::vector<std::uint8_t> CreateBytes(std::string_view text) {
    std::vector<std::uint8_t> bytes;std::size_t position{};
    // 일반16진 두 글자와 ~BYTEx개수~를 원래 순서로 읽는다.
    while (position<text.size()) {
        if (text[position]!='~') { bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(position,2)),nullptr,16)));position+=2;continue; }
        const auto end=text.find('~',position+1),separator=text.find('x',position+1);if (end==text.npos || separator>=end) throw std::invalid_argument("생성 원문의 반복 표기 누락");
        const auto value=static_cast<std::uint8_t>(std::stoul(std::string(text.substr(position+1,separator-position-1)),nullptr,16));const auto count=std::stoul(std::string(text.substr(separator+1,end-separator-1)));bytes.insert(bytes.end(),count,value);position=end+1;
    }
    return bytes;
}
// 원문의 이름 바이트를 첫NUL까지 같은 위치에서 빌린다.
std::string_view CreateName(std::span<const std::uint8_t> bytes,std::size_t at) {
    const auto tail=bytes.subspan(at);const auto end=std::find(tail.begin(),tail.end(),std::uint8_t{});if (end==tail.end()) throw std::out_of_range("생성 원문 이름 NUL 누락");return {reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin())};
}
}
// 열한 실제 본문의 전체17380BYTE·반환 핸들·모든 경계 직전 상태를 독립220행과 대조한다.
TEST_CASE(DataManagerCreate_Native220) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_CREATE_FIXTURE).rows;CHECK(rows.size()==220);
    // 각 행의 입력/기대 바이트는 원본 PE의 최초 관측만 사용한다.
    for (const auto& row:rows) {
        CHECK(row.size()==5);if (row.size()!=5) return;const auto inputs=Split(row[0],',');CHECK(inputs.size()==6);if (inputs.size()!=6) return;
        auto bytes=CreateBytes(row[1]);CHECK(bytes.size()==17380);if (bytes.size()!=17380) return;auto manager=std::span(bytes).subspan(8,12),sourceManager=std::span(bytes).subspan(36,12);
        const auto kind=inputs[0][0];const auto variant=Number(inputs[5]);const auto original=kNodes+3*static_cast<std::uint32_t>(kNodeStride)+8,fresh=kNodes+4*static_cast<std::uint32_t>(kNodeStride)+8;std::uint32_t smallAllocations{};std::size_t eventIndex{};const auto events=row[4]=="-" ? std::vector<std::string>{} : Split(row[4],';');
        const auto node=[&](std::uint32_t handle) { const auto offset=handle-kNodes-8;if (offset%kNodeStride || offset/kNodeStride>=5) throw std::out_of_range("생성 원문 노드 범위");return std::span(bytes).subspan(kNodeStart+(offset/kNodeStride)*kNodeStride+8,60); }; // 같은 실제 노드60BYTE다.
        const auto buffer=[&](std::uint32_t pointer) {
            if (pointer>=kLarge+8 && pointer<kLarge+8+0x4000) { const auto offset=pointer-kLarge-8;return std::span(bytes).subspan(kLargeStart+8+offset,0x4000-offset); }
            const auto offset=pointer-kBuffers-8;const auto index=offset/kBufferStride,inside=offset%kBufferStride;if (index>=6 || inside>=64) throw std::out_of_range("생성 원문 실제 버퍼 범위");return std::span(bytes).subspan(kBufferStart+index*kBufferStride+8+inside,64-inside);
        }; // 원문 data의+3BYTE 변이도 같은 할당 내부에서 읽는다.
        const auto event=[&](char type,std::string value) {
            CHECK(eventIndex<events.size());if (eventIndex>=events.size()) throw std::logic_error("생성 원문에 없는 경계 효과");const auto expected=Split(events[eventIndex++],',');CHECK(expected.size()==3);if (expected.size()!=3) throw std::logic_error("생성 원문의 경계 열 누락");CHECK(expected[0]==std::string(1,type) && expected[1]==value && CreateBytes(expected[2])==bytes);
        }; // 제품의 현 메모리를 최초 경계 직전의 전체 원문과 즉시 비교한다.
        DataManagerResizeHooks hooks;hooks.selection.stream=node;
        hooks.selection.compare=[&](std::string_view left,std::string_view right) { event('Q',Hex({reinterpret_cast<const std::uint8_t*>(left.data()),left.size()})+'/'+Hex({reinterpret_cast<const std::uint8_t*>(right.data()),right.size()}));return CompareDataManagerSectionNames(left,right); };
        hooks.memory.buffer=buffer;hooks.memory.allocate=[&](std::uint32_t size) {
            event('A',std::to_string(size));const auto pointer=size==0x4000 ? kLarge+8 : kBuffers+(4+smallAllocations++)*static_cast<std::uint32_t>(kBufferStride)+8;
            if ((variant==1 && size==0x4000) || (variant==3 && kind=='M')) { Put(node(original),0,kBuffers+2*static_cast<std::uint32_t>(kBufferStride)+11);Put(node(original),8,2); }return pointer;
        }; // 할당 경계에서 원문을 바꿔 대입 부모의 data/end 재조회를 검사한다.
        const auto release=[&](char type,std::uint32_t pointer) { event(type,std::to_string(pointer));if (variant==2) { Put(manager,8,0xdead5678);Put(node(original),0,kBuffers+static_cast<std::uint32_t>(kBufferStride)+8);Put(node(original),8,3); } }; // 반납 뒤 current/원문 변이는 본문의 후속 쓰기와 구별한다.
        hooks.memory.release=[&](auto pointer) { release('F',pointer); };hooks.memory.assertion=[](auto) { throw std::logic_error("정상 생성 원문에 없는 MemStream 진단"); };
        hooks.list.lifetime.stream=node;hooks.list.lifetime.releaseBuffer=hooks.memory.release;hooks.list.lifetime.releaseStream=[&](auto handle) { release('N',handle); };
        hooks.list.compact=[&](auto handle) { CompactMemStream(node(handle),hooks.memory); };hooks.list.missingPredecessor=[] { throw std::logic_error("정상 생성 원문에 없는 선행 노드 진단"); };
        hooks.allocateStream=[&](auto size) { event('S',std::to_string(size));CHECK(size==60);return fresh; };hooks.missingSource=[] { throw std::logic_error("정상 복제 원문에 없는 원문 누락"); };
        std::uint32_t result{};const auto name=CreateName(bytes,kNamesStart);
        if (kind=='N') result=CreateDataManagerStream(manager,name,hooks);
        else if (kind=='C') result=CloneDataManagerStream(manager,sourceManager,name,hooks);
        else CopyMemStream(node(fresh),node(original),hooks.memory);
        CHECK((kind=='M' || result==Number(row[3])) && bytes==CreateBytes(row[2]) && eventIndex==events.size());
    }
}
// 빈 manager의 new60 실패는 원본처럼NULL을 추가하고 현재0을 반환하며 임의 노드를 만들지 않는다.
TEST_CASE(DataManagerCreate_EmptyNullAllocationAndRequiredEffects) {
    std::array<std::uint8_t,12> manager{};DataManagerResizeHooks hooks;std::uint32_t requests{};hooks.allocateStream=[&](auto size) { ++requests;CHECK(size==60);return 0U; };
    CHECK(CreateDataManagerStream(manager,"State",hooks)==0 && requests==1 && std::all_of(manager.begin(),manager.end(),[](auto byte) { return !byte; }));
    bool failed{};try { CreateDataManagerStream(manager,"State",{}); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
}
// 자기 대입은 free 뒤 같은 첫16BYTE가 비워져 길이0으로 재할당되는 원본의 결과다.
TEST_CASE(MemStreamCopy_SelfAssignmentResetsBeforeReadingLength) {
    std::array<std::uint8_t,60> node{};std::fill(node.begin()+16,node.end(),std::uint8_t{0xa7});Put(node,0,1);Put(node,4,4);Put(node,8,7);Put(node,12,8);std::uint32_t releases{},allocations{};MemStreamHooks hooks;
    hooks.release=[&](auto pointer) { ++releases;CHECK(pointer==1 && Get(node,4)==4 && Get(node,8)==7); };hooks.allocate=[&](auto size) { ++allocations;CHECK(!size && !Get(node,0) && !Get(node,4) && !Get(node,8) && !Get(node,12));return 2U; };
    hooks.buffer=[](auto)->std::span<std::uint8_t> { throw std::logic_error("0길이 자기 대입에서 버퍼 조회 금지"); };CopyMemStream(node,node,hooks);
    CHECK(releases==1 && allocations==1 && Get(node,0)==2 && !Get(node,4) && !Get(node,8) && !Get(node,12) && std::all_of(node.begin()+16,node.end(),[](auto byte) { return byte==0xa7; }));
}
// 실제 State 생성은 원본 전체 부모에서 기존 노드를 제거하고 빌린 헤더도 같은 새 할당을 받는다.
TEST_CASE(CampaignTemplates_StateCreationParentPreservesSharedOrderAndTail) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"createState");storage.Borrow(8,1);std::array<std::uint8_t,28> input{};
    // 첫 교체와 이미 마지막인 State의 반복 교체를 같은 소유 manager에 적용한다.
    for (std::uint8_t repeat=0;repeat<3;++repeat) {
        std::fill(input.begin(),input.end(),static_cast<std::uint8_t>(0x31+repeat));auto& state=storage.InitializeState(8,input);const auto order=storage.SectionOrder(1);
        CHECK(state.present && state.position==28 && state.end==28 && state.bytes.size()==28 && std::equal(input.begin(),input.end(),state.bytes.begin()) && &state==&storage.State(1));
        CHECK(order.size()==35 && order.back()=="State" && std::count(order.begin(),order.end(),"State")==1 && storage.CurrentSection(1)=="State" && state.bytes.data()==storage.Template(1).Section("State").data());
    }
    storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
}
