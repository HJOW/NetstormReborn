// 커서 취득/취소/삭제의 실제565고유 입력과 전체 상태·raw WORD·가상 효과 순서를 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCursorLifecycle.h"
#include "client/GameAssets.h"
#include <filesystem>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 독립 관찰의 입력 열과 다섯 측정 경계 입력이다.
enum Column : std::size_t { Operation,Flags,OldWord,Argument,TypeFlags,CostBits,Lock,Preview,Dead,ParentFlags,
    HighlightChange,PrepareChange,Mana,Range,LegalGump,ManaGump,ImageVariant };
constexpr std::array<std::int32_t,5> kWidths{20,33,-3,120,55},kHeights{10,30,-1,20,17}; // 명시 측정 입력이며 기대 결과가 아니다.
// 저장된 쉼표 DWORD를 읽는다. 출력값을 재계산하지 않는다.
std::vector<std::uint32_t> Numbers(std::string_view value) {
    std::vector<std::uint32_t> result;
    // 원문 순서대로 모든 열을 보존한다.
    for (const auto& field:Split(value,',')) result.push_back(Number(field));return result;
}
// 전체 커서 원문을 같은 저장소에 재생하는 격리 장면이다.
struct CursorScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // Form/부모/임시 객체의 단일 raw 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 같은 현재 TYPE 입력이다.
    CursorDropState drop; // 내려놓기와 공유하는 전역이다.
    CursorLifecycleState state; // 나머지 현재 커서 수명 전역이다.
    std::vector<std::uint32_t> input; // 저장 관찰의17개 명시 입력이다.
    std::string trace; // 가상 효과/측정의 실제 호출 순서다.
    std::size_t prepared{},measured{}; // 현재 경계 호출 횟수다.
    // 모든 raw 입력 슬롯을 실제 Pool에서 확보한다.
    CursorScene() {
        // 반환값은 실제 client 공간의 번호이며 강제 번호 사본을 만들지 않는다.
        for (std::uint16_t sid=5;sid<=101;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 명시 효과 사건을 원문과 같은 인자 순서로 덧붙인다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // signed 비트도 DWORD 십진수로 표시한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 독립 원문 입력만 준비한다. 이전 실행의 결과는 이어받지 않는다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;drop={};state={};trace.clear();prepared=measured=0;
        // 원본 입력의 무관한 바이트 표식을 동일하게 채운다.
        for (const auto sid:{50,51,77,100,101}) {
            const auto slot=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()}; // 독립 입력의 dead 표식도 초기화하는 검사 전용 접근이다.
            std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,1);
            raw[11]=static_cast<std::uint8_t>(sid==50 || sid==51 ? values[Dead] : sid==77 ? values[ParentFlags] : 0);
            Put(raw,12,values[Argument],2);Put(raw,18,100,2);Put(raw,20,values[OldWord],2);
        }
        types[100].flags2=values[TypeFlags];types[100].cost=std::bit_cast<float>(values[CostBits]);drop.type=state.abstractType=100;
        drop.lockDepth=values[Lock];drop.inputFlags=values[OldWord];drop.costFlags=values[Flags];drop.preview=values[Operation]==1 ? values[Preview] : 0U;
        drop.cursorGump=values[Operation]==1 ? 1U : 0U;state.previewCount=values[Operation]==2 ? std::bit_cast<std::int32_t>(values[Preview]) : 2;
        state.previews[0]=100;state.previews[1]=101;state.warningFont=17;state.costFont=19;
    }
    // 원본 실행기와 같은 외부 입력/효과만 공급한다. 수명 분기는 C++ 몸체가 계산한다.
    CursorLifecycleHooks Hooks() {
        CursorLifecycleHooks hooks;
        hooks.highlight=[this](std::uint32_t value) { Event("H",{value});if (input[HighlightChange]==1) drop.lockDepth=3;if (input[HighlightChange]==2) drop.preview=51U; };
        hooks.prepareCursor=[this] { Event("P");if (input[PrepareChange] && ++prepared==1) state.previewCount=1; };
        hooks.selectCursor=[this](std::uint32_t cursor) { Event("C",{cursor}); };hooks.closePopups=[this] { Event("X"); };
        hooks.source=[this](Sid sid) { Event("U",{sid.value});return Sid{77}; };
        hooks.sourceNotice=[this](Sid source,Sid preview,std::uint32_t flags) { Event("N",{source.value,preview.value,flags}); };
        hooks.detach=[this](Sid sid,std::uint32_t flags) { Event("D",{sid.value,flags}); };hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("O",{sid.value,flags}); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event("K",{sid.value,flags}); };hooks.removeProcess=[this](std::uint32_t process,std::uint32_t flags) { Event("R",{process,flags}); };
        hooks.sound=[this](std::string_view name) { Event(std::string("S:")+std::string(name),{0,0,0,0,0}); };hooks.addLocalSp=[this](float delta) { Event("A",{std::bit_cast<std::uint32_t>(delta)}); };
        hooks.defaultArgument=[](std::uint32_t type) { CHECK(type==100);return 11U; };
        hooks.createPreviews=[this] { Event("B");state.previews[0]=100;state.previews[1]=101;state.previewCount=2;drop.legalGump=input[LegalGump] ? 44U : 0U; };
        hooks.createMana=[this](Sid first,std::uint32_t flags) { Event("M",{first.value,flags});state.currentGump=input[ManaGump] ? 45U : 0U;return input[Mana] ? 1U : 0U; };
        hooks.createRange=[this](Sid first,std::uint32_t flags) { Event("G",{first.value,flags});return 2U; };
        hooks.processEvent=[this](std::uint32_t process,std::uint32_t event,std::uint32_t first,std::uint32_t second) { Event("E",{process,event,first,second}); };
        // 기존565관측은 getter를 외부 입력1로 바꿨다. 실제 번호/판정의 근거는 별도854원문이다.
        hooks.typeNumber=[this](std::uint32_t type) { Event("F",{type});return 1U; };hooks.hasRange=[this](std::uint32_t typeNumber) { Event("L",{typeNumber});return input[Range]!=0; };
        hooks.queryCursor=[this] { Event("Q");state.currentGump=46; };hooks.translate=[](std::string_view text) { return std::string(text); };
        hooks.measure=[this](std::string_view text,const std::array<std::uint32_t,8>& context) {
            std::string words;
            // 현재32바이트 문맥의 모든 필드를 비교한다.
            for (const auto value:context) words+=(words.empty() ? "" : ",")+std::to_string(value);
            Event(std::string("T:")+std::string(text)+':'+words,{0xffffffffU});const auto index=measured++;
            return index<5 ? std::array{kWidths[index],kHeights[index]} : std::array{37,input[ImageVariant] ? -9 : 12};
        };
        hooks.costImage=[this](std::uint32_t type,std::uint32_t frame) { CHECK(type==92 && frame==3);return CursorCostImage{1,static_cast<std::int16_t>(input[ImageVariant] ? -11 : 23),static_cast<std::int16_t>(input[ImageVariant] ? -7 : 19),-3,5}; };
        hooks.mouse=[] { return ScreenPoint{-3,47}; };hooks.updatePosition=[this](std::int32_t x,std::int32_t y) { Event("Z",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}); };
        return hooks;
    }
    // 원본과 같은78DWORD의 현재 상태를 의미 표현에서 그대로 직렬화한다.
    std::string Snapshot() const {
        std::vector<std::uint32_t> values{drop.lockDepth,drop.type,drop.argument,drop.direction,state.directionArgument,drop.preview,drop.inputFlags,drop.costFlags,drop.cursorGump,drop.legalGump,state.currentGump,
            static_cast<std::uint32_t>(state.previewCount),static_cast<std::uint32_t>(state.warningWidth),static_cast<std::uint32_t>(state.warningHeight),static_cast<std::uint32_t>(state.costTextWidth),static_cast<std::uint32_t>(state.costHeight),
            static_cast<std::uint32_t>(state.costHotspotX),static_cast<std::uint32_t>(state.costHotspotY),static_cast<std::uint32_t>(drop.cursorWidth),static_cast<std::uint32_t>(state.imageHeight)};
        // 다섯 개별 치수·두 문맥·30SID의 사용하지 않은 칸까지 비교한다.
        for (const auto value:state.warningWidths) values.push_back(static_cast<std::uint32_t>(value));
        for (const auto value:state.warningHeights) values.push_back(static_cast<std::uint32_t>(value));
        for (const auto value:state.warningContext) values.push_back(value);
        for (const auto value:state.costContext) values.push_back(value);
        for (const auto sid:state.previews) values.push_back(sid);
        values.push_back(state.abstractType);values.push_back(state.costImage);std::string result;
        // 원문의 DWORD 순서를 유지한다.
        for (const auto value:values) result+=(result.empty() ? "" : ",")+std::to_string(value);return result;
    }
};
}
// 실제565입력의 정상 반환/78DWORD/raw150바이트·모든 효과를 기대값 재계산 없이 대조한다.
TEST_CASE(CursorLifecycle_NativeTakeCancelAndActivate565) {
    CursorScene scene;std::size_t count=0;
    // 두 정밀도에서 일치했던 저장 원문만 읽는다. 테스트는 PE/Python/게임을 실행하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_LIFECYCLE_FIXTURE).rows) {
        if (row.size()!=7) throw std::runtime_error("커서 수명 fixture 열 수 오류");const auto input=Numbers(row[0]);if (input.size()!=17) throw std::runtime_error("커서 수명 입력 열 수 오류");scene.Prepare(input);
        RawCursorLifecycle logic(scene.pool,scene.types,scene.drop,scene.state,scene.Hooks());
        if (!input[Operation]) logic.Take(Sid{50},input[Flags]);else if (input[Operation]==1) logic.Cancel();else logic.Activate(input[Lock]);
        if (scene.trace.empty()) scene.trace="-";const auto snapshot=scene.Snapshot();const bool same=scene.trace==row[1] && snapshot==row[2] && scene.state.costText==row[3];CHECK(same);
        if (!same) { std::printf("커서 수명 행%zu 입력%s\n기대%s / %s\n실제%s / %s\n",count,row[0].c_str(),row[1].c_str(),row[2].c_str(),scene.trace.c_str(),snapshot.c_str());break; }
        CHECK(Hex(scene.pool.Slot(Sid{50}))==row[4]);CHECK(Hex(scene.pool.Slot(Sid{51}))==row[5]);CHECK(Hex(scene.pool.Slot(Sid{77}))==row[6]);++count;
    }
    CHECK(count==565);
}
// 연결 어댑터가 취득/취소/내려놓기의 같은 상태에 쓰며 누락 경계와 잘못된 판본은 생략하지 않는다.
TEST_CASE(CursorLifecycle_SharedMenuDropBindingsAndGuards) {
    CursorScene scene;const auto rows=LoadFixture(NETSTORM_CURSOR_LIFECYCLE_FIXTURE).rows;scene.Prepare(Numbers(rows.front()[0]));RawCursorLifecycle logic(scene.pool,scene.types,scene.drop,scene.state,scene.Hooks());
    PriestMenuHooks priest;CursorDropHooks drop;BindCursorLifecycle(priest,drop,logic);priest.takePreview(Sid{50},0x160);
    CHECK(scene.drop.preview==50U && scene.drop.lockDepth==0 && scene.drop.costFlags==0x160 && scene.state.previewCount==2 && &logic.Pool()==&scene.pool);
    drop.activatePreview(0);CHECK(scene.state.previewCount==2);drop.invalidGui();CHECK(scene.drop.preview==0 && scene.drop.lockDepth==1 && scene.state.previewCount==0);
    scene.Prepare(Numbers(rows.front()[0]));RawCursorLifecycle missing(scene.pool,scene.types,scene.drop,scene.state,{});CHECK(Throws([&] { missing.Take(Sid{50},0x160); }));
    SidPool cd(OriginalEdition::Cd1072,24000,false);CHECK(Throws([&] { RawCursorLifecycle invalid(cd,scene.types,scene.drop,scene.state,scene.Hooks()); }));
    CHECK(Throws([&] { RawCursorLifecycle invalid(scene.pool,std::span(scene.types).first(187),scene.drop,scene.state,scene.Hooks()); }));
    scene.state.previewCount=31;CHECK(Throws([&] { logic.Activate(1); }));
}
// 실제 보호 자산의 기본 프레임·비용 SHP 헤더와 기존 글꼴 측정기로 연결한다. 파일은 읽기만 한다.
TEST_CASE(CursorLifecycle_ProtectedAssetsAndActualText) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");GameAssets assets(files,OriginalEdition::Patch1078);
    FontStore fonts(files,"Arial");GumpText text(fonts);CursorLifecycleHooks hooks;BindCursorLifecycleAssets(hooks,assets,text);
    CHECK(hooks.defaultArgument(123)==static_cast<std::uint32_t>(assets.Types()[123-70].definition.specialFrames.defaultFrame));
    const auto image=hooks.costImage(92,3);const auto metrics=assets.Shapes().SquidMetrics(assets.Types()[92-70].block,3);
    CHECK(image.token==static_cast<std::uint32_t>(assets.Types()[92-70].block+1) && image.width==metrics.width && image.height==metrics.height && image.hotspotX==metrics.hotspotX && image.hotspotY==metrics.hotspotY);
    CursorLifecycleState state;const auto measured=hooks.measure("123",state.costContext);std::int32_t width{},height{};text.Measure(std::span("123",4),DecodeGumpTextContext(state.costContext),width,height,-1);CHECK(measured[0]==width && measured[1]==height);
    CHECK(Throws([&] { hooks.defaultArgument(69); }));CHECK(Throws([&] { hooks.typeNumber(188); }));CHECK(Throws([&] { hooks.costImage(92,10000); }));
}

// 실제49adc0/474240/49a840의 저장 원문을 사용하며 출력 판정을 테스트에서 재계산하지 않는다.
TEST_CASE(CursorLifecycle_NativeTypeNumberAndRange) {
    std::vector<RiftTypeRecord> types(188); // 각 독립 입력의 현재 group/flags2 저장소다.
    std::size_t count{}; // 실제 고유 입력의 대조 수다.
    const auto fixture=std::filesystem::path(NETSTORM_CURSOR_LIFECYCLE_FIXTURE).parent_path()/"cursor-type-range-x86.tsv"; // 기존 수명 기대값과 구별한다.
    // 입력을 같은 TYPE 칸에 덮어써 현재 값 변경도 이전 판정에 묶이지 않게 한다.
    for (const auto& row:LoadFixture(fixture.string().c_str()).rows) {
        CHECK(row.size()==5);const auto type=Number(row[0]);types[type].group=std::bit_cast<std::int32_t>(Number(row[1]));types[type].flags2=Number(row[2]);
        CHECK(type==Number(row[3]));CHECK(HasCursorRange(types,type)==(Number(row[4])!=0));++count;
    }
    CHECK(count==854);CHECK(Throws([&] { HasCursorRange(types,188); }));
    CHECK(Throws([&] { HasCursorRange(std::span<const RiftTypeRecord>{},0); }));
}
// 자산 연결기가 SHP 토큰 대신 실제 TYPE 번호를 사정거리 판정에 전달하고 Take가 그 판정을 소비한다.
TEST_CASE(CursorLifecycle_ActualAssetTypeRangeDuringTake) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"; // 읽기 전용 보호 자산의 루트다.
    BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");GameAssets assets(files,OriginalEdition::Patch1078);
    FontStore fonts(files,"Arial");GumpText text(fonts);CursorLifecycleHooks actual;BindCursorLifecycleAssets(actual,assets,text);
    // 내장 타입0~69도 번호 변환은 유효하며 자산 블록을 요구하지 않는다.
    for (std::uint32_t type=0;type<188;++type) CHECK(actual.typeNumber(type)==type);
    CHECK(actual.hasRange(actual.typeNumber(128)));CHECK(!actual.hasRange(actual.typeNumber(92)));
    CHECK(Throws([&] { actual.hasRange(188); }));
    CursorScene scene;scene.Prepare(Numbers(LoadFixture(NETSTORM_CURSOR_LIFECYCLE_FIXTURE).rows.front()[0]));
    scene.types.assign(assets.TypeTable().Types().begin(),assets.TypeTable().Types().end());Put(scene.pool.AllocatedBytes(Sid{50}),18,128,2);
    auto hooks=scene.Hooks();hooks.defaultArgument=actual.defaultArgument;hooks.typeNumber=actual.typeNumber;hooks.hasRange=actual.hasRange; // 외부 생성 효과만 기존 명시 관측 경계를 유지한다.
    RawCursorLifecycle logic(scene.pool,scene.types,scene.drop,scene.state,hooks);logic.Take(Sid{50},0x160);
    CHECK(scene.state.abstractType==128 && scene.drop.type==128 && scene.drop.cursorGump==2);
    CHECK(scene.trace.find("G:100:0")!=std::string::npos);
}
