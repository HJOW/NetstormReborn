// 실제AI0xb08생성/17목록소멸과Player/지역/방향별동맹을원본기계어전체출력에대조한다.
#include "RawSceneSupport.h"
#include "o/RawAiBrain.h"
#include "o/RawAiCreate.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kBrainBase=0x13000000; //실제소유메모리fixture의첫opaque32핸들이다.
// 선택되지않은Player생성/삭제부모서비스는빈성공을공급하지않는다.
[[noreturn]] void BrainMissing() { throw std::logic_error("AI생성자검사에서선택되지않은실제하위입니다"); }
// 명시사제좌표입력만Pool raw에쓴다. 제품은기존실제세계의같은기록을읽는다.
std::span<std::uint8_t> BrainInput(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
// 실제원문/메모리/동맹/섬하위몸체를한번의장면수명에소유한다.
struct BrainScene {
    std::vector<std::int64_t> input; //현재행의독립input9개다.
    SidPool pool{OriginalEdition::Patch1078,32768,true}; //원본SID유효성경계다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //공통Post의실제188개표다. 선택ctor입력은앞48개다.
    std::array<std::string,188> energy; //TYPE+a0의별도실제문자열입력이다.
    SquidPostPopState book; //실제PlayerWorld계약의공통장부다.
    SquidPostPop post{pool,types,book}; //같은실제Pool을공유한다.
    std::array<std::uint8_t,1620> records{}; //모든모듈의같은실제Player9다.
    PlayerWorldState playerState; //조회/생성의기존현재PlayerKnown/owner다.
    std::unique_ptr<RawPlayerWorld> player; //실제Technology/첫지역저장소를공유한다.
    std::array<std::uint32_t,81> allies{}; //실제방향별표이며대칭으로새로만들지않는다.
    std::array<std::uint8_t,9> hidden{}; //선택되지않은Chat필드도원본크기로유지한다.
    PlayerParticipationState participationState; //현재local/metamap/battle을공유한다.
    std::unique_ptr<RawPlayerParticipation> participation; //실제SetAlliance/AddAlly/수명을사용한다.
    SquidHash hash; //빈실제지면이며좌표지역반환만별도경계다.
    std::array<std::uint8_t,3204> islandBytes{}; //실제100개IslandList원문이다.
    FortFinishOwnerState modes; //지역owner의현재fort/special/chaff전역이다.
    SquidPostPopList residences; //이번장면은빈실제레지던스목록이다.
    std::uint32_t revision{},ownerChanged{}; // 현재 실제 섬/소유자 변경 전역이다.
    std::uint8_t invalidGraph{254}; // 실제 장부와 같은 BYTE backing이며 DWORD 임시 참조를 만들지 않는다.
    std::int32_t noRegion{127}; //실제첫지역없음전역이다.
    std::unique_ptr<RawIslandRuntime> islands; //현재실제존재/owner/Player첫지역몸체다.
    std::size_t allocationIndex{}; //각실제할당의독립미초기화byte패턴번호다.
    std::unique_ptr<RawAiBrainMemory> memory; //본체/목록의실제host소유저장소다.
    std::unique_ptr<RawAiBrainConstructor> runtime; //동일자료에실제생성자/소멸자를연결한다.
    std::array<std::uint8_t,600> profile{}; //실제600byte설정원문입력이다.
    std::string trace; //실제사제/좌표/에너지경계의순서다.
    // 원본과같은입력패턴을준비하며기대필드/동맹결과를계산하지않는다.
    explicit BrainScene(std::string_view values) {
        // DWORD와음수지역을손실없이읽는다.
        for (const auto& value:Split(values,',')) input.push_back(std::stoll(value));CHECK(input.size()==9);playerState.battleOwner=static_cast<std::uint32_t>(input[1]);participationState.localOwner=playerState.battleOwner;Put(records,static_cast<std::size_t>(input[0])*180+0x78,static_cast<std::uint32_t>(input[2]));
        // 원본과같은미초기화600byte설정입력이다.
        for (std::size_t index=0;index<profile.size();++index) profile[index]=static_cast<std::uint8_t>(index*37+input[8]);Put(profile,0x23c,static_cast<std::uint32_t>(input[4]));Put(profile,0x240,0);Put(profile,0x244,0);Put(profile,0x248,static_cast<std::uint32_t>(input[3]));
        // 초기표의각방향은독립7/0값이며원본과같은입력이다.
        for (std::size_t owner=0;owner<9;++owner) for (std::size_t target=0;target<9;++target) allies[owner*9+target]=owner==target ? 1U : (owner+target)%3==0 ? 7U : 0U;
        PlayerWorldHooks playerHooks;playerHooks.assertion=[](PlayerWorldAssertion) { BrainMissing(); };playerHooks.find=[](float,float,std::uint32_t)->Sid { BrainMissing(); };playerHooks.create=[](std::uint32_t,std::uint32_t)->Sid { BrainMissing(); };playerHooks.setOwner=[](Sid,std::uint32_t) { BrainMissing(); };playerHooks.pop=[](Sid,float,float,std::uint32_t) { BrainMissing(); };playerHooks.reserveRegular=[]()->bool { BrainMissing(); };playerHooks.addRegular=[](Sid,std::uint32_t,float) { BrainMissing(); };player=std::make_unique<RawPlayerWorld>(pool,post,records,playerState,playerHooks);
        PlayerParticipationHooks partHooks;partHooks.destructBrain=[this](std::uint32_t token) { runtime->Destruct(token); };partHooks.freeBrain=[this](std::uint32_t token) { runtime->FreeBody(token); };participation=std::make_unique<RawPlayerParticipation>(OriginalEdition::Patch1078,records,allies,hidden,participationState,partHooks);
        Put(islandBytes,0,4);
        // 존재/미존재4개기록과유효bounds입력만준비한다.
        for (std::size_t index=0;index<4;++index) { Put(islandBytes,4+index*32+4,index<3 ? 1U : 0U);Put(islandBytes,4+index*32+8,4);Put(islandBytes,4+index*32+16,std::bit_cast<std::uint32_t>(1.0F));Put(islandBytes,4+index*32+20,std::bit_cast<std::uint32_t>(1.0F));Put(islandBytes,4+index*32+24,std::bit_cast<std::uint32_t>(10.0F));Put(islandBytes,4+index*32+28,std::bit_cast<std::uint32_t>(10.0F)); }
        IslandRuntimeHooks islandHooks;islandHooks.setResidenceOwner=[](Sid,std::uint32_t) { BrainMissing(); };islandHooks.assertion=[](IslandRuntimeAssertion) { BrainMissing(); };islands=std::make_unique<RawIslandRuntime>(pool,hash,islandBytes,modes,residences,revision,ownerChanged,islandHooks);islands->BindPlayerRecords(records);
        if (input[6]>0 && input[6]<32768) { auto raw=BrainInput(pool,Sid{static_cast<std::uint16_t>(input[6])});Put(raw,14,std::bit_cast<std::uint32_t>(5.75F));Put(raw,18,std::bit_cast<std::uint32_t>(6.25F)); }
        if (input[5]==1) { types[3].flags2=0x40800;energy[3]="ws";types[4].flags2=0x800;energy[4]="Ss";types[5].flags2=0x40000;energy[5]="ss"; }else if (input[5]==2) { types[0].flags2=0x40800;energy[0]="ss"; }
        memory=std::make_unique<RawAiBrainMemory>(kBrainBase,[this](std::span<std::uint8_t> bytes) {
            // 실제할당의caller입력byte이며기대초기화결과가아니다.
            for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*29+input[8]+allocationIndex*17);++allocationIndex;
        });runtime=std::make_unique<RawAiBrainConstructor>(*memory,*player,*participation,*islands,allies,std::span<const RiftTypeRecord>(types).first(48),Globals(),Hooks());
    }
    // 실제 BYTE의 현재 값을 필요할 때 확장하며 세션처럼 같은 backing을 계속 빌린다.
    AiBrainConstructorGlobals Globals() { return {playerState.battleOwner,noRegion,[this] { return invalidGraph; }}; }
    // 현재경계가실제로요청한순서만기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원본의외부몸체만명시입력에연결한다.
    AiBrainConstructorHooks Hooks() {
        AiBrainConstructorHooks hooks;hooks.priest=[this](std::uint32_t owner) { Add("P,"+std::to_string(owner));return Sid{static_cast<std::uint16_t>(input[6])}; };hooks.regionAt=[this](float x,float y) { Add("R,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y)));return static_cast<std::int32_t>(input[7]); };hooks.requiredEnergy=[this](std::uint32_t type) { Add("E,"+std::to_string(type));return energy.at(type); };return hooks;
    }
    // 방향별전체DWORD를실제byte순서로보존한다.
    std::string Allies() const { std::array<std::uint8_t,324> raw{};for (std::size_t index=0;index<allies.size();++index) Put(raw,index*4,allies[index]);return Hex(raw); }
    // 소유자의실제할당순서/크기/전체byteAdler를직렬화한다.
    std::string Blocks() const { std::string result;for (const auto& block:memory->Allocations()) { if (!result.empty()) result+=';';result+=std::to_string(block.token)+','+std::to_string(block.size)+','+std::to_string(Adler(memory->Bytes(block.token))); }return result; }
    // 실제해제순서를원본CRT관찰과같은표현으로쓴다.
    std::string Frees() const { std::string result;for (const auto token:memory->Frees()) { if (!result.empty()) result+=',';result+=std::to_string(token); }return result; }
};
}
// 원본전체body/Player9/방향별81DWORD/섬3204byte/실제17할당·역순free를160고유입력에대조한다.
TEST_CASE(AiBrain_Native160) {
    std::size_t count=0;std::set<std::string> unique;
    // 기대출력은실행이후에만읽는다.
    for (const auto& row:LoadFixture(NETSTORM_AI_BRAIN_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==8);CHECK(unique.insert(row[0]).second);BrainScene scene(row[0]);const auto token=scene.memory->Allocate(RawAiBrainConstructor::kBytes);scene.runtime->Initialize(token,static_cast<std::uint32_t>(scene.input[0]),scene.profile);CHECK(Hex(scene.runtime->Bytes(token))==row[1]);CHECK(Hex(scene.records)==row[2]);CHECK(scene.Allies()==row[3]);CHECK(Hex(scene.islandBytes)==row[4]);CHECK(scene.Blocks()==row[5]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[7]);scene.runtime->Destruct(token);CHECK(scene.Frees()==row[6]);CHECK(scene.memory->Contains(token));CHECK(Hex(scene.runtime->Bytes(token))==row[1]);scene.runtime->FreeBody(token);CHECK(!scene.memory->Contains(token));++count;
    }
    CHECK(count==160);
}
// 실제생성소유자를참여에전달하며초기update누락은먼저중단하고예외시모든소유메모리를정리한다.
TEST_CASE(AiBrain_OwnedLifetimeAndMissingInitialUpdate) {
    BrainScene scene("1,8,5,0,256,1,70,2,19");AiCreateHooks hooks;hooks.construct=[&](std::uint32_t owner,std::span<const std::uint8_t> settings,std::size_t bytes) { return scene.runtime->Construct(owner,settings,bytes); };RawAiCreate wrapper(OriginalEdition::Patch1078,*scene.participation,hooks);bool rejected=false;try { wrapper.Create(1,scene.profile); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.memory->Allocations().empty() && scene.participation->Brain(1)==0);
    const auto token=scene.runtime->Construct(1,scene.profile);CHECK(scene.memory->Allocations().size()==18);CHECK(scene.participation->Brain(1)==0 && (scene.records[180+0x50]&0x20)==0);scene.participation->AttachBrain(1,token);scene.participation->RemoveBrain(1);CHECK(!scene.memory->Contains(token) && scene.participation->Brain(1)==0 && (scene.records[180+0x50]&0x20)!=0);CHECK(scene.memory->Frees().size()==18);
    BrainScene missing("1,8,5,1,256,1,70,2,19");auto missingHooks=missing.Hooks();missingHooks.requiredEnergy={};RawAiBrainConstructor incomplete(*missing.memory,*missing.player,*missing.participation,*missing.islands,missing.allies,std::span<const RiftTypeRecord>(missing.types).first(48),missing.Globals(),missingHooks);rejected=false;try { incomplete.Construct(1,missing.profile); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.memory->Allocations().size()==18 && missing.memory->Frees().size()==18);CHECK(missing.participation->Brain(1)==0 && (missing.records[180+0x50]&0x20)==0);
}
// 생성자는 각 원본 쓰기 위치에서 BYTE를 다시 읽고 두 조회 사이의 실제 초기화 순서를 지킨다.
TEST_CASE(AiBrain_ReadsCurrentByteAtBothGraphWrites) {
    BrainScene scene("1,8,5,0,256,1,70,2,19");auto globals=scene.Globals();
    const auto token=scene.memory->Allocate(RawAiBrainConstructor::kBytes);std::uint32_t calls=0;
    globals.invalidGraph=[&] {
        ++calls;const auto value=scene.invalidGraph;
        if (calls==1) scene.invalidGraph=17;
        else { CHECK(Get(scene.memory->Bytes(token),0x298)==254);CHECK(Get(scene.memory->Bytes(token),0x2a8)==0); }
        return value;
    };
    RawAiBrainConstructor runtime(*scene.memory,*scene.player,*scene.participation,*scene.islands,scene.allies,
        std::span<const RiftTypeRecord>(scene.types).first(48),std::move(globals),scene.Hooks());
    runtime.Initialize(token,1,scene.profile);CHECK(calls==2);
    CHECK(Get(runtime.Bytes(token),0x298)==254 && Get(runtime.Bytes(token),0x2ac)==17);
    runtime.Destruct(token);runtime.FreeBody(token);CHECK(!scene.memory->Contains(token));
}
// 현재 조회기가 없으면 실제 본체를 확보하기 전에 연결 오류를 보고한다.
TEST_CASE(AiBrain_RejectsMissingCurrentGraphGetter) {
    BrainScene scene("1,8,5,0,256,1,70,2,19");auto globals=scene.Globals();globals.invalidGraph={};bool rejected=false;
    try {
        RawAiBrainConstructor invalid(*scene.memory,*scene.player,*scene.participation,*scene.islands,scene.allies,
            std::span<const RiftTypeRecord>(scene.types).first(48),std::move(globals),scene.Hooks());
    } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && scene.memory->Allocations().empty());
}
