// 독립 원본 소리 재생 계층 관찰을 재생하고, 보호막 소리가 소리 프로세스→재생 계층→장치 호출로 이어지는지 검사한다.
#include "RawSceneSupport.h"
#include "client/SoundProcess.h"
#include "o/RawForcefieldLifecycle.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFrameBinding.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 판본별 입력 수(연산 묶음 1,065 + 월드→화면 399 + 화면 점 3,360)와 원본 실행기의 가짜 버퍼 값(기준 + 16 × 순번)이다.
constexpr std::size_t kCases=4824;
constexpr SoundBuffer kBufferBase=0x16001000;
// 일반 CTest는 PE/Python을 실행하지 않고 저장한 독립 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_SOUNDPLAY_FIXTURE);return data.rows;
}
// 원본 실행기의 대체 장치와 같은 규칙의 상태표다: 재생하면 재생 중(반복이면 반복 비트), 정지하면 두 비트를 끈다.
// 호출 사실과 인자는 실행기와 같은 사건 문자열로 적는다. 적재/복제/설정의 결과는 입력으로 정한다.
struct DeviceMock {
    std::vector<std::uint32_t> status;std::vector<std::string> events;
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
        hooks.status=[this](SoundBuffer buffer) { events.push_back("Q:"+Ordinal(buffer));return status.at(Index(buffer)); };
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
    CHECK(row.size()==8);DeviceMock device;SoundState state;SoundList list(edition,device.Report());
    SoundPlayer player(list,state,device.Hooks(),device.Report());std::vector<std::uint32_t> results;std::string tokens;
    // $k는 k번째 연산의 반환 항목, 그 밖에는 숫자다.
    const auto handle=[&](const std::string& text) { return text[0]=='$' ? results.at(std::stoul(text.substr(1))) : Number(text); };
    // 좌표 칸은 단정도 비트를 10진수로 적은 것이다.
    const auto real=[](const std::string& text) { return std::bit_cast<float>(Number(text)); };
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
            else throw std::runtime_error("알 수 없는 전역");
        }
        else if (op[0]=="view") { state.view.left=whole(op[1]);state.view.top=whole(op[2]);state.view.right=whole(op[3]);state.view.bottom=whole(op[4]); }
        else if (op[0]=="cam") { state.view.cameraX=whole(op[1]);state.view.cameraY=whole(op[2]); }
        else if (op[0]=="st") device.status.at(Number(op[1]))=Number(op[2]);
        else if (op[0]=="hr") (op[1]=="copy" ? device.copyResult : op[1]=="gain" ? device.gainResult : device.balanceResult)=whole(op[2]);
        else if (op[0]=="ld") { device.loadKind=op[1][0];device.loadAttenuation=whole(op[2]); }
        else if (op[0]=="clr") list.SetField(handle(op[1]),SoundField::Buffer,0);
        else if (op[0]=="play") { result=player.Play(handle(op[1]),Number(op[2]),whole(op[3]),whole(op[4]),Number(op[5]),whole(op[6]));entry=true; }
        else if (op[0]=="loop") { result=player.PlayLoopAt(real(op[1]),real(op[2]),handle(op[3]),handle(op[4]));entry=true; }
        else if (op[0]=="once") { result=player.PlayOnceAt(real(op[1]),real(op[2]),handle(op[3]),Number(op[4]),Number(op[5]));entry=true; }
        else if (op[0]=="stop") player.Stop(handle(op[1]));
        else if (op[0]=="isp") { result=player.IsPlaying(handle(op[1]));shown=std::to_string(result); }
        else if (op[0]=="name") { result=player.PlayByName(Decode(op[1]),Number(op[2]),whole(op[3]),whole(op[4]),Number(op[5]),whole(op[6]));entry=true; }
        else if (op[0]=="at") { result=player.PlayNameAt(real(op[1]),real(op[2]),Decode(op[3]),Number(op[4]));entry=true; }
        else throw std::runtime_error("알 수 없는 연산");
        if (entry) shown=result ? std::to_string(static_cast<std::int32_t>(result-kSoundListBase)) : "null";
        results.push_back(result);
        if (!tokens.empty()) tokens+='|';
        tokens+=device.Take()+'='+shown;
    }
    const bool same=tokens==row[3] && list.FirstFree()-kSoundListBase==Number(row[4]) && Adler(list.Raw())==Number(row[5]) &&
        std::to_string(state.playing)==row[6] && state.serial==Number(row[7]);
    if (!same) std::printf("%s 소리 재생: %s\n  기대 %s\n  실제 %s\n",row[0].c_str(),row[2].c_str(),row[3].c_str(),tokens.c_str());
    return same;
}
// 좌표 계산 행: 월드→화면, 또는 화면 점의 화면 안 판정/좌우/음량을 비교한다.
bool ReplayPoint(const std::vector<std::string>& row) {
    // 칸 번호로 부호 있는 값을 읽는다(DWORD 10진수 표기).
    const auto whole=[&](std::size_t index) { return static_cast<std::int32_t>(Number(row[index])); };
    if (row[1]=="Screen") {
        CHECK(row.size()==8);const SoundView view{whole(4),whole(5),0,0,640,480};
        const auto point=SoundToScreen(view,std::bit_cast<float>(Number(row[2])),std::bit_cast<float>(Number(row[3])));
        return std::to_string(point.x)==row[6] && std::to_string(point.y)==row[7];
    }
    CHECK(row.size()==12);const SoundView view{0,0,whole(4),whole(5),whole(6),whole(7)};const SoundPoint point{whole(2),whole(3)};
    return (SoundOnScreen(view,point) ? "1" : "0")==row[9] && std::to_string(SoundPan(view,point,row[8]=="1"))==row[10] &&
        std::to_string(SoundVolume(view,point))==row[11];
}
// 한 판본의 모든 원본 관찰을 실제 C++ 모듈에 재생한다. 원본 출력값을 다시 계산하지 않는다.
void Replay(const char* edition) {
    const auto kind=std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    std::size_t count=0;CHECK(Fixture().size()==kCases*3);
    // 종류별 재생 함수로 나눈다. 첫 불일치에서 멈춰 출력이 넘치지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        const bool same=row[1]=="Script" ? ReplayScript(kind,row) : ReplayPoint(row);
        CHECK(same);++count;
        if (!same) { if (row[1]!="Script") std::printf("%s 좌표 계산 행 %zu 불일치\n",edition,count);break; }
    }
    CHECK(count==kCases);
}
// 실제 표시 계산기의 콘솔 대상이다. 창 없이 표시 갱신을 받는다.
struct PlaySink final:SquidDisplaySink {
    // 표시를 억제하지 않는다.
    bool Suppressed() const override { return false; }
    // 무효화 영역은 이 검사에서 쓰지 않는다.
    void Invalidate(SquidDisplayRect,std::uint32_t) override {}
};
}
// 패치판의 실제 재생/정지/위치 계산 관찰을 재생한다.
TEST_CASE(SoundPlay_ReplaysOriginals) { Replay("originals"); }
// CD판(위치 반복 재생이 일반 재생을 호출하는 구조)의 관찰을 같은 실제 모듈에 대조한다.
TEST_CASE(SoundPlay_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 관찰로 대조한다.
TEST_CASE(SoundPlay_ReplaysExtra1037) { Replay("original1037"); }

// 실제 보호막 생성 wrapper→이름 표→소리 프로세스→재생 계층→장치 호출과, 로컬 안내 소리·사제 낙하 소리의 연결을 검사한다.
TEST_CASE(SoundPlay_DrivesDeviceFromShieldLifecycle) {
    // 두 raw 배치에서 원본 생성자/가상 표와 실제 공간/프로세스/삭제/재생 모듈을 사용한다. 장치만 상태표다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[kForcefieldType].constructorAddress=TypeConstructorAddress(edition,kForcefieldType);
        types[kForcefieldType].maxHitPoints=100;types[kForcefieldType].cost=37;types[kForcefieldType].footX=types[kForcefieldType].footY=1;
        types[kPriestType].footX=types[kPriestType].footY=1;types[kPriestType].flags2=0x210000;std::vector<std::uint8_t> spots(65536);
        const Sid priest=pool.Allocate(2);auto raw=pool.AllocatedBytes(priest);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[patch ? 34 : 32]=1;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));raw[patch ? 33 : 31]=1;hash.Bucket(1,20.75f,21.9f)=priest.value;
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[kForcefieldType]=RiftTypeFrames({{65,80,1,0},{65,80,2,0}});
        std::vector<SquidDisplayShape> shapes(types.size());shapes[kForcefieldType]={2,true,{{12,20,3,18,1,1},{16,21,4,19,1,1}}};
        shapes[kPriestType]={1,true,{{12,20,3,18,1,1}}};
        PlaySink sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidUnpop unpop(pool,hash,spots,&display);SquidPostPopState book;book.localOwner=1;SquidPostPop base(pool,types,book);SquidPop pop(pool,hash,spots,&display,&base);
        RawSquidDestroy destroy(pool,unpop,types);SquidDeletionState deletion;deletion.unitLostSuppressed1=true;
        SquidDestroyLifecycle lifecycle(pool,types,book,deletion,destroy,{[](const SquidDeletionEvent&) {},[](const SquidDestroyEvent&) { CHECK(false); }});
        Kernel kernel;SquidProcessState clock;clock.now=12.5;std::unique_ptr<RawForcefieldRegular> regular;
        SquidProcessHost host(pool,types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return regular->Handle(sid,event,count,payload); },lifecycle.Hooks());
        RawForcefieldPostPop prefix(pool,MakeForcefieldPostPopProcessHooks(pool,host,[] { return true; }));base.SetForcefieldPostPop(&prefix);
        SquidFrame setter(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));RawFrameAdvance advance(pool,frames,setter);ForcefieldRegularState state;
        regular=std::make_unique<RawForcefieldRegular>(pool,hash,types,state,MakeForcefieldFrameHooks(pool,advance,{{},[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); }}));
        // 실제 이름 표·재생 계층·소리 프로세스. 장치는 준비됐고 소리 옵션이 켜져 있다. 파일 감쇠 300, 전체 음량 −200.
        DeviceMock device;device.loadAttenuation=300;SoundList sounds(edition);sounds.Initialize();
        SoundState soundState;soundState.initialized=soundState.device=soundState.enabled=true;soundState.masterVolume=-200;
        SoundPlayer player(sounds,soundState,device.Hooks());SoundProcessState processState;processState.frame=100;
        SoundProcessSystem sound(host,processState,MakeSoundProcessHooks(player,[] { return 3.0; }));
        SquidFactory factory(pool,types);SquidOwnerMode ownerMode;SquidOwner owner(pool,types,book,ownerMode);
        PriestForcefieldState fieldState;RawPriestForcefield lookup(pool,hash,types,fieldState);PriestShieldState notice{1,0,0.0};Sid shield;std::string told;
        auto hooks=MakePriestShieldNoticeHooks(player,MakePriestShieldSoundHooks(pool,sounds,sound,MakePriestShieldCreationHooks(pool,factory,owner,{{},{},
            [&](Sid sid,float x,float y,std::uint32_t flags) { shield=sid;CHECK(pop.Pop(sid,types[kForcefieldType],1,1,x,y,flags)==RawPopResult::Registered); },
            [] { return true; },{},{},{},[](std::string_view key) { return std::string(key); },[&](std::string_view text) { told=text; }})));
        RawPriestShield create(pool,lookup,notice,std::move(hooks));const auto before=pool.FreeCount();create.Ensure(priest);
        // 생성 직후: 보호막 소리는 아직 장치에 닿지 않고, 로컬 안내 소리만 전역 우선 재생으로 적재·재생된다(감쇠 300 + 전체 −200 → −500).
        const auto shieldSound=sounds.Lookup("priestForceField.wav"),noticeSound=sounds.Lookup("ourPriestImmobile.wav");
        CHECK(kernel.Size()==3 && shieldSound==kSoundListBase+44 && noticeSound==kSoundListBase+93 && (told=="PriestImmobile")==patch);
        CHECK(device.Take()=="F:ourPriestImmobile.wav;Q:#0;V:#0:-500;N:#0:0;P:#0:0:0:0" && sounds.Field(noticeSound,SoundField::Priority)==1 && soundState.playing==1);
        // 첫 프레임: 보호막 위치(20.75, 21.9 → 화면 332, 241)가 화면 안이므로 적재 → 반복 재생 → 위치 음량/좌우 갱신.
        const SoundPoint point=SoundToScreen(soundState.view,20.75f,21.9f);CHECK(point.x==332 && point.y==241);
        const auto gain=std::to_string(SoundVolume(soundState.view,point)-500),balance=std::to_string(SoundPan(soundState.view,point,false));
        kernel.RunFrame();
        CHECK(device.Take()=="F:priestForceField.wav;Q:#1;V:#1:-500;N:#1:0;P:#1:0:0:1;Q:#1;V:#1:"+gain+";N:#1:"+balance);
        CHECK(soundState.playing==2 && processState.typeFrames[kForcefieldType]==100 && sounds.Field(shieldSound,SoundField::Attenuation)==300);
        // 다음 프레임: 재생 중이므로 다시 시작하지 않고 위치만 갱신한다(이름 비교 → 상태 조회 두 번 → 음량 → 좌우).
        ++processState.frame;kernel.RunFrame();CHECK(device.Take()=="Q:#1;Q:#1;V:#1:"+gain+";N:#1:"+balance && soundState.playing==2);
        // 화면 밖으로 벗어나면 정지하고, 다시 들어오면 같은 버퍼로 다시 시작한다.
        soundState.view.cameraX=5000;kernel.RunFrame();CHECK(device.Take()=="Q:#1;Q:#1;X:#1;C:#1:0" && soundState.playing==1);
        soundState.view.cameraX=0;++processState.frame;kernel.RunFrame();
        CHECK(device.Take()=="Q:#1;V:#1:-500;N:#1:0;P:#1:0:0:1;Q:#1;V:#1:"+gain+";N:#1:"+balance && soundState.playing==2);
        // 사제가 사라지면 보호막이 자기 삭제하고, 종속 form 정리에서 반복 소리를 실제로 정지한다. 통지의 재생 여부 확인이 뒤따른다.
        const double due=host.FindEvent(shield,2)->Time();unpop.Unpop(priest,types[kPriestType],0);clock.now=due;kernel.RunFrame();
        const auto events=device.Take();
        CHECK(kernel.Size()==0 && pool.FreeCount()==before && events.ends_with("Q:#1;Q:#1;X:#1;C:#1:0;Q:#1") && soundState.playing==1 && device.status[1]==0);
        // 사제 낙하 소리: 화면 안의 좌표에서 이름으로 한 번 재생한다. 화면 밖이면 이름만 등록된다.
        const auto fall=MakePriestFallSound(player);fall(priest,20.0f,21.0f);
        CHECK(device.Take()=="F:priestFall.wav;Q:#2;V:#2:"+std::to_string(SoundVolume(soundState.view,SoundToScreen(soundState.view,20.0f,21.0f))-500)+";N:#2:0;P:#2:0:0:0");
        fall(priest,200.0f,21.0f);CHECK(device.Take()=="-" && soundState.playing==2);
    }
}

// 잘못된 연결/항목/화면 영역의 거부와, 원본이 보고 뒤 멈추는 호출 계약 위반의 진단을 검사한다.
TEST_CASE(SoundPlay_RejectsInvalidUseAndDiagnosesContractViolations) {
    DeviceMock device;SoundState state;SoundList list(OriginalEdition::Patch1078,device.Report());list.Initialize();
    auto broken=device.Hooks();broken.duplicate={};CHECK(Throws([&] { SoundPlayer(list,state,broken); }));
    SoundPlayer player(list,state,device.Hooks(),device.Report());state.initialized=state.device=state.enabled=true;
    const auto sound=list.Lookup("a.wav");
    // 표 밖의 항목 값과 필드 쓰기는 거부한다. 복제 항목은 표 끝을 넘지 못한다.
    CHECK(Throws([&] { list.Field(0,SoundField::Buffer); }) && Throws([&] { list.SetField(kSoundListBase+kSoundListBytes-4,SoundField::Buffer,1); }));
    CHECK(Throws([&] { player.Play(kSoundListBase-4,0,0,0,0,0); }));
    // 화면 가로가 2보다 작으면 원본은 0으로 나누어 멈춘다. 가운데 점의 좌우 값만 나눗셈 없이 0이다.
    const SoundView narrow{0,0,10,0,11,480};
    CHECK(Throws([&] { SoundVolume(narrow,{10,0}); }) && Throws([&] { SoundPan(narrow,{11,0},false); }) && SoundPan(narrow,{10,0},false)==0);
    // 버퍼가 없는 현재 항목을 다른 이름으로 바꾸는 반복 재생은 원본이 두 조건을 보고한 뒤 멈춘다.
    const auto other=list.Lookup("b.wav");
    CHECK(Throws([&] { player.PlayLoopAt(20.0f,21.0f,other,sound); }) && device.Take()=="!155;!156");
    // 소리 없음 표식의 항목은 재생·정지·조회 모두 장치에 닿지 않는다.
    device.loadKind='s';CHECK(player.Play(sound,1,0,0,0,0)==sound && device.Take()=="F:a.wav" && player.IsPlaying(sound)==0);
    player.Stop(sound);CHECK(device.Take()=="-" && state.playing==0);
    CHECK(Throws([&] { player.PlayLoopAt(20.0f,21.0f,sound,other); }));
    // 벽시계가 없는 소리 프로세스 연결은 거부한다.
    CHECK(Throws([&] { MakeSoundProcessHooks(player,{}); }));
    // 가득 찬 표에서는 복제 항목을 만들 수 없다는 진단을 준다(원본은 표 밖에 쓴다).
    SoundList full(OriginalEdition::Patch1078);const std::string big(900,'x');
    // 표를 긴 이름으로 채운다. 한도를 넘으면 조회가 대체 소리(여기서는 0)를 돌려준다.
    for (int i=0;i<40;++i) static_cast<void>(full.Lookup(std::to_string(i)+big+".wav"));
    const auto last=full.Lookup("0"+big+".wav");CHECK(last==kSoundListBase && Throws([&] { full.AppendDuplicate(last,last,1); }));
}
