// 실제 두 PE의 Carrier 전체 메뉴를 같은 Pool/contained 체인과 독립 입력으로 비교한다.
#include "RawSceneSupport.h"
#include "client/RawCarrierMenu.h"
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원문 fixture22개 입력 열의 원본 의미다.
enum Input { Owner,Local,RawType,Flags1,Flags2,Extra,Ground,Path,Active,Count,ContentFlags,ContentType,X,Y,Limit,Cost,CdCost,Sp,Editor,Blocked,Building,Mutation };
// 바이트 문자열을 원문 효과와 같은hex로 만든다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 기대 메뉴를 계산하지 않고 실제 외부효과와 callback 변경만 제공한다.
struct CarrierScene {
    SidPool pool; // 두 판본의 현재 실제 슬롯이다.
    std::vector<RiftTypeRecord> types; // 현재 TYPE 표를 메뉴와 공유한다.
    ContainedFinderState contained; // 실제 기존 finder가 읽는 전역이다.
    ObjectMenuState common; // 다른 메뉴와 같은 로컬/편집/도움말 전역이다.
    std::int32_t limit{256}; // 원본 지도 크기 전역이다.
    float cdSp{}; // CD의 직접 SP 전역이다.
    CarrierMenuState state{common.localOwner,common.editor,limit,cdSp,common.helpCommand}; // 같은 전역의 참조만 보관한다.
    CarrierMenuHooks hooks; // 실제 외부 조회/효과 경계다.
    std::vector<std::uint32_t> input; // fixture22DWORD 입력이다.
    std::string trace; // 실제로 도달한 효과 순서다.
    std::uint32_t points{}; // 현재 위치 조회가 처리하는 내용물 순번이다.
    // 두 판본에서 같은 실제 부모/내용물 클라이언트 슬롯과 체인을 만든다.
    CarrierScene(OriginalEdition edition,const std::vector<std::string>& row):pool(edition,24000,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 음수 인자도DWORD원문 비트로 보존한다.
        for (std::size_t index=1;index<=22;++index) input.push_back(static_cast<std::uint32_t>(std::stoll(row[index])));
        // 실제 클라이언트 번호5..70을 확보한다.
        for (std::uint32_t number=5;number<=70;++number) CHECK(pool.Allocate(2).value==number);
        auto raw=pool.AllocatedBytes(Sid{50});raw[10]=static_cast<std::uint8_t>(input[RawType]);raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=static_cast<std::uint8_t>(input[Owner]);raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(input[Extra]);Put(raw,6,input[Count] ? 60 : 0,2);
        // 타입 변경 콜백이 가리키는 다른 타입도 같은 독립 플래그/비용 입력을 갖는다.
        for (auto& type:types) { type.flags1=input[Flags1];type.flags2=input[Flags2];type.cost=std::bit_cast<float>(input[CdCost]); }
        // 기존 실제 finder가 읽는 혼합 내용물 체인이다.
        for (std::uint32_t index=0;index<input[Count];++index) {
            auto content=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(60+index)});content[10]=6;Put(content,18,input[ContentType]+index,2);Put(content,20,input[ContentFlags]^(index&1),2);Put(content,4,index+1<input[Count] ? 61+index : 0,2);
        }
        common.localOwner=input[Local];common.editor=input[Editor]!=0;limit=std::bit_cast<std::int32_t>(input[Limit]);cdSp=std::bit_cast<float>(input[Sp]);
        hooks.groundImmobile=[this](Sid sid) { Add("G,"+std::to_string(sid.value));return input[Ground]!=0; };
        hooks.description=[this,edition](std::uint32_t type) { Add("D,"+std::to_string(type));if (input[Mutation]&2) pool.AllocatedBytes(Sid{50})[edition==OriginalEdition::Patch1078 ? 34 : 32]=2;return "Type"+std::to_string(type); };
        hooks.movementType=[this](std::uint32_t type) { Add("M,"+std::to_string(type));if (input[Mutation]&4) pool.AllocatedBytes(Sid{50})[10]=133;return 54U; };
        hooks.findPath=[this](Sid parent,std::uint32_t type) { Add("L,"+std::to_string(parent.value)+','+std::to_string(type));return Sid{static_cast<std::uint16_t>(input[Path] ? 70 : 0)}; };
        hooks.pathActive=[this](Sid sid) { Add("A,"+std::to_string(sid.value));return input[Active]!=0; };
        hooks.dropPoint=[this](Sid sid,std::uint32_t type) {
            Add("P,"+std::to_string(sid.value)+','+std::to_string(type));auto content=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(60+points++)});
            if (input[Mutation]&1) Put(content,20,Get(content,20,2)^1,2);if (input[Mutation]&16) common.localOwner=2;
            return std::array<float,2>{std::bit_cast<float>(input[X]),std::bit_cast<float>(input[Y])};
        };
        hooks.patchCost=[this](std::uint32_t) { return std::bit_cast<std::int32_t>(input[Cost]); };
        hooks.money=[this](std::uint32_t owner) { Add("S,"+std::to_string(owner));if (input[Mutation]&8) common.editor=true;return std::bit_cast<float>(input[Sp]); };
        hooks.hasEvent=[this](Sid sid,std::uint32_t event) { Add("E,"+std::to_string(sid.value)+','+std::to_string(event));if ((input[Mutation]&32) && event==0x493) pool.AllocatedBytes(sid)[10]=133;return (event==0x493 ? input[Blocked] : input[Building])!=0; };
        hooks.format=[this](std::string_view key,std::span<const PriestMenuArgument> arguments) { Add("T,"+TextHex(key));std::string result;std::size_t argument=0;
            // 번역된 원본 고정 서식의%s/%d를 인자 순서대로 처리한다.
            for (std::size_t index=0;index<key.size();++index) { if (key[index]!='%') { result+=key[index];continue; }++index;CHECK(index<key.size() && argument<arguments.size());if (key[index]=='s') result+=std::get<std::string>(arguments[argument++]);else { CHECK(key[index]=='d');result+=std::to_string(std::get<std::int32_t>(arguments[argument++])); } }
            CHECK(argument==arguments.size());return result;
        };
        hooks.header=[this](std::string_view text) { Add("H,"+TextHex(text)); };
        hooks.item=[this](std::string_view text,std::uint32_t command,std::uint32_t parameter,bool enabled) {
            Add("I,"+TextHex(text)+','+std::to_string(command)+','+std::to_string(parameter)+','+std::to_string(enabled));
            if ((input[Mutation]&64) && points<input[Count]) { auto next=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(60+points)});Put(next,20,Get(next,20,2)^1,2); }
        };
        hooks.damageableMenu=[this](Sid sid) { Add("B,"+std::to_string(sid.value)); };
    }
    // 예상 메뉴를 만들지 않고 실제 효과만 순서대로 연결한다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 원본 현재 참조를 빌린 처리기를 만들며 게임/GUI는 실행하지 않는다.
    RawCarrierMenu Menu() { return RawCarrierMenu(pool,types,contained,state,hooks); }
};
}
// 전체 원문628행의 문자열/명령/숫자/활성화·외부 조회 순서를 비교한다.
TEST_CASE(CarrierMenu_Native628) {
    std::size_t count=0;
    // 결과는 실제 두 PE의 전체 메뉴 몸체에서 수집한 독립 기대값이다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_MENU_FIXTURE).rows) {
        if (row[0]=="edition") continue;CHECK(row.size()==24);CarrierScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);scene.Menu().Open(Sid{50});
        if (scene.trace!=row.back()) std::fprintf(stderr,"Carrier 메뉴 원문 차이 %s owner%u flags%u count%u x%u mut%u\n실제 %s\n원문 %s\n",row[0].c_str(),scene.input[Owner],scene.input[ContentFlags],scene.input[Count],scene.input[X],scene.input[Mutation],scene.trace.c_str(),row.back().c_str());
        CHECK(scene.trace==row.back());++count;
    }
    CHECK(count==628);
}
// genus 생략은 기본 메뉴로 이어지고 도달한 누락 하위는 명시 실패해야 한다.
TEST_CASE(CarrierMenu_RequiresReachedDependenciesAndPriestBinding) {
    const auto rows=LoadFixture(NETSTORM_CARRIER_MENU_FIXTURE).rows;CarrierScene scene(OriginalEdition::Patch1078,rows.at(1));scene.hooks.groundImmobile={};CHECK(Throws([&] { scene.Menu().Open(Sid{50}); }));
    scene.types[158].flags1=0x1000000;scene.trace.clear();auto carrier=scene.Menu();PriestMenuHooks priest;BindPriestMenuCarrier(priest,carrier);priest.baseMenu(Sid{50});CHECK(scene.trace=="B,50");
    scene.hooks.damageableMenu={};CHECK(Throws([&] { scene.Menu().Open(Sid{50}); }));
}
