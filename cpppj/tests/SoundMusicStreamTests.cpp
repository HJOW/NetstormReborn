// 실제 음악 갱신/생성의 독립 관찰과 채널·파일·두 버퍼 구간 연결을 검사한다. 장치/작업 스레드는 열지 않는다.
#include "RawSceneSupport.h"
#include "client/SoundDevice.h"
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원본 실행기의 음악 버퍼/파일 토큰과 HRESULT 실패 입력이다.
constexpr SoundBuffer kBuffer=0x16001010;
constexpr std::uint32_t kFile=0x17001000;
constexpr std::int32_t kFail=static_cast<std::int32_t>(0x80004005U);
// 원본 관찰을 한 번만 읽는다. 일반 검사는 PE/Python 없이 저장 결과만 재생한다.
const std::vector<std::vector<std::string>>& Fixture() { static const auto data=LoadFixture(NETSTORM_MUSICSTREAM_FIXTURE);return data.rows; }
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
struct StreamMock {
    std::vector<std::uint8_t> file;std::array<std::uint8_t,128> output;std::vector<std::string> events;
    std::size_t position=5;bool opened=true,locked=false,readThrows=false,badRegions=false;
    int cap=-2,seekFailure{},failed{},split=-1,lockDepth{};std::uint32_t status{},cursor{};
    std::int32_t createResult{};bool createWrites=true;
    // 출력 전역은 원본 실행기와 같은 감시 바이트로 시작한다.
    StreamMock() { output.fill(0xa7); }
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
        hooks.close=[this](std::uint32_t token) { events.push_back("C:"+std::to_string(token));opened=false; }; // 원본의 재닫기/0 토큰도 기록한다.
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
void Seed(MusicChannelState& state,StreamMock& mock) {
    auto raw=state.Raw();
    // 변경하지 않는 모든 바이트의 보존을 관찰한다.
    for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
    state.SetField(MusicField::Buffer,kBuffer);state.SetField(MusicField::BufferBytes,16);state.SetField(MusicField::Flags,3);
    state.SetField(MusicField::File,kFile);state.SetField(MusicField::Length,16);state.SetField(MusicField::ReadOffset,0);
    state.SetField(MusicField::WriteOffset,9);state.SetField(MusicField::DataOffset,5);state.SetField(MusicField::StopDepth,0);mock.File(16,0);
}
// 한 독립 입력을 실제 채널/공유 계층에 전달하고 사건/반환/상태/출력/파일을 대조한다.
bool ReplayRow(const std::vector<std::string>& row) {
    CHECK(row.size()==8);const bool patch=row[0]=="originals";const auto edition=patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    StreamMock mock;SoundState sound;sound.musicInitialized=true;sound.musicVolume=-777;SoundList list(edition);SoundPlayer effects(list,sound,UnusedEffects());
    MusicChannelState state;Seed(state,mock);MusicChannel channel(edition,state,mock.ChannelHooks());SoundMusic music(edition,sound,effects,channel);
    const auto fields=Split(row[1].substr(2),',');const bool fill=row[1].front()=='F';
    if (fill) {
        state.SetField(MusicField::Flags,Number(fields[0]));sound.musicInitialized=Number(fields[1])!=0;mock.status=Number(fields[2]);mock.cursor=Number(fields[3]);
        state.SetField(MusicField::WriteOffset,Number(fields[4]));state.SetField(MusicField::Length,Number(fields[5]));state.SetField(MusicField::ReadOffset,Number(fields[6]));
        mock.File(Number(fields[5]),Number(fields[6]));mock.cap=std::stoi(fields[7]);mock.seekFailure=std::stoi(fields[8]);mock.failed=std::stoi(fields[9]);mock.split=std::stoi(fields[10]);
    } else { state.SetField(MusicField::Buffer,Number(fields[0]) ? kBuffer : 0);mock.createResult=static_cast<std::int32_t>(Number(fields[1]));mock.createWrites=Number(fields[2])!=0; }
    const auto hooks=mock.BufferHooks();const bool result=fill ? (patch ? channel.FillBuffer(hooks,sound) : music.Update(hooks)) : channel.EnsureBuffer(hooks);
    CHECK(mock.lockDepth==0);CHECK(!mock.locked);
    const bool same=mock.Events()==row[2] && (patch ? (result ? "1" : "0") : "-")==row[3] && Hex(state.Raw())==row[4] && Hex(mock.output)==row[5] && mock.position==Number(row[6]) && (mock.opened ? "1" : "0")==row[7];
    if (!same) std::printf("음악 갱신 불일치 %s %s\n사건 기대 %s\n사건 실제 %s\nraw 기대 %s\nraw 실제 %s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),mock.Events().c_str(),row[4].c_str(),Hex(state.Raw()).c_str());return same;
}
// 해당 판본의 고정 입력 전체를 대조한다.
void Replay(const char* edition,std::size_t expected) {
    std::size_t count=0;
    // 독립 관찰 행을 재생하고 첫 실패에서 상세 사건을 남긴다.
    for (const auto& row:Fixture()) { if (row[0]!=edition) continue;const bool same=ReplayRow(row);CHECK(same);++count;if (!same) break; }
    CHECK(count==expected);
}
// 진단 예외를 요구하는 호스트 경계를 확인한다.
bool Throws(const std::function<void()>& call) { try { call(); } catch (const std::exception&) { return true; }return false; }
}

// 패치 음악 버퍼 생성/갱신의 실제 명령 관찰이다.
TEST_CASE(SoundMusicStream_ReplaysOriginals) { Replay("originals",1704); }
// CD 공개 갱신은 음악 초기화 검사도 포함한다. 반환값은 void이므로 비교하지 않는다.
TEST_CASE(SoundMusicStream_ReplaysCd) { Replay("originalCD",1692); }
// 추가 10.37도 별도 원본 관찰로 검사한다.
TEST_CASE(SoundMusicStream_ReplaysExtra1037) { Replay("original1037",1692); }
// 첫 채우기/재생 뒤 재생 커서가 링 끝을 넘어가면 실제 Read가 두 구간을 채운다.
TEST_CASE(SoundMusicStream_ConnectsStartFillWrapAndShortReadStop) {
    StreamMock mock;MusicChannelState state;Seed(state,mock);state.SetField(MusicField::Length,64);mock.File(64,0);
    MusicChannel channel(OriginalEdition::Patch1078,state,mock.ChannelHooks());SoundState sound;sound.musicVolume=-900;auto hooks=mock.BufferHooks();
    CHECK(channel.Start(1));CHECK(channel.FillBuffer(hooks,sound));CHECK(state.Field(MusicField::WriteOffset)==0);CHECK(state.Field(MusicField::ReadOffset)==16);
    CHECK(mock.Events().find("P:#1:1:0")!=std::string::npos);mock.events.clear();mock.status=1;mock.cursor=12;
    CHECK(channel.FillBuffer(hooks,sound));CHECK(state.Field(MusicField::WriteOffset)==12);mock.events.clear();mock.cursor=3;mock.split=4;
    CHECK(channel.FillBuffer(hooks,sound));CHECK(state.Field(MusicField::WriteOffset)==3);CHECK(state.Field(MusicField::ReadOffset)==35);
    CHECK(mock.output[0]==mock.file[33]);CHECK(mock.output[64]==mock.file[37]);CHECK(mock.Events().find("K:#1:12:7:4:3:0")!=std::string::npos);
    mock.events.clear();mock.cap=2;mock.cursor=8;CHECK(!channel.FillBuffer(hooks,sound));CHECK(!mock.opened);CHECK(!mock.locked);CHECK(mock.lockDepth==0);
    CHECK(state.Field(MusicField::Buffer)==0);CHECK(state.Field(MusicField::Flags)==2);CHECK(mock.Events().find("U:#1:")<mock.Events().find("X:#1"));
}
// 호스트 IO 예외와 손상된 Lock 출력에서도 COM 잠금/재귀 잠금을 반납한다.
TEST_CASE(SoundMusicStream_ReleasesLocksOnInvalidRegionsAndIoExceptions) {
    StreamMock mock;MusicChannelState state;Seed(state,mock);MusicChannel channel(OriginalEdition::Patch1078,state,mock.ChannelHooks());SoundState sound;
    mock.readThrows=true;CHECK(Throws([&] { channel.FillBuffer(mock.BufferHooks(),sound); }));CHECK(!mock.locked);CHECK(mock.lockDepth==0);
    mock.readThrows=false;mock.badRegions=true;CHECK(Throws([&] { channel.FillBuffer(mock.BufferHooks(),sound); }));CHECK(!mock.locked);CHECK(mock.lockDepth==0);
    mock.badRegions=false;state.SetField(MusicField::WriteOffset,16);mock.status=1;CHECK(Throws([&] { channel.FillBuffer(mock.BufferHooks(),sound); }));CHECK(mock.lockDepth==0);
}
// 비활성/미준비는 COM 없이 반환하고 기존 버퍼는 create 경계가 없어도 보존한다.
TEST_CASE(SoundMusicStream_SkipsInactiveUninitializedAndExistingBuffer) {
    StreamMock mock;MusicChannelState state;Seed(state,mock);MusicChannel channel(OriginalEdition::Cd1072,state,mock.ChannelHooks());SoundState sound;
    SoundList list(OriginalEdition::Cd1072);SoundPlayer effects(list,sound,UnusedEffects());SoundMusic music(OriginalEdition::Cd1072,sound,effects,channel);
    CHECK(!music.Update({}));CHECK(mock.events.empty());state.SetField(MusicField::Flags,2);CHECK(!channel.FillBuffer({},sound));CHECK(mock.Events()=="E;L");
    mock.events.clear();CHECK(channel.EnsureBuffer({}));CHECK(state.Field(MusicField::BufferBytes)==16);CHECK(mock.Events()=="E;L");
    state.SetField(MusicField::Flags,3);CHECK(Throws([&] { channel.FillBuffer({},sound); }));CHECK(mock.lockDepth==0);
}
// 실제 장치 어댑터는 생성자/경계 준비만으로 DLL/장치를 열지 않는다. 장치 없는 생성 실패와 파일 경계 보존을 검사한다.
TEST_CASE(SoundMusicStream_BindsUnopenedDeviceWithoutOpeningResources) {
    SoundList list(OriginalEdition::Patch1078);SoundState sound;SoundDevice device(list,sound,[](std::string_view,bool)->std::optional<std::filesystem::path> { return {}; });
    StreamMock mock;MusicChannelState state;Seed(state,mock);state.SetField(MusicField::Buffer,0);
    MusicChannel channel(OriginalEdition::Patch1078,state,device.BindMusicBuffers(mock.ChannelHooks()));CHECK(!channel.EnsureBuffer(device.MusicBuffers()));
    CHECK(state.Field(MusicField::Buffer)==0);CHECK(state.Field(MusicField::BufferBytes)==16);CHECK(!sound.initialized && !sound.device);
    CHECK(channel.Rewind());CHECK(mock.position==5);CHECK(mock.Events().find("Failed to create a music buffer.")!=std::string::npos);
    CHECK(mock.lockDepth==0);
}
