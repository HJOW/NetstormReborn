// 커서 내려놓기의 세 PE 관찰·현재 상태 읽기·실제 로컬 배치/SP/소리 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/RawCursorDrop.h"

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 실행기와 같은 합성 미리보기 번호 및 입력 열 위치다. 출력은 저장 fixture만 사용한다.
constexpr Sid kPreview{50};
// 입력 필드의 위치다. 비트값/번호는 DWORD 그대로 공급한다.
enum Column : std::size_t { Lock,Legal,X,Y,InputFlags,CostFlags,LegalGump,CursorGump,Recording,Argument,Direction,
    Flags,Quality,Allowed,First,Second,Cost,FootX,ScreenX,ScreenY,Width,Battle,Blocked,
    QueryChange,PrepareChange,SourceChange,DetachChange,ReceiveChange,LocalChange,AddChange };
// 저장 독립 관찰을 한 번 읽는다. CTest는 원본 PE/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CURSORDROP_FIXTURE).rows;return rows; }
// 원본 실행기의 명시 경계를 그대로 공급하는 장면이다.
struct DropScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    CursorDropState state;
    std::vector<std::uint32_t> input;
    std::string events;
    std::uint32_t prepared{},sources{};
    // 타입/풀 주소를 행 사이에서 유지한다.
    explicit DropScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {}
    // 사건의 전체 DWORD와 좌표 비트를 같은 순서로 기록한다.
    void Append(char letter,std::initializer_list<std::uint32_t> values) {
        if (!events.empty()) events+=';';events+=letter;
        // 인자는 원본 관찰처럼 부호 없는 십진수로 적는다.
        for (const auto value:values) events+=':'+std::to_string(value);
    }
    // 합성 타입/Form/현재 상태의 입력만 준비한다.
    void Prepare(const std::vector<std::uint32_t>& v) {
        input=v;events.clear();prepared=sources=0;state={};
        state.lockDepth=v[Lock];state.legal=v[Legal];state.preview=kPreview;state.x=Float(v[X]);state.y=Float(v[Y]);
        state.inputFlags=v[InputFlags];state.costFlags=v[CostFlags];state.legalGump=v[LegalGump] ? 1U : 0U;state.cursorGump=v[CursorGump] ? 2U : 0U;
        state.recording=v[Recording];state.recorder=27;state.type=100;state.argument=v[Argument];state.direction=v[Direction];
        state.screenX=std::bit_cast<std::int32_t>(v[ScreenX]);state.screenY=std::bit_cast<std::int32_t>(v[ScreenY]);state.cursorWidth=std::bit_cast<std::int32_t>(v[Width]);
        state.battle=v[Battle];state.blocked=v[Blocked];types[100].cost=Float(v[Cost]);types[100].footX=std::bit_cast<std::int32_t>(v[FootX]);types[100].name="cursorType";
        auto raw=InputRaw(pool,kPreview);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,20,v[Flags],2);
        raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 26]=static_cast<std::uint8_t>(v[Quality]);
    }
    // 실제 원본과 같은 명시 효과를 공급하고 지정된 현재 상태 변경만 수행한다.
    CursorDropHooks Hooks() {
        CursorDropHooks h;
        // 초기 판정 효과는 법적 위치와 깊이를 바꾸는 입력도 포함한다.
        h.queryCursor=[this] { Append('Q',{0});if (input[QueryChange]==1) state.legal^=1;if (input[QueryChange]==2) state.lockDepth=7; };
        // 첫 준비 뒤 반올림, 두 번째 준비 뒤 전역 SID 변경과 재스냅을 확인한다.
        h.prepareCursor=[this] {
            Append('P',{++prepared,Bits(state.x),Bits(state.y),state.preview.value});
            if (input[PrepareChange]==1 && prepared==1) { state.x=-0.500001F;state.y=0.499999F; }
            if (input[PrepareChange]==2 && prepared==2) { state.x=20.25F;state.y=21.75F;state.preview=Sid{51}; }
        };
        h.highlight=[this](std::uint32_t value) { Append('H',{value}); };
        h.playDrop=[this] { Append('S',{});events+=":droppiece.wav:0:0:0:0:0"; };
        // 첫 조회와 두 번째 조회는 다른 번호를 반환하며 raw 변경은 두 번째 조회에서 일어난다.
        h.previewSource=[this](Sid sid) {
            const auto result=input[++sources==1 ? First : Second];Append('U',{sid.value,result});
            if (input[SourceChange] && sources==2) { auto raw=InputRaw(pool,sid);Put(raw,20,input[Flags]^0x108,2);raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 26]=static_cast<std::uint8_t>(input[Quality]^3); }
            return result;
        };
        h.destroy=[this](Sid sid,std::uint32_t flags) { Append('D',{sid.value,flags}); };
        h.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Append('O',{sid.value,Bits(x),Bits(y),flags}); };
        h.record=[this](std::uint32_t recorder,std::string_view name,float x,float y,std::uint32_t argument,std::uint32_t direction) {
            Append('R',{recorder});events+=':'+std::string(name)+':'+std::to_string(Bits(x))+':'+std::to_string(Bits(y))+':'+std::to_string(argument)+':'+std::to_string(direction);
        };
        // 로컬 배치 뒤 깊이/비용 표시 플래그는 현재 전역으로 다시 읽는다.
        h.placeLocal=[this](const ConstructionLocalRequest& q) {
            Append('L',{q.preview.value,Bits(q.x),Bits(q.y),q.argument,q.direction});
            if (input[LocalChange]==1) state.lockDepth=0;if (input[LocalChange]==2) state.costFlags^=0x40;
        };
        h.addLocalSp=[this](float delta) { Append('A',{Bits(delta)});if (input[AddChange]) { types[100].cost=9.75F;state.screenX=-17;state.cursorWidth=-3; } };
        h.showSp=[this](std::int32_t amount,std::int32_t x,std::int32_t y,std::uint32_t flags) { Append('V',{std::bit_cast<std::uint32_t>(amount),std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags}); };
        h.battleRefresh=[this] { Append('B',{0}); };h.invalidGui=[this] { Append('I',{0}); };
        h.guiFlags=[this](std::uint32_t gump) { Append('G',{gump,901,0,0});return input[Allowed]; };
        h.guiParent=[](std::uint32_t) { return 123U; };
        // detach가 raw를 바꾸어도 반환은 이미 캡처된 기존 WORD/품질을 써야 한다.
        h.detach=[this](Sid sid,std::uint32_t flags) {
            Append('N',{sid.value,flags});if (input[DetachChange]) { auto raw=InputRaw(pool,sid);Put(raw,20,0xffff,2);raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 26]=0xfe; }
        };
        h.attach=[this](Sid sid,std::uint32_t parent,std::uint32_t flags) { Append('J',{sid.value,parent,flags}); };
        h.guiReceive=[this](std::uint32_t gump,Sid sid) { Append('G',{gump,902,sid.value,0});if (input[ReceiveChange]) state.inputFlags^=0x80; };
        h.returnPiece=[this](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { Append('T',{flags,source,quality}); };
        h.activatePreview=[this](std::uint32_t value) { Append('E',{value}); };h.selectCursor=[this](std::uint32_t value) { Append('C',{value}); };
        h.refresh=[this](std::uint32_t a,std::uint32_t b,std::uint32_t c) { Append('F',{a,b,c}); };
        h.closeGump=[this](std::uint32_t gump,std::uint32_t a,std::uint32_t b,std::uint32_t c) { Append('X',{gump,a,b,c}); };
        return h;
    }
    // 원본 관찰과 같은 최종 전역/raw 14개를 꺼낸다.
    std::string Snapshot() const {
        const auto raw=pool.Slot(kPreview);const std::array values{state.lockDepth,state.legal,static_cast<std::uint32_t>(state.preview.value),Bits(state.x),Bits(state.y),
            state.inputFlags,state.costFlags,state.legalGump ? 1U : 0U,state.cursorGump ? 1U : 0U,state.argument,state.direction,Bits(types[100].cost),Get(raw,20,2),
            static_cast<std::uint32_t>(raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 26])};
        std::string text;
        // 전역과 raw 상태는 저장된 관찰 순서대로 적는다.
        for (const auto value:values) text+=(text.empty() ? "" : ",")+std::to_string(value);
        return text;
    }
};
// 저장된 세 PE의 독립 관찰을 같은 입력으로 재생한다.
void Replay(std::string_view edition) {
    DropScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 기대 출력은 재계산하지 않고 원본 사건/최종 상태와 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==4);const auto v=Inputs(row[1]);CHECK(v.size()==30);scene.Prepare(v);
        RawCursorDrop drop(scene.pool,scene.types,scene.state,scene.Hooks());drop.Drop();if (scene.events.empty()) scene.events="-";
        const bool same=scene.events==row[2] && scene.Snapshot()==row[3];CHECK(same);
        if (!same) { std::printf("커서 %s 행 %zu 불일치\n입력 %s\n기대 %s / %s\n실제 %s / %s\n",std::string(edition).c_str(),count,row[1].c_str(),row[2].c_str(),row[3].c_str(),scene.events.c_str(),scene.Snapshot().c_str());break; }++count;
    }
    CHECK(count==686 && count*3==Fixture().size());
}
}
// 10.78 전체 커서 명령 관찰을 비교한다.
TEST_CASE(CursorDrop_ReplaysOriginals) { Replay("originals"); }
// CD의 GUI 비용 억제 표시와 잠금 helper도 비교한다.
TEST_CASE(CursorDrop_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 관찰을 비교한다.
TEST_CASE(CursorDrop_ReplaysExtra1037) { Replay("original1037"); }

// 원본 assert/잘못된 호스트 자료 계약을 거부하며 조기 반환은 자료를 읽지 않는다.
TEST_CASE(CursorDrop_RejectsInvalidInputs) {
    DropScene scene(OriginalEdition::Patch1078);scene.Prepare(Inputs(Fixture().front()[1]));auto h=scene.Hooks();
    CHECK(Throws([&] { RawCursorDrop bad(scene.pool,{},scene.state,h); }));CHECK(Throws([&] { RawCursorDrop bad(scene.pool,scene.types,scene.state,{}); }));
    RawCursorDrop drop(scene.pool,scene.types,scene.state,h);CHECK(&drop.Pool()==&scene.pool);
    scene.state.preview={};scene.state.lockDepth=1;drop.Drop();CHECK(scene.events.empty());
    scene.state.lockDepth=0;CHECK(Throws([&] { drop.Drop(); }));scene.state.preview=kPreview;scene.state.x=Float(0x7fc01234);
    CHECK(Throws([&] { drop.Drop(); }));scene.state.x=20;scene.state.type=300;scene.state.recording=1;CHECK(Throws([&] { drop.Drop(); }));
    scene.Prepare(Inputs(Fixture().front()[1]));scene.state.legalGump=1;h.guiParent=[](std::uint32_t) { return 0U; };
    RawCursorDrop missingParent(scene.pool,scene.types,scene.state,h);CHECK(Throws([&] { missingParent.Drop(); }));
    CHECK(Get(scene.pool.Slot(kPreview),20,2)==0x100 && scene.state.preview==kPreview && scene.state.lockDepth==0);
    scene.Prepare(Inputs(Fixture().front()[1]));scene.state.inputFlags=0x80;h=scene.Hooks();h.previewSource=[](Sid) { return 0xffffffffU; };
    RawCursorDrop badSource(scene.pool,scene.types,scene.state,h);CHECK(Throws([&] { badSource.Drop(); }));
}

// GUI 거부는 예측을 보존하며 새 건설의 두 잠금 이후 재호출은 효과를 발생시키지 않는다.
TEST_CASE(CursorDrop_PreservesRejectedGuiAndLocksConstruction) {
    // 두 판본의 조기 반환과 잠금 깊이를 같이 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DropScene scene(edition);auto v=Inputs(Fixture().front()[1]);v[LegalGump]=1;v[Allowed]=8;scene.Prepare(v);
        RawCursorDrop drop(scene.pool,scene.types,scene.state,scene.Hooks());drop.Drop();
        CHECK(scene.events.ends_with("I:0") && scene.state.preview==kPreview && scene.state.cursorGump==2 && scene.state.lockDepth==0);
        CHECK(Get(scene.pool.Slot(kPreview),20,2)==8);v[LegalGump]=0;scene.Prepare(v);drop.Drop();
        CHECK(scene.state.lockDepth==2 && scene.state.preview.value==0 && scene.state.cursorGump==0);
        const auto before=scene.events;drop.Drop();CHECK(scene.events==before);
    }
}
