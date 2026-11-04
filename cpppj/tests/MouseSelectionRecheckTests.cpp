// 원문004da650의기억점검색/현재선택관문/선택거부·재지정뒤원래found재검사를대조한다.
#include "TestSupport.h"
#include "client/RawUserSelection.h"
#include <algorithm>
#include <bit>
#include <memory>
#include <stdexcept>

using namespace netstorm::o;
using namespace netstorm::client;
namespace { // 실제선택부모를사용하고큰외부효과만명시관측한다.
struct RecheckScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; //통신을사용하지않는실제풀이다.
    Sid first{pool.Allocate(2)},second{pool.Allocate(2)}; //검색결과와제단재지정결과를구별한다.
    std::array<RiftTypeRecord,188> types{}; //실제선택의현재TYPE배열이다.
    std::uint32_t selected{},mouse{74},paused{},finds{},dumps{},notices{},sounds{}; //현재선택전역과관측이다.
    std::array<std::uint32_t,2> point{std::bit_cast<std::uint32_t>(12.5F),std::bit_cast<std::uint32_t>(13.25F)}; //정수보정없이검색할저장float비트다.
    std::array<std::uint32_t,0> multiple{};std::int32_t count{}; //선택과공유하는빈다중목록이다.
    std::unique_ptr<RawUserSelection> selection; //같은실제부모를각사례의명시상태에연결한다.
    std::span<std::uint8_t> Raw(Sid sid) { return pool.AllocatedBytes(sid); } //유효명시슬롯만수정한다.
    RecheckScene() { //정상Select와paused/다이스재지정에서요구되는외부경계를준비한다.
        for (const auto sid:{first,second}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;for (std::size_t index=0;index<4;++index) { raw[14+index]=static_cast<std::uint8_t>(point[0]>>(index*8));raw[18+index]=static_cast<std::uint8_t>(point[1]>>(index*8)); } } //두객체는같은기억좌표이며필드폭을그대로쓴다.
        UserSelectionHooks hooks;hooks.paused=[this] { return paused; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };hooks.dais=[] { return 165U; };hooks.manaProcess=[] { return 50U; };hooks.rangeProcess=[] { return 51U; };hooks.rangeType=[](auto) { return 0U; };
        hooks.dump=[this](Sid) { ++dumps; };hooks.selectNotice=[this](Sid,std::uint32_t) { ++notices; };hooks.refresh=[](Sid) {};hooks.processNotice=[](Sid,auto,auto,auto) {};hooks.sound=[this](auto,auto,auto,auto) { ++sounds; };hooks.altar=[this](Sid sid) { CHECK(sid==first);return second; };
        selection=std::make_unique<RawUserSelection>(pool,types,UserSelectionState{selected,mouse,point,multiple,count},std::move(hooks));
    }
    std::uint32_t Find(std::uint32_t x,std::uint32_t y,std::uint32_t type) { ++finds;CHECK(x==point[0] && y==point[1] && type==mouse);return first.value; } //검색계산자체는실제Session통합검사에서검증한다.
    void Run() { selection->RecheckMouse([this](auto x,auto y,auto type) { return Find(x,y,type); }); } //실제선택과같은현재저장소를사용한다.
};
}

TEST_CASE(MouseSelectionRecheck_ZeroTypeDoesNotRequireFinder) { //마우스TYPE0이면검색/Select장치를조회하지않는다.
    RecheckScene scene;scene.mouse=0;scene.selection->RecheckMouse({});CHECK(scene.finds==0 && scene.dumps==0 && scene.mouse==0);
}
TEST_CASE(MouseSelectionRecheck_MissingResultPreservesCurrentMutation) { //검색하위가바꾼현재마우스TYPE를복구하지않는다.
    RecheckScene scene;scene.selection->RecheckMouse([&](auto x,auto y,auto type) { CHECK(scene.Find(x,y,type)==scene.first.value);scene.mouse=75;return 0U; });CHECK(scene.finds==1 && scene.dumps==0 && scene.mouse==75);
}
TEST_CASE(MouseSelectionRecheck_CurrentDifferentSelectionBlocksAfterFind) { //검색중변경된현재선택이found와다르면Select를생략한다.
    RecheckScene scene;scene.selection->RecheckMouse([&](auto x,auto y,auto type) { const auto found=scene.Find(x,y,type);scene.selected=scene.second.value;return found; });CHECK(scene.finds==1 && scene.dumps==0 && scene.selected==scene.second.value && scene.mouse==74);
}
TEST_CASE(MouseSelectionRecheck_PausedSelectStillClearsOrdinaryFound) { //Select의거부0반환도원래found상태검사/tail을생략하지않는다.
    RecheckScene scene;scene.paused=1;scene.Run();CHECK(scene.finds==1 && scene.dumps==0 && scene.selected==0 && scene.mouse==0);
}
TEST_CASE(MouseSelectionRecheck_OrdinaryFoundIsActuallySelectedBeforeClear) { //실제Select(0,1)의가상통지/음향을거친뒤마우스TYPE만지운다.
    RecheckScene scene;scene.Run();CHECK(scene.finds==1 && scene.dumps==1 && scene.notices==1 && scene.sounds==1 && scene.selected==scene.first.value && scene.mouse==0);
}
TEST_CASE(MouseSelectionRecheck_RedirectedSelectUsesOriginalFoundExtra) { //다이스가제단으로선택을재지정해도검색결과의현재extra를읽는다.
    for (const auto extra:{0U,1U}) { RecheckScene scene;scene.Raw(scene.first)[10]=165;scene.Raw(scene.first)[40]=static_cast<std::uint8_t>(extra);scene.Raw(scene.second)[40]=static_cast<std::uint8_t>(extra^1U);scene.Run();CHECK(scene.selected==scene.second.value && scene.mouse==(extra ? 74U : 0U) && scene.dumps==2 && scene.sounds==1); } //원래found와반환대상의상태를반대로둔다.
}
