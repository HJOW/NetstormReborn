// 실제두PE DudeNotice/nearest/point 출력과같은실제Host Regular재사용을대조한다.
#include "RawSceneSupport.h"
#include "o/RawDudeNotice.h"
#include <cstdio>
#include <tuple>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본의독립출력은저장된fixture에서만읽는다.
const std::vector<std::vector<std::string>>& DudeRows() { static const auto rows=LoadFixture(NETSTORM_DUDENOTICE_FIXTURE).rows;return rows; }
// Host가 생성 시 복사하는 타입 표에도 같은 현재 genus와 발자국 입력을 공급한다.
std::vector<RiftTypeRecord> DudeTypes(bool patch) {
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[70].flags2=0x400;types[70].footX=types[70].footY=1;types[89].flags2=0x40000000;types[89].footX=types[89].footY=3;return types;
}
// 같은raw/현재타입/실제Regular와명시burning/path 요청경계를소유하는독립장면이다.
struct DudeScene {
    SidPool pool; // 원본번호50/60/61/77/78/83/84를공유하는판본별저장소다.
    SquidHash hash; // 원본과같은수동등록입력의네단계버킷이다.
    std::vector<std::uint8_t> spots; // 공통삭제처리기의현재spot이며fixture에서는표시공간을실행하지않는다.
    std::vector<RiftTypeRecord> types; // 현재Dude70/거주지89의genus/발자국이다.
    std::vector<RiftTypeFrames> frames; // 현재A/I 단계의합성실제형식코드입력이다.
    std::vector<DudeNoticeTypeData> speeds; // native와같은분수속도저장소다.
    SquidPostPopList residences; // 현재active DWORD 거주지목록이다.
    SquidUnpop unpop; // 실제같은풀/해시의Host 삭제경로를제공한다.
    RawSquidDestroy destroy; // 실제Form/종속수명에사용하는현재삭제몸체다.
    Kernel kernel; // 현재Regular 객체를실제로소유한다.
    SquidProcessState clock; // 현재공유double 게임시각이다.
    SquidProcessHost host; // 같은실제Regular 조회/생성/반복실행을소비한다.
    std::vector<std::string> events; // 현재원본과같은관찰경계인자/순서다.
    const std::vector<std::string>& row; // 현재입력행만참조하며기대값칸을조회하지않는다.
    bool patch; // frame/owner의두판본쓰기폭을선택한다.
    // 원본83Regular번호를같은client SID배치로확보한다.
    explicit DudeScene(const std::vector<std::string>& input):pool(input[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true),spots(65536),types(DudeTypes(input[0]=="originals")),frames(types.size(),RiftTypeFrames({})),speeds(types.size()),unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock,[](Sid,std::uint32_t,std::uint32_t,float) { return 1.0f; }),row(input),patch(input[0]=="originals") {
        // 같은관찰번호앞의슬롯만입력배치로확보한다.
        for (std::uint16_t id=5;id<=82;++id) CHECK(pool.Allocate(2)==Sid{id});
        frames[70]=RiftTypeFrames({{'A','P',0,0},{'I','P',0,0}});speeds[70].speed=std::bit_cast<float>(Number(row[9]));
        // 현재source/두거주지의raw폭과등록입력만공급한다.
        for (const auto [sid,type,x,y,owner]:{std::tuple{std::uint16_t{50},std::uint8_t{70},20.75f,21.9f,std::uint8_t{1}},std::tuple{std::uint16_t{60},std::uint8_t{89},24.75f,24.5f,static_cast<std::uint8_t>(Number(row[6])==2 ? 2 : 1)},std::tuple{std::uint16_t{61},std::uint8_t{89},16.75f,19.3f,std::uint8_t{1}}}) {
            auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[patch ? 34U : 32U]=owner;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));Put(raw,12,sid==50 ? Number(row[3]) : 0U,2);Put(raw,patch ? 36U : 34U,sid==50 ? Number(row[10]) : 0U,patch ? 4U : 1U);
        }
        Sid head;
        if (Number(row[7])) { Marker(Sid{78},24,{});head=Sid{78}; }
        if (Number(row[5])) { Marker(Sid{77},14,head);head=Sid{77}; }
        Put(pool.AllocatedBytes(kSource),6,head.value,2);hash.Bucket(1,20.75f,21.9f)=50;
        if (Number(row[4])) {
            clock.now=20;auto* regular=host.AddRegular(kSource,0x5104,1.0f,0x50);CHECK(regular && regular->Form()==Sid{83});if (!regular) throw std::logic_error("Dude 검사 현재Regular생성 실패");
            // 기존count7/time27은기대값직접대입없이실제Regular의7회반복으로준비한다.
            for (int count=0;count<7;++count) { clock.now=regular->Time();regular->RunFrame(); }
            CHECK(regular->Count()==7 && regular->Time()==27 && regular->Payload()==1);
        }
        pool.AllocatedBytes(kSource)[11]=static_cast<std::uint8_t>(Number(row[2]));clock.now=std::bit_cast<double>(std::stoull(row[11]));const auto mode=Number(row[6]);residences.entries=mode==0 ? std::vector<std::uint32_t>{} : mode==3 ? std::vector<std::uint32_t>{60,61} : std::vector<std::uint32_t>{60};residences.count=static_cast<std::uint32_t>(residences.entries.size());
        if (Number(row[1])==2) types[89].flags2=std::array<std::uint32_t,4>{0,0x40000000,0x10000000,0x40000}.at(mode);
    }
    // 관찰된기존typed Form 또는미복원burning ctor경계의체인효과만공급한다.
    void Marker(Sid sid,std::uint8_t type,Sid next) { auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=8;Put(raw,4,next.value,2); }
    // 실제Host Regular만연결하고burning/path/Frame은native와같은명시관찰경계다.
    DudeNoticeHooks Hooks() {
        auto hooks=MakeDudeNoticeHostHooks(pool,host,{});auto find=std::move(hooks.taskRegular),add=std::move(hooks.addTaskRegular);
        hooks.taskRegular=[this,find=std::move(find)](Sid sid) { events.push_back("R:50:65520:20736");return find(sid); };
        hooks.addTaskRegular=[this,add=std::move(add)](Sid sid) { events.push_back("N:40");auto* regular=add(sid);events.push_back("B:46:50:0:80");return regular; };
        hooks.createBurning=[this](Sid) { events.push_back("N:144");events.push_back("D:50:0");CHECK(pool.Allocate(2)==Sid{84});Marker(Sid{84},14,Sid{static_cast<std::uint16_t>(Get(pool.Slot(kSource),6,2))});Put(pool.AllocatedBytes(kSource),6,84,2); };
        hooks.pathEndpoint=[this](Sid) { return std::optional<std::array<float,2>>{Number(row[7])==1 ? std::array<float,2>{25,22} : std::array<float,2>{100,100}}; };
        hooks.startPath=[this](Sid sid,float x,float y) { std::string event="P:"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y));
            // Patch8/CD7의 실제 명령 인자만 직렬화하며 임의 reserved 값을 추가하지 않는다.
            for (int index=3;index<(patch ? 8 : 7);++index) event+=",0";
            event+=':'+std::to_string(std::bit_cast<std::uint32_t>(speeds[70].speed));events.push_back(event);return Number(row[8])!=0;
        };
        hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { events.push_back("F:50:"+std::to_string(frame)+':'+std::to_string(flags));Put(pool.AllocatedBytes(sid),patch ? 36U : 34U,static_cast<std::uint32_t>(frame),patch ? 4U : 1U); };return hooks;
    }
    // 원본대조출력은현재처리기를실행한뒤의숫자/순서/raw/실제Regular 상태다.
    std::vector<std::string> Run() {
        DudeNoticeState state{clock};RawDudeNotice logic(pool,hash,types,frames,speeds,residences,state,Hooks());std::string result="return";std::array<std::uint8_t,8> point{};
        switch (Number(row[1])) {
        case 0:logic.Around(20.75f,21.9f);break;
        case 1:result=std::to_string(logic.NearestResidence(kSource).value);break;
        case 2:{ const auto coordinates=logic.ResidencePoint(Sid{60});Put(point,0,std::bit_cast<std::uint32_t>(coordinates[0]));Put(point,4,std::bit_cast<std::uint32_t>(coordinates[1]));break; }
        default:throw std::logic_error("Dude notice fixture 종류오류");
        }
        std::string actions,regular="-";
        // 실제관찰호출을원본과같은시점순서로묶는다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        if (const auto* current=host.FindEventMasked(kSource,0xfff0,0x5100)) regular=std::to_string(std::bit_cast<std::uint64_t>(current->Time()))+','+std::to_string(current->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(current->Payload()))+','+std::to_string(current->Count());
        return {result,actions,Hex(pool.Slot(kSource)),regular,std::to_string(std::bit_cast<std::uint32_t>(speeds[70].speed)),Hex(point)};
    }
};
// 각원본의독립고유행을현재같은입력의실제처리기로전수재생한다.
void ReplayDude(std::string_view edition) {
    std::size_t count=0;int failures=0;
    // 원본출력은fixture의마지막6칸만사용하며입력에서기대값을계산하지않는다.
    for (const auto& row:DudeRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==18);DudeScene scene(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+12,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초차이는양쪽의원본/C++값을함께보여준다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s DudeNotice 행%zu/kind%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==DudeRows().size()/2);
}
}
// 실제10.78 전체notice/Regular쓰기/nearest/표적진입점과대피호출을대조한다.
TEST_CASE(DudeNotice_PatchNativeTaskAndRetreat) { ReplayDude("originals"); }
// CD10.72 인라인notice/currentdouble/이중speed/현재Iframe 분기를대조한다.
TEST_CASE(DudeNotice_CdNativeTaskAndRetreat) { ReplayDude("originalCD"); }
// 실제burning/Path 생성이없는호출을묵시성공으로바꾸지않는다.
TEST_CASE(DudeNotice_MissingBurningAndPathFactoriesRejected) {
    auto row=DudeRows().front();DudeScene scene(row);auto hooks=scene.Hooks();hooks.createBurning={};hooks.startPath={};DudeNoticeState state{scene.clock};RawDudeNotice logic(scene.pool,scene.hash,scene.types,scene.frames,scene.speeds,scene.residences,state,std::move(hooks));CHECK(Throws([&] { logic.Notice(kSource); }));CHECK(Get(scene.pool.Slot(kSource),12,2)==0x5107);CHECK(scene.host.FindEvent(kSource,0x5107)!=nullptr);CHECK(Throws([&] { logic.Move(kSource,{24.5f,21.5f}); }));
}
