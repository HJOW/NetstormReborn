// 원본 CheatAllTechnology128바이트의 최초 독립1200입력과 현재 getter/contained 호출 순서를 대조한다.
#include "TestSupport.h"
#include "o/RawThemeTechnology.h"
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// 마지막 빈 칸도 보존하는 fixture 구분자 읽기다.
std::vector<std::string> ThemeSplit(const std::string& text,char separator) {
    std::vector<std::string> result;std::size_t begin=0;
    // 남은 마지막 칸을 추가할 때까지 순서대로 읽는다.
    for (;;) { const auto end=text.find(separator,begin);result.push_back(text.substr(begin,end==std::string::npos ? end : end-begin));if (end==std::string::npos) return result;begin=end+1; }
}
// 선택된 독립 TYPE 입력만 만든다. 지급될 타입 목록은 여기서 계산하지 않는다.
RiftTypeRecord ThemeInput(std::int32_t index,std::int32_t variant) {
    RiftTypeRecord record;
    if (variant==0) { record.group=index%5==0 ? 10 : index%6;record.theme=index%8-1; }
    else if (variant==1) { record.group=index%7==6 ? 10 : index*3%11;record.theme=(index+3)%8-1; }
    else { constexpr std::array<std::int32_t,8> themes{0,1,2,3,4,5,-1,6}; // 독립 TYPE theme DWORD의 입력 배열이다.
        record.group=index%4==2 ? 10 : index%3;record.theme=themes[static_cast<std::size_t>(index%8)]; }
    return record;
}
// 명시 입력과 외부 getter/contained 반응만 공급한다. 본체 판단·기대 trace는 fixture에서만 읽는다.
struct ThemeScene {
    std::array<std::int32_t,6> input{}; // theme/count/variant/pattern/mutation/countMode의 독립 입력이다.
    std::array<RiftTypeRecord,32> types{}; // 번호0을 포함하는 명시 기록이다.
    std::map<std::pair<std::uint16_t,std::uint32_t>,Sid> contained; // 하위 Find가 받는 입력과 Create 후 현재 존재 상태다.
    std::int32_t count{}; // callback이 수정할 수 있는 현재 signed 전역이다.
    std::uint16_t parent{50}; // getter가 매번 반환하는 현재 기술 부모다.
    bool countChanged{}; // 지정 callback의 count 변경을 한 번만 수행한다.
    std::string trace; // 실제 경계 요청과 반환 순서를 기록한다.
    std::vector<std::array<std::uint32_t,3>> born; // 실제 Create 경계가 받은 부모/type 및 반환 SID다.
    // fixture 기대 출력 없이 초기 기록·개수·현재 부모와 존재 여부만 준비한다.
    explicit ThemeScene(const std::string& text) {
        const auto values=ThemeSplit(text,',');if (values.size()!=input.size()) throw std::runtime_error("테마 기술 입력 칸 수 오류");
        // 원래 signed DWORD 표현을 그대로 사용한다.
        for (std::size_t index=0;index<input.size();++index) input[index]=std::stoi(values[index]);count=input[1];
        // 독립 타입32개는 처음부터 두 필드의 입력만 가지고 있다.
        for (std::int32_t index=0;index<32;++index) types[static_cast<std::size_t>(index)]=ThemeInput(index,input[2]);
        // 명시 하위 존재 여부는 본체의 필터와 무관하게 모든 부모/타입에서 준비한다.
        for (std::uint16_t owner=50;owner<53;++owner) {
            // 각 부모의 모든 타입에 초기 Find 결과를 독립 공급한다.
            for (std::uint32_t type=0;type<32;++type)
                if (input[3] && (owner+type)%3==0) contained[{owner,type}]=Sid{static_cast<std::uint16_t>(1000+owner*32+type)};
        }
    }
    // 관측한 한 경계만 순서대로 추가한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 현재 부모를 세 명시 부모 사이에서 한 칸 전진한다.
    void AdvanceParent() { parent=static_cast<std::uint16_t>(50+(parent-49)%3); }
    // 정해진 하위 경계에서만 현재 count를 수정한다.
    void ChangeCount(std::string_view boundary,std::int32_t type) {
        if (countChanged) return;
        if (input[5]==1 && boundary=="find") count=type+1;
        else if ((input[5]==2 && boundary=="create") || (input[5]==3 && boundary=="record")) count=12;
        else return;countChanged=true;
    }
    // 제품의 실제 제어 흐름에 연결할 다섯 명시 경계를 구성한다.
    ThemeTechnologyHooks Hooks() {
        ThemeTechnologyHooks hooks;
        // 실제 현재 count 읽기를 기록하며 변경된 전역을 그대로 반환한다.
        hooks.typeCount=[this] { Add("N,"+std::to_string(count));return count; };
        // 실제 번호의 현재 두 DWORD를 반환하고 지정된 외부 입력 변경을 수행한다.
        hooks.typeAt=[this](std::int32_t type)->const RiftTypeRecord& { const auto& record=types.at(static_cast<std::size_t>(type));Add("R,"+std::to_string(type)+','+std::to_string(record.group)+','+std::to_string(record.theme));ChangeCount("record",type);return record; };
        // 현재 부모를 캡처해 반환한 다음 지정된 getter 효과로 다음 현재 값을 바꾼다.
        hooks.currentTechnology=[this] { const Sid current{parent};Add("G,"+std::to_string(parent));if (input[4]==4) AdvanceParent();return current; };
        // 받은 부모와 모든 Find 인자를 기록한 뒤 명시 존재 여부를 반환한다.
        hooks.findContained=[this](Sid owner,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { const auto found=contained.find({owner.value,type});const Sid result=found==contained.end() ? Sid{} : found->second;Add("F,"+std::to_string(owner.value)+','+std::to_string(mask)+','+std::to_string(type)+','+std::to_string(index)+','+std::to_string(result.value));ChangeCount("find",static_cast<std::int32_t>(type));if (input[4]==1 || input[4]==3) AdvanceParent();return result; };
        // 받은 부모와 모든 Create 인자를 보존하고 실제 요청 순서의 새 SID를 반환한다.
        hooks.createContained=[this](Sid owner,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { const Sid result{static_cast<std::uint16_t>(700+born.size())};Add("C,"+std::to_string(owner.value)+','+std::to_string(type)+','+std::to_string(argument)+','+std::to_string(flags)+','+std::to_string(attach)+','+std::to_string(quality)+','+std::to_string(result.value));born.push_back({owner.value,type,result.value});contained[{owner.value,type}]=result;ChangeCount("create",static_cast<std::int32_t>(type));if (input[4]==2 || input[4]==3) AdvanceParent();return result; };
        return hooks;
    }
    // 경계에서 실제 생성된 기록만 원본과 같은 형식으로 내보낸다.
    std::string Born() const {
        std::string result;
        // 새 SID의 관측 순서를 변경하지 않는다.
        for (const auto& row:born) { if (!result.empty()) result+=';';result+=std::to_string(row[0])+','+std::to_string(row[1])+','+std::to_string(row[2]); }return result.empty() ? "-" : result;
    }
};
}
// 실제 원본1200개 고유 입력의 전체 호출 순서와 현재 count/부모 및 생성 기록을 대조한다.
TEST_CASE(ThemeTechnology_Native1200) {
    std::ifstream stream(NETSTORM_THEME_TECHNOLOGY_FIXTURE);if (!stream) throw std::runtime_error("테마 기술 최초 x86 fixture 없음");
    std::string line;std::set<std::string> unique;std::size_t count=0;
    // 기대 출력은 제품 Apply를 완료한 뒤에만 비교에 사용한다.
    while (std::getline(stream,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto fields=ThemeSplit(line,'\t');if (fields.size()!=5) throw std::runtime_error("테마 기술 fixture 칸 수 오류");CHECK(unique.insert(fields[0]).second);
        ThemeScene scene(fields[0]);RawThemeTechnology technology(scene.Hooks());technology.Apply(scene.input[0]);
        CHECK(scene.trace==fields[1]);CHECK(scene.count==std::stoi(fields[2]));CHECK(scene.parent==std::stoi(fields[3]));CHECK(scene.Born()==fields[4]);++count;
    }
    CHECK(count==1200);
}
// 실제 하위 경계를 누락했을 때 아무 동작도 하지 않은 성공으로 숨기지 않는다.
TEST_CASE(ThemeTechnology_RequiresEveryBoundary) {
    // 다섯 경계 각각의 누락을 따로 검증한다.
    for (std::size_t index=0;index<5;++index) {
        ThemeScene scene("0,1,2,0,0,0");auto hooks=scene.Hooks();
        if (index==0) hooks.typeCount={};else if (index==1) hooks.typeAt={};else if (index==2) hooks.currentTechnology={};else if (index==3) hooks.findContained={};else hooks.createContained={};
        bool rejected=false;try { RawThemeTechnology technology(std::move(hooks)); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    }
}
