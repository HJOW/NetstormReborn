// 통지 처리기 전체 분기를 세 PE의 독립 출력과 비교하고 잘못된 입력·효과 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include <sstream>

namespace {
using namespace netstorm::test::construction;
// 각 PE의 독립 관찰 행 수와 합성 사제 타입 번호다.
constexpr std::size_t kRowsPerEdition=537;
constexpr std::uint32_t kPriestType=158;
// 저장된 원본 출력을 한 번 읽는다. Python/원본 PE는 CTest에서 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONNOTICE_FIXTURE).rows;return rows; }
// raw 풀·타입 표·전역과 효과 관찰을 행마다 새 입력으로 준비하는 장면이다.
struct NoticeScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    ConstructionNoticeState state;
    std::vector<std::uint32_t> values;
    ConstructionNotice notice;
    bool patch;
    std::string events;
    std::size_t priestCalls{},moveCalls{};
    // raw 입력은 실행기와 같은 번호에 놓되 실제 풀 할당 없이 공급한다.
    explicit NoticeScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {}
    // 관찰 사건을 원본 실행기와 같은 문자열로 잇는다.
    template<class... T> void Emit(char letter,const T&... fields) {
        std::ostringstream stream;stream<<letter;((stream<<':'<<fields),...);
        if constexpr (sizeof...(fields)==0) stream<<':';
        if (!events.empty()) events+=';';events+=stream.str();
    }
    // 입력 필드 순서는 decomp_constructionnotice_oracle.py의 KEYS와 같다.
    void Prepare(const std::string& input) {
        values=Inputs(input);CHECK(values.size()==36);const auto& v=values;
        types.assign(types.size(),RiftTypeRecord{});events.clear();priestCalls=moveCalls=0;state=ConstructionNoticeState{};
        types[v[0]].flags1=v[1];types[v[0]].group=std::bit_cast<int>(v[2]);types[v[17]].flags2=v[19];types[kPriestType].flags2=v[23];
        state.localPlayer=v[13];state.authority=v[14]!=0;state.moveLocalBuilder=v[15]!=0;state.daisType=v[16];state.historyCounter=v[34];
        // 초기 이력 비트는 실행기의 HISTORY 입력과 같다. 갱신 결과를 미리 계산하지 않는다.
        for (std::size_t i=0;i<10;++i) {
            const float value=Float(0x3f800000U+static_cast<std::uint32_t>(i)*0x01020304U);
            if (i%2) state.history[i/2].y=value;else state.history[i/2].x=value;
        }
        notice={};notice.type=static_cast<std::uint8_t>(v[0]);notice.x=Float(v[3]);notice.y=Float(v[4]);
        notice.argument=static_cast<std::uint8_t>(v[5]);notice.direction=static_cast<std::uint8_t>(v[6]);notice.player=static_cast<std::uint8_t>(v[7]);
        notice.flags=static_cast<std::uint8_t>(v[8]);notice.quality=static_cast<std::uint8_t>(v[9]);notice.timeLow=v[10];notice.timeHigh=v[11];
        // SID 목록의 순서는 실제 통지 입력 순서다.
        for (std::size_t i=0;i<v[12];++i) notice.sids.push_back(static_cast<std::uint16_t>(50+i));
        Node(Sid{50},v[0],0);Node(Sid{static_cast<std::uint16_t>(v[20])},kPriestType,v[22]);
    }
    // 원본 실행기에 공급한 객체 raw 슬롯과 좌표를 쓴다.
    void Node(Sid sid,std::uint32_t type,std::uint32_t flags) {
        auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(flags);
        Put(raw,14,values[25]);Put(raw,18,values[26]);
    }
    // 몸체 밖 효과만 관찰하며 배치/이동 경계의 합성 raw 변경도 원본 실행기와 같이 공급한다.
    ConstructionNoticeHooks Hooks() {
        auto hooks=SilentNoticeHooks();
        // 정리에 전달한 타입·좌표·decoder 인자·플레이어를 기록한다.
        hooks.clear=[this](const ConstructionClearRequest& r) { Emit('Q',r.type,Bits(r.x),Bits(r.y),r.argument,r.direction,r.player); };
        // 배치 시점의 인자와 이력을 기록하고 합성 배치 뒤 타입/소유자를 쓴다.
        hooks.place=[this](const ConstructionPlaceRequest& r) {
            std::string sids;
            // 전달받은 SID 전체 목록의 순서를 기록한다.
            for (const auto sid:r.sids) { if (!sids.empty()) sids+=',';sids+=std::to_string(sid); }
            Emit('P',r.type,Bits(r.x),Bits(r.y),r.argument,r.direction,r.player,r.sids.size(),sids,r.abstract,r.quality);
            Emit('H',NoticeHistory(state),state.historyCounter);
            auto raw=InputRaw(pool,Sid{r.sids.front()});raw[10]=static_cast<std::uint8_t>(values[17]);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(values[18]);
        };
        // 첫 SID와 도착 시각 두 DWORD의 순서를 기록한다.
        hooks.moveBuilder=[this](std::uint32_t player,Sid sid,std::uint32_t low,std::uint32_t high) { Emit('B',player,sid.value,low,high); };
        // 첫/두 번째 소유 사제 조회에 서로 다른 입력 SID를 돌려준다.
        hooks.findPriest=[this](std::uint32_t player) { const auto sid=values[priestCalls++==0 ? 20 : 21];Emit('R',player,sid);return Sid{static_cast<std::uint16_t>(sid)}; };
        // contained 타입 조회와 두 번째 사제의 삭제 인자를 기록한다.
        hooks.containedType=[this](std::uint32_t type) { Emit('T',type,values[35]);return values[35]; };
        hooks.removeContained=[this](Sid sid,std::uint32_t type) { Emit('X',sid.value,type,0,1); };
        // 이동 불가 조회의 SID와 합성 응답을 기록한다.
        hooks.immobile=[this](Sid sid) { Emit('I',sid.value,values[24]);return values[24]!=0; };
        // 접근점 계산에 공급한 현재 사제 좌표와 플래그별 결과 비트를 기록한다.
        hooks.approach=[this](Sid object,float x,float y,std::uint32_t flags) {
            const auto index=flags==7 ? 27U : 29U;Emit('A',object.value,Bits(x),Bits(y),flags,values[index],values[index+1]);
            return ConstructionNoticePoint{Float(values[index]),Float(values[index+1])};
        };
        // 거의 올림한 이동 인자를 기록하고 첫 경계가 사제 좌표를 바꾸는 입력도 반영한다.
        hooks.tryMove=[this](Sid priest,float x,float y,Sid object) {
            const auto success=values[moveCalls==0 ? 31 : 32];Emit('M',priest.value,Bits(x),Bits(y),object.value,success);
            if (moveCalls==0 && values[33]) { auto raw=InputRaw(pool,priest);Put(raw,14,Bits(15.25F));Put(raw,18,Bits(16.5F)); }
            ++moveCalls;return success!=0;
        };
        // 실패 시 현재 객체 타입·소유자 환불과 첫 객체 삭제 순서를 기록한다.
        hooks.refund=[this](std::uint32_t type,std::uint32_t owner) { Emit('F',type,owner); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Emit('D',sid.value,flags); };
        // 자동 건설 시작의 첫 SID와 최종 갱신 횟수를 기록한다.
        hooks.start=[this](Sid sid,std::uint32_t flags) { Emit('S',sid.value,flags); };
        hooks.refresh=[this] { Emit('W'); };return hooks;
    }
};
// 행마다 독립 원본의 효과 순서/인자와 배치 시점 및 최종 이력 비트를 모두 비교한다.
void Replay(std::string_view edition) {
    NoticeScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRowsPerEdition);
    // 각 판본의 저장된 원본 관찰을 순서대로 재생한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==5);scene.Prepare(row[1]);
        RawConstructionNotice handler(scene.pool,scene.types,scene.state,scene.Hooks());handler.Handle(scene.notice);
        const bool same=scene.events==row[2] && NoticeHistory(scene.state)==row[3] && scene.state.historyCounter==Number(row[4]);CHECK(same);
        if (!same) { std::printf("건설 통지 %s 행 %zu 불일치\n  입력 %s\n  기대 %s\n  실제 %s\n",std::string(edition).c_str(),count,row[1].c_str(),row[2].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 10.78 통지 처리기의 패치 전용 contained 효과까지 비교한다.
TEST_CASE(ConstructionNotice_ReplaysOriginals) { Replay("originals"); }
// CD판의 통지 처리기와 다른 슬롯/함수 배치를 비교한다.
TEST_CASE(ConstructionNotice_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE의 실제 출력에 비교한다.
TEST_CASE(ConstructionNotice_ReplaysExtra1037) { Replay("original1037"); }
// 잘못된 통지는 예측 삭제나 이력 갱신 전에 거부한다. CD판에서는 패치 전용 경계가 없어도 된다.
TEST_CASE(ConstructionNotice_RejectsInvalidInputsBeforeEffects) {
    // 두 판본의 입력/효과 연결 경계를 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        NoticeScene scene(edition);scene.Prepare(Fixture().front()[1]);auto hooks=scene.Hooks();
        if (!scene.patch) { hooks.containedType={};hooks.removeContained={}; }
        RawConstructionNotice handler(scene.pool,scene.types,scene.state,hooks);const auto history=NoticeHistory(scene.state);
        // 누락된 타입·빈 목록·상한 초과·0/풀 밖 번호를 각각 검사한다.
        for (int kind=0;kind<5;++kind) {
            auto invalid=scene.notice;
            if (kind==0) invalid.type=255;else if (kind==1) invalid.sids.clear();else if (kind==2) invalid.sids.resize(20,50);
            else invalid.sids={50,static_cast<std::uint16_t>(kind==3 ? 0 : kCapacity)};
            CHECK(Throws([&] { handler.Handle(invalid); }) && scene.events.empty() && NoticeHistory(scene.state)==history && scene.state.historyCounter==scene.values[34]);
        }
        hooks.refresh={};CHECK(Throws([&] { RawConstructionNotice bad(scene.pool,scene.types,scene.state,hooks); }));
        if (scene.patch) { hooks=scene.Hooks();hooks.removeContained={};CHECK(Throws([&] { RawConstructionNotice bad(scene.pool,scene.types,scene.state,hooks); })); }
        SidPool other(edition,kCapacity,false);ConstructionConfirmHooks confirmHooks;
        CHECK(Throws([&] { MakeConstructionNoticeConfirmHooks(other,handler,confirmHooks); }));
    }
}
// 이력은 배치 전에 갱신하며 배치 이후의 권한/로컬 플레이어 변화는 뒤쪽 사제 분기에서 다시 읽는다.
TEST_CASE(ConstructionNotice_ObservesStateChangesAtEffectBoundaries) {
    NoticeScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front()[1]);
    scene.types[scene.notice.type].flags1=0x8010;scene.state.localPlayer=scene.notice.player;scene.state.moveLocalBuilder=false;
    auto hooks=scene.Hooks();const auto place=hooks.place;
    // 배치 경계에서 로컬 플레이어가 바뀌는 상황을 공급한다.
    hooks.place=[&](const ConstructionPlaceRequest& request) { place(request);scene.state.localPlayer=5; };
    RawConstructionNotice handler(scene.pool,scene.types,scene.state,hooks);handler.Handle(scene.notice);
    CHECK(scene.events.find(";B:3:50:305419896:1074270772;W")!=std::string::npos);
    CHECK(Bits(scene.state.history[0].x)==Bits(scene.notice.x) && scene.state.historyCounter==0);
}
