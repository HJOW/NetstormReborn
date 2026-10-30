// 구름4칸/배경 선택의 원문695행과 실제 장기 전역/앞단의 같은 count 연결을 검사한다.
#include "TestSupport.h"
#include "client/RawRendererClouds.h"
#include "client/ClientRendererFrameGlobals.h"
#include "client/RawRendererPrelude.h"
#include "o/RawKencloudRender.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client; //원문 좌표/배경 장치의 같은 타입을 사용한다.
// 정상671개와 실제 IDIV fault24개의 전체 부분 효과/인자·호출 순서를 제품 본체와 비교한다.
TEST_CASE(RendererCloud_Original695AndCurrentEffects) {
    std::ifstream file(NETSTORM_RENDERER_CLOUDS_FIXTURE);CHECK(file.good());std::string line;std::size_t count{},faults{};
    while (std::getline(file,line)) { //각 행은 이전 실행과 독립인4칸/카메라/색/표시 입력이다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::array<std::int64_t,17> input{};std::array<std::uint32_t,31> expected{};std::string expectedEvents;
        for (auto& item:input) row>>item; //signed와 전체DWORD를 넓은 정수로 보존한다.
        for (auto& item:expected) row>>item; //원문 출력은 하위 DWORD다.
        row>>expectedEvents;CHECK(!row.fail());if (row.fail()) continue;
        const auto word=[&](std::size_t index) { return static_cast<std::uint32_t>(input[index]); }; //입력의 실제32비트를 반환한다.
        const auto integer=[&](std::size_t index) { return std::bit_cast<std::int32_t>(word(index)); }; //좌표/IDIV의 signed 해석이다.
        std::array<ScreenRect,4> rectangles{};std::array<std::uint32_t,16> initial{};
        for (std::uint32_t i=0;i<16;++i) initial[i]=i*17U-3U; //기존 미사용 칸도 원문과 같은 보존 입력이다.
        std::memcpy(rectangles.data(),initial.data(),64);
        std::uint32_t queueCount=word(1),checking=word(2),mode=word(3),hidden=word(4),dirty=word(5),covered=word(6),tint=word(7),black=word(8);
        std::int32_t cameraX=integer(9),cameraY=integer(10),denominator=integer(11);
        ScreenRect clipped{integer(12),integer(13),integer(14),integer(15)};std::string events;std::set<char> changed;
        const auto append=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; //실제 순서/원시 인수를 기록한다.
        const auto effect=[&](char label) {
            if (!changed.insert(label).second) return;
            const auto mutation=input[16];
            if (label=='A' && (mutation==1 || mutation==2)) queueCount=mutation==1 ? 1U : 0x7fffffffU;
            else if (label=='P' && mutation==3) queueCount=1;
            else if (label=='P' && (mutation==4 || mutation==5)) { queueCount=3;denominator=mutation==4 ? 7 : 0;cameraX=-17;cameraY=23;rectangles[1]={101,103,107,109}; }
            else if (label=='P' && mutation==10) queueCount=0xffffffffU;
            else if (label=='F' && (mutation==6 || mutation==7 || mutation==9)) { mode=mutation==6 ? 0U : 24U;tint=247;dirty=mutation==9 ? 4U : 0U;covered=mutation==9 ? 1U : 0U;denominator=3;cameraX=-17;cameraY=23;clipped={10,20,30,40}; }
        }; //명시 하위 효과만 공급하며 기대 분기를 다시 계산하지 않는다.
        const auto text=[](ScreenRect rect) { return std::to_string(static_cast<std::uint32_t>(rect.left))+','+std::to_string(static_cast<std::uint32_t>(rect.top))+','+std::to_string(static_cast<std::uint32_t>(rect.right))+','+std::to_string(static_cast<std::uint32_t>(rect.bottom)); }; //C++ signed 인자를 원문 DWORD 표기로 바꾼다.
        RendererCloudHooks hooks;
        hooks.assertion=[&](auto at) { append("A:"+std::to_string(at));effect('A'); };
        hooks.clear=[&](auto rect,auto color) { append("F:"+text(rect)+','+std::to_string(color));effect('F'); };
        hooks.draw=[&](auto rect,auto x,auto y,bool alternate) { const char label=alternate ? 'P' : 'C';append(std::string(1,label)+':'+text(rect)+','+std::to_string(static_cast<std::uint32_t>(x))+','+std::to_string(static_cast<std::uint32_t>(y)));effect(label); };
        bool fault{};
        try {
            if (input[0]==0) AppendRendererCloudDirty({rectangles,queueCount},clipped,checking,hooks);
            else if (input[0]==1) RefreshRendererCloudBackground({rectangles,queueCount},cameraX,cameraY,denominator,hooks);
            else DrawRendererBackdrop(clipped,dirty,covered,{mode,hidden,tint,black,cameraX,cameraY,denominator},hooks);
        }catch (const std::domain_error&) { fault=true; }
        std::vector<std::uint32_t> actual{queueCount};
        for (const auto rect:rectangles) actual.insert(actual.end(),{static_cast<std::uint32_t>(rect.left),static_cast<std::uint32_t>(rect.top),static_cast<std::uint32_t>(rect.right),static_cast<std::uint32_t>(rect.bottom)}); //기존4칸 전체를 비교한다.
        actual.insert(actual.end(),{mode,hidden,dirty,covered,tint,black,static_cast<std::uint32_t>(cameraX),static_cast<std::uint32_t>(cameraY),static_cast<std::uint32_t>(denominator),static_cast<std::uint32_t>(clipped.left),static_cast<std::uint32_t>(clipped.top),static_cast<std::uint32_t>(clipped.right),static_cast<std::uint32_t>(clipped.bottom),fault ? 1U : 0U});
        CHECK(actual==std::vector<std::uint32_t>(expected.begin(),expected.end()));CHECK((events.empty() ? "-" : events)==expectedEvents);++count;if (fault) ++faults;
    }
    CHECK(count==695 && faults==24);
}

//4칸 경계의 자연 생략/선택된 진단과 제공하지 않은 앞선 메모리 쓰기를 구분한다.
TEST_CASE(RendererCloud_SelectedDevicesAndPhysicalBounds) {
    std::array<ScreenRect,4> rectangles{};std::uint32_t count=4,checking{};
    AppendRendererCloudDirty({rectangles,count},{1,2,3,4},checking,{});CHECK(count==4);
    checking=1;bool missing{};try { AppendRendererCloudDirty({rectangles,count},{1,2,3,4},checking,{}); }catch (const std::logic_error&) { missing=true; }CHECK(missing && count==4);
    checking=0;count=0xffffffffU;bool physical{};try { AppendRendererCloudDirty({rectangles,count},{1,2,3,4},checking,{}); }catch (const std::out_of_range&) { physical=true; }CHECK(physical && count==0xffffffffU);
    std::int32_t x{},y{},denominator{};RefreshRendererCloudBackground({rectangles,count},x,y,denominator,{});CHECK(count==0); //음수 count는 IDIV/표시도 요구하지 않는다.
}

// 실제 장기4칸/count와 앞단의 별도 count0이 연결되며 일반 Renderer dirty 개수는 따로 유지된다.
TEST_CASE(RendererCloud_FrameGlobalsShareActualPreludeQueue) {
    ClientRendererFrameGlobals first,second;CHECK(std::all_of(first.Scan().begin(),first.Scan().end(),[](auto value) { return value==0; }));CHECK(first.CloudDirtyCount()==0 && first.HideGroups()==0);
    CHECK(first.Scan().data()!=second.Scan().data() && &first.CloudDirtyCount()!=&second.CloudDirtyCount());
    first.Scan()[0]=3;std::uint32_t checking{};AppendRendererCloudDirty(first.CloudQueue(),{1,2,3,4},checking,{});CHECK(first.CloudDirtyCount()==1 && second.CloudDirtyCount()==0);
    std::uint32_t mode=139,force=1,width=1024,height=768,initialHeight=768,cameraY{},left{},top{},right=1024,bottom=768,background=7,dirtyCount=9;std::array<std::uint32_t,2054> cloud{};std::vector<ScreenRect> added;
    RendererPreludeHooks hooks;hooks.markCloudDirty=[&] { netstorm::o::MarkKencloudDirty(cloud); };
    hooks.queueCloudDirty=[&](auto rect) { AppendRendererCloudDirty(first.CloudQueue(),rect,checking,{}); };
    hooks.overrideActive=[] { return 0U; };hooks.invalidate=[&](auto rect,auto flags) { CHECK(flags==0);added.push_back(rect); };
    PrepareRendererFrame({first.Scan(),background,dirtyCount,first.CloudDirtyCount()},{mode,force,width,height,initialHeight,cameraY,left,top,right,bottom},hooks);
    CHECK(first.CloudDirtyCount()==0 && dirtyCount==0 && first.Scan()[0]==3 && first.Scan()[18]==139 && background==7);
    CHECK(first.CloudRectangles()[0].left==1 && first.CloudRectangles()[1].right==1024 && first.CloudRectangles()[2].bottom==768);
    CHECK(added.size()==1 && cloud[2051]==1 && cloud[2052]==0 && second.CloudRectangles()[1].right==0);
}
