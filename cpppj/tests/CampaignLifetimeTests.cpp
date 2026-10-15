// 실제Player통지와같은월드의공통삭제/Content수명후설치를검사한다.
#include "ConstructionSupport.h"
#include "client/RawCampaignLifetimeBindings.h"
#include "o/RawContainedCreate.h"
#include "o/RawContentParentNotice.h"
#include <bit>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::construction;
namespace {
constexpr std::uint32_t kPlayerNoticeCases=88; // 현재독립원본출력의고유입력수다.
// 실제현재전역을전체DWORD/WORD목록으로기록한다.
std::string NoticeWords(std::span<const std::uint32_t> values) {
    std::string result;
    // 전체필드의현재값을같은순서로기록한다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 원본입력과같은부모/세종속을실제풀에구성한다.
void NoticeSlots(SidPool& pool,std::uint32_t layout) {
    // 전체50BYTE를명시입력으로초기화하고free/dead검사를정답에서예측하지않는다.
    for (const auto sid:{50,60,61,62}) { auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
    auto parent=InputRaw(pool,Sid{50});Put(parent,0,0x50ee08);parent[10]=152;
    if (!layout) return;Put(parent,6,60,2);
    // 세내용물의type/state/등록ID입력은Native와동일하다.
    for (const auto sid:{60,61,62}) {
        auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});Put(raw,4,sid==62 ? 0U : static_cast<std::uint32_t>(sid+1),2);
        raw[10]=static_cast<std::uint8_t>(sid==61 || (layout==1 && sid==60) ? 6 : layout==5 ? 2 : 3);raw[11]=layout==4 ? 3 : 0;Put(raw,18,static_cast<std::uint32_t>(sid==60 ? 7 : sid==61 ? 8 : 9));
    }
}
// 같은풀에서실제factory/Pop/장부를쓸최소월드자산입력이다.
struct LifetimeAssets {
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 원본번호를직접쓰는현재타입표다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 같은번호의실제프레임저장소다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188); // 공간등록에쓸명시SHP치수다.
    // 실제empty84는base생성자이며다른현재타입은이검사에서Pop하지않는다.
    LifetimeAssets() {
        types[84].cost=20;types[84].footX=types[84].footY=1;frames[84]=RiftTypeFrames({FrameCode{'A','A',0,0}});
        types[152].constructorAddress=TypeConstructorAddress(OriginalEdition::Patch1078,152);
        shapes[84].loaded=true;shapes[84].frameCount=1;shapes[84].frames={SquidDisplayFrame{20,16,8,12,1,1}};
    }
};
// 실제월드가요구하는PATCH표면/다리/연결객체의현재전역번호를명시한다.
BridgeConnectState LifetimeLinks() {
    BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;result.connectorType=155;
    result.noIslandType=157;result.battle=true;result.mission=true;return result;
}
}

// 실제wrapper/접두/등록표88행의전체200BYTE/전역11필드/가상알림순서를대조한다.
TEST_CASE(CampaignLifetime_PlayerNoticeIndependentPeFixture) {
    std::ifstream input(NETSTORM_PLAYER_CONTENT_NOTICE_FIXTURE);if (!input) throw std::runtime_error("Player 내용물 fixture 없음");
    std::string line;std::uint32_t count=0;
    // 각행은원본과같은독립raw/등록표입력이다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=20) throw std::runtime_error("Player 내용물 fixture 열 수 오류");++count;
        SidPool pool(OriginalEdition::Patch1078,32768);NoticeSlots(pool,Number(row[12]));std::array<std::uint32_t,11> current;
        // 현재각전역의명시입력을손실없이기록한다.
        for (std::size_t index=0;index<current.size();++index) current[index]=Number(row[index]);
        auto first=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(current[8]));std::string trace;const auto mutation=Number(row[13]);
        GumpCreateState registry;registry.collection.edition=OriginalEdition::Patch1078;
        // 현재등록된세가상노드를원본과동일하게공급한다.
        for (const auto token:{7,8,9}) registry.entries[token]=Number(row[12])==3 ? 0U : static_cast<std::uint32_t>(token);
        PlayerContentNoticeHooks hooks;
        hooks.money=[&](std::uint32_t owner) { if (!trace.empty()) trace+='|';trace+="M:"+std::to_string(owner);if (mutation==1) { current[3]=91;current[4]=0; }return Float(Number(row[11])); };
        GumpEventHooks events;events.handle=[&](std::uint32_t token,std::uint32_t event,std::uint32_t form,std::uint32_t reason) {
            if (!trace.empty()) trace+='|';trace+="H:"+std::to_string(token)+':'+std::to_string(event)+':'+std::to_string(form)+':'+std::to_string(reason);
            if (mutation==2) { Put(InputRaw(pool,Sid{60}),4,0,2);current[10]=0x11223344;registry.entries[9]=9; }
            else if (mutation==3) current[9]=6;return 0U;
        };
        RawPlayerContentNotice notice(pool,{current[0],current[1],current[2],current[3],current[4],current[5],current[6],current[7],first,current[9],current[10]},BindPlayerContentNoticeGumps(registry,std::move(events),std::move(hooks)));
        notice.Notify(Sid{50},Number(row[14]),Number(row[15]));current[8]=static_cast<std::uint16_t>(first);CHECK(&notice.Pool()==&pool);
        std::string slots;
        // 실제전체네슬롯을원본출력과비교한다.
        for (const auto sid:{50,60,61,62}) { if (!slots.empty()) slots+=',';slots+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(sid)})); }
        CHECK(NoticeWords(current)==row[17]);CHECK((trace.empty() ? "-" : trace)==row[18]);CHECK(slots==row[19]);
    }
    CHECK(count==kPlayerNoticeCases);
}

// 실제후설치의보상/선택/장부/반납은기존관찰카운터를실행하지않고같은전역에반영된다.
TEST_CASE(CampaignLifetime_EmptyBaseAndContentUseSameDeletion) {
    LifetimeAssets data;RawSurfaceWorld world(OriginalEdition::Patch1078,data.types,data.frames,data.shapes,LifetimeLinks(),83);auto core=world.Core();
    GameRandom random(123);ScrambledSpStore store(random,[] { return 456U; });store.Initialize();store.Set(0,Bits(0));SquidRewardState rewardState;SquidReward reward(core.pool,core.types,core.bookkeeping,rewardState,&store);
    Sid selected{};std::uint32_t clear=0,sounds=0,forwarded=0;SquidDestroyHooks effects;effects.selected=[&] { return selected; };effects.emit=[&](const SquidDestroyEvent& event) {
        if (event.effect==SquidDestroyEffect::ClearSelection) { selected={};++clear; }else if (event.effect==SquidDestroyEffect::Transmit) ++forwarded;else throw std::logic_error("통합검사의다른실제파생효과");
    };
    auto* deletion=&world.DeletionState(); // 실제후설치가교체하지않아야하는기존단일전역주소다.
    world.BindDeletion(reward,effects.selected,MakeCampaignDeletionEffects(effects,[&](Sid) { ++sounds; }));CHECK(&world.DeletionState()==deletion);
    std::array<std::uint32_t,11> globals{3,3,77,9,0,1,1,1,0,3,0x02000035};std::int16_t first=0;GumpCreateState registry;registry.collection.edition=OriginalEdition::Patch1078;
    PlayerContentNoticeHooks noticeHooks;noticeHooks.money=[&](std::uint32_t owner) { return Float(store.Get(owner)); };GumpEventHooks events;events.handle=[](auto,auto,auto,auto)->std::uint32_t { throw std::logic_error("통합검사에없는Gump등록"); };
    RawPlayerContentNotice player(core.pool,{globals[0],globals[1],globals[2],globals[3],globals[4],globals[5],globals[6],globals[7],first,globals[9],globals[10]},BindPlayerContentNoticeGumps(registry,std::move(events),std::move(noticeHooks)));
    RawCampaignLifetimeBindings* binding=nullptr;RawContentParentNotice parentNotice(core.pool,[&](Sid parent,Sid form,std::uint32_t reason) { binding->ParentNotice(parent,form,reason); });
    ContentFormHost forms(core.pool,core.types,[&](Sid parent,Sid form,std::uint32_t reason) { parentNotice.Notice(parent,form,reason); });ContentLifetimeState contentState;
    RawCampaignLifetimeBindings lifetime(world,forms,contentState,{},reward,player);binding=&lifetime;
    core.forms.SetAssets(lifetime.DestroyHooks(effects),lifetime.Destroy());
    const auto empty=world.Create(84);world.Pop(empty,24,24,0);selected=empty;const auto oldPool=rewardState.geyserPool;CHECK(core.bookkeeping.globalCounts[84]==1);
    CHECK(world.Destroy(empty,0x400000));CHECK(selected.value==0 && clear==1 && sounds==0 && forwarded==1);CHECK(rewardState.geyserPool==oldPool+20);
    CHECK(core.bookkeeping.globalCounts[84]==0 && core.bookkeeping.totalCost==0 && (core.pool.Slot(empty)[11]&1U));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    const auto parent=world.Create(152);ContainedCreateState createState;ContainedCreateHooks createHooks;createHooks.notifySurface=[&](Sid sid) { core.building.NotifySurface(sid); };
    RawContainedCreate create(core.pool,createState,forms.BindCreate(core.factory,std::move(createHooks)));const auto content=create.Create(parent,71,0,4,0x10,0);
    CHECK(Get(core.pool.Slot(parent),6,2)==content.value);CHECK(core.pool.Slot(content)[10]==6);CHECK(world.Destroy(content,0));CHECK(Get(core.pool.Slot(parent),6,2)==0);
    CHECK(core.pool.Slot(content)[11]&1U);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);CHECK(&lifetime.Contents().Pool()==&core.pool);
    world.ClearProcesses();
}

// 미연결/다른풀/설치후교체를명시거부하고기존inspect전용기본과실제바인딩을구분한다.
TEST_CASE(CampaignLifetime_RejectsMissingOrLateDeletionBinding) {
    LifetimeAssets data;RawSurfaceWorld world(OriginalEdition::Patch1078,data.types,data.frames,data.shapes,LifetimeLinks(),83);auto core=world.Core();GameRandom random(123);ScrambledSpStore store(random,[] { return 456U; });SquidRewardState state;SquidReward reward(core.pool,core.types,core.bookkeeping,state,&store);
    bool rejected=false;try { world.BindDeletion(reward,{},{}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    const auto sid=world.Create(84);static_cast<void>(sid);rejected=false;
    try { world.BindDeletion(reward,[] { return Sid{}; },{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
