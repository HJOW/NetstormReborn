// 실제 미리보기 기계어의 부분 출력/쌍/피드백과 기존 UI·자산 공통 제출을 검사한다.
#include "ConstructionSupport.h"
#include "CursorPreviewBindingSupport.h"
#include "client/PreviewCollection.h"
#include "client/GumpCollection.h"
#include "client/TileGroupCollection.h"
#include "client/ObjectCollection.h"
#include "client/DrawSort.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 독립 원본의 raw 풀 토큰 기준과 작업 칸 폭이다.
constexpr std::uint32_t kPreviewPool=0x15002000;
constexpr std::size_t kPreviewCapacity=64;
// 완성 fixture 슬롯의 모든 바이트를 그대로 읽는다.
std::vector<std::uint8_t> PreviewBytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 원본 입력 바이트 하나다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 원본 SP getter의 관찰 진입 횟수만 읽고 기대 호출 조건을 계산하지 않는다.
int NativeSpReads(const std::string& entries) {
    const std::string key="\"0040ec50\":";const auto start=entries.find(key);
    return start==std::string::npos ? 0 : std::stoi(entries.substr(start+key.size()));
}
// 세 PE의 완성 입력과 전체 작업 칸/표시 상태/실제 getter 진입을 대조한다.
void ReplayPreviews(const std::string& edition,bool bound=false) {
    static const auto rows=LoadFixture(NETSTORM_PREVIEWCOLLECTION_FIXTURE).rows;
    SidPool pool(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false);std::size_t replayed=0;
    // 행별 기대값은 저장한 원본 관찰만 사용한다.
    for (const auto& row:rows) if (row[0]==edition) {
        const auto c=Split(row[2],',');std::vector<std::uint32_t> v;
        // 고정 필드 21개는 DWORD 비트를 보존한다. 제한 인수의 signed 문자열은 따로 읽는다.
        for (std::size_t i=0;i<21;++i) v.push_back(Number(c[i]));
        PreviewCollectionState state;state.lockDepth=v[0];state.pieceCount=std::bit_cast<std::int32_t>(v[1]);state.type=1;state.parentGump=v[4];
        state.legal=v[6];state.x=Float(v[7]);state.y=Float(v[8]);state.specialTypeA=v[9];state.specialTypeB=v[10];state.scrambledOffset=v[11];state.copiedOffset=v[12];
        state.now=v[13];state.highlight=v[14];state.countdown=v[15];state.querySp=v[16];state.localPlayer=v[17];state.firstSpPoolPresent=v[18]!=0;
        state.firstSpIndex=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(v[19]));
        if (row[3]!="-") {
            // 목록 순서/중복은 원본 입력 그대로 공급한다.
            for (const auto& sid:Split(row[3],',')) state.pieces.push_back({static_cast<std::uint16_t>(Number(sid))});
        }
        std::vector<std::uint8_t> spots(65536);
        if (row[4]!="-") {
            // 희소 spot 입력이며 방문 주소나 성공 여부를 계산하지 않는다.
            for (const auto& input:Split(row[4],';')) { const auto values=Inputs(input);spots.at(values[0])=static_cast<std::uint8_t>(values[1]); }
        }
        if (row[5]!="-") {
            // next/depth/frame/미사용 바이트도 전체 슬롯에 넣어 실제 사용 여부를 대조한다.
            for (const auto& slot:Split(row[5],';')) {
                const auto pair=Split(slot,'=');const auto bytes=PreviewBytes(pair[1]);CHECK(bytes.size()==pool.Layout().stride);
                std::copy(bytes.begin(),bytes.end(),InputRaw(pool,{static_cast<std::uint16_t>(Number(pair[0]))}).begin());
            }
        }
        std::vector<RiftTypeRecord> types(2);types[1].flags2=v[2];types[1].zOrder=std::bit_cast<std::int32_t>(v[3]);
        const std::vector<std::uint8_t> before(pool.Bytes().begin(),pool.Bytes().end()),beforeSpots=spots;std::vector<std::uint32_t> owners;
        // 부모의 실제 깊이/준비된 SP 읽기의 반환 비트만 공급한다. 원본 쪽은 getter 전체를 실행했다.
        PreviewCollectionHooks hooks{[&](auto token) { CHECK(token==v[4]);return v[5]; },
            [&](auto owner) { owners.push_back(owner);return v[20]; }};
        netstorm::test::previewbinding::Scene actual;actual.Collection(state);
        PreviewCollection collector(pool,types,spots,state,hooks,kPreviewPool);
        if (bound) {
            collector.BindCursor(actual.Source(pool));state.lockDepth=1;state.type=0xffffffff;state.pieceCount=0;state.pieces.clear();
            state.parentGump=0xffffffff;state.localPlayer=0xffffffff;state.legal=!actual.placement.placementAllowed;state.x=-900;state.y=-900;
        }
        std::array<RenderCollectionEntry,kPreviewCapacity+2> guarded;
        std::memset(guarded.data(),0xa5,sizeof(guarded));auto work=std::span(guarded).subspan(1,kPreviewCapacity);
        const auto count=collector.Collect({Float(Number(c[22])),Float(Number(c[23])),Float(Number(c[24])),Float(Number(c[25]))},work,std::stoi(c[21]));
        const auto bytes=std::span(reinterpret_cast<const std::uint8_t*>(work.data()),work.size_bytes());std::array<std::uint8_t,32> guards;
        std::memcpy(guards.data(),guarded.data(),16);std::memcpy(guards.data()+16,&guarded.back(),16);
        const auto feedback=Inputs(row[8]);const bool same=count==std::stoi(row[6]) && Hex(bytes)==row[7] && state.highlight==feedback[0] && state.countdown==feedback[1];
        CHECK(same);CHECK(Hex(guards)==row[10]);CHECK(owners.size()==static_cast<std::size_t>(NativeSpReads(row[9])));
        // 모든 관찰 SP 읽기는 현재 플레이어로 호출되어야 한다.
        for (const auto owner:owners) CHECK(owner==v[17]);
        CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));CHECK(spots==beforeSpots);
        // 두 레이어의 실제 raw 객체 토큰은 같은 풀 번호로 돌아간다.
        for (int i=0;i<count;++i) CHECK(collector.Object(work[i].token).value==(work[i].token-kPreviewPool)/pool.Layout().stride);
        if (!same) { std::printf("PreviewCollection %s %s: count=%d highlight=%u down=%u\n",edition.c_str(),row[1].c_str(),count,state.highlight,state.countdown);break; }++replayed;
    }
    CHECK(replayed==520);
}
// 실제 host 연결/보호 검사용 최소 커서 목록·타입·raw 슬롯이다.
struct PreviewScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(2);/* spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    PreviewCollectionState state;
    // 미리보기와 raw 타입을 분리하고 기본 한 조각을 준비한다.
    explicit PreviewScene(OriginalEdition edition=OriginalEdition::Patch1078):pool(edition,24000,false) {
        types[1].zOrder=7;state.type=1;state.pieceCount=1;state.pieces={{5}};state.legal=1;state.specialTypeA=82;state.specialTypeB=94;
        state.x=20;state.y=25;state.now=99;state.highlight=77;Place({5},1,20,25);
    }
    // 프레임/깊이/next를 0으로 만들고 현재 raw 위치/타입만 넣는다.
    void Place(Sid sid,std::uint8_t type,float x,float y) {
        auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=255;Put(raw,14,Bits(x));Put(raw,18,Bits(y));
    }
};
}
// 패치의 실제 부분 출력/원래 두 깊이/강조 상태/SP 진입을 재생한다.
TEST_CASE(PreviewCollection_PatchNative) { ReplayPreviews("originals"); }
// 같은520개 원문 입력을 실제 커서 참조에 공급하며 중복 입력은 서로 다르게 둔다.
TEST_CASE(PreviewCollection_PatchNativeSharedCursor) { ReplayPreviews("originals",true); }
// CD의 별도 활성 판정과 SP 피드백을 바꾸지 않는 수집을 재생한다.
TEST_CASE(PreviewCollection_CdNative) { ReplayPreviews("originalCD"); }
// 추가판의 독립 기계어 관찰도 같은 복원 경로에서 대조한다.
TEST_CASE(PreviewCollection_AdditionalNative) { ReplayPreviews("original1037"); }

// soft 쌍 제한과 실패 후보의 14바이트 부분 기록, 실제 span의 넘침 방지를 구분한다.
TEST_CASE(PreviewCollection_PairLimitAndPartialWrites) {
    PreviewScene scene;PreviewCollection collector(scene.pool,scene.types,scene.spots,scene.state);std::array<RenderCollectionEntry,4> work;
    std::memset(work.data(),0xa5,sizeof(work));CHECK(collector.Collect({},std::span(work).first(2),1)==2);CHECK(work[0].tail==0 && work[1].tail==2 && work[2].kind==0xa5);
    scene.state.countdown=3;std::memset(work.data(),0xa5,sizeof(work));CHECK(Throws([&] { collector.Collect({},std::span(work).first(1),1); }));
    CHECK(work[0].x==20 && work[0].depth==7 && work[0].kind==0xa5 && work[0].tail==0xa5);CHECK(work[1].kind==0xa5 && scene.state.countdown==3);
    scene.Place({5},82,20,25);scene.state.legal=0;CHECK(collector.Collect({},std::span(work).first(1),1)==0);CHECK(work[0].token==250 && work[0].kind==0xa5);
    scene.state.lockDepth=1;scene.state.pieces={{65535}};CHECK(collector.Collect({},{},100)==0);
    scene.state.lockDepth=0;scene.state.pieceCount=-1;CHECK(collector.Collect({},{},100)==0);
}

// 손상 목록/토큰/부모/SP/spot 계약은 원본의 잘못된 메모리 접근 대신 예외로 거부한다.
TEST_CASE(PreviewCollection_Contracts) {
    PreviewScene scene;PreviewCollection collector(scene.pool,scene.types,scene.spots,scene.state);std::array<RenderCollectionEntry,4> work;
    scene.state.pieceCount=2;CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.pieceCount=1;
    scene.state.pieces[0]={0};CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.pieces[0]={65535};CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.pieces[0]={5};
    scene.state.type=2;CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.type=1;scene.state.parentGump=2;
    CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.parentGump=0;scene.state.countdown=1;scene.state.querySp=1;
    CHECK(Throws([&] { collector.Collect({},work,10); }));scene.state.querySp=0;scene.Place({5},82,20,25);scene.state.x=-20;scene.state.y=0;
    CHECK(Throws([&] { collector.Collect({},work,10); }));CHECK(collector.Object(250)==Sid{5});
    CHECK(Throws([&] { collector.Object(0); }));CHECK(Throws([&] { collector.Object(251); }));CHECK(Throws([&] { collector.Object(24000*50); }));
    CHECK(Throws([&] { PreviewCollection invalid(scene.pool,scene.types,{},scene.state); }));
    CHECK(Throws([&] { PreviewCollection invalid(scene.pool,scene.types,scene.spots,scene.state,{},0xffff0000U); }));
    GameRandom rng;ScrambledSpStore unprepared(rng,[] { return 1U; });CHECK(Throws([&] { MakePreviewSpHooks(unprepared); }));
}

// 준비한 실제 SP 저장소의 float 비트를 읽고 강조/카운터 우선순위와 CD 불변 상태를 확인한다.
TEST_CASE(PreviewCollection_ActualSpFeedback) {
    PreviewScene scene;GameRandom rng(1);ScrambledSpStore store(rng,[] { return 17U; });store.Initialize();
    scene.state.scrambledOffset=store.Offset();scene.state.copiedOffset=store.Offset();scene.state.querySp=1;scene.state.localPlayer=3;scene.state.countdown=3;
    PreviewCollection collector(scene.pool,scene.types,scene.spots,scene.state,MakePreviewSpHooks(store));std::array<RenderCollectionEntry,8> work;
    store.Set(3,Bits(7000));CHECK(collector.Collect({},work,8)==2);CHECK(scene.state.highlight==77 && scene.state.countdown==2);
    store.Set(3,Bits(7000.00048828125f));CHECK(collector.Collect({},work,8)==2);CHECK(scene.state.highlight==99 && scene.state.countdown==1);
    store.Set(3,0x7fc00000);scene.state.highlight=77;CHECK(collector.Collect({},work,8)==2);CHECK(scene.state.highlight==77 && scene.state.countdown==0);
    scene.state.querySp=0;scene.state.firstSpIndex=-1;CHECK(collector.Collect({},work,8)==2);CHECK(scene.state.highlight==99);
    scene.state.firstSpIndex=0;scene.state.highlight=77;scene.state.firstSpPoolPresent=false;CHECK(collector.Collect({},work,8)==2);CHECK(scene.state.highlight==99);
    PreviewScene cd(OriginalEdition::Cd1072);cd.state.querySp=1;cd.state.countdown=3;cd.state.firstSpPoolPresent=false;cd.state.firstSpIndex=-1;
    PreviewCollection cdCollector(cd.pool,cd.types,cd.spots,cd.state);CHECK(cdCollector.Collect({},work,8)==2);CHECK(cd.state.highlight==77 && cd.state.countdown==3);
}

// 조회 경계가 상태를 바꾸면 두 번째 토큰/다음 반복은 현재 목록을 읽고 XY/깊이는 저장한 값을 쓴다.
TEST_CASE(PreviewCollection_CurrentReadsAndSavedKey) {
    PreviewScene scene;scene.Place({6},1,-10,-30);scene.state.pieces.push_back({6});scene.state.pieceCount=2;scene.state.querySp=1;scene.state.countdown=1;
    PreviewCollectionHooks hooks;hooks.readSp=[&](auto) { scene.state.pieces[0]={6};scene.state.pieceCount=1;scene.Place({5},1,70,80);scene.types[1].zOrder=100;return Bits(0); };
    PreviewCollection collector(scene.pool,scene.types,scene.spots,scene.state,hooks);std::array<RenderCollectionEntry,8> work;
    CHECK(collector.Collect({},work,8)==2);CHECK(work[0].token==250 && work[1].token==300);
    CHECK(work[1].x==20 && work[1].y==25 && work[0].depth==7 && work[1].depth==6);
}

// 실제 네 수집기의 UI·지면·본체/그림자·미리보기를 같은 CRT/kind 분배에 연결한다.
TEST_CASE(PreviewCollection_CommonSubmissionIntegration) {
    PreviewScene scene;scene.types[1].zOrder=12;scene.types[1].maxHitPoints=100;scene.types[1].flags1=0x40000;scene.types[1].flags2=0x10000;
    scene.Place({5},1,20,25);Put(InputRaw(scene.pool,{5}),35,5,1);scene.Place({6},1,20,25);Put(InputRaw(scene.pool,{6}),35,10,1);scene.Place({7},1,500,-100);
    scene.state.pieces={{7}};SquidHash hash;hash.Cell(0,20,25)=5;hash.Cell(3,1,1)=6;
    std::vector<SquidDisplayShape> shapes(2);shapes[1].loaded=true;shapes[1].frameCount=2;shapes[1].frames.resize(4);
    // 실제 자산 수집이 사용하는 월드 경계를 각 물리 프레임에 공급한다.
    for (auto& frame:shapes[1].frames) { frame.cellWidth=2;frame.cellHeight=3; }
    ObjectCollection objects(hash,scene.pool,scene.types,shapes);TileGroupCollection tiles(hash,scene.pool);
    std::map<std::uint32_t,GumpCollectionNode> nodes;nodes[1].child=2;nodes[2].flags=1;nodes[2].parent=1;nodes[2].width=10;nodes[2].height=10;nodes[2].x=10;nodes[2].y=10;nodes[2].depth=15;
    GumpCollectionState uiState{1};GumpCollection gumps(uiState,{[&](auto token) { return &nodes.at(token); },{}});
    PreviewCollection preview(scene.pool,scene.types,scene.spots,scene.state);DrawCollectionHooks collection;DrawDispatchHooks dispatch;
    BindGumpCollection(collection,gumps);BindTileGroupCollection(collection,dispatch,tiles);BindObjectCollection(collection,objects);BindPreviewCollection(collection,preview);
    DrawCollectionRequest request;request.screen={0,0,100,100};request.visible={10,20,30,40};request.unclipped={100,100,-100,-100};request.groupRowLimit=100;
    std::array<RenderCollectionEntry,16> work{};const auto result=CollectDrawEntries(request,work,collection);CHECK(result.count==6);
    auto entries=std::span(work).first(static_cast<std::size_t>(result.count));SortDrawEntries(entries,OriginalEdition::Patch1078);std::vector<std::string> drawn;
    // 미리보기는 다른 종류보다 깊이가 높고 두 레이어의 tail을 분배 경계까지 보존한다.
    dispatch.ui=[&](auto token) { drawn.push_back("ui:"+std::to_string(token)); };
    dispatch.sprite=[&](const auto& entry) { drawn.push_back("s:"+std::to_string(objects.Object(entry.token).value)+":"+std::to_string(entry.tail)); };
    dispatch.preview=[&](const auto& entry) { drawn.push_back("p:"+std::to_string(preview.Object(entry.token).value)+":"+std::to_string(entry.tail));CHECK(entry.x==500 && entry.y==-100); };
    DispatchDrawEntries(entries,{},dispatch);CHECK(drawn==std::vector<std::string>({"ui:2","p:7:0","p:7:2","s:6:4","s:6:0","s:5:0"}));
    scene.state.countdown=3;request.dirtyFlags=4;CHECK(CollectDrawEntries(request,work,collection).count==1);CHECK(scene.state.countdown==3);
}

// 실제 SP 경계가 임시 목록을 바꾼 직후 보조 토큰과 다음 반복이 현재 배열을 읽는다.
TEST_CASE(PreviewCollection_SharedCursorCurrentReadsAndContracts) {
    PreviewScene scene;scene.Place({6},1,-10,-30);scene.state.pieces.push_back({6});scene.state.pieceCount=2;
    scene.state.querySp=1;scene.state.countdown=1;netstorm::test::previewbinding::Scene actual;actual.Collection(scene.state);
    PreviewCollectionHooks hooks;hooks.readSp=[&](auto owner) {
        CHECK(owner==actual.drop.localPlayer);actual.lifecycle.previews[0]={6};actual.lifecycle.previewCount=1;
        scene.Place({5},1,70,80);scene.types[1].zOrder=100;return Bits(0);
    };
    PreviewCollection collector(scene.pool,scene.types,scene.spots,scene.state,hooks);collector.BindCursor(actual.Source(scene.pool));
    scene.state.lockDepth=1;std::array<RenderCollectionEntry,8> work;
    CHECK(collector.Collect({},work,8)==2);CHECK(work[0].token==250 && work[1].token==300);
    CHECK(work[1].x==20 && work[1].y==25 && work[0].depth==7 && work[1].depth==6);
    CHECK(actual.drop.legal==0 && actual.placement.placementAllowed==1);
    PreviewScene other;CHECK(Throws([&] { collector.BindCursor(actual.Source(other.pool)); }));
    PreviewScene cd(OriginalEdition::Cd1072);PreviewCollection cdCollector(cd.pool,cd.types,cd.spots,cd.state);
    CHECK(Throws([&] { cdCollector.BindCursor(actual.Source(cd.pool)); }));
}
