// AI 에너지 부족 좌표의 실제 본체/목록·순환·효과 뒤 재조회를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDrySpots.h"
#include "RawSceneSupport.h"
#include <limits>
#include <tuple>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 크기의 소유 블록과 명시 에너지/로그 입력을 준비한다.
struct DryScene {
    RawAiBrainMemory memory{0x48000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5}); }}; //미정바이트를보존할같은소유등록부다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},backing{memory.Allocate(120)},result{},logs{}; //실제본체/12바이트열기록과명시에너지결과/로그횟수다.
    SidPool pool{OriginalEdition::Patch1078,32768,true};AiDrySpotHooks hooks; //같은판본의실제풀과선택효과다.
    std::vector<std::tuple<float,float,std::uint32_t,std::uint32_t>> calls;std::vector<bool> fakeLogs; //실제판정인자와로그분류다.
    // 원본 생성자의 내장 목록 필드만 설정한다.
    DryScene() {
        Field(0xadc,10);Field(0xae0,0);Field(0xae4,backing);Field(0xae8,0);Field(0xaec,2);Field(0x280,73);Field(0xb00,1);
        hooks.energy=[this](std::string& text,float x,float y,auto type,auto owner) { CHECK(text.empty());calls.emplace_back(x,y,type,owner);text="s";return result; };hooks.log=[this](auto brain,bool fake) { CHECK(brain==token);++logs;fakeLogs.push_back(fake); };
    }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 선택된 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 backing의 기록 한 DWORD를 읽는다.
    std::uint32_t Item(std::size_t index,std::size_t field) const { return Get(memory.Bytes(Field(0xae4)),index*12+field*4); }
    // 같은 본체/풀과 명시 하위로 제품을 조립한다.
    RawAiDrySpots Make() { return RawAiDrySpots(memory,pool,hooks); }
    // 원본 자기 경계와 비교 영역 경계를 서로 다른 좌표로 설정한다.
    void SeedBounds() {
        Field(0x248,0x20);Field(0x270,std::bit_cast<std::uint32_t>(10.0F));Field(0x274,std::bit_cast<std::uint32_t>(20.0F));Field(0x278,std::bit_cast<std::uint32_t>(30.0F));Field(0x27c,std::bit_cast<std::uint32_t>(60.0F));
        Field(0x2b8,std::bit_cast<std::uint32_t>(110.0F));Field(0x2bc,std::bit_cast<std::uint32_t>(120.0F));Field(0x2c0,std::bit_cast<std::uint32_t>(130.0F));Field(0x2c4,std::bit_cast<std::uint32_t>(160.0F));
    }
};
}
// 좌표만중복검사하며signed0은같고NaN은원본처럼서로다르다.TYPE비트는그대로저장한다.
TEST_CASE(AiDrySpots_AddCoordinatesAndRawTypeBits) {
    DryScene input;CHECK(input.Make().Add(input.token,0.0F,4,73));CHECK(!input.Make().Add(input.token,-0.0F,4,99));const auto nan=std::bit_cast<float>(0x7fc12345U);CHECK(input.Make().Add(input.token,nan,4,0x7fc01234U));CHECK(input.Make().Add(input.token,nan,4,0xdeadbeefU));CHECK(input.Field(0xae0)==3 && input.Item(0,0)==0 && input.Item(1,0)==0x7fc12345U && input.Item(1,2)==0x7fc01234U && input.Item(2,2)==0xdeadbeefU && input.calls.empty());
}
// full/빈목록은포인터에접근하지않으며빈검사는cursor를변경하지않는다.
TEST_CASE(AiDrySpots_FullAndEmptyAvoidBackingAccess) {
    DryScene input;input.Field(0xae4,0);input.Field(0xae0,10);input.hooks={};CHECK(!input.Make().Add(input.token,1,2,73));input.Field(0xae0,0);input.Field(0xae8,7);CHECK(input.Make().Scan(input.token)==0 && input.Field(0xae8)==7);CHECK(input.Make().Maintain(input.token)==0 && !input.logs);
}
// 원래cursor+1의한항목만조회하고제거해도shift된항목용cursor보정을추가하지않는다.
TEST_CASE(AiDrySpots_ScanOneAndPreservesOriginalCursor) {
    DryScene input;input.Make().Add(input.token,1,2,71);input.Make().Add(input.token,3,4,72);input.Make().Add(input.token,5,6,73);input.result=0;CHECK(input.Make().Scan(input.token)==0 && input.Field(0xae8)==1 && input.Field(0xae0)==3 && input.calls.size()==1 && std::get<2>(input.calls.back())==72);input.result=0x80000000U;CHECK(input.Make().Scan(input.token)==73 && input.Field(0xae0)==2 && input.Field(0xae8)==2);CHECK(input.Make().Scan(input.token)==72 && input.Field(0xae0)==1 && input.Field(0xae8)==1);CHECK(std::get<2>(input.calls.back())==72);
}
// 제거는12바이트기록을안정압축하고미사용꼬리/cursor를그대로둔다.
TEST_CASE(AiDrySpots_RemovePreservesTailAndUncheckedEndCount) {
    DryScene input;input.Make().Add(input.token,1,2,71);input.Make().Add(input.token,3,4,72);input.Make().Add(input.token,5,6,0x7fc01234U);input.Field(0xae8,2);const auto tail=std::vector<std::uint8_t>(input.memory.Bytes(input.backing).begin()+24,input.memory.Bytes(input.backing).end());input.Make().Remove(input.token,0);CHECK(input.Field(0xae0)==2 && input.Field(0xae8)==2 && input.Item(0,2)==72 && input.Item(1,2)==0x7fc01234U);CHECK(std::equal(tail.begin(),tail.end(),input.memory.Bytes(input.backing).begin()+24));input.Make().Remove(input.token,99);CHECK(input.Field(0xae0)==1);input.Field(0xae0,0);input.Field(0xae4,0);input.Make().Remove(input.token,0);CHECK(input.Field(0xae0)==0xffffffffU);
}
// 에너지효과가backing/cursor/count를바꾼경우현재기록TYPE와압축대상을재조회한다.
TEST_CASE(AiDrySpots_EnergyMutationUsesCurrentBackingAndCursor) {
    DryScene input;input.Make().Add(input.token,1,2,71);input.Make().Add(input.token,3,4,72);const auto replacement=input.memory.Allocate(120);Put(input.memory.Bytes(replacement),8,91);Put(input.memory.Bytes(replacement),20,92);input.hooks.energy=[&](std::string& text,float x,float y,auto type,auto owner) { CHECK(text.empty() && x==3 && y==4 && type==72 && owner==2 && input.Field(0xae8)==1);input.memory.Free(input.backing);input.Field(0xae4,replacement);input.Field(0xae8,0);input.Field(0xae0,2);input.Field(0xaec,7);input.Field(0x280,91);return 1U; };CHECK(input.Make().Maintain(input.token)==91 && input.Field(0xae0)==1 && input.Item(0,2)==92 && !input.memory.Contains(input.backing) && input.fakeLogs==std::vector<bool>({true}));
}
// 실제제거TYPE와효과뒤현재fakeTYPE/디버그값으로로그를선택한다.TYPE0제거는로그하지않는다.
TEST_CASE(AiDrySpots_MaintainUsesCurrentFakeAndDebug) {
    DryScene input;input.Make().Add(input.token,1,2,73);input.result=1;CHECK(input.Make().Maintain(input.token)==73 && input.fakeLogs==std::vector<bool>({true}));input.Make().Add(input.token,3,4,74);CHECK(input.Make().Maintain(input.token)==74 && input.fakeLogs==std::vector<bool>({true,false}));input.hooks.energy=[&](std::string&,float,float,auto,auto) { input.Field(0xb00,0);return 1U; };input.hooks.log={};input.Make().Add(input.token,5,6,75);CHECK(input.Make().Maintain(input.token)==75 && input.logs==2);input.Field(0xb00,1);input.Make().Add(input.token,7,8,0);CHECK(input.Make().Maintain(input.token)==0 && input.Field(0xae0)==0 && input.logs==2);
}
// 선택된하위누락/물리오류앞에서도이미쓴cursor는보존한다.정상범위밖나눗셈은명시거부한다.
TEST_CASE(AiDrySpots_SelectedMissingAndPhysicalBounds) {
    DryScene input;input.Make().Add(input.token,1,2,71);input.Make().Add(input.token,3,4,72);input.hooks.energy={};CHECK(Throws([&] { input.Make().Scan(input.token); }) && input.Field(0xae8)==1 && input.Field(0xae0)==2);input.Field(0xae4,0);input.Field(0xae8,0);CHECK(Throws([&] { input.Make().Scan(input.token); }) && input.Field(0xae8)==1);input.Field(0xae0,0xffffffffU);input.Field(0xae8,0x7fffffffU);CHECK(Throws([&] { input.Make().Scan(input.token); }));const auto small=input.memory.Allocate(4);CHECK(Throws([&] { input.Make().Scan(small); }));
}
// 능력은 하위 BYTE20h만 검사하며 금지 상태에서 난수/좌표/목록/효과를 바꾸지 않는다.
TEST_CASE(AiDrySpots_SeedCapabilityUsesOnlyLowByteNoEffects) {
    DryScene input;input.SeedBounds();input.Field(0x248,0x2000);input.Field(0xae4,0);input.hooks={};const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());GameRandom random(8);input.Make().Seed(input.token,random);CHECK(random.State()==8 && std::equal(before.begin(),before.end(),input.memory.Bytes(input.token).begin()) && input.calls.empty() && !input.logs);
}
// 공용 난수 한 번으로 여덟 방향 모두를 선택하고 원본 TYPE DWORD와 두 float 비트를 등록한다.
TEST_CASE(AiDrySpots_SeedEightDirectionsAndCurrentRawType) {
    constexpr std::array<std::array<float,2>,8> expected{{{70,20},{70,60},{10,90},{30,90},{20,20},{20,60},{10,40},{30,40}}}; // 각 switch 번호의 독립적인 기대 좌표다.
    // 작은 실제 시드1..8은 Next8에서1..7,0을 반환하므로 전 분기를 원본 생성기로 선택한다.
    for (std::uint32_t seed=1;seed<=8;++seed) {
        DryScene input;input.SeedBounds();input.Field(0x280,0xdeadbeef);input.Field(0xae8,7);const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());GameRandom random(seed),oracle(seed);const auto choice=oracle.Next(8);CHECK(choice==seed%8);input.Make().Seed(input.token,random);const auto x=std::bit_cast<std::uint32_t>(expected[choice][0]),y=std::bit_cast<std::uint32_t>(expected[choice][1]);CHECK(random.State()==oracle.State() && input.Field(0xaf0)==x && input.Field(0xaf4)==y && input.Field(0xae0)==1 && input.Item(0,0)==x && input.Item(0,1)==y && input.Item(0,2)==0xdeadbeef && input.Field(0xae8)==7 && input.calls.empty() && !input.logs);
        auto after=before;Put(after,0xaf0,x);Put(after,0xaf4,y);Put(after,0xae0,1);CHECK(std::equal(after.begin(),after.end(),input.memory.Bytes(input.token).begin()));
    }
}
// x87 원본의 합/곱 전체 뒤 float 저장과 중간 float 덧셈을 구별한다.
TEST_CASE(AiDrySpots_SeedStoresAfterWholeSums) {
    DryScene input;input.SeedBounds();input.Field(0x2c0,std::bit_cast<std::uint32_t>(16777216.0F));input.Field(0x2b8,std::bit_cast<std::uint32_t>(1.0F));input.Field(0x278,std::bit_cast<std::uint32_t>(-16777216.0F));input.Field(0x270,std::bit_cast<std::uint32_t>(3.0F));GameRandom random(8);input.Make().Seed(input.token,random);CHECK(input.Field(0xaf0)==std::bit_cast<std::uint32_t>(1.0F) && input.Item(0,0)==std::bit_cast<std::uint32_t>(1.0F));
}
// full은 등록을 건너뛰지만 앞서 쓴 가짜 좌표와 공용 난수 소비를 되돌리지 않는다.
TEST_CASE(AiDrySpots_SeedFullPreservesPointAndConsumesRandom) {
    DryScene input;input.SeedBounds();input.Field(0xadc,0);input.Field(0xae4,0);input.hooks={};GameRandom random(8),expected(8);expected.Next(8);input.Make().Seed(input.token,random);CHECK(!input.Field(0xae0) && input.Field(0xaf0)==std::bit_cast<std::uint32_t>(70.0F) && input.Field(0xaf4)==std::bit_cast<std::uint32_t>(20.0F) && random.State()==expected.State());
}
// 중복 좌표의 기존 TYPE를 교체하지 않으며 새 좌표/난수 소비는 유지한다.
TEST_CASE(AiDrySpots_SeedDuplicateDoesNotReplaceType) {
    DryScene input;input.SeedBounds();CHECK(input.Make().Add(input.token,70,20,91));GameRandom random(8);input.Make().Seed(input.token,random);CHECK(input.Field(0xae0)==1 && input.Item(0,2)==91 && input.Field(0x280)==73 && input.Field(0xaf0)==input.Item(0,0) && input.Field(0xaf4)==input.Item(0,1));
}
// 원본에는 fake TYPE0/지도 경계 검사가 없으므로 선택된 음수 좌표도 같은 목록에 등록한다.
TEST_CASE(AiDrySpots_SeedAllowsZeroTypeAndOutsideBounds) {
    DryScene input;input.SeedBounds();input.Field(0x280,0);input.Field(0x274,std::bit_cast<std::uint32_t>(-300.0F));GameRandom random(8);input.Make().Seed(input.token,random);CHECK(input.Field(0xae0)==1 && input.Item(0,2)==0 && input.Item(0,1)==std::bit_cast<std::uint32_t>(-300.0F));
}
// 등록 위치의 물리 오류는 앞선 좌표/난수 쓰기를 유지하고 count를 가짜 성공으로 증가시키지 않는다.
TEST_CASE(AiDrySpots_SeedPhysicalFailureRetainsPointAndRandom) {
    DryScene input;input.SeedBounds();input.Field(0xae4,0);GameRandom random(8),expected(8);expected.Next(8);CHECK(Throws([&] { input.Make().Seed(input.token,random); }) && random.State()==expected.State() && input.Field(0xaf0)==std::bit_cast<std::uint32_t>(70.0F) && input.Field(0xaf4)==std::bit_cast<std::uint32_t>(20.0F) && !input.Field(0xae0));
}
