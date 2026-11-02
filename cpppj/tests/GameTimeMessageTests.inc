// 실제제품작은몸체/같은MessageGump를독립기계어출력에대조한다.게임/OS/MP는실행하지않는다.
#include "client/GameTimeDisplay.h"

namespace { //기존MessageGumpTests의실제192BYTE직렬화만재사용한다.
struct GameTimeMessageScene754:MessageScene { //외부CRT/벽시각/번역입력만소유하고메시지계산을기대값에서만들지않는다.
    std::uint32_t now{},origin{},stamp{0x87654321U},language{}; //원문clock입력/원점/갱신대상과명시번역번호다.
    double wall{}; //게임FrameTime과별개로입력하는현재벽시각이다.
    MessageGumpHooks TimeHooks() { //기존Text/이름/Style/측정을쓰고kind16의세선택경계만추가한다.
        auto hooks=Hooks();
        hooks.gameTimeSeconds=[this] { return ReadGameTimeSeconds(stamp,origin,[this] { Event("Clock,"+std::to_string(now));return now; }); };
        hooks.translate=[this](auto source) { Event("Translate,"+TextHex(source));const std::array<std::string_view,3> formats{"~E~yGame Time~r:~w %d:%02d:%02d","~ETime %d/%02d/%02d",""};return std::string(formats.at(language)); };
        hooks.wallSeconds=[this] { Event("Wall,"+Hex(std::bit_cast<std::array<std::uint8_t,8>>(wall)));return wall; };
        hooks.idle=[this](auto token,auto enabled) { CHECK(token==kNode && enabled);Event("Idle,1");node.flags|=0x80U; };
        return hooks;
    }
};
// C++에서는실제snprintf가실행되므로원문CRT호출기록한종류만비교trace에서분리한다.
std::string GameTimeWithoutCrtTrace754(std::string_view trace) {
    std::string result; //나머지외부효과순서/인자는원문기록그대로비교한다.
    for (const auto& part:Split(trace,';')) { if (part.starts_with("Format,")) continue;if (!result.empty()) result+=';';result+=part; } //다른경계나공백은정규화하지않는다.
    return result;
}
}

// 원문T/t의뗌/모달/정지/수정키와DWORD토글을각각검사한다.
TEST_CASE(GameTimeDisplay_KeyNative168) {
    std::size_t count{}; //독립기계어고정행수이며원문기대값은fixture에서만읽는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_GAME_TIME_KEY_FIXTURE).rows) { //선행모달은실제Uber부모와같은호출거부로구별한다.
        CHECK(row.size()==7);if (row.size()!=7) return;
        const InputEvent event{Number(row[0]),0,0};auto enabled=Number(row[3]);const bool modal=Number(row[2])!=0;std::string trace;
        if (!modal) static_cast<void>(DispatchGameTimeKey(event,enabled,[&] { trace="P:"+row[1];return Number(row[1])!=0; }));
        CHECK(enabled==Number(row[4]));CHECK((trace.empty() ? "-" : trace)==row[5]);++count;
    }
    CHECK(count==168);
}

// 실제kind16의192BYTE·CRTstamp·서식/기한/Idle/측정순서와음수IDIV경계를확인한다.
TEST_CASE(GameTimeDisplay_MessageNative90) {
    std::size_t count{}; //15시계×2벽시각×3번역의서로다른원문입력이다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_GAME_TIME_MESSAGE_FIXTURE).rows) {
        CHECK(row.size()==7);if (row.size()!=7) return;GameTimeMessageScene754 scene;
        scene.now=Number(row[0]);scene.origin=Number(row[1]);scene.language=Number(row[3]);const auto wall=Bytes(row[2]);CHECK(wall.size()==8);if (wall.size()!=8) return;
        std::array<std::uint8_t,8> wallBits;std::copy(wall.begin(),wall.end(),wallBits.begin());scene.wall=std::bit_cast<double>(wallBits);
        MessageGump message(scene.create,scene.TimeHooks());CHECK(message.Construct(kNode,16,1,0)==kNode);
        CHECK(scene.Snapshot()==row[4]);CHECK(scene.stamp==Number(row[5]));CHECK(scene.trace==GameTimeWithoutCrtTrace754(row[6]));++count;
    }
    CHECK(count==90);
}

// clock콜백뒤현재원점을읽고예외전후의부분쓰기를원문위치에유지한다.
TEST_CASE(GameTimeDisplay_CurrentOriginAndSelectedFailures) {
    std::uint32_t stamp{9},origin{0};CHECK(ReadGameTimeSeconds(stamp,origin,[&] { origin=2000;return 3001U; })==1 && stamp==3001);
    bool failed{};try { ReadGameTimeSeconds(stamp,origin,[]()->std::uint32_t { throw std::logic_error("clock stop"); }); }catch (const std::logic_error&) { failed=true; }CHECK(failed && stamp==3001 && origin==2000);
    GameTimeMessageScene754 scene;auto hooks=scene.TimeHooks();hooks.wallSeconds={};MessageGump message(scene.create,std::move(hooks));failed=false;
    try { message.Construct(kNode,16,1,0); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.node.deadline==0.0 && scene.node.head==0x12408000 && scene.stamp==0 && !(scene.node.flags&0x80U));
}
