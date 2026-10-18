// 새 C++ 전에 동결한 실제 요청985행의 전체 호출/현재 필드와 원문 보존을 대조한다.
#include "GumpRawSupport.h"
#include "RawSceneSupport.h"
#include "client/QuickHelpRequest.h"
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
// 명시 조회/창·장치 효과만 공급하며 주제 선택·부착 판단은 제품 코드에 맡긴다.
struct RequestScene {
    GumpCreateNode node;std::array<std::int32_t,3> results{};std::size_t lookup{};std::uint32_t mode{};std::string trace; // 원문 입력과 현재 외부 관측이다.
    // 같은 native 입력으로 현재 플래그/좌표를 준비한다.
    explicit RequestScene(const std::vector<std::string>& input) {
        // 세 signed 반환은 원본 DWORD의 음수 여부를 보존한다.
        for (std::size_t i=0;i<results.size();++i) results[i]=static_cast<std::int32_t>(std::stol(input[i]));
        mode=Number(input[4]);node.flags=mode==1 || mode==3 ? 8U : 0U;node.x=Float(0x41880000);node.y=Float(0xc1100000);
    }
    // 문자열을 바꾸지 않고 실제 순서만 기록한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 주소 대신 원문 바이트16진수로 외부 효과를 직렬화한다.
    static std::string Text(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
    // 현재 창 변화와 가상 Push 뒤 위치 재조회를 명시 외부 입력으로 교차한다.
    QuickHelpRequestHooks Hooks() {
        QuickHelpRequestHooks h;
        h.print=[this](std::string_view pattern,std::string_view value) { Event("Format,"+Text(pattern)+','+Text(value));std::string out(pattern);const auto pos=out.find("%s");CHECK(pos!=std::string::npos);return out.replace(pos,2,value); };
        h.lookup=[this](std::string_view name) { CHECK(lookup<results.size());const auto result=results.at(lookup++);Event("Lookup,"+Text(name)+','+std::to_string(std::bit_cast<std::uint32_t>(result)));return result; };
        h.history=[this](std::string_view name) { Event("History,"+Text(name)); };
        h.show=[this](auto kind,std::string_view name) { Event("Show,"+std::to_string(kind)+','+Text(name));if (mode==2) node.flags=8;else if (mode==3) node.flags=0; };
        h.push=[this] { Event("Push");if (mode==4) { node.flags=0x800;node.x=Float(0x80000000);node.y=Float(0x7fc12345); } };
        h.attach=[this](float x,float y,auto parent,auto flags) { Event("Attach,"+std::to_string(Bits(x))+','+std::to_string(Bits(y))+','+std::to_string(parent)+','+std::to_string(flags)); };return h;
    }
};
}
// 실제 C 로케일 CRT/분기985입력 전체를 독립 원문과 비교한다.
TEST_CASE(QuickHelpRequest_Native985) {
    std::size_t count{}; // 실제 비교한 동결 행 수다.
    // 모든 입력/결과를 읽기만 하며 기대값을 제품 계산으로 만들지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_QUICK_HELP_REQUEST_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;const auto input=Split(row[1],',');const auto bytes=Bytes(row[0]);const std::string topic(bytes.begin(),bytes.end());RequestScene scene(input);QuickHelpRequest request(scene.node,scene.Hooks());
        request.Open(Number(input[3]),topic);scene.trace+='|'+std::to_string(scene.node.flags)+','+std::to_string(Bits(scene.node.x))+','+std::to_string(Bits(scene.node.y));
        if (scene.trace!=row[2]) std::printf("Quick Help 요청 원문 차이 %zu %s\n%s\n",count,row[1].c_str(),scene.trace.c_str());CHECK(scene.trace==row[2]);CHECK(std::vector<std::uint8_t>(topic.begin(),topic.end())==bytes);++count;
    }
    CHECK(count==985);
}
// 원본의 버퍼 밖 읽기/쓰기를 정상 요청으로 만들지 않고 어떤 외부 효과보다 먼저 거부한다.
TEST_CASE(QuickHelpRequest_RejectsUndefinedTopicBufferAccess) {
    GumpCreateNode node;QuickHelpRequest request(node,{}); // 금지 입력에는 외부 콜백이 도달하면 안 된다.
    // 짧은 첫 비교·type 제거 뒤 짧은 둘째 비교·NUL 공간 없는 입력을 구별한다.
    for (const auto& topic:std::array<std::string,3>{"abc","type",std::string(80,'a')}) {
        bool rejected{};try { request.Open(0,topic); } catch (const std::out_of_range&) { rejected=true; } catch (const std::length_error&) { rejected=true; }CHECK(rejected);
    }
}
