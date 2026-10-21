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
