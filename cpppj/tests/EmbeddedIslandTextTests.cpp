// 보호 PlacingIslands의 원문 섹션 조회/기본 HTML 전체 출력을 같은 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "client/HtmlTextParser.h"
#include "o/Config.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 실제 원문 자료와 제품 결과의 비교만 소유한다.
// 실제 각 BYTE를 문자열로 옮기며 변환 기대값을 계산하지 않는다.
std::string IslandTextBytes(const std::string& hex) {
    if (hex.size()%2) throw std::invalid_argument("원문 섬 설명 hex 길이 오류");std::string value;
    // 원문 인코딩의 두 hex 글자마다 실제 한 BYTE를 보존한다.
    for (std::size_t i=0;i<hex.size();i+=2) value.push_back(static_cast<char>(std::stoul(hex.substr(i,2),nullptr,16)));
    return value;
}
// 실제 조회 위치/본문/전체 출력과 순회 필드를 한 판본의 독립 관측에 맞춘다.
void ReplayIslandText(const std::string& editionName,OriginalEdition edition) {
    const auto rows=LoadFixture(NETSTORM_EMBEDDED_ISLAND_TEXT_FIXTURE).rows;std::size_t count{};
    // 두 보호 판본의 입력/전역을 섞지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==14);if (row.size()!=14 || row[0]!=editionName) continue;
        ConfigRegistry registry;Config config(registry,"tell");config.Assign(IslandTextBytes(row[1]));const auto body=config.Section(IslandTextBytes(row[2]));CHECK(body.has_value());if (!body) continue;
        CHECK(*body==IslandTextBytes(row[3]));CHECK(static_cast<std::size_t>(body->data()-config.Text().data())==std::stoul(row[4]) && body->size()==std::stoul(row[5]));
        HtmlTextParserState state;state.shown=1;state.hiddenDepth=0;HtmlTextParserGlobals globals;
        CHECK(HtmlTextParser(state,globals,{},edition).Parse(*body)==IslandTextBytes(row[6]));CHECK(state.inputOffset==std::stoul(row[7]) && state.shown==std::stoul(row[8]) && state.hiddenDepth==std::stoul(row[9]));
        CHECK(state.inTag==std::stoul(row[10]) && state.textAllowed==std::stoul(row[11]) && state.afterHeading==std::stoul(row[12]) && globals.suppressParagraph==std::stoul(row[13]));++count;
    }
    CHECK(count==1);
}
}
TEST_CASE(EmbeddedIslandText_Original1078RetainsLiteralTextMarker) { ReplayIslandText("originals",OriginalEdition::Patch1078); } // 실제10.78의 리터럴 표기를 임의 제거하지 않는다.
TEST_CASE(EmbeddedIslandText_Original1072UsesItsOwnResource) { ReplayIslandText("originalCD",OriginalEdition::Cd1072); } // 표기가 없는10.72 자료/기본 HTML의 전체 출력도 별도로 보존한다.
