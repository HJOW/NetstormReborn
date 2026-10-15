// 기술wrapper/실제contained조회/생성과40개목록을독립원본기계어에대조한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "o/RawPlayerTechnology.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kTechnologyVtable=0x12010000; // 원본격리입력과동일한가상삭제/부착경계표식이다.
// 검사입력만raw슬롯에쓴다. 제품은실제Factory/참여/contained모듈의저장소를사용한다.
std::span<std::uint8_t> TechnologyInput(SidPool& pool,Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
// 공통Post/Factory는실제PATCH188개표를받고선택기술검사는그앞48개의이름/group입력을사용한다.
std::vector<RiftTypeRecord> TechnologyTypes() {
    std::vector<RiftTypeRecord> result(188);
    // 기대all목록을계산하지않고실제TYPE속성만준비한다.
    for (std::size_t index=0;index<result.size();++index) { result[index].name="T"+std::to_string(index);result[index].group=index%7==0 ? 10 : static_cast<int>(index%10); }return result;
}
// TSV에서보존한목록문자열을실제바이트로복원한다.
std::string TechnologyText(std::string_view hex) {
    std::string result;
    // 두hex글자가원래한문자열바이트다.
    for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16));return result;
}
// 선택되지않은Player생성/삭제부모경계에빈성공을공급하지않는다.
[[noreturn]] void TechnologyMissing() { throw std::logic_error("기술검사에서선택되지않은실제하위경계입니다"); }
// 같은Pool/기술부모/contained/body를참조하며원본기대상태는읽지않는다.
struct TechnologyScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 실제clientSID5부터새contained를할당한다.
    std::vector<RiftTypeRecord> types=TechnologyTypes(); // 실제Post/Factory188개표이며기술조회선택입력은앞48개다.
    SquidPostPopState book; // 실제PlayerWorld생성계약의공통장부이며선택된기술함수는변경하지않는다.
    SquidPostPop post{pool,types,book}; // 같은Pool을확인하는실제공통몸체다.
    std::array<std::uint8_t,1620> records{}; // 실제9Player기록이다.
    PlayerWorldState state; // 같은PlayerKnown진단입력이다.
    CarrierPreDestroyState carrierState; // 기존contained타입6/커서권한이다.
    ContainedCreateState createState; // 기존Form6생성자의현재전역이다.
    SquidFactory factory{pool,types}; // 실제내장contained생성자/할당몸체다.
    std::unique_ptr<RawPlayerWorld> player; // 필수하위는선택시거부하며Technology조회는실제몸체다.
    std::unique_ptr<RawCarrierPreDestroy> finder; // 실제004ac9f0/containedfinder를재사용한다.
    std::unique_ptr<RawContainedCreate> create; // 실제0/4/10/0과Factory/Form필드쓰기를수행한다.
    std::unique_ptr<RawPlayerTechnology> technology; // 앞선저장소와세계가모두같아야생성된다.
    std::string trace; // 실제타입조회/진단/생성/부착/삭제요청의순서다.
    std::uint32_t owner{},pattern{},mutate{}; // fixture의명시현재입력이다.
    std::vector<Sid> born; // 실제Factory가생성한번호순이며기대목록이아니다.
    // fixture입력만읽고현재raw체인을준비한다.
    explicit TechnologyScene(std::string_view input) {
        const auto fields=Split(input,'|');const auto numbers=Split(fields.at(0),',');owner=Number(numbers.at(0));state.playerKnown=Number(numbers.at(2));pattern=Number(numbers.at(3));mutate=Number(numbers.at(4));Put(records,owner*180+0x9c,50);
        // 부모와종속네개의주변바이트를원본과동일하게초기화한다.
        for (const Sid sid:{Sid{50},Sid{51},Sid{60},Sid{61},Sid{62},Sid{63}}) { auto raw=TechnologyInput(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        Put(TechnologyInput(pool,Sid{50}),0,kTechnologyVtable);TechnologyInput(pool,Sid{50})[10]=152;
        if (pattern) {
            Put(TechnologyInput(pool,Sid{50}),6,60,2);
            constexpr std::array<std::uint32_t,4> kinds{1,2,2,7},flags{4,4,1,4}; // 서로다른타입/flags조건의입력이다.
            // 비contained타입/flags1/flags4를같은raw체인에섞는다.
            for (std::uint16_t index=0;index<4;++index) { auto raw=TechnologyInput(pool,Sid{static_cast<std::uint16_t>(60+index)});Put(raw,0,kTechnologyVtable);raw[10]=index==1 ? 46 : 6;Put(raw,4,index<3 ? 61+index : 0,2);Put(raw,18,kinds[index],2);Put(raw,20,flags[index],2); }
        }
        PlayerWorldHooks playerHooks;playerHooks.assertion=[this](PlayerWorldAssertion value) { CHECK(value==PlayerWorldAssertion::PlayerIdKnown);Add("A,669"); };playerHooks.find=[](float,float,std::uint32_t)->Sid { TechnologyMissing(); };playerHooks.create=[](std::uint32_t,std::uint32_t)->Sid { TechnologyMissing(); };playerHooks.setOwner=[](Sid,std::uint32_t) { TechnologyMissing(); };playerHooks.pop=[](Sid,float,float,std::uint32_t) { TechnologyMissing(); };playerHooks.reserveRegular=[]()->bool { TechnologyMissing(); };playerHooks.addRegular=[](Sid,std::uint32_t,float) { TechnologyMissing(); };player=std::make_unique<RawPlayerWorld>(pool,post,records,state,playerHooks);
        CarrierPreDestroyHooks carrierHooks;carrierHooks.validateDamageable=[](Sid,std::uint32_t) { TechnologyMissing(); };carrierHooks.damageablePre=[](Sid,std::uint32_t) { TechnologyMissing(); };carrierHooks.dependentFound=[] { TechnologyMissing(); };finder=std::make_unique<RawCarrierPreDestroy>(pool,carrierState,carrierHooks);
        ContainedCreateHooks createHooks;createHooks.create=[this](std::uint32_t type,std::uint32_t flags) { Add("C,"+std::to_string(type)+','+std::to_string(flags));CHECK(type==6);const auto sid=factory.CreateContained(flags);born.push_back(sid);return sid; };createHooks.notifySurface=[](Sid) { TechnologyMissing(); };createHooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) { Add("B,"+std::to_string(sid.value)+','+std::to_string(parent.value)+','+std::to_string(flags));auto raw=pool.AllocatedBytes(sid);const auto head=Get(pool.Slot(parent),6,2);Put(raw,4,head,2);Put(TechnologyInput(pool,parent),6,sid.value,2); };create=std::make_unique<RawContainedCreate>(pool,createState,createHooks);
        PlayerTechnologyHooks hooks;hooks.findType=[this](std::string_view name) { std::string hex;
            // 실제이름조회요청바이트를기록한다.
            for (const auto byte:name) { constexpr char digits[]="0123456789abcdef";hex+=digits[static_cast<unsigned char>(byte)>>4];hex+=digits[static_cast<unsigned char>(byte)&15]; }Add("F,"+hex);
            // 명시fixture이름입력의첫일치조회이며제품의기대목록을계산하지않는다.
            for (std::size_t index=0;index<types.size();++index) { auto first=std::string(name),second=types[index].name;std::transform(first.begin(),first.end(),first.begin(),[](unsigned char byte) { return static_cast<char>(byte>='A' && byte<='Z' ? byte+32 : byte); });std::transform(second.begin(),second.end(),second.begin(),[](unsigned char byte) { return static_cast<char>(byte>='A' && byte<='Z' ? byte+32 : byte); });if (first==second) return static_cast<std::int32_t>(index); }return -1;
        };hooks.assertion=[this](PlayerTechnologyAssertion value) { Add(std::string("A,")+(value==PlayerTechnologyAssertion::FindTypeRange ? "867" : "886")); };hooks.destroy=[this](Sid sid,std::uint32_t flags) { Add("D,"+std::to_string(sid.value)+','+std::to_string(flags));Sid previous{50};std::size_t field=6;auto current=Get(pool.Slot(previous),field,2);
            // 외부삭제응답은같은현재raw체인에서해당SID만제거한다.
            while (current && current!=sid.value) { previous=Sid{static_cast<std::uint16_t>(current)};field=4;current=Get(pool.Slot(previous),field,2); }CHECK(current==sid.value);Put(TechnologyInput(pool,previous),field,Get(pool.Slot(sid),4,2),2);if (mutate) Put(records,owner*180+0x9c,51);
        };technology=std::make_unique<RawPlayerTechnology>(*player,*finder,*create,std::span<const RiftTypeRecord>(types).first(48),hooks);
    }
    // 외부경계의실제요청만누적한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 같은관찰슬롯전체를메모리순서대로직렬화한다.
    std::string Raw() const { std::string result=Hex(pool.Slot(Sid{50}));for (std::uint16_t sid=60;sid<64;++sid) result+=':'+Hex(pool.Slot(Sid{sid}));return result; }
    // 실제생성된모든슬롯의전체50바이트다.
    std::string Born() const { std::string result;for (const auto sid:born) { if (!result.empty()) result+=':';result+=Hex(pool.Slot(sid)); }return result.empty() ? "-" : result; }
};
}
// 원본158고유입력에서정상반환/두반복무진행/전체raw와생성슬롯/모든효과를비교한다.
TEST_CASE(PlayerTechnology_Native158) {
    std::size_t count=0;std::set<std::pair<std::string,std::string>> unique;
    // fixture출력은실행이후에만읽어기대모델로사용하지않는다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_TECHNOLOGY_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==8);CHECK(unique.emplace(row[0],row[1]).second);TechnologyScene scene(row[1]);const auto fields=Split(row[1],'|');const auto numbers=Split(fields[0],',');const auto type=static_cast<std::int32_t>(std::stoll(numbers[1]));const auto text=TechnologyText(fields.at(1));std::string result="-",values;
        if (row[0]=="find") result=std::to_string(scene.technology->Find(static_cast<std::int32_t>(scene.owner),type).value);else if (row[0]=="clear") scene.technology->Clear(static_cast<std::int32_t>(scene.owner));else if (row[0]=="ensure") scene.technology->Ensure(static_cast<std::int32_t>(scene.owner),type);else if (row[0]=="add") scene.technology->AddList(static_cast<std::int32_t>(scene.owner),text);
        else if (row[0]=="parse" || row[0]=="stalled") { PlayerTechnologyList list;
            // 원본정상종료또는동일미등록항목의실제두반복까지만진행한다.
            for (;;) { const auto step=scene.technology->ParseStep(text,list);if (step==PlayerTechnologyParseStep::Complete) break;if (step==PlayerTechnologyParseStep::Stalled) { CHECK(row[0]=="stalled");CHECK(scene.technology->ParseStep(text,list)==PlayerTechnologyParseStep::Stalled);result=std::to_string(list.index);break; } }
            for (std::size_t index=0;index<list.count;++index) { if (!values.empty()) values+=',';values+=std::to_string(list.types[index]); }
        } else CHECK(false);
        CHECK(result==row[2]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[3]);CHECK((values.empty() ? "-" : values)==row[4]);CHECK(scene.Raw()==row[5]);CHECK(scene.Born()==row[6]);CHECK(Get(scene.records,scene.owner*180+0x9c)==Number(row[7]));++count;
    }
    CHECK(count==158);
}
// 실제보호TYPE이름/group을기존RiftTypeTable::Find에직접연결하고무진행오류는기술생성전에보고한다.
TEST_CASE(PlayerTechnology_ActualTypeBindingsAndStalledBoundary) {
    TechnologyScene scene("1,0,1,0,0|");const auto& actual=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game.TypeTable();PlayerTechnologyHooks hooks;hooks=BindPlayerTechnologyTypes(actual,std::move(hooks));
    hooks.destroy=[](Sid,std::uint32_t) { TechnologyMissing(); };hooks.assertion=[](PlayerTechnologyAssertion) { TechnologyMissing(); };RawPlayerTechnology technology(*scene.player,*scene.finder,*scene.create,actual.Types(),hooks);
    PlayerTechnologyList list;CHECK(technology.ParseStep("sunWalker;windBattery;sunCannon",list)==PlayerTechnologyParseStep::Progress);CHECK(list.count==1 && list.types[0]==actual.Find("sunWalker"));CHECK(technology.ParseStep("sunWalker;windBattery;sunCannon",list)==PlayerTechnologyParseStep::Progress);CHECK(technology.ParseStep("sunWalker;windBattery;sunCannon",list)==PlayerTechnologyParseStep::Progress);CHECK(technology.ParseStep("sunWalker;windBattery;sunCannon",list)==PlayerTechnologyParseStep::Complete);CHECK(list.count==3);
    bool rejected=false;try { technology.AddList(1,"sunWalker;missing;sunCannon"); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.born.empty());
}
