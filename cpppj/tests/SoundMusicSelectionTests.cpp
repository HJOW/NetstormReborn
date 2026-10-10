// 실제 상위 곡 선택/존재 검사/옵션·demo 재선택을 PE 관찰과 대조한다. 파일은 Winmm로 열되 장치는 열지 않는다.
#include "RawSceneSupport.h"
#include "client/SoundMusicFile.h"
#include "client/SoundMusicSelection.h"
#include <chrono>
#include <set>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원본 실행기에 공급한 COM 버퍼 토큰이다.
constexpr SoundBuffer kBuffer=0x16001010;
// 저장한 원본 관찰만 읽는다. C++ 검사는 PE/Python/게임을 실행하지 않는다.
const std::vector<std::vector<std::string>>& Fixture() { static const auto data=LoadFixture(NETSTORM_MUSICSELECTION_FIXTURE);return data.rows; }
// 입력 이름과 경로의 원래 바이트를 복원한다.
std::string Decode(std::string_view hex) {
    std::string result;
    // 두 문자마다 한 바이트를 복원한다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(i,2)),nullptr,16)));
    return result;
}
// 원본의 null 경로와 빈 문자열을 구별한다.
std::optional<std::string> Text(const std::string& hex) { return hex=="-" ? std::nullopt : std::optional<std::string>(Decode(hex)); }
// 사건의 경로 문자열을 같은 소문자 hex로 기록한다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 독립 실행기에 공급한 유효 stereo16/22050 Hz RIFF 입력이다. 기대 상태/사건은 계산하지 않는다.
std::vector<std::uint8_t> Wave() {
    std::vector<std::uint8_t> bytes(100);
    std::copy_n("RIFF",4,bytes.begin());Put(bytes,4,92);std::copy_n("WAVEfmt ",8,bytes.begin()+8);Put(bytes,16,18);
    Put(bytes,20,1,2);Put(bytes,22,2,2);Put(bytes,24,22050);Put(bytes,28,88200);Put(bytes,32,4,2);Put(bytes,34,16,2);
    std::copy_n("data",4,bytes.begin()+38);Put(bytes,42,53);
    // data 표본과 홀수 크기의 끝 패딩을 배치한다.
    for (std::size_t i=0;i<53;++i) bytes[46+i]=static_cast<std::uint8_t>(i*7+11);
    return bytes;
}
// 저장소 밖의 검사 전용 RIFF 파일을 소유한다. 원본 경로는 수정하지 않는다.
struct TemporaryFiles {
    std::filesystem::path path,valid,invalid;
    // 충돌하지 않는 디렉터리와 유효/청크 누락 입력을 만든다.
    TemporaryFiles():path(std::filesystem::temp_directory_path()/("NetstormCppMusicSelection-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::filesystem::create_directories(path);valid=Write("valid.mus",Wave());auto bad=Wave();bad.resize(38);Put(bad,4,30);invalid=Write("invalid.mus",bad);
    }
    // 이 객체가 만든 경로만 정리한다.
    ~TemporaryFiles() { std::error_code ignored;std::filesystem::remove_all(path,ignored); }
    // 이 검사에서 소유한 디렉터리에 RIFF 입력을 쓴다.
    std::filesystem::path Write(const std::filesystem::path& name,const std::vector<std::uint8_t>& bytes) const {
        const auto file=path/name;std::filesystem::create_directories(file.parent_path());std::ofstream out(file,std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!out) throw std::runtime_error("음악 선택 검사 파일 저장 실패");return file;
    }
};
// 이 단계에서 호출하지 않는 효과음 경계를 진단 예외로 채운다.
SoundDeviceHooks UnusedEffects() {
    SoundDeviceHooks h;
    h.status=[](SoundBuffer)->std::uint32_t { throw std::logic_error("효과음 조회 호출"); };
    h.play=[](SoundBuffer,std::uint32_t) { throw std::logic_error("효과음 재생 호출"); };
    h.setPosition=[](SoundBuffer,std::uint32_t) { throw std::logic_error("효과음 위치 호출"); };
    h.setVolume=[](SoundBuffer,std::int32_t)->std::int32_t { throw std::logic_error("효과음 음량 호출"); };
    h.setPan=[](SoundBuffer,std::int32_t)->std::int32_t { throw std::logic_error("효과음 좌우 호출"); };
    h.stop=[](SoundBuffer) { throw std::logic_error("효과음 정지 호출"); };
    h.duplicate=[](SoundBuffer,SoundBuffer&)->std::int32_t { throw std::logic_error("효과음 복제 호출"); };
    h.load=[](std::string_view)->SoundLoad { throw std::logic_error("효과음 적재 호출"); };return h;
}
// 파일은 실제 열린 Winmm 소유자다. 조회/COM/실패 응답만 원본 입력으로 공급하며 fallback 본체는 대체하지 않는다.
struct SelectionBoundary {
    MusicFileStore files;const TemporaryFiles& temporary;std::vector<std::string> events;std::set<std::uint32_t> liveFiles;
    int findMode=1,fileMode=0,seekFailure=0,lockDepth=0;std::int32_t createResult=0;
    // 검사 파일의 수명은 이 객체보다 길어야 한다.
    explicit SelectionBoundary(const TemporaryFiles& source):temporary(source) {}
    // 실제 파일 IO와 버퍼 정지/해제·로그·재귀 잠금을 관찰한다.
    MusicChannelHooks ChannelHooks() {
        auto h=files.FileHooks();const auto seek=h.seek;const auto close=h.close;
        h.seek=[this,seek](std::uint32_t token,std::uint32_t offset) {
            const auto result=seekFailure==-1 ? -1 : seek(token,offset);events.push_back("S:"+std::to_string(token)+':'+std::to_string(offset)+':'+std::to_string(result));return result;
        }; // 되감기 실패 입력 또는 실제 seek다.
        h.close=[this,close](std::uint32_t token) { events.push_back("C:"+std::to_string(token));close(token);liveFiles.erase(token); }; // 만료 토큰 재닫기도 기록한다.
        h.setVolume=[](SoundBuffer,std::int32_t) { return 0; }; // 선택/시작에는 음량 호출이 없다.
        h.stop=[this](SoundBuffer token) { CHECK(token==kBuffer);events.push_back("X:#1"); }; // 실제 정지 요청 순서다.
        h.release=[this](SoundBuffer token) { CHECK(token==kBuffer);events.push_back("B:#1");return 0; }; // 참조 해제 응답이다.
        h.log=[this](std::uint32_t channel,std::string_view text) {
            // 원본 문장의 끝 개행만 사건 구분자로 바꾼다.
            while (!text.empty() && text.back()=='\n') text.remove_suffix(1);
            events.push_back("G:"+std::to_string(channel)+':'+std::string(text));
        };
        h.lockEvent=[this](bool entering) { lockDepth+=entering ? 1 : -1;CHECK(lockDepth>=0);events.push_back(entering ? "E" : "L"); };return h; // 원본 잠금 중첩이다.
    }
    // 이름별 조회/실제 RIFF 열기 실패를 공급한다. fallback은 MusicSelection이 실제 옵션 래퍼로 연결한다.
    MusicOpenHooks OpenHooks() {
        MusicOpenHooks h;
        h.find=[this](std::string_view query)->std::optional<std::string> {
            std::string lower(query);std::transform(lower.begin(),lower.end(),lower.begin(),[](char c) { return c>='A' && c<='Z' ? static_cast<char>(c-'A'+'a') : c; }); // ASCII 파일 이름 비교다.
            bool found=findMode!=0 && lower.find("missing.mus")==std::string::npos;
            if (findMode==2) found=found && lower.find("demo.mus")==std::string::npos;
            if (findMode==3) found=found && lower.find("demo.mus")!=std::string::npos;
            events.push_back("F:"+TextHex(query)+':'+(found ? TextHex(query) : "-"));return found ? std::optional<std::string>(query) : std::nullopt;
        };
        h.open=[this](std::string_view name,MusicFileHeader& header) {
            events.push_back("H:"+TextHex(name));std::string lower(name);
            std::transform(lower.begin(),lower.end(),lower.begin(),[](char c) { return c>='A' && c<='Z' ? static_cast<char>(c-'A'+'a') : c; }); // demo 파일 실패 입력을 구별한다.
            const bool fail=fileMode==1 || (fileMode==3 && lower.find("demo.mus")==std::string::npos);
            const bool opened=files.Open(fail ? temporary.path/"missing.mus" : fileMode==2 ? temporary.invalid : temporary.valid,header);
            if (opened) liveFiles.insert(header.file);return opened;
        };return h;
    }
    // 명시 COM 생성 결과만 공급한다. 재생/읽기/선택 결과는 구현에서 얻는다.
    MusicBufferHooks BufferHooks() {
        MusicBufferHooks h;h.create=[this](std::span<const std::uint8_t> format,std::uint32_t flags,std::uint32_t bytes,SoundBuffer& token) {
            token=kBuffer;events.push_back("N:"+std::to_string(flags)+':'+std::to_string(bytes)+':'+Hex(format)+':'+std::to_string(token)+':'+std::to_string(createResult));return createResult;
        };return h;
    }
    // 원본 사건 순서와 같은 구분자로 합친다.
    std::string Events() const {
        std::string joined;
        // 수집한 순서를 바꾸지 않는다.
        for (const auto& event:events) { if (!joined.empty()) joined+=';';joined+=event; }
        return joined.empty() ? "-" : joined;
    }
};
// 파일 토큰 1과 채널/현재 이름의 입력 바이트를 배치한다. 기대 출력은 쓰지 않는다.
void Seed(MusicChannelState& channel,MusicSelectionState& selection,SelectionBoundary& boundary,const std::vector<std::string>& fields) {
    MusicFileHeader header;CHECK(boundary.files.Open(boundary.temporary.valid,header));CHECK(header.file==1);boundary.liveFiles.insert(header.file);CHECK(boundary.files.Seek(header.file,5)==5);
    auto raw=channel.Raw();
    // 쓰지 않는 패딩/필드의 보존도 대조하는 입력 패턴이다.
    for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
    channel.SetField(MusicField::Buffer,Number(fields[7]) ? kBuffer : 0);channel.SetField(MusicField::BufferBytes,16);channel.SetField(MusicField::Flags,Number(fields[6]));
    channel.SetField(MusicField::File,1);channel.SetField(MusicField::Length,16);channel.SetField(MusicField::ReadOffset,0);
    channel.SetField(MusicField::WriteOffset,9);channel.SetField(MusicField::DataOffset,5);channel.SetField(MusicField::StopDepth,0);
    auto name=selection.Raw();std::fill(name.begin(),name.end(),std::uint8_t{0xae});const auto old=Decode(fields[0]);std::copy(old.begin(),old.end(),name.begin());name[old.size()]=0;name[255]=0x7d;
    selection.enabled=static_cast<std::int32_t>(Number(fields[2]));
}
// 세 판본의 동일 입력/관찰을 실제 C++ 옵션→선택→파일→버퍼→되감기 경로와 비교한다.
bool ReplayRow(const std::vector<std::string>& row,const TemporaryFiles& temporary) {
    CHECK(row.size()==7);const auto fields=Split(row[1],',');CHECK(fields.size()==15);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SelectionBoundary boundary(temporary);boundary.findMode=std::stoi(fields[8]);boundary.fileMode=std::stoi(fields[9]);boundary.createResult=static_cast<std::int32_t>(Number(fields[10]));boundary.seekFailure=std::stoi(fields[11]);
    MusicChannelState channelState;MusicSelectionState selectionState;Seed(channelState,selectionState,boundary,fields);
    SoundState sound;sound.musicInitialized=Number(fields[4])!=0;sound.device=Number(fields[5])!=0;
    SoundList list(edition);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel channel(edition,channelState,boundary.ChannelHooks());SoundMusic music(edition,sound,effects,channel);
    MusicSelection selection(selectionState,music,{Text(fields[12]),Text(fields[13])},boundary.OpenHooks(),boundary.BufferHooks());const auto name=Decode(fields[1]);
    if (Number(fields[14])) selection.Select(name.c_str());else selection.Select(name.c_str(),static_cast<std::int32_t>(Number(fields[3])));
    const auto position=boundary.files.Position(channelState.Field(MusicField::File));CHECK(boundary.lockDepth==0);
    const bool same=boundary.Events()==row[2] && Hex(channelState.Raw())==row[3] && Hex(selectionState.Raw())==row[4] && position==std::stoi(row[5]) && boundary.liveFiles.size()==Number(row[6]);
    if (!same) std::printf("음악 선택 불일치 %s %s\n사건 기대 %s\n사건 실제 %s\nraw 기대 %s\nraw 실제 %s\n이름 기대 %s\n이름 실제 %s\n파일 %d 기대 %s, 열린 파일 %zu 기대 %s\n",
        row[0].c_str(),row[1].c_str(),row[2].c_str(),boundary.Events().c_str(),row[3].c_str(),Hex(channelState.Raw()).c_str(),row[4].c_str(),Hex(selectionState.Raw()).c_str(),position,row[5].c_str(),boundary.liveFiles.size(),row[6].c_str());
    return same;
}
// 지정 판본의 전체 독립 관찰을 확인한다.
void Replay(const char* edition) {
    TemporaryFiles temporary;std::size_t count=0;
    // 첫 불일치를 상세 기록하며 전체 행을 순서대로 검사한다.
    for (const auto& row:Fixture()) { if (row[0]!=edition) continue;const bool same=ReplayRow(row,temporary);CHECK(same);++count;if (!same) break; }
    CHECK(count==688);
}
// 호스트 입력의 진단 예외를 관찰한다.
bool Throws(const std::function<void()>& call) { try { call(); } catch (const std::exception&) { return true; }return false; }
}

// 10.78의 실제 옵션/strncpy/존재/상위 fallback까지 대조한다.
TEST_CASE(SoundMusicSelection_ReplaysOriginals) { Replay("originals"); }
// CD 인라인 공개 시작도 실제 상위 선택과 함께 비교한다.
TEST_CASE(SoundMusicSelection_ReplaysCd) { Replay("originalCD"); }
// 추가 PE의 관찰은 별도로 대조한다.
TEST_CASE(SoundMusicSelection_ReplaysExtra1037) { Replay("original1037"); }
// 연속 선택에서 실제 파일 수명, 대소문자 보존, 특수 곡 loop와 없는 파일의 기존 곡 유지 규칙을 확인한다.
TEST_CASE(SoundMusicSelection_SwitchesTracksAndHonorsOption) {
    TemporaryFiles temporary;SelectionBoundary boundary(temporary);MusicChannelState state;MusicSelectionState selected;
    SoundState sound;sound.device=sound.musicInitialized=true;SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());
    MusicChannel channel(OriginalEdition::Patch1078,state,boundary.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,channel);
    MusicSelection selection(selected,music,{{"music"},{"backup"}},boundary.OpenHooks(),boundary.BufferHooks());
    selection.Select("old.mus");const auto old=state.Field(MusicField::File);CHECK(channel.Active());CHECK(state.Field(MusicField::Flags)==3);
    boundary.events.clear();selection.Select("OLD.MUS");CHECK(selected.Current()=="old.mus");CHECK(state.Field(MusicField::File)==old);CHECK(boundary.events.empty());
    selection.Select("FANFARE.MUS");CHECK(selected.Current()=="FANFARE.MUS");CHECK(state.Field(MusicField::Flags)==1);CHECK(boundary.files.Position(old)==-1);
    const auto fanfare=state.Field(MusicField::File);selection.Select("missing.mus");CHECK(selected.Current()=="missing.mus");CHECK(state.Field(MusicField::File)==fanfare);CHECK(channel.Active());
    selection.Select("defeat.mus");CHECK(state.Field(MusicField::Flags)==1);CHECK(boundary.files.Position(fanfare)==-1);
    selected.enabled=0;selection.Select("track.mus");CHECK(selected.Current()=="track.mus");CHECK(!channel.Active());CHECK(boundary.liveFiles.empty());CHECK(boundary.lockDepth==0);
}
// 열기 실패는 실제 옵션 래퍼를 재진입한다. 명시 옵션과 전역 옵션이 다를 때도 demo가 전역 옵션을 따른다.
TEST_CASE(SoundMusicSelection_FallbackReadsGlobalOptionAndTerminates) {
    TemporaryFiles temporary;SelectionBoundary boundary(temporary);boundary.fileMode=3;MusicChannelState state;MusicSelectionState selected;
    SoundState sound;sound.device=sound.musicInitialized=true;SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());
    MusicChannel channel(OriginalEdition::Patch1078,state,boundary.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,channel);
    MusicSelection selection(selected,music,{},boundary.OpenHooks(),boundary.BufferHooks());
    selection.Select("track.mus");CHECK(selected.Current()=="demo.mus");CHECK(channel.Active());CHECK(state.Field(MusicField::Flags)==3);CHECK(boundary.liveFiles.size()==1);
    music.Stop();selected.enabled=0;selection.Select("track.mus",1);CHECK(selected.Current()=="demo.mus");CHECK(!channel.Active());CHECK(boundary.liveFiles.empty());
    selected.enabled=1;boundary.fileMode=1;selection.Select("track.mus");CHECK(selected.Current()=="demo.mus");CHECK(!channel.Active());CHECK(boundary.liveFiles.empty());CHECK(boundary.lockDepth==0);
}
// 실제 디스크 조회를 상위 존재 검사와 공개 열기가 공유하고 보조 경로의 음악도 교체할 수 있다.
TEST_CASE(SoundMusicSelection_ResolvesDiskTracksThroughSecondaryDirectory) {
    TemporaryFiles temporary;temporary.Write("music/old.mus",Wave());temporary.Write("backup/track.mus",Wave());SelectionBoundary boundary(temporary);
    MusicChannelState state;MusicSelectionState selected;SoundState sound;sound.device=sound.musicInitialized=true;SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());
    MusicChannel channel(OriginalEdition::Patch1078,state,boundary.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,channel);
    MusicSelection selection(selected,music,{{"music"},{"backup"}},boundary.files.OpenHooks(MakeDiskSoundResolver(temporary.path)),boundary.BufferHooks());
    selection.Select("old.mus");const auto old=state.Field(MusicField::File);CHECK(channel.Active());selection.Select("track.mus");CHECK(boundary.files.Position(old)==-1);CHECK(channel.Active());
    CHECK(selected.Current()=="track.mus");CHECK(boundary.files.Position(state.Field(MusicField::File))==46);selected.enabled=0;selection.Select("track.mus");CHECK(state.Field(MusicField::File)==0);CHECK(boundary.lockDepth==0);
}
// strncpy의 255바이트 한계/패딩/마지막 바이트와 손상된 호스트 입력의 진단 경계를 확인한다.
TEST_CASE(SoundMusicSelection_PreservesFixedNameStorageAndRejectsUnsafeInputs) {
    MusicSelectionState state;std::fill(state.Raw().begin(),state.Raw().end(),std::uint8_t{0xae});state.Raw()[255]=0x7d;state.Assign("x");CHECK(state.Current()=="x");
    CHECK(std::all_of(state.Raw().begin()+1,state.Raw().end()-1,[](std::uint8_t value) { return value==0; }));CHECK(state.Raw()[255]==0x7d);
    state.Assign(std::string(260,'x'));CHECK(std::all_of(state.Raw().begin(),state.Raw().end()-1,[](std::uint8_t value) { return value=='x'; }));CHECK(state.Raw()[255]==0x7d);CHECK(Throws([&] { state.Current(); }));
    TemporaryFiles temporary;SelectionBoundary boundary(temporary);MusicChannelState channelState;MusicSelectionState selected;SoundState sound;
    SoundList list(OriginalEdition::Patch1078);SoundPlayer effects(list,sound,UnusedEffects());MusicChannel channel(OriginalEdition::Patch1078,channelState,boundary.ChannelHooks());SoundMusic music(OriginalEdition::Patch1078,sound,effects,channel);
    CHECK(Throws([&] { MusicSelection missing(selected,music,{}, {},boundary.BufferHooks()); }));
    MusicSelection selection(selected,music,{},boundary.OpenHooks(),boundary.BufferHooks());CHECK(Throws([&] { selection.Select(nullptr); }));
    std::fill(selected.Raw().begin(),selected.Raw().end(),std::uint8_t{1});CHECK(Throws([&] { selection.Select("track.mus"); }));CHECK(boundary.events.empty());
    CHECK(Throws([&] { FindMusicFile(std::string(256,'x'),{},boundary.OpenHooks().find); }));CHECK(boundary.lockDepth==0);
}
