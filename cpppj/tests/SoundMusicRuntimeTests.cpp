// 두 채널/worker 수명의 독립 PE 관찰과 실제 Windows 스레드·Winmm 파일의 종료/재시작을 검사한다. 장치/창은 열지 않는다.
#include "RawSceneSupport.h"
#include "client/SoundMusicRuntime.h"
#include "client/SoundMusicFile.h"
#include "client/SoundMusicSelection.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <set>
#include <thread>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기에 공급한 COM/메모리 파일/커널 핸들과 실패 HRESULT다.
constexpr SoundBuffer kBuffer=0x16001010;
constexpr std::uint32_t kFile=0x17001000;
constexpr MusicRuntimeHandle kEvent=0x18001000,kThread=0x18002000;
constexpr std::int32_t kFail=static_cast<std::int32_t>(0x80004005U);
// C++는 저장한 독립 원본 관찰만 읽는다. 원본 PE/Python/게임을 실행하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() { static const auto data=LoadFixture(NETSTORM_MUSICRUNTIME_FIXTURE);return data.rows; }
// 이 계층에서 사용하지 않는 효과음 경계는 호출 시 진단한다. SoundPlayer 자체는 실제 공유 전역을 참조한다.
SoundDeviceHooks UnusedEffects() {
    SoundDeviceHooks hooks;
    hooks.status=[](SoundBuffer)->std::uint32_t { throw std::logic_error("예상하지 않은 효과음 조회"); };
    hooks.play=[](SoundBuffer,std::uint32_t) { throw std::logic_error("예상하지 않은 효과음 재생"); };
    hooks.setPosition=[](SoundBuffer,std::uint32_t) { throw std::logic_error("예상하지 않은 효과음 위치"); };
    hooks.setVolume=[](SoundBuffer,std::int32_t)->std::int32_t { throw std::logic_error("예상하지 않은 효과음 음량"); };
    hooks.setPan=[](SoundBuffer,std::int32_t)->std::int32_t { throw std::logic_error("예상하지 않은 효과음 좌우"); };
    hooks.stop=[](SoundBuffer) { throw std::logic_error("예상하지 않은 효과음 정지"); };
    hooks.duplicate=[](SoundBuffer,SoundBuffer&)->std::int32_t { throw std::logic_error("예상하지 않은 효과음 복제"); };
    hooks.load=[](std::string_view)->SoundLoad { throw std::logic_error("예상하지 않은 효과음 적재"); };return hooks;
}
// IO/COM 입력과 사건을 제공한다. 상태 전이·채우기·실패 정리는 실제 MusicChannel이 수행한다.
struct RuntimeBoundary {
    std::vector<std::uint8_t> file;std::array<std::uint8_t,128> output;std::vector<std::string> events;
    std::size_t position=5;bool opened=true,locked=false,readThrows=false,badRegions=false;
    int cap=-2,seekFailure{},failed{},split=-1,lockDepth{};std::uint32_t status{},cursor{};
    std::int32_t createResult{};bool createWrites=true;
    // 출력 전역은 원본 실행기와 같은 감시 바이트로 시작한다.
    RuntimeBoundary() { output.fill(0xa7); }
    // 파일 입력 바이트와 시작 위치다. 실제 디스크는 쓰지 않는다.
    void File(std::size_t length,std::size_t offset) {
        file.resize(5+length);position=5+offset;
        // 독립 파일 경계의 입력 패턴만 공급한다.
        for (std::size_t i=0;i<file.size();++i) file[i]=static_cast<std::uint8_t>(i*7+11);
    }
    // 원본 기록 문장은 줄바꿈을 뗀 사건으로 비교한다.
    void Log(std::uint32_t channel,std::string_view text) {
        while (!text.empty() && text.back()=='\n') text.remove_suffix(1);
        events.push_back("G:"+std::to_string(channel)+':'+std::string(text));
    }
    // 기존 채널의 파일·버퍼 정리 경계다. 실제 Read/Stop과 연결한다.
    MusicChannelHooks ChannelHooks() {
        MusicChannelHooks hooks;
        hooks.setVolume=[this](SoundBuffer token,std::int32_t volume) { CHECK(token==kBuffer);events.push_back("V:#1:"+std::to_string(volume));return failed==3 ? kFail : 0; }; // 음량 실패는 갱신을 중단하지 않는다.
        hooks.stop=[this](SoundBuffer token) { CHECK(token==kBuffer);events.push_back("X:#1"); }; // 정지 요청을 기록한다.
        hooks.release=[this](SoundBuffer token) { CHECK(token==kBuffer);events.push_back("B:#1");return 0; }; // 참조 해제를 기록한다.
        hooks.seek=[this](std::uint32_t token,std::uint32_t offset) {
            const auto result=opened && token==kFile && seekFailure!=-1 ? static_cast<std::int32_t>(offset) : -1;
            events.push_back("S:"+std::to_string(token)+':'+std::to_string(offset)+':'+std::to_string(result));if (result!=-1) position=offset;return result;
        }; // 절대 파일 위치 변경의 성공/실패를 입력한다.
        hooks.read=[this](std::uint32_t token,std::span<std::uint8_t> target) {
            if (readThrows) throw std::runtime_error("음악 읽기 예외 입력");
            const auto before=position;std::int32_t result=-1;
            if (opened && token==kFile && cap!=-1) {
                auto size=std::min(target.size(),position>file.size() ? std::size_t{} : file.size()-position);
                if (cap>=0) size=std::min(size,static_cast<std::size_t>(cap));
                if (size!=0) std::copy_n(file.begin()+position,size,target.begin());position+=size;result=static_cast<std::int32_t>(size);
            }
            events.push_back("R:"+std::to_string(token)+':'+std::to_string(before)+':'+std::to_string(target.size())+':'+std::to_string(result));return result;
        }; // 요청 크기와 실제 IO 반환을 별도로 기록한다.
        hooks.close=[this](std::uint32_t token) { events.push_back("C:"+std::to_string(token));if (token==kFile) opened=false; }; // 0/다른 토큰의 재닫기는 열린 입력 파일을 지우지 않는다.
        hooks.log=[this](std::uint32_t channel,std::string_view text) { Log(channel,text); }; // 원본 오류 사건이다.
        hooks.lockEvent=[this](bool enter) { lockDepth+=enter ? 1 : -1;CHECK(lockDepth>=0);events.push_back(enter ? "E" : "L"); };return hooks; // 중첩 잠금 균형을 확인한다.
    }
    // 장치의 잠금 구간·HRESULT만 돌려준다. 채우기 판단은 계산하지 않는다.
    MusicBufferHooks BufferHooks() {
        MusicBufferHooks hooks;
        hooks.create=[this](std::span<const std::uint8_t> format,std::uint32_t flags,std::uint32_t bytes,SoundBuffer& token) {
            token=createWrites ? kBuffer : 0;events.push_back("N:"+std::to_string(flags)+':'+std::to_string(bytes)+':'+Hex(format)+':'+std::to_string(token)+':'+std::to_string(createResult));return createResult;
        }; // COM 출력 토큰과 HRESULT를 독립 입력으로 쓴다.
        hooks.status=[this](SoundBuffer token,std::uint32_t& value) { CHECK(token==kBuffer);value=status;const auto result=failed==1 ? kFail : 0;events.push_back("T:#1:"+std::to_string(value)+':'+std::to_string(result));return result; }; // 상태와 HRESULT를 공급한다.
        hooks.restore=[this](SoundBuffer token) { CHECK(token==kBuffer);const auto result=failed==2 ? kFail : 0;events.push_back("A:#1:"+std::to_string(result));return result; }; // 복구 실패 입력이다.
        hooks.cursor=[this](SoundBuffer token,std::uint32_t& play,std::uint32_t& write) { CHECK(token==kBuffer);play=cursor;write=13;const auto result=failed==4 ? kFail : 0;events.push_back("Q:#1:"+std::to_string(play)+":13:"+std::to_string(result));return result; }; // 원본 갱신에 쓰기 커서는 영향이 없다.
        hooks.lock=[this](SoundBuffer token,std::uint32_t offset,std::uint32_t count,MusicBufferRegions& regions) {
            CHECK(token==kBuffer);CHECK(!locked);const auto result=failed==5 ? kFail : 0;
            const auto first=result<0 ? 0U : split<0 ? count : std::min(count,static_cast<std::uint32_t>(split));const auto second=result<0 ? 0U : count-first;
            if (result>=0) { regions={std::span(output).first(first),std::span(output).subspan(64,second)};locked=true;if (badRegions) regions.second=std::span(output).subspan(64,second+1); }
            events.push_back("K:#1:"+std::to_string(offset)+':'+std::to_string(count)+':'+std::to_string(first)+':'+std::to_string(second)+':'+std::to_string(result));return result;
        }; // 첫 구간/두 번째 구간은 독립 감시 출력에 둔다.
        hooks.unlock=[this](SoundBuffer token,MusicBufferRegions regions) { CHECK(token==kBuffer);CHECK(locked);locked=false;const auto result=failed==6 ? kFail : 0;events.push_back("U:#1:"+std::to_string(regions.first.size())+':'+std::to_string(regions.second.size())+':'+std::to_string(result));return result; }; // 읽기 실패/예외에도 잠금을 반납한다.
        hooks.play=[this](SoundBuffer token,std::uint32_t flags) { CHECK(token==kBuffer);const auto result=failed==7 ? kFail : 0;events.push_back("P:#1:"+std::to_string(flags)+':'+std::to_string(result));return result; };return hooks; // 파일 반복과 별도로 장치 반복 플래그를 기록한다.
    }
    // 사건을 순서대로 결합한다.
    std::string Events() const {
        std::string text;
        // 요청 순서를 바꾸지 않고 사건을 이어 붙인다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 실행기와 같은 초기 raw 바이트와 실제 파일 입력이다. 원본 출력으로 상태를 초기화하지 않는다.
void Seed(MusicChannelState& state,RuntimeBoundary& mock) {
    auto raw=state.Raw();
    // 변경하지 않는 모든 바이트의 보존을 관찰한다.
    for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
    state.SetField(MusicField::Buffer,kBuffer);state.SetField(MusicField::BufferBytes,16);state.SetField(MusicField::Flags,3);
    state.SetField(MusicField::File,kFile);state.SetField(MusicField::Length,16);state.SetField(MusicField::ReadOffset,0);
    state.SetField(MusicField::WriteOffset,9);state.SetField(MusicField::DataOffset,5);state.SetField(MusicField::StopDepth,0);mock.File(16,0);
}
// 커널 응답만 공급하고 Runtime 정책의 결정은 구현에서 얻는다. worker는 Native W 입력에서 직접 실행한다.
struct KernelBoundary {
    RuntimeBoundary& io;bool eventResult=true,threadResult=true,signalResult=true;std::vector<std::uint32_t> waits;std::size_t next{};
    std::function<std::uint32_t()> worker;
    // 같은 사건 목록에 커널/채널 순서를 모은다.
    explicit KernelBoundary(RuntimeBoundary& boundary):io(boundary) {}
    // 고정 핸들을 원본 관찰과 같은 짧은 이름으로 바꾼다.
    std::string Handle(MusicRuntimeHandle h) const { return h==kEvent ? "E" : h==kThread ? "T" : std::to_string(h); }
    // 생성/대기/신호/중단/닫기는 실제 커널 대신 명시 입력으로만 공급한다.
    MusicRuntimeHooks Hooks() {
        MusicRuntimeHooks h;
        h.section=[this](std::uint32_t index,bool initialize) { io.events.push_back(std::string(initialize ? "I:" : "D:")+std::to_string(index)); }; // raw 잠금 공간은 덮지 않는다.
        h.createEvent=[this] { io.events.push_back(std::string("A:")+(eventResult ? "1" : "0"));return eventResult ? kEvent : 0; }; // 수동 리셋 이벤트 생성 응답이다.
        h.createThread=[this](std::function<std::uint32_t()> callback,std::uint32_t& id) {
            worker=std::move(callback);id=77;io.events.push_back(std::string("T:77:")+(threadResult ? "1" : "0"));return threadResult ? kThread : 0;
        }; // 실제 정책/채널 전이를 이 콜백에서 대신 수행하지 않는다.
        h.wait=[this](MusicRuntimeHandle handle,std::uint32_t ms) {
            if (next>=waits.size()) throw std::logic_error("대기 응답 소진");const auto result=waits[next++];io.events.push_back("W:"+Handle(handle)+':'+std::to_string(ms)+':'+std::to_string(result));return result;
        }; // 초기 0ms/종료 5000ms/worker 500ms 요청을 구별한다.
        h.signal=[this](MusicRuntimeHandle handle) { io.events.push_back("Z:"+Handle(handle)+':'+(signalResult ? "1" : "0"));return signalResult; }; // 실패/성공 입력이다.
        h.terminate=[this](MusicRuntimeHandle handle) { io.events.push_back("J:"+Handle(handle)); }; // 원본 강제 중단 위치의 관찰만 남긴다.
        h.close=[this](MusicRuntimeHandle handle) { io.events.push_back("O:"+Handle(handle));if (handle==kThread) worker={}; }; // 실제 worker는 종료하지 않는다.
        h.log=[this](std::uint32_t channel,std::string_view text) { io.Log(channel,text); }; // 원본 진단 문장이다.
        h.report=[this](std::string_view,int line) { io.events.push_back("!"+std::to_string(line)); };return h; // 예상한 생성 실패 보고 후 계속한다.
    }
};
// 전체 raw192·출력·핸들·준비/파일 상태를 원본 관찰과 비교한다.
bool ReplayRow(const std::vector<std::string>& row) {
    CHECK(row.size()==12);const auto fields=Split(row[1],',');CHECK(fields.size()==13);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    RuntimeBoundary io;io.status=Number(fields[11]);io.failed=std::stoi(fields[12]);MusicChannelState a,b;Seed(a,io);
    a.SetField(MusicField::Flags,Number(fields[3]));a.SetField(MusicField::Buffer,Number(fields[4]) ? kBuffer : 0);
    // 두 번째 채널은 사용하지 않은 필드/패딩까지 보존 또는 초기화되는지 확인한다.
    for (std::size_t i=0;i<b.Raw().size();++i) b.Raw()[i]=static_cast<std::uint8_t>(i*11+5);
    SoundState sound;sound.musicInitialized=Number(fields[1])!=0;sound.initialized=Number(fields[2])!=0;sound.device=true;sound.musicVolume=-777;
    SoundList list(edition);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel primary(edition,a,io.ChannelHooks()),secondary(edition,b,io.ChannelHooks());SoundMusic music(edition,sound,effects,primary);
    MusicRuntimeState state{Number(fields[6]) ? kThread : 0,Number(fields[5]) ? kEvent : 0,91};KernelBoundary kernel(io);
    kernel.eventResult=Number(fields[7])!=0;kernel.threadResult=Number(fields[8])!=0;kernel.signalResult=Number(fields[9])!=0;
    // 외부 대기 결과열만 입력하고 종료 재시도 횟수는 계산하지 않는다.
    for (const auto& value:Split(fields[10],'|')) kernel.waits.push_back(Number(value));
    MusicRuntime runtime(sound,state,primary,secondary,music,io.BufferHooks(),kernel.Hooks());std::string result="-";
    if (fields[0]=="I") runtime.Initialize();else if (fields[0]=="D") runtime.Shutdown();else if (fields[0]=="W") result=std::to_string(runtime.RunWorker());
    else { runtime.Initialize();runtime.Initialize();runtime.Shutdown();runtime.Shutdown(); }
    CHECK(io.lockDepth==0);CHECK(!io.locked);CHECK(!runtime.WorkerFailure());
    const bool same=io.Events()==row[2] && Hex(a.Raw())+Hex(b.Raw())==row[3] && Hex(io.output)==row[4] && sound.musicInitialized==(Number(row[5])!=0) &&
        state.thread==Number(row[6]) && state.shutdown==Number(row[7]) && state.threadId==Number(row[8]) && io.position==Number(row[9]) && io.opened==(Number(row[10])!=0) && result==row[11];
    if (!same) std::printf("음악 Runtime 불일치 %s %s\n사건 기대 %s\n사건 실제 %s\nraw 기대 %s\nraw 실제 %s\n출력 기대 %s\n출력 실제 %s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),io.Events().c_str(),row[3].c_str(),(Hex(a.Raw())+Hex(b.Raw())).c_str(),row[4].c_str(),Hex(io.output).c_str());
    // 직접 worker 검사에서 음악 준비만 배치한 상태는 실제 스레드를 소유하지 않는다.
    if (fields[0]=="W") sound.musicInitialized=false;
    return same;
}
// 지정 판본의 독립 입력 전체를 검사한다.
void Replay(const char* edition) {
    std::size_t count=0;
    // 첫 불일치에서 상세 진단을 남기며 전체 행을 대조한다.
    for (const auto& row:Fixture()) { if (row[0]!=edition) continue;const bool same=ReplayRow(row);CHECK(same);++count;if (!same) break; }
    CHECK(count==1600);
}
// 저장소 밖에 실제 읽기 전용 검사 파일을 만든다. 원본 경로는 수정하지 않는다.
struct TemporaryFile {
    std::filesystem::path directory,path;std::vector<std::uint8_t> bytes;
    // 한 링 버퍼보다 긴 stereo16/22050 Hz WAVE를 만든다.
    TemporaryFile():directory(std::filesystem::temp_directory_path()/("NetstormCppMusicRuntime-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),path(directory/"track.mus"),bytes(46+kMusicBufferBytes+32) {
        std::filesystem::create_directories(directory);std::copy_n("RIFF",4,bytes.begin());Put(bytes,4,static_cast<std::uint32_t>(bytes.size()-8));std::copy_n("WAVEfmt ",8,bytes.begin()+8);
        Put(bytes,16,18);Put(bytes,20,1,2);Put(bytes,22,2,2);Put(bytes,24,22050);Put(bytes,28,88200);Put(bytes,32,4,2);Put(bytes,34,16,2);std::copy_n("data",4,bytes.begin()+38);Put(bytes,42,kMusicBufferBytes+32);
        // 실제 worker가 읽을 표본 입력을 채운다.
        for (std::size_t i=46;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>((i-46)*7+11);
        std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!out) throw std::runtime_error("Runtime 검사 WAVE 저장 실패");
    }
    // 이 객체가 만든 파일/디렉터리만 반납한다.
    ~TemporaryFile() { std::error_code ignored;std::filesystem::remove_all(directory,ignored); }
};
// 실제 파일·스레드를 사용하고 COM만 메모리 버퍼로 공급한다. worker 안에서는 검사 전역을 수정하지 않는다.
struct LiveBoundary {
    MusicFileStore files;std::vector<std::uint8_t> buffer=std::vector<std::uint8_t>(kMusicBufferBytes);
    std::atomic<unsigned> plays{0},releases{0};std::uint32_t status{};std::mutex notifyMutex;std::condition_variable notify;bool locked=false,throwStatus=false;
    // 실제 파일 IO와 의미 있는 COM 수명만 연결한다.
    MusicChannelHooks ChannelHooks() {
        MusicChannelHooks h;h.setVolume=[](SoundBuffer,std::int32_t) { return 0; }; // 음량 변경/worker는 같은 명령 잠금 아래 호출한다.
        h.stop=[this](SoundBuffer) { status=0; }; // 메모리 버퍼의 재생 상태만 멈춘다.
        h.release=[this](SoundBuffer) { ++releases;return 0; };return files.FileHooks(h); // 파일은 실제 Winmm 소유자가 닫는다.
    }
    // 파일 입력 경계는 실제 디스크 resolver로 구성한다.
    MusicOpenHooks OpenHooks(const TemporaryFile& temporary) { return files.OpenHooks(MakeDiskSoundResolver(temporary.directory)); }
    // 첫 링 채우기/Play를 실제 Update가 수행하도록 COM 응답만 공급한다.
    MusicBufferHooks BufferHooks() {
        MusicBufferHooks h;h.create=[this](std::span<const std::uint8_t>,std::uint32_t,std::uint32_t,SoundBuffer& token) { status=0;token=kBuffer;return 0; }; // 새 버퍼는 재생 전 상태다.
        h.status=[this](SoundBuffer,std::uint32_t& value) { if (throwStatus) throw std::runtime_error("worker 상태 조회 예외 입력");value=status;return 0; }; // 재생 상태는 Play/Stop이 바꾸며 모든 접근은 음악 명령 잠금 아래다.
        h.restore=[](SoundBuffer) { return 0; }; // 이 입력의 버퍼는 손실되지 않는다.
        h.cursor=[](SoundBuffer,std::uint32_t& play,std::uint32_t& write) { play=write=0;return 0; }; // 첫 채우기에 쓰이지 않는다.
        h.lock=[this](SoundBuffer,std::uint32_t offset,std::uint32_t count,MusicBufferRegions& regions) {
            if (locked || offset!=0 || count!=buffer.size()) throw std::runtime_error("worker 잠금 입력 오류");locked=true;regions={buffer,{}};return 0;
        }; // Update의 실제 읽기 대상이다.
        h.unlock=[this](SoundBuffer,MusicBufferRegions) { if (!locked) throw std::runtime_error("worker 잠금 해제 오류");locked=false;return 0; }; // Stop/종료 이전에 해제한다.
        h.play=[this](SoundBuffer,std::uint32_t flags) {
            if (flags!=1) throw std::runtime_error("worker 재생 flags 오류");status=1;{ std::lock_guard guard(notifyMutex);++plays; }notify.notify_all();return 0;
        };return h; // 실제 스레드 검사의 완료 신호다.
    }
    // 벽시계로 충분히 기다리되 테스트를 영구 대기시키지 않는다.
    bool WaitForPlay(unsigned count) { std::unique_lock guard(notifyMutex);return notify.wait_for(guard,std::chrono::seconds(3),[&] { return plays.load()>=count; }); }
};
// 진단 예외를 확인한다.
bool Throws(const std::function<void()>& call) { try { call(); } catch (const std::exception&) { return true; }return false; }
}

// 10.78 실제 초기화/종료/worker/Read 명령을 전수 대조한다.
TEST_CASE(SoundMusicRuntime_ReplaysOriginals) { Replay("originals"); }
// CD의 공개 갱신/정지 인라인 경로도 실제 실행 결과와 비교한다.
TEST_CASE(SoundMusicRuntime_ReplaysCd) { Replay("originalCD"); }
// 추가 PE의 별도 관찰도 확인한다.
TEST_CASE(SoundMusicRuntime_ReplaysExtra1037) { Replay("original1037"); }
// 실제 Windows 스레드/이벤트·Winmm 파일을 연결해 첫 채우기/정상 종료/재시작을 반복한다. 장치/창은 열지 않는다.
TEST_CASE(SoundMusicRuntime_StreamsRealFileOnWin32WorkerAndRestarts) {
    TemporaryFile temporary;LiveBoundary live;MusicChannelState a,b;MusicRuntimeState state;MusicSelectionState selected;SoundState sound;sound.initialized=sound.device=true;
    SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel primary(OriginalEdition::Patch1078,a,live.ChannelHooks()),secondary(OriginalEdition::Patch1078,b,live.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,primary);
    MusicRuntime runtime(sound,state,primary,secondary,music,live.BufferHooks(),MakeWin32MusicRuntimeHooks());MusicSelection selection(selected,music,{},live.OpenHooks(temporary),live.BufferHooks());
    // 같은 객체의 수명을 세 번 반복하며 오래된 핸들/파일을 다시 쓰지 않는지 검사한다.
    for (unsigned cycle=1;cycle<=3;++cycle) {
        runtime.Initialize();CHECK(sound.musicInitialized);CHECK(state.thread!=0 && state.shutdown!=0);selection.Select("track.mus");const auto token=a.Field(MusicField::File);
        CHECK(live.WaitForPlay(cycle));
        // 실제 갱신과 음악 음량 명령을 동시에 허용하는 공유 잠금을 확인한다.
        for (int volume=-1000;volume<-900;++volume) music.SetVolume(volume);
        runtime.Shutdown();CHECK(!runtime.WorkerFailure());CHECK(!sound.musicInitialized);CHECK(state.thread==0 && state.shutdown==0 && state.threadId!=0);
        CHECK(!primary.Active());CHECK(live.files.Position(token)==-1);CHECK(!live.locked);CHECK(std::equal(live.buffer.begin(),live.buffer.end(),temporary.bytes.begin()+46));
        CHECK(a.Field(MusicField::Buffer)==0);CHECK(b.Field(MusicField::Buffer)==0);
    }
    CHECK(live.releases==3);
}
// worker 예외가 프로세스를 종료하지 않으며 정상 합류/파일 닫기 뒤에도 진단을 남긴다.
TEST_CASE(SoundMusicRuntime_PreservesWorkerFailureAndCleansUp) {
    TemporaryFile temporary;LiveBoundary live;live.throwStatus=true;MusicChannelState a,b;MusicRuntimeState state;SoundState sound;sound.initialized=sound.device=true;
    SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel primary(OriginalEdition::Patch1078,a,live.ChannelHooks()),secondary(OriginalEdition::Patch1078,b,live.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,primary);
    MusicRuntime runtime(sound,state,primary,secondary,music,live.BufferHooks(),MakeWin32MusicRuntimeHooks());runtime.Initialize();CHECK(music.Play("track.mus",1,{},live.OpenHooks(temporary),live.BufferHooks()));const auto token=a.Field(MusicField::File);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    // 예외가 기록되거나 제한 시간이 지날 때까지 짧게 양보한다. 종료 합류 후에만 상태를 검사한다.
    while (!runtime.WorkerFailure() && std::chrono::steady_clock::now()<deadline) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    runtime.Shutdown();CHECK(runtime.WorkerFailure()!=nullptr);CHECK(live.files.Position(token)==-1);CHECK(!live.locked);CHECK(state.thread==0 && state.shutdown==0);
}
// 종료 대기 중 새 Play를 거부하고 음악 명령 잠금을 놓아 다른 명령/worker가 교착되지 않게 한다.
TEST_CASE(SoundMusicRuntime_RejectsNewPlayWhileWaitingForShutdown) {
    TemporaryFile temporary;LiveBoundary live;MusicChannelState a,b;MusicRuntimeState state;SoundState sound;sound.initialized=sound.device=true;
    SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel primary(OriginalEdition::Patch1078,a,live.ChannelHooks()),secondary(OriginalEdition::Patch1078,b,live.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,primary);
    std::mutex barrier;std::condition_variable notify;bool waiting=false,finish=false;MusicRuntimeHooks hooks;
    hooks.createEvent=[] { return kEvent; };hooks.createThread=[](std::function<std::uint32_t()>,std::uint32_t& id) { id=77;return kThread; }; // 이 검사는 worker 대신 종료 대기 경계를 제어한다.
    hooks.signal=[](MusicRuntimeHandle) { return true; };hooks.terminate=[](MusicRuntimeHandle) {};hooks.close=[](MusicRuntimeHandle) {}; // fake 핸들은 OS를 호출하지 않는다.
    hooks.wait=[&](MusicRuntimeHandle,std::uint32_t ms) {
        if (!ms) return kMusicWaitTimeout;std::unique_lock guard(barrier);waiting=true;notify.notify_all();notify.wait(guard,[&] { return finish; });return std::uint32_t{};
    }; // 종료 대기를 결정적으로 멈춰 그 사이 공개 시작을 요청한다.
    MusicRuntime runtime(sound,state,primary,secondary,music,live.BufferHooks(),hooks);runtime.Initialize();CHECK(music.Play("track.mus",1,{},live.OpenHooks(temporary),live.BufferHooks()));
    std::thread stopping([&] { runtime.Shutdown(); }); // 다른 호출 스레드가 종료 대기 중 음악 명령 잠금을 소유하지 않는다.
    { std::unique_lock guard(barrier);CHECK(notify.wait_for(guard,std::chrono::seconds(3),[&] { return waiting; })); }
    CHECK(!music.Play("track.mus",1,{},live.OpenHooks(temporary),live.BufferHooks()));music.SetVolume(-1500);
    { std::lock_guard guard(barrier);finish=true; }notify.notify_all();stopping.join();CHECK(!sound.musicInitialized);CHECK(state.thread==0 && state.shutdown==0);CHECK(live.releases==1);
}
// 여러 음악 파일의 열기/읽기/닫기가 같은 토큰 표에서 충돌하거나 만료 핸들을 다시 읽지 않는지 확인한다.
TEST_CASE(SoundMusicRuntime_SerializesWinmmTokenStoreAcrossReaders) {
    TemporaryFile temporary;MusicFileStore files;std::mutex observedMutex;std::set<std::uint32_t> observed;std::atomic<unsigned> errors{0};
    const auto reader=[&] {
        // 두 스레드가 각각 200번 독립 파일 수명을 만들고 반납한다.
        for (int i=0;i<200;++i) {
            MusicFileHeader header;if (!files.Open(temporary.path,header)) { ++errors;continue; }
            { std::lock_guard guard(observedMutex);if (!observed.insert(header.file).second) ++errors; }
            std::array<std::uint8_t,8> sample{};if (files.Read(header.file,sample)!=8 || !std::equal(sample.begin(),sample.end(),temporary.bytes.begin()+46)) ++errors;
            if (files.Seek(header.file,46)!=46 || files.Position(header.file)!=46) ++errors;files.Close(header.file);files.Close(header.file);if (files.Read(header.file,sample)!=-1) ++errors;
        }
    }; // 각 IO 호출은 소유자의 같은 재귀 잠금으로 핸들 조회/사용을 보호한다.
    std::thread first(reader),second(reader);first.join();second.join();CHECK(errors==0);CHECK(observed.size()==400);
}
// 연결 누락/잘못된 공유 상태와 초기화 진단 예외에서 자원 반납을 확인한다.
TEST_CASE(SoundMusicRuntime_ValidatesBindingsAndCleansInitializationException) {
    RuntimeBoundary io;KernelBoundary kernel(io);kernel.eventResult=false;kernel.waits={0};MusicChannelState a,b;MusicRuntimeState state;SoundState sound;sound.initialized=true;
    SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel primary(OriginalEdition::Patch1078,a,io.ChannelHooks()),secondary(OriginalEdition::Patch1078,b,io.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,primary);
    CHECK(Throws([&] { MusicRuntime invalid(sound,state,primary,primary,music,io.BufferHooks(),kernel.Hooks()); }));
    CHECK(Throws([&] { MusicRuntime invalid(sound,state,primary,secondary,music,io.BufferHooks(),{}); }));
    auto hooks=kernel.Hooks();hooks.report=[](std::string_view,int) { throw std::runtime_error("초기화 진단 예외 입력"); }; // 이벤트 실패 보고 중 예외를 입력한다.
    MusicRuntime runtime(sound,state,primary,secondary,music,io.BufferHooks(),hooks);CHECK(Throws([&] { runtime.Initialize(); }));CHECK(!sound.musicInitialized);CHECK(state.thread==0 && state.shutdown==0);CHECK(io.lockDepth==0);
}
