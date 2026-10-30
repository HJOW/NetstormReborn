// 원본 MovieGump 공장/소멸과 검증한592/593 본체를 현재 공통 Gump 장치에 연결한다.
#define NOMINMAX
#include "client/RawMovieControls.h"
#include "client/RawMenuControls.h"
#include "client/GumpDraw.h"
#include "client/MoviePcmOutput.h"
#include "client/MovieTimer.h"
#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace netstorm::client { // 같은 현재 노드/스트림/PCM을 원문 순서의 callback에 공급한다.
struct RawMovieControls::Impl { // 실제 공통 저장소와 singleton/디코더/표시 시계를 고정 주소로 소유한다.
    struct Movie: GumpCreateNode { // 기본 노드 주소가 모든 공통 등록/입력/수명 처리기에서 같다.
        MovieLifecycleState movie;std::unique_ptr<SmackerDecoder> decoder;std::vector<std::uint8_t> buffer; // 원래 팔레트/압축 스트림과 표시용 원시 픽셀이다.
        std::filesystem::path source; // 실제 OpenFile에 전달한 경로를 관측하며 디코더 선택/파일 내용을 바꾸지 않는다.
        std::unique_ptr<MoviePcmOutput> pcm;std::array<std::uint8_t,768> palette{}; // 현재 준비된 RGB와 비동기 PCM 소유자다.
        bool paletteChanged{},rectPending{},begun{},ended{};double epoch{};std::uint32_t displayed{}; // 현재 변경/시작·종료/벽시계 원점과 실제 표시 수다.
        MovieTimerState timer;std::uint32_t originTicks{}; // SDK fallback 시계와 같은 영상 구간의 시작 밀리초다.
    };
    RawMenuTree common;RawMovieContext context;std::map<std::uint32_t,std::unique_ptr<Movie>> nodes; // 같은 실제 공통 트리와 고정 파생 주소다.
    std::uint32_t current{},dirtyOverride{};std::uint64_t closed{},displayed{},pcmSubmitted{}; // 원문 singleton/dirty와 반납 뒤 남기는 읽기 전용 누적 관측이다.
    double beganAt{},endedAt{}; // 실제 시작과 end 호출 시각의 관측이며 재생 속도를 결정하는 값이 아니다.
    Impl(RawMenuTree tree,RawMovieContext devices):common(std::move(tree)),context(std::move(devices)) { // 원문10.78 조립만 허용하며 생성만으로 파일/장치를 열지 않는다.
        if (common.create.collection.edition!=o::OriginalEdition::Patch1078 || !context.now || !context.invalidate || !context.clearAndPresent) throw std::invalid_argument("MovieGump 실제10.78/시계/표시 장치 필요");
    }
    Movie* Node(std::uint32_t token) const { const auto it=nodes.find(token);return it==nodes.end() ? nullptr : it->second.get(); } // 죽은 노드도 scalar 반납 전에는 같은 주소를 반환한다.
    std::uint32_t Milliseconds() const { return context.milliseconds ? context.milliseconds() : timeGetTime(); } // 실제 SDK와 같은 밀리초 장치 또는 명시 검사 입력이다.
    bool AtEnd(const Movie& node) const { return node.decoder && node.decoder->FrameIndex()==node.decoder->Info().frameCount-1U; } //004838d0의 실제0기반 마지막 프레임 비교다.
    void Prepare(Movie& node) { // 원본 NextFrame이 준비하던 다음 자료를 별도64비트 디코더에서 미리 해제하되 아직 표시/재생하지 않는다.
        node.timer.ready=0;const auto status=node.decoder->Next();if (status!=SmackerFrameStatus::More && status!=SmackerFrameStatus::Last) throw std::runtime_error("영상 다음 프레임 해제 실패");
        const auto palette=node.decoder->Palette();node.paletteChanged=node.decoder->FrameIndex()==0 || !std::equal(palette.begin(),palette.end(),node.palette.begin());std::copy(palette.begin(),palette.end(),node.palette.begin());
    }
    MovieLifecycleHooks Lifecycle(std::uint32_t token) { // 같은 노드의 현재 Gump 기하/디코더와 Client의 실제 장치를 조립한다.
        auto& node=*Node(token);auto h=context.devices;h.isOpen=[&node] { return node.decoder!=nullptr; };h.hasBuffer=[&node] { return !node.buffer.empty(); };
        h.videoSize=[&node] { return ScreenPoint{static_cast<std::int32_t>(node.decoder->Info().width),static_cast<std::int32_t>(node.decoder->Info().height)}; };h.windowSize=[&node] { return std::array<float,2>{node.width,node.height}; };
        h.bindBuffer=[this,&node] { node.epoch=context.now();node.originTicks=Milliseconds(); }; // 실제 버퍼는open에서 할당했다. 게임 시계를 쓰지 않는다.
        h.idle=[this,token](auto enabled) { common.lifetime.SetIdle(token,enabled); };h.bounds=[this,token] { return common.collection.Bounds(token); };
        h.redraw=[this](auto erase) { common.changed();if (context.devices.redraw) context.devices.redraw(erase); };
        return h;
    }
    void End(std::uint32_t token) { // 같은 입력/idle 종료가 두 정지를 복원하고 Kill은 호출한 원문 판단에 남긴다.
        auto& node=*Node(token);if (!node.begun || node.ended) return;endedAt=context.now();EndMovieLifecycle(node.movie,Lifecycle(token));node.ended=true;common.changed();
    }
    MoviePlaybackHooks Playback(std::uint32_t token) { // 검증한 원문 판단에 현재 디코더/PCM·원시 전체 변경 영역을 제공한다.
        auto& node=*Node(token);MoviePlaybackHooks h;h.atEnd=[this,&node] { return AtEnd(node); };
        h.wait=[this,&node] {
            if (node.pcm->Ready()) { if (!node.displayed) return false;if (const auto elapsed=node.pcm->ElapsedSeconds()) return *elapsed<static_cast<double>(node.decoder->FrameIndex())*node.decoder->Info().microsecondsPerFrame/1000000.0; }
            MovieTimerHooks timer;timer.milliseconds=[this,&node] { return Milliseconds()-node.originTicks; }; // Windows10/11의 장시간 uptime에서 SDK100배 tick의 중간 감김으로 멈추지 않도록 같은 영상 구간의 상대 tick을 입력한다. SDK 판단 자체는 원문 그대로다.
            timer.yield=[] {};return WaitMovieTimer(node.timer,timer); // 원본 SDK의 소리 서비스는 독립 waveOut가 처리하며 기존 Client 루프 대기를 유지한다.
        };
        h.paletteChanged=[&node] { return node.paletteChanged; };h.palette=[&node] { return std::span<const std::uint8_t,768>(node.palette); };h.paletteLog=[] { std::clog<<"SmackerMovieGump::setNewPalette()\n"; };
        h.doFrame=[&node] { const auto pixels=node.decoder->Pixels();std::copy(pixels.begin(),pixels.end(),node.buffer.begin());node.rectPending=true;++node.displayed;
            // 실제 원문 파일이 가진 일곱 트랙만 원래 PCM 규격으로 출력한다.
            for (std::uint32_t track=0;track<7;++track) node.pcm->Submit(track,node.decoder->Info().audio[track],node.decoder->Audio(track),static_cast<double>(node.decoder->FrameIndex())*node.decoder->Info().microsecondsPerFrame/1000000.0);
        };
        h.origin=[this,token] { return common.collection.Position(token); };h.nextRect=[&node]() -> std::optional<ScreenRect> { if (!node.rectPending) return {};node.rectPending=false;return ScreenRect{0,0,static_cast<int>(node.decoder->Info().width),static_cast<int>(node.decoder->Info().height)}; }; // 원본32비트 SDK의 dirty API 대신64비트 디코더의 같은 원시 프레임 전체를 무효화한다. 픽셀은 바꾸지 않는다.
        h.invalidate=[this](auto rect,auto flags) { context.invalidate(rect,flags);common.changed(); };h.nextFrame=[this,&node] { Prepare(node); };
        h.end=[this,token] { End(token); };h.kill=[this,token] { common.lifetime.Kill(token); };h.invalidOption=[](auto option) { std::clog<<"잘못된 MovieGump 옵션 "<<option<<'\n'; };return h;
    }
};
RawMovieControls::RawMovieControls(RawMenuTree tree,RawMovieContext context):impl_(std::make_unique<Impl>(std::move(tree),std::move(context))) {} // 실제 공통 장치 주소를 고정한다.
RawMovieControls::~RawMovieControls()=default; // 공통 소유자가 먼저 Close/scalar를 실행하고 잔여 RAII 메모리만 해제한다.
bool RawMovieControls::Open(const std::filesystem::path& path) {
    auto& s=*impl_;if (s.current) { if (!s.AtEnd(*s.Node(s.current))) return false;s.common.lifetime.Kill(s.current);s.current=0; }
    const auto token=s.common.token();auto owned=std::make_unique<Impl::Movie>();s.nodes.emplace(token,std::move(owned));auto& node=*s.Node(token);s.common.factory.Construct(token,0,0,0,1);node.vtable=0x50d8d8U;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(s.context.type));node.flags|=0x600U;
    node.source=path;node.decoder=SmackerDecoder::OpenFile(path);if (!node.decoder) { std::clog<<"인트로 영상 열기 실패: "<<path.string()<<'\n';s.common.lifetime.Kill(token);return false; }
    try {
        const auto& info=node.decoder->Info();if (!info.frameCount || info.width>static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) || info.height>static_cast<std::uint32_t>((std::numeric_limits<int>::max)())) throw std::runtime_error("영상 실제 치수 범위");node.buffer.resize(static_cast<std::size_t>(info.width)*info.height);node.pcm=std::make_unique<MoviePcmOutput>(s.context.audio,s.context.mute);node.pcm->Prepare(info.audio);node.timer.interval=static_cast<std::uint32_t>(std::llround(info.microsecondsPerFrame/10.0));s.Prepare(node);
    }catch (...) { node.decoder.reset();s.common.lifetime.Kill(token);throw; } // 부분 실패 노드는 공통 scalar가 반납하며 영상 성공으로 가장하지 않는다.
    s.current=token;node.baseWidth=static_cast<float>(s.common.create.screenWidth);node.baseHeight=static_cast<float>(s.common.create.screenHeight);node.width=node.baseWidth*node.scale;node.height=node.baseHeight*node.scale;
    s.common.input.AddDialog(token);s.common.lifetime.SetOverlay(token,true);auto playback=s.Playback(token);SetMoviePlaybackOption(node.movie.playback,1,1,playback);SetMoviePlaybackOption(node.movie.playback,2,1,playback);
    s.common.attach(token,(static_cast<float>(s.common.create.screenWidth)-node.baseWidth)*0.5F,(static_cast<float>(s.common.create.screenHeight)-node.baseHeight)*0.5F,s.common.create.collection.root,0);node.depth=std::bit_cast<std::uint32_t>(-99);s.context.clearAndPresent();
    BeginMovieLifecycle(node.movie,s.Lifecycle(token));node.begun=true;s.beganAt=node.epoch;s.endedAt=0;s.common.changed();std::clog<<"인트로 영상 시작 "<<token<<' '<<node.decoder->Info().width<<'x'<<node.decoder->Info().height<<'\n';return true;
}
bool RawMovieControls::Active() const { return impl_->current!=0; } // 원문 singleton은 실제 scalar 소멸에서만 지워진다.
GumpCreateNode* RawMovieControls::Node(std::uint32_t token) const { return impl_->Node(token); } // 공통 저장소와 같은 기본 주소다.
std::uint32_t RawMovieControls::Handle(std::uint32_t token,std::uint32_t event) const { auto& s=*impl_;return HandleMoviePlayback(s.Node(token)->movie.playback,event,s.Playback(token)); } // 원문 종료 입력만 소비한다.
void RawMovieControls::Idle(std::uint32_t token) const { auto& s=*impl_;IdleMoviePlayback(s.Node(token)->movie.playback,s.dirtyOverride,s.Playback(token)); } // 원문 atEnd/대기/dirty/다음 준비 순서다.
void RawMovieControls::Draw(std::uint32_t token,const GumpPixelTarget& pixels,ScreenRect clip) const {
    auto& s=*impl_;auto& node=*s.Node(token);auto h=s.Lifecycle(token);h.clip=[clip] { return clip; };h.fill=[&pixels](auto rect,auto color) { pixels.Fill(rect,color); };
    h.blit=[&pixels,&node](auto position,auto size) { pixels.RawBitmap(node.buffer,size.x,0,size,position,{(std::numeric_limits<int>::min)(),(std::numeric_limits<int>::min)(),(std::numeric_limits<int>::max)(),(std::numeric_limits<int>::max)()}); };DrawMovieLifecycle(node.movie,h);
}
void RawMovieControls::DeleteScalar(std::uint32_t token,std::uint32_t flags) {
    auto& s=*impl_;auto& node=*s.Node(token);s.displayed+=node.displayed;if (node.pcm) s.pcmSubmitted+=node.pcm->SubmittedBytes();node.pcm.reset();node.decoder.reset();node.buffer.clear();if (s.current==token) s.current=0;++s.closed;s.common.lifetime.DestructBase(token);if (flags&1U) s.nodes.erase(token);s.common.changed();std::clog<<"인트로 영상 반납 "<<token<<'\n';
}
void RawMovieControls::Close() {
    auto& s=*impl_;std::vector<std::uint32_t> tokens; // 삭제 callback이 map을 반납하므로 현재 토큰 목록만 먼저 보관한다.
    // 실제 생성된 노드를 end/Kill한 뒤 공통 Cleanup에서만 메모리를 해제한다.
    for (const auto& [token,node]:s.nodes) { s.End(token);if (!(node->flags&0x10000U)) tokens.push_back(token); }
    // 보관한 각 노드를 같은 공통 지연 삭제에 전달한다.
    for (auto token:tokens) s.common.lifetime.Kill(token);s.common.lifetime.Cleanup();
}
std::string RawMovieControls::Report() const {
    const auto& s=*impl_;std::ostringstream out;out<<"movie_runtime\t"<<s.current<<'\t'<<s.nodes.size()<<'\t'<<s.closed<<'\t'<<s.displayed<<'\t'<<s.pcmSubmitted<<'\n';
    out<<"movie_timing\t"<<s.beganAt<<'\t'<<s.endedAt<<'\n'; // 실제 시계의 시작/end 진입 시각만 관측한다.
    // 살아 있는 실제 스트림/현재 표시 수와 PCM 수락을 각각 읽는다.
    for (const auto& [token,node]:s.nodes) { const auto path=node->source.u8string();out<<"movie_source\t"<<token<<'\t'<<std::string(path.begin(),path.end())<<'\n';out<<"movie_stream\t"<<token<<'\t'<<(node->decoder ? node->decoder->FrameIndex() : 0)<<'\t'<<node->displayed<<'\t'<<node->begun<<'\t'<<node->ended<<'\t'<<(node->pcm ? node->pcm->SubmittedBytes() : 0)<<'\t'<<node->flags<<'\n'; }return out.str();
}
}
