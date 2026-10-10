// 로컬 예측 조각 정리의 세 PE 관찰 재생과 칸 범위·후보 조건의 경계를 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionClear.h"
#include "o/RawConstructionConfirm.h"
#include "o/RawConstructionPlace.h"
#include "ConstructionNoticeSupport.h"

namespace {
using namespace netstorm::test::construction;
// 판본별 독립 관찰 행 수와 실행기가 후보로 섞는 다른 타입 번호다.
constexpr std::size_t kPatchRows=420,kCdRows=300;
constexpr std::uint32_t kOtherType=102;
// Python/원본 파일 없이 기대 출력만 한 번 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONCLEAR_FIXTURE).rows;return rows; }
// 풀·해시·타입·프레임과 후보 등록을 행마다 다시 준비하는 장면이다.
struct ClearScene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<PriestPlainCanonType> frames;
    ConstructionClearState state;
    bool patch;
    std::string events;
    // 풀 객체의 주소는 행 사이에 바뀌지 않는다.
    explicit ClearScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size()),patch(edition==OriginalEdition::Patch1078) {}
    // 놓을 타입과 다른 타입의 발자국, 프레임 표, 권한 전역을 준비하고 해시를 비운다.
    void Prepare(std::uint32_t type,int footX,int footY,int otherFootX,int otherFootY,std::uint32_t frameProfile,std::uint32_t defaultFrame,bool authority) {
        types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),PriestPlainCanonType{});hash.Reset();events.clear();
        types.at(type).footX=footX;types.at(type).footY=footY;types.at(kOtherType).footX=otherFootX;types.at(kOtherType).footY=otherFootY;
        frames.at(type)={InputFrames(frameProfile),std::bit_cast<int>(defaultFrame)};
        state=ConstructionClearState{};state.authority=authority;
    }
    // 후보 하나의 raw 슬롯을 쓰고 지정한 단계의 버킷 체인 머리에 잇는다. 실행기의 등록 순서와 같다.
    void Node(Sid sid,std::uint32_t type,std::uint8_t extra,float x,float y,std::uint8_t owner,std::uint32_t frame,int level) {
        auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[patch ? 40 : 35]=extra;raw[patch ? 34 : 32]=owner;
        Put(raw,14,Bits(x));Put(raw,18,Bits(y));Put(raw,patch ? 36 : 34,frame,patch ? 4 : 1);
        auto& head=hash.Bucket(level,x,y);Put(raw,4,head,2);head=sid.value;
    }
    // 사건 문자열에 한 항목을 잇는다.
    void Append(const std::string& value) { if (!events.empty()) events+=';';events+=value; }
    // 삭제는 기록만 하고 해시에서 빼지 않는다(실행기의 경계와 같다). 탐색 범위도 기록한다.
    ConstructionClearHooks Hooks() {
        return {[this](Sid sid,std::uint32_t flags) { Append("K:"+std::to_string(sid.value)+':'+std::to_string(flags)); },
            [this](const SquidSearchArea& area) {
                Append("A:"+std::to_string(area.left)+':'+std::to_string(area.top)+':'+std::to_string(area.right)+':'+std::to_string(area.bottom));
            }};
    }
};
// 한 판본의 모든 관찰 행을 재생한다. 칸마다의 탐색 범위와 지운 후보·삭제 플래그의 순서를 원본 출력과 그대로 비교한다.
void Replay(std::string_view edition) {
    ClearScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);std::size_t count=0;
    // 행마다 장면을 다시 만들고 같은 호출을 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==4);const auto v=Inputs(row[1]);CHECK(v.size()==13);
        scene.Prepare(v[0],std::bit_cast<int>(v[7]),std::bit_cast<int>(v[8]),std::bit_cast<int>(v[11]),std::bit_cast<int>(v[12]),v[9],v[10],v[6]!=0);
        if (row[2]!="-") {
            // 후보 칸은 번호, 타입, extra, x, y, 소유자, 프레임, 해시 단계다.
            for (const auto& entry:Split(row[2],';')) {
                const auto n=Inputs(entry);CHECK(n.size()==8);
                scene.Node(Sid{static_cast<std::uint16_t>(n[0])},n[1],static_cast<std::uint8_t>(n[2]),Float(n[3]),Float(n[4]),static_cast<std::uint8_t>(n[5]),n[6],static_cast<int>(n[7]));
            }
        }
        RawConstructionClear clear(scene.pool,scene.hash,scene.types,scene.frames,scene.state,scene.Hooks());
        clear.Clear({v[0],Float(v[3]),Float(v[4]),v[1],v[2],v[5]});
        const auto events=scene.events.empty() ? std::string("-") : scene.events;const bool same=events==row[3];
        CHECK(same);if (!same) { std::printf("건설 예측 정리 %s 행 %zu 불일치\n  입력 %s | %s\n  기대 %s\n  실제 %s\n",std::string(edition).c_str(),count,row[1].c_str(),row[2].c_str(),row[3].c_str(),events.c_str());break; }
        ++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
}

// 10.78의 정리 전체를 원본 관찰과 대조한다.
TEST_CASE(ConstructionClear_ReplaysOriginals) { Replay("originals"); }
// CD판의 정리 전체(다른 칸 범위 계산 코드)를 대조한다.
TEST_CASE(ConstructionClear_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 원본 관찰에 대조한다.
TEST_CASE(ConstructionClear_ReplaysExtra1037) { Replay("original1037"); }

// 칸 범위: 발자국만큼 왼쪽/위로 넓히고, 지도 밖 칸은 1~255로 자른 점 하나가 된다.
TEST_CASE(ConstructionClear_ComputesCellAreaWithClampAndValidity) {
    const auto same=[](SquidSearchArea area,int left,int top,int right,int bottom) { return area.left==left && area.top==top && area.right==right && area.bottom==bottom; };
    CHECK(same(RawConstructionClear::CellArea(20.5F,21.25F,1,1),20,21,21,22));
    CHECK(same(RawConstructionClear::CellArea(20.5F,21.25F,3,2),18,20,21,22));
    CHECK(same(RawConstructionClear::CellArea(20.0F,21.0F,1,1),20,21,20,21));
    // 왼쪽/위로 물린 점은 1 아래로 내려가지 않는다. 칸 좌표가 유효하면 칸 쪽 모서리는 그대로다.
    CHECK(same(RawConstructionClear::CellArea(0.5F,0.5F,2,2),0,0,1,1));
    // 칸 좌표가 0 이하이거나 256 이상이면 두 모서리 모두 자른 점이다.
    CHECK(same(RawConstructionClear::CellArea(0.0F,5.0F,1,1),1,5,1,5));
    CHECK(same(RawConstructionClear::CellArea(256.0F,300.0F,1,1),255,255,255,255));
    CHECK(same(RawConstructionClear::CellArea(-3.5F,-2.5F,2,2),1,1,1,1));
}

// 한 칸에서 첫 후보 하나만 지우고, 서버 영역 번호·abstract가 아닌 조각·(권한이 있을 때) 다른 타입은 건드리지 않는다.
TEST_CASE(ConstructionClear_RemovesOnePredictionPerCell) {
    // 두 판본에서 같은 장면을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        ClearScene scene(edition);scene.Prepare(kTypePlain,1,1,1,1,0,3,true);
        const auto server=static_cast<std::uint16_t>(scene.pool.Layout().serverFirst+4);
        scene.Node(Sid{60},kTypePlain,1,20.5F,21.25F,2,3,0);       // 같은 타입·프레임·소유자의 로컬 예측
        scene.Node(Sid{61},kTypePlain,1,20.5F,21.25F,2,3,0);       // 같은 칸의 두 번째 예측
        scene.Node(Sid{62},kTypePlain,0,20.5F,21.25F,2,3,0);       // abstract가 아닌 조각
        scene.Node(Sid{server},kTypePlain,1,20.5F,21.25F,2,3,0);   // 서버 영역 번호
        scene.Node(Sid{63},kOtherType,1,20.5F,21.25F,2,3,0);       // 다른 타입
        std::vector<std::pair<std::uint16_t,std::uint32_t>> removed;
        // 지운 조각이 정확히 하나이고 번호와 삭제 플래그가 기대와 같은지 본다.
        const auto only=[&](std::uint16_t sid,std::uint32_t flags) { return removed.size()==1 && removed[0].first==sid && removed[0].second==flags; };
        RawConstructionClear clear(scene.pool,scene.hash,scene.types,scene.frames,scene.state,{[&](Sid sid,std::uint32_t flags) { removed.emplace_back(sid.value,flags); },{}});
        clear.Clear({kTypePlain,20.5F,21.25F,7,0,2});
        // 체인의 머리는 마지막에 등록한 것부터다. 다른 타입(63)과 서버 번호를 건너뛰고 61 하나만 같은 조각으로 지운다.
        CHECK(only(61,RawConstructionClear::kSamePieceFlag));
        // 소유자가 다르거나 프레임이 다르면 삭제 플래그가 0이다.
        removed.clear();clear.Clear({kTypePlain,20.5F,21.25F,7,0,5});CHECK(only(61,0));
        // 권한이 없으면 타입이 다른 abstract 조각도 후보가 되어 먼저 만난 63을 플래그 0으로 지운다.
        scene.state.authority=false;removed.clear();clear.Clear({kTypePlain,20.5F,21.25F,7,0,2});
        CHECK(only(63,0));
    }
    // 삭제 경계가 없거나 타입 번호가 범위 밖이면 거부한다.
    ClearScene scene(OriginalEdition::Patch1078);scene.Prepare(kTypePlain,1,1,1,1,0,3,true);
    CHECK(Throws([&] { RawConstructionClear bad(scene.pool,scene.hash,scene.types,scene.frames,scene.state,{}); }));
    RawConstructionClear clear(scene.pool,scene.hash,scene.types,scene.frames,scene.state,scene.Hooks());
    CHECK(Throws([&] { clear.Clear({300,20.5F,21.25F,0,0,1}); }) && scene.events.empty());
}

// 로컬 예측 → 서버 확정 → 통지 처리(예측 정리 → 확정 조각 배치)를 실제 네 모듈과 실제 SID 생성·소유자 지정으로 한 번에 잇는다.
// Pop과 삭제는 탐색에 필요한 해시 등록/해제만 하는 경계이며, 실제 통지 처리기가 abstract를 판정한다.
TEST_CASE(ConstructionClear_ReplacesLocalPredictionWithConfirmedPieces) {
    // 두 판본에서 noIsland 아홉 칸을 예측으로 놓았다가 확정 조각으로 바꾼다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypeNoIsland].footX=types[kTypeNoIsland].footY=1;frames[kTypeNoIsland]={InputFrames(0),3};
        // 이 장면의 로컬 통지가 배치 이력도 갱신하도록 그룹 10 밖의 입력을 준다.
        types[kTypeNoIsland].group=1;
        SquidFactory factory(pool,types);SquidPostPopState books;SquidOwnerMode mode;SquidOwner owner(pool,types,books,mode);
        SquidRewardState rewardState;GameRandom rng;ScrambledSpStore store(rng,[] { return 1U; });
        SquidReward reward(pool,types,books,rewardState,patch ? &store : nullptr);
        ConstructionPlaceState placeState;placeState.localPlayer=2;
        ConstructionPlaceHooks placeHooks;
        placeHooks.take=[](std::uint32_t,Sid sid) { return sid; };placeHooks.setOwner=[](Sid,std::uint32_t) {};placeHooks.notifySurface=[](Sid) {};
        // Pop 경계: 좌표를 쓰고 void를 끈 뒤 해시 0단계 체인의 머리에 잇는다.
        placeHooks.pop=[&](Sid sid,float x,float y,std::uint32_t) {
            auto raw=pool.AllocatedBytes(sid);Put(raw,14,Bits(x));Put(raw,18,Bits(y));raw[11]=static_cast<std::uint8_t>(raw[11]&~4);
            auto& head=hash.Bucket(0,x,y);Put(raw,4,head,2);head=sid.value;
        };
        placeHooks.sendMoney=[](std::uint32_t,const ConstructionMoneyNotice&) {};placeHooks.storedSp=[](std::uint32_t) { return 0.0F; };
        placeHooks.addLocalSp=[](float) {};placeHooks.addPlayerSp=[](std::uint32_t,float) {};
        placeHooks.format=[](std::string_view,std::string_view) { return std::string{}; };placeHooks.tellOthers=[](std::uint32_t,std::string_view) {};
        placeHooks.tellPlayer=[](std::uint32_t,std::string_view) {};placeHooks.tell=[](std::string_view) {};
        placeHooks.sendReject=[](std::uint32_t,const ConstructionRejectNotice&) {};
        RawConstructionPlace place(pool,types,frames,placeState,reward,
            MakeConstructionPlaceHooks(pool,factory,owner,reward,patch ? &store : nullptr,placeState,placeHooks));
        // 1) 로컬 예측: 클라이언트 영역 SID 아홉 개로 abstract 배치를 한다.
        std::vector<std::uint16_t> predicted;
        // 조각마다 클라이언트 영역의 void 객체를 만든다.
        for (int i=0;i<9;++i) predicted.push_back(factory.Create(kTypeNoIsland,2).value);
        place.Place({kTypeNoIsland,20.0F,21.0F,0,0,2,predicted,1,0});
        CHECK(predicted.front()<pool.Layout().serverFirst && (pool.Slot(Sid{predicted.front()})[patch ? 40 : 35]&1)==1);
        // 2) 통지 처리: 예측 조각을 지우는 삭제 경계는 체인에서 빼고 SID를 반납한다.
        ConstructionClearState clearState;std::vector<std::uint16_t> removed;
        RawConstructionClear clear(pool,hash,types,frames,clearState,{[&](Sid sid,std::uint32_t flags) {
            CHECK(flags==RawConstructionClear::kSamePieceFlag);removed.push_back(sid.value);
            // 이 장면에서는 칸마다 예측 조각 하나만 등록돼 있으므로 버킷의 머리가 바로 그 조각이다.
            auto raw=pool.AllocatedBytes(sid);auto& head=hash.Bucket(0,Float(Get(raw,14)),Float(Get(raw,18)));
            CHECK(head==sid.value);head=static_cast<std::uint16_t>(Get(raw,4,2));
            raw[11]=static_cast<std::uint8_t>(raw[11]|4);pool.Release(sid);
        },{}});
        ConstructionNoticeState noticeState;noticeState.localPlayer=2;std::uint32_t refreshed=0;
        auto noticeHooks=SilentNoticeHooks();
        // 정상 통지 처리의 마지막 갱신을 정확히 한 번 요청했는지 센다.
        noticeHooks.refresh=[&] { ++refreshed; };
        RawConstructionNotice noticeHandler(pool,types,noticeState,MakeConstructionNoticeHooks(pool,clear,place,noticeHooks));
        ConstructionConfirmState confirmState;
        ConstructionConfirmHooks confirmHooks{[](std::uint32_t,std::uint32_t,float,float) { return Sid{77}; },{},[](const ConstructionNotice&) {},
            {},[] { return false; },[](Sid) {}};
        RawConstructionConfirm confirm(pool,types,frames,confirmState,MakeConstructionConfirmHooks(pool,factory,
            MakeConstructionNoticeConfirmHooks(pool,noticeHandler,confirmHooks)));
        CHECK(confirm.Confirm({kTypeNoIsland,20.0F,21.0F,2,0,0,0,0,0,0}) && refreshed==1);
        CHECK(noticeState.history[0].x==20.0F && noticeState.history[0].y==21.0F);
        SidPool other(edition,kCapacity,false);
        CHECK(Throws([&] { MakeConstructionNoticeHooks(other,clear,place,noticeHooks); }));
        // 예측 아홉 개가 칸 순서대로 모두 지워지고, 칸마다 서버 영역 번호의 확정 조각 하나만 남는다.
        CHECK(removed==predicted);
        RawSquidFinder finder(pool,hash,types);std::size_t found=0;
        // 아홉 칸 전체 범위에 남은 객체를 센다.
        for (Sid current=finder.Begin({20,21,22,23});current.value!=0;current=finder.Next()) {
            const auto raw=pool.Slot(current);++found;
            CHECK(current.value>=pool.Layout().serverFirst && (raw[patch ? 40 : 35]&1)==0 && raw[patch ? 34 : 32]==2 && raw[10]==kTypeNoIsland);
        }
        CHECK(found==9);
    }
}
