// 미리보기 Draw의 원본 명령열과 실제 SHP 색·투명·체크 무늬 출력을 검사한다.
#include "ConstructionSupport.h"
#include "CursorPreviewBindingSupport.h"
#include "client/PreviewDraw.h"
#include "client/DrawSort.h"
#include <sstream>
#include <filesystem>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// native fixture의 raw 객체와 호스트 풀 SID 5를 연결하는 DWORD 기준이다.
constexpr std::uint32_t kPreviewDrawTokenBase=0x16000200-5*50;
// full fixture의 byte 입력을 복원한다.
std::vector<std::uint8_t> PreviewDrawBytes(const std::string& text) {
    std::vector<std::uint8_t> result;if (text=="-") return result;
    // 두 글자가 한 원본 바이트다.
    for (std::size_t i=0;i<text.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
// 원본의 DWORD 출력 인수와 같은 문자열을 만든다.
template<class... V> std::string PreviewEvent(const char* prefix,V... values) {
    std::ostringstream out;out<<prefix;((out<<','<<static_cast<std::uint32_t>(values)),...);return out.str();
}
// 세 판본의 전체 완성 입력과 저장된 native 명령열을 대조한다.
void ReplayPreviewDraw(const std::string& edition,bool bound=false) {
    static const auto rows=LoadFixture(NETSTORM_PREVIEWDRAW_FIXTURE).rows;
    SidPool pool(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000,false);int replayed=0;
    // 기대 분기나 좌표를 계산하지 않고 원본 관찰 행을 그대로 재생한다.
    for (const auto& row:rows) if (row[0]==edition) {
        const auto v=Inputs(row[2]);PreviewCollectionState c;PreviewDrawState s;
        c.parentGump=v[0];c.legal=v[1];c.localPlayer=v[8];s.selectedType=v[2];s.destroyType=v[3];s.noOutlineType=v[4];s.overlayTrigger=v[5];s.overlayType=v[6];s.labelType=v[7];
        s.cameraX=std::bit_cast<std::int32_t>(v[9]);s.cameraY=std::bit_cast<std::int32_t>(v[10]);s.gridOption=v[11];s.showLetters=v[12];s.spacing=std::bit_cast<std::int32_t>(v[13]);
        s.labelOffsetX=std::bit_cast<std::int32_t>(v[14]);s.labelOffsetY=std::bit_cast<std::int32_t>(v[15]);s.labelWidth=std::bit_cast<std::int32_t>(v[16]);s.labelHeight=std::bit_cast<std::int32_t>(v[17]);
        s.flags=v[18];s.messageOverride=v[19];s.blocked=v[20];s.powered=v[21];s.reachable=v[22];s.priceWidth=std::bit_cast<std::int32_t>(v[23]);s.priceIconOffset=std::bit_cast<std::int32_t>(v[24]);
        s.priceIconX=std::bit_cast<std::int32_t>(v[25]);s.priceIconY=std::bit_cast<std::int32_t>(v[26]);s.priceType=v[27];s.ownerMaps.resize(74);s.ownerMaps[70]=v[28]!=0;s.goodColor=231;s.badColor=189;
        auto grid=PreviewDrawBytes(row[4]);std::copy(grid.begin(),grid.end(),s.grid.begin());const auto labels=PreviewDrawBytes(row[3]);s.labels.assign(labels.begin(),labels.end());
        // 원본 signed char 프레임 표와 측정된 문구 크기 입력이다.
        for (int i=0;i<256;++i) s.labelFrames[i]=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(i))));
        // 다섯 문구의 원본 측정 입력이다.
        for (int i=0;i<5;++i) { s.messageWidths[i]=21+i*13;s.messageHeights[i]=9+i*7; }
        std::vector<RiftTypeRecord> types(74);auto& type=types[70];type.footX=std::bit_cast<std::int32_t>(v[29]);type.footY=std::bit_cast<std::int32_t>(v[30]);
        type.hotspotX=std::bit_cast<std::int32_t>(v[31]);type.hotspotY=std::bit_cast<std::int32_t>(v[32]);type.flags2=v[33];
        auto raw=InputRaw(pool,{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,22,v[36]&0xffff,2);Put(raw,24,v[37]&0xffff,2);Put(raw,14,v[38]);Put(raw,18,v[39]);
        Put(raw,edition=="originals" ? 36 : 34,v[35],edition=="originals" ? 4 : 1);const std::vector<std::uint8_t> before(raw.begin(),raw.end());
        std::vector<std::string> events;int current=0;
        // 색 문맥 호출까지 포함해 native 출력 순서를 기록한다.
        PreviewDrawHooks hooks{[&](auto r) { CHECK(r.type==70);CHECK(r.owner==v[8]);current=static_cast<int>(r.colors);events.push_back(PreviewEvent("M",current)); },
            [&](auto r) { const int mode=r.mode==PreviewShapeMode::Plain ? 0 : r.mode==PreviewShapeMode::Illegal ? 3 : current;events.push_back(PreviewEvent("S",r.type,r.frame,r.position.x,r.position.y,mode)); },
            [&](auto r,auto color,auto checker) { events.push_back(PreviewEvent("R",checker,r.left,r.top,r.right,r.bottom,color)); },
            [&](auto r) { events.push_back(PreviewEvent("T",r.index,r.position.x,r.position.y)); }};
        const auto base=0x16000200U-5*pool.Layout().stride;PreviewDraw draw(pool,types,c,s,hooks,base);
        netstorm::test::previewbinding::Scene actual;actual.Drawing(c,s);
        if (bound) {
            draw.BindCursor(actual.Source(pool));c.parentGump=0xffffffff;c.legal=!actual.placement.placementAllowed;c.localPlayer=0xffffffff;
            s.selectedType=0xffffffff;s.destroyType=0xffffffff;s.noOutlineType=0xffffffff;s.cameraX=-900;s.cameraY=-900;
            s.spacing=-900;s.labelOffsetX=-900;s.labelOffsetY=-900;s.labelWidth=-900;s.labelHeight=-900;
            s.priceWidth=-900;s.priceIconOffset=-900;s.priceIconX=-900;s.priceIconY=-900;s.flags=~actual.drop.costFlags;
            s.messageOverride=!actual.placement.sourceAllowed;s.blocked=!actual.placement.placementFlags;
            s.powered=!actual.placement.descriptionAllowed;s.reachable=!actual.placement.spaceAllowed;
            s.grid.fill(0x7f);s.labels="Wrong";s.messageWidths.fill(-900);s.messageHeights.fill(-900);
        }
        draw.Draw({999,-999,0x16000200,0,4,static_cast<std::uint8_t>(v[34])});
        std::string observed;
        // native의 명령 구분과 동일하게 합친다.
        for (const auto& event:events) { if (!observed.empty()) observed+=';';observed+=event; }
        if (observed.empty()) observed="-";const bool same=observed==row[5];CHECK(same);CHECK(std::equal(before.begin(),before.end(),raw.begin()));
        if (!same) { std::printf("PreviewDraw %s %s\nactual: %s\nnative: %s\n",edition.c_str(),row[1].c_str(),observed.c_str(),row[5].c_str());break; }++replayed;
    }
    CHECK(replayed==477);
}
// 실제 수집과 픽셀/계약 검사를 위한 작은 원본 상태다.
struct PreviewDrawScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false};/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(74);
    // c: 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태다 (PreviewCollectionState).
    PreviewCollectionState c;/* s: 준비한 실제 커서 전역과 격자/글자 표다. legal/parent/플레이어는 수집 상태를 공유한다 (PreviewDrawState). */ PreviewDrawState s;
    // SID 5의 raw 화면 WORD/프레임과 원본 타입 깊이/footprint를 준비한다.
    PreviewDrawScene() {
        c.legal=1;c.pieceCount=1;c.pieces={{5}};c.type=70;c.localPlayer=2;
        auto raw=InputRaw(pool,{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;Put(raw,22,4,2);Put(raw,24,4,2);
        types[70].footX=1;types[70].footY=1;types[70].zOrder=7;s.ownerMaps.resize(74);s.noOutlineType=70;s.flags=0x20;
    }
};
// 투명 한 칸/불투명 0·255를 포함한 실제 VFX 압축 입력이다.
std::vector<std::uint8_t> PreviewPixelShape() {
    std::vector<std::uint8_t> bytes(40);Put(bytes,0,0x30312e31);Put(bytes,4,1);Put(bytes,8,16);Put(bytes,32,2);
    bytes.insert(bytes.end(),{1,1,5,0,255,0});return bytes;
}
}
// 패치판 전체 Draw의 분기/현재 색 표/명령 순서를 독립 관찰과 대조한다.
TEST_CASE(PreviewDraw_PatchNative) { ReplayPreviewDraw("originals"); }
// 같은477개 원문 명령열을 실제 전역 참조로 재생하고 이전 중복 입력을 사용하지 않는지 확인한다.
TEST_CASE(PreviewDraw_PatchNativeSharedCursor) { ReplayPreviewDraw("originals",true); }
// CD의 BYTE 프레임·실제 스택 좌표도 독립 기계어 관찰과 대조한다.
TEST_CASE(PreviewDraw_CdNative) { ReplayPreviewDraw("originalCD"); }
// 추가판의 별도 PE 관찰을 재생한다.
TEST_CASE(PreviewDraw_AdditionalNative) { ReplayPreviewDraw("original1037"); }

// 실제 수집→CRT→kind 4 Draw→VFX 픽셀 출력까지 한 흐름으로 연결한다.
TEST_CASE(PreviewDraw_CommonCollectionDispatch) {
    // 기존 입력과 실제 공유 참조가 같은 공통 수집·정렬·분배·픽셀 경로를 통과한다.
    for (const bool bound:{false,true}) {
        PreviewDrawScene scene;scene.c.parentGump=1;std::vector<std::uint8_t> spots(65536),pixels(64,9);
        ShapeDatabase shape(PreviewPixelShape());PreviewPixelTarget target(pixels,8,8,{0,0,8,8},[&](auto) { return PreviewShapeBinding{&shape,0}; },{},{});
        PreviewCollection collector(scene.pool,scene.types,spots,scene.c,{[](auto) { return 16U; },{}},kPreviewDrawTokenBase);
        PreviewDraw draw(scene.pool,scene.types,scene.c,scene.s,target.Hooks(),kPreviewDrawTokenBase);
        netstorm::test::previewbinding::Scene actual;actual.Drawing(scene.c,scene.s);
        if (bound) {
            collector.BindCursor(actual.Source(scene.pool));draw.BindCursor(actual.Source(scene.pool));
            scene.c.pieceCount=0;scene.c.parentGump=0;scene.c.legal=0;
        }
        DrawCollectionHooks collection;BindPreviewCollection(collection,collector);DrawDispatchHooks dispatch;BindPreviewDraw(dispatch,draw);
        std::array<RenderCollectionEntry,8> work;const auto result=CollectDrawEntries({{0,0,8,8},{0,0,8,8},{0,0,8,8},0,8},work,collection);
        CHECK(result.count==2);CHECK(work[0].tail==0);CHECK(work[1].tail==2);SortDrawEntries(std::span(work).first(2),scene.pool.Edition());
        DispatchDrawEntries(std::span(work).first(2),{},dispatch);CHECK(pixels[4*8+4]==9);CHECK(pixels[4*8+5]==0);CHECK(pixels[4*8+6]==255);
    }
}
// 실제 압축 해제의 투명/원색/현재 색/명시 불법 표와 클립·캐시 재사용을 검사한다.
TEST_CASE(PreviewDraw_PixelsColorsTransparencyClip) {
    ShapeDatabase shape(PreviewPixelShape());std::vector<std::uint8_t> pixels(32,9);int colorCalls=0;
    // 원색 0/255를 서로 다른 색으로 바꿔 경계 구분을 확인한다.
    const auto color=[&](PreviewColorRequest request) { ++colorCalls;ColorMap map{};
        // 치환하지 않는 색은 항등 값으로 유지한다.
        for (int i=0;i<256;++i) map[i]=static_cast<std::uint8_t>(i);
        map[0]=request.colors==PreviewColors::Illegal ? 7 : 20;map[255]=42;return map; };
    PreviewPixelTarget target(pixels,8,4,{1,1,5,3},[&](auto) { return PreviewShapeBinding{&shape,0}; },color,{});auto hooks=target.Hooks();
    hooks.shape({70,0,{1,1},PreviewShapeMode::Plain});CHECK(pixels[9]==9);CHECK(pixels[10]==0);CHECK(pixels[11]==255);
    hooks.setColors({PreviewColors::Owner,70,2});hooks.shape({70,0,{1,1},PreviewShapeMode::CurrentColors});CHECK(pixels[10]==20);CHECK(pixels[11]==42);
    hooks.shape({70,0,{1,2},PreviewShapeMode::Illegal});CHECK(pixels[18]==7);CHECK(pixels[19]==42);
    hooks.shape({70,0,{4,2},PreviewShapeMode::Plain});CHECK(pixels[21]==9);CHECK(colorCalls==2);
    CHECK(Throws([&] { hooks.shape({70,1,{1,1},PreviewShapeMode::Plain}); }));
}
// 체크 무늬는 잘린 위치에서도 절대 좌표 위상을 유지하며 윤곽은 잘린 면에 새 선을 만들지 않는다.
TEST_CASE(PreviewDraw_PixelsCheckerAndOutline) {
    std::vector<std::uint8_t> pixels(64,9);PreviewPixelTarget target(pixels,8,8,{2,2,6,6},{},{},{});auto hooks=target.Hooks();
    hooks.rectangle({1,1,7,7},189,true);
    // clip 안의 모든 픽셀에서 native (~y^x)&1의 행별 위상을 확인한다.
    for (int y=0;y<8;++y) {
        // 각 열은 화면 전체에서 일정한 체크 무늬 위상을 사용한다.
        for (int x=0;x<8;++x) CHECK(pixels[y*8+x]==(x>=2 && x<6 && y>=2 && y<6 && ((x^y)&1) ? 189 : 9));
    }
    const auto before=pixels;hooks.rectangle({1,1,7,7},231,false);CHECK(pixels==before);
    hooks.rectangle({2,2,5,5},231,false);CHECK(pixels[18]==231);CHECK(pixels[36]==231);CHECK(pixels[27]==before[27]);
    hooks.rectangle({4,4,4,4},42,false);CHECK(pixels[27]==42);CHECK(pixels[28]==42);CHECK(pixels[35]==42);CHECK(pixels[36]==42);
}
// raw 토큰/테이블/격자/표면/그림/필수 경계 부족은 기록된 분기에 도달할 때 거부한다.
TEST_CASE(PreviewDraw_Contracts) {
    PreviewDrawScene scene;PreviewDraw draw(scene.pool,scene.types,scene.c,scene.s,{},kPreviewDrawTokenBase);
    CHECK(Throws([&] { draw.Draw({0,0,0,0,4,0}); }));CHECK(Throws([&] { draw.Draw({0,0,kPreviewDrawTokenBase+1,0,4,0}); }));
    CHECK(Throws([&] { draw.Draw({0,0,0x16000200,0,4,0}); }));scene.c.parentGump=1;draw.Draw({0,0,0x16000200,0,4,2});
    scene.c.parentGump=0;scene.c.legal=0;scene.s.noOutlineType=90;scene.s.gridOption=1;scene.types[70].footX=11;
    CHECK(Throws([&] { draw.Draw({0,0,0x16000200,0,4,2}); }));
    // 출력 경계가 현재 footprint를 늘려도 다음 표 조회에서 배열 밖을 거부한다.
    scene.types[70].footX=1;scene.s.grid.fill(1);PreviewDrawHooks growing;
    growing.rectangle=[&](auto,auto,auto) { scene.types[70].footX=11; };
    PreviewDraw guarded(scene.pool,scene.types,scene.c,scene.s,growing,kPreviewDrawTokenBase);
    CHECK(Throws([&] { guarded.Draw({0,0,0x16000200,0,4,2}); }));
    std::vector<std::uint8_t> pixels(16,0);CHECK(Throws([&] { PreviewPixelTarget target(pixels,8,8,{0,0,8,8},{},{},{}); }));
    PreviewPixelTarget target(pixels,4,4,{0,0,4,4},{},{},{});auto hooks=target.Hooks();CHECK(Throws([&] { hooks.setColors({PreviewColors::Owner,70,2}); }));
    CHECK(Throws([&] { hooks.shape({70,0,{0,0},PreviewShapeMode::Plain}); }));
}
// 원본의 뒤쪽 분기가 출력 도중 바뀐 현재 parent/표시 플래그를 읽는 host 계약을 검사한다.
TEST_CASE(PreviewDraw_CurrentStateAfterGrid) {
    PreviewDrawScene scene;scene.c.legal=0;scene.s.noOutlineType=90;scene.s.gridOption=1;scene.s.grid.fill(1);scene.s.flags=0x40;int rectangles=0,texts=0,shapes=0;
    PreviewDrawHooks hooks;hooks.rectangle=[&](auto,auto,auto checker) { CHECK(checker);++rectangles;scene.c.parentGump=1; };
    hooks.text=[&](auto) { ++texts; };hooks.shape=[&](auto) { ++shapes; };PreviewDraw draw(scene.pool,scene.types,scene.c,scene.s,hooks,kPreviewDrawTokenBase);
    draw.Draw({0,0,0x16000200,0,4,2});CHECK(rectangles==9);CHECK(texts==0);CHECK(shapes==0);
}

// 실제 격자·부모 변경과 본체 출력 중 허용·플레이어 변경을 현재 참조에서 다시 읽는다.
TEST_CASE(PreviewDraw_SharedCursorCurrentReadsAndContracts) {
    PreviewDrawScene scene;scene.c.legal=0;scene.s.noOutlineType=90;scene.s.gridOption=1;scene.s.flags=0x40;
    scene.s.grid.fill(1);netstorm::test::previewbinding::Scene actual;actual.Drawing(scene.c,scene.s);
    int rectangles=0,texts=0,shapes=0;PreviewDrawHooks hooks;
    hooks.rectangle=[&](auto,auto,auto checker) {
        CHECK(checker);++rectangles;actual.lifecycle.currentGump=1;actual.pipeline.preview.blocked.fill(0);
    };
    hooks.text=[&](auto) { ++texts; };hooks.shape=[&](auto) { ++shapes; };
    PreviewDraw draw(scene.pool,scene.types,scene.c,scene.s,hooks,kPreviewDrawTokenBase);draw.BindCursor(actual.Source(scene.pool));
    draw.Draw({0,0,0x16000200,0,4,2});CHECK(rectangles==1 && texts==0 && shapes==0);
    actual.lifecycle.currentGump=0;actual.placement.placementAllowed=1;actual.drop.legal=0;scene.s.ownerMaps[70]=true;
    hooks.rectangle=[&](auto,auto,auto checker) { CHECK(!checker);actual.placement.placementAllowed=0;actual.drop.type=actual.surfaces.bridgeType;actual.drop.localPlayer=3; };
    hooks.setColors=[&](auto request) { CHECK(request.colors==PreviewColors::Destroy && request.owner==3);actual.drop.localPlayer=4; };
    hooks.shape=[&](auto request) { ++shapes;CHECK(request.mode==PreviewShapeMode::CurrentColors && actual.drop.localPlayer==4); };
    PreviewDraw changing(scene.pool,scene.types,scene.c,scene.s,hooks,kPreviewDrawTokenBase);changing.BindCursor(actual.Source(scene.pool));
    changing.Draw({0,0,0x16000200,0,4,0});CHECK(shapes==1);
    PreviewDrawScene other;CHECK(Throws([&] { changing.BindCursor(actual.Source(other.pool)); }));
    SidPool cd(OriginalEdition::Cd1072,24000,false);PreviewDraw cdDraw(cd,scene.types,scene.c,scene.s,hooks);
    CHECK(Throws([&] { cdDraw.BindCursor(actual.Source(cd)); }));
}

// 실제 Take 자산 어댑터가 준 현재 비용 SHP를 사용하며 저장소 변경 뒤 낡은 바인딩을 재사용하지 않는다.
TEST_CASE(PreviewDraw_CurrentCursorProtectedAssetBinding) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");GameAssets assets(files,OriginalEdition::Patch1078);
    FontStore fonts(files,"Arial");GumpText text(fonts);CursorLifecycleHooks hooks;BindCursorLifecycleAssets(hooks,assets,text);
    CursorLifecycleState lifecycle;lifecycle.costImage=hooks.costImage(92,3).token;
    const auto binding=MakeCursorPreviewAssetShapes(assets,0xfffffffe,lifecycle);
    CHECK(binding(0xfffffffe).database==&assets.Shapes() && binding(0xfffffffe).block==assets.Types()[92-70].block);
    CHECK(binding(123).block==assets.Types()[123-70].block);
    lifecycle.costImage=hooks.defaultShape(123);CHECK(binding(0xfffffffe).block==assets.Types()[123-70].block);
    lifecycle.costImage=0;CHECK(Throws([&] { binding(0xfffffffe); }));
    lifecycle.costImage=static_cast<std::uint32_t>(assets.Shapes().Blocks().size()+1);CHECK(Throws([&] { binding(0xfffffffe); }));
}
