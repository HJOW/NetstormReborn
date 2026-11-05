// 실제 내부 부모 관측과 동일 슬롯/선택 TYPE의 표시 연결만 검사한다. 게임/MP는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "client/SpriteDraw.h"
#include "client/SpriteHitPointBar.h"
#include "o/SquidFactory.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 실제 SpriteDraw의 선택 분기만 실행하는 작은 원문 입력이다.
struct BarScene { // 현재 raw 슬롯/TYPE/선택 헤더와 출력 기록을 같은 수명에 둔다.
    SidPool pool; // 창 없이 사용하는 실제 판본별 슬롯이다.
    std::vector<RiftTypeRecord> types; // 선택 TYPE와 공장 getter가 공유하는 물리 표다.
    std::vector<SpriteTypeView> views; // 실제 선택 헤더와 프레임 수다.
    SpriteDrawState state; // 색/선택/타이머 전역 입력이다.
    std::vector<std::pair<ScreenRect,std::uint32_t>> fills; // 실제 픽셀 장치 경계의 순서 있는 출력이다.
    std::uint32_t lines{}; // HP 실패 앞의 여덟 선택선을 확인한다.
    BarScene(OriginalEdition edition):pool(edition,24000,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),views(types.size()) { // 실제 Allocate 뒤 검증 입력만 설치한다.
        CHECK(pool.Allocate(2)==Sid{5});auto raw=pool.AllocatedBytes(Sid{5});raw[10]=70;raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=4;Put(raw,26,50,edition==OriginalEdition::Patch1078 ? 4 : 2);
        types[70].flags1=16;types[70].maxHitPoints=100;views[70].shape={1,true,{{4,4,1,1}}};state.ownerMaps.resize(types.size());state.ownerPalettes={0};state.selectionColors={7};state.shadowAliasType=69;state.timerTypes={69,69,69};state.hitPointColors={11,22,33};
    }
    SpriteDrawHooks Hooks() { // 기하/색을 기록하고 관련 없는 규칙은 명시 빈 결과로 공급한다.
        SpriteDrawHooks hooks;hooks.shape=[](auto) {};hooks.virtualFrame=[](auto) { return -1; };hooks.virtualDraw=[](auto,auto) {};hooks.line=[this](auto,auto,auto) { ++lines; };hooks.fill=[this](auto rect,auto color) { fills.emplace_back(rect,color); };hooks.resource=[](auto)->std::optional<SpriteResourceStatus> { return {}; };return hooks;
    }
    void Draw(SpriteDrawHooks hooks) { SpriteDraw draw(pool,types,views,state,std::move(hooks),0);draw.Draw({0,0,static_cast<std::uint32_t>(5*pool.Layout().stride),0,0,0}); } // raw0좌표/화면0/프레임0의 선택 그림을 실행한다.
};
}

TEST_CASE(SpriteHitPointBar_Native504CurrentReadsColorsAndConditionalCount) { // 원문 구간의 모든 추적/색/현재값/최대값/최종 HP를 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_SPRITE_HP_BAR_FIXTURE).rows) { // 기존 구현으로 기대값을 만들지 않고 동결 관측을 읽는다.
        CHECK(row.size()==6);const auto input=Split(row[1],',');CHECK(input.size()==6);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        auto hp=std::stoi(input[0]);if (edition==OriginalEdition::Cd1072) hp=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(hp));const std::array<std::int32_t,3> maxima{std::stoi(input[1]),std::stoi(input[2]),std::stoi(input[3])};
        const bool mutate=Number(input[4])!=0,recolor=Number(input[5])!=0;std::array<std::uint32_t,3> colors{11,22,33};std::size_t calls{};std::string trace;
        const auto emit=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 원문 current 읽기와 getter 호출만 같은 순서로 기록한다.
        SpriteHitPointBarHooks hooks;hooks.current=[&] { emit("H"+std::to_string(hp));return hp; };
        hooks.maximum=[&] { CHECK(calls<3);const auto value=maxima[calls++];emit("M"+std::to_string(value));if (mutate) hp=std::array<std::int32_t,3>{-5,17,200}[calls-1];if (recolor) colors={static_cast<std::uint32_t>(100+calls),static_cast<std::uint32_t>(200+calls),static_cast<std::uint32_t>(300+calls)};return value; };
        const auto result=ReadSpriteHitPointBar(edition,colors,hooks);const auto actual=std::to_string(result.color)+','+std::to_string(result.current)+','+std::to_string(result.maximum);
        CHECK(actual==row[2] && trace==row[3] && hp==std::stoi(row[4]) && calls==Number(row[5]));++rows;
    }
    CHECK(rows==504);
}

TEST_CASE(SpriteHitPointBar_ActualDrawUsesSelectedFactoryTypeAcrossThreeDiagnoses) { // 진단이 raw TYPE/HP/최대HP/색을 바꿔도 저장된 TYPE this의 세 읽기를 유지한다.
    BarScene scene(OriginalEdition::Patch1078);scene.types[70].flags1=0;scene.types[70].flags2=0x400000;auto raw=scene.pool.AllocatedBytes(Sid{5});raw[40]|=1;std::uint32_t checking=0x80000000,calls{};SquidFactory factory(scene.pool,scene.types);
    factory.BindDiagnostics({scene.types,[&](auto expression,auto file,auto line) { CHECK(expression=="(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType" && file=="\\Ns\\O\\Rifttype.cpp" && line==0x1e3);++calls;scene.types[70].maxHitPoints=std::array<std::int32_t,3>{40,80,120}[calls-1];Put(raw,26,std::array<std::uint32_t,3>{17,200,300}[calls-1]);raw[10]=71;scene.state.hitPointColors={100+calls,200+calls,300+calls}; },{},checking});
    auto hooks=scene.Hooks();hooks.typeHitPoints=[&](auto number) { CHECK(number==70);return factory.ReadSelectedHitPoints(scene.types[number],scene.types); };scene.Draw(std::move(hooks));
    CHECK(calls==3 && scene.lines==8 && scene.fills.size()==2 && scene.fills[1].second==302 && scene.fills[1].first.right-scene.fills[1].first.left==4 && Get(raw,26)==300 && raw[10]==71);
}

TEST_CASE(SpriteHitPointBar_CdCallsAgainOnlyForPositiveFirstMaximum) { // 첫0은기본1, 첫양수는두번째실제값으로CD막대를그린다.
    for (const auto first:{0,100}) { // 두 정상 경로 모두 signed WORD HP50을 최대값으로 clamp한다.
        BarScene scene(OriginalEdition::Cd1072);auto hooks=scene.Hooks();std::uint32_t calls{};hooks.typeHitPoints=[&](auto number) { CHECK(number==70);return ++calls==1 ? first : 40; };scene.Draw(std::move(hooks));
        CHECK(calls==(first ? 2U : 1U) && scene.lines==8 && scene.fills.size()==2 && scene.fills[1].first.right-scene.fills[1].first.left==19);
    }
}

TEST_CASE(SpriteHitPointBar_NoHpSkipsGetterAndCdZeroKeepsBackground) { // 미선택 getter를 요구하지 않고 CD의 나눗셈 실패 앞 출력을 보존한다.
    BarScene absent(OriginalEdition::Patch1078);absent.types[70].flags1=0;auto hooks=absent.Hooks();hooks.typeHitPoints=[](auto)->std::int32_t { throw std::logic_error("미선택 HP getter"); };absent.Draw(std::move(hooks));CHECK(absent.lines==8 && absent.fills.empty());
    BarScene cd(OriginalEdition::Cd1072);auto cdHooks=cd.Hooks();std::uint32_t calls{};cdHooks.typeHitPoints=[&](auto) { return ++calls==1 ? 1 : 0; };CHECK(Throws([&] { cd.Draw(std::move(cdHooks)); }));CHECK(calls==2 && cd.lines==8 && cd.fills.size()==1);
}

TEST_CASE(SpriteHitPointBar_MissingReadsKeepSelectedPrefix) { // 부모의 실제 읽기 순서를 실패 경계에서도 보존한다.
    std::array<std::uint32_t,3> colors{11,22,33};SpriteHitPointBarHooks hooks;std::string trace;hooks.current=[&] { trace+='H';return 9; };
    CHECK(Throws([&] { ReadSpriteHitPointBar(OriginalEdition::Patch1078,colors,hooks); }) && trace=="H");trace.clear();CHECK(Throws([&] { ReadSpriteHitPointBar(OriginalEdition::Cd1072,colors,hooks); }) && trace.empty());
}
