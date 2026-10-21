// AI 행동 말미의 같은 지갑/SP·현재 콜백·타이머 계약을 합성 입력으로 검사한다. 작성·컴파일만 한다.
#include "o/RawAiEconomy.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 본체와 실제 난수/분산SP를 하나의 검사 장면에 연결한다.
struct EconomyInput {
    RawAiBrainMemory memory{0x44000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //명시0 본체 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //실제로 소유하는 같은 본체다.
    GameRandom random{0x12345678};ScrambledSpStore sp{random,[] { return 19U; }}; //지원 전후 실제SP Set의 난수를 공유한다.
    double now{16777216.25};bool demo{}; //현재 QWORD 시각과 데모 입력이다.
    std::vector<std::uint32_t> queries;std::uint32_t demos{}; //실제 콜백 순서/횟수의 관찰값이다.
    AiEconomyHooks hooks; //각 검사에서 선택 효과만 교체한다.
    // 같은 지갑·타이머를 초기 입력으로 준비한다.
    EconomyInput() { Field(0x260,1);Field(0x264,2);Field(0x2d8,100);Field(0x290,500);Field(0x24c,7);hooks.demo=[this] { ++demos;return demo; };hooks.captured=[this](auto owner) { queries.push_back(owner);return false; }; }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 지정 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 QWORD 기한을 명시 입력으로 쓴다.
    void Due(double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Field(0x5d8,static_cast<std::uint32_t>(bits));Field(0x5dc,static_cast<std::uint32_t>(bits>>32)); }
    // 실제 본체에 기록된 QWORD 기한을 읽는다.
    double Due() const { return std::bit_cast<double>((std::uint64_t(Field(0x5dc))<<32)|Field(0x5d8)); }
    // 복사본 없이 실제 저장소를 연결한다.
    RawAiEconomy Make() { return RawAiEconomy(memory,sp,now,hooks); }
};

// 기한과 같을 때만 단일 지원을 수행하며 큰 시간 점프도 누적 보충 반복으로 바꾸지 않는다.
TEST_CASE(AiEconomy_TickBoundaryAndSingleSupport) {
    EconomyInput input;input.Due(input.now+0.25);auto economy=input.Make();economy.Reconcile(input.token);CHECK(input.Field(0x2d8)==100 && input.sp.Get(1)==std::bit_cast<std::uint32_t>(100.0F) && input.demos==0 && input.queries==std::vector<std::uint32_t>({2,1}));input.now+=0.25;economy.Reconcile(input.token);CHECK(input.Field(0x2d8)==107 && input.Due()==16777217.5 && input.demos==1);economy.Reconcile(input.token);CHECK(input.Field(0x2d8)==107 && input.demos==1);input.now+=100;economy.Reconcile(input.token);CHECK(input.Field(0x2d8)==114 && input.Due()==input.now+1 && input.demos==2);
}
// 일반 지원의 DWORD 감김과 데모 하한을 signed 값으로 적용한다.
TEST_CASE(AiEconomy_SignedSupportAndDemoFloor) {
    EconomyInput normal;normal.Field(0x2d8,0x80000010);normal.Field(0x24c,0xffffffffU);normal.Make().Reconcile(normal.token);CHECK(normal.Field(0x2d8)==0x8000000fU && normal.sp.Get(1)==std::bit_cast<std::uint32_t>(static_cast<float>(std::bit_cast<std::int32_t>(0x8000000fU))));
    EconomyInput demo;demo.demo=true;demo.Field(0x2d8,0);demo.Field(0x24c,0x80000000U);demo.Make().Reconcile(demo.token);CHECK(demo.Field(0x2d8)==1190 && demo.sp.Get(1)==std::bit_cast<std::uint32_t>(1190.0F));
    EconomyInput ceiling;ceiling.Field(0x2d8,2700);ceiling.Make().Reconcile(ceiling.token);CHECK(ceiling.Field(0x2d8)==2700 && ceiling.demos==0 && ceiling.Due()==ceiling.now+1);
}
// 포획/데모 하위가 바꾼 owner·비용·지갑을 다음 원문 읽기에서 다시 사용한다.
TEST_CASE(AiEconomy_CaptureBudgetRereadsCurrentBody) {
    EconomyInput input;input.Due(input.now+100);input.hooks.captured=[&](auto owner) { input.queries.push_back(owner);if (owner==2) { input.Field(0x290,100);return true; }CHECK(owner==3);input.Field(0x290,5000);return true; };input.hooks.demo=[&] { ++input.demos;if (input.demos==1) { CHECK(input.Field(0x2d8)==2300);input.Field(0x260,3);input.Field(0x2d8,5000); }return true; };input.Make().Reconcile(input.token);CHECK(input.queries==std::vector<std::uint32_t>({2,3}) && input.demos==2 && input.Field(0x2d8)==7200 && input.sp.Get(3)==std::bit_cast<std::uint32_t>(7200.0F));
}
// 비용+2200도 DWORD 감김 뒤 비교하며 데모 클램프는 현재 signed 지갑을 다시 읽는다.
TEST_CASE(AiEconomy_CaptureBudgetOverflowAndCurrentDemoFloor) {
    EconomyInput input;input.Due(input.now+100);input.Field(0x290,0x7fffffffU);input.Field(0x2d8,0x80000000U);input.hooks.captured=[](auto owner) { return owner==2; };input.hooks.demo=[&] { CHECK(input.Field(0x2d8)==0x80000897U);input.Field(0x2d8,1191);return true; };input.Make().Reconcile(input.token);CHECK(input.Field(0x2d8)==1191 && input.sp.Get(1)==std::bit_cast<std::uint32_t>(1191.0F));
}
// 최종SP Set은 매번 수행하며 float 반올림 때문에 본체 정수 지갑을 바꾸지 않는다.
TEST_CASE(AiEconomy_FinalSpUsesSameStoreWithoutWalletCredit) {
    EconomyInput input;input.Due(input.now+100);input.Field(0x2d8,16777217);input.hooks.demo={};const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());auto economy=input.Make();economy.Reconcile(input.token);CHECK(input.sp.Get(1)==std::bit_cast<std::uint32_t>(16777216.0F) && input.Field(0x2d8)==16777217 && std::equal(before.begin(),before.end(),input.memory.Bytes(input.token).begin()));const auto random=input.random.State();economy.Reconcile(input.token);CHECK(input.random.State()!=random && input.Field(0x2d8)==16777217);
}
// unordered 타이머는 지원을 생략해도 포획/최종SP 처리는 계속하며 누락된 실제 하위는 실패한다.
TEST_CASE(AiEconomy_UnorderedAndSelectedMissingHooks) {
    EconomyInput input;input.Due(std::numeric_limits<double>::quiet_NaN());input.hooks.demo={};input.Make().Reconcile(input.token);CHECK(input.Field(0x2d8)==100 && input.queries==std::vector<std::uint32_t>({2,1}) && std::isnan(input.Due()));
    EconomyInput missing;missing.hooks.demo={};CHECK(Throws([&] { missing.Make().Reconcile(missing.token); }));CHECK(missing.Field(0x2d8)==107 && missing.Due()==missing.now+1 && !missing.sp.Initialized());
    EconomyInput capture;capture.Due(capture.now+100);capture.hooks.captured={};CHECK(Throws([&] { capture.Make().Reconcile(capture.token); }));CHECK(!capture.sp.Initialized());const auto small=capture.memory.Allocate(4);CHECK(Throws([&] { capture.Make().Reconcile(small); }));
}
}
