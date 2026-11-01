// 제품 변경 전에 동결한 원본004c8df0/strchr의 전체 노드/외부 효과 순서를 대조한다.
#include "ConstructionSupport.h"
#include "client/MoveDialog.h"
#include <sstream>
#include <iomanip>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
std::vector<std::uint8_t> MoveBytes(const std::string& text) {
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 원문 입력/기대 바이트만 해독한다.
    return result;
} // NUL/공백을 포함하는 인자를 탭 구분자와 분리한다.
}
TEST_CASE(MoveDialog_Original73InputsAndPushOrder) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_MOVE_DIALOG_FIXTURE).rows) { // 문자열·화면·창치수·현재포인터변경을독립원문과대조한다.
        CHECK(row.size()==4);if (row.size()!=4) continue;
        const auto values=Inputs(row[1]);const auto expected=MoveBytes(row[2]);CHECK(values.size()==8 && expected.size()==0xa0);
        const auto text=MoveBytes(row[0]=="-" ? "" : row[0]);std::string location(text.begin(),text.end());
        int width=std::bit_cast<std::int32_t>(values[1]),height=std::bit_cast<std::int32_t>(values[2]);
        GumpCreateNode node,other;node.width=std::bit_cast<float>(values[3]);node.height=std::bit_cast<float>(values[4]);node.x=std::bit_cast<float>(values[5]);node.y=std::bit_cast<float>(values[6]);
        node.flags=0x71717171U;node.depth=0x7171;other.x=999;auto* current=values[0] ? &node : nullptr;std::string events;
        MoveDialogHooks hooks;hooks.push=[&] { events="P";if (values[7]) { current=&other;width=1920;height=1080;node.width=123.5F;node.height=222.25F;node.x=-10.5F;node.y=12.25F; } };
        hooks.attach=[&](float x,float y,auto parent,auto mode) { std::ostringstream output;output<<";A:"<<std::hex<<std::setfill('0')<<std::setw(8)<<Bits(x)<<','<<std::setw(8)<<Bits(y)<<','<<std::setw(8)<<parent<<','<<std::setw(8)<<mode;events+=output.str(); };
        MoveDialog(current,location,width,height,hooks);
        CHECK(Bits(node.width)==Get(expected,0x3c) && Bits(node.height)==Get(expected,0x40));CHECK(Bits(node.x)==Get(expected,0x4c) && Bits(node.y)==Get(expected,0x50));
        CHECK(node.flags==0x71717171U && node.depth==0x7171 && other.x==999);CHECK((events.empty() ? "-" : events)==row[3]);++count;
    }
    CHECK(count==73);
} // 원문 기대값은 재생성하지 않고 동일 float 비트/호출 순서를 비교한다.
