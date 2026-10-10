// 음악 채널/공유 음소거의 독립 기계어 관찰을 재생한다. 실제 장치/파일/스레드를 열지 않는다.
#include "RawSceneSupport.h"
#include "client/SoundMusic.h"
#include <climits>
#include <cstdio>
#include <optional>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 독립 실행기의 버퍼/파일 토큰과 출력 관찰 크기다.
constexpr SoundBuffer kBufferBase=0x16001000;
constexpr std::uint32_t kFile=0x17001000;
constexpr std::size_t kOutputBytes=128;
// 저장한 원본 관찰만 읽는다. 일반 CTest에 원본 PE/Python/장치 실행은 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() { static const auto data=LoadFixture(NETSTORM_MUSICCONTROL_FIXTURE);return data.rows; }
// 부호 있는 값도 DWORD decimal로 적은 fixture의 인자를 읽는다.
std::int32_t Whole(const std::string& text) { return static_cast<std::int32_t>(Number(text)); }
// 독립 메모리 파일/버퍼/잠금 대체다. 상태 전이를 예상하지 않고 실제 모듈이 요청한 IO와 사건을 기록한다.
struct MusicMock {
    std::vector<std::string> events;std::vector<std::uint8_t> file;
    std::size_t position=5;bool opened=true;int cap=-2,seekResult{},releaseResult{},gainResult{},lockDepth{};
    SoundState* state{};
    // 한 번의 COM 호출 재진입으로 공유 깊이/예약 음악을 바꿀 입력이다.
    struct Callback { bool music;std::uint32_t depth,pending; };
    std::optional<Callback> callback;
    // 버퍼의 사건 표기를 원본 실행기처럼 만든다.
    std::string Ordinal(SoundBuffer buffer) const { return "#"+std::to_string((buffer-kBufferBase)/16U); }
    // 음량 COM을 기록하고 명시 재진입 입력이 있으면 해당 버퍼에서 한 번 적용한다.
    std::int32_t Gain(SoundBuffer buffer,std::int32_t volume) {
        events.push_back("V:"+Ordinal(buffer)+':'+std::to_string(volume));
        if (callback && buffer==kBufferBase+(callback->music ? 16U : 0U)) {
            const auto change=*callback;callback.reset();state->volumeHoldDepth=static_cast<std::int32_t>(change.depth);
            state->pendingMusicVolume=static_cast<std::int32_t>(change.pending);
            events.push_back("J:"+std::to_string(state->volumeHoldDepth)+':'+std::to_string(state->pendingMusicVolume));
        }
        return gainResult;
    }
    // 기록 문장의 줄바꿈만 떼어 같은 사건 형식으로 만든다.
    void Log(std::uint32_t channel,std::string_view text) {
        while (!text.empty() && text.back()=='\n') text.remove_suffix(1);
        events.push_back("G:"+std::to_string(channel)+':'+std::string(text));
    }
    // 채널에 전달할 실제 IO 의미의 함수들이다. 실패한 닫기/재닫기도 사건으로 보존한다.
    MusicChannelHooks Hooks() {
        MusicChannelHooks hooks;
        hooks.setVolume=[this](SoundBuffer buffer,std::int32_t volume) { return Gain(buffer,volume); };
        hooks.stop=[this](SoundBuffer buffer) { events.push_back("X:"+Ordinal(buffer)); };
        hooks.release=[this](SoundBuffer buffer) { events.push_back("B:"+Ordinal(buffer));return releaseResult; };
        hooks.seek=[this](std::uint32_t token,std::uint32_t offset) {
            const auto result=opened && token==kFile && seekResult!=-1 ? static_cast<std::int32_t>(offset) : -1;
            events.push_back("S:"+std::to_string(token)+':'+std::to_string(offset)+':'+std::to_string(result));
            if (result!=-1) position=offset;return result;
        };
        hooks.read=[this](std::uint32_t token,std::span<std::uint8_t> target) {
            const auto before=position;std::int32_t result=-1;
            if (opened && token==kFile && cap!=-1) {
                auto size=std::min(target.size(),position>file.size() ? std::size_t{} : file.size()-position);
                if (cap>=0) size=std::min(size,static_cast<std::size_t>(cap));
                if (size!=0) std::copy_n(file.begin()+position,size,target.begin());
                position+=size;result=static_cast<std::int32_t>(size);
            }
            events.push_back("R:"+std::to_string(token)+':'+std::to_string(before)+':'+std::to_string(target.size())+':'+std::to_string(result));return result;
        };
        hooks.close=[this](std::uint32_t token) { events.push_back("C:"+std::to_string(token));opened=false; };
        hooks.log=[this](std::uint32_t channel,std::string_view text) { Log(channel,text); };
        hooks.report=[this](std::string_view,int line) { events.push_back("!"+std::to_string(line)); };
        hooks.lockEvent=[this](bool enter) { lockDepth+=enter ? 1 : -1;CHECK(lockDepth>=0);events.push_back(enter ? "E" : "L"); };
        return hooks;
    }
    // 같은 공유 전역에 연결할 실제 효과음 계층의 경계다. 제어 대조에서 필요한 음량 외 호출은 거부한다.
    SoundDeviceHooks Effects() {
        SoundDeviceHooks hooks;
        hooks.status=[](SoundBuffer)->std::uint32_t { throw std::logic_error("예상하지 않은 상태 조회"); };
        hooks.play=[](SoundBuffer,std::uint32_t) { throw std::logic_error("예상하지 않은 재생"); };
        hooks.setPosition=[](SoundBuffer,std::uint32_t) { throw std::logic_error("예상하지 않은 위치 변경"); };
        hooks.setVolume=[this](SoundBuffer buffer,std::int32_t volume) { return Gain(buffer,volume); };
        hooks.setPan=[](SoundBuffer,std::int32_t)->std::int32_t { throw std::logic_error("예상하지 않은 좌우 변경"); };
        hooks.stop=[](SoundBuffer) { throw std::logic_error("예상하지 않은 효과음 정지"); };
        hooks.duplicate=[](SoundBuffer,SoundBuffer&)->std::int32_t { throw std::logic_error("예상하지 않은 복제"); };
        hooks.load=[](std::string_view)->SoundLoad { throw std::logic_error("예상하지 않은 적재"); };
        return hooks;
    }
    // 사건을 순서대로 꺼내 비운다.
    std::string Take() {
        std::string text;
        // 호출 순서를 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        events.clear();return text.empty() ? "-" : text;
    }
    // 반복 가능한 파일 바이트를 입력으로 공급한다. 결과 버퍼 내용은 실제 Read가 만든다.
    void File(std::size_t length,std::size_t offset,std::size_t tail=0) {
        file.resize(5+length+tail);position=5+offset;
        // 실행기와 같은 파일 입력 바이트다.
        for (std::size_t i=0;i<file.size();++i) file[i]=static_cast<std::uint8_t>(i*7+11);
    }
};
// 실제 모듈·공유 상태를 만들고 원본 입력 설정/함수 호출을 재생한다.
bool ReplayRow(OriginalEdition edition,const std::vector<std::string>& row) {
    CHECK(row.size()==13);MusicMock device;SoundState state;device.state=&state;
    state.initialized=state.device=state.enabled=state.musicInitialized=true;state.masterVolume=-400;state.pendingMasterVolume=123;
    state.musicVolume=-600;state.pendingMusicVolume=222;SoundList list(edition);
    // 예정 assert가 있는 판본에서도 보고 뒤 실행을 계속한다.
    SoundPlayer player(list,state,device.Effects());MusicChannelState channelState;MusicChannel channel(edition,channelState,device.Hooks());
    SoundMusic music(edition,state,player,channel);std::array<std::uint8_t,kOutputBytes> output;output.fill(0xa7);std::string tokens;
    // 입력 연산을 실제 계층에 순서대로 전달한다. 원본 관찰로부터 기대값을 계산하지 않는다.
    for (const auto& operation:Split(row[1],'|')) {
        const auto op=Split(operation,':');std::string shown="-";
        if (op[0]=="seed") {
            list.Initialize();const auto effect=list.Lookup("effect.wav");list.SetField(effect,SoundField::Buffer,kBufferBase);
            list.SetField(effect,SoundField::Attenuation,300);list.SetField(effect,SoundField::Volume,static_cast<std::uint32_t>(-200));
            auto raw=channelState.Raw();
            // 변경하지 않는 패딩/예약 필드의 보존을 볼 수 있도록 전체 상태를 서로 다른 입력 바이트로 채운다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
            channelState.SetField(MusicField::Buffer,kBufferBase+16);channelState.SetField(MusicField::BufferBytes,16);
            channelState.SetField(MusicField::Flags,3);channelState.SetField(MusicField::File,kFile);channelState.SetField(MusicField::Length,16);
            channelState.SetField(MusicField::ReadOffset,0);channelState.SetField(MusicField::WriteOffset,9);channelState.SetField(MusicField::DataOffset,5);
            channelState.SetField(MusicField::StopDepth,0);device.File(16,0);
        }
        else if (op[0]=="g") {
            const auto value=Number(op[2]);
            if (op[1]=="depth") state.volumeHoldDepth=static_cast<std::int32_t>(value);
            else if (op[1]=="musicinit") state.musicInitialized=value!=0;
            else if (op[1]=="initialized") state.initialized=value!=0;
            else throw std::logic_error("알 수 없는 음악 전역");
        }
        else if (op[0]=="f") channelState.SetField(static_cast<MusicField>(Number(op[1])),Number(op[2]));
        else if (op[0]=="mb") channelState.SetField(MusicField::Buffer,Number(op[1]) ? kBufferBase+16 : 0);
        else if (op[0]=="file") {
            channelState.SetField(MusicField::Length,Number(op[1]));channelState.SetField(MusicField::ReadOffset,Number(op[2]));
            device.File(Number(op[1]),Number(op[2]),Number(op[3]));
        }
        else if (op[0]=="cap") device.cap=Whole(op[1]);
        else if (op[0]=="seek") device.seekResult=Whole(op[1]);
        else if (op[0]=="hr") (op[1]=="release" ? device.releaseResult : device.gainResult)=Whole(op[2]);
        else if (op[0]=="cb") device.callback=MusicMock::Callback{op[1]=="music",Number(op[2]),Number(op[3])};
        else if (op[0]=="read") shown=std::to_string(channel.Read(std::span(output).first(Number(op[1]))));
        else if (op[0]=="start") shown=channel.Start(Number(op[1])) ? "1" : "0";
        else if (op[0]=="rewind") shown=channel.Rewind() ? "1" : "0";
        else if (op[0]=="close") channel.Close();
        else if (op[0]=="stop") channel.Stop();
        else if (op[0]=="allstop") music.Stop();
        else if (op[0]=="mv") music.SetVolume(Whole(op[1]));
        else if (op[0]=="master") player.SetMasterVolume(Whole(op[1]));
        else if (op[0]=="push") music.PushMute();
        else if (op[0]=="pop") music.PopMute();
        else throw std::logic_error("알 수 없는 음악 연산");
        CHECK(device.lockDepth==0);if (!tokens.empty()) tokens+='|';tokens+=device.Take()+'='+shown;
    }
    const bool same=tokens==row[2] && Hex(channelState.Raw())==row[3] && Hex(output)==row[4] && Adler(list.Raw())==Number(row[5]) &&
        device.position==Number(row[6]) && (device.opened ? "1" : "0")==row[7] && std::to_string(state.masterVolume)==row[8] &&
        std::to_string(state.pendingMasterVolume)==row[9] && std::to_string(state.volumeHoldDepth)==row[10] &&
        std::to_string(state.musicVolume)==row[11] && std::to_string(state.pendingMusicVolume)==row[12];
    if (!same) std::printf("%s 음악 제어 불일치 %s\n기대 %s\n실제 %s\nraw 기대 %s\nraw 실제 %s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),tokens.c_str(),row[3].c_str(),Hex(channelState.Raw()).c_str());
    return same;
}
// 판본의 모든 독립 관찰을 읽고 첫 불일치에서 멈춘다.
void Replay(const char* edition,std::size_t expected) {
    const auto kind=std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;std::size_t count=0;
    // 같은 판본 행만 실제 모듈에 재생한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;const bool same=ReplayRow(kind,row);CHECK(same);++count;if (!same) break;
    }
    CHECK(count==expected);
}
}

// 10.78의 음악 읽기/시작/정지/공유 음소거 실제 명령 관찰을 대조한다.
TEST_CASE(SoundMusic_ReplaysOriginals) { Replay("originals",1780); }
// CD의 읽기/정지/음소거를 대조한다. 인라인 시작의 단독 입력은 포함하지 않는다.
TEST_CASE(SoundMusic_ReplaysCd) { Replay("originalCD",1756); }
// 추가 10.37 PE의 별도 관찰을 대조한다.
TEST_CASE(SoundMusic_ReplaysExtra1037) { Replay("original1037",1756); }

// 실제 채널을 시작→파일 끝 통과→부분 IO 실패→정지로 연결하며 필드/잠금/출력을 확인한다.
TEST_CASE(SoundMusic_ConnectsStartReadAndStop) {
    MusicMock device;MusicChannelState state;state.SetField(MusicField::Buffer,kBufferBase+16);state.SetField(MusicField::File,kFile);
    state.SetField(MusicField::Length,8);state.SetField(MusicField::DataOffset,5);device.File(8,0);
    MusicChannel channel(OriginalEdition::Patch1078,state,device.Hooks());CHECK(channel.Start(1));device.Take();
    std::array<std::uint8_t,6> target;target.fill(0xa7);
    CHECK(channel.Read(target)==6 && state.Field(MusicField::ReadOffset)==6);
    CHECK(channel.Read(target)==6 && state.Field(MusicField::ReadOffset)==4 && device.position==9);
    CHECK(target[0]==device.file[11] && target[1]==device.file[12] && target[2]==device.file[5]);
    device.cap=2;CHECK(channel.Read(std::span(target).first(4))==0 && !device.opened && state.Field(MusicField::ReadOffset)==4 && state.Field(MusicField::File)==kFile);
    device.Take();state.Raw()[0x3a]=0x5a;channel.Stop();
    CHECK(state.Field(MusicField::Buffer)==0 && state.Field(MusicField::File)==0 && state.Field(MusicField::Length)==0 &&
        state.Field(MusicField::Flags)==2 && state.Raw()[0x3a]==0x5a && device.lockDepth==0);
    const auto events=device.Take();CHECK(events.find("X:#1")<events.find("B:#1") && events.find("B:#1")<events.find("C:"));
}

// 실제 공유 상태에서 두 번 음소거하고 보류 중 음량을 바꾼 뒤 마지막 해제만 장치에 적용한다.
TEST_CASE(SoundMusic_RestoresLatestVolumesAfterNestedMute) {
    MusicMock device;SoundState state;device.state=&state;state.initialized=state.enabled=state.musicInitialized=true;
    state.masterVolume=-400;state.musicVolume=-600;SoundList list(OriginalEdition::Patch1078);list.Initialize();
    const auto effect=list.Lookup("effect.wav");list.SetField(effect,SoundField::Buffer,kBufferBase);list.SetField(effect,SoundField::Volume,100);
    MusicChannelState channelState;channelState.SetField(MusicField::Buffer,kBufferBase+16);
    SoundPlayer effects(list,state,device.Effects());MusicChannel channel(OriginalEdition::Patch1078,channelState,device.Hooks());
    SoundMusic music(OriginalEdition::Patch1078,state,effects,channel);
    music.PushMute();music.PushMute();CHECK(state.volumeHoldDepth==2 && state.masterVolume==-10000 && state.musicVolume==-10000);
    device.Take();effects.SetMasterVolume(-2000);music.SetVolume(-3000);music.PopMute();
    CHECK(state.volumeHoldDepth==1 && state.masterVolume==-10000 && state.musicVolume==-10000 && device.Take()=="-");
    music.PopMute();CHECK(state.volumeHoldDepth==0 && state.masterVolume==-2000 && state.musicVolume==-3000 && device.Take()=="V:#0:-1900;E;V:#1:-3000;L");
    music.PopMute();CHECK(device.Take()=="-");
}

// 잘못된 경계/범위는 거부하며 예외가 나도 재귀 잠금 균형을 회복한다.
TEST_CASE(SoundMusic_RejectsInvalidUseAndReleasesLocksOnExceptions) {
    MusicMock device;MusicChannelState state;auto broken=device.Hooks();broken.read={};
    CHECK(Throws([&] { MusicChannel channel(OriginalEdition::Patch1078,state,broken); }));
    CHECK(Throws([&] { state.Field(static_cast<MusicField>(kMusicChannelBytes)); }));
    MusicChannel channel(OriginalEdition::Patch1078,state,device.Hooks());state.SetField(MusicField::ReadOffset,1);
    std::array<std::uint8_t,1> target;CHECK(Throws([&] { channel.Read(target); }) && device.lockDepth==0 && device.Take()=="E;L");
    auto hooks=device.Hooks();hooks.read=[](std::uint32_t,std::span<std::uint8_t>)->std::int32_t { throw std::runtime_error("IO 예외"); };
    MusicChannel failing(OriginalEdition::Patch1078,state,std::move(hooks));state.SetField(MusicField::ReadOffset,0);state.SetField(MusicField::Length,8);
    CHECK(Throws([&] { failing.Read(target); }) && device.lockDepth==0 && device.Take()=="E;L");
}
