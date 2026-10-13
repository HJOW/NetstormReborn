// 실제 일반 자산/그림자 수집 몸체와 SHP→해시/SID→공통 목록 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/ObjectCollection.h"
#include "client/GumpCollection.h"
#include "client/TileGroupCollection.h"
#include "client/DrawSort.h"
#include "client/VFXDraw.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 독립 원본의 풀 기준 토큰과 미사용/보호 영역을 포함한 작업 배열 폭이다.
constexpr std::uint32_t kObjectPool=0x15030000;
constexpr std::size_t kObjectCapacity=64;
// 완성 슬롯/SHP 입력을 바이트로 읽는다. 기대 경계나 목록을 계산하지 않는다.
std::vector<std::uint8_t> ObjectBytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 입력 바이트 하나다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 완성 raw 입력과 실제 타입/프레임 표를 독립 원본의 전체 출력과 대조한다.
void ReplayObjects(const std::string& edition) {
    static const auto rows=LoadFixture(NETSTORM_OBJECTCOLLECTION_FIXTURE).rows;
    SidPool pool(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false);
    SquidHash hash;std::size_t replayed=0;
    // 각 행의 기대 출력은 fixture에서만 읽는다.
    for (const auto& row:rows) if (row[0]==edition) {
        const auto config=Split(row[2],',');hash.Reset();hash.Select(std::stoi(config[5]));
        if (row[3]!="-") {
            // 0단계 입력도 보존해 일반 수집기가 방문하지 않는지 확인한다.
            for (const auto& cell:Split(row[3],';')) {
                const auto values=Inputs(cell);hash.Cell(static_cast<int>(values[0]),static_cast<int>(values[1]),static_cast<int>(values[2]))=static_cast<std::uint16_t>(values[3]);
            }
        }
        if (row[4]!="-") {
            // next/type/state/extra 및 미사용 바이트까지 실제 풀 슬롯에 공급한다.
            for (const auto& slot:Split(row[4],';')) {
                const auto pair=Split(slot,'=');const auto raw=ObjectBytes(pair[1]);const Sid sid{static_cast<std::uint16_t>(Number(pair[0]))};
                CHECK(raw.size()==pool.Layout().stride);std::copy(raw.begin(),raw.end(),InputRaw(pool,sid).begin());
            }
        }
        std::vector<RiftTypeRecord> types(5);std::vector<SquidDisplayShape> shapes(5);
        // 타입 플래그와 네 물리 SHP 경계를 C++ 공급 표로만 옮긴다.
        for (const auto& input:Split(row[5],';')) {
            const auto fields=Split(input,':');const auto number=Number(fields[0]);const auto flags=Split(fields[1],',');
            auto& type=types.at(number);type.flags1=Number(flags[0]);type.flags2=Number(flags[1]);type.maxHitPoints=std::stoi(flags[2]);
            auto& shape=shapes.at(number);shape.loaded=true;shape.frameCount=std::stoi(flags[3]);const auto geometry=ObjectBytes(fields[2]);
            // 각 16바이트 입력은 cellWidth/Height와 월드 hotspot 둘이다.
            for (std::size_t offset=0;offset<geometry.size();offset+=16) {
                SquidDisplayFrame frame;frame.cellWidth=Float(Get(geometry,offset));frame.cellHeight=Float(Get(geometry,offset+4));
                frame.cellHotspotX=Float(Get(geometry,offset+8));frame.cellHotspotY=Float(Get(geometry,offset+12));shape.frames.push_back(frame);
            }
        }
        const std::vector<std::uint8_t> before(pool.Bytes().begin(),pool.Bytes().end());std::vector<std::vector<std::uint16_t>> grids;
        // 네 단계 해시의 읽기 전용 입력을 따로 저장한다.
        for (int level=0;level<4;++level) grids.emplace_back(hash.Entries(level).begin(),hash.Entries(level).end());
        ObjectCollection collector(hash,pool,types,shapes,kObjectPool);std::array<RenderCollectionEntry,kObjectCapacity+2> guarded;
        std::memset(guarded.data(),0xa5,sizeof(guarded));auto output=std::span(guarded).subspan(1,kObjectCapacity);
        const RenderWorldRect view{Float(Number(config[0])),Float(Number(config[1])),Float(Number(config[2])),Float(Number(config[3]))};
        const auto count=collector.Collect(view,output,std::stoi(config[4]));
        const auto bytes=std::span(reinterpret_cast<const std::uint8_t*>(output.data()),output.size_bytes());
        std::array<std::uint8_t,32> guards;std::memcpy(guards.data(),guarded.data(),16);std::memcpy(guards.data()+16,&guarded.back(),16);
        const bool same=count==std::stoi(row[6]) && Hex(bytes)==row[7];CHECK(same);CHECK(Hex(guards)==row[10]);
        CHECK(hash.Selected()==2 && row[8]=="2,4,64");CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));
        // 원본처럼 각 칸에 같은 SID가 반복되어도 입력과 토큰을 보존한다.
        for (int level=0;level<4;++level) CHECK(std::equal(grids[level].begin(),grids[level].end(),hash.Entries(level).begin()));
        // 사용 항목의 포인터 대리 토큰은 모두 같은 raw 풀 번호로 돌아간다.
        for (int i=0;i<count;++i) CHECK(collector.Object(output[i].token).value==(output[i].token-kObjectPool)/pool.Layout().stride);
        if (!same) { std::printf("ObjectCollection %s %s: count=%d\n",edition.c_str(),row[1].c_str(),count);break; }++replayed;
    }
    CHECK(replayed==477);
}
// 계약과 실제 공통 제출 검사용 최소 타입/프레임 저장소다.
struct ObjectScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;/* hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidHash hash;/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(2);
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(2);
    // 판본별 슬롯 폭만 다르게 하며 논리 2/물리 4프레임을 준비한다.
    explicit ObjectScene(OriginalEdition edition=OriginalEdition::Patch1078):pool(edition,24000,false) {
        types[1].maxHitPoints=100;shapes[1].loaded=true;shapes[1].frameCount=2;shapes[1].frames.resize(4);
        // 각 물리 프레임은 같은 유효 월드 경계를 갖는다.
        for (auto& frame:shapes[1].frames) { frame.cellWidth=2;frame.cellHeight=3;frame.cellHotspotX=0.25f;frame.cellHotspotY=0.5f; }
        Place({5},20,25,10);hash.Cell(3,1,1)=5;
    }
    // next/type/frame/깊이를 포함한 슬롯을 매번 깨끗한 입력으로 채운다.
    void Place(Sid sid,float x,float y,std::int8_t depth) {
        auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=1;raw[11]=255;
        Put(raw,14,Bits(x));Put(raw,18,Bits(y));Put(raw,pool.Edition()==OriginalEdition::Patch1078 ? 35 : 33,static_cast<std::uint8_t>(depth),1);
    }
    // 판본별 signed DWORD/BYTE 프레임 필드에 입력을 쓴다.
    void SetFrame(std::uint32_t frame) { Put(InputRaw(pool,{5}),pool.Edition()==OriginalEdition::Patch1078 ? 36 : 34,frame,pool.Edition()==OriginalEdition::Patch1078 ? 4 : 1); }
};
}
// 패치판 전체 기계어의 정밀도/NaN/본체와 그림자 관찰을 재생한다.
TEST_CASE(ObjectCollection_PatchNative) { ReplayObjects("originals"); }
// CD판의 float 저장과 unordered 비교 차이를 그대로 대조한다.
TEST_CASE(ObjectCollection_CdNative) { ReplayObjects("originalCD"); }
// 추가판 PE의 독립 수집 관찰도 같은 복원 경로에서 대조한다.
TEST_CASE(ObjectCollection_AdditionalNative) { ReplayObjects("original1037"); }

// strict 제한은 기록 후, 실제 버퍼 크기는 기록 전에 검사한다. 실패 시 현재 해시/기록을 보존한다.
TEST_CASE(ObjectCollection_StrictLimitsAndCapacity) {
    ObjectScene scene;ObjectCollection collector(scene.hash,scene.pool,scene.types,scene.shapes);std::array<RenderCollectionEntry,4> work{};
    CHECK(Throws([&] { collector.Collect({10,20,30,40},work,1); }));CHECK(scene.hash.Selected()==3);CHECK(work[0].token==250);
    CHECK(Throws([&] { collector.Collect({10,20,30,40},{},10); }));
    scene.types[1].flags1=0x40000;work={};CHECK(Throws([&] { collector.Collect({10,20,30,40},work,2); }));CHECK(work[1].tail==4);
    CHECK(collector.Collect({10,20,30,40},work,3)==2);CHECK(scene.hash.Selected()==2);
    scene.hash.Reset();CHECK(collector.Collect({10,20,30,40},{},0)==0);CHECK(scene.hash.Selected()==2);
}

// 손상 체인/타입/프레임과 포인터 대리 토큰을 안전하게 거부하되 CD의 원본 프레임 차이는 유지한다.
TEST_CASE(ObjectCollection_ContractsAndEditionFrames) {
    ObjectScene patch,cd(OriginalEdition::Cd1072);ObjectCollection a(patch.hash,patch.pool,patch.types,patch.shapes),b(cd.hash,cd.pool,cd.types,cd.shapes);
    std::array<RenderCollectionEntry,8> work;patch.SetFrame(2);cd.SetFrame(2);
    CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));CHECK(b.Collect({10,20,30,40},work,8)==1);
    patch.SetFrame(0);patch.types[1].maxHitPoints=-0x22222223;cd.types[1].maxHitPoints=-0x22222223;
    CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));CHECK(b.Collect({10,20,30,40},work,8)==1);
    patch.types[1].maxHitPoints=100;patch.shapes[1].loaded=false;CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));patch.shapes[1].loaded=true;
    Put(InputRaw(patch.pool,{5}),4,5,2);CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));Put(InputRaw(patch.pool,{5}),4,0,2);
    InputRaw(patch.pool,{5})[10]=0;CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));InputRaw(patch.pool,{5})[10]=2;
    CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));patch.hash.Cell(3,1,1)=65535;CHECK(Throws([&] { a.Collect({10,20,30,40},work,8); }));
    CHECK(a.Object(250)==Sid{5});CHECK(Throws([&] { a.Object(0); }));CHECK(Throws([&] { a.Object(251); }));CHECK(Throws([&] { a.Object(24000*50); }));
    CHECK(Throws([&] { ObjectCollection invalid(patch.hash,patch.pool,patch.types,{},0); }));
    CHECK(Throws([&] { ObjectCollection invalid(patch.hash,patch.pool,patch.types,patch.shapes,0xffff0000U); }));
}

// 실제 UI·지면·자산/그림자를 같은 수집/CRT 정렬/종류 분배에 연결한다.
TEST_CASE(ObjectCollection_CommonSubmissionIntegration) {
    ObjectScene scene;scene.types[1].flags1=0x40000;scene.types[1].flags2=0x10000;scene.Place({6},20,25,10);scene.hash.Cell(3,1,1)=6;
    scene.Place({5},20,25,5);scene.hash.Cell(0,20,25)=5;
    ObjectCollection objects(scene.hash,scene.pool,scene.types,scene.shapes);TileGroupCollection tiles(scene.hash,scene.pool);
    std::map<std::uint32_t,GumpCollectionNode> nodes;nodes[1].child=2;auto& ui=nodes[2];ui.flags=1;ui.parent=1;ui.width=10;ui.height=10;ui.x=10;ui.y=10;ui.depth=15;
    GumpCollectionState state{1};GumpCollection gumps(state,{[&](auto token) { return &nodes.at(token); },{}});
    DrawCollectionHooks collection;DrawDispatchHooks dispatch;BindGumpCollection(collection,gumps);BindTileGroupCollection(collection,dispatch,tiles);BindObjectCollection(collection,objects);
    DrawCollectionRequest request;request.screen={0,0,100,100};request.visible={10,20,30,40};request.groupRowLimit=100;
    std::array<RenderCollectionEntry,12> work{};const auto result=CollectDrawEntries(request,work,collection);CHECK(result.count==4);
    auto entries=std::span(work).first(static_cast<std::size_t>(result.count));SortDrawEntries(entries,OriginalEdition::Patch1078);
    std::vector<std::string> drawn;
    // 그림자 y가 본체보다 작아 같은 깊이에서 먼저 분배된다. 지면 내부는 kind 1을 유지한다.
    dispatch.sprite=[&](const auto& entry) { drawn.push_back(std::to_string(objects.Object(entry.token).value)+":"+std::to_string(entry.kind)+":"+std::to_string(entry.tail)); };
    dispatch.ui=[&](auto token) { drawn.push_back("ui:"+std::to_string(token)); };
    DispatchDrawEntries(entries,{},dispatch);CHECK(drawn==std::vector<std::string>({"ui:2","6:0:4","6:0:0","5:1:0"}));
    drawn.clear();DispatchDrawEntries(entries,{true,false},dispatch);CHECK(drawn==std::vector<std::string>({"ui:2","6:0:4","6:0:0"}));
    request.dirtyFlags=4;scene.hash.Select(1);CHECK(CollectDrawEntries(request,work,collection).count==1);CHECK(scene.hash.Selected()==1);
}

// SHP의 월드 hotspot은 픽셀 hotspot과 별도이며 VFX 앞 28/24바이트에서 읽는다.
TEST_CASE(ObjectCollection_ShpWorldHotspots) {
    std::vector<std::uint8_t> bytes(79);Put(bytes,0,0x30312e31);Put(bytes,4,1);Put(bytes,8,52);
    Put(bytes,16,Bits(2));Put(bytes,20,Bits(3));Put(bytes,24,Bits(0.25f));Put(bytes,28,Bits(0.5f));
    Put(bytes,40,11,2);Put(bytes,42,17,2);Put(bytes,44,static_cast<std::uint16_t>(-12),2);Put(bytes,46,16,2);bytes[76]=2;bytes[77]=9;
    ShapeDatabase database(bytes);const auto metric=database.SquidMetrics(0,0);
    CHECK(metric.cellWidth==2 && metric.cellHeight==3);CHECK(metric.cellHotspotX==0.25f && metric.cellHotspotY==0.5f);
    CHECK(metric.hotspotX==-12 && metric.hotspotY==16);
}
