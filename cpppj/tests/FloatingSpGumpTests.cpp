// 독립 원본 기계어426입력의 필드 비트·효과 순서와 콜백 뒤 현재 읽기를 대조한다.
#include "RawSceneSupport.h"
#include "client/FloatingSpGump.h"
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNode=0x12201000,kRoot=0x12345678,kCanary=0xa7a7a7a7; // 원본 격리 관측과 같은 토큰/패딩 입력이다.
// 외부 효과만 관측 경계로 재생한다. 위치/속도/분기/서식 계산은 제품에 맡긴다.
struct FloatingScene {
    GumpCreateState create; // 현재 원본 화면/루트 입력이다.
    FloatingSpGumpNode node; // 같은 기본/파생 실제 저장소다.
    double now{3.25}; // 현재 게임 시각이다.
    bool mutate{}; // 첫 Redraw에서만 필드를 바꾸는 원본 관측 입력이다.
    std::string trace; // 모든 실제 외부 호출 순서다.
    // 바이트 canary에 대응하는 입력 필드를 명시 초기화한다.
    FloatingScene() {
        create.collection.root=kRoot;create.screenWidth=640;create.screenHeight=480;node.vtable=0x50688c;node.depth=kCanary;
        node.x=node.y=node.velocity=node.acceleration=std::bit_cast<float>(kCanary);node.padding=kCanary;node.stamp=std::bit_cast<double>(std::uint64_t{0xa7a7a7a7a7a7a7a7});
    }
    // 원본과 같은 쉼표/DWORD/정확한 double 바이트로 현재 필드를 보고한다.
    std::string Snapshot() const {
        return std::to_string(node.vtable)+','+std::to_string(node.depth)+','+std::to_string(std::bit_cast<std::uint32_t>(node.x))+','+
            std::to_string(std::bit_cast<std::uint32_t>(node.y))+','+std::to_string(std::bit_cast<std::uint32_t>(node.velocity))+','+
            std::to_string(std::bit_cast<std::uint32_t>(node.acceleration))+','+std::to_string(node.padding)+','+Hex(std::bit_cast<std::array<std::uint8_t,8>>(node.stamp));
    }
    // 경계 효과를 빠짐없이 원본 순서로 덧붙인다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 기존 Text/수명/장치 경계의 입력만 기록한다. 출력 기대값을 제품에 주입하지 않는다.
    FloatingSpGumpHooks Hooks() {
        FloatingSpGumpHooks h;h.node=[this](auto token) { CHECK(token==kNode);return &node; };h.now=[this] { return now; };
        h.allocate=[this](auto bytes) { CHECK(bytes==0xb8);Event("N");return kNode; };
        h.constructText=[this](auto) { node.vtable=0x5144d4;Event("T"); };h.clearText=[this](auto) { Event("C"); };h.fitText=[this](auto) { Event("F"); };
        h.styledText=[this](auto,std::string_view text) { Event("S,"+std::string(text)); };h.idle=[this](auto,bool enabled) { Event("I,"+std::to_string(enabled)); };
        h.redraw=[this](auto,bool erase) {
            CHECK(!erase);const bool first=trace.empty();Event("R,"+Snapshot());
            if (mutate && first) { node.y=7.5F;node.velocity=-1.25F;node.acceleration=3; }
        };
        h.kill=[this](auto) { Event("K"); };h.attach=[this](auto,float x,float y,auto parent,auto mode) {
            Event("A,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(parent)+','+std::to_string(mode));node.x=x;node.y=y;
        };
        h.reference=[this](auto,auto object) { Event("O,"+std::to_string(object)); };
        h.deleteText=[this](auto token,auto flags) { node.vtable=0x5144d4;Event("C");Event("C");Event("B");if (flags&1) Event("D");return token; };return h;
    }
};
}
// 원본 두 정밀도의 전체 관측 행을 모두 비교한다. 파생 가상 효과의 현재 읽기도 포함한다.
TEST_CASE(FloatingSpGump_Native426) {
    std::size_t count{}; // 실제로 재생한 고유 원문 입력 수다.
    // 이미 동결된 보호PE 관측을 읽기만 하며 기대값을 다시 생성하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FLOATING_SP_FIXTURE).rows) {
        if (row[0]=="section") continue;CHECK(row.size()==6);if (row.size()!=6) return;
        const auto input=Split(row[1],',');FloatingScene scene;FloatingSpGump gump(scene.create,scene.Hooks());
        if (row[0]=="construct") gump.Construct(kNode,std::stoi(input[0]),std::bit_cast<float>(Number(input[1])));
        else if (row[0]=="show") gump.Show(std::stoi(input[0]),std::stoi(input[1]),std::stoi(input[2]),Number(input[3]));
        else if (row[0]=="delete") CHECK(gump.DeleteScalar(kNode,Number(input[0]))==kNode);
        else {
            CHECK(row[0]=="idle" && input.size()==9);scene.node.x=std::bit_cast<float>(Number(input[0]));scene.node.y=std::bit_cast<float>(Number(input[1]));
            scene.node.velocity=std::bit_cast<float>(Number(input[2]));scene.node.acceleration=std::bit_cast<float>(Number(input[3]));scene.node.stamp=std::stod(input[4]);
            scene.now=std::stod(input[5]);scene.create.screenWidth=std::stoi(input[6]);scene.create.screenHeight=std::stoi(input[7]);scene.mutate=input[8]=="1";gump.Idle(kNode);
        }
        if (scene.Snapshot()!=row[2] || scene.trace!=row[3]) std::printf("비용 표시 원문 차이 %zu %s %s\n%s\n%s\n",count,row[0].c_str(),row[1].c_str(),scene.Snapshot().c_str(),scene.trace.c_str());
        CHECK(scene.Snapshot()==row[2] && scene.Snapshot()==row[4]);CHECK(scene.trace==row[3] && scene.trace==row[5]);++count;
    }
    CHECK(count==426);
}
