// 원본 기술 순환/회전·직전 후보 재사용/현재 상태·unordered 기한을 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDropSelection.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 본체/후보 압축·공용 난수와 명시 기술 getter를 준비한다.
struct DropSelectionScene {
    RawAiBrainMemory memory{0x53000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},list{memory.Allocate(32)}; // 본체와 직전 표적 목록의 실제 저장소다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};std::vector<RiftTypeRecord> types{188};std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; // 실제 풀과 현재 TYPE/frame이다.
    std::vector<std::uint8_t> spots{std::vector<std::uint8_t>(65536)};std::vector<std::uint16_t> surfaces{std::vector<std::uint16_t>(65536)};std::array<std::uint32_t,81> allies{};AiBuildSearchBounds bounds{10,10,246,246};std::uint32_t bridge{82}; // 기존 실제 후보 압축기를 연결하는 동일 지도 입력이다.
    GameRandom random{1};double now{100};std::array<std::uint32_t,4> directional{131,129,140,142}; // 현재 공용 난수/시각/네 TYPE 전역이다.
    Sid parent{5},blocker{};std::int32_t count{2};std::vector<std::uint32_t> products{128,129}; // 기술 부모/개수/실제 ordinal 반환의 명시 입력이다.
    AiDropSelectionHooks hooks;std::unique_ptr<RawAiDropScan> scan;std::vector<std::string> trace;std::vector<CanonPlacementQuery> queries;std::vector<bool> legal; // 같은 후보 압축과 선택된 하위 기록이다.
    std::function<void()> duringTechnology,duringCount,duringBlocker,duringNth;std::function<void(AiDropSelectionLog)> duringLog;std::function<void(std::size_t)> duringPlace; // 각 원문 하위의 명시 현재 상태 변경이다.
    // 기본은 새 순환 선택/빈 후보/직전 점수123이며 최초 ordinal0에서1로 진행한다.
    DropSelectionScene() {
        CHECK(pool.Allocate(2)==Sid{5});Field(0x260,1);Field(0x36c,list);Field(0x3e8,123);Field(0x3f0,9);Field(0x3ec,7);Field(0xaf8,8);Field(0xafc,9);AiDropScanHooks remove;remove.assertion=[](std::string_view,auto) { throw std::logic_error("명시 후보 압축 진단"); };scan=std::make_unique<RawAiDropScan>(memory,pool,types,frames,spots,surfaces,allies,random,AiDropScanGlobals{bridge,bounds},std::move(remove));
        hooks.technology=[this](auto owner) { CHECK(Field(0xaf8)==10 && Field(0xafc)==10 && !Field(0x3ec));trace.push_back("tech:"+std::to_string(owner));const auto saved=parent;if (duringTechnology) duringTechnology();return saved; };
        hooks.count=[this](Sid technology) { trace.push_back("count:"+std::to_string(technology.value));const auto saved=count;if (duringCount) duringCount();return saved; };
        hooks.blocker=[this](Sid technology) { trace.push_back("blocker:"+std::to_string(technology.value));if (duringBlocker) duringBlocker();return blocker; };
        hooks.nth=[this](Sid technology,auto index) { trace.push_back("nth:"+std::to_string(technology.value)+":"+std::to_string(index));if (duringNth) duringNth();return index<products.size() ? products[index] : 0U; };
        hooks.mayPlace=[this](const CanonPlacementQuery& query) { const auto index=queries.size();queries.push_back(query);trace.push_back("place:"+std::to_string(static_cast<int>(query.x)));if (duringPlace) duringPlace(index);return legal.empty() || legal.at(std::min(index,legal.size()-1)); };
        hooks.remove=[this](auto brain,auto index) { CHECK(brain==token);trace.push_back("remove:"+std::to_string(index));scan->Remove(brain,index); };
        hooks.log=[this](auto brain,AiDropSelectionLog action,auto type,auto direction) { CHECK(brain==token);trace.push_back(action==AiDropSelectionLog::Hated ? "hate" : action==AiDropSelectionLog::Retried ? "retry" : "scan:"+std::to_string(type)+":"+std::to_string(direction));if (duringLog) duringLog(action); };
    }
    // 지정 본체 DWORD만 입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 원본 QWORD 기한을 축소 없이 입력한다.
    void Time(std::size_t at,double value) { const auto raw=std::bit_cast<std::uint64_t>(value);Field(at,static_cast<std::uint32_t>(raw));Field(at+4,static_cast<std::uint32_t>(raw>>32)); }
    // 직전 표적 목록의 순서/중복을 그대로 기록한다.
    void List(std::initializer_list<std::uint32_t> values) { Field(0x374,static_cast<std::uint32_t>(values.size()));std::size_t at{};for (auto value:values) { Put(memory.Bytes(list),at,value);at+=4; } } // 반복문은 입력 목록 순서를 보존한다.
    // 실제 같은 후보 XY/표적 배열의 명시 입력이다.
    void Choice(std::size_t index,float x,float y,std::uint32_t target) { Field(0x408+index*8,std::bit_cast<std::uint32_t>(x));Field(0x40c+index*8,std::bit_cast<std::uint32_t>(y));Field(0x4f8+index*4,target); }
    // 원본 반복 관문을 통과할 명시 직전 상태를 준비한다.
    void Repeat(std::uint32_t type=128) { Field(0x248,0x80);Field(0x3f4,1);Field(0x3e0,type);Field(0x3fc,77);Field(0xb00,1);List({77}); }
    // 현재 실제 저장소와 하위로 한 번 선택한다.
    void Select() { RawAiDropSelection body(memory,pool,types,directional,random,now,hooks);CHECK(&body.Pool()==&pool);body.Select(token); }
};
}

// 기술0개는 진행/완료표식·TYPE/재선택만 쓰고 후보·점수·직전 상태는 보존한다.
TEST_CASE(AiDropSelection_EmptyTechnologyOnlyWritesNativePrefix) {
    DropSelectionScene scene;scene.count=0;scene.Repeat();scene.Field(0x3e4,6);scene.Field(0x570,2);scene.Field(0x404,1);scene.Select();CHECK(scene.trace==std::vector<std::string>({"tech:1","count:5"}) && !scene.Field(0x3e0) && scene.Field(0x3f0)==1 && scene.Field(0x3e4)==6 && scene.Field(0x570)==2 && scene.Field(0x3e8)==123 && scene.Field(0x3f4)==1 && scene.Field(0x3fc)==77 && scene.Field(0x404)==1 && scene.random.State()==1);
}
// 현재 owner 조회는 한 번이고 나머지는 최초 기술부모/개수를 계속 사용한다.
TEST_CASE(AiDropSelection_CachesActualTechnologyAndCount) {
    DropSelectionScene scene;scene.Field(0xb00,1);scene.duringTechnology=[&] { scene.Field(0x260,2);scene.parent=Sid{6}; };scene.duringCount=[&] { scene.count=0; };scene.Select();CHECK(scene.trace==std::vector<std::string>({"tech:1","count:5","nth:5:1","scan:129:0"}) && scene.Field(0x3e0)==129 && scene.Field(0x3dc)==1 && !scene.Field(0x3e8) && !scene.Field(0x3f0) && scene.Field(0x400)==1);
}
// 네 현재 전역만 전체DWORD로 비교하고6미만 회전은2씩 증가한다.
TEST_CASE(AiDropSelection_CurrentDirectionalTypesAndRotation) {
    DropSelectionScene scene;RawAiDropSelection body(scene.memory,scene.pool,scene.types,scene.directional,scene.random,scene.now,scene.hooks);CHECK(body.Directional(131) && !body.Directional(0x10083));scene.directional[0]=130;CHECK(body.Directional(130) && !body.Directional(131));scene.Field(0x3e0,130);scene.Field(0x3e4,4);scene.Select();CHECK(scene.Field(0x3e0)==130 && scene.Field(0x3e4)==6 && scene.trace==std::vector<std::string>({"tech:1","count:5"}));
}
// 방향은 signed 비교라 홀수/음수도 원본 그대로2를 더하고6이상은 기술을 순환한다.
TEST_CASE(AiDropSelection_SignedOddRotationAndOrdinalAdvance) {
    DropSelectionScene scene;scene.Field(0x3e0,131);scene.Field(0x3e4,5);scene.Select();CHECK(scene.Field(0x3e4)==7 && scene.Field(0x3e0)==131);scene.trace.clear();scene.Field(0x3e4,0xffffffffU);scene.Select();CHECK(scene.Field(0x3e4)==1);scene.Field(0x3e4,6);scene.Select();CHECK(scene.Field(0x3e0)==129 && !scene.Field(0x3e4) && scene.Field(0x3dc)==1);
}
// ordinal은 DWORD증가 뒤 signed 나머지이며 음수는 다른 양수 인덱스로 고치지 않는다.
TEST_CASE(AiDropSelection_OrdinalWrapAndSignedRemainder) {
    DropSelectionScene scene;scene.Field(0x3dc,0xffffffffU);scene.Select();CHECK(scene.Field(0x3dc)==0 && scene.Field(0x3e0)==128);scene.Field(0x3e0,0);scene.Field(0x3dc,0xfffffffeU);scene.Select();CHECK(scene.Field(0x3dc)==0xffffffffU && !scene.Field(0x3e0) && scene.trace.back()=="nth:5:4294967295");
}
// 하위 BYTE80h는 직전 배치가 없어도 난수100을 쓰며 상위BYTE80h는 쓰지 않는다.
TEST_CASE(AiDropSelection_RepeatByteConsumesRandomBeforeOtherGates) {
    DropSelectionScene scene;scene.Field(0x248,0x80);GameRandom expected(1);expected.Next(100);scene.Select();CHECK(scene.random.State()==expected.State());DropSelectionScene higher;higher.Field(0x248,0x8000);higher.Select();CHECK(higher.random.State()==1);
}
// 실제 목록 압축 뒤 같은index에서 다음 후보를 재검사하고 직전 점수·회전을 유지한다.
TEST_CASE(AiDropSelection_RepeatCompactsAndRechecksSameIndex) {
    DropSelectionScene scene;scene.Repeat();scene.Field(0x3e4,4);scene.Field(0x570,3);scene.Choice(0,11,12,7);scene.Choice(1,21,22,8);scene.Choice(2,31,32,9);scene.legal={false,false,true};scene.Select();CHECK(scene.trace==std::vector<std::string>({"tech:1","count:5","hate","place:11","remove:0","place:21","remove:0","place:31","retry","scan:128:4"}));CHECK(scene.Field(0x570)==1 && scene.Field(0x3e8)==123 && !scene.Field(0x400) && !scene.Field(0x3f4) && !scene.Field(0x3fc) && std::bit_cast<float>(scene.Field(0x408))==31 && scene.Field(0x4f8)==9);
}
// 직전 표적 목록의80/30 확률 경계와 비어있지 않은 비일치 목록을 구분한다.
TEST_CASE(AiDropSelection_RepeatProbabilityBoundaries) {
    // 작은 원본 시드는 첫Next100이 시드와 같으므로 실제 경계 입력으로 쓴다.
    for (auto seed:{79U,80U,29U,30U}) { DropSelectionScene scene;scene.Repeat();scene.random.SetState(seed);if (seed<40) scene.List({});scene.Select();const bool repeated=seed==79 || seed==29;CHECK((std::find(scene.trace.begin(),scene.trace.end(),"retry")!=scene.trace.end())==repeated); }
    DropSelectionScene nonempty;nonempty.Repeat();nonempty.List({88});nonempty.Select();CHECK(std::find(nonempty.trace.begin(),nonempty.trace.end(),"retry")==nonempty.trace.end());
}
// 재사용 승인 쿼리는 현재 TYPE/ownerBYTE이며 회전값은flags에 넣지 않는다.
TEST_CASE(AiDropSelection_RepeatUsesCurrentTypeOwnerAndCanonicalFlags) {
    DropSelectionScene scene;scene.Repeat();scene.Field(0x570,2);scene.Choice(0,11,12,7);scene.Choice(1,21,22,8);scene.duringLog=[&](auto action) { if (action==AiDropSelectionLog::Hated) { scene.Field(0x3e0,129);scene.Field(0x260,0x102); } };scene.Select();CHECK(scene.queries.size()==2);for (const auto& query:scene.queries) CHECK(query.type==129 && query.owner==2 && !query.argument && !query.flags && query.mode==1); // 반복문은 각 현재 승인 입력을 원본 계약과 비교한다.
}
// 블로커가 있으면 반복보다 우선하고 원본 순환 예산 안에서 찾은 TYPE만 선택한다.
TEST_CASE(AiDropSelection_BlockerPriorityOverridesRepeat) {
    DropSelectionScene scene;scene.Repeat();scene.Field(0x404,1);scene.count=3;scene.products={128,136,142};scene.types[142].flags2=0x4000000;scene.blocker=Sid{6};scene.Select();CHECK(scene.Field(0x3e0)==142 && !scene.Field(0x404) && !scene.Field(0x570) && !scene.Field(0x3e8));CHECK(scene.trace==std::vector<std::string>({"tech:1","count:5","hate","blocker:5","nth:5:1","nth:5:2","scan:142:0"}));
}
// 현재 TYPE가 블로커면 조회를 생략하고 재사용 경로의404를 강제로 지우지 않는다.
TEST_CASE(AiDropSelection_CurrentBlockerRepeatKeepsPriorityFlag) {
    DropSelectionScene scene;scene.Repeat(142);scene.types[142].flags2=0x4000000;scene.Field(0x404,1);scene.Field(0x570,1);scene.Choice(0,11,12,7);scene.Select();CHECK(scene.Field(0x404)==1 && scene.Field(0x570)==1 && scene.Field(0x3e8)==123 && std::find(scene.trace.begin(),scene.trace.end(),"blocker:5")==scene.trace.end());
}
// 블로커 부재는404만 지우고 원본 반복을 허용한다.기술개수+2 회전 예산도 그대로다.
TEST_CASE(AiDropSelection_BlockerAbsentAndRotationBudget) {
    DropSelectionScene absent;absent.Repeat();absent.Field(0x404,1);absent.Select();CHECK(!absent.Field(0x404) && std::find(absent.trace.begin(),absent.trace.end(),"retry")!=absent.trace.end());DropSelectionScene bounded;bounded.Field(0x404,1);bounded.Field(0x3e0,131);bounded.count=1;bounded.products={128};bounded.blocker=Sid{6};bounded.Select();CHECK(bounded.Field(0x3e0)==131 && bounded.Field(0x3e4)==6 && !bounded.Field(0x404) && bounded.trace==std::vector<std::string>({"tech:1","count:5","blocker:5"}));
}
// 최고점수는count==0에서만0으로 만들고 signed음수count는 점수를 보존한다.
TEST_CASE(AiDropSelection_ExactZeroScoreResetAndSignedFirstAttempt) {
    DropSelectionScene scene;scene.Repeat();scene.Field(0x570,0xffffffffU);scene.Select();CHECK(scene.Field(0x570)==0xffffffffU && scene.Field(0x3e8)==123 && scene.Field(0x400)==1);DropSelectionScene empty;empty.Repeat();empty.Select();CHECK(!empty.Field(0x570) && !empty.Field(0x3e8) && empty.Field(0x400)==1);
}
// 운반체는 선택을 지우고 공급TYPE의less/unordered 기한도 재선택 상태로 만든다.
TEST_CASE(AiDropSelection_CarrierAndOrderedSupplyCooldown) {
    DropSelectionScene carrier;carrier.products[1]=133;carrier.types[133].flags2=0x10000;carrier.Select();CHECK(!carrier.Field(0x3e0) && carrier.Field(0x3f0)==1);
    // NaN도미도달이며 equality는도달이다.저장QWORD를float로줄이지않는다.
    for (double due:{100.0,100.00000001,std::numeric_limits<double>::quiet_NaN()}) { DropSelectionScene scene;scene.products[1]=136;scene.types[136].flags2=0x800;scene.Time(0x5b8,due);scene.Select();CHECK((scene.Field(0x3e0)==136)==(due==100.0));CHECK(scene.Field(0x3f0)==(due==100.0 ? 0U : 1U)); }
    DropSelectionScene nan;nan.products[1]=136;nan.types[136].flags2=0x800;nan.now=std::numeric_limits<double>::quiet_NaN();nan.Select();CHECK(!nan.Field(0x3e0) && nan.Field(0x3f0)==1);
}
// 선택된nth 누락은 이미 기록한순번/진행을 보존하고 뒤 정리를 수행하지 않는다.
TEST_CASE(AiDropSelection_MissingNthPreservesPartialWrites) {
    DropSelectionScene scene;scene.Field(0x3e0,128);scene.Field(0x3e4,4);scene.hooks.nth={};CHECK(Throws([&] { scene.Select(); }) && scene.Field(0x3dc)==1 && scene.Field(0x3e0)==128 && scene.Field(0x3e4)==4 && scene.Field(0x3e8)==123 && scene.Field(0x3f0)==9 && scene.Field(0xaf8)==10);
}
