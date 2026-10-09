// 독립 원본 소리 프로세스/이름 표 관찰과 보호막 생성→소리 재생→삭제 때 정지의 실제 모듈 연결을 검사한다.
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
// 원본 실행기와 같은 부모 SID·합성 타입 번호와 판본별 입력 수(실행 5,808 + 통지 144 + 생성 168 + 이름 표 7)다.
constexpr Sid kParent{50};
constexpr std::uint8_t kObjectType=82;
constexpr std::size_t kCases=6127;
// 일반 CTest는 PE/Python을 실행하지 않고 저장한 독립 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_SOUNDPROCESS_FIXTURE);return data.rows;
}
// 16진수 객체 덤프의 바이트 오프셋에서 리틀 엔디언 값을 읽는다(width는 바이트 수).
std::uint64_t Field(const std::string& hex,std::size_t offset,std::size_t width=4) {
    std::uint64_t value=0;
    // 낮은 바이트부터 두 글자씩 읽어 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint64_t>(std::stoul(hex.substr((offset+i)*2,2),nullptr,16))<<(8*i);
    return value;
}
// float 인자를 실행기와 같은 10진 비트 값으로 적는다.
std::string Bits(float value) { return std::to_string(std::bit_cast<std::uint32_t>(value)); }
// 실제 프로세스 host/Kernel/공통 삭제 위에 소리 프로세스를 올리고 장치 효과만 기록하는 장면이다.
struct SoundScene {
    bool patch;SidPool pool;SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);std::vector<RiftTypeRecord> types;
    SquidUnpop unpop;RawSquidDestroy destroy;Kernel kernel;SquidProcessState clock;SquidProcessHost host;
    SoundProcessState state;std::vector<std::string> events;std::uint32_t reply{};double now{};SoundProcessSystem system;
    // 실행기 풀과 같은 번호 5~127을 client 번호로 확보한다. form은 그 뒤 번호를 받는다.
    explicit SoundScene(OriginalEdition edition):patch(edition==OriginalEdition::Patch1078),pool(edition,kCapacity,false),types(patch ? 188 : 171),
        unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock),system(host,state,Hooks()) {
        // 입력 부모 번호가 모두 유효한 할당 슬롯이 되게 한다.
        for (std::uint16_t sid=5;sid<=kLastSid;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 원본 실행기의 대체 경계와 같은 사건 문자열을 만든다. 재생 반환과 벽시계는 행 입력이다.
    SoundProcessHooks Hooks() {
        return {[this] { events.push_back("T");return now; },
            [this](float x,float y,SoundHandle current,SoundHandle sound) {
                events.push_back("L:"+Bits(x)+':'+Bits(y)+':'+std::to_string(current)+':'+std::to_string(sound));return reply; },
            [this](float x,float y,SoundHandle sound,std::uint32_t priority,std::uint32_t zero) {
                events.push_back("O:"+Bits(x)+':'+Bits(y)+':'+std::to_string(sound)+':'+std::to_string(priority)+':'+std::to_string(zero));return reply; },
            [this](SoundHandle sound,std::uint32_t loop,std::uint32_t a,std::uint32_t b,std::uint32_t priority,std::uint32_t c) {
                events.push_back("G:"+std::to_string(sound)+':'+std::to_string(loop)+':'+std::to_string(a)+':'+std::to_string(b)+':'+std::to_string(priority)+':'+std::to_string(c));return reply; },
            [this](SoundHandle sound) { events.push_back("S:"+std::to_string(sound)); },
            [this](SoundHandle sound) { events.push_back("P:"+std::to_string(sound));return reply; },
            [this](std::string_view,int line) { events.push_back("!"+std::to_string(line)); }};
    }
    // 부모는 타입·상태 단어(+0xc)·좌표만 읽힌다.
    void Parent(std::uint32_t word,std::uint32_t x,std::uint32_t y) {
        auto raw=pool.AllocatedBytes(kParent);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        raw[10]=kObjectType;Put(raw,12,word,2);Put(raw,14,x);Put(raw,18,y);
    }
    // 외부 효과의 실제 순서만 문자열로 합친다.
    std::string Events() const {
        std::string text;
        // 같은 호출 순서를 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
    // 객체 덤프(원본 +0xc 이후 필드)와 살아 있는 프로세스의 필드를 비교한다. skip은 덤프가 +4부터일 때의 4다.
    static bool SameFields(const SoundProcess& process,const std::string& hex,std::size_t skip=0) {
        return process.Current()==Field(hex,12-skip) && process.Sound()==Field(hex,16-skip) && process.Alternate()==Field(hex,20-skip) &&
            process.Flags()==Field(hex,24-skip) && std::bit_cast<std::uint64_t>(process.StartTime())==Field(hex,32-skip,8);
    }
    // 검사한 프로세스를 실제 Kill로 정리한다. 이때의 정지 사건은 관찰 대상이 아니다.
    void Discard(SoundProcess* process) {
        if (kernel.Size()) host.Kill(*process,0x10);
        CHECK(kernel.Size()==0);events.clear();
    }
};
// 실행 몸체 한 행: 실제 부착 뒤 한 프레임을 실행하고 사건/객체/타입 프레임을 비교한다.
bool ReplayRun(SoundScene& scene,const std::vector<std::string>& row) {
    CHECK(row.size()==18);scene.Parent(Number(row[4]),Number(row[11]),Number(row[12]));
    scene.state.frame=Number(row[5]);scene.state.typeFrames.fill(0xa5a5a5a5U);scene.state.typeFrames[kObjectType]=Number(row[6]);
    scene.state.debug=row[10]=="1";scene.reply=Number(row[7]);scene.now=std::bit_cast<double>(std::stoull(row[8],nullptr,16));
    auto* process=scene.system.Add(kParent,Number(row[13]),Number(row[14]),Number(row[2]));CHECK(process!=nullptr);
    process->SetCurrent(Number(row[3]));process->SetStartTime(std::bit_cast<double>(std::stoull(row[9],nullptr,16)));
    scene.events.clear();process->RunFrame();
    // 원본의 Kill 호출은 실제 form 삭제로 관찰한다. 삭제된 객체의 필드는 더 읽지 않는다.
    const bool killed=scene.kernel.Size()==0;if (killed) scene.events.push_back("K:0");
    bool same=scene.Events()==row[15] && scene.state.typeFrames[kObjectType]==Number(row[17]) && scene.state.typeFrames[kObjectType+1]==0xa5a5a5a5U;
    if (!killed) same=same && SoundScene::SameFields(*process,row[16]);
    if (!same) std::printf("%s 소리 실행: 기대 %s/%s 실제 %s/%u\n",row[0].c_str(),row[15].c_str(),row[17].c_str(),scene.Events().c_str(),scene.state.typeFrames[kObjectType]);
    scene.Discard(process);return same;
}
// form 삭제 통지 한 행: 통지를 직접 불러 정지/재생 여부 조회/보고 순서를 비교한다.
bool ReplayDestroy(SoundScene& scene,const std::vector<std::string>& row) {
    CHECK(row.size()==8);scene.Parent(0,0,0);scene.reply=Number(row[4]);
    auto* process=scene.system.Add(kParent,static_cast<SoundHandle>(Field(row[7],16)),static_cast<SoundHandle>(Field(row[7],20)),Number(row[2]));CHECK(process!=nullptr);
    process->SetCurrent(Number(row[3]));scene.events.clear();process->OnFormDestroy(Number(row[5]));
    const bool same=scene.Events()==row[6] && SoundScene::SameFields(*process,row[7]);
    if (!same) std::printf("%s 소리 통지: 기대 %s 실제 %s\n",row[0].c_str(),row[6].c_str(),scene.Events().c_str());
    scene.Discard(process);return same;
}
// 생성자 한 행: 실제 부착 결과(form 타입·부모·client 번호)를 원본의 부착 인자와 비교한다.
bool ReplayCtor(SoundScene& scene,const std::vector<std::string>& row) {
    CHECK(row.size()==9);scene.state.processType=Number(row[6]);
    auto* process=scene.system.Add(Sid{static_cast<std::uint16_t>(Number(row[2]))},Number(row[3]),Number(row[4]),Number(row[5]));CHECK(process!=nullptr);
    const auto form=process->Form();const bool client=form.value>4 && form.value<scene.pool.Layout().serverFirst;
    const std::string attached="B:"+std::to_string(scene.pool.Slot(form)[10])+':'+std::to_string(process->Parent().value)+":0:"+(client ? "16" : "0");
    const bool same=attached==row[7] && SoundScene::SameFields(*process,row[8],4) && scene.events.empty();
    if (!same) std::printf("%s 소리 생성: 기대 %s 실제 %s\n",row[0].c_str(),row[7].c_str(),attached.c_str());
    scene.state.processType=kSoundProcessType;scene.Discard(process);return same;
}
// 이름 표 연산 묶음 한 행: 실제 표에 같은 연산을 차례로 적용하고 반환·보고·빈 위치·표 전체를 비교한다.
bool ReplayLookup(OriginalEdition edition,const std::vector<std::string>& row) {
    CHECK(row.size()==6);std::vector<std::string> tokens;
    SoundList list(edition,[&](std::string_view,int line) { tokens.push_back("!"+std::to_string(line)); });
    // 연산마다 원본 진입 한 번에 해당하는 호출을 한다.
    for (const auto& operation:Split(row[2],'|')) {
        SoundHandle result=0;
        if (operation=="i") { list.Initialize();result=list.Fallback(); }
        else if (operation=="z") result=list.Lookup(static_cast<const char*>(nullptr));
        else if (operation[0]=='p') {
            // 표 끝 바로 뒤의 포인터는 표 밖 문자열이다. 실행기는 그 자리에 "edge.wav"를 둔다.
            const auto offset=Number(operation.substr(2));
            result=offset<kSoundListBytes ? list.LookupPointer(kSoundListBase+offset) : list.Lookup("edge.wav");
        } else {
            std::string name;
            // 16진수 두 글자를 한 바이트로 되돌린다.
            for (std::size_t i=2;i<operation.size();i+=2) name+=static_cast<char>(std::stoul(operation.substr(i,2),nullptr,16));
            result=list.Lookup(name);
        }
        tokens.push_back(result ? std::to_string(static_cast<std::int32_t>(result-kSoundListBase)) : "null");
    }
    std::string text;
    // 실행기와 같은 쉼표 구분이다.
    for (const auto& token:tokens) { if (!text.empty()) text+=',';text+=token; }
    const bool same=text==row[3] && list.FirstFree()-kSoundListBase==Number(row[4]) && Adler(list.Raw())==Number(row[5]);
    if (!same) std::printf("%s 이름 표: 기대 %s 실제 %s\n",row[0].c_str(),row[3].c_str(),text.c_str());
    return same;
}
// 한 판본의 모든 원본 관찰을 실제 C++ 모듈에 재생한다. 원본 출력값을 다시 계산하지 않는다.
void Replay(const char* edition) {
    const auto kind=std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    SoundScene scene(kind);std::size_t count=0;CHECK(Fixture().size()==kCases*3);
    // 종류별 재생 함수로 나눈다. 첫 불일치에서 멈춰 출력이 넘치지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        const bool same=row[1]=="Run" ? ReplayRun(scene,row) : row[1]=="Destroy" ? ReplayDestroy(scene,row) :
            row[1]=="Ctor" ? ReplayCtor(scene,row) : ReplayLookup(kind,row);
        CHECK(same);++count;if (!same) break;
    }
    CHECK(count==kCases);
}
// 실제 표시 계산기의 콘솔 대상이다. 창 없이 표시 갱신을 받는다.
struct SoundSink final:SquidDisplaySink {
    // 표시를 억제하지 않는다.
    bool Suppressed() const override { return false; }
    // 무효화 영역은 이 검사에서 쓰지 않는다.
    void Invalidate(SquidDisplayRect,std::uint32_t) override {}
};
}
// 패치판의 실제 생성자/실행/통지/이름 표 관찰을 재생한다.
TEST_CASE(SoundProcess_ReplaysOriginals) { Replay("originals"); }
// CD판의 NaN 시각 차이·이름 끝 글자 보고를 같은 실제 모듈에 대조한다.
TEST_CASE(SoundProcess_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 관찰로 대조한다.
TEST_CASE(SoundProcess_ReplaysExtra1037) { Replay("original1037"); }

// 실제 보호막 생성 wrapper→이름 표 조회→소리 프로세스 부착→매 프레임 위치 반복 재생→보호막 삭제 때 정지를 검사한다.
TEST_CASE(SoundProcess_FollowsShieldCreationAndDeletion) {
    // 두 raw 배치에서 원본 생성자/가상 표와 실제 공간/프로세스/삭제 모듈을 사용한다.
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
        SoundSink sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidUnpop unpop(pool,hash,spots,&display);SquidPostPopState book;book.localOwner=1;SquidPostPop base(pool,types,book);SquidPop pop(pool,hash,spots,&display,&base);
        RawSquidDestroy destroy(pool,unpop,types);SquidDeletionState deletion;deletion.unitLostSuppressed1=true;
        SquidDestroyLifecycle lifecycle(pool,types,book,deletion,destroy,{[](const SquidDeletionEvent&) {},[](const SquidDestroyEvent&) { CHECK(false); }});
        Kernel kernel;SquidProcessState clock;clock.now=12.5;std::unique_ptr<RawForcefieldRegular> regular;
        SquidProcessHost host(pool,types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return regular->Handle(sid,event,count,payload); },lifecycle.Hooks());
        RawForcefieldPostPop prefix(pool,MakeForcefieldPostPopProcessHooks(pool,host,[] { return true; }));base.SetForcefieldPostPop(&prefix);
        SquidFrame setter(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));RawFrameAdvance advance(pool,frames,setter);ForcefieldRegularState state;
        regular=std::make_unique<RawForcefieldRegular>(pool,hash,types,state,MakeForcefieldFrameHooks(pool,advance,{{},[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); }}));
        // 소리 장치만 기록 대체한다. 재생 함수는 고정 항목을 재생 중이라고 답한다.
        SoundList sounds(edition);sounds.Initialize();SoundProcessState soundState;soundState.frame=100;std::vector<std::string> played;bool reserve=true;
        const SoundHandle playing=0x15007000;
        SoundProcessSystem sound(host,soundState,{[] { return 3.0; },
            [&](float x,float y,SoundHandle current,SoundHandle entry) {
                played.push_back("L:"+Bits(x)+':'+Bits(y)+':'+std::to_string(current)+':'+std::string(sounds.Name(entry)));return playing; },
            [](float,float,SoundHandle,std::uint32_t,std::uint32_t) { CHECK(false);return SoundHandle{}; },
            [](SoundHandle,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { CHECK(false);return SoundHandle{}; },
            [&](SoundHandle entry) { played.push_back("S:"+std::to_string(entry)); },
            [&](SoundHandle entry) { played.push_back("P:"+std::to_string(entry));return 0U; },{}});
        SquidFactory factory(pool,types);SquidOwnerMode ownerMode;SquidOwner owner(pool,types,book,ownerMode);
        PriestForcefieldState fieldState;RawPriestForcefield lookup(pool,hash,types,fieldState);PriestShieldState notice{0,1,1};Sid shield;
        auto hooks=MakePriestShieldSoundHooks(pool,sounds,sound,MakePriestShieldCreationHooks(pool,factory,owner,{{},{},[&](Sid sid,float x,float y,std::uint32_t flags) {
            shield=sid;CHECK(pop.Pop(sid,types[kForcefieldType],1,1,x,y,flags)==RawPopResult::Registered);
        },[&] { return reserve; },{},{},[](const PriestShieldNotice&) { CHECK(false); },[](std::string_view) { CHECK(false);return std::string{}; },[](std::string_view) { CHECK(false); }}));
        RawPriestShield create(pool,lookup,notice,std::move(hooks));const auto before=pool.FreeCount();create.Ensure(priest);
        // 보호막에는 두 Regular와 소리 프로세스가 붙는다. 조회한 이름은 대체 소리 다음의 첫 항목이다.
        const SoundHandle entry=sounds.Lookup("PRIESTFORCEFIELD.WAV");const std::string where=Bits(20.75f)+':'+Bits(21.9f);
        CHECK(lookup.Find(priest)==shield && kernel.Size()==3 && entry==kSoundListBase+44 && sounds.Name(entry)=="priestForceField.wav" && played.empty());
        // 첫 프레임에 재생을 시작하고 보호막 타입의 마지막 소리 프레임을 적는다.
        kernel.RunFrame();CHECK(played==std::vector<std::string>{"L:"+where+":0:priestForceField.wav"} && soundState.typeFrames[kForcefieldType]==100);
        // 재생 중이면 같은 프레임 번호여도 매번 위치 갱신을 요청한다.
        kernel.RunFrame();++soundState.frame;kernel.RunFrame();
        CHECK(played.size()==3 && played[2]=="L:"+where+':'+std::to_string(playing)+":priestForceField.wav" && soundState.typeFrames[kForcefieldType]==101);
        // 사제가 사라지면 보호막이 자기 삭제하고 종속 form 정리에서 반복 소리를 멈춘다.
        const double due=host.FindEvent(shield,2)->Time();unpop.Unpop(priest,types[kPriestType],0);played.clear();clock.now=due;kernel.RunFrame();
        CHECK(kernel.Size()==0 && pool.FreeCount()==before && played.size()>=2 && played[played.size()-2]=="S:"+std::to_string(playing) && played.back()=="P:"+std::to_string(playing));
        // 소리 프로세스 확보 실패(new 실패)면 조회/부착 없이 보호막만 다시 만든다.
        reserve=false;played.clear();create.Ensure(priest);CHECK(kernel.Size()==2 && lookup.Find(priest)==shield && played.empty());
        destroy.Destroy(shield,0,host.Hooks());CHECK(kernel.Size()==0 && pool.FreeCount()==before && played.empty());
    }
}

// 지연 시작·대체 소리·같은 프레임의 타입 제한·한 번 재생의 종료와 잘못된 연결/이름의 거부를 검사한다.
TEST_CASE(SoundProcess_GatesStartAndRejectsInvalidUse) {
    SoundScene scene(OriginalEdition::Patch1078);scene.Parent(0,std::bit_cast<std::uint32_t>(3.5f),std::bit_cast<std::uint32_t>(4.25f));
    scene.state.frame=9;scene.state.typeFrames[kObjectType]=1;scene.now=5.0;scene.reply=0x15000100;
    // 시작 시각 전에는 벽시계만 읽고, 시각이 되면 대체 소리 조건을 다시 읽어 재생한다.
    auto* loop=scene.system.Add(kParent,0x15000040,0x15000080,kSoundPlayAt|kSoundUseAlternate);loop->SetStartTime(5.5);
    scene.kernel.RunFrame();CHECK(scene.Events()=="T" && loop->Current()==0);
    scene.now=5.5;Put(scene.pool.AllocatedBytes(kParent),12,0x100,2);scene.events.clear();scene.kernel.RunFrame();
    CHECK(scene.Events()=="T;L:"+Bits(3.5f)+':'+Bits(4.25f)+":0:"+std::to_string(0x15000080) && loop->Current()==0x15000100 && scene.state.typeFrames[kObjectType]==9);
    // 같은 프레임에 같은 타입의 두 번째 프로세스는 시작하지 않고, 다음 프레임에 시작한다.
    auto* second=scene.system.Add(kParent,0x15000040,0,kSoundPlayAt);scene.events.clear();second->RunFrame();CHECK(scene.Events()=="T" && second->Current()==0);
    ++scene.state.frame;scene.events.clear();second->RunFrame();CHECK(second->Current()==0x15000100 && scene.state.typeFrames[kObjectType]==10);
    // 한 번 재생은 요청 뒤 스스로 끝나며 소리를 멈추지 않는다. 반복 프로세스는 Kill 때 멈춘다.
    ++scene.state.frame;auto* once=scene.system.Add(kParent,0x15000040,0,kSoundPlayOnce);scene.events.clear();once->RunFrame();
    CHECK(scene.Events()=="T;G:"+std::to_string(0x15000040)+":0:0:0:0:0" && scene.kernel.Size()==2);
    scene.reply=0;scene.events.clear();scene.host.Kill(*second,0x10);CHECK(scene.Events()=="S:"+std::to_string(0x15000100)+";P:"+std::to_string(0x15000100) && scene.kernel.Size()==1);
    // 패치판은 dead 부모에 붙이지 않는다. 잘못된 타입 번호·빠진 효과·다른 풀은 거부한다.
    scene.pool.AllocatedBytes(Sid{60})[11]=2;CHECK(scene.system.Add(Sid{60},1,0,0)==nullptr);scene.pool.AllocatedBytes(Sid{60})[11]=0;
    scene.state.processType=5;CHECK(Throws([&] { scene.system.Add(kParent,1,0,0); }));scene.state.processType=kSoundProcessType;
    auto broken=scene.Hooks();broken.stop={};CHECK(Throws([&] { SoundProcessSystem(scene.host,scene.state,broken); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);SoundList list(OriginalEdition::Patch1078);
    CHECK(Throws([&] { MakePriestShieldSoundHooks(other,list,scene.system,{}); }));
    // 이름 표: 초기화 전 가득 참은 0, 빈 이름/NUL/표 밖 포인터는 거부, 초기화는 한 번만 등록한다.
    CHECK(list.Fallback()==0 && Throws([&] { list.Lookup(std::string_view{}); }) && Throws([&] { list.Lookup(std::string_view("a\0b",3)); }));
    CHECK(Throws([&] { list.LookupPointer(kSoundListBase-1); }) && Throws([&] { list.LookupPointer(kSoundListBase+kSoundListBytes); }) && Throws([&] { list.Name(0); }));
    list.Initialize();const auto free=list.FirstFree();list.Initialize();
    CHECK(list.Fallback()==kSoundListBase && list.FirstFree()==free && list.Name(list.Fallback())==kFallbackSoundName && list.Lookup("NonExistant.WAV")==kSoundListBase);
    scene.Discard(loop);
}
