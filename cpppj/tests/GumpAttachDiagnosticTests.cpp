// Gump 부착의 signed GT 진단 접두와 기록 뒤 실제 트리 부착을 창 없이 검증한다.
#include "GumpRawSupport.h"
#include "client/GumpTree.h"
#include <optional>
using namespace netstorm::test::gumpraw;
namespace { // 원문 바이트와 nullable 상세를 그대로 비교한다.
std::vector<std::uint32_t> AttachDiagnosticNumbers(std::string_view text) { // signed 입력도 전체 DWORD 비트를 읽는다.
    std::vector<std::uint32_t> result; // 원문 입력 순서다.
    for (const auto& value:Split(text,',')) result.push_back(Number(value));return result; // 기대 분기나 결과를 계산하지 않는다.
}
std::string AttachDiagnosticHex(std::string_view text) { return Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())); } // 실제 저장한 상세 문자열의 ASCII 바이트다.
struct AttachDiagnosticScene { // 세 안정 노드와 실제 GumpTree만 소유하는 명시 장면이다.
    std::map<std::uint32_t,GumpCreateNode> nodes;GumpCreateState state;GumpTreeState tree;std::vector<std::string> events;std::optional<std::string> detail; // 현재 노드/전역/효과를 원문과 같은 주소 수명에 둔다.
    AttachDiagnosticScene() { state.collection.root=1;state.screenWidth=640;state.screenHeight=480;nodes[1].baseWidth=640;nodes[1].baseHeight=480;nodes[2].flags=9;nodes[2].parent=1;nodes[2].command=0;nodes[2].depth=123;nodes[3].depth=77; } // 자식은 없고 실제 부착만 검사한다.
    GumpTreeHooks Hooks() { // 계산을 모형으로 대체하지 않고 실제 가상/상세 효과만 공급한다.
        GumpTreeHooks hooks;hooks.node=[this](auto token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; };hooks.redraw=[this](auto token,bool erase) { CHECK(token==2 && !erase);events.push_back("R"); };hooks.push=[](auto) {};hooks.attach=[](auto,auto,auto,auto,auto) {};hooks.kill=[](auto) {};return hooks;
    }
};
}

//432개 원문 접두의 전체 노드/현재checking/상세와 첫 쓰기 전 도달점을 비교한다.
TEST_CASE(GumpAttachDiagnostic_NativeSignedTypeAndLimit432) {
    std::size_t count{}; // 원문 고유 입력 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_GUMP_ATTACH_DIAGNOSTIC_FIXTURE).rows) { // 기존 게임/PE를 실행하지 않고 최초 관측만 읽는다.
        CHECK(row.size()==6);const auto input=AttachDiagnosticNumbers(row[0]);CHECK(input.size()==4);std::array<std::uint8_t,160> raw{};
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*17+3); // 무관한 기본 필드도 동일한 바이트 표식으로 보존한다.
        Put(raw,32,input[0]);auto node=Decode(raw);auto checking=input[1];const auto limit=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(input[2]));std::optional<std::string> detail;std::string events;
        RecordGumpAttachDiagnostic(node,netstorm::o::RawCheckingFlag{checking},limit,[&](std::string_view text) { detail=std::string(text);events="L:"+AttachDiagnosticHex(text)+';';if (input[3]) { checking=0;node.command=2;node.flags=0x200009; } });events+='B';
        CHECK(events==row[1]);CHECK(Encode(node)==row[2]);CHECK(checking==Number(row[3]));CHECK((detail ? AttachDiagnosticHex(*detail) : "-")==row[4]);CHECK(row[5]=="body");++count;
    }
    CHECK(count==432);
}

// 상세 기록이 현재 flags/부모/GT를 바꾸면 같은 실제 Attach 몸체가 그 상태로 부착한다.
TEST_CASE(GumpAttachDiagnostic_ActualAttachUsesCurrentFieldsAfterWriter) {
    AttachDiagnosticScene scene;std::int32_t checking=std::bit_cast<std::int32_t>(0x80000000U);scene.tree.checking=netstorm::o::RawCheckingFlag{checking};auto hooks=scene.Hooks();hooks.diagnostic=[&](std::string_view text) { scene.events.push_back("D");scene.detail=std::string(text);checking=0;scene.nodes[2].command=2;scene.nodes[2].flags|=0x200000;scene.nodes[2].parent=3;scene.nodes[2].x=999; };
    GumpTree tree(scene.state,scene.tree,std::move(hooks));tree.Attach(2,3.25f,-1.5f,0,1);const auto& node=scene.nodes.at(2);
    CHECK(scene.events==std::vector<std::string>({"D","R"}) && scene.detail=="gt = 0");CHECK(checking==0 && node.command==2 && node.flags==0x200001 && node.parent==3 && node.depth==123 && node.x==3.25f && node.y==-1.5f);CHECK(scene.nodes.at(1).child==0 && scene.nodes.at(3).child==2);
}

// 미선택 writer는 요구하지 않고 기존 CD 부착에는 새10.78 GT 진단을 추가하지 않는다.
TEST_CASE(GumpAttachDiagnostic_SelectedWriterAndExistingCdBody) {
    GumpCreateNode node;node.command=0;RecordGumpAttachDiagnostic(node,netstorm::o::RawCheckingFlag{},67,{});bool rejected{};try { RecordGumpAttachDiagnostic(node,netstorm::o::RawCheckingFlag{1U},67,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    AttachDiagnosticScene scene;scene.state.collection.edition=netstorm::o::OriginalEdition::Cd1072;scene.tree.checking=1U;auto hooks=scene.Hooks();hooks.diagnostic=[](auto) { CHECK(false); };GumpTree tree(scene.state,scene.tree,std::move(hooks));tree.Attach(2,3.25f,-1.5f,0,1);CHECK(scene.events==std::vector<std::string>{"R"} && scene.nodes.at(2).parent==1 && scene.nodes.at(1).child==2);
}
