// 독립 원본864입력의 전체256BYTE·경계 직전 상태와 같은 Template의 선택을 검사한다.
#include "o/RawDataManagerSelection.h"
#include "client/CampaignTemplateStorage.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12100100; // 최초 원문에 사용한 불투명 노드 주소다.
constexpr std::size_t kStride=76; // 실제60BYTE와 양쪽 보호8BYTE의 한 입력 폭이다.
// 원본의 입력/기대 바이트를 변환하며 선택 결과를 계산하지 않는다.
std::vector<std::uint8_t> SelectionBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 원문의 두16진 글자를 한 바이트로 옮긴다.
    for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));
    return result;
}
}
// 이름/순서/첫 일치/검색 실패/current 변이/두 진단 뒤 재조회와 정확한 쓰기 폭을 대조한다.
TEST_CASE(DataManagerSelection_Native864) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_SELECTION_FIXTURE).rows;CHECK(rows.size()==865 && rows.front()[0]=="input");
    // 각 행은 독립적으로 관측한 원본 전체 메모리를 같은 저장소에 적용한다.
    for (const auto& row:rows) {
        if (row[0]=="input") continue; // 최초 CSV 머리글은 관측 입력에 포함하지 않는다.
        CHECK(row.size()==5);if (row.size()!=5) return;const auto input=Split(row[0],',');CHECK(input.size()==7);if (input.size()!=7) return;
        auto bytes=SelectionBytes(row[1]);CHECK(bytes.size()==256);if (bytes.size()!=256) return;
        auto manager=std::span(bytes).subspan(8,12);const auto variant=Number(input[6]),query=Number(input[3]);std::string trace;std::uint32_t comparisons{}; // 독립 원문의 경계 변이와 관측 횟수다.
        DataManagerSelectionHooks hooks;
        hooks.stream=[&](std::uint32_t handle) { const auto index=(handle-kNodes-8)/kStride;if (index>=3 || (handle-kNodes-8)%kStride) throw std::out_of_range("검색 원문 노드 범위");return std::span(bytes).subspan(28+index*kStride+8,60); }; // 같은 입력 노드의 실제60BYTE만 빌린다.
        hooks.compare=[&](std::string_view left,std::string_view right) {
            if (!trace.empty()) trace+=';';trace+="C,"+Hex(std::span(reinterpret_cast<const std::uint8_t*>(right.data()),right.size()))+','+Hex(bytes);++comparisons;
            if ((variant==1 || variant==2) && comparisons==1) Put(manager,8,kNodes+2*kStride+8);
            return variant==1 && comparisons==1 ? 0 : CompareDataManagerSectionNames(left,right);
        }; // 비교 결과는 공식 ASCII CRT 경계이며 원본 판단 본문을 대체하지 않는다.
        hooks.assertion=[&](auto kind) {
            const auto line=kind==DataManagerSelectionAssertion::MissingCurrent ? 0x68 : 0x6a;
            if (!trace.empty()) trace+=';';trace+="A,"+std::to_string(line)+','+Hex(bytes);
            if (line==0x68 || variant==3) Put(manager,8,kNodes+2*kStride+8);
        }; // 원본 관측에 명시한 진단 수리 입력만 적용한다.
        const std::array<std::string_view,3> names{"sTaTe","TERRITORY","missing"};const auto result=input[0]=="F" ? FindDataManagerSection(manager,names.at(query),hooks) : SelectDataManagerSection(manager,names.at(query),std::bit_cast<std::int32_t>(Number(input[5])),hooks);
        CHECK(result==Number(row[3]) && Hex(bytes)==row[2] && (trace.empty() ? "-" : trace)==row[4]);
    }
}
// 실제 이름 규칙과 물리 범위를 검증하고 복원되지 않은 CRT 로캘/잘못된 포인터를 성공으로 처리하지 않는다.
TEST_CASE(DataManagerSelection_PhysicalContractsAndAsciiNames) {
    CHECK(CompareDataManagerSectionNames("STATE","state")==0 && CompareDataManagerSectionNames(std::string_view("State\0ignored",13),"state")==0);
    CHECK(CompareDataManagerSectionNames("State","State0")<0 && CompareDataManagerSectionNames("Territory","State")>0);
    std::array<std::uint8_t,12> manager{};manager.fill(0xa5);const auto before=manager;bool rejected{};
    try { FindDataManagerSection(std::span(manager).first(11),"State",{}); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && manager==before);rejected=false;Put(manager,4,0);CHECK(!FindDataManagerSection(manager,"State",{}) && !Get(manager,8));
    try { SelectDataManagerSection(manager,"State",1,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && !Get(manager,8));rejected=false;
    try { CompareDataManagerSectionNames("\xc0","\xe0"); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
// 실제 Player Get이 같은 manager를 선택하고 빌린 헤더/State 원시 섹션을 함께 사용한다.
TEST_CASE(CampaignTemplates_PlayerSelectsCurrentSectionAndSharedCursor) {
    const auto names=FortTemplate::SectionNames();const auto at=static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin());
    FortTemplate fort;fort.sections.resize(at+1);fort.sections[at]={0x12,0x34,0x56};CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"selection");storage.Borrow(8,1);
    CHECK(storage.CurrentSection(1).empty());storage.State(1).position=2;std::uint32_t depth{};bool mission{}; // 실제 짧은 원문을28BYTE로 확장하는 입력이다.
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("선택 연동 예상 밖 진단"); },[](auto) {},[&](auto slot,auto size) { return storage.GrowState(slot,size); },[&](auto slot) { storage.CommitStateGrowth(slot); });
    const auto first=state.Get(8);CHECK(first.size()==28 && first[0]==0x12 && first[1]==0x34 && first[2]==0x56 && storage.CurrentSection(1)=="State" && storage.State(1).position==28);
    storage.State(8).position=17;const auto second=state.Get(1);CHECK(second.data()==first.data() && second.data()==storage.Template(8).Section("State").data() && !storage.State(8).position && storage.CurrentSection(8)=="State");
    storage.Clear(8);CHECK(storage.CurrentSection(1)=="State" && storage.OwnedImages()==1);storage.ClearAll();CHECK(!storage.OwnedImages());
}
