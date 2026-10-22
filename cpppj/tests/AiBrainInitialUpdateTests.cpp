// 실제초기갱신/기술/contained원본133입력을같은AI소유메모리와기존실제하위에대조한다.
#include "RawSceneSupport.h"
#include "o/RawAiBrainInitialUpdate.h"
#include <set>
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 선택되지않은삭제/가상후속은성공으로채우지않는다.
[[noreturn]] void InitialMissing() { throw std::logic_error("AI초기갱신선택범위밖실제하위입니다"); }
// 검사입력만현재raw에쓴다. 제품은원본모듈들의같은실제Pool을사용한다.
std::span<std::uint8_t> InitialInput(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
// 내용물type배열은기대분류가아닌독립입력이다.
std::vector<std::uint32_t> InitialNodes(int pattern) {
    if (pattern==0) return {};if (pattern==1) return {1,2,3,4,5,133};if (pattern==2) return {2};if (pattern==3) return {2,3};if (pattern==4) return std::vector<std::uint32_t>(6,1);if (pattern==5) { std::vector<std::uint32_t> values(16,1);values.push_back(4);return values; }if (pattern==6) return {1,2,3,4};return {5,1};
}
// 실제getter/기술/Factory/contained/소유메모리를한장면으로조립한다.
struct InitialScene {
    std::vector<std::int64_t> input; //독립input7개다.
    SidPool pool{OriginalEdition::Patch1078,32768,true}; //실제clientSID와원본슬롯폭이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //공통Post/현재TYPE의실제188개크기다.
    std::array<std::string,188> energy; //명시TYPE+a0문자열입력이다.
    SquidPostPopState book; //실제Player생성계약의공통장부다.
    SquidPostPop post{pool,types,book}; //같은Pool의기존공통몸체다.
    SquidFactory factory{pool,types}; //실제client 할당/내장Form6생성자다.
    std::array<std::uint8_t,1620> records{}; //같은실제9Player이다.
    PlayerWorldState state; //현재PlayerKnown/owner전역이다.
    std::unique_ptr<RawPlayerWorld> player; //실제getter/Create의기존순서를사용한다.
    CarrierPreDestroyState finderState; //현재contained타입6/권한전역이다.
    std::unique_ptr<RawCarrierPreDestroy> finder; //실제004ac9f0조회다.
    ContainedCreateState createState; //실제Form6/다리전역이다.
    std::unique_ptr<RawContainedCreate> create; //실제Form필드쓰기/Factory/부착이다.
    std::unique_ptr<RawPlayerTechnology> technology; //실제40목록/Ensure사슬이다.
    std::unique_ptr<RawAiBrainMemory> memory; //생성자와같은소유storage형식이다.
    std::unique_ptr<RawAiBrainInitialUpdate> runtime; //같은소유메모리/Player/내용물을참조한다.
    std::uint32_t mission{},excluded{133},token{},builders{}; //원본전역/실제소유자핸들이다.
    std::array<std::uint32_t,4> workshops{86,87,88,85}; //실제004709d0의현재반환표다.
    std::array<std::uint8_t,600> profile{}; //독립600byte입력이다.
    std::string trace; //실제언어/타입/부착요청순서다.
    std::vector<Sid> born; //실제새Form할당순서다.
    std::vector<std::uint32_t> nodeKinds; //명시현재내용물입력이다.
    // 원본의입력만같은raw저장소에준비한다.
    explicit InitialScene(std::string_view text) {
        // 부호있는fixture입력을보존한다.
        for (const auto& value:Split(text,',')) input.push_back(std::stoll(value));CHECK(input.size()==7);mission=static_cast<std::uint32_t>(input[1]);state.playerKnown=1;state.battleOwner=1;nodeKinds=InitialNodes(static_cast<int>(input[0]));Put(records,180+0x9c,input[4] ? 50U : 0U);records[180+12]='O';records[180+13]='l';records[180+14]='d';
        // 부모/내용물만명시입력패턴으로준비한다.
        for (const Sid sid:{Sid{5},Sid{50},Sid{60},Sid{61},Sid{62},Sid{63},Sid{64},Sid{65},Sid{66},Sid{67},Sid{68},Sid{69},Sid{70},Sid{71},Sid{72},Sid{73},Sid{74},Sid{75},Sid{76}}) { auto raw=InitialInput(pool,sid);if (sid.value!=5) std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        const Sid parent{static_cast<std::uint16_t>(input[4] ? 50 : 5)};if (input[4]) Put(InitialInput(pool,parent),6,nodeKinds.empty() ? 0U : 60U,2);
        // free/dead/비contained를원본과같은입력으로섞는다.
        for (std::size_t index=0;index<nodeKinds.size();++index) { auto raw=InitialInput(pool,Sid{static_cast<std::uint16_t>(60+index)});Put(raw,0,0x12010000);raw[10]=input[0]==6 && index==1 ? 46 : 6;raw[11]=static_cast<std::uint8_t>(index%3==0 ? 0 : index%3==1 ? 8 : 9);Put(raw,4,index+1<nodeKinds.size() ? static_cast<std::uint32_t>(61+index) : 0U,2);Put(raw,18,nodeKinds[index],2);Put(raw,20,input[0]==6 && index==2 ? 0U : 4U,2); }
        // 명시TYPE표의이름/비용/원소/flags를준비한다.
        for (std::size_t index=0;index<types.size();++index) { types[index].name="T"+std::to_string(index);types[index].theme=0; }
        const auto metadata=[&](std::size_t type,int theme,std::uint32_t flags,float cost,std::string textValue) { types[type].theme=theme;types[type].flags2=flags;types[type].cost=cost;energy[type]=std::move(textValue); }; //원본과같은명시속성입력이다.
        metadata(1,0,0x10000,150.5F,"s");metadata(2,0,0x800,100.0F,"ss");metadata(3,1,0x40000,200.75F,"wz");metadata(4,2,0x800,800.25F,"rrrr");metadata(5,3,0,input[0]==7 ? std::bit_cast<float>(0x7fc00000U) : -1.0F,"");metadata(133,5,0x10000,99.0F,"s");types[85].cost=200.25F;types[86].cost=150.75F;types[87].cost=500.5F;types[88].cost=300.0F;
        PlayerWorldHooks playerHooks;playerHooks.assertion=[](PlayerWorldAssertion) { InitialMissing(); };playerHooks.find=[](float,float,std::uint32_t) { return Sid{}; };playerHooks.create=[this](std::uint32_t type,std::uint32_t flags) { const auto sid=factory.Create(type,flags);Put(pool.AllocatedBytes(sid),6,nodeKinds.empty() ? 0U : 60U,2);return sid; };playerHooks.setOwner=[this](Sid sid,std::uint32_t owner) { pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner); };playerHooks.pop=[this](Sid sid,float,float,std::uint32_t) { const auto owner=pool.Slot(sid)[34];Put(records,owner*180+0x9c,sid.value);Add("J,"+std::to_string(owner)); };playerHooks.reserveRegular=[]()->bool { InitialMissing(); };playerHooks.addRegular=[](Sid,std::uint32_t,float) { InitialMissing(); };player=std::make_unique<RawPlayerWorld>(pool,post,records,state,playerHooks);
        CarrierPreDestroyHooks carrierHooks;carrierHooks.validateDamageable=[](Sid,std::uint32_t) { InitialMissing(); };carrierHooks.damageablePre=[](Sid,std::uint32_t) { InitialMissing(); };carrierHooks.dependentFound=[] { InitialMissing(); };finder=std::make_unique<RawCarrierPreDestroy>(pool,finderState,carrierHooks);
        ContainedCreateHooks createHooks;createHooks.create=[this](std::uint32_t type,std::uint32_t flags) { Add("C,"+std::to_string(type)+','+std::to_string(flags));CHECK(type==6);const auto sid=factory.CreateContained(flags);born.push_back(sid);return sid; };createHooks.notifySurface=[](Sid) { InitialMissing(); };createHooks.attach=[this](Sid sid,Sid target,std::uint32_t flags) { Add("B,"+std::to_string(sid.value)+','+std::to_string(target.value)+','+std::to_string(flags));Put(pool.AllocatedBytes(sid),4,Get(pool.Slot(target),6,2),2);Put(InitialInput(pool,target),6,sid.value,2); };create=std::make_unique<RawContainedCreate>(pool,createState,createHooks);
        PlayerTechnologyHooks techHooks;techHooks.findType=[this](std::string_view name) { Add("F,"+Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(name.data()),name.size())));for (std::size_t index=0;index<48;++index) if (name==types[index].name) return static_cast<std::int32_t>(index);return -1; };techHooks.assertion=[](PlayerTechnologyAssertion) { InitialMissing(); };techHooks.destroy=[](Sid,std::uint32_t) { InitialMissing(); };technology=std::make_unique<RawPlayerTechnology>(*player,*finder,*create,types,techHooks);
        std::size_t allocation=0;memory=std::make_unique<RawAiBrainMemory>(0x13000000,[seed=input[6],allocation](std::span<std::uint8_t> bytes) mutable { for (std::size_t index=0;index<bytes.size();++index) bytes[index]=allocation ? 0 : static_cast<std::uint8_t>(index*29+seed);++allocation; });token=memory->Allocate(RawAiBrainConstructor::kBytes);builders=memory->Allocate(40);auto raw=memory->Bytes(token);
        for (const auto [offset,value]:std::array<std::pair<std::size_t,std::uint32_t>,10>{{{0x260,1},{0x284,builders},{0x288,10},{0x28c,static_cast<std::uint32_t>(input[5])},{0x3b0,static_cast<std::uint32_t>(input[5])},{0x394,input[5] ? 85U : 0U},{0x390,3},{0xad8,input[5] ? 7U : 2U},{0x2d8,999},{0x290,0}}}) Put(raw,offset,value);
        if (input[5]) { Put(memory->Bytes(builders),0,99);Put(memory->Bytes(builders),4,98);Put(raw,0x398,1);Put(raw,0x39c,3); }
        // 같은600byte미초기화입력에이름/기술/새StartMoney만쓴다.
        for (std::size_t index=0;index<profile.size();++index) profile[index]=static_cast<std::uint8_t>(index*37+input[6]);if (input[2]) { profile[0]='A';profile[1]='I';profile[2]=0; }else profile[0]=0;const std::string tech=input[3] ? "T1;T2" : "";std::copy(tech.begin(),tech.end(),profile.begin()+30);profile[30+tech.size()]=0;Put(profile,0x22c,777);runtime=std::make_unique<RawAiBrainInitialUpdate>(*memory,*player,*technology,finderState,types,AiBrainInitialUpdateGlobals{mission,excluded,workshops},Hooks());
    }
    // 실제호출만순서대로기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원본외부언어/TYPE문자열요청만기록한다.
    AiBrainInitialUpdateHooks Hooks() { AiBrainInitialUpdateHooks hooks;hooks.defaultName=[this] { Add("D");return std::string("Default AI"); };hooks.requiredEnergy=[this](std::uint32_t type) { Add("E,"+std::to_string(type));return energy.at(type); };return hooks; }
    // 실제새Form의전체raw50byte를원본할당순서로읽는다.
    std::string Born() const { std::string result;for (const auto sid:born) { if (!result.empty()) result+=':';result+=Hex(pool.Slot(sid)); }return result.empty() ? "-" : result; }
};
}
// 원본133고유input의전체body/builder40byte/Player1620byte/새Form/모든조회효과를비교한다.
TEST_CASE(AiInitialUpdate_Native133) {
    std::size_t count=0;std::set<std::string> unique;
    // 원본기대출력은실행이후에만읽는다.
    for (const auto& row:LoadFixture(NETSTORM_AI_INITIAL_UPDATE_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==6);CHECK(unique.insert(row[0]).second);InitialScene scene(row[0]);scene.runtime->Update(scene.token,scene.profile);CHECK(Hex(scene.memory->Bytes(scene.token))==row[1]);CHECK(Hex(scene.memory->Bytes(scene.builders))==row[2]);CHECK(Hex(scene.records)==row[3]);CHECK(scene.Born()==row[4]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[5]);CHECK(Get(scene.memory->Bytes(scene.token),0x2d8)==999);++count;
    }
    CHECK(count==133);
}
// 선택되지않은경계는요구하지않고실제에너지누락은일부효과뒤에도오류로남긴다. SP지갑을재지정하지않는다.
TEST_CASE(AiInitialUpdate_MissingSelectedMetadataAndWalletPreserved) {
    InitialScene empty("0,0,0,0,1,0,19");RawAiBrainInitialUpdate inactive(*empty.memory,*empty.player,*empty.technology,empty.finderState,empty.types,{empty.mission,empty.excluded,empty.workshops},{});inactive.Update(empty.token,empty.profile);CHECK(Get(empty.memory->Bytes(empty.token),0x2d8)==999 && Get(empty.memory->Bytes(empty.token),0x22c)==777);
    InitialScene missing("1,1,1,0,1,0,19");auto hooks=missing.Hooks();hooks.requiredEnergy={};RawAiBrainInitialUpdate incomplete(*missing.memory,*missing.player,*missing.technology,missing.finderState,missing.types,{missing.mission,missing.excluded,missing.workshops},hooks);bool rejected=false;try { incomplete.Update(missing.token,missing.profile); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && Get(missing.memory->Bytes(missing.token),0x28c)==1 && Get(missing.memory->Bytes(missing.token),0x2d8)==999);
}

// theme5의실제stack칸공유와후속battery중립화·원소목록·전체본문을별도원본16입력으로대조한다.
TEST_CASE(AiInitialUpdate_NativeTheme5AndSharedCounter) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/ai-initial-update-theme5-x86.tsv").string();std::size_t count{};
    // 기존133입력fixture는보존하고추가원문만읽는다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;InitialScene scene(row[1]);scene.types[1].theme=static_cast<std::int32_t>(Number(row[0]));
        scene.runtime->Update(scene.token,scene.profile);CHECK(Hex(scene.memory->Bytes(scene.token))==row[2]);CHECK(Hex(scene.memory->Bytes(scene.builders))==row[3]);
        CHECK(Hex(scene.records)==row[4] && scene.Born()==row[5] && (scene.trace.empty() ? "-" : scene.trace)==row[6]);++count;
    }
    CHECK(count==16);
}
