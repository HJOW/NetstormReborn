// 원본 미리보기 전체 몸체와 실제 Query/Prepare 사슬의 독립 기대값을 비교한다.
#include "RawSceneSupport.h"
#include "client/RawCursorPreviews.h"
#include <set>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 독립 입력14개의 순서이며 모든 값은 원문 fixture에서만 읽는다.
enum Column : std::size_t { Profile,Quality,Flags1,Flags2,Existing,Connectors,Hp,CodeProfile,X,Y,Mutation,Repeat,Prefix,Chain };
// 쉼표로 저장된 입력 DWORD를 순서대로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 결과를 계산하지 않고 입력 비트만 복원한다.
    for (const auto& value:Split(text,',')) result.push_back(Number(value));return result;
}
// 실제 FindFlags가 읽을 세 기본 코드와 품질 변형의 명시 입력이다.
RiftTypeFrames Frames(std::uint32_t profile) {
    const std::array<FrameCode,3> plain{{{65,49,0,0},{66,50,1,0},{67,51,2,0}}},special{{{193,192,0,128},{127,49,1,255},{65,255,2,16}}};std::vector<FrameCode> result;
    // 기본/cracked/hard 묶음을 원문과 같은 순서로 놓는다.
    for (const auto mask:{0,32,64}) {
        // 실제 코드 바이트의 signed 해석은 제품 FindFlags가 수행한다.
        for (auto code:profile ? special : plain) { code.flags=static_cast<std::uint8_t>(code.flags|mask);result.push_back(code); }
    }
    return RiftTypeFrames(std::move(result));
}
// 외부 decoder의 상대 좌표이며 기대 생성 좌표가 아니다.
std::vector<CursorCanonCell> Cells(std::uint32_t profile) {
    if (!profile) return {};if (profile==1) return {{0,0,0}};return {{0,0,0},{1,1.25f,-0.5f},{2,-2.75f,3.5f}};
}
// 한 실제 풀과 같은 커서 상태를 두 생성 호출까지 유지하는 장면이다.
struct PreviewScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 모든 Form/기존/생성/연결 SID의 단일 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 abstract TYPE/Factory 타입 입력이다.
    CursorDropState drop; // 원래 Form·위치·실제 타입과 허용 전역이다.
    CursorLifecycleState life; // 목록/현재 개수/글자 치수다.
    CursorPlacementState placementState; // 실제 Query/Prepare의 나머지 전역이다.
    BridgeConnectState surfaces; // bridge/island/stalag의 현재 공유 타입 전역이다.
    RiftTypeFrames frames{std::vector<FrameCode>{}}; // 현재 bridge 프레임 코드의 입력이다.
    std::vector<std::uint32_t> input; // 독립 입력14개다.
    std::set<std::uint16_t> touched; // 비교할 슬롯은 실제 경계가 접근한 번호만 포함한다.
    std::string trace; // 모든 경계의 순서와 인자다.
    std::uint16_t nextMain{},nextConnector{}; // 명시 Factory/연결 경계에서만 사용하는 할당 입력이다.
    std::uint32_t ownerCalls{}; // 재진입 입력을 첫 Owner에 적용한다.
    // 실제 client 풀에서 독립 입력에 필요한 SID를 확보한다.
    PreviewScene() {
        // 검사마다 실제 풀 객체를 새로 만들지 않고 확보한 슬롯만 재초기화한다.
        for (std::uint16_t sid=5;sid<230;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 현재 경계의 원래 DWORD 인자를 기록한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // signed 값도 원문 비트의 unsigned 십진수로 표시한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // Factory/연결 또는 기존 슬롯의 독립50바이트 입력이다. 실제 생성자 기대값이 아니다.
    void Raw(std::uint16_t sid,std::uint32_t type=100) {
        auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,1);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;raw[34]=7;Put(raw,26,99999);touched.insert(sid);
    }
    // 입력만 초기화한다. 한 입력의 반복 호출은 결과 상태를 그대로 이어 간다.
    void Prepare(const std::vector<std::uint32_t>& values) {
        input=values;drop={};life={};placementState={};surfaces={};trace.clear();touched.clear();nextMain=100;nextConnector=200;ownerCalls=0;
        // 이전 입력의 출력 바이트를 새 입력으로 재사용하지 않는다.
        for (const auto sid:{50,51,100,101,102,103,104}) Raw(static_cast<std::uint16_t>(sid));
        pool.AllocatedBytes(Sid{50})[30]=static_cast<std::uint8_t>(values[Quality]);pool.AllocatedBytes(Sid{51})[30]=3;
        // 첫 SID가 생성 여부를 정한다. 현재 개수는 별도로 유지한다.
        for (std::size_t index=0;index<5;++index) life.previews[index]=Sid{static_cast<std::uint16_t>(values[Existing] ? 100+index : 0)};
        life.previewCount=values[Existing] ? 5 : static_cast<std::int32_t>(values[Prefix]);life.abstractType=drop.type=100;drop.argument=7;drop.direction=3;drop.preview=Sid{50};drop.localPlayer=1;
        drop.x=std::bit_cast<float>(values[X]);drop.y=std::bit_cast<float>(values[Y]);drop.inputFlags=values[Chain] ? 0x40U : 0U;drop.screenX=-3;drop.screenY=47;
        life.warningWidth=35;life.warningHeight=17;drop.cursorWidth=49;life.costHeight=23;placementState.screenOrigin={-17,31};surfaces.bridgeType=99;surfaces.islandType=91;surfaces.stalagType=92;
        // 현재 TYPE 세 개는 재조회/Factory 인자 변경에 사용할 동일 입력을 갖는다.
        for (const auto type:{100,101,102}) { types[type].flags1=values[Flags1];types[type].flags2=values[Flags2];types[type].footX=2;types[type].footY=3; }
        frames=Frames(values[CodeProfile]);
    }
    // Query 경계가 변경한 전역을 본문이 읽는지 확인하는 명시 입력이다.
    void QueryMutation() {
        if (input[Mutation]==1) { life.abstractType=drop.type=101;drop.argument=99;drop.localPlayer=2;drop.x=-3.25f;drop.y=4.75f; }
        if (input[Mutation]==4) {
            // 실제로 재사용 가능한 목록을 Query 경계의 독립 입력으로 공급한다.
            for (std::size_t index=0;index<5;++index) life.previews[index]=Sid{static_cast<std::uint16_t>(100+index)};
            life.previewCount=5;
        }
    }
    // 복원 산술을 대신하지 않는 명시 효과만 원문과 같이 공급한다.
    CursorPreviewHooks Hooks() {
        CursorPreviewHooks hooks;hooks.queryCursor=[this] { Event("Q");QueryMutation(); };hooks.prepareCursor=[this] { Event("P"); };
        hooks.walk=[this](CanonTypeQuery query,const std::function<bool(CursorCanonCell)>& visit) {
            Event("W",{query.type,static_cast<std::uint32_t>(query.argument),static_cast<std::uint32_t>(query.direction),std::bit_cast<std::uint32_t>(query.x),std::bit_cast<std::uint32_t>(query.y),query.explicitFrame});
            // 상대 좌표를 실제 단정도로 저장하며 false이면 Advance를 생략한다.
            for (auto cell:Cells(input[Profile])) { cell.x+=query.x;cell.y+=query.y;if (!visit(cell)) return;Event("N"); }
        };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            const auto sid=nextMain++;Event("C",{type,flags,sid});Raw(sid,type);
            if (input[Mutation]==2 && nextMain==101) { drop.type=102;drop.argument=0xface9999;drop.localPlayer=2;drop.x=-7.75f;drop.y=6.5f; }return Sid{sid};
        };
        hooks.setWord=[this](Sid sid,std::uint32_t value) { Event("S",{sid.value,value});Put(pool.AllocatedBytes(sid),12,value,2); };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) {
            Event("O",{sid.value,owner});pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner);++ownerCalls;
            if (input[Mutation]==3 && ownerCalls==1) { types[100].flags2=4;drop.preview=Sid{51}; }
        };
        hooks.hasHitPoints=[this](Sid sid) { Event("H",{sid.value});return input[Hp]!=0; };
        hooks.setHitPoints=[this](Sid sid,std::int32_t value) { Event("Z",{sid.value,static_cast<std::uint32_t>(value)});Put(pool.AllocatedBytes(sid),26,static_cast<std::uint32_t>(value)); };
        hooks.frames=[this](std::uint32_t type)->const RiftTypeFrames& { Event("F",{type});return frames; };
        hooks.connect=[this](Sid sid,std::span<Sid> output) {
            const auto raw=pool.Slot(sid);const float x=std::bit_cast<float>(Get(raw,14,4)),y=std::bit_cast<float>(Get(raw,18,4));
            Event("B",{sid.value,Get(raw,14,4),Get(raw,18,4),Get(raw,22,2),Get(raw,24,2),Get(raw,36,4),static_cast<std::uint32_t>(life.previewCount)});
            // 4213b0의 실제 몸체는 별도 관찰 범위다. 여기서는 입력 위치와 SID만 공급한다.
            for (std::uint32_t index=0;index<input[Connectors];++index) {
                const auto target=nextConnector++;Raw(target,90);auto data=pool.AllocatedBytes(Sid{target});
                Put(data,14,std::bit_cast<std::uint32_t>(x+static_cast<float>(static_cast<int>(index%2)*2-1)));Put(data,18,std::bit_cast<std::uint32_t>(y+static_cast<float>(static_cast<int>(index%3)-1)));output[index]=Sid{target};
            }
            return static_cast<std::int32_t>(input[Connectors]);
        };
        return hooks;
    }
    // 실제 Query/Prepare 사슬의 외부 공급자다. 부모 세 몸체는 제품 코드를 실행한다.
    CursorPlacementHooks PlacementHooks(const CursorPreviewHooks& previews) {
        CursorPlacementHooks hooks;hooks.walk=previews.walk;
        hooks.describe=[this](std::string& text,float x,float y,std::uint32_t type,std::uint32_t owner) { Event("D",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),type,owner});text="Cursor";return 1U; };
        hooks.image=[this](std::uint32_t type,std::int32_t frame) { Event("I",{type,0,static_cast<std::uint32_t>(frame)});return CursorCostImage{1,31,23,8,17}; };
        hooks.gumpAt=[this](std::int32_t x,std::int32_t y) { Event("G",{static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y)});return 0U; };
        hooks.mayPlace=[this](const CanonPlacementQuery& query) { Event("M",{query.type,query.argument,std::bit_cast<std::uint32_t>(query.x),std::bit_cast<std::uint32_t>(query.y),query.flags,query.owner,query.mode});return 1U; };
        hooks.project=[this](float x,float y) { Event("J",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});return ScreenPoint{501,603}; };
        hooks.invalidate=[this](ScreenRect rect,std::uint32_t flags) { Event("V",{static_cast<std::uint32_t>(rect.left),static_cast<std::uint32_t>(rect.top),static_cast<std::uint32_t>(rect.right),static_cast<std::uint32_t>(rect.bottom),flags}); };
        return hooks;
    }
    // 원문52DWORD의 현재 상태를 같은 순서로 표시한다.
    std::string Snapshot() const {
        std::vector<std::uint32_t> values{static_cast<std::uint32_t>(life.previewCount),drop.type,drop.argument,drop.localPlayer,std::bit_cast<std::uint32_t>(drop.x),std::bit_cast<std::uint32_t>(drop.y),drop.legal,placementState.placementAllowed,placementState.descriptionAllowed,placementState.spaceAllowed,placementState.sourceAllowed};
        // 네 표시 값은 기준점/전체 폭/높이다.
        for (const auto value:placementState.bounds) values.push_back(static_cast<std::uint32_t>(value));
        values.insert(values.end(),{static_cast<std::uint32_t>(placementState.hotspotX),static_cast<std::uint32_t>(placementState.hotspotY),static_cast<std::uint32_t>(placementState.imageWidth),static_cast<std::uint32_t>(placementState.imageHeight)});
        // 미사용 칸도 포함한30SID를 그대로 비교한다.
        for (const auto sid:life.previews) values.push_back(sid.value);
        values.insert(values.end(),{life.abstractType,types[100].flags2,drop.preview.value});std::string result;
        // signed 값도 원문 DWORD 표현으로 보존한다.
        for (const auto value:values) result+=(result.empty() ? "" : ",")+std::to_string(value);return result;
    }
    // 접근한 모든 슬롯의 전체50바이트를 원문과 비교한다.
    std::string Raws() const {
        std::string result;
        // 번호 순서와 미수정 바이트까지 보존한다.
        for (const auto sid:touched) result+=(result.empty() ? "" : ";")+std::to_string(sid)+':'+Hex(pool.Slot(Sid{sid}));return result;
    }
};
}
// 전체 원문과 실제 생성→Query/Prepare 사슬의 반환/전체 상태/바이트/효과를 대조한다.
TEST_CASE(CursorPreviews_NativeGenerationReuseAndPlacementChain) {
    PreviewScene scene;std::size_t count{},chains{};
    // 입력마다 상태를 새로 주며 repeat 안에서는 같은 목록을 유지한다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_PREVIEWS_FIXTURE).rows) {
        CHECK(row.size()==4);if (row.size()!=4) continue;scene.Prepare(Numbers(row[0]));auto hooks=scene.Hooks();const auto placementHooks=scene.PlacementHooks(hooks);
        RawCursorPlacement placement(scene.pool,scene.types,{},scene.drop,scene.life,scene.placementState,placementHooks);
        if (scene.input[Chain]) { BindCursorPreviewPlacement(hooks,scene.pool,placement,placementHooks);++chains; }
        RawCursorPreviews previews(scene.pool,scene.types,scene.drop,scene.life,scene.surfaces,std::move(hooks));CursorLifecycleHooks lifecycle;BindCursorPreviews(lifecycle,previews);
        // 실제 원문처럼 두 번째 호출도 첫 호출 결과를 이어 사용한다.
        for (std::uint32_t call=0;call<scene.input[Repeat];++call) lifecycle.createPreviews();
        const bool same=(scene.trace.empty() ? "-" : scene.trace)==row[1] && scene.Snapshot()==row[2] && scene.Raws()==row[3];CHECK(same);
        if (!same) {
            std::printf("미리보기 입력=%s\n기대=%s / %s\n실제=%s / %s\n",row[0].c_str(),row[2].c_str(),row[1].c_str(),scene.Snapshot().c_str(),scene.trace.c_str());
            if (scene.Raws()!=row[3]) std::printf("기대 슬롯=%s\n실제 슬롯=%s\n",row[3].c_str(),scene.Raws().c_str());break;
        }++count;
    }
    CHECK(count==522);CHECK(chains==32);
}
// 누락 경계와 다른 판본/풀 연결을 성공으로 처리하지 않는다.
TEST_CASE(CursorPreviews_RequiresDependenciesAndOneWorld) {
    PreviewScene scene;scene.Prepare({1,1,0,4,0,2,1,0,0,0,0,1,0,0});RawCursorPreviews missing(scene.pool,scene.types,scene.drop,scene.life,scene.surfaces,{});
    CHECK(Throws([&] { missing.Create(); }));SidPool cd(OriginalEdition::Cd1072,24000,false);
    CHECK(Throws([&] { RawCursorPreviews invalid(cd,scene.types,scene.drop,scene.life,scene.surfaces,scene.Hooks()); }));
    auto hooks=scene.Hooks();const auto ph=scene.PlacementHooks(hooks);RawCursorPlacement placement(scene.pool,scene.types,{},scene.drop,scene.life,scene.placementState,ph);
    CHECK(Throws([&] { BindCursorPreviewPlacement(hooks,cd,placement,ph); }));CHECK(&missing.Pool()==&scene.pool);
}
