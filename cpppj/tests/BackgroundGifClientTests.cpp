// 실제 보호 VFS/설정과 공통 등록표에 원문 GIF 공장·원시 픽셀·입력 큐·수명을 접합한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawBackgroundGifControls.h"
#include "client/GifImage.h"
#include "client/PaletteShade.h"
#include "platform/Bitmap.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
using namespace netstorm::client;
namespace { // 보호 자산의 같은 실제 UI와 외부 입력 관측만 구성한다.
ClientOptions GifOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 창/게임 루프 없이 현재 Client를 만든다.
std::uint64_t GifFingerprint(std::span<const std::uint8_t> bytes) { std::uint64_t value=14695981039346656037ULL;for (const auto byte:bytes) value=(value^byte)*1099511628211ULL;return value; } // 실제 출력 바이트만 독립 Native 관측과 비교한다.
std::vector<std::uint8_t> GifVariant(std::span<const std::uint8_t> original,std::string_view name) { // 독립 Native와 같은 입력 헤더만 바꾸며 기대 픽셀은 계산하지 않는다.
    const std::size_t descriptor=13+(3U<<((original[10]&7)+1));std::vector<std::uint8_t> data(original.begin(),original.end()); // 같은 보호 전역 팔레트 뒤 descriptor다.
    const auto word=[&](std::size_t offset,std::uint16_t value) { data[offset]=static_cast<std::uint8_t>(value);data[offset+1]=static_cast<std::uint8_t>(value>>8); }; // 명시 입력의 리틀 엔디언 두BYTE다.
    if (name=="localPalette") { data.resize(13);data[10]&=0x7f;data.insert(data.end(),original.begin()+descriptor,original.begin()+descriptor+10);data[22]=(data[22]&0x78)|0x80|(original[10]&7);data.insert(data.end(),original.begin()+13,original.begin()+descriptor);data.insert(data.end(),original.begin()+descriptor+10,original.end());return data; }
    CHECK(name=="logicalOffsets" || name=="smallCanvas" || name=="interlaced");const bool small=name=="smallCanvas";word(6,small ? 1 : 683);word(8,small ? 1 : 521);word(descriptor+1,small ? 40000 : 17);word(descriptor+3,small ? 40000 : 19);if (name=="interlaced") data[descriptor+9]|=0x40;return data;
}
struct GifScene { // 모든 실제 Client/파일/설정/글꼴/공통 UI 수명을 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{GifOptions()}; // 보호 자산과 장기 소유자다.
    FontStore fonts{assets.files,"Arial"};std::array<std::uint32_t,256> logical{};PaletteColorTable palette; // 같은 실제 글꼴/팔레트다.
    std::vector<std::array<std::uint32_t,3>> queued;std::uint32_t destroyOnQueue{}; // 입력 장치의 실제 전달 순서와 명시 재진입 입력이다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 0.0; }}};RawMenuControls* menus{};RawBackgroundGifControls* gifs{}; // 같은 실제 공통 등록표와 원문 창이다.
    GifScene() { // 파일 저장이나 Win32 창 없이 실제 원문 설정/공통 UI를 설치한다.
        ConfigInterface::StartupOptions startup;startup.majorVersion=10;startup.minorVersion=78;startup.installDir=assets.root.string();startup.cdDir=startup.installDir;client.Configuration().Startup(startup);
        for (std::size_t index=0;index<logical.size();++index) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 실제 보호 RGB의 원문 DWORD 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);Resize();
        RawBackgroundGifContext context;context.files=&client.Files();context.config=&client.Configuration();context.dispatch=[this](auto event,auto first,auto second) { queued.push_back({event,first,second});if (destroyOnQueue) { const auto token=destroyOnQueue;destroyOnQueue=0;buttons.KillGump(token);buttons.CleanupGumps(); } };gifs=&menus->EnableBackgroundGifs(std::move(context));
    }
    void Resize() { const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}}); } // 실제 페이지 전환/반납이다.
    std::uint32_t Count() const { std::istringstream input(buttons.Report());std::string name;std::uint32_t controls{},count{};input>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 같은 현재 등록표 수만 관측한다.
    std::vector<std::uint8_t> Draw(std::optional<ScreenRect> clip={}) { std::vector<std::uint8_t> pixels(647*480,0x5a);menus->Draw(pixels,647,480,palette,BuildGumpShadeMaps(logical),clip);return pixels; } // 실제 공통 정렬/위치/클립/행 padding으로 그린다.
};
}

// 기본 BackgroundGifSpec을 실제 파일/설정에서 치환하고 미부착/동시 창의 독립 이미지 소유를 확인한다.
TEST_CASE(BackgroundGifClient_ActualDefaultPathFactoryAndConcurrentStorage) {
    GifScene scene;const auto before=scene.Count(),first=scene.gifs->Create("titleMenu"),second=scene.gifs->Create("defeat");auto& a=*scene.gifs->Node(first);auto& b=*scene.gifs->Node(second);
    CHECK(scene.menus->Node(first)==&a && a.vtable==0x501b60 && a.command==3 && a.flags==9 && a.event==0 && a.first==0 && a.second==0 && a.bitmap && b.bitmap && a.bitmap!=b.bitmap);
    CHECK(scene.gifs->ImageCount()==2 && scene.Count()==before+2 && a.baseWidth==639 && a.baseHeight==480 && b.baseWidth==640 && b.baseHeight==480);
    const auto old=b.bitmap;scene.gifs->Backend().Refresh(second,"victory");CHECK(b.bitmap!=old && scene.gifs->ImageCount()==2);bool oldReleased{};try { scene.gifs->Bitmap(old); }catch (const std::out_of_range&) { oldReleased=true; }CHECK(oldReleased);
    scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(first) && scene.menus->Node(second)==&b && scene.gifs->ImageCount()==1 && scene.Count()==before+1);scene.menus->CloseBackgroundGifs();CHECK(scene.Count()==before && scene.gifs->ImageCount()==0);
}

// 원본 LZW 본체로 미리 관측한 보호 GIF 전체의 색 번호와 실제 공장 출력을 대조한다.
TEST_CASE(BackgroundGifClient_AllProtectedAssetsIndependentNativePixels) {
    GifScene scene;std::ifstream input(NETSTORM_BACKGROUND_GIF_ASSETS_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // C++ 해독기에서 기대 체크섬을 만들지 않고 각 최초 Native 행을 읽는다.
        std::istringstream row(line);std::string path,fnv,pixelSha,sourceSha;std::uint32_t width{},height{},nativeRows{};std::uint64_t blocks{};row>>path>>width>>height>>fnv>>pixelSha>>sourceSha>>nativeRows>>blocks;CHECK(row && nativeRows==height && blocks>0);
        scene.client.Configuration().Set("BackgroundGifSpec",path);const auto token=scene.gifs->Create("native-observation");const auto& node=*scene.gifs->Node(token);const auto& bitmap=scene.gifs->Bitmap(node.bitmap);
        CHECK(node.baseWidth==width && node.baseHeight==height && bitmap.width==width && bitmap.height==height && bitmap.indices.size()==std::size_t(width)*height && GifFingerprint(bitmap.indices)==std::stoull(fnv,nullptr,16));
        scene.menus->CloseBackgroundGifs();CHECK(scene.gifs->ImageCount()==0);++rows;
    }
    CHECK(rows==34);
}

// 원문은 논리 화면/오프셋을 무시하고 첫 descriptor·로컬 표·네 인터레이스 패스를 사용한다.
TEST_CASE(BackgroundGifClient_IndependentNativeDescriptorAndInterlaceVariants) {
    GifScene scene;const auto source=scene.client.Files().Read("d/titleMenu.gif");std::ifstream input(NETSTORM_BACKGROUND_GIF_VARIANTS_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 메모리 입력4개와 최초 Native 픽셀 지문만 대조한다.
        std::istringstream row(line);std::string name,fnv,pixelSha,sourceSha;std::uint32_t width{},height{},nativeRows{};std::uint64_t blocks{};row>>name>>width>>height>>fnv>>pixelSha>>sourceSha>>nativeRows>>blocks;CHECK(row && nativeRows==height && blocks>0);
        const auto data=GifVariant(source,name);const auto image=DecodeBackgroundGif(data);CHECK(image.width==width && image.height==height && image.indices.size()==std::size_t(width)*height && GifFingerprint(image.indices)==std::stoull(fnv,nullptr,16));if (name=="logicalOffsets") CHECK(DecodeGif(data).width==683 && image.width==639);++rows;
    }
    CHECK(rows==4);
}

// 공통 수집/부착/클립을 사용하며 원문 Draw가 배율 치수 대신 원시 base 치수로 복사하는지 검사한다.
TEST_CASE(BackgroundGifClient_ActualCommonPixelsBaseDimensionsAndClip) {
    GifScene scene;const auto token=scene.gifs->Create("titleMenu");scene.menus->Attach(token,11,13,scene.buttons.RootToken(),1);auto& node=*scene.gifs->Node(token);node.width=320;node.height=240;const auto& bitmap=scene.gifs->Bitmap(node.bitmap);const auto pixels=scene.Draw();const ScreenRect clip{29,31,603,459};const auto clipped=scene.Draw(clip);std::size_t changed{},fullDifferences{},clipDifferences{};
    for (int y=0;y<480;++y) for (int x=0;x<647;++x) { const auto inside=x>=11 && x<640 && y>=13;const auto expected=inside ? bitmap.indices[(y-13)*bitmap.width+x-11] : std::uint8_t{0x5a};fullDifferences+=pixels[y*647+x]!=expected;const auto inClip=x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom;clipDifferences+=clipped[y*647+x]!=(inClip ? expected : 0x5a);changed+=clipped[y*647+x]!=0x5a; } // 실제 화면/클립과 행 끝7바이트를 모두 대조한다.
    CHECK(fullDifferences==0 && clipDifferences==0 && changed>0 && pixels[400*647+500]==bitmap.indices[(400-13)*bitmap.width+500-11]);
    if (const auto* output=std::getenv("NETSTORM_BACKGROUND_GIF_QA")) { std::vector<std::uint8_t> rgba(640*480*4);for (int y=0;y<480;++y) for (int x=0;x<640;++x) { const auto color=scene.assets.game.Palette().Color(pixels[y*647+x]);const auto at=(y*640+x)*4;rgba[at]=color.red;rgba[at+1]=color.green;rgba[at+2]=color.blue;rgba[at+3]=255; }netstorm::platform::WriteBitmap(output,640,480,rgba); } // 명시 QA 경로에만 실제 같은 표면을 쓴다.
    scene.menus->CloseBackgroundGifs();
}

// 원문 SetEvent/공통 Route와 raw 큐 사건의 수정자/해제/키 구분 및 콜백 후 재조회를 확인한다.
TEST_CASE(BackgroundGifClient_CommonInputPreservesRawQueueTripletAndRetirement) {
    GifScene scene;const auto before=scene.Count(),token=scene.gifs->Create("defeat");scene.menus->Attach(token,0,0,scene.buttons.RootToken(),0);scene.gifs->Backend().SetEvent(token,0xfedcba98,0x87654321,0xabcdef01);CHECK((scene.gifs->Node(token)->flags&0x600)==0x600);
    CHECK(scene.buttons.HandleGump(token,InputCode::kLeftButton,123,456)==1);CHECK(scene.buttons.HandleGump(token,InputCode::kRightButton|InputCode::kShift,123,456)==1);CHECK(scene.buttons.HandleGump(token,27|InputCode::kControl,123,456)==1);CHECK(scene.queued.size()==3);
    for (const auto& value:scene.queued) CHECK((value==std::array<std::uint32_t,3>{0xfedcba98,0x87654321,0xabcdef01})); // 사건을 Command로 정규화하지 않는다.
    scene.buttons.HandleGump(token,27|InputCode::kRelease,1,2);scene.buttons.HandleGump(token,27|InputCode::kCommand,1,2);scene.buttons.HandleGump(token,'A',1,2);CHECK(scene.queued.size()==3);
    const auto routed=scene.buttons.RouteInput({27,5,7});CHECK(routed.consumed==token && scene.queued.size()==4);scene.destroyOnQueue=token;CHECK(scene.buttons.HandleGump(token,27,5,7)==1);CHECK(!scene.menus->Node(token) && scene.Count()==before && scene.gifs->ImageCount()==0 && scene.queued.size()==5);
}

// 없는 파일/잘못된 파일의 부분 생성과 페이지 전환에서도 공통 등록표와 실제 이미지가 회수되어야 한다.
TEST_CASE(BackgroundGifClient_MissingFilePartialDecodeAndPageLifetime) {
    GifScene scene;const auto before=scene.Count(),token=scene.gifs->Create("titleMenu");scene.gifs->Backend().Refresh(token,"nonexistent-background-402");CHECK(scene.gifs->Node(token)->bitmap==0 && scene.gifs->Node(token)->baseWidth==0 && scene.gifs->Node(token)->height==0 && scene.gifs->ImageCount()==0);scene.Resize();CHECK(scene.Count()==before && !scene.menus->Node(token));
    scene.client.Configuration().Set("BackgroundGifSpec","d/setup.cfg");bool rejected{};try { scene.gifs->Create("invalid-gif-input"); }catch (const std::runtime_error&) { rejected=true; }CHECK(rejected && scene.Count()==before+1 && scene.gifs->ImageCount()==0);scene.menus->CloseBackgroundGifs();CHECK(scene.Count()==before);
    scene.client.Configuration().Set("BackgroundGifSpec","d/titleMenu.gif");scene.gifs->Create("first");scene.gifs->Create("second");CHECK(scene.gifs->ImageCount()==2);scene.Resize();CHECK(scene.Count()==before && scene.gifs->ImageCount()==0);
}
// ?? ??? ?? ?? Draw????????? ?? ?? ??? ?? GifScene?? ????.
#include "MainMenuPaintTests.inc"
