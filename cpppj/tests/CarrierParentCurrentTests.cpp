// Carrier/Damageable의선택된장치·현재TYPE/로딩·실패앞부분효과를창없이검사한다.
#include "TestSupport.h"
#include "o/RawCarrierPostPop.h"
#include <algorithm>
#include <bit>
#include <memory>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace { //원문하위경계만관측하며상위분기/결과를대신계산하지않는다.
template<class F> bool CarrierFailed(F&& action) { try { action(); }catch (const std::exception&) { return true; }return false; } //실패뒤이미쓴값과미도달경계를대조한다.
struct CarrierCurrentScene { //같은물리슬롯/현재타입/장치상태를처리기보다오래보관한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false}; //통신을사용하지않는실제client풀이다.
    Sid sid{pool.Allocate(2)}; //모든현재필드가가리키는한슬롯이다.
    std::array<RiftTypeRecord,188> types{}; //대여뒤필드만수정하고재할당하지않는다.
    CarrierPostPopState state; //현재권한/로딩/간헐천TYPE입력이다.
    std::vector<std::string> events; //실제선택된check/ground/base순서다.
    std::function<void()> onCheck,onGround,onBase; //해당경계의명시변경또는중단이다.
    std::array<float,2> point{};std::uint8_t owner{};Sid lastSid{}; //지면인자와공통몸체의실제SID를보관한다.
    CarrierPostPopHooks hooks; //상위대신실제RawCarrierPostPop에공급하는장치다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } //free/contained변경뒤에도같은물리를읽는다.
    void Write(std::size_t offset,std::uint32_t value) { auto raw=Raw();for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); } //원문DWORD하나만쓴다.
    CarrierCurrentScene() { //현재조건의기본명시입력과관측장치를준비한다.
        auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;raw[34]=3;Write(14,std::bit_cast<std::uint32_t>(30.25F));Write(18,std::bit_cast<std::uint32_t>(40.5F));
        hooks.carrierCheck=[this](Sid value) { CHECK(value==sid);events.push_back("check");const auto action=onCheck;if (action) action(); };
        hooks.claimGround=[this](float x,float y,std::uint8_t value) { point={x,y};owner=value;events.push_back("ground");const auto action=onGround;if (action) action(); };
        hooks.validateBase=[](Sid,std::uint32_t) { throw std::logic_error("실제PATCH의미도달전체사전검사"); };
        hooks.base=[this](Sid value,std::uint32_t) { lastSid=value;events.push_back("base");const auto action=onBase;if (action) action(); };
    }
    RawCarrierPostPop Make() { return RawCarrierPostPop(pool,types,state,hooks); } //독립소유사본으로만들고필요한사례에서현재배열을명시대여한다.
};
}

TEST_CASE(CarrierParentCurrent_OnlyReachedBaseRequiresDevice) { //로딩0/권한있음이면미연결check/ground/validate는요구하지않는다.
    CarrierCurrentScene scene;scene.hooks.carrierCheck={};scene.hooks.claimGround={};scene.hooks.validateBase={};auto post=scene.Make();post.PostPop(scene.sid,0);CHECK(scene.events==(std::vector<std::string>{"base"}));
}
TEST_CASE(CarrierParentCurrent_CheckRunsBeforeMissingCommonDevice) { //공통장치가없어도앞서도달한가상확인효과를보존한다.
    CarrierCurrentScene scene;scene.state.boss=false;scene.hooks.base={};scene.onCheck=[&] { scene.Raw()[34]=5; };auto post=scene.Make();CHECK(CarrierFailed([&] { post.PostPop(scene.sid,0); }) && scene.Raw()[34]==5 && scene.events==(std::vector<std::string>{"check"}));
}
TEST_CASE(CarrierParentCurrent_CheckFailureStopsBeforeDamageable) { //가상확인이중단하면뒤의현재타입/공통몸체를실행하지않는다.
    CarrierCurrentScene scene;scene.state.boss=false;scene.Raw()[10]=255;scene.onCheck=[] { throw std::logic_error("명시가상확인중단"); };auto post=scene.Make();CHECK(CarrierFailed([&] { post.PostPop(scene.sid,0); }) && scene.events==(std::vector<std::string>{"check"}));
}
TEST_CASE(CarrierParentCurrent_FirstFlagReadsTypeBeforeZeroLoading) { //반환을버리는004ad080도현재TYPE물리범위접근을생략하지않는다.
    CarrierCurrentScene scene;scene.Raw()[10]=255;auto post=scene.Make();CHECK(CarrierFailed([&] { post.DamageablePostPop(scene.sid,1); }) && scene.events.empty());post.DamageablePostPop(scene.sid,0);CHECK(scene.events==(std::vector<std::string>{"base"}));
}
TEST_CASE(CarrierParentCurrent_FirstFlagSkipsMissingVirtualCheck) { //첫flags1은boss가거짓이어도가상cc를호출하지않는다.
    CarrierCurrentScene scene;scene.state.boss=false;scene.hooks.carrierCheck={};auto post=scene.Make();post.PostPop(scene.sid,1);CHECK(scene.events==(std::vector<std::string>{"base"}));
}
TEST_CASE(CarrierParentCurrent_ConditionalTypeReadFollowsGeyserAndExtra) { //로딩중이어도현재간헐천/extra관문이면flags1접근을생략한다.
    CarrierCurrentScene scene;scene.state.loadingDepth=1;scene.state.geyserType=255;scene.Raw()[10]=255;auto post=scene.Make();post.DamageablePostPop(scene.sid,0);scene.state.geyserType=122;scene.Raw()[40]=1;post.DamageablePostPop(scene.sid,0);scene.Raw()[40]=0;CHECK(CarrierFailed([&] { post.DamageablePostPop(scene.sid,0); }) && scene.events==(std::vector<std::string>{"base","base"}));
}
TEST_CASE(CarrierParentCurrent_CheckChangesCurrentTypeFlagsAndLoading) { //가상확인뒤현재대여TYPE/깊이/좌표/owner를같은실제몸체에서읽는다.
    CarrierCurrentScene scene;scene.state.boss=false;std::uint32_t depth{};scene.state.sharedLoadingDepth=&depth;auto post=scene.Make();post.BindCurrentTypes(scene.types);
    scene.onCheck=[&] { depth=1;scene.Raw()[10]=75;scene.Raw()[34]=5;scene.types[75].flags1=0x400;scene.Write(14,std::bit_cast<std::uint32_t>(50.25F));scene.Write(18,std::bit_cast<std::uint32_t>(60.5F)); };
    post.PostPop(scene.sid,0);CHECK(scene.events==(std::vector<std::string>{"check","ground","base"}) && scene.point==(std::array<float,2>{50.25F,60.5F}) && scene.owner==5 && scene.state.loadingDepth==0);
}
TEST_CASE(CarrierParentCurrent_CurrentPointersOverrideFallbackCopies) { //같은현재깊이와간헐천TYPE를바꾸면값형사본과무관하게다음관문이바뀐다.
    CarrierCurrentScene scene;scene.types[74].flags1=0x400;std::uint32_t depth=1,geyser=74;scene.state.sharedLoadingDepth=&depth;scene.state.sharedGeyserType=&geyser;auto post=scene.Make();post.DamageablePostPop(scene.sid,0);geyser=122;post.DamageablePostPop(scene.sid,0);depth=0;post.DamageablePostPop(scene.sid,0);CHECK(scene.events==(std::vector<std::string>{"base","ground","base","base"}) && scene.state.loadingDepth==0 && scene.state.geyserType==122);
}
TEST_CASE(CarrierParentCurrent_GroundFailureKeepsEarlierCheckMutation) { //지면하위가중단해도먼저쓴가상확인효과는남는다.
    CarrierCurrentScene scene;scene.state.boss=false;scene.state.loadingDepth=1;scene.types[74].flags1=0x400;scene.onCheck=[&] { scene.Raw()[34]=5; };scene.onGround=[] { throw std::logic_error("명시지면중단"); };auto post=scene.Make();CHECK(CarrierFailed([&] { post.PostPop(scene.sid,0); }) && scene.Raw()[34]==5 && scene.owner==5 && scene.events==(std::vector<std::string>{"check","ground"}));
}
TEST_CASE(CarrierParentCurrent_CommonFailureKeepsEarlierGroundMutation) { //공통장부하위실패는이미수행한지면변경을되돌리지않는다.
    CarrierCurrentScene scene;scene.state.loadingDepth=1;scene.types[74].flags1=0x400;scene.onGround=[&] { scene.Raw()[34]=6; };scene.onBase=[] { throw std::logic_error("명시공통중단"); };auto post=scene.Make();CHECK(CarrierFailed([&] { post.DamageablePostPop(scene.sid,0); }) && scene.Raw()[34]==6 && scene.events==(std::vector<std::string>{"ground","base"}));
}
TEST_CASE(CarrierParentCurrent_BorrowValidationKeepsLiveBindingAndOwnedFallback) { //잘못된새대여는기존연결을보존하며빈span은기존생성시사본으로돌린다.
    CarrierCurrentScene scene;scene.state.loadingDepth=1;auto post=scene.Make();post.BindCurrentTypes(scene.types);scene.types[74].flags1=0x400;CHECK(CarrierFailed([&] { post.BindCurrentTypes(std::span<const RiftTypeRecord>(scene.types).first(187)); }));post.DamageablePostPop(scene.sid,0);post.BindCurrentTypes({});post.DamageablePostPop(scene.sid,0);CHECK(scene.events==(std::vector<std::string>{"ground","base","base"}));
}
TEST_CASE(CarrierParentCurrent_ReservedAndInactiveSlotsKeepPhysicalContract) { //낮은SID/free/contained를공통사전guard로추가하지않는다.
    CarrierCurrentScene scene;auto post=scene.Make();post.DamageablePostPop(Sid{0},0);CHECK(scene.lastSid.value==0);scene.Raw()[11]=9;post.DamageablePostPop(scene.sid,1);CHECK(scene.Raw()[11]==9 && scene.lastSid==scene.sid && scene.events==(std::vector<std::string>{"base","base"}));
}
