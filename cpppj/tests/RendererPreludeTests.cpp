// 원본720개 관측의 전체 인접 상태/현재 전역·부분 쓰기와 장치 순서를 비교한다.
#include "TestSupport.h"
#include "client/RawRendererPrelude.h"
#include "o/RawKencloudRender.h"
#include <array>
#include <bit>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client; //같은 실제 clip과 프레임 앞단을 독립 원문 출력에 대조한다.
// 원문720행의 입력과 전체 출력·효과 뒤 재조회·호출 순서를 실제 제품 본체에 재생한다.
TEST_CASE(RendererPrelude_Original720AndCurrentEffects) {
    std::ifstream file(NETSTORM_RENDERER_PRELUDE_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    // 각 행은 별개 화면/Renderer/구름·스캔 상태를 가진 독립 원문 입력이다.
    while (std::getline(file,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::array<std::int64_t,19> input{};std::array<std::uint32_t,35> expected{};std::string expectedEvents;
        for (auto& value:input) row>>value; //입력은 signed 값과 전체 unsigned DWORD를 함께 보존한다.
        for (auto& value:expected) row>>value; //원문 출력의 실제 하위 DWORD다.
        row>>expectedEvents;CHECK(!row.fail());if (row.fail()) continue;
        const auto value=[&](std::size_t i) { return static_cast<std::uint32_t>(input[i]); }; //부호가 있는 입력도 같은 비트로 사용한다.
        std::array<std::uint32_t,19> scan{};scan[0]=value(12);scan[17]=value(13);scan[18]=value(1);
        for (std::uint32_t i=0;i<16;++i) scan[i+1]=i*7U-11U; //원문 초기화와 무관한 기존16행 입력이다.
        std::array<std::uint32_t,11> globals{value(0),value(2),value(3),value(4),value(5),value(6),value(7),value(8),value(9),value(10),value(11)};
        std::uint32_t background=value(16),dirtyCount=value(17),drawCounter=value(18),queries{};
        std::array<std::uint32_t,2054> cloud{};cloud[2051]=17;cloud[2052]=19;std::string events;std::set<char> changed;
        const auto append=[&](std::string entry) { if (!events.empty()) events+=';';events+=entry; }; //원문 호출의 순서와 원시 인수를 기록한다.
        const auto effect=[&](char label) {
            if (!changed.insert(label).second) return;
            if (input[15]==1 && label=='M') { globals[0]=0x38;globals[3]=801;globals[1]=1; }
            else if (input[15]==2 && label=='F') { globals[0]=0x29;globals[4]=611; }
            else if (input[15]==3 && label=='O') globals[0]=0;
            else if (input[15]==4 && label=='U') globals[0]=0x19;
            else if (input[15]==5 && label=='Q') { scan[0]=15;globals[0]=0x19; }
            else if (input[15]==6 && label=='A') { globals[0]=0;globals[1]=0; }
            else if (input[15]==7 && label=='Q') { scan[0]=2;scan[3]=100;globals[3]=704;globals[7]=17; }
            else if (input[15]==8 && label=='B') { globals[1]=0;background=17; }
        }; //명시 하위 효과만 입력하며 상위 계산/기대값을 구현하지 않는다.
        const auto rectangle=[](ScreenRect rect) { return std::to_string(static_cast<std::uint32_t>(rect.left))+','+std::to_string(static_cast<std::uint32_t>(rect.top))+','+std::to_string(static_cast<std::uint32_t>(rect.right))+','+std::to_string(static_cast<std::uint32_t>(rect.bottom)); }; //주소 대신 실제 네 DWORD 인수를 비교한다.
        RendererPreludeHooks hooks;
        hooks.markCloudDirty=[&] { append("M");effect('M');netstorm::o::MarkKencloudDirty(cloud); };
        hooks.queueCloudDirty=[&](auto rect) { append("F:"+rectangle(rect));effect('F'); };
        hooks.overrideActive=[&] { append("O");effect('O');return globals[2]; };
        hooks.updateClouds=[&] { append("U");effect('U'); };
        hooks.rowDirty=[&](auto y,auto cameraY) { ++queries;append("Q:"+std::to_string(static_cast<std::uint32_t>(y))+','+std::to_string(static_cast<std::uint32_t>(cameraY)));effect('Q');return queries==value(14) ? 1U : 0U; };
        hooks.invalidate=[&](auto rect,auto flags) { append("A:"+rectangle(rect)+','+std::to_string(flags));effect('A'); };
        hooks.invalidateBackground=[&] { append("B");effect('B'); };
        PrepareRendererFrame({scan,background,dirtyCount,drawCounter},{globals[0],globals[1],globals[3],globals[4],globals[5],globals[6],globals[7],globals[8],globals[9],globals[10]},hooks);
        std::vector<std::uint32_t> actual(scan.begin(),scan.end());actual.insert(actual.end(),globals.begin(),globals.end());
        actual.insert(actual.end(),{background,dirtyCount,drawCounter,cloud[2051],cloud[2052]});
        CHECK(actual==std::vector<std::uint32_t>(expected.begin(),expected.end()));CHECK((events.empty() ? "-" : events)==expectedEvents);++count;
    }
    CHECK(count==720);
}

// 구름 비표시/override 조기 생략은 연결하지 않은 하위를 강제로 요구하지 않는다.
TEST_CASE(RendererPrelude_SelectedDevicesAndPartialFailure) {
    std::array<std::uint32_t,19> scan{};std::uint32_t background=7,dirty=9,counter=11,mode{},force{},width=1024,height=768,initialHeight=768,cameraY{},left=84,top{},right=1024,bottom=768;
    const RendererPreludeGlobals globals{mode,force,width,height,initialHeight,cameraY,left,top,right,bottom};
    PrepareRendererFrame({scan,background,dirty,counter},globals,{});CHECK(counter==0 && background==7 && dirty==9);
    mode=8;scan[18]=8;counter=17;RendererPreludeHooks hooks;hooks.overrideActive=[] { return 19U; };
    PrepareRendererFrame({scan,background,dirty,counter},globals,hooks);CHECK(counter==0 && scan[0]==0 && scan[17]==0);
    counter=23;force=1;bool rejected{};
    try { PrepareRendererFrame({scan,background,dirty,counter},globals,hooks); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && counter==23 && dirty==9); //선택된 첫 MarkDirty 누락에서 멈추며 뒤 counter0/표 지우기를 가짜로 실행하지 않는다.
}
