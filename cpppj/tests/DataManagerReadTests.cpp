// 실제 파일 읽기 부모의144행·mark 진단·목표11파일의 같은 버퍼/목록 연결을 검사한다.
#include "o/RawDataManagerImage.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12100100,kBuffers=0x12200100,kInputs=0x12201000; // 최초 원문에 사용한 불투명 관측 주소다.
constexpr std::size_t kNodeStart=64,kBufferStart=520,kInputStart=1080,kNamesStart=1336; // 전체1400BYTE 안의 실제 자료 위치다.
// 반복 표기로 저장한 원문도 모든 바이트를 복원하며 기대 결과를 제품에서 만들지 않는다.
std::vector<std::uint8_t> ReadBytes(std::string_view text) {
    std::vector<std::uint8_t> bytes;std::size_t at{};
    // 일반16진 두 글자 또는 ~BYTEx개수~를 원래 순서대로 펼친다.
    while (at<text.size()) {
        if (text[at]!='~') { bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));at+=2;continue; }
        const auto end=text.find('~',at+1),separator=text.find('x',at+1);if (end==text.npos || separator>=end) throw std::invalid_argument("파일 읽기 원문의 반복 표기 누락");
        const auto value=static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at+1,separator-at-1)),nullptr,16));const auto count=std::stoul(std::string(text.substr(separator+1,end-separator-1)));bytes.insert(bytes.end(),count,value);at=end+1;
    }
    return bytes;
}
}
// 아홉 본문과 큰 부모의 인라인 처리를 전체1400BYTE/모든 경계 상태의 독립144행과 대조한다.
TEST_CASE(DataManagerRead_Native144) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_READ_FIXTURE).rows;CHECK(rows.size()==144);
    // 초기화와 변이는 원본 입력만 사용하며 행 사이에 결과를 넘기지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==4);if (row.size()!=4) return;const auto input=Split(row[0],',');CHECK(input.size()==7);if (input.size()!=7) return;const auto variant=Number(input[6]);
        auto bytes=ReadBytes(row[1]);CHECK(bytes.size()==1400);if (bytes.size()!=1400) return;auto manager=std::span(bytes).subspan(8,12),source=std::span(bytes).subspan(36,20);std::uint32_t created{},allocating{};std::size_t eventIndex{};const auto events=Split(row[3],';');
        const auto node=[&](std::uint32_t pointer) { const auto offset=pointer-kNodes-8;if (offset%76 || offset/76>=6) throw std::out_of_range("파일 읽기 원문의 노드 범위");return std::span(bytes).subspan(kNodeStart+(offset/76)*76+8,60); }; // 같은60BYTE다.
        const auto buffer=[&](std::uint32_t pointer) {
            if (pointer==kInputs || pointer==kInputs+128) return std::span(bytes).subspan(kInputStart+pointer-kInputs,128);
            const auto offset=pointer-kBuffers-8;if (offset%80 || offset/80>=7) throw std::out_of_range("파일 읽기 원문의 버퍼 범위");return std::span(bytes).subspan(kBufferStart+(offset/80)*80+8,64);
        }; // 실제 입력 원문/대체 입력/할당은 서로 다른 같은 배열이다.
        const auto event=[&](char kind,std::string value) {
            CHECK(eventIndex<events.size());if (eventIndex>=events.size()) throw std::logic_error("파일 읽기 원문에 없는 경계 효과");const auto expected=Split(events[eventIndex++],',');CHECK(expected.size()==3);if (expected.size()!=3) throw std::logic_error("파일 읽기 경계 열 누락");CHECK(expected[0]==std::string(1,kind) && expected[1]==value && ReadBytes(expected[2])==bytes);
        }; // 외부 효과 직전 전체 상태를 즉시 최초 원문과 비교한다.
        DataManagerImageHooks hooks;hooks.names=[&] { event('T',"names");return std::span(bytes).subspan(kNamesStart,64); };
        hooks.streams.selection.stream=node;hooks.streams.selection.compare=[&](std::string_view left,std::string_view right) { event('Q',Hex({reinterpret_cast<const std::uint8_t*>(left.data()),left.size()})+'/'+Hex({reinterpret_cast<const std::uint8_t*>(right.data()),right.size()}));return CompareDataManagerSectionNames(left,right); };
        hooks.streams.allocateStream=[&](auto size) { event('S',std::to_string(size));CHECK(size==60 && created<3);allocating=kNodes+(3+created++)*76+8;return allocating; };
        hooks.streams.memory.buffer=buffer;hooks.streams.memory.allocate=[&](auto size) { event('A',std::to_string(size));const auto index=(allocating-kNodes-8)/76;CHECK(size<=64);if (variant==1) Put(source,0,kInputs+128);return kBuffers+(index>=3 ? index : 6)*80+8; };
        const auto release=[&](char kind,std::uint32_t pointer) { event(kind,std::to_string(pointer));if (variant==2) Put(manager,8,0xdead5678); }; // free 뒤 current 변이를 원본 후속 쓰기와 구분한다.
        hooks.streams.memory.release=[&](auto pointer) { release('F',pointer); };hooks.streams.list.lifetime.stream=node;hooks.streams.list.lifetime.releaseBuffer=hooks.streams.memory.release;hooks.streams.list.lifetime.releaseStream=[&](auto pointer) { release('N',pointer); };
        hooks.streams.list.compact=[&](auto pointer) { allocating=pointer;CompactMemStream(node(pointer),hooks.streams.memory); };
        hooks.streams.list.missingPredecessor=[] { throw std::logic_error("정상 파일 원문에 없는 선행 링크 진단"); };hooks.streams.memory.assertion=[](auto) { throw std::logic_error("정상 파일 원문에 없는 MemStream 진단"); };hooks.assertion=[](auto) { throw std::logic_error("정상 파일 원문에 없는 mark/SNART 진단"); };
        ReadDataManagerImage(manager,source,hooks);CHECK(bytes==ReadBytes(row[2]) && eventIndex==events.size());
    }
}
// 진단이pos/end를 바꾸면 원본 mark 부모도 그 현재 값을 읽어 진행한다.
TEST_CASE(DataManagerRead_MarkAssertionUsesCurrentFields) {
    std::array<std::uint8_t,20> source{};std::array<std::uint8_t,8> image{5,0};Put(source,0,1);Put(source,4,3);Put(source,8,3);Put(source,16,0);std::uint32_t diagnoses{};DataManagerImageHooks hooks;
    hooks.streams.memory.buffer=[&](auto pointer) { CHECK(pointer==1);return std::span(image); };hooks.assertion=[&](auto kind) { ++diagnoses;CHECK(kind==DataManagerImageAssertion::MarkLength);Put(source,4,4);Put(source,8,4); };
    AdvanceDataManagerMark(source,hooks);CHECK(diagnoses==1 && Get(source,16)==4 && Get(source,4)==6 && Get(source,8)==4);
}
// 이름 표 가상 조회가 먼저 수행되고 잘못된 시작pos 진단의 수리를 다음mark가 읽어야 한다.
TEST_CASE(DataManagerRead_InitialPositionAndRequiredNames) {
    std::array<std::uint8_t,12> manager{};std::array<std::uint8_t,20> source{};std::array<std::uint8_t,4> names{'/','/',0,0};Put(source,4,3);Put(source,8,2);Put(source,16,0xffffffffU);std::string order;DataManagerImageHooks hooks;
    hooks.names=[&] { order+='N';return std::span(names); };hooks.assertion=[&](auto kind) { order+='D';CHECK(kind==DataManagerImageAssertion::SourcePosition);Put(source,4,2); };
    ReadDataManagerImage(manager,source,hooks);CHECK(order=="ND" && Get(source,16)==2 && Get(source,4)==4 && names[0]==0 && names[1]==0 && names[2]==0xff);
    bool failed{};try { ReadDataManagerImage(manager,source,{}); } catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
// 목표11미션의 실제 선택fort를 원본 읽기 부모로 설치하고 바이트/순서/빌림/typed cursor를 확인한다.
TEST_CASE(CampaignTemplates_ReadTargetElevenFiles) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    constexpr std::array<std::string_view,11> forts{"bridgethegap","secretworkshop","capturethepriest","tacticalcombat","subtledefense","rawpower","thewarbegins","masterofwhirligigs","demostormrages","guardmyback","dissolvedalliance"}; // Early6/캠페인1-1~1-2/데모3의 실제 loadFort다.
    // 매파일은 새 owner와 같은 빌림만 만들며 다른파일의 현재 자료를 재사용하지 않는다.
    for (const auto name:forts) {
        const auto bytes=assets.files.Read("d/"+std::string(name)+".fort");auto fort=FortTemplate::Parse(bytes,assets.game.TypeTable());const auto expected=fort.sections;CampaignTemplateStorage storage;storage.InstallFile(1,std::move(fort),bytes,std::string(name));storage.Borrow(8,1);
        CHECK(storage.Template(1).sections==expected && &storage.Template(1)==&storage.Template(8) && storage.CurrentSection(1)=="Terr19");const auto order=storage.SectionOrder(8);const auto names=FortTemplate::SectionNames();CHECK(order.size()==names.size() && std::equal(order.begin(),order.end(),names.begin()));
        const auto& state=storage.State(8);const auto& territory=storage.Territory(8);CHECK(!state.position && !territory.position && state.end==storage.Template(1).Section("State").size() && territory.end==storage.Template(1).Section("Territory").size());
        CHECK(territory.bytes.data()==storage.Template(1).Section("Territory").data());storage.Clear(8);storage.ClearAll();CHECK(!storage.OwnedImages());
    }
}
