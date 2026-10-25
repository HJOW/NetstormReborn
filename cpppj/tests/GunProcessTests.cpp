// 두 실제 PE가 정한 GunProcess raw/padding/사건/packet과 복원 몸체를 독립 비교한다.
#include "RawSceneSupport.h"
#include "o/RawGunProcess.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// fixture 입력은 little endian double의 64비트 그대로 기록한다.
std::uint64_t Wide(const std::string& value) { return std::stoull(value); }
// 저장된 native 관찰만 읽으며 기대값을 C++ 구현으로 계산하지 않는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_GUNPROCESS_FIXTURE).rows;return rows; }
// 실제 효과 대신 oracle과 같은 명시 외부 경계를 제공하는 검사 장면이다.
struct GunScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    GunProcessState state;
    // fields: 원본 프로세스의 직접 읽기·쓰기 필드 이미지. 입력 구성 또는 실행 결과 대조에 사용한다.
    GunProcessFields fields;
    // packet: 패킷. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::array<std::uint8_t,64> packet{};
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::string> events;
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    // exists: 존재 여부. 입력 구성 또는 실행 결과 대조에 사용한다.
    // shotResult: 발사 결과 여부. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch{},exists{},shotResult{};
    // animation: 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다 (Sid).
    // found: 암묵적인 런타임 SquidId 변환을 막는 원본 슬롯 번호다 (Sid).
    Sid animation{},found{};
    // random: 난수 선택에 사용하는 생성기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t random{};
    // native에 등장하는 raw SID만 확보하며 전체 풀 최소 크기는 원본 레이아웃을 따른다.
    explicit GunScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // native의 관찰 번호를 client 영역에 확보한다.
        for (std::uint16_t id=5;id<128;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // fixture 초기화만 수행하는 raw 접근이다. free 입력도 다음 행에서 다시 준비한다.
    std::span<std::uint8_t> FixtureSlot(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 입력 필드와 padding을 만들고 native 결과의 판단은 읽지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==34);events.clear();state={};
        // 원본 new padding과 패킷의 이전 바이트도 모두 보존한다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        for (std::size_t i=0;i<packet.size();++i) packet[i]=static_cast<std::uint8_t>(i*7+3);
        Put(fields.bytes,0,patch ? 0x50b668U : 0x501118U);fields.bytes[8]=static_cast<std::uint8_t>(Number(row[4]));
        const auto time=Wide(row[17]);Put(fields.bytes,16,static_cast<std::uint32_t>(time));Put(fields.bytes,20,static_cast<std::uint32_t>(time>>32));
        Put(fields.bytes,24,Number(row[3]),2);Put(fields.bytes,28,Number(row[2]));Put(fields.bytes,32,Number(row[5]));Put(fields.bytes,40,patch ? 0x50b648U : 0x5010f8U);Put(fields.bytes,44,70);Put(fields.bytes,48,50);
        auto parent=FixtureSlot(Sid{50});std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=static_cast<std::uint8_t>(Number(row[6]));parent[11]=static_cast<std::uint8_t>(Number(row[8]));
        parent[patch ? 34 : 32]=1;parent[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[7]));Put(parent,6,70,2);Put(parent,14,0x42480000);Put(parent,18,0x42500000);
        auto target=FixtureSlot(Sid{60});std::fill(target.begin(),target.end(),std::uint8_t{});target[11]=static_cast<std::uint8_t>(Number(row[9]));target[patch ? 34 : 32]=2;
        types[parent[10]].group=static_cast<std::int32_t>(Number(row[5]));state.now=std::bit_cast<double>(Wide(row[18]));
        state.boss=Number(row[10])!=0;state.server=Number(row[11])!=0;state.fortMode=Number(row[12])!=0;state.fixedMode=Number(row[13])!=0;state.disabled=Number(row[14])!=0;state.friendlyFire=Number(row[15])!=0;
        state.allies[1][2]=Number(row[16])!=0;found=Sid{static_cast<std::uint16_t>(Number(row[19]))};random=Number(row[20]);animation=Sid{static_cast<std::uint16_t>(Number(row[21]))};exists=Number(row[22])!=0;shotResult=Number(row[23])!=0;
    }
    // 각 현재 필드 재조회와 외부 호출의 인자/순서를 oracle과 같은 사건으로 기록한다.
    GunProcessHooks Hooks() {
        GunProcessHooks hooks;
        hooks.removeDependency=[this](Sid form,Sid target) { events.push_back("D:"+std::to_string(form.value)+':'+std::to_string(target.value)); };
        hooks.addDependency=[this](Sid target,GunProcessFields&) { events.push_back("A:"+std::to_string(target.value)); };
        hooks.repopForm=[this](GunProcessFields&) { events.push_back("P"); };
        hooks.animationForm=[this](Sid sid) { events.push_back("L:"+std::to_string(sid.value));return animation; };
        hooks.killAnimation=[this](Sid form,std::uint32_t flags) { if (exists) events.push_back("K:"+std::to_string(form.value)+':'+std::to_string(flags)); };
        hooks.refreshRange=[this](Sid sid) { events.push_back("R:"+std::to_string(sid.value)); };
        hooks.targetingMode=[this](Sid sid,std::uint8_t owner) { events.push_back("M:"+std::to_string(sid.value)+':'+std::to_string(owner));return 2U; };
        hooks.findTarget=[this](float x,float y,std::uint32_t type,std::uint32_t mode,std::uint8_t owner) { events.push_back("F:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(type)+':'+std::to_string(mode)+':'+std::to_string(owner));return found; };
        hooks.randomBetween=[this](float low,float high) { events.push_back("N:"+std::to_string(std::bit_cast<std::uint32_t>(low))+':'+std::to_string(std::bit_cast<std::uint32_t>(high)));return static_cast<double>(std::bit_cast<float>(random)); };
        hooks.startAnimation=[this](std::uint32_t stage,Sid form,Sid target) { events.push_back("S:"+std::to_string(stage)+':'+std::to_string(form.value)+':'+std::to_string(target.value)); };
        hooks.parentBlocked=[this](Sid sid) { return (pool.Slot(sid)[patch ? 40 : 35]&0x20)!=0; };
        hooks.shotContext=[this](Sid sid) {
            const auto raw=pool.Slot(sid);events.push_back("M:"+std::to_string(sid.value)+":1");events.push_back("C:"+std::to_string(sid.value));
            return std::array<std::uint32_t,8>{Get(raw,14),Get(raw,18),0x42500000,0x42540000,raw[10],2,raw[patch ? 34 : 32],Get(raw,patch ? 36U : 34U,patch ? 4U : 1U)};
        };
        hooks.shoot=[this](const std::array<std::uint32_t,8>& context,Sid target) {
            std::string event="H:";
            // 실제 descriptor의 모든 DWORD와 마지막 표적 인자를 기록한다.
            for (std::size_t i=0;i<context.size();++i) { if (i) event+=',';event+=std::to_string(context[i]); }
            events.push_back(event+','+std::to_string(target.value));return shotResult;
        };
        hooks.firingInterval=[](Sid) { return 2.25; };
        hooks.attach=[this](GunProcessFields& data,Sid parent) { events.push_back("T:34:"+std::to_string(parent.value)+":0:0");Put(data.bytes,44,70);Put(data.bytes,48,parent.value); };
        return hooks;
    }
    // 각 원본 진입점을 같은 fixture 입력으로 실행한다.
    std::uint32_t Run(const std::vector<std::string>& row,RawGunProcess& logic) {
        switch (Number(row[1])) {
        case 0:logic.Construct(fields,Sid{50});break;
        case 1:logic.Frame(fields);break;
        case 2:logic.ClearTarget(fields,Number(row[24])!=0);break;
        case 3:logic.SetTarget(fields,Sid{61});break;
        case 4:logic.Attached(fields);break;
        case 5:logic.Notify(fields,Number(row[25]),0x12345678,Number(row[26]));break;
        case 6:logic.Fire(fields);break;
        case 7:return logic.WriteData(fields,packet,70,Number(row[29]),Number(row[27])!=0,1,0x3a);
        case 8:return logic.WriteProcess(fields,packet,Number(row[28])!=0,Number(row[27])!=0,1,0x3a);
        case 9:logic.ReceiveUpdate(fields,static_cast<std::uint8_t>(Number(row[4])),Number(row[29]));break;
        default:throw std::logic_error("GunProcess fixture 진입점 오류");
        }
        return 0;
    }
    // 원본의 전체 raw/padding/packet과 사건 순서를 직렬화한다.
    std::vector<std::string> Output(std::uint32_t length) const {
        std::string actions;
        // 기록 순서를 그대로 합친다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {actions,Hex(fields.bytes),Hex(packet),std::to_string(length)};
    }
};
// 같은 판본의 모든 저장 native 행을 복원 결과와 비교한다.
void Replay(const char* edition,bool sharedAuthority=false) {
    RawAuthorityState authority;GunScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0; // owner를 먼저 선언하여 Scene/실행기보다 오래 보관한다.
    // 정상 반환당 전체 결과를 독립 비교한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);authority={scene.state.server,scene.state.boss}; // Native 입력만 같은 두 bool에 옮기고 기대값은 수정하지 않는다.
        if (sharedAuthority) { scene.state.sharedAuthority=&authority;scene.state.server=!authority.server;scene.state.boss=!authority.boss; } // 잘못된 fallback이 실제 관문으로 새지 않는지 독립 검증한다.
        RawGunProcess logic(scene.pool,scene.types,scene.state,scene.Hooks());const auto length=scene.Run(row,logic);
        const auto output=scene.Output(length);const auto expected=std::vector<std::string>(row.begin()+30,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 차이를 출력하고 기대값을 구현값으로 덮지 않는다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s Gun 행%zu/body%s 결과%zu 기대%s / 실제%s\n",edition,count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==Rows().size()/2);
}
}
// 패치 10.78의 raw 상태·권한·현재 표적·전송 선택을 비교한다.
TEST_CASE(GunProcess_PatchNativeBodies) { Replay("originals"); }
// CD 10.72의 다중 helper·시간 조건·같은 dirty 전송을 비교한다.
TEST_CASE(GunProcess_CdNativeBodies) { Replay("originalCD"); }
// 두 판본의 기존 native 상태/효과를 현재 방향별 DWORD getter로 다시 비교한다.
TEST_CASE(GunProcess_CurrentDirectionalAlliances) {
    GunScene patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);std::size_t compared=0,reads=0;
    // 표적을 검사하는 Frame 입력만 재사용하며 새 native 고유입력을 만들지 않는다.
    for (const auto& row:Rows()) {
        if (Number(row[1])!=1) continue;
        auto& scene=row[0]=="originals" ? patch : cd;scene.Prepare(row);
        std::array<std::uint32_t,81> current{}; // 실제 소비자는 bool로 복제하지 않은 방향별 DWORD를 읽는다.
        scene.state.allianceRead=[&](std::uint32_t owner,std::uint32_t target) { ++reads;return current.at(owner*9+target); };
        RawGunProcess logic(scene.pool,scene.types,scene.state,scene.Hooks());
        current[11]=Number(row[16]) ? 7U : 0U;current[19]=Number(row[16]) ? 0U : 9U;
        scene.state.allies[1][2]=Number(row[16])==0; // 반대값의 초기 사본을 쓰면 원본 결과가 달라진다.
        const auto length=scene.Run(row,logic);CHECK(scene.Output(length)==std::vector<std::string>(row.begin()+30,row.end()));++compared;
    }
    CHECK(compared>0 && reads>0);
}
// 현재 getter의 호출은 원본 boss/유효표적/죽음 분기 뒤이며 친군발사 조회보다 앞이다.
TEST_CASE(GunProcess_CurrentAllianceShortCircuit) {
    // PATCH와 CD의 동일한 짧은 평가 순서를 검사한다. native 고유입력 수에는 넣지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        GunScene scene(edition);scene.Prepare(Rows().front());
        auto parent=scene.FixtureSlot(Sid{50});auto target=scene.FixtureSlot(Sid{60});
        parent[10]=128;parent[scene.patch ? 40 : 35]=0;parent[scene.patch ? 34 : 32]=1;
        target[11]=0;target[scene.patch ? 34 : 32]=2;scene.types[128].group=1;
        Put(scene.fields.bytes,32,1);Put(scene.fields.bytes,24,60,2);Put(scene.fields.bytes,16,0);Put(scene.fields.bytes,20,0);
        scene.state.fixedMode=false;scene.state.disabled=false;scene.state.now=-1;scene.state.boss=false;
        std::size_t reads=0; // 원본의 선택된 한 방향 읽기 횟수다.
        scene.state.allianceRead=[&](std::uint32_t owner,std::uint32_t other) { CHECK(owner==1 && other==2);++reads;return 0U; };
        RawGunProcess logic(scene.pool,scene.types,scene.state,scene.Hooks());
        logic.Frame(scene.fields);CHECK(reads==0 && scene.fields.Target()==Sid{60});
        scene.state.boss=true;logic.Frame(scene.fields);CHECK(reads==1 && scene.fields.Target()==Sid{60});
        // 죽은 표적은 표를 읽기 전에 ClearTarget으로 이동한다.
        target[11]=2;logic.Frame(scene.fields);CHECK(reads==1 && scene.fields.Target().value==0);
        target[11]=0;Put(scene.fields.bytes,24,60,2);scene.state.friendlyFire=true;
        scene.state.allianceRead=[&](std::uint32_t,std::uint32_t) { ++reads;scene.state.friendlyFire=false;return 7U; };
        logic.Frame(scene.fields);CHECK(reads==2 && scene.fields.Target().value==0);
    }
}
// 미복원 검색/생성/효과를 noop로 성공시키지 않으며 packet 부족은 선행 변경 전에 거부한다.
TEST_CASE(GunProcess_MissingEffectAndPacketBounds) {
    GunScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());RawGunProcess logic(scene.pool,scene.types,scene.state,{});
    const auto before=scene.fields.bytes;CHECK(Throws([&] { logic.Construct(scene.fields,Sid{50}); }));CHECK(scene.fields.bytes==before);
    CHECK(Throws([&] { logic.SetTarget(scene.fields,Sid{61}); }));CHECK(scene.fields.bytes==before);
    CHECK(Throws([&] { logic.WriteData(scene.fields,std::span(scene.packet).first(8),70,50,true,1,0x3a); }));CHECK(scene.fields.bytes==before);
}
// 원본이 실제 외부 효과를 호출하지 않는 boss0/표적0/전송없음 Clear를 빈 경계로 재생한다.
TEST_CASE(GunProcess_ClearNaturalZeroBoundary) {
    std::size_t count=0;
    // 같은 native 입력 중 실제 외부 효과가 없는 정상 반환만 독립 비교한다.
    for (const auto& row:Rows()) {
        if (Number(row[1])!=2) continue;
        if (Number(row[10]) && (Number(row[3]) || (Number(row[24]) && Number(row[2])!=1))) continue;
        GunScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);scene.Prepare(row);
        RawGunProcess logic(scene.pool,scene.types,scene.state,{});const auto length=scene.Run(row,logic);
        CHECK(scene.Output(length)==std::vector<std::string>(row.begin()+30,row.end()));++count;
    }
    CHECK(count==42);
}
// 실제 flags0 factory·writer의 OnAttached 이전 base 필드·Kernel 실행·repop·form 삭제 수명을 검증한다.
TEST_CASE(GunProcess_ActualHostTransmitAndLifetime) {
    // 두 판본에서 같은 호스트 수명과 서로 다른 서버 SID 영역을 방문한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);types[128].group=1;
        Sid parent;
        // 부모 입력을 native와 같은 SID50에 확보한다.
        for (std::uint16_t id=5;id<=50;++id) parent=pool.Allocate(2);
        auto raw=pool.AllocatedBytes(parent);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=128;
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;
        SquidProcessState hostState;hostState.now=10;GunProcessState gunState;gunState.now=10;gunState.boss=false;
        std::array<std::uint8_t,64> buffer{};std::vector<std::vector<std::uint8_t>> packets;std::vector<bool> changed;
        ProcessFormTransmission transmission;
        transmission.writeData=[&](SquidProcess& process,bool parentChanged) {
            auto* gun=dynamic_cast<GunProcess*>(&process);CHECK(gun!=nullptr);if (!gun) throw std::logic_error("Gun host 직렬화 타입 오류");
            changed.push_back(parentChanged);CHECK(pool.Slot(process.Form())[11]&4);
            return static_cast<std::int32_t>(gun->WritePacket(buffer,parentChanged));
        };
        transmission.transmit=[&](std::int32_t length) { packets.emplace_back(buffer.begin(),buffer.begin()+length); };
        SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmission);RawGunProcess logic(pool,types,gunState,{});
        auto* gun=CreateGunProcess(host,logic,parent);CHECK(gun!=nullptr);if (!gun) throw std::logic_error("Gun flags0 부착 실패");
        const auto form=gun->Form();CHECK(form.value==pool.Layout().serverFirst && host.FromForm(form)==gun && kernel.Size()==1);
        CHECK(gun->Fields().Form()==form && gun->Fields().Parent()==parent && gun->Fields().Dirty()==0 && gun->Fields().Stage()==0);
        CHECK(packets.size()==1 && changed==std::vector<bool>{true});CHECK(packets[0].size()==31 && Get(packets[0],2,2)==31 && Get(packets[0],4)==form.value && Get(packets[0],9)==parent.value);
        CHECK(packets[0][0]==1 && packets[0][1]==0x3a && packets[0][8]==0xff && Get(packets[0],21,2)==0 && Get(packets[0],23)==0 && Get(packets[0],27)==1);
        CHECK(Get(pool.Slot(parent),6,2)==form.value && (pool.Slot(form)[11]&0xc)==8);
        kernel.RunFrame();CHECK(gun->Fields().Stage()==1);
        Put(gun->Fields().bytes,28,3);gun->Fields().bytes[8]=8;host.RefreshForm(*gun);
        CHECK(gun->Fields().Stage()==3 && gun->Fields().Dirty()==0 && packets.size()==2 && !changed[1] && packets[1].size()==13 && Get(packets[1],9)==3);
        host.Kill(*gun,0x40);CHECK(kernel.Size()==0 && Get(pool.Slot(parent),6,2)==0 && (pool.Slot(form)[11]&3));
        CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0);
    }
}

// 기존 두 PE의 모든 raw/packet/효과 기대값으로 같은 공유 권한 경로를 대조한다.
TEST_CASE(GunProcess_SharedAuthorityReplaysBothNativeBodies) { Replay("originals",true);Replay("originalCD",true); }
// 생성 이후 바뀐 실제 boss/server를 각 Clear 호출이 다시 읽으며 잘못된 fallback은 무시한다.
TEST_CASE(GunProcess_SharedAuthorityChangesWithoutShadowSynchronization) {
    RawAuthorityState authority{false,false};GunScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());scene.state.sharedAuthority=&authority;scene.state.boss=scene.state.server=true;
    Put(scene.fields.bytes,24,0,2);Put(scene.fields.bytes,28,1);scene.fields.bytes[8]=0;const auto before=scene.fields.bytes;RawGunProcess logic(scene.pool,scene.types,scene.state,{});
    logic.ClearTarget(scene.fields,false);CHECK(scene.fields.bytes==before);authority.boss=true;CHECK(Throws([&] { logic.ClearTarget(scene.fields,false); }));
    authority.server=true;logic.ClearTarget(scene.fields,false);CHECK(scene.fields.Dirty()==4 && !scene.fields.Target().value && scene.fields.Stage()==1 && scene.state.boss && scene.state.server); // 자연 빈 표적도 원문 target dirty4는 쓰고 효과 장치는 요구하지 않는다.
}
