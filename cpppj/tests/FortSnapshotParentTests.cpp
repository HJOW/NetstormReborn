// 네 전체 Snapshot 부모의 최초 독립 PE2728관측을 실제 getter/child 순서·부분 상태와 대조한다.
#include "TestSupport.h"
#include "o/RawFortSnapshotParent.h"
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
constexpr std::uint32_t kSnapshotManager11=0x12000100U,kSnapshotManager22=0x12000200U; // 최초 원문 입력의 두 가상 manager 포인터다.
// 부모에 노출하는 ID를 전체 raw 메모리의 실제 가상 포인터로 변환한다.
std::uint32_t SnapshotPointer(std::uint32_t manager) {
    if (!manager) return 0;if (manager==11) return kSnapshotManager11;if (manager==22) return kSnapshotManager22;throw std::out_of_range("Snapshot manager ID 범위");
}
// 원문이 읽은 실제 가상 포인터만 명시 장치의 manager ID로 바꾼다.
std::uint32_t SnapshotManager(std::uint32_t pointer) {
    if (!pointer) return 0;if (pointer==kSnapshotManager11) return 11;if (pointer==kSnapshotManager22) return 22;throw std::out_of_range("Snapshot manager 포인터 범위");
}
// 마지막 빈 칸도 보존하는 최초 fixture의 입력/출력 필드 분리다.
std::vector<std::string> SnapshotFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { // 각 구분자 뒤의 원래 순서·빈 문자열을 보존한다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1;
    }
}
// Template의 실제 첫 DWORD만 낮은 BYTE부터 읽는다.
std::uint32_t SnapshotGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t result{};
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[at+index])<<(index*8);return result; // 해당 네 BYTE만 조합한다.
}
// Template의 manager DWORD만 쓰고 부가 필드는 보존한다.
void SnapshotPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 원본 네 BYTE 폭만 사용한다.
}
// 원본 CPU와 명시 callback이 끝난 전체 Template 저장소를 빠짐없이 표현한다.
std::string SnapshotHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 최초 관측의 소문자 BYTE 표현이다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; // 앞뒤 모든 BYTE도 함께 비교한다.
}
// 독립 원문의 지정 child 진입 emu_stop에 대응하는 검사 중단 표식이다.
struct SnapshotStop {};
// 최초 관측의 입력과 명시 외부 응답만 공급한다. 부모 순서/관문은 제품 함수가 결정한다.
struct SnapshotDevices {
    std::uint32_t kind{},manager{},owner{},fort{},battle{},parent{111},created{}; // 입력 kind/현재 전역과 명시 child 응답 상태다.
    std::int32_t slot{},activeSlot{}; // 최초 고정 this와 현재 부모의 실제 this다.
    std::array<std::uint8_t,240> headers{}; // -2..9의 실제12기록×20BYTE다. 부가 필드도 검증한다.
    std::map<std::uint32_t,std::uint32_t> currents{{0,90},{11,91},{22,92}}; // Create/current 변경은 명시 DataManager 장치의 응답이다.
    std::map<std::string,std::uint32_t> calls; // 각 실제 child의1기반 순번이다.
    std::string mutation,trace; // 입력 callback 효과와 부모의 전체 메모리읽기/child 순서다.
    // 최초 fixture의 입력열만 파싱하고 관측 기대열은 읽지 않는다.
    explicit SnapshotDevices(std::string_view input) {
        const auto fields=SnapshotFields(input,',');if (fields.size()!=7) throw std::runtime_error("Snapshot 부모 입력 칸 수 오류");
        kind=std::stoul(fields[0]);slot=std::stoi(fields[1]);manager=std::stoul(fields[2]);owner=std::stoul(fields[3]);fort=std::stoul(fields[4]);battle=std::stoul(fields[5]);mutation=fields[6];activeSlot=slot;
        for (std::int32_t selected=-2;selected<=9;++selected) { // 모든 관측 Template를 원문 입력과 같은 패턴으로 초기화한다.
            const auto at=static_cast<std::size_t>(selected+2)*20;
            for (std::size_t byte=0;byte<20;++byte) headers[at+byte]=static_cast<std::uint8_t>((selected+2)*13+byte); // 부가16BYTE도 최초 입력 그대로 둔다.
            SnapshotPut(headers,at,SnapshotPointer(selected%2 ? 11U : 22U));
        }
        SnapshotPut(headers,static_cast<std::size_t>(slot+2)*20,SnapshotPointer(manager));
    }
    // 실제 읽기와 child 진입을 같은 하나의 시간순 trace에 보관한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 실제 child만 별도 횟수를 세며 getter 호출은 이 계수에 섞지 않는다.
    std::string Event(std::string_view kindName,std::string text) { Record(std::move(text));return std::string(kindName)+std::to_string(++calls[std::string(kindName)]); }
    // 지정된 외부 경계의 입력 효과만 수행한다. 부모의 다음 분기·getter를 대신하지 않는다.
    void Mutate(std::string_view boundary) {
        const auto dash=mutation.rfind('-');if (dash==mutation.npos || mutation.substr(0,dash)!=boundary) return;const auto action=mutation.substr(dash+1);
        if (action.starts_with("manager")) SnapshotPut(headers,static_cast<std::size_t>(activeSlot+2)*20,SnapshotPointer(std::stoul(action.substr(7))));
        else if (action.starts_with("owner")) owner=std::stoul(action.substr(5));
        else if (action.starts_with("fort")) fort=std::stoul(action.substr(4));
        else if (action.starts_with("battle")) battle=std::stoul(action.substr(6));
        else if (action.starts_with("parent")) parent=std::stoul(action.substr(6));
        else if (action=="current999") for (auto& current:currents) current.second=999; // 실제 current 선택 변경만 공급한다.
        else if (action=="stop") throw SnapshotStop{};
    }
    // 원본의 현재 전역 읽기와 같은 위치에 getter trace를 남긴다.
    FortSnapshotParentGlobals Globals() {
        return {[this] { Record("owner:"+std::to_string(owner));return owner; },[this] { Record("fort:"+std::to_string(fort));return fort; },[this] { Record("battle:"+std::to_string(battle));return battle; }};
    }
    // 같은 this child의 명시 응답을 원본에서 받은 슬롯으로 기록한다.
    void SlotChild(std::string_view name,std::int32_t selected) { CHECK(selected==activeSlot);const auto boundary=Event(name,std::string(name)+':'+std::to_string(selected));Mutate(boundary); }
    // 실제 native CALL의 인자/호출 규약에 맞춘 외부 장치만 구성한다.
    FortSnapshotParentHooks Hooks() {
        FortSnapshotParentHooks hooks;
        hooks.manager=[this](std::int32_t selected) { if (selected<-2 || selected>9) throw std::out_of_range("Snapshot 명시 Template 물리 범위");activeSlot=selected;const auto result=SnapshotManager(SnapshotGet(headers,static_cast<std::size_t>(selected+2)*20));Record("manager:"+std::to_string(selected)+':'+std::to_string(result));return result; };
        hooks.assertion=[this](auto reason) { const auto boundary=Event("assert","assert:"+std::to_string(static_cast<std::uint32_t>(reason)));Mutate(boundary); };
        hooks.technologyParent=[this] { const auto result=parent;const auto boundary=Event("parent","parent:"+std::to_string(result));Mutate(boundary);return mutation==boundary+"-return0" ? 0U : result; };
        hooks.create=[this](auto selected,std::string_view name) { const auto token=++created;const auto boundary=Event("create","create:"+std::to_string(selected)+':'+std::string(name)+':'+std::to_string(token));currents.at(selected)=token;Mutate(boundary);return mutation==boundary+"-return0" ? 0U : token; };
        hooks.technology=[this](auto selected,auto stream) { const auto boundary=Event("technology","technology:"+std::to_string(selected)+':'+std::to_string(stream));Mutate(boundary); };
        hooks.money=[this](auto stream) { const auto boundary=Event("money","money:"+std::to_string(stream));Mutate(boundary); };
        hooks.mission=[this](auto stream) { const auto boundary=Event("mission","mission:"+std::to_string(stream));Mutate(boundary); };
        hooks.typeNames=[this](auto selected) { SlotChild("names",selected); };hooks.deck=[this](auto selected) { SlotChild("deck",selected); };
        hooks.objects=[this](auto selected) { SlotChild("objects",selected); };hooks.territory=[this](auto selected) { SlotChild("territory",selected); };
        hooks.badges=[this](auto selected) { SlotChild("badges",selected); };hooks.coreData=[this](auto selected) { SlotChild("core",selected); };hooks.notify=[this](auto selected) { SlotChild("notify",selected); };return hooks;
    }
    // 입력 kind에 따라 제품 네 진입 중 하나만 실행한다.
    void Run(FortSnapshotParentHooks hooks) {
        const auto globals=Globals();switch (kind) {
        case 0:SnapshotFortTemplate(slot,globals,hooks);break;case 1:SnapshotBattleTemplate(slot,globals,hooks);break;
        case 2:SnapshotCurrentTemplate(globals,hooks);break;case 3:SnapshotCurrentTemplateIfPresent(globals,hooks);break;
        default:throw std::out_of_range("Snapshot 부모 kind 범위");
        }
    }
    // 전체 현재 전역8DWORD를 최초 원문과 같은 순서로 표현한다.
    std::string State() const {
        return std::to_string(owner)+','+std::to_string(fort)+','+std::to_string(battle)+','+std::to_string(parent)+','+std::to_string(created)+','+std::to_string(currents.at(0))+','+std::to_string(currents.at(11))+','+std::to_string(currents.at(22));
    }
};
}
// 최초1364입력×두정밀도의정상/부분중단과getter순서·전체메모리를비교한다.
TEST_CASE(FortSnapshotParent_IndependentNative2728) {
    std::ifstream input(NETSTORM_FORT_SNAPSHOT_PARENT_FIXTURE);if (!input) throw std::runtime_error("전체 Snapshot 부모 최초 x86 fixture 없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique;
    while (std::getline(input,line)) { // 관측열은 제품 실행을 마친 뒤에만 비교에 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=SnapshotFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Snapshot 부모 fixture 칸 수 오류");CHECK(unique.emplace(row[0],row[1]).second);
        SnapshotDevices devices(row[1]);bool completed=true;try { devices.Run(devices.Hooks()); }catch (const SnapshotStop&) { completed=false;++stopped; }
        CHECK(completed==(row[2]=="1"));CHECK(devices.State()==row[3]);CHECK(SnapshotHex(devices.headers)==row[4]);CHECK(devices.trace==row[5]);++compared;
    }
    CHECK(compared==2728 && stopped==378);
}
// 각 부모의 미선택 child는 요구하지 않고 도달한 누락 child는 앞선 실제 효과 뒤에 실패한다.
TEST_CASE(FortSnapshotParent_SelectedAndMissingDevices) {
    SnapshotDevices fort("0,1,11,1,1,1,none");auto hooks=fort.Hooks();hooks.coreData={};fort.Run(hooks);CHECK(fort.calls["badges"]==1 && fort.calls["core"]==0);
    SnapshotDevices battle("1,1,11,1,1,1,none");hooks=battle.Hooks();hooks.typeNames={};hooks.badges={};battle.Run(hooks);CHECK(battle.calls["names"]==0 && battle.calls["badges"]==0 && battle.calls["core"]==1);
    SnapshotDevices missing("0,1,11,1,1,1,none");hooks=missing.Hooks();hooks.create={};bool rejected{};try { missing.Run(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.calls["names"]==1 && missing.calls["parent"]==1 && missing.created==0);
    SnapshotDevices absent("3,1,0,1,1,1,none");hooks={};hooks.manager=absent.Hooks().manager;absent.Run(hooks);CHECK(absent.trace=="owner:1;manager:1:0");
}
