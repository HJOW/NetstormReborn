// 실제 Fury 설치 세션의 같은 Player/TYPE/contained 생성에 네 테마 명령을 연결해 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/RawPriestWorldBindings.h"
#include "client/ThemeTechnologyCommand.h"
#include "o/RawPlayerWorld.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
using namespace netstorm::client;
namespace {
// GameMenuSessionServices와 같은 설치 장치 경계를 공급하며 게임 계산은 실제 Session에 맡긴다.
struct ThemeSessionServices {
    SoundList names{OriginalEdition::Patch1078}; // 세션보다 오래 사는 실제 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 명시 영어 번역 바이트다.
    // 소리/번역/표시만 외부 경계로 두고 Player·기술·Factory·내용물 사슬은 대체하지 않는다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;
        result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("기술검사의 미설치 UI 대상 처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("기술검사의 미선택 삭제 효과"); };
        result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// 보호 .type의 명시 group/theme에 해당하는 전체 기술 이름이다. 제품 필터를 검사 안에서 다시 계산하지 않는다.
const std::array<std::vector<std::string_view>,4> kThemeNames{{
    {"windBattery","windWalker","windAviary","windArcher","windBalloon","windBlocker"},
    {"rainBattery","rainCannon","rainAviary","rainFence","rainBalloon","rainBlocker","rainWalker"},
    {"thunderBattery","thunderBlocker","thunderCannon","bulf","thunderArcher","thunderFence"},
    {"sunArcher","sunAviary","sunBlocker","sunBalloon","sunCannon","sunWalker","sunFence"}
}};
// 같은 실제 부모의 raw next/head를 직접 읽어 기술을 모으며 중복·필드 폭·체인 순환도 확인한다.
std::map<std::uint32_t,Sid> ThemeContents(RawCampaignSession& session,Sid parent) {
    const auto core=session.Core();std::map<std::uint32_t,Sid> result;std::set<std::uint16_t> visited;
    auto next=Get(core.pool.Slot(parent),6,2);
    // 모든 실제 종속을 따라가며 Form6/mask4만 기술 목록으로 관측한다.
    while (next) {
        if (next>=core.pool.Capacity() || !visited.insert(static_cast<std::uint16_t>(next)).second) throw std::logic_error("실제 기술 부모의 종속 체인 손상");
        const Sid sid{static_cast<std::uint16_t>(next)};const auto raw=core.pool.Slot(sid);next=Get(raw,4,2);
        if (raw[10]!=6 || !(Get(raw,20,2)&4U)) continue;
        CHECK(Get(raw,20,2)==4 && Get(raw,12,2)==0 && raw[30]==0);
        CHECK(result.emplace(Get(raw,18,2),sid).second);
    }
    return result;
}
// 선택되지 않은 실제 Player 생성/Pop/예약 경계를 사용하면 검사 실패로 보고한다.
[[noreturn]] void ThemePlayerBoundary() { throw std::logic_error("현재 기술 getter 검사에서 생성/공간/예약 경계는 선택하지 않습니다"); }
}
// Fury의 초기 두 기술을 실제 AddTechnology로 만든 뒤 네 선택을 모두 실제 내용물로 검증한다.
TEST_CASE(CampaignThemeTechnology_FuryAllFourThemesAndRepeat) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    // 네 테마는 독립적으로 새 설치 세션에서 같은 초기 기술을 받는다.
    for (std::int32_t theme=0;theme<4;++theme) {
        ThemeSessionServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());
        RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",external.Make());session.Load();session.ClearTechnology(1);
        session.AddTechnology(1,"outpost;windBattery");const auto core=session.Core();const Sid parent{static_cast<std::uint16_t>(session.CurrentTechnologyParent())};
        CHECK(session.PlayerKnown()==1 && session.LocalOwner()==1 && parent.value);
        const auto initial=ThemeContents(session,parent);std::set<std::uint32_t> expected;
        // 원본 Fury Header.mytech의 두 항목은 선택 테마와 무관하게 보존된다.
        for (const auto name:{"outpost","windBattery"}) { const auto type=assets.game.TypeTable().Find(name);CHECK(type>0);expected.insert(static_cast<std::uint32_t>(type)); }
        CHECK(initial.size()==2);
        // 독립 보호 자산에서 확정한 선택 테마의 전체 이름을 번호로 해석한다.
        for (const auto name:kThemeNames[static_cast<std::size_t>(theme)]) { const auto type=assets.game.TypeTable().Find(name);CHECK(type>0);expected.insert(static_cast<std::uint32_t>(type)); }
        ApplyThemeTechnology(session,theme);const auto contents=ThemeContents(session,parent);CHECK(contents.size()==expected.size());
        // 기대 목록의 전 항목은 동일 raw 부모에 하나씩 존재해야 한다.
        for (const auto type:expected) CHECK(contents.contains(type));
        // 기존 두 기술은 삭제 후 재생성하지 않고 원래 SID를 보존한다.
        for (const auto& [type,sid]:initial) CHECK(contents.at(type)==sid);
        const auto before=std::vector<std::uint8_t>(core.pool.Bytes().begin(),core.pool.Bytes().end());
        ApplyThemeTechnology(session,theme);CHECK(ThemeContents(session,parent)==contents);CHECK(std::equal(before.begin(),before.end(),core.pool.Bytes().begin()));
        ApplyThemeTechnology(session,5);CHECK(ThemeContents(session,parent)==contents);CHECK(std::equal(before.begin(),before.end(),core.pool.Bytes().begin()));
        CHECK(session.CurrentTechnologyParent()==parent.value && session.PlayerKnown()==1 && core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    }
}
// 같은 설치 Player 기록에서 실제 CurrentTechnology의 Known0 짧은 반환과 명시 owner 진단의 차이를 확인한다.
TEST_CASE(CampaignThemeTechnology_ActualCurrentGetterKnownZeroBoundary) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ThemeSessionServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",external.Make());session.Load();
    const auto core=session.Core();PlayerWorldState state;state.playerKnown=0;state.battleOwner=0xffffffffU;std::uint32_t assertions=0;
    PlayerWorldHooks hooks;
    hooks.assertion=[&assertions](PlayerWorldAssertion value) { CHECK(value==PlayerWorldAssertion::PlayerIdKnown);++assertions; };
    hooks.find=[](float,float,std::uint32_t)->Sid { ThemePlayerBoundary(); };hooks.create=[](std::uint32_t,std::uint32_t)->Sid { ThemePlayerBoundary(); };
    hooks.setOwner=[](Sid,std::uint32_t) { ThemePlayerBoundary(); };hooks.pop=[](Sid,float,float,std::uint32_t) { ThemePlayerBoundary(); };
    hooks.reserveRegular=[]()->bool { ThemePlayerBoundary(); };hooks.addRegular=[](Sid,std::uint32_t,float) { ThemePlayerBoundary(); };
    RawPlayerWorld getter(core.pool,core.postPop,session.PlayerRecords(),state,std::move(hooks));
    CHECK(getter.CurrentTechnology()==0 && assertions==0); // Known0은 무효 battleOwner의 실제 기록조차 조회하지 않는다.
    state.battleOwner=1;CHECK(getter.CurrentTechnology()==0 && assertions==0);
    CHECK(getter.Technology(1)==session.CurrentTechnologyParent() && assertions==1); // 명시 owner getter는 같은 Known0에서 진단을 수행한다.
    state.playerKnown=1;CHECK(getter.CurrentTechnology()==session.CurrentTechnologyParent() && assertions==1);
    CHECK(session.PlayerKnown()==1); // 검사 facade의 상태를 실제 세션의 비공개 전역으로 가장하지 않는다.
}
