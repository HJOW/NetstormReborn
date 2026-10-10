// 소리 제어의 독립 기계어 관찰을 실제 모듈에 재생하고 자연 종료·옵션 변경·복제 소리의 경계를 검사한다.
#include "RawSceneSupport.h"
#include "client/Sound.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 판본별 제어 입력 수와 독립 실행기의 가짜 버퍼 값(기준 + 16 × 순번)이다.
constexpr std::size_t kCases=1063;
constexpr SoundBuffer kBufferBase=0x16001000;
// 일반 CTest는 PE/Python을 실행하지 않고 저장한 독립 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_SOUNDCONTROL_FIXTURE);return data.rows;
}
// 원본 실행기의 대체 장치와 같은 규칙의 상태표다: 재생하면 재생 중(반복이면 반복 비트), 정지하면 두 비트를 끈다.
// 호출 사실과 인자는 실행기와 같은 사건 문자열로 적는다. 적재/복제/설정의 결과는 입력으로 정한다.
struct DeviceMock {
    std::vector<std::uint32_t> status;std::vector<std::string> events;
    // 연속 조회 사이의 상태 변화 입력이다. 큐가 비면 마지막 상태를 유지한다.
    std::map<std::size_t,std::vector<std::uint32_t>> queues;
    char loadKind='b';std::int32_t loadAttenuation{},copyResult{},gainResult{},balanceResult{};
    // 새 가짜 버퍼를 만든다. 처음 상태는 멈춤이다.
    SoundBuffer Create() { status.push_back(0);return kBufferBase+16U*static_cast<SoundBuffer>(status.size()-1); }
    // 버퍼 값의 순번이다. 만든 적 없는 값이면 예외다(원본 실행기도 거부한다).
    std::size_t Index(SoundBuffer buffer) const {
        if (buffer<kBufferBase || (buffer-kBufferBase)%16U || (buffer-kBufferBase)/16U>=status.size()) throw std::out_of_range("알 수 없는 버퍼");
        return (buffer-kBufferBase)/16U;
    }
    // 사건에 적는 버퍼 표기다. 순번이 없는 값(0, 소리 없음 표식)은 숫자 그대로 적는다.
    std::string Ordinal(SoundBuffer buffer) const {
        const bool known=buffer>=kBufferBase && (buffer-kBufferBase)%16U==0 && (buffer-kBufferBase)/16U<status.size();
        return known ? "#"+std::to_string((buffer-kBufferBase)/16U) : std::to_string(buffer);
    }
    // 재생 계층에 넘길 장치 경계다.
    SoundDeviceHooks Hooks() {
        SoundDeviceHooks hooks;
        hooks.status=[this](SoundBuffer buffer) {
            const auto index=Index(buffer);auto& queue=queues[index];
            if (!queue.empty()) { status.at(index)=queue.front();queue.erase(queue.begin()); }
            events.push_back("Q:"+Ordinal(buffer));return status.at(index); };
        hooks.play=[this](SoundBuffer buffer,std::uint32_t flags) {
            status.at(Index(buffer))=1U|((flags&1U) ? 4U : 0U);events.push_back("P:"+Ordinal(buffer)+":0:0:"+std::to_string(flags)); };
        hooks.setPosition=[this](SoundBuffer buffer,std::uint32_t position) { events.push_back("C:"+Ordinal(buffer)+':'+std::to_string(position)); };
        hooks.setVolume=[this](SoundBuffer buffer,std::int32_t value) { events.push_back("V:"+Ordinal(buffer)+':'+std::to_string(value));return gainResult; };
        hooks.setPan=[this](SoundBuffer buffer,std::int32_t value) { events.push_back("N:"+Ordinal(buffer)+':'+std::to_string(value));return balanceResult; };
        hooks.stop=[this](SoundBuffer buffer) { status.at(Index(buffer))&=~5U;events.push_back("X:"+Ordinal(buffer)); };
        hooks.duplicate=[this](SoundBuffer original,SoundBuffer& copy) {
            events.push_back("D:"+Ordinal(original));if (copyResult>=0) copy=Create();return copyResult; };
        hooks.load=[this](std::string_view name) {
            SoundLoad loaded{loadKind=='b' ? Create() : loadKind=='s' ? kSilentSoundBuffer : SoundBuffer{},loadAttenuation};
            events.push_back("F:"+std::string(name));return loaded; };
        hooks.log=[this](std::uint32_t channel,std::string_view text) {
            // 실행기는 문장 끝의 줄바꿈을 떼고 적는다.
            while (!text.empty() && (text.back()=='\n' || text.back()==' ')) text.remove_suffix(1);
            events.push_back("G:"+std::to_string(channel)+':'+std::string(text)); };
        return hooks;
    }
    // 보고 경계다. 줄 번호만 적는다.
    SoundAssertReport Report() { return [this](std::string_view,int line) { events.push_back("!"+std::to_string(line)); }; }
    // 한 연산 동안의 사건을 실행기와 같은 형식으로 꺼내고 비운다.
    std::string Take() {
        std::string text;
        // 같은 호출 순서를 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        events.clear();return text.empty() ? "-" : text;
    }
};
// 16진수 두 글자씩을 한 바이트로 되돌린다.
std::string Decode(const std::string& hex) {
    std::string text;
    // 한 바이트씩 읽는다.
    for (std::size_t i=0;i<hex.size();i+=2) text+=static_cast<char>(std::stoul(hex.substr(i,2),nullptr,16));
    return text;
}
// 연산 묶음 한 행: 실행기와 같은 연산을 실제 이름 표/재생 계층에 차례로 적용하고 연산별 사건·반환과 마지막 상태를 비교한다.
bool ReplayScript(OriginalEdition edition,const std::vector<std::string>& row) {
    CHECK(row.size()==11);DeviceMock device;SoundState state;SoundList list(edition,device.Report());
    SoundPlayer player(list,state,device.Hooks(),device.Report());std::vector<std::uint32_t> results;std::string tokens;
    // $k는 k번째 연산의 반환 항목, 그 밖에는 숫자다.
    const auto handle=[&](const std::string& text) { return text[0]=='$' ? results.at(std::stoul(text.substr(1))) : Number(text); };
    // 부호 있는 값도 DWORD 10진수로 적혀 있다.
    const auto whole=[](const std::string& text) { return static_cast<std::int32_t>(Number(text)); };
    // 연산마다 실제 진입 한 번 또는 입력 설정 한 번이다.
    for (const auto& operation:Split(row[2],'|')) {
        const auto op=Split(operation,':');std::uint32_t result=0;std::string shown="-";bool entry=false;
        if (op[0]=="i") { list.Initialize();result=list.Fallback();entry=true; }
        else if (op[0]=="n") { result=list.Lookup(Decode(op[1]));entry=true; }
        else if (op[0]=="g") {
            const auto value=Number(op[2]);
            if (op[1]=="initialized") state.initialized=value!=0;
            else if (op[1]=="device") state.device=value!=0;
            else if (op[1]=="enabled") state.enabled=value!=0;
            else if (op[1]=="swap") state.swapSpeakers=value!=0;
            else if (op[1]=="max") state.maxPlaying=static_cast<std::int32_t>(value);
            else if (op[1]=="playing") state.playing=static_cast<std::int32_t>(value);
            else if (op[1]=="master") state.masterVolume=static_cast<std::int32_t>(value);
            else if (op[1]=="depth") state.volumeHoldDepth=static_cast<std::int32_t>(value);
            else if (op[1]=="pending") state.pendingMasterVolume=static_cast<std::int32_t>(value);
            else throw std::runtime_error("알 수 없는 전역");
        }
        else if (op[0]=="view") { state.view.left=whole(op[1]);state.view.top=whole(op[2]);state.view.right=whole(op[3]);state.view.bottom=whole(op[4]); }
        else if (op[0]=="cam") { state.view.cameraX=whole(op[1]);state.view.cameraY=whole(op[2]); }
        else if (op[0]=="st") device.status.at(Number(op[1]))=Number(op[2]);
        else if (op[0]=="sq") {
            auto& queue=device.queues[Number(op[1])];queue.clear();
            // 순서대로 반환할 장치 상태들을 입력한다.
            for (const auto& value:Split(op[2],',')) queue.push_back(Number(value));
        }
        else if (op[0]=="hr") (op[1]=="copy" ? device.copyResult : op[1]=="gain" ? device.gainResult : device.balanceResult)=whole(op[2]);
        else if (op[0]=="ld") { device.loadKind=op[1][0];device.loadAttenuation=whole(op[2]); }
        else if (op[0]=="clr") list.SetField(handle(op[1]),SoundField::Buffer,0);
        else if (op[0]=="play") { result=player.Play(handle(op[1]),Number(op[2]),whole(op[3]),whole(op[4]),Number(op[5]),whole(op[6]));entry=true; }
        else if (op[0]=="recount") { result=static_cast<std::uint32_t>(player.Recount());shown=std::to_string(static_cast<std::int32_t>(result)); }
        else if (op[0]=="stopname") player.StopByName(Decode(op[1]));
        else if (op[0]=="isname") { result=player.IsPlayingByName(Decode(op[1]));shown=std::to_string(result); }
        else if (op[0]=="switch") {
            const auto name=op[2]=="-" ? std::string{} : Decode(op[2]);
            result=static_cast<std::uint32_t>(player.SetNamePlaying(whole(op[1]),op[2]=="-" ? nullptr : name.c_str(),Number(op[3]),whole(op[4])));
            shown=std::to_string(static_cast<std::int32_t>(result));
        }
        else if (op[0]=="stoploops") player.StopLoops();
        else if (op[0]=="master") player.SetMasterVolume(whole(op[1]));
        else throw std::runtime_error("알 수 없는 연산");
        if (entry) shown=result ? std::to_string(static_cast<std::int32_t>(result-kSoundListBase)) : "null";
        results.push_back(result);
        if (!tokens.empty()) tokens+='|';
        tokens+=device.Take()+'='+shown;
    }
    const bool same=tokens==row[3] && list.FirstFree()-kSoundListBase==Number(row[4]) && Adler(list.Raw())==Number(row[5]) &&
        std::to_string(state.playing)==row[6] && state.serial==Number(row[7]) && std::to_string(state.masterVolume)==row[8] &&
        std::to_string(state.volumeHoldDepth)==row[9] && std::to_string(state.pendingMasterVolume)==row[10];
    if (!same) std::printf("%s 소리 제어: %s\n  기대 %s\n  실제 %s\n",row[0].c_str(),row[2].c_str(),row[3].c_str(),tokens.c_str());
    return same;
}
// 한 판본의 모든 제어 관찰을 대조한다. 결과/사건을 다시 계산하지 않고 원본 저장 관찰과 비교한다.
void Replay(const char* edition) {
    const auto kind=std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    std::size_t count=0;CHECK(Fixture().size()==kCases*3);
    // 첫 불일치에서 멈춰 실패 입력과 호출 순서를 읽을 수 있게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        const bool same=ReplayScript(kind,row);CHECK(same);++count;
        if (!same) break;
    }
    CHECK(count==kCases);
}
}

// 10.78의 여섯 제어 몸체와 내부 실제 재생/조회 관찰을 대조한다.
TEST_CASE(SoundControl_ReplaysOriginals) { Replay("originals"); }
// CD판의 별도 견고성 보고·정지 구조도 원본 관찰로 대조한다.
TEST_CASE(SoundControl_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE 역시 독립 실행 관찰로 대조한다.
TEST_CASE(SoundControl_ReplaysExtra1037) { Replay("original1037"); }

// 한 번 소리가 자연 종료한 뒤 동시 재생 한도를 회복하고, 옵션을 끈 상태에서도 반복 소리만 정지하는 실제 계층 연결이다.
TEST_CASE(SoundControl_RecoversCapacityAndStopsOnlyLoops) {
    DeviceMock device;SoundList list(OriginalEdition::Patch1078);list.Initialize();SoundState state;
    state.initialized=state.device=state.enabled=true;state.maxPlaying=1;
    SoundPlayer player(list,state,device.Hooks());const auto notice=list.Lookup("ourPriestImmobile.wav"),field=list.Lookup("priestForceField.wav");
    CHECK(player.Play(notice,0,0,0,0,0)==notice && state.playing==1);
    // 장치의 자연 종료는 재생 수를 줄이지 않는다. 재계산 전에는 보호막의 일반 재생이 한도에 막힌다.
    device.status[0]=0;device.Take();CHECK(player.Play(field,1,0,0,0,0)==0 && device.Take()=="-");
    CHECK(player.Recount()==0 && device.Take()=="Q:#0");
    CHECK(player.Play(field,1,0,0,0,0)==field && state.playing==1);
    CHECK(player.Play(notice,0,0,0,1,0)==notice && state.playing==2);device.Take();
    const auto before=std::vector<std::uint8_t>(list.Raw().begin(),list.Raw().end());
    // 소리 옵션이 꺼져도 기존 반복 버퍼는 정지한다. 한 번 소리는 유지되고 표의 모든 필드는 그대로다.
    state.initialized=state.enabled=false;player.StopLoops();
    CHECK(device.Take()=="Q:#0;Q:#0;Q:#1;Q:#1;Q:#1;X:#1;C:#1:0" && state.playing==1);
    CHECK(device.status[0]==1 && device.status[1]==0 && std::equal(before.begin(),before.end(),list.Raw().begin()));
    CHECK(player.Recount()==1 && device.Take()=="Q:#0;Q:#1");
}

// 이름 켜기/끄기의 반환은 on 그대로이며, 이름 정지는 복제가 있어도 원본 항목만 정지하는 계약이다.
TEST_CASE(SoundControl_PreservesNameSwitchAndDuplicateContract) {
    DeviceMock device;SoundList list(OriginalEdition::Patch1078);list.Initialize();SoundState state;
    state.initialized=state.device=state.enabled=true;SoundPlayer player(list,state,device.Hooks(),device.Report());
    const auto root=list.Lookup("a.wav");player.Play(root,1,0,0,0,0);
    const auto copy=player.Play(root,1,-100,0,0,0);device.Take();
    // 이미 재생 중이면 새 음량·반복 인자를 적용하지 않고 비영 입력 값을 반환한다.
    CHECK(player.SetNamePlaying(-7,"A.wav",0,-999)==-7 && device.Take()=="Q:#0");
    player.StopByName("A.WAV");CHECK(device.Take()=="!1016;Q:#0;Q:#0;X:#0;C:#0:0");
    CHECK(state.playing==1 && device.status[1]==5 && player.IsPlaying(copy)==1);device.Take();
    // 옵션을 끄면 조회는 새 이름을 등록하지만 켜기 요청은 조회 전에 반환한다. null 역시 조회하지 않는다.
    state.enabled=false;const auto free=list.FirstFree();
    CHECK(player.SetNamePlaying(8,"noLookup.wav",1,0)==0 && player.SetNamePlaying(8,nullptr,1,0)==0 && list.FirstFree()==free);
    CHECK(player.IsPlayingByName("newName.wav")==0 && list.FirstFree()>free && device.Take()=="-");
    state.enabled=true;state.maxPlaying=1;
    CHECK(player.SetNamePlaying(8,"a.wav",0,0)==8 && device.Take()=="Q:#0" && state.playing==1);
    // 0인 입력은 정지를 요청한 뒤 0을 반환한다. 원본 항목이 이미 끝났으면 복제는 계속 재생한다.
    CHECK(player.SetNamePlaying(0,"a.wav",0,0)==0 && device.Take()=="!1016;Q:#0" && device.status[1]==5);
}

// 전체 음량은 멈춘 버퍼·복제 버퍼에도 범위를 자르지 않고 적용하고, 보류 중에는 예약 전역만 바꾼다.
TEST_CASE(SoundControl_AppliesUnclampedVolumeAndDefersUpdates) {
    DeviceMock device;device.loadAttenuation=300;SoundList list(OriginalEdition::Patch1078);list.Initialize();SoundState state;
    state.initialized=state.device=state.enabled=true;SoundPlayer player(list,state,device.Hooks());
    const auto root=list.Lookup("a.wav");player.Play(root,0,200,0,0,0);player.Play(root,0,700,0,0,0);
    device.status[0]=device.status[1]=0;device.Take();state.enabled=false;device.gainResult=-1;
    const auto before=std::vector<std::uint8_t>(list.Raw().begin(),list.Raw().end());
    player.SetMasterVolume(-12000);
    CHECK(device.Take()=="V:#0:-12100;V:#1:-11300" && state.masterVolume==-12000 && state.pendingMasterVolume==0 && state.playing==2);
    CHECK(std::equal(before.begin(),before.end(),list.Raw().begin()));
    state.volumeHoldDepth=-1;player.SetMasterVolume(30000);
    CHECK(state.masterVolume==-12000 && state.pendingMasterVolume==30000 && state.volumeHoldDepth==-1 && device.Take()=="-");
    state.volumeHoldDepth=0;state.initialized=false;player.SetMasterVolume(1000);
    CHECK(state.masterVolume==1000 && state.pendingMasterVolume==30000 && device.Take()=="-");
    // COM 호출이 첫 버퍼 적용 중 전역 음량을 바꾸면 다음 버퍼는 변경된 전역을 읽는다(원본은 루프마다 읽는다).
    auto hooks=device.Hooks();const auto apply=hooks.setVolume;
    hooks.setVolume=[&](SoundBuffer buffer,std::int32_t volume) {
        const auto result=apply(buffer,volume);if (buffer==kBufferBase) state.masterVolume=-500;return result;
    };
    SoundPlayer reentrant(list,state,std::move(hooks));state.initialized=true;reentrant.SetMasterVolume(2000);
    CHECK(device.Take()=="V:#0:1900;V:#1:200" && state.masterVolume==-500);
}
