// DrawWater의 실제 전체 finder/투영/VFX·현재 필드와 일반 입력/이름/FIFO 관측을 대조한다.
#include "GumpRawSupport.h"
#include "client/DrawWater.h"
#include "client/ClientGameplayGlobals.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/Renderer.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

using namespace netstorm::client; //제품의 같은 실제 표시/카메라/입력/큐다.
using namespace netstorm::o; //현재 raw Pool/Hash/TYPE를 그대로 조립한다.
using namespace netstorm::test::gumpraw; //기대값의 BYTE/열 해독만 공유한다.
namespace { //원문 기대 표시를 계산하지 않는 명시 화면 장치와 입력 도구다.
constexpr int kWidth=96,kHeight=80,kPitch=100; //Native 표면과 같은 폭/행padding이다.
std::string RawText(std::string_view hex) { const auto value=Bytes(std::string(hex));return {value.begin(),value.end()}; } //NUL/바이트를 그대로 해독한다.
std::string RectText(ScreenRect rect) { return std::to_string(rect.left)+':'+std::to_string(rect.top)+':'+std::to_string(rect.right)+':'+std::to_string(rect.bottom); } //현재 실제 사각형의 직렬화다.
//원문과 같은 여섯 합성 일반 객체/실제 네 단계 해시를 소유한다. 실제 공중3종/월드 생성은 아니다.
struct Drawing {
    SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash;std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(256); //실제 Native 입력의 현재물리다.
    RawCameraViewState renderer,snapshot;std::uint32_t enabled{},color{0x1000002dU},mutation{},rects{},presents{}; //현재 입력/관측이며 화면/카메라를 같은 저장소로 합치지 않는다.
    ScreenRect clip{5,7,91,73};std::vector<std::uint8_t> pixels=std::vector<std::uint8_t>(kPitch*kHeight,199); //이름없는 메모리 화면과 현재 clip이다.
    std::string trace;DrawWaterHooks hooks; //장치의 실제 호출 순서만 기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //Native의 동일 효과 표기다.
    explicit Drawing(const std::vector<std::string>& input) { //독립 input9열을 그대로 현재 상태에 공급한다.
        enabled=Number(input[0]);renderer.cellPixels={std::stoi(input[1]),std::stoi(input[2])};snapshot.cellPixels=renderer.cellPixels;renderer.worldBits={std::bit_cast<std::uint32_t>(std::stof(input[3])),std::bit_cast<std::uint32_t>(std::stof(input[4]))};snapshot.pixels={std::stoi(input[5]),std::stoi(input[6])};mutation=Number(input[7]);const auto scene=Number(input[8]);
        constexpr std::array<std::uint16_t,6> ids{50,60,61,62,63,64};constexpr std::array<std::uint8_t,6> states{0,1,2,4,8,6}; //원문 일반 finder는 이 상태를 새로 제외하지 않는다.
        for (std::size_t i=0;i<ids.size();++i) { //입력 순서로 같은 버킷 머리에 연결한다. 순회/포함 결과는 제품이 계산한다.
            const float x=scene==2 ? static_cast<float>(100+i) : static_cast<float>(20+i%3)+.25F,y=scene==2 ? static_cast<float>(110+i) : static_cast<float>(20+i/3)+.5F;auto raw=InputRaw(pool,Sid{ids[i]});std::fill(raw.begin(),raw.end(),std::uint8_t{0});raw[10]=static_cast<std::uint8_t>(100+i);raw[11]=states[i];raw[40]=scene==1 && i%2 ? 8:0;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));types[100+i].footX=static_cast<int>(1+i%3);types[100+i].footY=static_cast<int>(1+i%2);
            auto& head=hash.Bucket(static_cast<int>(i%4),x,y);Put(raw,4,head,2);head=ids[i];
        }
        hooks.clip=[this](auto rect) { Add("Clip:"+RectText(rect));clip=rect; };
        hooks.rectangle=[this](auto rectangle,auto ink) { Add("Rect:"+RectText(rectangle)+':'+std::to_string(ink));++rects;
            if (rects==1) { if (mutation==1) color=0x123456a1U;else if (mutation==2) snapshot.cellPixels={7,13};else if (mutation==3) enabled=0;else if (mutation==4) InputRaw(pool,Sid{63})[40]=8; } //Native 경계에서 공급한 현재 필드 변이를 같은 호출 위치에 적용한다.
            const auto saved=clip;clip={0,0,kWidth,kHeight};Add("Lock:"+RectText(clip));DrawWaterRectangle(pixels,kPitch,kHeight,clip,rectangle,ink);Add("Unlock:"+RectText(clip));clip=saved; //같은 실제 VFX를 그린 뒤 원문의 clip을 복구한다.
        };
        hooks.present=[this] { const auto saved=clip;Add("Present:0:0:96:80");clip={0,0,kWidth,kHeight};++presents;clip=saved; }; //실제4a46d0의 OS 출력만 명시 장치다.
    }
    void Draw() { if (enabled) { const auto area=DrawWaterVisibleArea(renderer,kWidth,kHeight);Add("Begin:"+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.left)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.top)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.right)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.bottom)))+":0"); }RawDrawWater(pool,hash,types).Draw(renderer,snapshot,kWidth,kHeight,enabled,color,hooks); } //Native constructor의 입력만 기록하며 전체 순회는 같은 제품 몸체다.
};
//Native 변경 구간은 전체 표면 기대값으로만 되돌린다. 선/clip 기대결과를 재계산하지 않는다.
std::vector<std::uint8_t> ExpectedPixels(std::string_view changed) {
    std::vector<std::uint8_t> result(kPitch*kHeight,199);if (changed=="-") return result;
    for (const auto& run:Split(changed,';')) { const auto fields=Split(run,':');CHECK(fields.size()==2);if (fields.size()!=2) continue;const auto offset=Number(fields[0]);const auto bytes=Bytes(fields[1]);CHECK(offset+bytes.size()<=result.size());if (offset+bytes.size()<=result.size()) std::copy(bytes.begin(),bytes.end(),result.begin()+offset); } //관측한 실제 변경 BYTE만 원위치에 놓는다.
    return result;
}
}

TEST_CASE(DrawWater_Native76FullFinderProjectionRectPixelsAndMutations) { //실제76원문 입력의 전체 픽셀/clip/색/현재 flag와 효과 순서를 대조한다.
    const auto rows=LoadFixture(NETSTORM_DRAW_WATER_FIXTURE).rows;CHECK(rows.size()==76);
    for (const auto& row:rows) { //서로 다른 Renderer/snapshot·음수 half·buried/상태·현재색/칸/flag 변이까지 포함한다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const auto input=Split(row[0].substr(1,row[0].size()-2),',');CHECK(input.size()==9);if (input.size()!=9) continue;Drawing d(input);d.Draw();
        CHECK(d.pixels==ExpectedPixels(row[1]));CHECK(RectText(d.clip)==row[2]);CHECK(d.enabled==Number(row[3]) && d.color==Number(row[4]));CHECK((d.trace.empty() ? "-" : d.trace)==row[5]);CHECK(d.presents==(Number(input[0]) ? 1U:0U)); //flag를 반복 중 끈 입력도 현재 원문 호출은 끝까지 진행한다.
    }
}

TEST_CASE(DrawWater_Native384GeneralInputAndFullDwordStores) { //Guide/Gump/release/정지/Console/모드의 실제 일반 분기를 대조한다.
    const auto rows=LoadFixture(NETSTORM_DRAW_WATER_INPUT_FIXTURE).rows;CHECK(rows.size()==384);
    for (const auto& row:rows) { //선행 Guide/Gump의 현재 소비를 일반 분배보다 먼저 적용한다.
        CHECK(row.size()==14);if (row.size()!=14) continue;auto enabled=Number(row[8]),redraw=Number(row[9]);if (!Number(row[5]) && !Number(row[6])) CHECK(DispatchDrawWater({Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))},enabled,redraw));
        CHECK(enabled==Number(row[10]) && redraw==Number(row[11]) && row[12]=="1" && row[13]=="0");
    }
    std::uint32_t enabled=7,redraw=92;CHECK(!DispatchDrawWater({'W',0,0},enabled,redraw) && enabled==7 && redraw==92);
}

TEST_CASE(DrawWater_Native9NamesCurrentQueueAndNoDefaultMenuAlias) { //실제 등록명과 숫자/문자열 인자를 기존 큐로 보내고 추가 developer UI는 만들지 않는다.
    const auto rows=LoadFixture(NETSTORM_DRAW_WATER_NAMES_FIXTURE).rows;CHECK(rows.size()==9);
    for (const auto& row:rows) { //기대 이름/인자/고리 결과는 실제 원문 fixture에서만 읽는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};QueueScriptCommand(RawText(row[1]),queue,strings,{GameMenuScriptCode,[&](auto) { ++unknown; }});CHECK(!unknown && strings.Index()==Number(row[7]));const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;
        const auto event=queue.Pop(false);CHECK(queue.Empty() && event.code==Number(expected[0]) && event.x==event.y && IsGameMenuCommand(event.code));const auto parameter=std::bit_cast<std::uint32_t>(event.y);if (expected[1].starts_with('n')) CHECK(parameter==Number(expected[1].substr(1)));else { const auto text=strings.Resolve(parameter);CHECK(Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()})==expected[1].substr(1)); }
    }
    CHECK(GameMenuCommandValue(0x542760U)==kDrawWaterCommand && DrawWaterScriptCode("Water")==0xffffffffU);
}

TEST_CASE(DrawWater_LongOwnerAndDisabledNoDeviceNoViewportDivision) { //flag는 미션/Renderer 재생성으로 초기화하지 않고 미도달 장치도 요구하지 않는다.
    ClientGameplayGlobals globals;CHECK(!globals.DrawWater());auto* identity=&globals.DrawWater();globals.DrawWater()=7;
    { auto use=globals.AcquirePrepared();CHECK(&globals.DrawWater()==identity && globals.DrawWater()==7); }CHECK(&globals.DrawWater()==identity && globals.DrawWater()==7);
    { Renderer replaced(kWidth,kHeight);CHECK(globals.DrawWater()==7); }CHECK(&globals.DrawWater()==identity && globals.DrawWater()==7);
    SidPool pool(OriginalEdition::Patch1078,24000,false);SquidHash hash;std::vector<RiftTypeRecord> types(1);std::uint32_t enabled{},color{};RawDrawWater(pool,hash,types).Draw({}, {},0,0,enabled,color,{}); //cellPixels0도flag0에서는나눗셈에 도달하지 않는다.
    RawCameraViewState invalid;bool failed{};try { static_cast<void>(DrawWaterVisibleArea(invalid,96,80)); }catch (const std::domain_error&) { failed=true; }CHECK(failed);
}

TEST_CASE(DrawWater_PostSceneIndependentOfDirtyAndScreenClip) { //같은 실제 Renderer의 dirty가 비어도 후단은 전체 viewport 출력과 기존 clip 복구를 수행한다.
    Drawing d({"1","16","11","19.25","19.5","304","209","0","2"});Renderer renderer(kWidth,kHeight);
    renderer.Draw(d.pixels,kPitch);renderer.Present([](auto) {});CHECK(renderer.CameraDirtyEntries().empty());
    std::uint32_t regularPresents{};renderer.Draw(d.pixels,kPitch);d.Draw();renderer.Present([&](auto) { ++regularPresents; });CHECK(!regularPresents && d.presents==1 && d.rects==0 && RectText(d.clip)=="5:7:91:73"); //후보0개/dirty0이어도 원문 즉시 viewport 출력은 별도이며 지형/배경 표시를 끄지 않는다.
}

// M7.4(1271): 원문 434970의 세 겹 사각형 좌표를 순서와 감김까지 확인한다.
TEST_CASE(PathMarker_ThreeShrinkingCellRectanglesFollowOriginalOrder) {
    const auto rectangles = netstorm::client::PathMarkerRectangles({100, 80}, {16, 11}); // 오른쪽 아래가 투영점이고 칸 크기 16x11이다.
    CHECK(rectangles[0].left == 85 && rectangles[0].top == 70 && rectangles[0].right == 99 && rectangles[0].bottom == 79);
    CHECK(rectangles[1].left == 86 && rectangles[1].top == 71 && rectangles[1].right == 98 && rectangles[1].bottom == 78);
    CHECK(rectangles[2].left == 87 && rectangles[2].top == 72 && rectangles[2].right == 97 && rectangles[2].bottom == 77);
}
// 음수/경계 좌표도 signed DWORD 감김으로 정의된 값을 만든다.
TEST_CASE(PathMarker_WrapsSignedDwordWithoutUndefinedBehavior) {
    const auto rectangles = netstorm::client::PathMarkerRectangles({-2147483647 - 1, 2147483647}, {-1, 5});
    CHECK(rectangles[0].right == 2147483647 && rectangles[0].bottom == 2147483646); // INT_MIN-1이 INT_MAX로 감긴다.
    CHECK(rectangles[0].left == -2147483646); // 첫 겹 left는 point-cell+1이다: INT_MIN-(-1)+1.
    CHECK(rectangles[2].bottom == 2147483644 && rectangles[2].top == 2147483647 - 5 + 3);
}
