// 실제 PE 전체 배경 출력/픽셀 관찰과 독립 SHP 입력으로 자산 공급·클립·색·분배를 검사한다.
#include "client/GumpBackgroundDraw.h"
#include "GumpBackgroundTestAssets.h"
#include <sstream>
#include <cstdio>

namespace {
using namespace netstorm::test::gumpbackground;
// 원본/독립 검사 표면의 작은 실제 행 간격과 높이다.
constexpr int kPitch=32,kHeight=24;
// signed 좌표도 원본 DWORD 인자와 같은 바이트로 직렬화한다. 사건 기대값은 계산하지 않는다.
std::string Event(char kind,std::initializer_list<std::uint32_t> values) {
    std::string result(1,kind);
    // PE가 기록한 것과 같은 인자 순서를 유지한다.
    for (const auto value:values) result+=','+std::to_string(value);
    return result;
}
// 위치/측정/배경의 기본·파생 노드가 실제 같은 저장소를 사용하는 작은 장면이다.
struct BackgroundScene {
    // create: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState create;
    // root: 같은 객체의 표시 필드를 상속하므로 수집/Draw의 캐시 쓰기가 생성 저장소에 바로 반영된다 (GumpCreateNode).
    GumpCreateNode root;
    // node: +a0/+a4/+a8/+ac/+b0/+b4의 SHP·타일 크기·타입·프레임·배경 플래그다 (GumpBackgroundNode).
    GumpBackgroundNode node;
    // factory: 타입에 맞는 객체를 생성하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpCreate factory;
    // collection: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    GumpCollection collection;
    // 생성과 표시 조회의 같은 주소를 반환한다.
    GumpCreateNode* Find(std::uint32_t token) { return token==1 ? &root : token==2 ? &node : nullptr; }
    // 배경 생성자/수집/출력은 같은 안정된 노드를 참조한다. 원본 루트 자동 초기화도 사용한다.
    explicit BackgroundScene(netstorm::o::OriginalEdition edition=netstorm::o::OriginalEdition::Patch1078)
        :factory(create,{[this](auto token) { return Find(token); },[](auto) { return 1U; }}),
        collection(create.collection,factory.CollectionHooks()) {
        create.collection.edition=edition;create.screenWidth=32;create.screenHeight=24;factory.InitializeRoot();
    }
};
}

// 전체 가상/직접 분배와 실제 PE 소프트웨어 fill 768바이트, 모든 VFX/클립/선 사건을 전수 비교한다.
TEST_CASE(GumpBackgroundDraw_AllThreeNativeBodiesAndSoftwarePixelsMatch) {
    std::ifstream input(NETSTORM_GUMPBACKGROUNDDRAW_FIXTURE);CHECK(input.good());std::string line;
    std::array<std::size_t,3> counts{};
    // 저장한 독립 원본 결과만 기대값으로 읽으며 실제 게임/원본/Python은 필요 없다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');CHECK(row.size()==20);if (row.size()!=20) continue;
        const auto edition=row[0]=="originals" ? 0U : row[0]=="originalCD" ? 1U : 2U;
        BackgroundScene scene(edition ? netstorm::o::OriginalEdition::Cd1072 : netstorm::o::OriginalEdition::Patch1078);
        scene.node.x=static_cast<float>(std::stoi(row[2]));scene.node.y=static_cast<float>(std::stoi(row[3]));
        scene.node.width=Float(Number(row[4]));scene.node.height=Float(Number(row[5]));
        scene.node.backgroundFlags=Number(row[6]);scene.node.type=Number(row[7]);scene.node.frame=Number(row[8]);
        scene.node.shape=0xdeadbeef;scene.node.tileWidth=123;scene.node.tileHeight=456;
        const auto clip=Split(row[11],',');GumpDrawState state;SetGumpDrawClip(state,{std::stoi(clip[0]),std::stoi(clip[1]),std::stoi(clip[2]),std::stoi(clip[3])});
        GumpBackgroundDrawState globals{Number(row[12])!=0,Number(row[13]),Number(row[14]),Number(row[15])};
        std::vector<std::uint8_t> guarded(kPitch*kHeight+2,77);auto pixels=std::span(guarded).subspan(1,kPitch*kHeight);
        // 실제 PE에 공급한 픽셀 초기값을 그대로 입력한다.
        for (std::size_t i=0;i<pixels.size();++i) pixels[i]=static_cast<std::uint8_t>(i*19+37);
        GumpPixelTarget target(pixels,kPitch,kHeight,state);std::vector<std::string> events;unsigned current=0;
        GumpBackgroundDrawHooks hooks;hooks.node=[&](auto token) { return token==2 ? &scene.node : nullptr; };
        hooks.frame=[&](auto type,auto frame) {
            events.push_back(Event('T',{type,frame}));return GumpBackgroundFrame{static_cast<std::int16_t>(std::stoi(row[9])),static_cast<std::int16_t>(std::stoi(row[10]))};
        };
        hooks.clip=[&](auto clipRect) {
            events.push_back(Event('C',{std::uint32_t(clipRect.left),std::uint32_t(clipRect.top),std::uint32_t(clipRect.right),std::uint32_t(clipRect.bottom)}));SetGumpDrawClip(state,clipRect);
        };
        hooks.shade=[&](auto shade) { current=shade;events.push_back(Event('M',{shade})); };
        hooks.shape=[&](auto request) { events.push_back(Event('D',{request.type,request.frame,std::uint32_t(request.position.x),std::uint32_t(request.position.y),request.mapped ? current : 0U})); };
        // 원본 fill 몸체가 쓴 실제 픽셀과 같은 C++ 출력 결과를 기록한다. 선/VFX 그림은 이 fixture의 명시 경계다.
        hooks.fill=[&](auto rect,auto color,auto checker) {
            events.push_back(Event('F',{std::uint32_t(rect.left),std::uint32_t(rect.top),std::uint32_t(rect.right),std::uint32_t(rect.bottom),color,checker ? 1U : 0U}));target.Fill(rect,color,checker);
        };
        hooks.line=[&](auto segment) { events.push_back(Event('L',{std::uint32_t(segment.first.x),std::uint32_t(segment.first.y),std::uint32_t(segment.last.x),std::uint32_t(segment.last.y),segment.color})); };
        GumpBackgroundDraw draw(scene.collection,state,globals,hooks);
        if (row[1]=="Virtual") draw.Draw(2);
        else {
            const auto right=static_cast<int>(double(scene.node.x)+scene.node.width),bottom=static_cast<int>(double(scene.node.y)+scene.node.height);
            draw.Background({std::stoi(row[2]),std::stoi(row[3]),right,bottom},scene.node.backgroundFlags,scene.node.type,scene.node.frame);
        }
        std::string actual;
        // 사건의 순서/전체 인자까지 실제 원본과 비교한다.
        for (const auto& event:events) { if (!actual.empty()) actual+=';';actual+=event; }
        if (actual.empty()) actual="-";
        CHECK(actual==row[16]);CHECK(globals.debugColor==Number(row[17]));CHECK(current==Number(row[18]));
        CHECK(Hex(pixels)==row[19]);CHECK(guarded.front()==77 && guarded.back()==77);
        CHECK(state.clip.left==std::stoi(clip[0]) && state.clip.top==std::stoi(clip[1]) && state.clip.right==std::stoi(clip[2]) && state.clip.bottom==std::stoi(clip[3]));
        ++counts[edition];
    }
    CHECK((counts==std::array<std::size_t,3>{209,209,209}));
}

// 실제 생성 경계가 같은 안정된 타입/프레임 표를 사용하고 type=0의 미작성 필드를 유지하는지 확인한다.
TEST_CASE(GumpBackgroundDraw_AssetHeadersBindBothEditionConstructors) {
    const auto shapes=Shapes();const std::array<GumpBackgroundAssetBinding,2> bindings{{{82,0,1,0},{83,0,1,0x440000}}};
    GumpBackgroundAssets assets(shapes,bindings);
    // 패치/CD의 원본 생성 순서는 기존 생성자에 맡기고 실제 자산 공급만 연결한다.
    for (auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        BackgroundScene scene(edition);std::int8_t color=-7;GumpBackgroundCreateHooks create;
        create.node=[&](auto token) { return token==2 ? &scene.node : nullptr; };GumpBackgroundDrawHooks draw;
        BindGumpBackgroundAssets(create,draw,assets);GumpBackgroundCreate factory(scene.create,scene.factory,color,create);
        factory.Construct(2,13,9,GumpBackgroundFlags::kButton,1,83,1);
        CHECK(scene.node.shape==83 && scene.node.tileWidth==5 && scene.node.tileHeight==4 && scene.node.command==0xfffffff9U);
        CHECK(scene.node.width==13 && scene.node.height==9 && scene.create.count==2);
        const auto* stable=create.type(83);const auto* header=create.frame(stable->token,0,1);CHECK(header->width==5 && header->height==4);
        CHECK(create.type(82)!=stable && create.type(83)==stable && create.frame(83,0,1)==header);
        factory.Construct(2,11,8,0,0x100000,0,0);
        CHECK(scene.node.shape==83 && scene.node.tileWidth==5 && scene.node.tileHeight==4 && scene.node.type==0);
    }
}

// 논리 수/추가 레이어/물리 표를 구분하며 signed 프레임과 잘못된 바인딩을 거부한다.
TEST_CASE(GumpBackgroundDraw_AssetFrameContractsKeepLogicalAndPhysicalLayersSeparate) {
    const auto shapes=Shapes();const std::array<GumpBackgroundAssetBinding,2> bindings{{{82,0,1,0},{83,0,1,0x440000}}};GumpBackgroundAssets assets(shapes,bindings);
    CHECK(assets.Frame(82,0,0).width==3 && assets.Frame(83,0,1).width==5);
    CHECK(Throws([&] { assets.Frame(82,0,1); }));CHECK(Throws([&] { assets.Frame(83,0,2); }));
    CHECK(Throws([&] { assets.Frame(83,0,0xffffffff); }));CHECK(Throws([&] { assets.Frame(83,1,0); }));
    CHECK(Throws([&] { assets.Type(0); }));CHECK(Throws([&] { assets.Type(1000); }));
    const std::array<GumpBackgroundAssetBinding,2> duplicate{{{82,0,1,0},{82,0,1,0}}};
    CHECK(Throws([&] { GumpBackgroundAssets bad(shapes,duplicate); }));
    // 구조체의 각 잘못된 입력을 독립 생성에서 확인한다.
    for (const auto binding:{GumpBackgroundAssetBinding{0,0,1,0},{82,1,1,0},{82,0,0,0},{82,0,3,0}})
        CHECK(Throws([&] { GumpBackgroundAssets bad(shapes,std::span(&binding,1)); }));
}

// 실제 VFX/메타데이터로 정상·눌림·레이어 변경/투명/현재 색 표/클립 복구를 손계산 픽셀에 대조한다.
TEST_CASE(GumpBackgroundDraw_PixelsUseFreshAssetsAndPersistentShadeMaps) {
    const auto shapes=Shapes();const std::array<GumpBackgroundAssetBinding,2> bindings{{{82,0,1,0},{83,0,1,0x440000}}};GumpBackgroundAssets assets(shapes,bindings);
    BackgroundScene scene;scene.node.x=1;scene.node.y=1;scene.node.width=6;scene.node.height=6;
    scene.node.type=82;scene.node.backgroundFlags=GumpBackgroundFlags::kButton;scene.node.shape=999;scene.node.tileWidth=99;scene.node.tileHeight=99;
    GumpDrawState state;SetGumpDrawClip(state,{0,0,8,8});GumpBackgroundDrawState globals;
    const auto maps=Maps();ColorMap colors;colors.fill(77);std::vector<std::uint8_t> guarded(66,9);auto pixels=std::span(guarded).subspan(1,64);
    GumpPixelTarget target(pixels,8,8,state);GumpBackgroundPixelTarget raster(target,assets,maps,colors);
    GumpBackgroundCreateHooks create;GumpBackgroundDrawHooks hooks;hooks.node=[&](auto token) { return token==2 ? &scene.node : nullptr; };
    BindGumpBackgroundAssets(create,hooks,assets);GumpBackgroundDraw draw(scene.collection,state,globals,raster.Hooks(state,hooks));
    draw.Draw(2);CHECK(pixels[3*8+3]==4 && pixels[1*8+3]==14 && pixels[6*8+3]==22);
    CHECK(colors==maps.dark && guarded.front()==9 && guarded.back()==9 && state.clip.right==8 && state.vfxClip.right==7);
    std::fill(pixels.begin(),pixels.end(),std::uint8_t{9});scene.node.backgroundFlags|=GumpBackgroundFlags::kPressed;draw.Draw(2);
    CHECK(pixels[1*8+3]==24 && pixels[6*8+3]==12 && colors==maps.bright);
    // 이전 캐시 크기/shape가 틀려도 현재 type/frame의 반복 주기 4×3과 투명 중앙을 사용한다.
    std::fill(pixels.begin(),pixels.end(),std::uint8_t{9});scene.node.type=83;scene.node.frame=1;scene.node.backgroundFlags=GumpBackgroundFlags::kTexture;
    draw.Draw(2);CHECK(pixels[1*8+1]==9 && pixels[3*8+4]==5 && colors==maps.bright);
    CHECK(state.clip.left==0 && state.clip.right==8 && scene.node.shape==999 && scene.node.tileWidth==99);
}

// flat 경로의 우선순위/낮은 색 BYTE와 현재 1픽셀 선 전역, 디버그 색 증가를 실제 표면에서 검사한다.
TEST_CASE(GumpBackgroundDraw_FlatCheckerDebugAndCurrentPaletteColors) {
    BackgroundScene scene;scene.node.x=1;scene.node.y=1;scene.node.width=6;scene.node.height=6;scene.node.backgroundFlags=0x0fe40091;
    GumpDrawState state;SetGumpDrawClip(state,{0,0,8,8});std::vector<std::uint8_t> pixels(64,9);GumpPixelTarget target(pixels,8,8,state);
    GumpBackgroundDrawState globals{false,0xffffffff,77,33};GumpBackgroundDrawHooks hooks;hooks.node=[&](auto) { return &scene.node; };
    hooks.frame=[](auto,auto) -> GumpBackgroundFrame { throw std::runtime_error("flat 경로의 자산 조회 금지"); };
    hooks.fill=[&](auto rect,auto color,auto checker) { target.Fill(rect,color,checker); };
    hooks.line=[&](auto line) { target.Lines(std::span(&line,1)); };
    GumpBackgroundDraw draw(scene.collection,state,globals,hooks);draw.Draw(2);
    CHECK(pixels[1*8+3]==77 && pixels[6*8+3]==33 && pixels[3*8+3]==9 && pixels[3*8+4]==145);
    scene.node.backgroundFlags=GumpBackgroundFlags::kSolid|42;std::fill(pixels.begin(),pixels.end(),std::uint8_t{9});draw.Draw(2);
    CHECK(pixels[3*8+3]==42 && pixels[0]==9);globals.debug=true;draw.Draw(2);
    CHECK(globals.debugColor==0 && pixels[3*8+4]==255 && pixels[3*8+3]==42);
    PaletteColorTable palette;palette.basic[7]=19;palette.basic[8]=21;SetGumpBackgroundColors(globals,palette);
    CHECK(globals.nearColor==19 && globals.farColor==21 && globals.debug && globals.debugColor==0);
}

// 외부 조회가 바꾼 현재 클립을 저장하며 출력 실패 때 복구하고 앞선 픽셀/색 변경은 보존한다.
TEST_CASE(GumpBackgroundDraw_CurrentCallbacksAndTextureFailureRestoreClip) {
    BackgroundScene scene;GumpDrawState state;SetGumpDrawClip(state,{0,0,8,8});GumpBackgroundDrawState globals{false,0,11,13};
    std::vector<std::uint32_t> colors;GumpBackgroundDrawHooks hooks;hooks.node=[&](auto) { return &scene.node; };
    hooks.line=[&](auto line) { colors.push_back(line.color);globals.nearColor=17;globals.farColor=19; };
    hooks.frame=[&](auto,auto) { SetGumpDrawClip(state,{2,2,6,6});return GumpBackgroundFrame{3,3}; };
    hooks.clip=[&](auto clip) { SetGumpDrawClip(state,clip); };
    unsigned mapped=0,draws=0;hooks.shade=[&](auto shade) { mapped=shade; };
    hooks.shape=[&](auto) { if (++draws==2) throw std::runtime_error("배경 픽셀 출력 실패"); };
    GumpBackgroundDraw draw(scene.collection,state,globals,hooks);
    draw.Background({1,1,7,7},0x01e00000,0,0);CHECK((colors==std::vector<std::uint32_t>{11,17,19,19}));
    CHECK(Throws([&] { draw.Background({1,1,7,7},GumpBackgroundFlags::kTexture|GumpBackgroundFlags::kDark,82,0); }));
    CHECK(draws==2 && mapped==2 && state.clip.left==2 && state.clip.right==6 && state.vfxClip.right==5);
    // 디버그 증가가 먼저 발생하므로 fill 경계의 현재 전역 조회에서도 새 값을 볼 수 있다.
    globals.debug=true;globals.debugColor=7;hooks.fill=[&](auto,auto color,auto checker) { CHECK(color==7 && checker && globals.debugColor==8); };
    GumpBackgroundDraw debug(scene.collection,state,globals,hooks);debug.Draw(2);CHECK(globals.debugColor==8);
}

// invalid 타일 조기 반환/필수 경계·분배 fallback 계약을 확인한다.
TEST_CASE(GumpBackgroundDraw_ContractsAndVirtualDispatchPreserveFallback) {
    BackgroundScene scene;GumpDrawState state;SetGumpDrawClip(state,{0,0,8,8});GumpBackgroundDrawState globals;
    GumpBackgroundDrawHooks hooks;hooks.node=[&](auto token) { return token==2 ? &scene.node : nullptr; };
    hooks.frame=[](auto,auto) { return GumpBackgroundFrame{1,3}; };GumpBackgroundDraw draw(scene.collection,state,globals,hooks);
    draw.Background({1,1,7,7},GumpBackgroundFlags::kButton,82,0);CHECK(state.clip.right==8);
    CHECK(Throws([&] { draw.Background({1,1,7,7},GumpBackgroundFlags::kSolid,0,0); }));
    CHECK(Throws([&] { draw.Draw(99); }));CHECK(Throws([&] { GumpBackgroundDraw missing(scene.collection,state,globals,{}); }));
    std::vector<std::uint32_t> fallback;GumpDrawHooks dispatch{[&](auto token) { fallback.push_back(token); }};
    BindGumpBackgroundDraw(dispatch,draw,[](auto token) { return token==2; });dispatch.draw(1);dispatch.draw(2);
    CHECK((fallback==std::vector<std::uint32_t>{1}));CHECK(Throws([&] { BindGumpBackgroundDraw(dispatch,draw,{}); }));
}

namespace netstorm::test {
// 선택 검사에서 실제 두 판본의 모든 논리 프레임 헤더와 raw 배경 네 종류의 픽셀을 JSON으로 내보낸다.
// 파일은 읽기 전용이며 기본 CTest에서 호출하지 않는다. Python은 원본 PE/SHP/팔레트로 독립 대조한다.
void InspectGumpBackgroundAssets(const std::filesystem::path& root,bool cd) {
    using namespace netstorm::client;
    using namespace netstorm::test::gumpraw;
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");const auto edition=cd ? o::OriginalEdition::Cd1072 : o::OriginalEdition::Patch1078;
    GameAssets game(files,edition);GumpBackgroundAssets assets(game);const auto& fort=game.Find("fortGump");
    const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+fort.block);
    const auto frame=static_cast<std::uint32_t>(game.FrameIndex(fort,"A00",0));const auto metrics=assets.Frame(type,0,frame);
    std::array<std::uint32_t,256> logical{};
    // 원본 Screen 파일 팔레트처럼 0/255번을 검정/흰색으로 고정하고 나머지 RGB를 그대로 사용한다.
    for (std::size_t i=0;i<logical.size();++i) {
        const auto color=game.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16);
    }
    logical[0]=0;logical[255]=0xffffff;const auto maps=BuildGumpShadeMaps(logical);GumpBackgroundDrawState globals;
    SetGumpBackgroundColors(globals,BuildPaletteColorTable(logical,edition));
    BackgroundScene scene(edition);scene.node.x=3;scene.node.y=2;scene.node.width=26;scene.node.height=19;scene.node.type=type;scene.node.frame=frame;
    GumpDrawState state;SetGumpDrawClip(state,{0,0,kPitch,kHeight});std::vector<std::uint8_t> pixels(kPitch*kHeight);
    GumpPixelTarget target(pixels,kPitch,kHeight,state);ColorMap colors{};GumpBackgroundPixelTarget raster(target,assets,maps,colors);
    GumpBackgroundCreateHooks create;GumpBackgroundDrawHooks hooks;hooks.node=[&](auto token) { return token==2 ? &scene.node : nullptr; };
    BindGumpBackgroundAssets(create,hooks,assets);GumpBackgroundDraw draw(scene.collection,state,globals,raster.Hooks(state,hooks));
    std::printf("{\"types\":%zu,\"fortType\":%u,\"frame\":%u,\"tileWidth\":%d,\"tileHeight\":%d,\"headers\":[",game.Types().size(),type,frame,metrics.width,metrics.height);
    std::size_t headers=0;
    // 실제 자산 타입의 모든 논리 프레임 헤더를 번호 순서로 제공한다. 추상 레이어를 기본 프레임 수에 섞지 않는다.
    for (const auto& asset:game.Types()) {
        const auto number=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+asset.block);
        // 원본 클러스터 수와 같은 논리 프레임의 signed 메타 폭/높이를 내보낸다.
        for (std::size_t index=0;index<asset.definition.clusters.size();++index) {
            const auto header=assets.Frame(number,0,static_cast<std::uint32_t>(index));
            std::printf("%s[%u,%zu,%d,%d,%zu]",headers++ ? "," : "",number,index,header.width,header.height,asset.block);
        }
    }
    std::printf("],\"samples\":[");
    // 질감 정상/눌림·단색·체크 우선순위의 네 실제 출력 표면이다. 출력 크기는 32×24다.
    constexpr std::array<std::uint32_t,4> flags{0x03e00000,0x03e40000,0x05e0002a,0x0fe00091};
    // 매 샘플은 같은 초기 픽셀에서 시작하며 같은 현재 GumpDraw/색 상태를 사용한다.
    for (std::size_t index=0;index<flags.size();++index) {
        // 원본 실행기와 같은 초기 픽셀을 명시 입력으로 공급한다.
        for (std::size_t i=0;i<pixels.size();++i) pixels[i]=static_cast<std::uint8_t>(i*19+37);
        colors.fill(0);scene.node.backgroundFlags=flags[index];draw.Draw(2);
        const auto hex=Hex(pixels);std::printf("%s[%u,\"%s\"]",index ? "," : "",flags[index],hex.c_str());
    }
    std::printf("]}\n");
}
}
