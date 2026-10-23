// 실제10.78 ChatWindow 기하 관측을 같은 노드/명시 가상 경계로 재생한다. 전체 생성/Console 검사가 아니다.
#include "TestSupport.h"
#include "client/HudChatGeometry.h"
#include <array>
#include <bit>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::size_t kColumns=20,kNativeRows=3960; // 독립1,980입력의 두 x87 정밀도 관측 수다.
using ChatRow=std::array<std::string,kColumns>; // 앞14열 입력/뒤6열 실제 관측을 보존한다.

// 원문 끝 빈 필드를 지우지 않고 TSV/레코드를 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{}; // 순서대로 복사할 필드와 시작 위치다.
    for (;;) { // 현재 구분자까지의 원문 필드만 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return result;begin=end+1;
    }
}
// 실제 DWORD 출력과 단정도 비트는 signed 해석 없이 읽는다.
std::uint32_t Number(std::string_view value) { return static_cast<std::uint32_t>(std::stoull(std::string(value))); }
// 실제 콜백/상태의 지정 인자만 직렬화한다. 기하 기대값은 계산하지 않는다.
std::string Packed(std::initializer_list<std::uint32_t> values) {
    std::string result; // 실제 인자 순서의 목록이다.
    for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(value); } // 원문의 DWORD 표현을 그대로 쓴다.
    return result;
}
// 실제4ba3d0 내부 Redraw는 제품 기하 모듈의 외부 요청 경계 안쪽이므로 직접 trace 비교에서 제외한다.
std::string Observable(std::string_view native) {
    if (native=="-") return {};std::string result; // 공개된 가상/텍스트 요청의 실제 순서다.
    for (const auto& item:Split(native,';')) { // 나머지 관측은 수정하지 않고 같은 순서로 유지한다.
        if (item.starts_with("redraw:")) continue;if (!result.empty()) result+=';';result+=item;
    }
    return result;
}
// 독립 원문과 동일한 다섯 노드를 공급하고 최종 공통 트리/텍스트 조회를 명시 경계로 기록한다.
struct ChatScene {
    HudChatNode root; // 기하 모듈과 기본 노드 조회가 공유하는 실제 root다.
    std::array<GumpCreateNode,4> children; // Console/Text/helper/background 순서의 같은 저장소다.
    std::uint32_t bottom{},challenge{}; // oracle의 명시 텍스트 조회/현재 모드 입력이다.
    std::string trace; // 실제 호출 순서를 기록하며 계산 기대값을 만들지 않는다.
    HudChatGeometry geometry{Hooks()}; // 같은 root/자식 주소를 쓰는 제품 본체다.

    // 원문 포인터 정규화1..5와 같은 검사 토큰을 반환한다.
    GumpCreateNode* Node(std::uint32_t token) { return token==1 ? &root : token>=2 && token<=5 ? &children[token-2] : nullptr; }
    // 최종 가상 경계는 native 관측기처럼 링크/좌표를 쓰지 않는다.
    void Append(std::string item) { if (!trace.empty()) trace+=';';trace+=item; }
    // 실제 모듈이 전달한 인자와 호출 순서만 관측한다.
    HudChatGeometryHooks Hooks() {
        return {[this](auto token) { return token==1 ? &root : nullptr; },[this](auto token) { return Node(token); },
            [this](auto token,float x,float y,auto parent,auto mode) { Append("baseAttach:"+Packed({token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode})); },
            [this](auto token) { Append("push:"+std::to_string(token)); },
            [this](auto token,float x,float y,auto parent,auto mode) { Append("attach:"+Packed({token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode})); },
            [this](auto token) { Append("bottomIndex:"+std::to_string(token));return bottom; },
            [this](auto token,auto extra) { Append("bottom:"+Packed({token,extra})); },
            [this] { Append("challenge:"+std::to_string(challenge));return challenge; }};
    }
    // 앞14열 입력만 상태에 제공한다. 기대 기하/호출 순서는 뒤 관측 열에서만 읽는다.
    explicit ChatScene(const ChatRow& row) {
        root.console=2;root.text=3;root.helper=4;root.background=5;root.leftPadding=static_cast<std::int32_t>(std::stoll(row[4]));root.helperVisible=static_cast<std::uint8_t>(Number(row[5]));
        const auto scales=Split(row[7],':'),positions=Split(row[9],':'); // 네 자식 배율과 현재 위치/전달 인자는 별도 입력이다.
        if (scales.size()!=4 || positions.size()!=6) throw std::invalid_argument("ChatWindow fixture 기하 입력 개수");
        for (std::uint32_t token=1;token<=5;++token) { // 실제 초기값은 oracle과 같은 명시 표식이며 출력 수식을 쓰지 않는다.
            auto& node=*Node(token);node.flags=token==1 ? 0xe01U : token==2 ? Number(row[6]) : 0x601U;
            node.scale=token==1 ? 1.0F : std::bit_cast<float>(Number(scales[token-2]));
            node.baseWidth=token==1 ? std::bit_cast<float>(Number(row[2])) : std::array<float,4>{60,40,1,20}[token-2];
            node.baseHeight=token==1 ? std::bit_cast<float>(Number(row[3])) : token==2 ? std::bit_cast<float>(Number(row[8])) : std::array<float,4>{0,50,9,30}[token-2];
            node.width=123;node.height=-456;
        }
        root.x=std::bit_cast<float>(Number(positions[0]));root.y=std::bit_cast<float>(Number(positions[1]));
        children[0].x=std::bit_cast<float>(Number(positions[2]));children[0].y=std::bit_cast<float>(Number(positions[3]));bottom=Number(row[13]);challenge=Number(row[11]);
    }
    // 제품이 실제로 기록한 다섯 노드의 크기/표시만 읽는다.
    std::string State() {
        std::string result; // native state와 같은 순서의 원시 출력이다.
        for (std::uint32_t token=1;token<=5;++token) { // untouched Console 높이와 root 크기도 빠뜨리지 않는다.
            const auto& node=*Node(token);if (!result.empty()) result+=';';
            result+=Packed({token,node.flags,std::bit_cast<std::uint32_t>(node.baseWidth),std::bit_cast<std::uint32_t>(node.baseHeight),std::bit_cast<std::uint32_t>(node.width),std::bit_cast<std::uint32_t>(node.height)});
        }
        return result;
    }
};
}

// native 전체 진입/RET 증거와 기하 제품의 공개 경계 직접 대조를 구별한다.
TEST_CASE(HudChatGeometry_Native3960SizesAndCallbacks) {
    std::ifstream input(NETSTORM_HUD_CHAT_GEOMETRY_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{}; // 두 정밀도 관측의 전체 재생 수다.
    while (std::getline(input,line)) { // 원문 관측의 순서/필드를 변경하지 않는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;
        const auto fields=Split(line,'\t');if (fields[0]=="name") continue;CHECK(fields.size()==kColumns);if (fields.size()!=kColumns) continue;
        ChatRow row;for (std::size_t index=0;index<row.size();++index) row[index]=fields[index]; // 독립 입력/관측 열을 그대로 보존한다.
        CHECK(row[14]=="1");CHECK(row[18]=="0");CHECK(Number(row[19])>0);ChatScene scene(row);
        if (row[1]=="attach") { const auto positions=Split(row[9],':');scene.geometry.Attach(1,std::bit_cast<float>(Number(positions[4])),std::bit_cast<float>(Number(positions[5])),17,3); }
        else if (row[1]=="helper") scene.geometry.SetHelperVisible(1,Number(row[10]));
        else { CHECK(row[1]=="input");scene.geometry.SetInputVisible(Number(row[12]) ? 1U : 0U,Number(row[10])); }
        CHECK(scene.State()==row[15]);CHECK(scene.root.helperVisible==Number(row[16]));CHECK(scene.trace==Observable(row[17]));++count;
    }
    CHECK(count==kNativeRows);
}
