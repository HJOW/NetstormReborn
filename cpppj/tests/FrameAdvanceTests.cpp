// 실제 프레임 진행 관찰과 표시/공간/사제 낙하 예약의 모듈 합성을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawFrameAdvance.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidFactory.h"
#include "o/RawPriestFall.h"
#include "o/RawPriestOwner.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기가 입력한 타입/가상 표/SID와 판본별 독립 행 수다. 가상 표는 호출 가능한 호스트 주소가 아니다.
constexpr std::uint32_t kType=82,kVtable=0x11010000;
constexpr Sid kObject{50};
constexpr std::size_t kRows=7680;
// 실제 PE에 입력한 여덟 SHP 추가 헤더의 칸 크기다.
constexpr std::array<std::array<float,2>,8> kSizes={{{1,1},{2,2},{2.5f,1},{3,4},{4,4.5f},{0.5f,9},{2,1.5f},{4,4}}};
// 실제 PE에 입력한 flags1/flags2/프레임 수다.
constexpr std::array<std::array<std::uint32_t,3>,4> kKinds={{{0x802,4,8},{0x28000803,2,8},{0x28000003,0x1000000,8},{0x69012,0x210000,4}}};
// 프레임 코드의 동일한 입력을 만든다. 구간 첫 번호/개수/진행 결과는 생산 모듈이 계산한다.
RiftTypeFrames Codes(std::string_view letters) {
    std::vector<FrameCode> codes;
    // number/variant가 다른 글자 코드도 진행은 side의 구간을 사용한다.
    for (std::size_t i=0;i<letters.size();++i) codes.push_back({static_cast<std::uint8_t>(letters[i]),
        static_cast<std::uint8_t>(80+i%2),static_cast<std::uint8_t>(11-i),static_cast<std::uint8_t>(i&1)});
    return RiftTypeFrames(std::move(codes));
}
// 원본 PE/Python 없이 커밋된 독립 관찰을 재생한다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_FRAMEADVANCE_FIXTURE);return data.rows;
}
// 한 판본의 반환/순서/전체 슬롯을 실제 진행→지정으로 대조한다.
void Replay(const char* edition) {
    const bool patch=std::string_view(edition)=="originals";
    SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,false);
    // 원본 SID 50이 실제 풀의 활성 할당 범위에 들도록 확보한다.
    for (std::uint16_t sid=5;sid<=kObject.value;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);
    std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));
    std::size_t count=0;CHECK(Fixture().size()==kRows*3);
    // 저장된 기대값을 다시 계산하지 않고 입력을 생산 코드에 넘긴다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==10);
        const auto kind=kKinds.at(Number(row[2]));types[kType].flags1=kind[0];types[kType].flags2=kind[1];
        frames[kType]=Codes(Number(row[1])==0 ? "AAABBJJJ" : "JAJBAPJP");
        auto raw=pool.AllocatedBytes(kObject);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=kType;Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        Put(raw,patch ? 36 : 34,Number(row[4]),patch ? 4 : 1);raw[patch ? 33 : 31]=static_cast<std::uint8_t>(Number(row[3]));
        std::vector<std::string> events;
        // 모든 외부 효과에 호출 당시의 프레임을 붙여 지정 시점과 순서를 보존한다.
        const auto current=[&] { return std::to_string(Get(pool.Slot(kObject),patch ? 36 : 34,patch ? 4 : 1)); };
        SquidFrame setter(pool,types,{
            // 패치 헤더 helper의 현재 프레임 범위만 검사한다. 새 목표 프레임은 원본도 검사하지 않는다.
            [&](Sid,std::int32_t frame) {
                if (patch) CHECK(frame>=0 && frame<static_cast<std::int32_t>(kind[2]*((kind[0]&0x440000) ? 2U : 1U)));
                return kSizes.at(static_cast<std::size_t>(frame));
            },
            // 표시 대체는 실제 지정이 호출한 옛/새 프레임을 그대로 기록한다.
            [&](Sid sid,std::uint32_t flags) { events.push_back(std::string(flags ? "E:" : "D:")+std::to_string(sid.value)+':'+current()); },
            // 가상 Unpop 대체는 같은 flags와 쓰기 전 프레임을 기록한다.
            [&](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+current()); },
            // +0x4c 넘김 뒤 Pop 인자와 쓰기 후 프레임을 기록한다.
            [&](Sid sid,float x,float y,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+
                std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)+':'+current()); }});
        RawFrameAdvance advance(pool,frames,setter);
        const auto result=advance.Advance(kObject,static_cast<std::int32_t>(std::stoll(row[5])),Number(row[6]));
        std::string observed;
        // 발생한 효과 순서를 합친다.
        for (const auto& event:events) { if (!observed.empty()) observed+=';';observed+=event; }
        if (observed.empty()) observed="-";
        const bool same=static_cast<unsigned>(result)==Number(row[7]) && observed==row[8] && Hex(pool.Slot(kObject))==row[9];
        CHECK(same);++count;
        if (!same) { std::printf("%s 프레임 진행 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[8].c_str(),observed.c_str());break; }
    }
    CHECK(count==kRows);
}
// 원본 변경 영역과 그 시점의 프레임을 관찰하는 콘솔 표시 대상이다.
struct Sink final:SquidDisplaySink {
    SidPool& pool;Sid object{};std::vector<std::uint32_t> frames;std::vector<SquidDisplayRect> rects;
    // 표시 대상은 프레임 지정기와 같은 풀을 읽는다.
    explicit Sink(SidPool& input):pool(input) {}
    // 이 검사는 매 프레임 변경 영역을 관찰한다.
    bool Suppressed() const override { return false; }
    // 표시 계산기의 원본 사각형을 바꾸지 않고 현재 프레임과 함께 기록한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t) override {
        const bool patch=pool.Edition()==OriginalEdition::Patch1078;
        frames.push_back(Get(pool.Slot(object),patch ? 36 : 34,patch ? 4 : 1));rects.push_back(rect);
    }
    // 다음 진행의 옛/새 표시만 구별하기 위해 관찰을 비운다.
    void Clear() { frames.clear();rects.clear(); }
};
}
// 패치 DWORD/한 번 보정/비연속 구간의 전체 원본 관찰을 재생한다.
TEST_CASE(FrameAdvance_ReplaysOriginals) { Replay("originals"); }
// CD BYTE 프레임과 실제 방향 helper/지정의 관찰을 재생한다.
TEST_CASE(FrameAdvance_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37도 별도 PE의 원본 관찰을 재생한다.
TEST_CASE(FrameAdvance_ReplaysExtra1037) { Replay("original1037"); }

// 실제 공통 Pop/Unpop/표시로 진행의 한 호출 늦은 해시 이동과 반환을 확인한다.
TEST_CASE(FrameAdvance_ComposesRealDisplayAndSpace) {
    // DWORD/BYTE 두 배치에서 실제 생성자와 공간 모듈을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        // 원본에서 일반 Pop/postPop 경로가 확인된 연결 타입이다.
        constexpr std::uint32_t type=155;
        types[type].constructorAddress=TypeConstructorAddress(edition,type);types[type].flags1=0x28000002;
        types[type].flags2=0x2000000;types[type].footX=types[type].footY=1;
        std::vector<RiftTypeFrames> codes(types.size(),RiftTypeFrames({}));codes[type]=Codes("AAAA");
        std::vector<SquidDisplayShape> shapes(types.size());shapes[type]={4,true,{{11,17,12,16,1,1},{26,10,5,2,2,1.5f},{8,4,2,1,5,5},{6,3,1,0,6,6}}};
        Sink sink(pool);SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);
        SquidFactory factory(pool,types,false,&unpop);sink.object=factory.Create(type);
        CHECK(pop.Pop(sink.object,types[type],1,1,20,20)==RawPopResult::Registered);
        SquidFrame setter(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));RawFrameAdvance advance(pool,codes,setter);
        sink.Clear();CHECK(!advance.Advance(sink.object,1));CHECK(sink.frames==(std::vector<std::uint32_t>{0,1}));
        CHECK(sink.rects.size()==2 && sink.rects[0].left!=sink.rects[1].left);
        sink.Clear();CHECK(!advance.Advance(sink.object,1));CHECK(sink.frames==(std::vector<std::uint32_t>{1,2}));
        CHECK(hash.Bucket(1,20,20)==sink.object.value && pool.Slot(sink.object)[patch ? 33 : 31]==1);
        sink.Clear();CHECK(!advance.Advance(sink.object,1));CHECK(sink.frames==(std::vector<std::uint32_t>{2,3}));
        CHECK(hash.Bucket(1,20,20)==0 && hash.Bucket(3,20,20)==sink.object.value && pool.Slot(sink.object)[patch ? 33 : 31]==3);
        sink.Clear();CHECK(advance.Advance(sink.object,1));CHECK(sink.frames==(std::vector<std::uint32_t>{3,0}));
        CHECK(hash.Bucket(3,20,20)==sink.object.value);
        sink.Clear();CHECK(!advance.Advance(sink.object,1));CHECK(sink.frames==(std::vector<std::uint32_t>{0,1}));
        CHECK(hash.Bucket(3,20,20)==0 && hash.Bucket(1,20,20)==sink.object.value && (pool.Slot(sink.object)[11]&4)==0);
    }
}

// 실제 사제 0x25b/SharedRegular/Kernel에 프레임 지정/진행/공통 표시를 연결한다.
TEST_CASE(FrameAdvance_ComposesPriestFallSchedulingAndLanding) {
    // 일반 사제 Pop은 경계로 유지하면서 실제 사제 vtable의 이벤트를 두 배치에서 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);
        // 사제 입력 SID와 프로세스 form의 실제 번호 정책을 유지한다.
        for (std::uint16_t sid=5;sid<=kObject.value;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 원본 priest 타입과 가상 표의 주소 기록값이다.
        constexpr std::uint32_t priest=158;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[priest].maxHitPoints=200;
        types[priest].flags1=0x69012;types[priest].flags2=0x210000;types[priest].footX=types[priest].footY=1;
        std::vector<RiftTypeFrames> codes(types.size(),RiftTypeFrames({}));codes[priest]=Codes("AABJJJ");
        std::vector<PriestFallFrames> choices(types.size());choices[priest]={3,0};
        std::vector<std::uint8_t> spots(65536,2);std::vector<std::uint16_t> surfaces(65536);
        auto raw=pool.AllocatedBytes(kObject);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);
        raw[10]=priest;raw[patch ? 33 : 31]=1;Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));Put(raw,26,150,patch ? 4 : 2);
        std::vector<SquidDisplayShape> shapes(types.size());shapes[priest].frameCount=6;shapes[priest].loaded=true;
        // 실제 main/그림자 계산을 사용할 수 있도록 두 레이어의 합성 SHP 입력을 만든다.
        for (int i=0;i<12;++i) shapes[priest].frames.push_back({static_cast<std::int16_t>(12+i),20,3,18,1,1});
        Sink sink(pool);sink.object=kObject;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidHash hash;hash.Bucket(1,20.75f,21.9f)=kObject.value;SquidUnpop unpop(pool,hash,spots,&display);RawSquidDestroy destroy(pool,unpop,types);
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidPostPopState book;SquidRewardState hpMode;
        SquidReward hp(pool,types,book,hpMode,patch ? &store : nullptr);RawPriestState state(pool,hp,types,spots);
        SquidFrame setter(pool,types,{[&](Sid sid,std::int32_t frame) { return display.FrameSize(edition,pool.Slot(sid),frame); },
            [&](Sid sid,std::uint32_t flags) { display.Update(pool.Slot(sid),flags); },
            [&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[priest],flags); },
            [](Sid,float,float,std::uint32_t) { CHECK(false); }});
        RawFrameAdvance advance(pool,codes,setter);Kernel kernel;SquidProcessState clock;clock.now=12.5;
        PriestFallState mode{false};std::unique_ptr<RawPriestFall> fall;int shieldCalls=0,soundCalls=0,clearCalls=0;
        SquidProcessHost host(pool,types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            return MakePriestFallHandler(pool,*fall)(sid,event,count,payload);
        });
        PriestFallHooks hooks{[&](Sid) { ++shieldCalls; },[] { return true; },
            [&](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); },
            [](Sid,std::uint32_t) { CHECK(false); },[](Sid,std::uint32_t) { CHECK(false); },
            [&](Sid,float,float) { ++soundCalls; },{},{},[&](Sid) { ++clearCalls; }};
        fall=std::make_unique<RawPriestFall>(pool,state,codes,choices,surfaces,mode,MakePriestFallFrameHooks(setter,advance,std::move(hooks)));
        CHECK(fall->TryFall(kObject));CHECK(kernel.Size()==1 && shieldCalls==1 && soundCalls==1);
        auto* process=host.FindEvent(kObject,kPriestFallEvent);CHECK(process && pool.Slot(process->Form())[10]==61);
        kernel.RunFrame();CHECK(sink.frames==(std::vector<std::uint32_t>{0,0,3,3}));
        CHECK(process->Count()==1 && process->Payload()==0.1f && fall->TryFall(kObject) && kernel.Size()==1 && soundCalls==1);
        // 3→4→5→3의 실제 한 번 보정과 원본 0.1f 재예약을 관찰한다.
        for (const std::uint32_t expected:{4U,5U,3U}) {
            clock.now=process->Time();sink.Clear();kernel.RunFrame();CHECK(sink.frames.size()==4 && sink.frames[2]==expected);
            CHECK(process->Payload()==0.1f && kernel.Size()==1);
        }
        surfaces[22*256+21]=65535;clock.now=process->Time();sink.Clear();kernel.RunFrame();
        CHECK(sink.frames==(std::vector<std::uint32_t>{3,3,4,4,4,4,0,0}));
        CHECK(kernel.Size()==0 && !host.FindEvent(kObject,kPriestFallEvent) && clearCalls==1 && shieldCalls==1 && soundCalls==1);
        CHECK(Get(pool.Slot(kObject),patch ? 36 : 34,patch ? 4 : 1)==0 && hash.Bucket(1,20.75f,21.9f)==kObject.value);
        CHECK(!SquidPop::Supports(edition,patch ? kPatchPriestVtable : kCdPriestVtable,0));
    }
}

// 잘못된 풀/지정기/프레임/방향은 raw/표시를 바꾸기 전에 거부하고 현재 자산 표 교체는 반영한다.
TEST_CASE(FrameAdvance_RejectsInvalidBindingsAndReadsCurrentFrames) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false),other(OriginalEdition::Patch1078,kCapacity,false);
    // 두 풀에 같은 숫자 SID가 있어도 참조가 다르면 연결할 수 없다.
    for (std::uint16_t sid=5;sid<=kObject.value;++sid) { CHECK(pool.Allocate(2)==Sid{sid});CHECK(other.Allocate(2)==Sid{sid}); }
    std::vector<RiftTypeRecord> types(188);types[kType].flags2=4;
    std::vector<RiftTypeFrames> codes(types.size(),RiftTypeFrames({}));codes[kType]=Codes("AAAB");int updates=0;
    SquidFrameHooks hooks{[](Sid,std::int32_t) { return std::array<float,2>{1,1}; },[&](Sid,std::uint32_t) { ++updates; },
        [](Sid,std::uint32_t) { CHECK(false); },[](Sid,float,float,std::uint32_t) { CHECK(false); }};
    SquidFrame setter(pool,types,hooks),second(pool,types,hooks);RawFrameAdvance advance(pool,codes,setter);
    CHECK(Throws([&] { RawFrameAdvance wrong(other,codes,setter); }));CHECK(Throws([&] { RawFrameAdvance wrong(pool,{},setter); }));
    CHECK(Throws([&] { MakePriestFallFrameHooks(second,advance,{}); }));
    auto raw=pool.AllocatedBytes(kObject);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=kType;
    CHECK(!advance.Advance(kObject,1) && Get(raw,36)==1 && updates==2);
    codes[kType]=Codes("ABBB");CHECK(!advance.Advance(kObject,1) && Get(raw,36)==2 && updates==4);
    Put(raw,36,3);codes[kType]=Codes("ABQZ");auto before=Hex(raw);
    CHECK(Throws([&] { advance.Advance(kObject,1); }) && Hex(raw)==before && updates==4);
    Put(raw,36,0xffffffff);before=Hex(raw);CHECK(Throws([&] { advance.Advance(kObject,1); }) && Hex(raw)==before && updates==4);
    Put(raw,36,0);codes[kType]=Codes("ABBB");before=Hex(raw);CHECK(Throws([&] { advance.Advance(kObject,1,3); }) && Hex(raw)==before && updates==4);
    raw[10]=187;RawFrameAdvance shortTable(pool,std::span<const RiftTypeFrames>(codes).first(100),setter);before=Hex(raw);
    CHECK(Throws([&] { shortTable.Advance(kObject,1); }) && Hex(raw)==before && updates==4);
}
