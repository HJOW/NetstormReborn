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
