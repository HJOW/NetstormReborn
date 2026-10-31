// 실제 보호 CD 디코더와 같은 공통 Gump의 등록/모달/픽셀/idle/반납을 창 없이 검사한다.
#include "client/RawMovieControls.h"
#include "client/SmackerDecoder.h"

namespace { // 화면/음향만 명시 장치로 두고 영상/트리/수명은 제품을 사용한다.
struct MovieProbe {
    double now{};int updateDepth{},gameDepth{},muteDepth{},cursorDepth{},restore{},clears{}; // 게임 정지와 별개인 명시 벽시계/현재 장치 관측이다.
    std::array<ScreenColor,256> palette{}; // 적용 callback이 실제로 바꾸는 현재 BGR0 배열이다.
    RawMovieContext Context() { // 같은 원본 시작/종료 효과를 실제 Gump가 호출하도록 제공한다.
        RawMovieContext c;c.now=[this] { return now; };c.milliseconds=[this] { return static_cast<std::uint32_t>(now*1000); };c.clearAndPresent=[this] { ++clears; };c.invalidate=[](auto,auto) {};auto& h=c.devices;
        h.capturePalette=[this](auto output) { std::copy(palette.begin(),palette.end(),output.begin()); };h.applyPalette=[this](auto input) { input[0]={};input[255]={255,255,255,0};std::copy(input.begin(),input.end(),palette.begin()); };
        h.shutdownSound=[] {};h.muteSound=[this] { ++muteDepth; };h.unmuteSound=[this] { --muteDepth; };h.soundEnabled=[] { return true; };h.restoreSound=[this](auto quality) { CHECK(quality==3);++restore; };
        h.pauseUpdates=[this](auto paused) { updateDepth+=paused ? 1 : -1; };h.pauseGame=[this](auto paused) { gameDepth+=paused ? 1 : -1; };h.showCursor=[this](auto visible) { cursorDepth+=visible ? -1 : 1; };h.refreshCursor=[this] { cursorDepth=0; };
        h.clear=[this](auto,auto color) { CHECK(color==0);++clears; };h.black=[] { return 0; };h.redraw=[](auto) {};h.assertion=[](auto) { throw std::logic_error("영상 검사 예상 밖 진단"); };return c;
    }
};
}

// 파일 부재의 부분 생성 노드도 공통 지연 삭제가 반납하고 가짜 singleton은 만들지 않는다.
TEST_CASE(RawMenuMovie_FailedOpenHasNoSingletonAndUsesCommonScalar) {
    MovieProbe probe;MenuClient app;auto& movie=app.menus->EnableMovies(probe.Context());CHECK(!movie.Open(app.root.parent_path()/"missing-movie-594.smk"));CHECK(!movie.Active());CHECK(probe.updateDepth==0 && probe.gameDepth==0 && probe.clears==0);
    app.buttons.CleanupGumps();CHECK(movie.Report().find("movie_runtime\t0\t0\t1\t0\t0")!=std::string::npos);
}

// 네 화면/입력 사례에서 실제 영상 픽셀·모달·현재 정지/팔레트 복원과 singleton 반납 시점을 확인한다.
TEST_CASE(RawMenuMovie_ActualDiscPixelsFourSkipInputsAndDelayedSingleton) {
    constexpr std::array<std::array<int,3>,4> cases{{{1024,768,27},{1280,720,32},{1280,800,0x10000},{1024,768,0x20000}}}; // ESC/Space/좌/우 버튼의 실제 원시 입력이다.
    // 각 실제 영상은 별도 공통 UI에 연결해 이전 singleton/팔레트를 빌리지 않는다.
    for (const auto entry:cases) {
        MovieProbe probe;probe.palette.fill({1,2,3,0});MenuClient app;app.Resize(entry[0],entry[1]);auto& movie=app.menus->EnableMovies(probe.Context());const auto path=app.root.parent_path()/"originalCD/MOVIE/englishintro2x.smk";
        CHECK(movie.Open(path));CHECK(movie.Active() && probe.updateDepth==1 && probe.gameDepth==1 && probe.muteDepth==1 && probe.cursorDepth==1);CHECK(!movie.Open(path));
        app.buttons.IdleGumps(); // SDK의 최초 기한을 준비하고 아래에서 명시 시각만 진행한다. 프레임 번호를 강제로 바꾸지 않는다.
        // 실제 첫180개 프레임을 공통 idle로 진행한다.
        for (int frame=0;frame<180;++frame) { probe.now+=0.07;app.buttons.IdleGumps(); }
        auto reference=SmackerDecoder::OpenFile(path);CHECK(reference!=nullptr);if (!reference) return;
        // 비교용 별도 디코더도 같은 원시179번째 프레임까지 진행한다.
        for (int frame=0;frame<180;++frame) CHECK(reference->Next()==SmackerFrameStatus::More);
        const int pitch=entry[0]+7,height=entry[1],x=(entry[0]-640)/2,y=(entry[1]-320)/2;std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch)*height,0xa5);std::array<std::uint32_t,256> logical{};
        app.menus->Draw(pixels,pitch,height,app.Palette(),BuildGumpShadeMaps(logical));const auto expected=reference->Pixels();CHECK(std::any_of(expected.begin(),expected.end(),[](auto pixel) { return pixel!=0; })); // 첫110개의 검정 프레임만 비교하는 검사는 허용하지 않는다.
        // 원래 크기의 영상 모든 행과 행 padding을 확인한다. 리샘플/색 양자화는 허용하지 않는다.
        for (int row=0;row<320;++row) CHECK(std::equal(expected.begin()+row*640,expected.begin()+(row+1)*640,pixels.begin()+(row+y)*pitch+x));
        CHECK(pixels[0]==0 && pixels[pitch-1]==0xa5);const auto before=movie.Report();CHECK(before.find("\t180\t1\t0\t0\t")!=std::string::npos);
        const auto routed=app.buttons.RouteInput({static_cast<std::uint32_t>(entry[2]),300,300});CHECK(routed.consumed!=0);CHECK(movie.Active()); // Kill 예약만으로00595074를 먼저 지우지 않는다.
        CHECK(probe.updateDepth==0 && probe.gameDepth==0 && probe.muteDepth==0 && probe.cursorDepth==0 && probe.restore==1);CHECK(probe.palette[1].red==3);
        app.buttons.CleanupGumps();CHECK(!movie.Active());CHECK(movie.Report().find("movie_runtime\t0\t0\t1\t180\t0")!=std::string::npos);
    }
}

// 자연 종료의 원문 마지막 프레임 관문과 다음 정상 Cleanup의 실제 singleton 해제를 검사한다.
TEST_CASE(RawMenuMovie_DiscNaturalEndKeepsNativeLastFrameGate) {
    MovieProbe probe;MenuClient app;auto& movie=app.menus->EnableMovies(probe.Context());CHECK(movie.Open(app.root.parent_path()/"originalCD/MOVIE/englishintro2x.smk"));probe.now=0;
    // 원문처럼 갱신 잠금 중에는 idle만 실행하고 end가 잠금을 반납한 다음 바퀴에 Cleanup한다.
    for (int frame=0;frame<1900 && movie.Active();++frame) { probe.now+=0.07;if (!probe.updateDepth) app.buttons.CleanupGumps();app.buttons.IdleGumps(); }
    CHECK(!movie.Active() && probe.updateDepth==0 && probe.gameDepth==0 && probe.muteDepth==0 && probe.restore==1);CHECK(movie.Report().find("movie_runtime\t0\t0\t1\t1889\t0")!=std::string::npos);
}


#include "client/RawRendererPrelude.h"

namespace { // 실제 공통 override 조회를 이미 복원한 Renderer 앞단에 연결하는 명시 장치만 둔다.
struct MovieOverrideRendererProbe {
    RawButtonControls& buttons; // 영화와 같은 등록표/수집 override를 직접 조회한다.
    std::array<std::uint32_t,19> scan{}; // 프레임 사이에 보존되는 실제 행 스캔/이전 화면 flags 입력이다.
    std::uint32_t background{},dirtyCount{},drawCount{},force{},mode{42}; // DIB2/구름8/움직임32의 정상 호환 모드와 독립 요청 DWORD다.
    std::int32_t width{640},height{480},origin{}; // 실제 메뉴 검사와 같은 화면/카메라/지도 원점 입력이다.
    std::size_t updates{},rows{}; // 구름 갱신과 행 조회가 실제 선택된 횟수만 센다.
    explicit MovieOverrideRendererProbe(RawButtonControls& value):buttons(value) {} // 새 override 사본을 만들지 않고 현재 공통 소유자를 빌린다.
    void Prepare() { // 화면 flags는 유지한 채 실제 영화 override의 설치/반납이 선택하는 앞단 분기를 실행한다.
        RendererPreludeHooks hooks;hooks.markCloudDirty=[] {};hooks.queueCloudDirty=[](auto) {};
        hooks.overrideActive=[this] { return buttons.CurrentOverride(); };hooks.updateClouds=[this] { ++updates; };
        hooks.rowDirty=[this](auto,auto) { ++rows;return 0U; };hooks.invalidate=[](auto,auto) {};
        PrepareRendererFrame({scan,background,dirtyCount,drawCount},{mode,force,width,height,height,origin,origin,origin,width,height},hooks);
    }
};
}

//실제 파일은 Open의 최초 준비만 읽는다. Idle/Draw/PCM 제출/창 재생 없이 override가 present/Begin보다 먼저 설치되는지 확인한다.
TEST_CASE(RawMenuMovie_OverrideIsInstalledBeforePresentAndBegin) {
    MovieProbe probe;MenuClient app;RawMovieControls* movie{};std::uint32_t token{};bool presented{},captured{},ended{};auto context=probe.Context();
    const auto capture=context.devices.capturePalette;context.devices.capturePalette=[&,capture](auto palette) {
        CHECK(presented && app.buttons.CurrentOverride()==token);CHECK(probe.updateDepth==0 && probe.gameDepth==0);captured=true;capture(palette);
    }; // Begin의 첫 장치에서도 같은 override가 유지되며 정지는 그 뒤에 적용된다.
    context.clearAndPresent=[&] {
        ++probe.clears;token=app.buttons.CurrentOverride();CHECK(token && movie && movie->Active());
        if (token && movie) { const auto* node=movie->Node(token);CHECK(node && node->parent==app.buttons.RootToken() && node->depth==std::bit_cast<std::uint32_t>(-99) && !(node->flags&0x80000U)); }
        CHECK(!captured && probe.updateDepth==0 && probe.gameDepth==0);presented=true;
    }; // 원문의 depth-99/부착 뒤 즉시 출력이며 별도 overlay 표 등록은 없어야 한다.
    const auto clear=context.devices.clear;context.devices.clear=[&,clear](auto rect,auto color) { CHECK(app.buttons.CurrentOverride()==token);ended=true;clear(rect,color); }; // End 동안은 유지하고 뒤 공통 Kill에서만 override를 반납한다.
    movie=&app.menus->EnableMovies(std::move(context));CHECK(movie->Open(app.root.parent_path()/"originalCD/MOVIE/englishintro2x.smk"));if (!movie->Active() || !token) return;
    CHECK(presented && captured && !ended && app.buttons.CurrentOverride()==token && probe.updateDepth==1 && probe.gameDepth==1);CHECK(!movie->Open(app.root.parent_path()/"missing-override-670.smk") && app.buttons.CurrentOverride()==token);
    CHECK(movie->Handle(token,0x1bU)==1);CHECK(ended && !app.buttons.CurrentOverride() && movie->Active() && probe.updateDepth==0 && probe.gameDepth==0); // singleton의 지연 반납과 즉시 override 해제를 구별한다.
    app.buttons.CleanupGumps();CHECK(!movie->Active() && !app.buttons.CurrentOverride());
    CHECK(!movie->Open(app.root.parent_path()/"missing-override-670.smk") && !app.buttons.CurrentOverride());app.buttons.CleanupGumps();
}

//실제 수집 머리와 Renderer override 분기를 같은 영화 writer에 연결하고 Kill/Close 뒤 원래 순회를 확인한다. 영상 프레임은 진행하지 않는다.
TEST_CASE(RawMenuMovie_OverrideSelectsCollectionAndRendererUntilCommonKill) {
    MovieProbe probe;MenuClient app;MovieOverrideRendererProbe renderer(app.buttons);renderer.Prepare();CHECK(renderer.updates==1 && renderer.rows==30);
    auto& movie=app.menus->EnableMovies(probe.Context());const auto path=app.root.parent_path()/"originalCD/MOVIE/englishintro2x.smk";CHECK(movie.Open(path));if (!movie.Active()) return;const auto token=app.buttons.CurrentOverride();CHECK(token!=0);if (!token) return;
    std::uint32_t laterMenu{};app.menus->Open({200,100},[&](const auto& widgets,const auto& controls,const auto&) { laterMenu=widgets.BeginVertical();controls.AddText("Later",0x02000060U,0); }); // 영화 뒤 부착된 실제 새 root 자식이 override 앞에 있어야 한다.
    CHECK(laterMenu!=0);std::array<netstorm::o::RenderCollectionEntry,5000> entries{};const auto count=app.menus->CollectDrawing({0,0,640,480},entries,5000);const auto visible=std::span(entries).first(static_cast<std::size_t>(count));
    CHECK(std::any_of(visible.begin(),visible.end(),[&](const auto& entry) { return entry.token==token; }));CHECK(std::none_of(visible.begin(),visible.end(),[&](const auto& entry) { return entry.token==laterMenu; }));
    renderer.Prepare();CHECK(renderer.updates==1 && renderer.rows==30 && app.buttons.CurrentOverride()==token); // 같은 실제 override 때문에 구름 Update/행 스캔을 모두 생략한다.
    CHECK(movie.Handle(token,0x1bU)==1 && !app.buttons.CurrentOverride());const auto afterCount=app.menus->CollectDrawing({0,0,640,480},entries,5000);const auto after=std::span(entries).first(static_cast<std::size_t>(afterCount));
    CHECK(std::any_of(after.begin(),after.end(),[&](const auto& entry) { return entry.token==laterMenu; }));CHECK(std::none_of(after.begin(),after.end(),[&](const auto& entry) { return entry.token==token; }));
    renderer.Prepare();CHECK(renderer.updates==2 && renderer.rows==60);app.buttons.CleanupGumps();CHECK(!movie.Active());
    CHECK(movie.Open(path) && app.buttons.CurrentOverride());movie.Close();CHECK(!movie.Active() && !app.buttons.CurrentOverride() && probe.updateDepth==0 && probe.gameDepth==0); // 재진입 뒤 Close도 별도 override 정리 없이 같은 Kill/Cleanup을 사용한다.
}
