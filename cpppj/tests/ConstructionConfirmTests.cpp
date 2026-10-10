// 서버 건설 확정의 세 PE 관찰 재생과 실제 SID 생성·배치 실행 연결을 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionConfirm.h"
#include "o/RawConstructionPlace.h"

namespace {
using namespace netstorm::test::construction;
// 판본별 독립 관찰 행 수와 실행기가 새 SID를 서버 영역 첫 번호에서 띄운 거리다.
constexpr std::size_t kPatchRows=508,kCdRows=396;
constexpr std::uint32_t kBornOffset=8;
// 타입 flags1의 "사제가 지어야 함" 비트다.
constexpr std::uint32_t kNeedsBuilder=0x8000;
// Python/원본 파일 없이 기대 출력만 한 번 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONCONFIRM_FIXTURE).rows;return rows; }
// 통지 본문을 실행기와 같은 칸 순서의 한 줄로 만든다.
std::string NoticeText(const ConstructionNotice& notice) {
    std::string text=std::to_string(notice.type)+':'+std::to_string(Bits(notice.x))+':'+std::to_string(Bits(notice.y))+':'+
        std::to_string(notice.argument)+':'+std::to_string(notice.direction)+':'+std::to_string(notice.player)+':'+std::to_string(notice.flags)+':'+
        std::to_string(notice.quality)+':'+std::to_string(notice.timeLow)+':'+std::to_string(notice.timeHigh)+':'+std::to_string(notice.sids.size())+':';
    // SID는 쉼표로 잇고 없으면 '-'다.
    for (std::size_t i=0;i<notice.sids.size();++i) text+=(i ? "," : "")+std::to_string(notice.sids[i]);
    return notice.sids.empty() ? text+'-' : text;
}
// 한 판본의 모든 관찰 행을 재생한다. 경계 사건의 순서·인자와 반환값을 원본 출력과 그대로 비교한다.
void Replay(std::string_view edition) {
    const auto version=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SidPool pool(version,kCapacity,true);std::vector<RiftTypeRecord> types(version==OriginalEdition::Patch1078 ? 188 : 171);
    std::vector<PriestPlainCanonType> frames(types.size());
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);std::size_t count=0;
    // 행마다 타입·프레임·전역 입력을 다시 준비한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==4);const auto v=Inputs(row[1]);CHECK(v.size()==18);
        types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),PriestPlainCanonType{});
        types.at(v[0]).flags1=v[10];frames.at(v[0])={InputFrames(v[15]),std::bit_cast<int>(v[16])};
        ConstructionConfirmState state;state.skipBuilderCheck=v[11]!=0;state.recorderEnabled=v[13]!=0;
        std::string events;std::uint32_t born=0;
        // 사건 문자열에 한 항목을 잇는다.
        const auto append=[&](const std::string& value) { if (!events.empty()) events+=';';events+=value; };
        RawConstructionConfirm confirm(pool,types,frames,state,{
            [&](std::uint32_t player,std::uint32_t type,float x,float y) {
                append("B:"+std::to_string(player)+':'+std::to_string(type)+':'+std::to_string(Bits(x))+':'+std::to_string(Bits(y)));
                return Sid{static_cast<std::uint16_t>(v[12])};
            },[&](std::uint32_t type,std::uint32_t flags) {
                append("C:"+std::to_string(type)+':'+std::to_string(flags));
                return Sid{static_cast<std::uint16_t>(pool.Layout().serverFirst+kBornOffset+born++)};
            },[&](const ConstructionNotice& notice) { append("S:"+NoticeText(notice)); },
            [&](const ConstructionNotice& notice) { append("H:9:0:"+NoticeText(notice)); },
            [&] { append("Q");return v[14]!=0; },[&](Sid sid) { append("R:"+std::to_string(sid.value)); }});
        const bool result=confirm.Confirm({v[0],Float(v[1]),Float(v[2]),v[3],v[4],v[5],v[6],v[7],v[8],v[9]});
        if (events.empty()) events="-";
        const bool same=events==row[2] && std::to_string(result ? 1 : 0)==row[3] && born==v[17]*(result ? 1U : 0U);
        CHECK(same);if (!same) { std::printf("건설 확정 %s 행 %zu 불일치\n  기대 %s | %s\n  실제 %s | %d\n",std::string(edition).c_str(),count,row[2].c_str(),row[3].c_str(),events.c_str(),result ? 1 : 0);break; }
        ++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
// 기록만 하는 경계 묶음이다. 개별 검사가 필요한 것만 바꿔 쓴다.
ConstructionConfirmHooks QuietHooks() {
    return {[](std::uint32_t,std::uint32_t,float,float) { return Sid{77}; },[](std::uint32_t,std::uint32_t) { return Sid{}; },
        [](const ConstructionNotice&) {},[](const ConstructionNotice&) {},[] { return false; },[](Sid) {}};
}
}

// 10.78의 확정 전체를 원본 관찰과 대조한다.
TEST_CASE(ConstructionConfirm_ReplaysOriginals) { Replay("originals"); }
// CD판의 확정 전체를 대조한다.
TEST_CASE(ConstructionConfirm_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 원본 관찰에 대조한다.
TEST_CASE(ConstructionConfirm_ReplaysExtra1037) { Replay("original1037"); }

// 실제 factory가 만든 서버 SID가 통지에 담기고, 그 통지를 받은 배치 실행이 같은 SID에 조각을 놓는다.
TEST_CASE(ConstructionConfirm_CreatesServerSidsThatPlacementUses) {
    // 두 판본에서 noIsland 아홉 칸을 확정하고 곧바로 놓는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypeNoIsland].flags1=kHasHp;types[kTypeNoIsland].maxHitPoints=40;frames[kTypeNoIsland]={InputFrames(0),3};
        SquidFactory factory(pool,types);SquidPostPopState books;SquidOwnerMode mode;SquidOwner owner(pool,types,books,mode);
        SquidRewardState rewardState;GameRandom rng;ScrambledSpStore store(rng,[] { return 1U; });
        SquidReward reward(pool,types,books,rewardState,patch ? &store : nullptr);
        ConstructionPlaceState placeState;placeState.localPlayer=2;std::vector<std::string> pops;
        ConstructionPlaceHooks placeHooks;
        placeHooks.notifySurface=[](Sid) {};
        placeHooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { pops.push_back(std::to_string(sid.value)+':'+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(flags)); };
        placeHooks.sendMoney=[](std::uint32_t,const ConstructionMoneyNotice&) {};
        placeHooks.format=[](std::string_view,std::string_view) { return std::string{}; };
        placeHooks.tellOthers=[](std::uint32_t,std::string_view) {};placeHooks.tellPlayer=[](std::uint32_t,std::string_view) {};
        placeHooks.tell=[](std::string_view) {};placeHooks.sendReject=[](std::uint32_t,const ConstructionRejectNotice&) {};
        placeHooks.take=[](std::uint32_t,Sid sid) { return sid; };placeHooks.setOwner=[](Sid,std::uint32_t) {};
        placeHooks.storedSp=[](std::uint32_t) { return 0.0F; };placeHooks.addLocalSp=[](float) {};placeHooks.addPlayerSp=[](std::uint32_t,float) {};
        RawConstructionPlace place(pool,types,frames,placeState,reward,
            MakeConstructionPlaceHooks(pool,factory,owner,reward,patch ? &store : nullptr,placeState,placeHooks));
        ConstructionConfirmState state;std::vector<ConstructionNotice> sent;std::uint32_t handled=0;
        auto hooks=QuietHooks();
        hooks.broadcast=[&](const ConstructionNotice& notice) { sent.push_back(notice); };
        // 처리기 자리에서 통지의 SID로 확정 배치(abstract 0)를 실행한다. 실제 처리기의 abstract 판정은 아직 복원하지 않았다.
        hooks.handle=[&](const ConstructionNotice& notice) {
            ++handled;place.Place({notice.type,notice.x,notice.y,notice.argument,notice.direction,notice.player,notice.sids,0,notice.quality});
        };
        RawConstructionConfirm confirm(pool,types,frames,state,MakeConstructionConfirmHooks(pool,factory,hooks));
        const auto freeBefore=pool.FreeCount();
        CHECK(confirm.Confirm({kTypeNoIsland,20.0F,21.0F,2,0,0,0,0,0,0}));
        CHECK(sent.size()==1 && handled==1 && sent[0].sids.size()==9 && pops.size()==9 && pool.FreeCount()==freeBefore-9);
        // 아홉 SID는 모두 서버 영역이고 시작 칸만 단어 0x5d4를 받는다. 소유자·최대 HP·Pop 0x41이 적용된다.
        for (std::size_t i=0;i<sent[0].sids.size();++i) {
            const Sid sid{sent[0].sids[i]};const auto raw=pool.Slot(sid);
            CHECK(sid.value>=pool.Layout().serverFirst && sid.value<pool.Layout().predictableFirst);
            CHECK(raw[patch ? 34 : 32]==2 && raw[10]==kTypeNoIsland && Get(raw,26,patch ? 4 : 2)==40 && Get(raw,12,2)==(i==0 ? 0x5d4U : 0U));
            CHECK(pops[i].starts_with(std::to_string(sid.value)+':') && pops[i].ends_with(":65"));
        }
        CHECK(sent[0].type==kTypeNoIsland && sent[0].player==2 && sent[0].x==20.0F && sent[0].y==21.0F);
        // 사제가 지어야 하는 타입은 사제를 못 찾으면 SID를 만들지 않고 통지도 보내지 않는다.
        types[kTypePlain].flags1=kNeedsBuilder;frames[kTypePlain]={InputFrames(0),3};
        hooks.findBuilder=[](std::uint32_t,std::uint32_t,float,float) { return Sid{}; };
        RawConstructionConfirm blocked(pool,types,frames,state,MakeConstructionConfirmHooks(pool,factory,hooks));
        const auto freeAfter=pool.FreeCount();
        CHECK(!blocked.Confirm({kTypePlain,20.0F,21.0F,2,0,0,0,0,0,0}) && pool.FreeCount()==freeAfter && sent.size()==1);
        // 호출 플래그 bit 0이나 건너뛰기 전역이 켜져 있으면 사제 없이도 확정한다.
        CHECK(blocked.Confirm({kTypePlain,20.0F,21.0F,2,0,0,1,0,0,0}) && sent.size()==2 && sent[1].flags==1 && sent[1].sids.size()==1);
    }
}

// 누락 경계·다른 풀·범위 밖 타입·잘못된 생성 결과를 거부하고, 조각이 없을 때의 기록기 인자를 0으로 둔다.
TEST_CASE(ConstructionConfirm_RejectsBadInputsAndRecordsFirstSid) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(OriginalEdition::Patch1078,kCapacity,true);
    std::vector<RiftTypeRecord> types(188);std::vector<PriestPlainCanonType> frames(types.size());frames[kTypePlain]={InputFrames(0),3};
    ConstructionConfirmState state;auto missing=QuietHooks();missing.handle={};
    CHECK(Throws([&] { RawConstructionConfirm bad(pool,types,frames,state,missing); }));
    SquidFactory foreign(other,types);
    CHECK(Throws([&] { static_cast<void>(MakeConstructionConfirmHooks(pool,foreign,QuietHooks())); }));
    std::uint32_t sent=0;auto hooks=QuietHooks();hooks.broadcast=[&](const ConstructionNotice&) { ++sent; };
    RawConstructionConfirm confirm(pool,types,frames,state,hooks);
    // 생성 경계가 예약 번호 0을 돌려주면 통지를 보내기 전에 거부한다. 범위 밖 타입도 마찬가지다.
    CHECK(Throws([&] { static_cast<void>(confirm.Confirm({kTypePlain,20.0F,21.0F,1,0,0,0,0,0,0})); }) && sent==0);
    CHECK(Throws([&] { static_cast<void>(confirm.Confirm({300,20.0F,21.0F,1,0,0,0,0,0,0})); }) && sent==0);
    // 기록기가 켜져 있고 활성이면 첫 SID를, 조각이 없으면 0을 넘긴다(원본의 미초기화 값은 재현하지 않는다).
    state.recorderEnabled=true;std::vector<std::uint16_t> recorded;
    hooks.create=[](std::uint32_t,std::uint32_t) { return Sid{15010}; };hooks.recorderActive=[] { return true; };
    hooks.record=[&](Sid sid) { recorded.push_back(sid.value); };
    RawConstructionConfirm recording(pool,types,frames,state,hooks);
    CHECK(recording.Confirm({kTypePlain,20.0F,21.0F,1,0,0,0,0,0,0}));frames[kTypePlain].defaultFrame=-1;
    CHECK(recording.Confirm({kTypePlain,20.0F,21.0F,1,0,0,0,0,0,0}) && recorded==std::vector<std::uint16_t>({15010,0}) && sent==2);
}
