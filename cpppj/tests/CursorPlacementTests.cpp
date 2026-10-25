// 실제 커서 세 몸체의 독립 기대값으로 허용/표시 범위/전체 외부 효과를 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCursorPlacement.h"
#include <fstream>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 독립 입력23개의 순서다. 파생 결과를 입력으로 재사용하지 않는다.
enum Column : std::size_t { Operation,Profile,Flags1,Flags2,FootX,FootY,Spacing,Describe,Text,OldGump,NewGump,Accept,Place,Obstruction,SourceRegion,CursorFlags,PlacementFlags,Mana,X,Y,Match,Mutation,RectVariant };
// 원문 DWORD 열을 비트 손실 없이 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 저장 순서대로 숫자만 읽으며 기대 결과를 재계산하지 않는다.
    for (const auto& field:Split(text,',')) result.push_back(Number(field));return result;
}
// decoder 경계에 공급하는 독립 프레임/좌표 입력이다.
std::vector<CursorCanonCell> Cells(std::uint32_t profile) {
    if (profile==2) return {};
    if (profile==4) return {{0,std::bit_cast<float>(0x7fc00000U),1.1f}};
    if (profile==5) return {{0,-1.9998f,-0.9999f},{1,254.0002f,255.0002f}};
    if (profile==0) return {{0,0,0}};return {{0,0,0},{1,1.25f,-0.5f},{2,-2.75f,3.5f}};
}
// SHP 공급자의 signed WORD 입력이다. C++ 판정이 결정한 기대값이 아니다.
CursorCostImage Image(std::uint32_t profile,std::int32_t frame) {
    const std::array<CursorCostImage,3> standard{{{1,31,23,8,17},{1,19,29,-3,31},{1,47,13,7,-5}}};
    if (profile==3 && frame==0) return {1,-11,-17,23,-7};return standard.at(static_cast<std::size_t>(frame));
}
// 매 행마다 하나의 실제 풀과 커서 상태 세 개를 공유하는 장면이다.
struct PlacementScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // Form/미리보기의 실제 raw 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE100/101 입력이다.
    std::vector<std::uint16_t> surface=std::vector<std::uint16_t>(65536); // 단일 표면 지도다.
    CursorDropState drop; // 취득/내려놓기와 공유하는 전역이다.
    CursorLifecycleState lifecycle; // 목록/치수/현재 Gump 전역이다.
    CursorPlacementState state; // 나머지 위치/허용 전역이다.
    std::vector<std::uint32_t> input; // 저장된 명시 입력23개다.
    std::string trace; // 현재 외부 효과의 전체 순서다.
    ScreenPoint projected; // 독립 투영 경계 입력이다.
    // 원본과 같은 SID 번호를 실제 client 풀에서 확보한다.
    PlacementScene() {
        // 사용할 마지막 미리보기까지 하나의 풀에서 순서대로 할당한다.
        for (std::uint16_t sid=5;sid<=100;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 원문 DWORD 인자를 같은 문자 형식으로 기록한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // 부호는 비트 표현으로 저장해 감김을 비교한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 원문 입력만 초기화하고 이전 결과를 재사용하지 않는다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;drop={};lifecycle={};state={};trace.clear();std::fill(surface.begin(),surface.end(),std::uint16_t{0});
        surface[0]=80;surface[22*256+21]=81;surface[21*256+20]=82;surface[255*256+255]=83;
        drop.legal=5;state.placementAllowed=values[Place];state.descriptionAllowed=values[Describe];state.spaceAllowed=1-values[Obstruction];state.sourceAllowed=values[Match];
        lifecycle.currentGump=values[OldGump] ? 1U : 0U;drop.legalGump=2;state.bounds={3,7,43,51};state.hotspotX=44;state.hotspotY=45;state.imageWidth=91;state.imageHeight=93;
        drop.type=lifecycle.abstractType=100;drop.argument=7;drop.direction=3;drop.preview=Sid{50};drop.costFlags=values[CursorFlags];state.placementFlags=values[PlacementFlags];drop.cursorGump=values[Mana] ? 1U : 0U;
        drop.x=std::bit_cast<float>(values[X]);drop.y=std::bit_cast<float>(values[Y]);drop.localPlayer=0x123401;drop.screenX=-3;drop.screenY=47;
        state.labelSpacing=std::bit_cast<std::int32_t>(values[Spacing]);state.obstruction=values[Obstruction];state.acceptEvent=0x123456;state.screenOrigin={-17,31};state.description="old";lifecycle.previews[0]=100;
        types[100].footX=std::bit_cast<std::int32_t>(values[FootX]);types[100].footY=std::bit_cast<std::int32_t>(values[FootY]);types[100].flags1=values[Flags1];types[100].flags2=values[Flags2];
        types[101].footX=7;types[101].footY=9;types[101].flags1=values[Flags1];types[101].flags2=4;
        const std::array<std::array<std::uint32_t,4>,3> sizes{{{35,17,49,23},{0xfffffffd,0xfffffff9,0xfffffff5,0xfffffff3},{0x7fffffff,0x80000001,0x7ffffffd,0x80000003}}};
        const auto& size=sizes[values[RectVariant]];lifecycle.warningWidth=std::bit_cast<std::int32_t>(size[0]);lifecycle.warningHeight=std::bit_cast<std::int32_t>(size[1]);drop.cursorWidth=std::bit_cast<std::int32_t>(size[2]);lifecycle.costHeight=std::bit_cast<std::int32_t>(size[3]);
        const std::array<ScreenPoint,3> points{{{101,203},{-3,-7},{0x7ffffffe,std::bit_cast<std::int32_t>(0x80000002U)}}};projected=points[values[RectVariant]];
        Put(pool.AllocatedBytes(Sid{100}),22,0xfff9,2);Put(pool.AllocatedBytes(Sid{100}),24,0x8001,2);
    }
    // 계산을 대신하지 않는 decoder/공간/GUI/표시 경계만 원문과 같은 입력으로 연결한다.
    CursorPlacementHooks Hooks() {
        CursorPlacementHooks hooks;
        hooks.describe=[this](std::string& text,float x,float y,std::uint32_t type,std::uint32_t owner) {
            Event("D",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),type,owner});text=input[Text]==0 ? "AB" : input[Text]==1 ? "" : "ABCDE";
            if (input[Mutation]==1) lifecycle.abstractType=101;return input[Describe];
        };
        hooks.walk=[this](CanonTypeQuery query,const std::function<bool(CursorCanonCell)>& visit) {
            Event("W",{query.type,static_cast<std::uint32_t>(query.argument),static_cast<std::uint32_t>(query.direction),std::bit_cast<std::uint32_t>(query.x),std::bit_cast<std::uint32_t>(query.y),query.explicitFrame});
            // 외부 반복자 입력은 단정도 저장 후 본문에 넘긴다. false이면 원본처럼 Advance를 생략한다.
            for (auto cell:Cells(input[Profile])) { cell.x+=query.x;cell.y+=query.y;if (!visit(cell)) return;Event("N"); }
        };
        hooks.image=[this](std::uint32_t type,std::int32_t frame) { Event("I",{type,0,static_cast<std::uint32_t>(frame)});return Image(input[Profile],frame); };
        hooks.gumpAt=[this](std::int32_t x,std::int32_t y) { Event("G",{static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y),lifecycle.currentGump,drop.legalGump});return input[NewGump] ? 1U : 0U; };
        hooks.accept=[this](std::uint32_t gump,std::uint32_t event,std::uint32_t first,std::uint32_t second,std::uint32_t third) { Event("A",{gump,event,first,second,third});return input[Accept] ? 2U : 0U; };
        hooks.parent=[](std::uint32_t gump) { CHECK(gump==2);return 77U; };
        hooks.mayPlace=[this](const CanonPlacementQuery& query) {
            Event("P",{query.type,query.argument,std::bit_cast<std::uint32_t>(query.x),std::bit_cast<std::uint32_t>(query.y),query.flags,query.owner,query.mode});
            if (input[Mutation]==2) state.obstruction=1;return input[Place];
        };
        hooks.source=[this](Sid sid) { Event("U",{sid.value});return Sid{77}; };
        hooks.region=[this](Sid sid) { Event("R",{sid.value});return static_cast<std::int32_t>(sid.value==77 || input[Match] ? input[SourceRegion] : input[SourceRegion]+1); };
        hooks.processEvent=[this](std::uint32_t process,std::uint32_t event,std::uint32_t first,std::uint32_t second) { Event("E",{process,event,first,second}); };
        hooks.project=[this](float x,float y) { Event("J",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});return projected; };
        hooks.invalidate=[this](ScreenRect rect,std::uint32_t flags) { Event("V",{static_cast<std::uint32_t>(rect.left),static_cast<std::uint32_t>(rect.top),static_cast<std::uint32_t>(rect.right),static_cast<std::uint32_t>(rect.bottom),flags}); };
        return hooks;
    }
    // 독립 원문과 같은17DWORD를 의미 상태에서 직렬화한다.
    std::string Snapshot(std::uint32_t result) const {
        std::vector<std::uint32_t> values{drop.legal,state.placementAllowed,state.descriptionAllowed,state.spaceAllowed,state.sourceAllowed,lifecycle.currentGump,drop.legalGump};
        // 갱신 폭/높이와 기준점의 모든 비트를 보존한다.
        for (const auto value:state.bounds) values.push_back(static_cast<std::uint32_t>(value));
        values.insert(values.end(),{static_cast<std::uint32_t>(state.hotspotX),static_cast<std::uint32_t>(state.hotspotY),static_cast<std::uint32_t>(state.imageWidth),static_cast<std::uint32_t>(state.imageHeight),lifecycle.abstractType,result});std::string text;
        // 저장된 순서대로 현재 상태만 표시한다.
        for (const auto value:values) text+=(text.empty() ? "" : ",")+std::to_string(value);return text;
    }
};
}
// 세 몸체의 반환/전체 상태/효과 순서를 실제 원문과 비교한다.
TEST_CASE(CursorPlacement_OriginalBodiesMatchIndependentX86) {
    PlacementScene scene;std::ifstream stream(NETSTORM_CURSOR_PLACEMENT_FIXTURE);CHECK(stream.good());std::string line;std::size_t count{};
    // 모든 고유 입력을 실제 C++ 몸체에 재생하고 전체 상태와 효과를 비교한다.
    while (std::getline(stream,line)) {
        if (line.empty() || line[0]=='#') continue;const auto fields=Split(line,'\t');CHECK(fields.size()==4);if (fields.size()!=4) continue;
        const auto input=Numbers(fields[0]);scene.Prepare(input);RawCursorPlacement placement(scene.pool,scene.types,scene.surface,scene.drop,scene.lifecycle,scene.state,scene.Hooks());std::uint32_t result{};
        if (input[Operation]==0) placement.Query();else if (input[Operation]==1) placement.Prepare();else result=placement.SourceAllowed();
        if (scene.Snapshot(result)!=fields[2] || (scene.trace.empty() ? "-" : scene.trace)!=fields[1]) std::printf("커서 위치 입력=%s\n기대=%s / %s\n실제=%s / %s\n",fields[0].c_str(),fields[2].c_str(),fields[1].c_str(),scene.Snapshot(result).c_str(),scene.trace.c_str());
        CHECK(scene.Snapshot(result)==fields[2]);CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[1]);CHECK(scene.state.description==fields[3]);++count;
    }
    CHECK(count==573);
}
// 두 호출자의 준비가 같은 상태를 사용하고 누락된 판정이 성공으로 바뀌지 않는지 확인한다.
TEST_CASE(CursorPlacement_BindsSharedLifecycleAndRequiresRealDependencies) {
    PlacementScene scene;scene.Prepare({1,0,0,0,2,3,16,1,0,0,0,0,1,0,4,0,0,0,0,0,1,0,0});
    RawCursorPlacement placement(scene.pool,scene.types,scene.surface,scene.drop,scene.lifecycle,scene.state,scene.Hooks());CursorLifecycleHooks lifecycle;CursorDropHooks drop;
    BindCursorPlacement(lifecycle,drop,placement);lifecycle.prepareCursor();const auto first=scene.trace;scene.trace.clear();drop.prepareCursor();CHECK(scene.trace==first);CHECK(&placement.Pool()==&scene.pool);
    RawCursorPlacement missing(scene.pool,scene.types,{},scene.drop,scene.lifecycle,scene.state,{});bool failed{};
    try { missing.Query(); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
    scene.drop.inputFlags=0x20;scene.drop.costFlags=0;CHECK(missing.SourceAllowed()==1);scene.drop.costFlags=0x20;failed=false;
    try { missing.SourceAllowed(); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
}
