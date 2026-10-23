// 독립10.78 divider 입력/방송 관측의 같은 raw 입력과 상태/장치/최종 부착 인자를 대조한다.
#include "TestSupport.h"
#include "client/HudDividerInput.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::size_t kColumns=20,kNativeRows=13398; // 독립6,699입력의 두 정밀도 관측 수다.
constexpr std::uint32_t kDivider=1,kParent=2,kGrandparent=3,kOther=4; // native 포인터와 구별하는 안정된 검사 토큰이다.
using DividerRow=std::array<std::string,kColumns>; // 앞14열 입력과 뒤6열 원문 관측이다.

// 빈 마지막 항목까지 보존해 TSV와 콜백 레코드의 원문 경계를 유지한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{}; // 원문 필드의 시작 위치와 복사 목록이다.
    for (;;) { // 현재 구분자까지의 필드만 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return result;begin=end+1;
    }
}
// 전체 DWORD 비트/큰 양수는 unsigned로 읽으며 signed 화면 입력은 별도 함수에서 읽는다.
std::uint32_t Unsigned(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); }
// 부호 있는 제한/마우스 좌표의 원시 signed32 값을 유지한다.
std::int32_t Signed(std::string_view text) { return static_cast<std::int32_t>(std::stoll(std::string(text))); }
// 실제 콜백 인자만 구분자 목록으로 기록한다. 이동/분기 기대값은 만들지 않는다.
std::string Packed(std::initializer_list<std::uint32_t> values) {
    std::string result; // 실제 관측의 숫자 목록이다.
    for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(value); } // 인자 순서대로만 붙인다.
    return result;
}
// 제품 공개 경계에서 직접 관측할 수 없는 native 내부 함수 진입 표식만 제외한다.
std::string ObservableTrace(std::string_view text) {
    std::string result; // 외부 효과와 최종 가상 Push/Attach의 실제 순서다.
    if (text=="-") return result;
    for (const auto& item:Split(text,';')) { // raw local/anchor/capture/move는 별도 상태 및 최종 부착으로 대조한다.
        if (item=="local" || item.starts_with("localout:") || item.starts_with("anchor:") || item.starts_with("capture:") || item.starts_with("move:")) continue;
        if (!result.empty()) result+=';';result+=item;
    }
    return result;
}
// 기대 수식을 계산하지 않고 같은 공통 노드/위치/입력/이동에 입력을 재생한다.
struct DividerScene {
    HudDividerNode divider; // 제품과 공통 Gump가 동시에 읽는 같은 파생 노드다.
    GumpCreateNode parent,grandparent,other; // native 일반 부모 체인과 기존 capture에 대응한다.
    GumpCreateState create; // Collection/Tree/Events/Input이 공유하는 등록 상태다.
    GumpLifetimeState lifetime; // 실제 공통 focus를 검사한다.
    GumpEventState eventsState; // 이 검사에서는 필터/선택을 실행하지 않는다.
    GumpInputState inputState; // 실제 Capture만 재사용하며 Route는 별도 범위다.
    GumpTreeState treeState; // 최종 attach는 native와 동일한 관측 경계다.
    std::uint32_t menuCount{},pollCode{},baseResult{}; // native에 공급한 장치/메뉴 입력이다.
    std::string trace; // 공개 효과와 최종 가상 호출 순서를 기록한다.
    GumpCollection collection{create.collection,{[this](auto token)->GumpCollectionNode* { return Base(token); },{}}}; // 실제 부모별 float32 로컬 변환이다.
    GumpEvents events{create,lifetime,eventsState,collection,{[this](auto token) { return Base(token); },{},{}}}; // 공통 입력의 같은 의존성이다.
    GumpInput input{create,lifetime,inputState,collection,events,{[this](auto token) { return Base(token); },{},{}}}; // 실제 첫 캡처만 유지하는 처리기다.
    GumpTree tree{create,treeState,TreeHooks()}; // 실제 anchor 차감과 가상 Push/Attach 순서다.
    HudDividerInput handler{collection,input,tree,InputHooks()}; // 검사 전용 수식/별도 capture를 쓰지 않는다.

    // 같은 네 노드의 기본 주소를 공급한다.
    GumpCreateNode* Base(std::uint32_t token) {
        switch (token) { case kDivider:return &divider;case kParent:return &parent;case kGrandparent:return &grandparent;case kOther:return &other;default:return nullptr; }
    }
    // 원문의 stub과 동일하게 최종 링크/좌표를 바꾸지 않고 실제 콜백 인자만 읽는다.
    GumpTreeHooks TreeHooks() {
        return {[this](auto token) { return Base(token); },[](auto,auto) {},[this](auto) { Append("detach"); },
            [this](auto,float x,float y,auto destination,auto mode) { Append("attach:"+Packed({std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),destination,mode})); },
            [](auto) {},{}};
    }
    // 실제 폴링/기본 Help/커서/현재 메뉴 수의 경계만 기록한다.
    HudDividerInputHooks InputHooks() {
        return {[this](auto token) { return token==kDivider ? &divider : nullptr; },[this](auto token) { return Base(token); },
            [this](auto,auto event,auto first,auto second) { Append("base:"+Packed({event,first,second}));return baseResult; },
            [this](auto cursor) { Append("cursor:"+std::to_string(cursor)); },
            [this] { Append("modal:"+std::to_string(menuCount));return menuCount; },
            [this](auto code) { Append("poll:"+std::to_string(code));return InputEvent{pollCode,0x11223344,0x55667788}; },0x0200003b};
    }
    // 실제 콜백을 실행한 순서만 기록한다.
    void Append(std::string item) { if (!trace.empty()) trace+=';';trace+=item; }
    // 앞14열만 입력으로 사용한다. 뒤6열의 관측을 상태 준비에 사용하지 않는다.
    explicit DividerScene(const DividerRow& row) {
        divider.axis=Unsigned(row[5]);divider.flags=Unsigned(row[6]);
        const auto limits=Split(row[7],':'),geometry=Split(row[8],':'),parents=Split(row[9],':'); // 원문 명시 입력 목록이다.
        if (limits.size()!=4 || geometry.size()!=4 || parents.size()!=3) throw std::invalid_argument("Divider fixture 입력 열 개수");
        divider.minX=Signed(limits[0]);divider.maxX=Signed(limits[1]);divider.minY=Signed(limits[2]);divider.maxY=Signed(limits[3]);
        divider.x=std::bit_cast<float>(Unsigned(geometry[0]));divider.y=std::bit_cast<float>(Unsigned(geometry[1]));
        divider.positioning[0]=Unsigned(geometry[2]);divider.positioning[1]=Unsigned(geometry[3]);
        const auto depth=Unsigned(parents[0]); // native의 일반 부모 체인 길이다.
        divider.parent=depth ? kParent : 0U;parent.parent=depth==2 ? kGrandparent : 0U;
        parent.x=std::bit_cast<float>(Unsigned(parents[1]));parent.y=std::bit_cast<float>(Unsigned(parents[2]));
        lifetime.focus=Unsigned(row[10])==1 ? kDivider : Unsigned(row[10])==2 ? kOther : 0U;
        pollCode=Unsigned(row[11]);menuCount=Unsigned(row[12]);baseResult=Unsigned(row[13]);
    }
    // 제품의 실제 flags/anchor/focus를 native의0/self/other 표현으로 읽는다.
    std::string State() const {
        const auto focus=lifetime.focus==kDivider ? 1U : lifetime.focus==kOther ? 2U : 0U; // 현재 공유 focus의 정규화 표현이다.
        return Packed({divider.flags,divider.positioning[0],divider.positioning[1],focus});
    }
};
}

// native 정상RET/명령 수는 도구의 범위 증거다. 제품은 상태와 공개 효과/최종 부착을 직접 대조한다.
TEST_CASE(HudDividerInput_Native13398StateAndCallbacks) {
    std::ifstream input(NETSTORM_HUD_DIVIDER_INPUT_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 관측 누락을 막는 실제 재생 행 수다.
    while (std::getline(input,line)) { // 각 정밀도별 관측을 빠뜨리지 않고 그대로 재생한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;
        const auto fields=Split(line,'\t');if (fields[0]=="name") continue;CHECK(fields.size()==kColumns);if (fields.size()!=kColumns) continue;
        DividerRow row;for (std::size_t index=0;index<row.size();++index) row[index]=fields[index]; // 원문 입력/관측을 변경하지 않는다.
        CHECK(row[14]=="1");CHECK(row[18]=="0");CHECK(Unsigned(row[19])>0);
        DividerScene scene(row);const auto event=Unsigned(row[2]),first=static_cast<std::uint32_t>(Signed(row[3])),second=static_cast<std::uint32_t>(Signed(row[4])); // 원시 사건과 signed 화면 좌표다.
        if (row[1]=="handle") CHECK(scene.handler.Handle(kDivider,event,first,second)==Unsigned(row[15]));
        else { CHECK(row[1]=="broadcast");scene.handler.Broadcast(kDivider,event,first,second); }
        CHECK(scene.State()==row[16]);CHECK(scene.trace==ObservableTrace(row[17]));++count;
    }
    CHECK(count==kNativeRows);
}
