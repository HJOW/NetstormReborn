// 두 원본의 공급원 등록과 실제 Client/World의 같은 목록/진단/카운터를 화면 없이 대조한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "o/RawManaRegistration.h"
#include "client/RawSurfaceWorld.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include <bit>
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 입력만 조립하며 최종 목록/카운터는 실제 CPU fixture에서 읽는다.
// 현재 포인터/용량/개수/두 물리 배열을 서로 다른 저장소로 유지한다.
struct ManaRegistrationScene {
    std::array<std::uint32_t,16> a{},b{}; // 원문과 같은 현재/교체 목록의 stale 꼬리다.
    std::uint32_t pointer{},used{},capacity{},dirty{},checking{},value{},action{}; // CPU에 공급한 전체 DWORD 입력이다.
    SquidPostPopList list;ManaRegistrationDiagnostics diagnostics;std::string events; // 현재 목록 뷰와 선택 진단의 순서다.
    explicit ManaRegistrationScene(const std::vector<std::string>& row) { // 판본 차이는 실행할 때 제품 부모가 결정한다.
        value=Number(row[2]);used=Number(row[3]);capacity=Number(row[4]);dirty=Number(row[6]);checking=Number(row[7]);action=Number(row[8]);const auto pattern=Number(row[5]);
        for (std::uint32_t index=0;index<a.size();++index) { a[index]=0x11220000U+index;b[index]=0x33440000U+index; } // 예약 꼬리도 서로 구별한다.
        if (used && (pattern&1U)) a[0]=value;if (used && (pattern&2U)) a[used-1]=value;
        list.BindCurrent({[this] { return std::span(pointer ? b : a); },[this] { return capacity; },[this] { return used; },[this](auto count) { used=count; }});diagnostics.checking.Bind(checking);
        diagnostics.assertion=[this](auto expression,auto file,auto line) { events+="D,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';if (action&1U) checking=0;if (action&2U) { pointer=1;used=0;capacity=2; }if (action&4U) dirty=0xffffffff;if (action&8U) { for (std::uint32_t index=0;index<a.size();++index) if (a[index]==value) a[index]=0xdead0000U+index; } }; // 진단에서 실제 입력만 바꾸며 부모 분기 결과는 만들지 않는다.
    }
    static std::string Words(const std::array<std::uint32_t,16>& words) { std::array<std::uint8_t,64> raw{};for (std::size_t index=0;index<words.size();++index) Put(raw,index*4,words[index]);return Hex(raw); } // 모든 DWORD를 명시 little endian으로 직렬화한다.
    std::vector<std::string> Run(OriginalEdition edition) { RegisterManaProvider(edition,list,dirty,value,diagnostics);return {std::to_string(pointer),std::to_string(used),std::to_string(capacity),std::to_string(dirty),std::to_string(checking),Words(a),Words(b),events.empty() ? "-" : events}; } // 출력 모델 없이 현재 메모리만 읽는다.
};
const auto& ManaRows() { static const auto rows=LoadFixture(NETSTORM_MANA_REGISTRATION_FIXTURE).rows;return rows; } // 최초 CPU1084행을 읽기 전용으로 사용한다.
void ReplayManaRegistration(std::string_view edition) { // 판본별 signed 용량/진단 유무를 각각 대조한다.
    std::size_t count{};
    for (const auto& row:ManaRows()) { // 첫 불일치 필드만 원문/제품 값으로 남긴다.
        if (row[0]!=edition) continue;CHECK(row.size()==17);ManaRegistrationScene scene(row);const auto actual=scene.Run(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);const auto expected=std::vector<std::string>(row.begin()+9,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("마나 등록 %s 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),row[1].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count; // 기대 fixture를 교체하지 않는다.
    }CHECK(count==542);
}
}
// 중복/양끝/전체 DWORD 토큰·signed 용량·카운터 감김·진단 뒤 현재 owner를 실제 패치 CPU와 비교한다.
TEST_CASE(ManaRegistration_PatchNativeDuplicateResumeAndCurrentCounter) { ReplayManaRegistration("originals"); }
// CD는 checking 장치를 호출하지 않고 같은 목록/카운터와 별도 반복 조건을 따른다.
TEST_CASE(ManaRegistration_CdNativeSignedCapacityAndNoDiagnostic) { ReplayManaRegistration("originalCD"); }
// 기계어의 최초 양수 개수 분기 뒤 첫 원소 검사/중복 종료는 다음 count 읽기보다 앞선다.
TEST_CASE(ManaRegistration_FirstSearchPrecedesNextCurrentCountRead) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 패치는 저장한 count,CD는 do 본문 뒤 현재 count 조건을 사용한다.
        std::array<std::uint32_t,1> words{50};std::uint32_t used=1,reads{},dirty{};SquidPostPopList list;
        list.BindCurrent({[&] { return std::span(words); },[] { return 1U; },[&] { const auto value=used;if (++reads==1) used=0;return value; },[&](auto value) { used=value; }}); // 첫 읽기가 양수 값을 반환한 직후 현재 개수가0이 된다는 명시 외부 입력이다.
        RegisterManaProvider(edition,list,dirty,50,{});CHECK(used==0 && reads==1 && dirty==1 && words[0]==50); // 첫 중복을 찾았으므로 다음 count/용량/배열 쓰기를 선택하지 않는다.
    }
}
// 진단 누락/중단과 선택되지 않은 배열 장치를 빈 성공/사전 효과 취소로 바꾸지 않는다.
TEST_CASE(ManaRegistration_SelectedDevicesAndPartialEffects) {
    auto row=ManaRows().front();row[2]="50";row[3]="1";row[4]="4";row[5]="1";row[6]="123";row[7]="2147483648";
    ManaRegistrationScene missing(row);missing.diagnostics.assertion={};CHECK(Throws([&] { missing.Run(OriginalEdition::Patch1078); }));CHECK(missing.dirty==123 && missing.used==1 && missing.a[0]==50);
    ManaRegistrationScene stopped(row);stopped.diagnostics.assertion=[&](auto,auto,auto) { stopped.pointer=1;stopped.used=0;stopped.dirty=42;throw std::runtime_error("선택 진단 중단"); };CHECK(Throws([&] { stopped.Run(OriginalEdition::Patch1078); }));CHECK(stopped.pointer==1 && stopped.used==0 && stopped.dirty==42 && stopped.a[0]==50);
    std::uint32_t dirty=0xffffffff;SquidPostPopList noArray;noArray.BindCurrent({[]()->std::span<std::uint32_t> { throw std::logic_error("미선택 배열 장치"); },[] { return 0x80000000U; },[] { return 0U; },[](auto) { throw std::logic_error("미선택 개수 쓰기"); }});RegisterManaProvider(OriginalEdition::Patch1078,noArray,dirty,0xffffffff,{});CHECK(!dirty); // signed 음수 용량/빈 검색은 배열을 읽거나 쓰지 않는다.
}
// 실제 World의 공통 postPop이 같은 Client checking/장기 공급원/현재 카운터를 두 대여에서 사용한다.
TEST_CASE(ManaRegistration_ActualClientWorldCurrentOwnerAndPostPopContinuation) {
    const auto edition=OriginalEdition::Patch1078;ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);types[74].flags1=0x10000000;types[74].cost=3.75f;types[75].flags2=0x4200;
    auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes);storage->BindChecking(client.GameplayGlobals()->AiDebug().Checking());std::optional<std::string> detail;std::uint32_t diagnostics{};RawWorldCore* current{};Sid victim{};const auto* address=storage->sidLists.providers.Storage().data();
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="!alreadyThere" && file=="\\Ns\\O\\Mana.cpp" && line==0x44 && current);++diagnostics;client.GameplayGlobals()->AiDebug().Checking()=0;storage->sidLists.providers.SetCount(0);current->bookkeeping.productionDirty=0xffffffff;auto raw=current->pool.AllocatedBytes(victim);raw[10]=75;raw[34]=2;current->bookkeeping.localOwner=2; }); // 원문 진단 뒤에도 저장한 중복/TYPE flags2를 쓰고 현재 owner/type 통계를 읽는다.
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    for (std::uint32_t generation=0;generation<2;++generation) { // 같은 물리 배열을 다음 World에 전달해도 진단 owner를 복제하지 않는다.
        RawSurfaceWorld world(edition,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();current=&core;const auto local75=core.bookkeeping.LocalCounts()[75],global74=core.bookkeeping.GlobalCounts()[74],global75=core.bookkeeping.GlobalCounts()[75];victim=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(victim);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;raw[34]=1;core.bookkeeping.depth=1;core.postPop.PostPopBase(victim,1);CHECK(core.bookkeeping.Providers().Count()==1 && core.bookkeeping.productionDirty==1 && core.bookkeeping.TotalCost()==3 && !core.bookkeeping.depth);
        client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);core.bookkeeping.depth=1;core.postPop.PostPopBase(victim,1);CHECK(diagnostics==generation+1 && !client.GameplayGlobals()->AiDebug().Checking());CHECK(core.bookkeeping.Providers().Count()==0 && core.bookkeeping.Providers().Storage().data()==address && !core.bookkeeping.productionDirty && core.bookkeeping.TotalCost()==6 && !core.bookkeeping.depth);CHECK(core.bookkeeping.LocalCounts()[75]==local75+1 && core.bookkeeping.GlobalCounts()[74]==global74+1 && core.bookkeeping.GlobalCounts()[75]==global75+1 && core.bookkeeping.Factories().Count()==0);current=nullptr; // 저장된 중복은 삭제 뒤에도 재추가하지 않고 외곽의 현재 TYPE 통계까지 계속한다.
    }
}
