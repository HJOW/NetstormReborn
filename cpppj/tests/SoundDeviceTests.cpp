// 실제 Winmm 파일 읽기를 독립 PE 관찰과 비교한다. 단위 검사와 자산 전수 검사 모두 소리 장치를 열지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/SoundDevice.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace netstorm::client;
using netstorm::o::OriginalEdition;
// 검사 전용 임시 디렉터리를 소유하며 원본/저장소 파일을 수정하지 않는다.
struct TemporaryFiles {
    std::filesystem::path path;
    // 프로세스 작업과 독립인 새 임시 디렉터리를 만든다.
    TemporaryFiles():path(std::filesystem::temp_directory_path()/("NetstormCppSoundDevice-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::filesystem::create_directories(path);
    }
    // 이 검사에서 만든 디렉터리만 제거한다. 실패 경로에서도 원본 경로를 넘기지 않는다.
    ~TemporaryFiles() { std::error_code ignored;std::filesystem::remove_all(path,ignored); }
    // 검사 입력을 임시 디렉터리에 이진으로 저장한다.
    std::filesystem::path Write(std::string_view name,const std::vector<std::uint8_t>& bytes) const {
        const auto file=path/std::filesystem::path(name);std::filesystem::create_directories(file.parent_path());
        std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
        if (!out) throw std::runtime_error("WAVE 검사 파일 저장 실패");
        return file;
    }
};
// fixture의 16진수 바이트를 읽는다.
std::vector<std::uint8_t> Decode(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    // 두 문자마다 원본 바이트 하나를 복원한다.
    for (std::size_t i=0;i<hex.size();i+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return bytes;
}
// 작은 WAVE 검사용 리틀 엔디언 정수를 쓴다.
void Put(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value,std::size_t size=4) {
    // 낮은 바이트부터 기록한다.
    for (std::size_t i=0;i<size;++i) bytes.at(offset+i)=static_cast<std::uint8_t>(value>>(i*8));
}
// 16바이트 PCM fmt와 두 표본을 가진 합성 파일이다. bits 보정/손상 입력 검사에 쓴다.
std::vector<std::uint8_t> Wave() {
    std::vector<std::uint8_t> bytes(46);const std::string riff="RIFF",wave="WAVEfmt ",data="data";
    std::copy(riff.begin(),riff.end(),bytes.begin());std::copy(wave.begin(),wave.end(),bytes.begin()+8);std::copy(data.begin(),data.end(),bytes.begin()+36);
    Put(bytes,4,38);Put(bytes,16,16);Put(bytes,20,1,2);Put(bytes,22,1,2);Put(bytes,24,22050);Put(bytes,28,22050);Put(bytes,32,1,2);Put(bytes,34,8,2);Put(bytes,40,2);
    bytes[44]=0x81;bytes[45]=0x7f;return bytes;
}
// 오프라인 경계의 거부 예외를 확인한다.
bool Throws(const std::function<void()>& call) { try { call(); } catch (const std::exception&) { return true; }return false; }
// 세 PE 가운데 지정 판본의 입력만 실제 mmio로 읽고 형식/표본/위치와 실패를 대조한다.
void Replay(const char* edition) {
    std::ifstream input(NETSTORM_SOUNDDEVICE_FIXTURE);CHECK(input.good());TemporaryFiles temporary;std::string line;std::size_t count=0;int assertions=0;
    const auto version=std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    // 독립 PE에서 기록한 각 입력을 그대로 임시 파일로 저장하여 C++ 실제 Winmm 경로를 실행한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string selected,file,format,samples;int success;std::uint32_t offset;
        row>>selected>>file>>success>>format>>offset>>samples;if (selected!=edition) continue;
        CHECK(!row.fail());const auto path=temporary.Write("case.wav",Decode(file));
        const auto actual=ReadSoundWave(path,version,[&](std::string_view,int) { ++assertions; }); // 원본 assert가 나오지 않는 입력이다.
        CHECK(actual.has_value()==(success!=0));
        if (actual) { const auto expected=Decode(format);CHECK(std::equal(expected.begin(),expected.end(),actual->format.begin(),actual->format.end()));CHECK(actual->dataOffset==offset);CHECK(actual->samples==Decode(samples)); }
        ++count;
    }
    CHECK(count==435);CHECK(assertions==0);
}
}

// 패치판 실제 WAVE 세 몸체의 독립 기대값을 재생한다.
TEST_CASE(SoundDevice_ReplaysOriginals) { Replay("originals"); }
// CD판 실제 WAVE 세 몸체의 독립 기대값을 재생한다.
TEST_CASE(SoundDevice_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 관찰을 재생한다.
TEST_CASE(SoundDevice_ReplaysExtra1037) { Replay("original1037"); }

// Winmm이 자른 표본 크기·불완전 fmt 거부·잘못된 분모·확장 코덱과 원본 assert 계약을 확인한다.
TEST_CASE(SoundDevice_RejectsUnsafeWaveAndReportsShortNonPcm) {
    TemporaryFiles temporary;const auto edition=OriginalEdition::Patch1078;
    CHECK(!ReadSoundWave(temporary.path/"missing.wav",edition));
    auto bytes=Wave();bytes.pop_back();const auto truncated=ReadSoundWave(temporary.Write("truncated.wav",bytes),edition);
    CHECK(truncated.has_value());if (truncated) CHECK(truncated->samples==std::vector<std::uint8_t>{0x81}); // Winmm은 data 크기를 파일 끝까지 줄인다.
    bytes=Wave();Put(bytes,22,0,2);CHECK(!ReadSoundWave(temporary.Write("zero.wav",bytes),edition));
    bytes=Wave();Put(bytes,40,0x7fffffff);const auto bounded=ReadSoundWave(temporary.Write("huge.wav",bytes),edition);
    CHECK(bounded.has_value());if (bounded) CHECK(bounded->samples.size()==2); // RIFF 끝으로 줄인 크기만 확보해야 한다.
    bytes=Wave();bytes.resize(28);CHECK(!ReadSoundWave(temporary.Write("truncated-fmt.wav",bytes),edition));
    bytes=Wave();Put(bytes,16,2);CHECK(!ReadSoundWave(temporary.Write("short.wav",bytes),edition));
    bytes=Wave();Put(bytes,20,2,2);int assertions=0;
    const auto nonPcm=ReadSoundWave(temporary.Write("nonpcm.wav",bytes),edition,[&](std::string_view text,int line) {
        CHECK(text=="wfFormat.wFormatTag == WAVE_FORMAT_PCM");CHECK(line==0xeb);++assertions; }); // 짧은 비PCM 보고 뒤 원본처럼 계속 읽는다.
    CHECK(nonPcm.has_value());CHECK(assertions==1);
    bytes=Wave();bytes.insert(bytes.begin()+36,2,0);Put(bytes,4,40);Put(bytes,16,18);Put(bytes,36,22,2);Put(bytes,20,2,2);
    CHECK(!ReadSoundWave(temporary.Write("codec.wav",bytes),edition));
    Put(bytes,20,1,2);const auto pcmExtra=ReadSoundWave(temporary.Write("pcmextra.wav",bytes),edition);
    CHECK(pcmExtra.has_value());if (pcmExtra) CHECK(pcmExtra->format[16]==22); // PCM의 불필요한 cbSize도 원본 헤더 값으로 보존한다.
    // 반복 읽기 뒤 파일을 바꿀 수 있어야 하므로 성공/실패의 모든 mmio 핸들이 반납됐는지도 확인한다.
    CHECK(ReadSoundWave(temporary.Write("codec.wav",Wave()),edition).has_value());
}

// 언어/기본·주/보조·감쇠/일반의 여덟 검색 순서와 첫 일치에서의 종료를 확인한다.
TEST_CASE(SoundDevice_PreservesFileSearchAndAttenuation) {
    std::vector<std::string> visited;
    const std::vector<std::string> expected={"0:sound\\english\\beamOver-*.wav","0:sound\\english\\beamOver*.wav","0:sound\\beamOver-*.wav","0:sound\\beamOver*.wav",
        "1:sound\\english\\beamOver-*.wav","1:sound\\english\\beamOver*.wav","1:sound\\beamOver-*.wav","1:sound\\beamOver*.wav"};
    // 각 위치를 첫 일치로 공급하여 앞선 경로가 빠지거나 뒤 경로가 먼저 쓰이지 않는지 검사한다.
    for (std::size_t chosen=0;chosen<=expected.size();++chosen) {
        visited.clear();const auto file=FindSoundFile("beamOver.wav","sound","english",[&](std::string_view pattern,bool alternate)->std::optional<std::filesystem::path> {
            visited.push_back(std::to_string(alternate)+':'+std::string(pattern));
            if (visited.size()==chosen+1) return "sound/beamOver-1000.wav";
            return {};
        }); // 지정한 위치에서만 디스크 파일이 있는 상태다.
        CHECK(file.has_value()==(chosen<expected.size()));CHECK(visited.size()==std::min(chosen+1,expected.size()));
        CHECK(std::equal(visited.begin(),visited.end(),expected.begin()));
    }
    CHECK(SoundFileAttenuation("sound/beamOver-1000.wav")==1000);CHECK(SoundFileAttenuation("sound/a--200.wav")==-200);
    CHECK(SoundFileAttenuation("sound/a- 17oops.wav")==17);CHECK(SoundFileAttenuation("sound/a.wav")==0);
    CHECK(SoundFileAttenuation("root-with-hyphen/a-1000.wav")==0);
    CHECK(Throws([&] { FindSoundFile("","sound","english",MakeDiskSoundResolver(".")); }));
}

// 실제 FindFirstFile의 대소문자/언어/감쇠 선택과 보조 경로를 읽기 전용으로 확인한다.
TEST_CASE(SoundDevice_FindsActualDiskFiles) {
    TemporaryFiles temporary;temporary.Write("sound/a.wav",Wave());temporary.Write("sound/a-1000.wav",Wave());temporary.Write("sound/english/a-500.wav",Wave());
    const auto primary=MakeDiskSoundResolver(temporary.path);
    CHECK(FindSoundFile("A.wav","sound","english",primary)==temporary.path/"sound/english/a-500.wav");
    CHECK(FindSoundFile("a.wav","sound","korean",primary)==temporary.path/"sound/a-1000.wav");
    const auto secondary=MakeDiskSoundResolver(temporary.path/"absent",temporary.path);
    CHECK(FindSoundFile("a.wav","sound","english",secondary)==temporary.path/"sound/english/a-500.wav");
    CHECK(!FindSoundFile("missing.wav","sound","english",primary));
}

// 생성/경계 획득/무장치 적재/반복 종료/누락 인자는 오디오 장치를 열지 않고 검사한다.
TEST_CASE(SoundDevice_OfflineHooksAndShutdown) {
    SoundList list(OriginalEdition::Patch1078);SoundState state;state.enabled=true;
    CHECK(Throws([&] { SoundDevice invalid(list,state,{}); }));
    SoundDevice device(list,state,MakeDiskSoundResolver("."));auto hooks=device.Hooks();
    CHECK(hooks.status && hooks.play && hooks.setPosition && hooks.setVolume && hooks.setPan && hooks.stop && hooks.duplicate && hooks.load);
    CHECK(!state.initialized && !state.device);CHECK(hooks.load("a.wav").buffer==0);
    SoundBuffer copy=23;CHECK(hooks.duplicate(0,copy)<0);CHECK(copy==23);
    CHECK(Throws([&] { device.Initialize(0); })); // null 창은 LoadLibrary/DirectSoundCreate 전에 거부한다.
    const auto sound=list.Lookup("a.wav");list.SetField(sound,SoundField::Buffer,kSilentSoundBuffer);list.SetField(sound,SoundField::Attenuation,1000);
    const auto duplicate=list.AppendDuplicate(sound,sound,7);const auto bytesBefore=list.Raw();std::vector<std::uint8_t> before(bytesBefore.begin(),bytesBefore.end());
    state.playing=3;device.Shutdown();device.Shutdown();CHECK(!state.initialized && !state.device && state.playing==0 && state.enabled);
    CHECK(list.Field(sound,SoundField::Buffer)==0 && list.Field(duplicate,SoundField::Buffer)==0);
    CHECK(list.Field(sound,SoundField::Attenuation)==1000 && list.Field(sound,SoundField::Next)==duplicate);
    // 버퍼 네 바이트 이외의 이름/사슬/재생 이력은 보존한다.
    for (std::size_t i=0;i<before.size();++i) {
        const auto rootOffset=static_cast<std::size_t>(sound-kSoundListBase),copyOffset=static_cast<std::size_t>(duplicate-kSoundListBase);
        if ((rootOffset<=i && i<rootOffset+4) || (copyOffset<=i && i<copyOffset+4)) continue;
        CHECK(before[i]==list.Raw()[i]);
    }
    CHECK(Throws([&] { hooks.status(7); }));
    SoundPlayer player(list,state,hooks);CHECK(player.Play(sound,0,0,0,0,0)==0);
}

namespace netstorm::test {
// 지정한 원본 디스크 폴더의 모든 WAV를 실제 Winmm으로 읽는다. 장치·창은 열지 않고 원본은 변경하지 않는다.
void InspectSoundFiles(const std::filesystem::path& root) {
    std::size_t count=0,attenuated=0;std::uint64_t samples=0;
    // 하위 언어 디렉터리까지 실제 소리 파일을 전수 검사한다.
    for (const auto& entry:std::filesystem::recursive_directory_iterator(root/"sound")) {
        if (!entry.is_regular_file()) continue;
        auto extension=entry.path().extension().string();
        // 원본 디스크는 대소문자를 구별하지 않으므로 .WAV와 혼합 대소문자도 포함한다.
        for (auto& character:extension) if (character>='A' && character<='Z') character=static_cast<char>(character+'a'-'A');
        if (extension!=".wav") continue;
        const auto wave=client::ReadSoundWave(entry.path(),o::OriginalEdition::Patch1078);
        if (!wave) throw std::runtime_error("원본 WAV 적재 실패: "+entry.path().string());
        const auto source=o::ReadFileBytes(entry.path());
        if (wave->dataOffset+wave->samples.size()>source.size() || !std::equal(wave->samples.begin(),wave->samples.end(),source.begin()+wave->dataOffset))
            throw std::runtime_error("원본 WAV 표본 바이트 불일치");
        ++count;samples+=wave->samples.size();attenuated+=client::SoundFileAttenuation(entry.path().string())!=0;
    }
    if (count==0) throw std::runtime_error("검사할 원본 WAV 파일이 없습니다");
    std::printf("Sound files: %zu WAV, %llu sample bytes, %zu attenuated files; no sound device opened\n",count,static_cast<unsigned long long>(samples),attenuated);
}

// 명시적인 --inspect-sound-device 전용 검사다. 숨긴 창과 실제 오디오 장치를 열되 음량은 -10000으로 고정한다.
// 기본 CTest/파일 검사는 호출하지 않는다. 초기화·적재·복제·재생/정지·종료/재초기화의 COM 실행을 확인한다.
void InspectSoundDevice(const std::filesystem::path& root) {
    // DirectSound 협조 수준에 필요한 숨긴 창을 소유한다. ShowWindow/포커스 변경은 하지 않는다.
    struct CooperativeWindow {
        HWND handle{};
        // 시스템 STATIC 클래스를 사용하므로 별도 클래스 등록/메시지 루프는 필요 없다.
        CooperativeWindow():handle(CreateWindowExW(0,L"STATIC",L"NetstormCpp sound inspection",WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)) {
            if (!handle) throw std::runtime_error("소리 검사 창 생성 실패");
        }
        // 장치가 먼저 종료된 뒤 창을 제거한다.
        ~CooperativeWindow() { DestroyWindow(handle); }
    } window;
    client::SoundList list(o::OriginalEdition::Patch1078);client::SoundState state;state.enabled=true;state.masterVolume=client::kSoundMinimum;
    client::SoundDevice device(list,state,client::MakeDiskSoundResolver(root));
    const auto nativeWindow=reinterpret_cast<std::uintptr_t>(window.handle);
    if (!device.Initialize(nativeWindow,3)) throw std::runtime_error("DirectSound 초기화 실패");
    auto hooks=device.Hooks();client::SoundPlayer player(list,state,hooks);const auto sound=list.Lookup("priestForceField.wav");
    const auto playing=player.Play(sound,1,0,0,1,0),copy=player.Play(sound,1,0,1000,1,0);
    if (playing==0 || copy==0 || playing==copy || !player.IsPlaying(playing) || !player.IsPlaying(copy)) throw std::runtime_error("실제 버퍼 적재/복제/반복 재생 실패");
    player.Stop(playing);player.Stop(copy);
    if (player.IsPlaying(playing) || player.IsPlaying(copy) || state.playing!=0) throw std::runtime_error("실제 버퍼 정지 실패");
    device.Shutdown();
    if (!device.Initialize(nativeWindow,0)) throw std::runtime_error("DirectSound 재초기화 실패");
    // Shutdown이 비운 기존 복제 항목을 새 원본 버퍼에서 복제해 복원하는 경로를 확인한다.
    if (player.Play(copy,1,0,0,1,0)!=copy || !player.IsPlaying(copy)) throw std::runtime_error("재초기화 뒤 복제 항목 복원 실패");
    player.Stop(copy);device.Shutdown();device.Shutdown();
    std::printf("DirectSound device: initialize/load/duplicate/play/stop/shutdown/reinitialize passed; volume -10000\n");
}
}
