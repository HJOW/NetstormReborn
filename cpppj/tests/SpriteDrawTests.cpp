// 독립 PE 명령열과 일반 Draw의 실제 SHP/그룹 연결·계약·현재 상태를 검사한다.
#include "ConstructionSupport.h"
#include "client/SpriteDraw.h"
#include "client/DrawSort.h"
#include "client/ObjectCollection.h"
#include <cmath>
#include <sstream>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 실제 fixture의 raw 주소와 호스트 SID 5를 연결하는 기준이다.
constexpr std::uint32_t kSpriteRaw=0x16001000;
// DWORD 인수만 직렬화해 native 출력과 부호/폭을 그대로 대조한다.
template<class... V> std::string SpriteEvent(const char* name,V... values) {
    std::ostringstream out;out<<name;((out<<','<<static_cast<std::uint32_t>(values)),...);return out.str();
}
// 세 판본의 완성 입력을 재생하며 명령열/선택 SID/그림자 alias 쓰기를 함께 대조한다.
void ReplaySpriteDraw(const std::string& edition) {
    static const auto rows=LoadFixture(NETSTORM_SPRITEDRAW_FIXTURE).rows;
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false);int replayed=0;
    // 기대 분기/그림/막대는 계산하지 않고 원본이 관찰한 전체 호출열과 비교한다.
    for (const auto& row:rows) if (row[0]==edition) {
        const auto v=Inputs(row[2]);SpriteDrawState s;std::vector<RiftTypeRecord> types(91);std::vector<SpriteTypeView> views(91);
        // signed DWORD 입력을 원래 비트로 복원한다.
        const auto integer=[&](std::size_t i) { return std::bit_cast<std::int32_t>(v[i]); };
        s.cameraX=integer(5);s.cameraY=integer(6);s.localPlayer=v[15];s.specialMode=v[17];s.filteredFlags=v[18];s.wideOwnerType=v[14] ? 70 : 90;s.unfilteredType=v[21] ? 70 : 90;
        s.shadowAliasType=v[36];s.shadowAliasPinned=v[37]!=0;s.quarterHitPoints=v[35]!=0;s.selectionDisabled=v[28]!=0;
        s.markerEnabled=v[24]!=0;s.markerVisible=v[25]!=0;s.markerType=71;s.markerExcludeType=73;s.statusType=75;s.abstractStatusType=76;
        s.costType=v[46] ? 70 : 90;s.timerProcessType=40;s.statusProcessType=41;s.now=100.25;
        // 판본 전역의 개별 타이머 타입 번호 입력이다.
        for (std::size_t i=0;i<3;++i) s.timerTypes[i]=v[44]==i+1 || v[44]==4 ? 70 : 90;
        s.ownerMaps.resize(91);s.ownerMaps[70]=static_cast<std::uint8_t>(v[16]);s.ownerPalettes.resize(301);
        // 원본 표에 준비한 플레이어별 팔레트 번호다.
        for (std::size_t i=0;i<s.ownerPalettes.size();++i) s.ownerPalettes[i]=static_cast<int>(i%3+1);
        s.selectionColors={180,181,182,183};s.hitPointColors={100,101,102};s.barBackground=9;
        // native SHP의 모든 물리 프레임은 같은 측정 헤더를 갖는 입력이다.
        for (auto& view:views) {
            view.shape.loaded=true;view.shape.frameCount=integer(12);view.gumpFrame=v[23];view.shape.frames.assign(64,
                {static_cast<std::int16_t>(integer(29)),static_cast<std::int16_t>(integer(30)),static_cast<std::int16_t>(integer(31)),static_cast<std::int16_t>(integer(32))});
        }
        views[70].shape.loaded=v[11]!=0;views[74].capacity=integer(42);views[74].constructionTime=integer(39);views[75].duration=30;views[76].duration=20;
        auto& type=types[70];type.hotspotX=integer(7);type.hotspotY=integer(8);type.footX=integer(9);type.footY=integer(10);type.flags1=v[19];type.flags2=v[20];type.maxHitPoints=integer(34);type.cost=std::bit_cast<float>(v[47]);
        auto raw=InputRaw(pool,{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,14,v[3]);Put(raw,18,v[4]);
        Put(raw,22,static_cast<std::uint32_t>(static_cast<int>(static_cast<double>(std::bit_cast<float>(v[3]))*16+.5))&0xffff,2);
        Put(raw,24,static_cast<std::uint32_t>(static_cast<int>(static_cast<double>(std::bit_cast<float>(v[4]))*11+.5))&0xffff,2);
        Put(raw,patch ? 36 : 34,v[2],patch ? 4 : 1);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(v[1]);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(v[13]);
        Put(raw,12,v[14] ? v[13] : v[48],2);Put(raw,26,v[33],patch ? 4 : 2);if (patch) { Put(raw,42,74);Put(raw,46,v[40]); }
        auto surface=InputRaw(pool,{8});surface[10]=static_cast<std::uint8_t>(v[27] ? 73 : 72);const std::vector<std::uint8_t> before(raw.begin(),raw.end());
        std::vector<SpriteStatusProcess> processes(40000);std::array<std::vector<std::uint32_t>,7> members{{{5},{6,5,5},{5},{5},{5},{6},{5}}};
        if (v[49]) {
            processes[0]={41,1,members[0],999};processes[1]={41,3,members[1],111.9};processes[2]={41,2,members[2],122.8};processes[3]={41,1,members[3],99};
            processes[4]={42,1,members[4],900};processes[5]={41,2,members[5],900};processes[39999]={41,1,members[6],v[49]==2 ? 120.6 : 104};
        }
        s.processes=processes;std::vector<std::string> events;SpriteDrawHooks hooks;
        // 필수 출력과 가상 호출을 같은 순서의 native DWORD 명령으로 기록한다.
        hooks.setOwnerColors=[&](auto t,auto owner) { events.push_back(SpriteEvent("M",t,owner)); };
        hooks.shape=[&](auto r) { events.push_back(SpriteEvent("S",r.type,r.frame,r.position.x,r.position.y,static_cast<int>(r.mode))); };
        hooks.line=[&](auto a,auto b,auto color) { events.push_back(SpriteEvent("L",a.x,a.y,b.x,b.y,color)); };
        hooks.fill=[&](auto rect,auto color) { events.push_back(SpriteEvent("R",rect.left,rect.top,rect.right,rect.bottom,color)); };
        hooks.virtualFrame=[&](Sid sid) { events.push_back(SpriteEvent("F",sid.value));return integer(22); };
        hooks.virtualDraw=[&](Sid sid,auto p) { events.push_back(SpriteEvent("V",sid.value,p.x,p.y)); };
        SquidHash surfaceHash;const int ix=static_cast<int>(static_cast<double>(std::bit_cast<float>(v[3]))+static_cast<double>(.9999F));
        const int iy=static_cast<int>(static_cast<double>(std::bit_cast<float>(v[4]))+static_cast<double>(.9999F));
        if (ix>=0 && ix<256 && iy>=0 && iy<256) surfaceHash.Cell(0,ix,iy)=static_cast<std::uint16_t>(v[26] ? 8 : 0);
        hooks.surface=MakeSpriteSurfaceLookup(surfaceHash);
        hooks.constructionActive=[&](Sid sid) { CHECK(sid.value==5);return v[38]!=0; };
        hooks.resource=[&](Sid sid)->std::optional<SpriteResourceStatus> { CHECK(sid.value==5);if (v[41]) return SpriteResourceStatus{74,integer(43)};return {}; };
        hooks.timers=[&](Sid sid,auto t)->std::optional<std::array<double,3>> { CHECK(sid.value==5);CHECK(t==40);if (v[45]) return std::array<double,3>{119.99,160.99,151.01};return {}; };
        SpriteDraw draw(pool,types,views,s,hooks,kSpriteRaw-5*pool.Layout().stride);
        draw.Draw({std::bit_cast<float>(v[3]),std::bit_cast<float>(v[4]),kSpriteRaw,0,0,static_cast<std::uint8_t>(v[0])});std::string observed;
        // 명령 사이 구분을 원본과 동일하게 합친다.
        for (const auto& event:events) { if (!observed.empty()) observed+=';';observed+=event; }
        if (observed.empty()) observed="-";
        const bool same=observed==row[3];CHECK(same);CHECK(s.selectedSid==std::stoul(row[4]));
        CHECK(0x17000000U+s.shadowShapeType.value_or(s.shadowAliasType)*0x1000U==std::stoul(row[5]));CHECK(std::equal(before.begin(),before.end(),raw.begin()));
        if (!same) { std::printf("SpriteDraw %s %s\nactual: %s\nnative: %s\n",edition.c_str(),row[1].c_str(),observed.c_str(),row[3].c_str());break; }++replayed;
    }
    CHECK(replayed==463);
}
// 출력과 계약을 검사하는 최소 실제 raw/SHP 상태다.
struct SpriteScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false};/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(71);
    // views: 타입에서 프레임·셰이프 블록을 찾는 조회 정보 목록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<SpriteTypeView> views=std::vector<SpriteTypeView>(71);/* s: Draw가 읽는 원본 전역을 외부에서 현재 값으로 공급한다. 선택 SID/그림자 alias만 쓰기 대상이다 (SpriteDrawState). */ SpriteDrawState s;
    // raw 좌표는 x*16/y*11+0.5를 절삭한 화면 WORD다.
    SpriteScene() {
        auto raw=InputRaw(pool,{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,14,std::bit_cast<std::uint32_t>(.25F));Put(raw,18,std::bit_cast<std::uint32_t>(.25F));Put(raw,22,4,2);Put(raw,24,3,2);
        views[70].shape={1,true,{{4,4,1,1}}};s.ownerMaps.resize(71);s.ownerPalettes={0};s.selectionColors={7};s.shadowAliasType=69;s.timerTypes={69,69,69};s.costType=69;
    }
};
// 투명·불투명 0/255를 포함한 실제 VFX RLE다.
std::vector<std::uint8_t> SpritePixelShape() {
    std::vector<std::uint8_t> bytes(40);Put(bytes,0,0x30312e31);Put(bytes,4,1);Put(bytes,8,16);Put(bytes,32,2);bytes.insert(bytes.end(),{1,1,5,0,255,0});return bytes;
}
}
// 10.78의 전체 본체/그림자/표식/선택/상태 막대 관찰을 재생한다.
TEST_CASE(SpriteDraw_PatchNative) { ReplaySpriteDraw("originals"); }
// CD의 BYTE 프레임/WORD HP와 독립 체력 막대 배치를 재생한다.
TEST_CASE(SpriteDraw_CdNative) { ReplaySpriteDraw("originalCD"); }
// 추가판의 별도 PE 관찰도 같은 호스트 구현에 대조한다.
TEST_CASE(SpriteDraw_AdditionalNative) { ReplaySpriteDraw("original1037"); }
// 실제 kind 0과 kind 1 묶음 모두 같은 SHP 픽셀 출력에 연결된다.
TEST_CASE(SpriteDraw_CommonGroupDispatch) {
    SpriteScene scene;ShapeDatabase shape(SpritePixelShape());std::vector<std::uint8_t> pixels(64,9);
    SpritePixelTarget target(pixels,8,8,{0,0,8,8},[&](auto t) { CHECK(t==70);return PreviewShapeBinding{&shape,0}; },{});
    SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,target.Hooks(),kSpriteRaw-250);DrawDispatchHooks hooks;BindSpriteDraw(hooks,draw);
    const RenderCollectionEntry entry{.25F,.25F,kSpriteRaw,0,0,0};
    DispatchDrawEntries(std::span(&entry,1),{},hooks);CHECK(pixels[28]==9);CHECK(pixels[29]==0);CHECK(pixels[30]==255);
    std::fill(pixels.begin(),pixels.end(),std::uint8_t{9});std::int32_t count=1;hooks.group=[&](auto) { return DrawGroupView{1,std::span(&entry,1),&count}; };
    const RenderCollectionEntry group{0,0,1,0,1,0};DispatchDrawEntries(std::span(&group,1),{},hooks);CHECK(pixels[29]==0);CHECK(pixels[30]==255);
}
// 현재 소유자 색·명시 그림자 색은 소스 색을 치환하며 투명/클립을 유지한다.
TEST_CASE(SpriteDraw_PixelModesTransparencyClip) {
    ShapeDatabase shape(SpritePixelShape());std::vector<std::uint8_t> pixels(32,9);
    SpritePixelTarget target(pixels,8,4,{1,1,5,3},[&](auto) { return PreviewShapeBinding{&shape,0}; },[](auto mode,auto,auto) {
        ColorMap map{};
        // 불투명 0/255도 표를 통과하게 한다.
        for (std::size_t i=0;i<map.size();++i) map[i]=static_cast<std::uint8_t>(i+static_cast<int>(mode)*10);
        return map;
    });auto hooks=target.Hooks();hooks.shape({70,0,{1,1},SpriteShapeMode::Plain});CHECK(pixels[9]==9);CHECK(pixels[10]==0);CHECK(pixels[11]==255);
    CHECK(Throws([&] { hooks.shape({70,0,{1,1},SpriteShapeMode::Owner}); }));hooks.setOwnerColors(70,2);hooks.shape({70,0,{1,1},SpriteShapeMode::Owner});CHECK(pixels[10]==10);CHECK(pixels[11]==9);
    hooks.shape({70,0,{1,1},SpriteShapeMode::Shadow});CHECK(pixels[10]==40);CHECK(pixels[11]==39);CHECK(pixels[9]==9);
    hooks.shape({70,0,{4,2},SpriteShapeMode::Dead});CHECK(pixels[21]==9);CHECK(Throws([&] { hooks.shape({70,1,{1,1},SpriteShapeMode::Plain}); }));
}
// 선택선의 끝점/역순/클립과 half-open fill의 빈/역전 범위를 검사한다.
TEST_CASE(SpriteDraw_PixelLinesAndBars) {
    std::vector<std::uint8_t> pixels(64,9);SpritePixelTarget target(pixels,8,8,{2,2,6,6},{},{});auto hooks=target.Hooks();
    hooks.line({7,3},{1,3},17);CHECK(pixels[26]==17);CHECK(pixels[29]==17);CHECK(pixels[25]==9);CHECK(pixels[30]==9);
    hooks.line({4,7},{4,1},18);CHECK(pixels[20]==18);CHECK(pixels[44]==18);CHECK(pixels[52]==9);
    hooks.fill({3,4,5,6},19);CHECK(pixels[35]==19);CHECK(pixels[44]==19);CHECK(pixels[45]==9);
    const auto before=pixels;hooks.fill({5,4,3,6},99);hooks.fill({3,5,5,5},99);CHECK(pixels==before);CHECK(Throws([&] { hooks.line({2,2},{3,3},4); }));
}
// null shape/좌표 불일치/빠진 콜백/잘못된 token을 예외로 명시한다.
TEST_CASE(SpriteDraw_ContractsAndEarlyReturns) {
    SpriteScene scene;SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,{},kSpriteRaw-250);RenderCollectionEntry entry{0,0,kSpriteRaw,0,0,0};
    scene.views[70].shape.loaded=false;draw.Draw(entry);scene.views[70].shape.loaded=true;CHECK(Throws([&] { draw.Draw(entry); }));
    entry.token++;CHECK(Throws([&] { draw.Draw(entry); }));entry.token=kSpriteRaw;
    auto raw=InputRaw(scene.pool,{5});Put(raw,22,5,2);CHECK(Throws([&] { draw.Draw(entry); }));Put(raw,22,4,2);
    entry.tail=4;scene.s.specialMode=1;draw.Draw(entry);CHECK(scene.s.shadowShapeType==70U);scene.s.shadowShapeType.reset();scene.s.shadowAliasPinned=true;draw.Draw(entry);CHECK(!scene.s.shadowShapeType);
    scene.s.specialMode=0;raw[40]=0x10;draw.Draw(entry);CHECK(!scene.s.shadowShapeType);
}
// 가상 Draw가 선택/상태 비트를 바꾸면 후속 표시가 현재 슬롯을 사용한다.
TEST_CASE(SpriteDraw_CurrentRawAfterVirtualDraw) {
    SpriteScene scene;auto raw=InputRaw(scene.pool,{5});raw[40]=2;int lines=0;SpriteDrawHooks hooks;
    hooks.virtualFrame=[](auto) { return -1; };hooks.virtualDraw=[&](auto,auto) { raw[40]=4; };
    hooks.line=[&](auto,auto,auto) { ++lines; };hooks.resource=[](auto)->std::optional<SpriteResourceStatus> { return {}; };
    SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,hooks,kSpriteRaw-250);draw.Draw({0,0,kSpriteRaw,0,0,0});CHECK(lines==8);CHECK(scene.s.selectedSid==5);
}
// 실제 raw 해시 수집→CRT 정렬→공통 분배→SHP 픽셀 출력을 한 흐름으로 검사한다.
TEST_CASE(SpriteDraw_ObjectCollectionPixels) {
    SpriteScene scene;SquidHash hash;hash.Cell(1,0,0)=5;scene.views[70].shape.frames[0].cellWidth=1;scene.views[70].shape.frames[0].cellHeight=1;
    std::vector<SquidDisplayShape> shapes(71);shapes[70]=scene.views[70].shape;ObjectCollection collector(hash,scene.pool,scene.types,shapes,kSpriteRaw-250);
    ShapeDatabase shape(SpritePixelShape());std::vector<std::uint8_t> pixels(64,9);SpritePixelTarget target(pixels,8,8,{0,0,8,8},[&](auto) { return PreviewShapeBinding{&shape,0}; },{});
    SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,target.Hooks(),kSpriteRaw-250);DrawCollectionHooks collection;BindObjectCollection(collection,collector);
    DrawDispatchHooks dispatch;BindSpriteDraw(dispatch,draw);std::array<RenderCollectionEntry,4> work{};
    const auto result=CollectDrawEntries({{0,0,8,8},{-1,-1,2,2},{-1,-1,2,2},0,4},work,collection);CHECK(result.count==1);CHECK(hash.Selected()==2);
    auto entries=std::span(work).first(static_cast<std::size_t>(result.count));SortDrawEntries(entries,scene.pool.Edition());DispatchDrawEntries(entries,{},dispatch);
    CHECK(pixels[29]==0);CHECK(pixels[30]==255);
}
// 표면 조회의 +0.9999/음수 절삭/경계와 현재 해시 단계의 보존을 검사한다.
TEST_CASE(SpriteDraw_SurfaceLookupBoundaries) {
    SquidHash hash(3);hash.Cell(0,0,0)=5;hash.Cell(0,1,1)=6;const auto lookup=MakeSpriteSurfaceLookup(hash);
    CHECK(lookup(0,0)==Sid{5});CHECK(lookup(.25F,.25F)==Sid{6});CHECK(lookup(-.25F,-.25F)==Sid{5});
    CHECK(lookup(-2,-2)==Sid{});CHECK(lookup(256,256)==Sid{});CHECK(hash.Selected()==3);
    CHECK(Throws([&] { lookup(std::numeric_limits<float>::infinity(),0); }));
}
// 그림자 alias는 이후 loaded/선택 헤더/색 출력에 반영되며 alias 타입의 논리 count는 보존한다.
TEST_CASE(SpriteDraw_ShadowAliasCurrentShape) {
    SpriteScene scene;scene.s.timerTypes={68,68,68};scene.s.costType=68;scene.views[69].shape.frameCount=1;std::vector<ScreenPoint> lines;std::vector<SpriteShapeRequest> drawn;
    SpriteDrawHooks hooks;hooks.shape=[&](auto r) { drawn.push_back(r); };hooks.line=[&](auto first,auto,auto) { lines.push_back(first); };
    hooks.constructionActive=[](auto) { return false; };hooks.resource=[](auto)->std::optional<SpriteResourceStatus> { return {}; };
    SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,hooks,kSpriteRaw-250);draw.Draw({0,0,kSpriteRaw,0,0,4});CHECK(scene.s.shadowShapeType==70U);
    auto alias=InputRaw(scene.pool,{6});const auto source=scene.pool.Slot({5});std::copy(source.begin(),source.end(),alias.begin());alias[10]=69;alias[40]=4;Put(alias,42,70);
    draw.Draw({0,0,kSpriteRaw+50,0,0,0});CHECK(drawn.size()==2);if (drawn.size()==2) CHECK(drawn[1].type==69);
    CHECK(lines.size()==8);if (!lines.empty()) CHECK(lines[0].x==3);CHECK(scene.s.selectedSid==6);
    scene.views[69].shape.frameCount=0;CHECK(Throws([&] { draw.Draw({0,0,kSpriteRaw+50,0,0,0}); }));
}
// 가상 frame 조회에서 바뀐 특수 표시 전역은 조회 직후의 필터 분기에 반영한다.
TEST_CASE(SpriteDraw_CurrentFilterAfterVirtualFrame) {
    SpriteScene scene;InputRaw(scene.pool,{5})[40]=2;scene.types[70].flags2=8;scene.views[70].gumpFrame=7;
    std::vector<SpriteShapeRequest> drawn;SpriteDrawHooks hooks;hooks.virtualFrame=[&](auto) { scene.s.specialMode=1;scene.s.filteredFlags=8;return -1; };
    hooks.shape=[&](auto r) { drawn.push_back(r); };
    SpriteDraw draw(scene.pool,scene.types,scene.views,scene.s,hooks,kSpriteRaw-250);draw.Draw({0,0,kSpriteRaw,0,0,0});
    CHECK(drawn.size()==1);if (drawn.size()==1) { CHECK(drawn[0].frame==7);CHECK(drawn[0].mode==SpriteShapeMode::Abstract); }
}
