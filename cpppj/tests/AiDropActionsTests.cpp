// 원본 전체 배치 행동의 관문/현재 인자·로그/난수·부분 효과를 검사한다.작성·컴파일만 한다.
#include "o/RawAiDropActions.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 배치 경계에서 받은 원래 여섯 인자다.
struct DropActionRequest { std::uint32_t type{};RawPathPoint point;std::uint32_t argument{},direction{}; }; //시설TYPE/후보XY/argument0/현재방향이다.
// 같은 실제 본체·풀과 기존 실제 후보 압축기를 명시 원문 효과에 연결한다.
struct DropActionScene {
    RawAiBrainMemory memory{0x55000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //실제 본체 저장소다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};std::vector<RiftTypeRecord> types{188};std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; //같은 실제 풀/현재 TYPE다.
    std::vector<std::uint8_t> spots{std::vector<std::uint8_t>(65536)};std::vector<std::uint16_t> surfaces{std::vector<std::uint16_t>(65536)};std::array<std::uint32_t,81> allies{};AiBuildSearchBounds bounds{10,10,246,246};std::uint32_t bridge{82}; //실제 후보 압축기의 같은 입력이다.
    GameRandom random{1};double now{100};std::int32_t limit{256};bool checking{},product{true},pending{},afford{true};std::uint32_t selectedType{128}; //원본 공용 난수/현재 전역과 명시 하위 입력이다.
    AiDropActionHooks hooks;std::unique_ptr<RawAiDropScan> scan;std::vector<std::string> trace;std::vector<std::uint32_t> products,scanTypes;std::vector<std::int32_t> scanDirections;std::vector<CanonPlacementQuery> queries;std::vector<bool> legal;std::vector<DropActionRequest> deployments;std::vector<AiDropActionNotice> notices; //원문 호출/인자 관찰이다.
    std::function<void(std::size_t)> duringProduct,duringPlace,duringRemove;std::function<void()> duringSelect,duringScan,duringAfford,duringDeploy,duringAssertion;std::function<void(AiDropActionLog)> duringLog; //각 실제 효과 뒤의 명시 현재 상태 변경이다.
    // 기본은 TYPE128/스캔완료/지갑2000/빈후보이며 이전 성공·표적·집중값은 남겨 둔다.
    DropActionScene() {
        CHECK(pool.Allocate(2)==Sid{5} && pool.Allocate(2)==Sid{6} && pool.Allocate(2)==Sid{7});auto productRaw=pool.AllocatedBytes(Sid{5});std::fill(productRaw.begin(),productRaw.end(),std::uint8_t{});productRaw[10]=6;
        // 두 표적은 후보 좌표와 서로 다른 raw 위치/TYPE를 사용한다.
        for (auto sid:{Sid{6},Sid{7}}) { auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=sid.value==6 ? 150 : 151;Put(raw,14,std::bit_cast<std::uint32_t>(sid.value==6 ? 71.8F : 93.9F));Put(raw,18,std::bit_cast<std::uint32_t>(sid.value==6 ? 82.7F : 104.1F)); }
        Field(0x260,1);Field(0x3e0,128);Field(0x3e4,2);Field(0x3ec,1);Field(0x2d8,2000);Field(0x3f4,77);Field(0x3fc,88);Field(0x3f8,99);Field(0xb00,1);
        AiDropScanHooks compression;compression.assertion=[](auto,auto) { throw std::logic_error("예상하지 않은 빈 후보 압축"); };scan=std::make_unique<RawAiDropScan>(memory,pool,types,frames,spots,surfaces,allies,random,AiDropScanGlobals{bridge,bounds},std::move(compression));
        hooks.product=[this](auto brain,auto type) { CHECK(brain==token);const auto call=products.size();products.push_back(type);trace.push_back("product");const auto result=product ? Sid{5} : Sid{};if (duringProduct) duringProduct(call);return result; };
        hooks.select=[this](auto brain) { CHECK(brain==token);trace.push_back("select");Field(0x3e0,selectedType);if (duringSelect) duringSelect(); };hooks.scan=[this](auto brain,auto type,auto direction) { CHECK(brain==token);trace.push_back("scan");scanTypes.push_back(type);scanDirections.push_back(direction);if (duringScan) duringScan();return pending; };
        hooks.canAfford=[this](auto brain,auto type) { CHECK(brain==token && type==Field(0x3e0));trace.push_back("afford");if (duringAfford) duringAfford();return afford; };hooks.mayPlace=[this](const CanonPlacementQuery& query) { const auto call=queries.size();queries.push_back(query);trace.push_back("place");const auto result=legal.empty() ? true : legal.at(std::min(call,legal.size()-1));if (duringPlace) duringPlace(call);return result; };
        hooks.deploy=[this](auto brain,auto type,RawPathPoint point,auto argument,auto direction) { CHECK(brain==token);trace.push_back("deploy");deployments.push_back({type,point,argument,direction});if (duringDeploy) duringDeploy(); };hooks.fake=[this](auto brain) { CHECK(brain==token);trace.push_back("fake"); };hooks.seed=[this](auto brain) { CHECK(brain==token);trace.push_back("seed"); };
        hooks.remove=[this](auto brain,auto index) { CHECK(brain==token);trace.push_back("remove");scan->Remove(brain,index);if (duringRemove) duringRemove(static_cast<std::size_t>(index)); };hooks.assertion=[this](std::string_view text,auto line) { CHECK(text=="drop.value == 0" && line==0xf8b);trace.push_back("assert");if (duringAssertion) duringAssertion(); };
        hooks.log=[this](auto brain,const AiDropActionNotice& notice) { CHECK(brain==token);constexpr std::array<const char*,7> names{"lost","complete","partial","cannot","bad","lay","target"}; //원래 알림의 검사 trace 이름이다.
            trace.push_back(names.at(static_cast<std::size_t>(notice.action)));notices.push_back(notice);if (duringLog) duringLog(notice.action); };
    }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 지정 본체 DWORD를 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // QWORD 기한을 float 축소 없이 저장한다.
    void Time(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Field(at,static_cast<std::uint32_t>(bits));Field(at+4,static_cast<std::uint32_t>(bits>>32)); }
    // 실제 선택 배열에 후보와 표적 DWORD를 기록한다.
    void Choice(std::size_t index,float x,float y,std::uint32_t target=6) { Field(0x408+index*8,std::bit_cast<std::uint32_t>(x));Field(0x40c+index*8,std::bit_cast<std::uint32_t>(y));Field(0x4f8+index*4,target); }
    // 같은 실제 본체/풀/공용 난수와 명시 원문 하위로 행동한다.
    void Run() { RawAiDropActions(memory,pool,types,random,{now,limit,checking},hooks).Run(token); }
};
}
// 초기TYPE0에서는 생산조회를 하지 않으며 선택 뒤TYPE0이면 뒤 스캔/관문도 생략한다.
TEST_CASE(AiDropActions_SelectionZeroStopsAtNativeBoundary) {
    DropActionScene input;input.Field(0x3e0,0);input.selectedType=0;input.Run();CHECK(input.trace==std::vector<std::string>({"select"}) && input.Field(0x3f4)==77 && input.Field(0x3fc)==88 && !input.Field(0x3f0) && input.random.State()==1);
}
// 생산능력 상실 알림 자체는TYPE를 지우거나 재선택을 강제하지 않는다.
TEST_CASE(AiDropActions_LostAbilityDoesNotForceSelection) {
    DropActionScene input;input.product=false;input.pending=true;input.Field(0x3ec,0);input.Run();CHECK(input.trace==std::vector<std::string>({"product","lost","scan","product"}) && input.Field(0x3e0)==128 && !input.Field(0x3f0) && !input.Field(0x3ec) && input.Field(0x3f4)==77);
}
// 스캔 입력은 선택 직후 snapshot이고 완료 로그는 스캔 뒤 현재TYPE/방향과 이미 쓴 완료flag다.
TEST_CASE(AiDropActions_ScanSnapshotAndCompletionBeforeGates) {
    DropActionScene input;input.Field(0x3e0,0);input.Field(0x3ec,0);input.duringScan=[&] { input.Field(0x3e0,129);input.Field(0x3e4,4); };input.duringLog=[&](auto action) { if (action==AiDropActionLog::ScanComplete) { CHECK(input.Field(0x3ec)==1);input.Time(0x5a8,101); } };input.Run();CHECK(input.trace==std::vector<std::string>({"select","scan","complete"}) && input.scanTypes==std::vector<std::uint32_t>({128}) && input.scanDirections==std::vector<std::int32_t>({2}) && input.notices[0].type==129 && input.notices[0].direction==4);
}
// 미래/NaN의두 기한과 NaN 현재시각은 스캔 뒤 관문에서 ordered 미도달이다.
TEST_CASE(AiDropActions_OrderedQwordDeadlines) {
    const auto nan=std::numeric_limits<double>::quiet_NaN(); //원본 fcomp unordered 입력이다.
    // 두 기한을 각각 검증하고 equality는 통과시키며 float로 차이를 지우지 않는다.
    for (auto at:{0x5a8U,0x5e8U}) { for (double due:{100.00000001,nan}) { DropActionScene input;input.Time(at,due);input.Run();CHECK(input.trace==std::vector<std::string>({"product"})); } } //안쪽 반복은 미래/NaN을 각 기한에 적용한다.
    DropActionScene now;now.now=nan;now.Run();CHECK(now.trace==std::vector<std::string>({"product"}));DropActionScene equal;equal.Time(0x5a8,100);equal.Time(0x5e8,100);equal.Run();CHECK(equal.trace==std::vector<std::string>({"product","product","seed"}));
}
// 지갑은 signed1200이상일 때만 두 번째 생산조회/후보 행동을 진행한다.
TEST_CASE(AiDropActions_SignedWalletMinimum) {
    // 1199/음수 DWORD/정확한1200을 구분한다.
    for (auto wallet:{1199U,0xffffffffU,0x80000000U,1200U}) { DropActionScene input;input.Field(0x2d8,wallet);input.Run();CHECK(input.trace==(wallet==1200 ? std::vector<std::string>({"product","product","seed"}) : std::vector<std::string>({"product"}))); }
}
// 최초시도400은 완료를 기다리고 재사용 시도는 스캔 미완료에서도 원래 Partial 알림 뒤 진행한다.
TEST_CASE(AiDropActions_FirstAttemptAndPartialScan) {
    DropActionScene first;first.Field(0x400,1);first.Field(0x3ec,0);first.pending=true;first.Run();CHECK(first.trace==std::vector<std::string>({"product","scan"}) && !first.Field(0x3f0));DropActionScene reuse;reuse.Field(0x3ec,0);reuse.pending=true;reuse.Run();CHECK(reuse.trace==std::vector<std::string>({"product","scan","product","partial","seed"}) && reuse.Field(0x3f0)==1);
}
// 현재 생산조각의WORD100만 거르고 free/dead나 다른 pending비트를 추가 필터하지 않는다.
TEST_CASE(AiDropActions_CurrentPendingBitOnly) {
    DropActionScene input;input.pool.AllocatedBytes(Sid{5})[21]=1;input.Run();CHECK(input.trace==std::vector<std::string>({"product","product"}));DropActionScene other;other.pool.AllocatedBytes(Sid{5})[21]=2;other.pool.AllocatedBytes(Sid{5})[11]|=2;other.Run();CHECK(other.trace==std::vector<std::string>({"product","product","seed"}));
}
// 사제 탐색은 signed 진행 비교이고 유효한 목표 좌표만 배치를 막는다.
TEST_CASE(AiDropActions_PriestProgressAndStrictTarget) {
    DropActionScene searching;searching.Field(0x5a0,2);searching.Field(0x5a4,1);searching.Run();CHECK(searching.trace==std::vector<std::string>({"product","product"}));DropActionScene signedProgress;signedProgress.Field(0x5a0,0xffffffff);signedProgress.Run();CHECK(signedProgress.Field(0x3f0)==1);
    // strict 유효 좌표/0/NaN의 목표관문을 서로 구분한다.
    for (float x:{10.0F,0.0F,std::numeric_limits<float>::quiet_NaN()}) { DropActionScene input;input.Field(0xad0,std::bit_cast<std::uint32_t>(x));input.Field(0xad4,std::bit_cast<std::uint32_t>(10.0F));input.Run();CHECK(input.Field(0x3f0)==(x==10.0F ? 0U : 1U)); }
}
// 빈 후보 진단에서 복귀하면 현재TYPE flags를 다시 읽어 fake/seed를 선택하며 직전 성공은 보존한다.
TEST_CASE(AiDropActions_NoChoiceAssertionUsesCurrentType) {
    DropActionScene input;input.checking=true;input.Field(0x3e8,9);input.types[136].flags2=0x800;input.duringAssertion=[&] { input.Field(0x3e0,136); };input.Run();CHECK(input.trace==std::vector<std::string>({"product","product","assert","fake"}) && input.Field(0x3f0)==1 && input.Field(0x3f4)==77 && input.Field(0x3fc)==88 && input.random.State()==1);
}
// 비용 거부는 기존 성공/표적·집중/후보를 지우기 전이며 현재TYPE 이름으로 알린다.
TEST_CASE(AiDropActions_CannotAffordPreservesPriorState) {
    DropActionScene input;input.Field(0x570,1);input.Choice(0,11,12);input.afford=false;input.duringAfford=[&] { input.Field(0x3e0,129); };input.Run();CHECK(input.trace==std::vector<std::string>({"product","product","afford","cannot"}) && input.notices[0].type==129 && input.Field(0x3f4)==77 && input.Field(0x3fc)==88 && input.Field(0x3f8)==99 && input.Field(0x570)==1 && input.Field(0x3f0)==1 && input.random.State()==1);
}
// 공용 난수를 현재 count로 소비하고 실패 후보도 기존 실제 압축기로 제거한다.
TEST_CASE(AiDropActions_RandomRejectionsUseActualCompression) {
    DropActionScene input;input.Field(0x570,3);input.Choice(0,11,12);input.Choice(1,21,22);input.Choice(2,31,32);input.legal={false,false,true};GameRandom expected(1);std::vector<float> remaining{11,21,31},chosen; //독립적인 세 후보와 난수 소비 기준이다.
    // 실제 원본 난수로 매 압축 전 index를 계산하며 기대 후보의 상대 순서를 유지한다.
    while (!remaining.empty()) { const auto index=expected.Next(static_cast<std::uint32_t>(remaining.size()));chosen.push_back(remaining[index]);remaining.erase(remaining.begin()+index); }
    input.Run();CHECK(input.queries.size()==3 && input.deployments.size()==1 && input.random.State()==expected.State() && !input.Field(0x570) && input.Field(0x3f4)==1 && input.Field(0x3fc)==6 && input.Field(0x3f8)==99 && input.Field(0x3f0)==1);for (std::size_t index=0;index<3;++index) CHECK(input.queries[index].x==chosen[index]); //반복문은 압축 전 각각의 현재 선택 좌표를 비교한다.
    CHECK(input.deployments[0].point.x==chosen.back() && input.notices.back().action==AiDropActionLog::Target && input.notices.back().type==150 && input.notices.back().x==71 && input.notices.back().y==82);
}
// 모든 후보 거부는 이전 성공/표적만 지우고 집중표적을 건드리지 않는다.
TEST_CASE(AiDropActions_AllChoicesInvalidKeepFocus) {
    DropActionScene input;input.Field(0x570,2);input.Choice(0,11,12);input.Choice(1,21,22);input.legal={false,false};input.Run();CHECK(input.queries.size()==2 && input.deployments.empty() && !input.Field(0x570) && !input.Field(0x3f4) && !input.Field(0x3fc) && input.Field(0x3f8)==99 && input.Field(0x3f0)==1);
}
// 승인/로그/실제배치 뒤의 현재 후보·TYPE/방향·표적과 하위BYTE owner/focus를 각각 읽는다.
TEST_CASE(AiDropActions_CurrentQueriesLogsAndDeployArguments) {
    DropActionScene input;input.Field(0x570,1);input.Field(0x260,0x101);input.Choice(0,11.8F,12.8F);input.duringPlace=[&](auto) { input.Field(0x3e0,129);input.Field(0x3e4,6);input.Choice(0,21.9F,22.9F); };
    input.duringLog=[&](auto action) { if (action==AiDropActionLog::Laying) input.Field(0x4f8,7);if (action==AiDropActionLog::Target) { input.Field(0x3e0,130);input.Field(0x3e4,8);input.Choice(0,31.2F,32.2F,7); } };input.duringDeploy=[&] { input.Field(0x4f8,6);input.Field(0x248,0x40); };input.Run();const auto& query=input.queries[0];const auto& deployed=input.deployments[0];CHECK(query.type==128 && query.owner==1 && !query.argument && !query.flags && query.mode==1 && query.x==11.8F && query.y==12.8F);CHECK(input.notices[0].type==129 && input.notices[0].x==21 && input.notices[0].y==22 && input.notices[1].type==151 && input.notices[1].x==93 && input.notices[1].y==104);CHECK(deployed.type==130 && deployed.point.x==31.2F && deployed.point.y==32.2F && !deployed.argument && deployed.direction==8 && input.Field(0x3fc)==6 && input.Field(0x3f8)==6);
}
// 집중은 BYTE40h에서만 선택하며 상위BYTE40h는 이전 집중표적을 유지한다.
TEST_CASE(AiDropActions_FocusUsesLowByteOnly) {
    // 실제 현재 능력값 두 가지로 원문 BYTE 검사를 구분한다.
    for (auto capability:{0x40U,0x4000U}) { DropActionScene input;input.Field(0x248,capability);input.Field(0x570,1);input.Choice(0,11,12);input.Run();CHECK(input.Field(0x3f8)==(capability==0x40 ? 6U : 99U)); }
}
// 중단은 승인결과의 local이 아니라 실제 압축 효과 뒤 현재3f4로만 판단한다.
TEST_CASE(AiDropActions_RemoveEffectControlsStopping) {
    DropActionScene invalid;invalid.Field(0x570,1);invalid.Choice(0,11,12);invalid.legal={false};invalid.duringRemove=[&](auto) { invalid.Field(0x3f4,1); };invalid.Run();CHECK(invalid.deployments.empty() && invalid.Field(0x3f4)==1 && invalid.Field(0x3f0)==1);
    DropActionScene twice;twice.Field(0x570,2);twice.Choice(0,11,12);twice.Choice(1,21,22);twice.duringRemove=[&](auto) { if (twice.Field(0x570)) twice.Field(0x3f4,0); };twice.Run();CHECK(twice.deployments.size()==2 && !twice.Field(0x570) && twice.Field(0x3f4)==1);
}
// 비용 비교가 count를0으로 바꿔도 이미 들어온 후보 분기는 이전 성공/표적을 지운다.
TEST_CASE(AiDropActions_AffordMutationClearsWithoutRandom) {
    DropActionScene input;input.Field(0x570,1);input.Choice(0,11,12);input.duringAfford=[&] { input.Field(0x570,0); };input.Run();CHECK(input.trace==std::vector<std::string>({"product","product","afford"}) && !input.Field(0x3f4) && !input.Field(0x3fc) && input.Field(0x3f0)==1 && input.random.State()==1);
}
// 실제 배치/압축 경계 누락은 이미 수행한 난수/로그/성공 쓰기를 취소하지 않는다.
TEST_CASE(AiDropActions_MissingEffectsPreservePartialWrites) {
    DropActionScene input;input.Field(0x570,1);input.Choice(0,11,12);input.hooks.deploy={};CHECK(Throws([&] { input.Run(); }) && input.Field(0x570)==1 && !input.Field(0x3f4) && !input.Field(0x3fc) && !input.Field(0x3f0) && input.random.State()!=1 && input.notices.size()==2);DropActionScene removed;removed.Field(0x570,1);removed.Choice(0,11,12);removed.hooks.remove={};CHECK(Throws([&] { removed.Run(); }) && removed.Field(0x3f4)==1 && removed.Field(0x3fc)==6 && removed.Field(0x570)==1 && !removed.Field(0x3f0));
}
// 로그가 꺼져도 승인 뒤 현재 표적/TYPE 조회는 생략하지 않는다.
TEST_CASE(AiDropActions_DisabledLogStillReadsPhysicalTarget) {
    DropActionScene input;input.Field(0x570,1);input.Choice(0,11,12,65535);input.Field(0xb00,0);input.hooks.log={};CHECK(Throws([&] { input.Run(); }) && input.deployments.empty() && input.Field(0x570)==1 && !input.Field(0x3f4));
}
