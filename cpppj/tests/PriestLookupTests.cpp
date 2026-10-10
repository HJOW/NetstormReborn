// 소유 사제의 실제 x86 관찰과 공유 목록·건설 확정/통지 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionSelection.h"
#include "o/RawPriestPreDestroy.h"

namespace {
using namespace netstorm::test::construction;
// 각 PE에서 얻은 전체 조회의 독립 관찰 행 수와 입력 슬롯 개수다.
constexpr std::size_t kRows=1617,kSlots=128;
// 작업장 기준점과 통지 대상에 쓰는 검사 입력 SID다. 실제 생성 SID와 겹치지 않는다.
constexpr Sid kWorkshop{50},kSurface{116};
// 입력 fixture만 읽는다. 원본 PE나 Python은 CTest에서 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_PRIESTLOOKUP_FIXTURE).rows;return rows; }
// 판본별 현재 소유자와 상태/원래 소유자/extra를 공급한다. 기대 선택은 계산하지 않는다.
void Node(SidPool& pool,Sid sid,std::uint32_t type,std::uint32_t state,std::uint32_t owner,std::uint32_t original=0,std::uint32_t extra=0) {
    auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;raw[10]=static_cast<std::uint8_t>(type);
    raw[11]=static_cast<std::uint8_t>(state);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(owner);
    raw[12]=static_cast<std::uint8_t>(original);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(extra);
}
// 각 실제 PE의 반환 DWORD·전체 입력 checksum을 동일한 입력의 C++ 결과와 비교한다.
void Replay(std::string_view edition) {
    SidPool pool(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    PriestPostPopState state;RawPriestLookup lookup(pool,state);std::size_t count=0;CHECK(Fixture().size()==3*kRows);
    const auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);
    const std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    // 매 행에서 동일한 raw·목록 입력을 만들고 조회의 순수성을 함께 확인한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        state.priests.entries=Inputs(row[3]);state.priests.count=Number(row[2]);
        // 타입과 original/extra는 원본이 읽지 않는 필드지만 같은 값으로 채운다.
        for (const auto& text:Split(row[4],';')) {
            const auto node=Inputs(text);CHECK(node.size()==6);
            Node(pool,Sid{static_cast<std::uint16_t>(node[0])},node[1],node[2],node[3],node[4],node[5]);
        }
        std::array<std::uint8_t,16> list{};
        // 저장 목록 checksum은 개수 밖의 꼬리를 포함한다.
        for (std::size_t i=0;i<state.priests.entries.size();++i) Put(list,i*4,state.priests.entries[i]);
        const auto before=Hex(raw);const auto entries=state.priests.entries;const auto result=lookup.Find(Number(row[1]));
        const bool same=result==Number(row[5]) && Adler(raw)==Number(row[6]) && Adler(list)==Number(row[7]) &&
            Hex(raw)==before && state.priests.entries==entries && state.priests.count==Number(row[2]);
        CHECK(same);if (!same) { std::printf("소유 사제 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }
        ++count;
    }
    CHECK(count==kRows);
}
// 확정의 필수 효과 중 실제 생성/통지 연결만 교체할 수 있는 무효과 기본값이다.
ConstructionConfirmHooks ConfirmHooks() {
    return {[](std::uint32_t,std::uint32_t,float,float) { return Sid{}; },[](std::uint32_t,std::uint32_t) { return Sid{}; },
        [](const ConstructionNotice&) {},[](const ConstructionNotice&) {},[] { return false; },[](Sid) {}};
}
}

// 10.78의 signed 개수·DWORD 소유자·상태 비트를 전체 몸체 관찰과 비교한다.
TEST_CASE(PriestLookup_ReplaysOriginals) { Replay("originals"); }
// CD판의 현재 소유자 위치와 전체 조회를 비교한다.
TEST_CASE(PriestLookup_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE에서 얻은 독립 관찰을 비교한다.
TEST_CASE(PriestLookup_ReplaysExtra1037) { Replay("original1037"); }

// 원본 조기 경로는 보존하고, 실제 접근할 손상 목록만 WORD 절단 전에 거부한다.
TEST_CASE(PriestLookup_LazyGuardsPreserveEarlyMatchAndDwordWidth) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);PriestPostPopState state;RawPriestLookup lookup(pool,state);
    state.priests.count=0xffffffff;CHECK(lookup.Find(3)==0);state.priests.count=0x80000000;CHECK(lookup.Find(3)==0);
    state.priests.count=1;CHECK(Throws([&] { lookup.Find(3); }));
    Node(pool,kWorkshop,82,5,3,1,255);state.priests.entries={50,0x10032};state.priests.count=2;
    CHECK(lookup.Find(3)==50);CHECK(Throws([&] { lookup.Find(259); }));
    state.priests.entries={0x10032};state.priests.count=1;CHECK(Throws([&] { lookup.Find(3); }));
    state.priests.entries={kCapacity};CHECK(Throws([&] { lookup.Find(3); }));
    state.priests.entries={50};CHECK(lookup.Find(259)==0);
    state.priests.entries={0,0x10032};state.priests.count=2;Node(pool,Sid{},1,0,3);
    CHECK(lookup.Find(3)==0);state.priests.count=0;CHECK(lookup.Find(0xffffffff)==0);
}

// 실제 사제 등록/중복 억제·현재 소유자 지정·삭제 목록 압축을 같은 조회 객체에 반영한다.
TEST_CASE(PriestLookup_ObservesRealRegistrationOwnershipAndPreDestroy) {
    // 두 판본의 생성자와 공유 목록 효과를 모두 연결한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[kPriestType].flags1=0x10;
        types[kPriestType].flags2=0x210000;types[kPriestType].constructorAddress=TypeConstructorAddress(edition,kPriestType);
        SquidFactory factory(pool,types);SquidPostPopState books;SquidOwnerMode mode;SquidOwner owner(pool,types,books,mode);
        SquidRewardState rewardState;GameRandom random;ScrambledSpStore sp(random,[] { return 1U; });
        SquidReward reward(pool,types,books,rewardState,patch ? &sp : nullptr);PriestPostPopState state;state.priests.entries.resize(4);
        RawPriestLookup lookup(pool,state);RawPriestPostPop post(pool,reward,state,{
            [](Sid,std::uint32_t) { return true; },[] { return false; },[](Sid,std::uint32_t,float) {}});
        const auto first=factory.Create(kPriestType,2),second=factory.Create(kPriestType,2);
        // 가상 표는 실제 생성자가 쓴 값을 보존하고 공간 접두 입력만 준비한다.
        for (const auto sid:{first,second}) {
            auto raw=pool.AllocatedBytes(sid);raw[11]=0;raw[patch ? 40 : 35]=0;raw[12]=1;
            owner.Set(sid,1);post.Prefix(sid,1);post.Prefix(sid,1);
        }
        CHECK(state.priests.count==2 && lookup.Find(1)==first.value);owner.Set(first,3);
        CHECK(lookup.Find(1)==second.value && lookup.Find(3)==first.value);
        pool.AllocatedBytes(first)[11]|=2;CHECK(lookup.Find(3)==0);
        std::uint32_t carrierCalls=0;RawPriestPreDestroy pre(pool,state,{
            [](Sid) { return Sid{}; },[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {},
            [&](Sid sid,std::uint32_t flags) { CHECK(sid==first && flags==0);++carrierCalls; }});
        pre.PreDestroy(first,0);CHECK(state.priests.count==1 && state.priests.entries[0]==second.value && carrierCalls==1);
        pool.AllocatedBytes(first)[11]&=static_cast<std::uint8_t>(~2U);CHECK(lookup.Find(3)==0);
    }
}

// 확정이 실제 그래프 기준점으로 먼저 거부/허용하고 허용된 경우에만 서버 SID를 만든다.
TEST_CASE(PriestLookup_ConstructionConfirmUsesActualGraphAnchor) {
    // 두 판본에서 같은 위치의 그래프 일치·불일치와 우회 플래그를 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypePlain].flags1=0x8010;types[kTypePlain].flags2=4;frames[kTypePlain]={InputFrames(0),3};
        types[83].flags1=types[kTypeIsland].flags1=0x800;
        std::vector<std::uint16_t> surfaces(65536,kSurface.value);SquidPostPopState books;SquidPostPopList additional;
        ContainedFinderState contained;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
        Node(pool,kSurface,94,0,0);InputRaw(pool,kSurface)[patch ? 30 : 28]=7;
        Node(pool,kWorkshop,83,0,3);InputRaw(pool,kWorkshop)[patch ? 30 : 28]=6;
        books.ownerFactories[3]={{kWorkshop.value},1};
        RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,additional,contained,anchorState);
        SquidFactory factory(pool,types);ConstructionConfirmState state;auto hooks=ConfirmHooks();std::vector<ConstructionNotice> notices;
        hooks.broadcast=[&](const ConstructionNotice& notice) { notices.push_back(notice); };
        hooks=MakeConstructionAnchorHooks(pool,anchor,MakeConstructionConfirmHooks(pool,factory,hooks));
        RawConstructionConfirm confirm(pool,types,frames,state,hooks);const auto before=pool.FreeCount();
        const ConstructionConfirmRequest request{kTypePlain,20.25F,21.5F,3,0,0,0,0x12345678,0x40080000,0};
        CHECK(!confirm.Confirm(request) && notices.empty() && pool.FreeCount()==before);
        InputRaw(pool,kWorkshop)[patch ? 30 : 28]=7;
        CHECK(confirm.Confirm(request) && notices.size()==1 && pool.FreeCount()==before-1);
        CHECK(notices[0].sids.size()==1 && notices[0].sids[0]>=pool.Layout().serverFirst && notices[0].timeLow==request.timeLow);
        books.ownerFactories[3].count=0;CHECK(!confirm.Confirm(request) && notices.size()==1);
        auto bypass=request;bypass.flags=1;CHECK(confirm.Confirm(bypass) && notices.size()==2);
        state.skipBuilderCheck=true;anchorState.ignoreRestrictions=1;CHECK(confirm.Confirm(request) && notices.size()==3);
    }
}

// 실제 확정/factory→통지에서 현재 공유 사제를 찾는다. 패치의 두 번째 조회는 경계 변경을 다시 본다.
TEST_CASE(PriestLookup_ConfirmNoticePreservesCapturedPriestAndRereadsCurrentOwner) {
    // 패치의 contained 제거와 CD의 단일 조회가 같은 첫 이동 사제를 유지하는지 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypePlain].flags1=0x10;types[kTypePlain].flags2=0x400000;types[kPriestType].flags2=0x200000;
        frames[kTypePlain]={InputFrames(0),3};Node(pool,Sid{50},kPriestType,0,3);Node(pool,Sid{51},kPriestType,0,3);
        Put(InputRaw(pool,Sid{50}),14,Bits(11.25F));Put(InputRaw(pool,Sid{50}),18,Bits(12.5F));
        PriestPostPopState priests;priests.priests={{50,51},2};RawPriestLookup lookup(pool,priests);
        ConstructionNoticeState noticeState;noticeState.localPlayer=3;auto noticeHooks=SilentNoticeHooks();std::vector<std::string> events;
        noticeHooks.clear=[&](const ConstructionClearRequest&) { events.push_back("clear"); };
        noticeHooks.place=[&](const ConstructionPlaceRequest& request) {
            events.push_back("place");CHECK(request.abstract==1);Node(pool,Sid{request.sids.front()},kTypePlain,0,3);
        };
        noticeHooks.containedType=[&](std::uint32_t type) {
            CHECK(type==kPriestType);events.push_back("contained");InputRaw(pool,Sid{50})[34]=1;return 6U;
        };
        noticeHooks.removeContained=[&](Sid sid,std::uint32_t type) { CHECK(sid==Sid{51} && type==6);events.push_back("remove"); };
        noticeHooks.immobile=[&](Sid sid) { CHECK(sid==Sid{50});events.push_back("immobile");return false; };
        noticeHooks.approach=[&](Sid,float x,float y,std::uint32_t flags) {
            CHECK(x==11.25F && y==12.5F && flags==7);events.push_back("approach");return ConstructionNoticePoint{20.25F,21.5F};
        };
        noticeHooks.tryMove=[&](Sid sid,float x,float y,Sid object) {
            CHECK(sid==Sid{50} && x==21 && y==22 && object.value>=pool.Layout().serverFirst);events.push_back("move");return true;
        };
        noticeHooks.refund=[&](std::uint32_t,std::uint32_t) { events.push_back("refund"); };
        noticeHooks.destroy=[&](Sid,std::uint32_t) { events.push_back("destroy"); };
        noticeHooks.refresh=[&] { events.push_back("refresh"); };
        RawConstructionNotice handler(pool,types,noticeState,MakeConstructionPriestLookupHooks(pool,lookup,noticeHooks));
        SquidFactory factory(pool,types);ConstructionConfirmState confirmState;auto hooks=ConfirmHooks();
        hooks.broadcast=[&](const ConstructionNotice&) { events.push_back("broadcast"); };
        RawConstructionConfirm confirm(pool,types,frames,confirmState,
            MakeConstructionNoticeConfirmHooks(pool,handler,MakeConstructionConfirmHooks(pool,factory,hooks)));
        CHECK(confirm.Confirm({kTypePlain,20.25F,21.5F,3,0,0,0,0,0,0}));
        CHECK(events==(patch ? std::vector<std::string>{"broadcast","clear","place","contained","remove","immobile","approach","move","refresh"} :
            std::vector<std::string>{"broadcast","clear","place","immobile","approach","move","refresh"}));
        CHECK(lookup.Find(3)==(patch ? 51U : 50U));
        // 같은 어댑터에서 사망한 사제를 제외하고 아무 소유 사제도 없으면 환불/삭제한다.
        events.clear();InputRaw(pool,Sid{50})[11]=2;InputRaw(pool,Sid{51})[11]=2;
        noticeHooks.containedType=[](std::uint32_t) { return 6U; };
        noticeHooks.removeContained=[&](Sid sid,std::uint32_t type) { CHECK(sid==Sid{} && type==6); };
        RawConstructionNotice failed(pool,types,noticeState,MakeConstructionPriestLookupHooks(pool,lookup,noticeHooks));
        ConstructionNotice notice;notice.type=kTypePlain;notice.player=3;notice.sids={static_cast<std::uint16_t>(pool.Layout().serverFirst)};
        failed.Handle(notice);CHECK(events==std::vector<std::string>({"clear","place","refund","destroy","refresh"}));
    }
}

// 다른 풀의 기준점이나 사제 목록을 효과 훅에 연결하기 전에 거부한다.
TEST_CASE(PriestLookup_SelectionAdaptersRejectForeignPools) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(pool.Edition(),kCapacity,true);
    PriestPostPopState priests;RawPriestLookup lookup(pool,priests);std::vector<RiftTypeRecord> types(188);
    std::vector<std::uint16_t> surfaces(65536);SquidPostPopState books;SquidPostPopList additional;
    ContainedFinderState contained;PlayerPlacementAnchorState state;RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,additional,contained,state);
    CHECK(Throws([&] { MakeConstructionAnchorHooks(other,anchor,ConfirmHooks()); }));
    CHECK(Throws([&] { MakeConstructionPriestLookupHooks(other,lookup,SilentNoticeHooks()); }));
}
