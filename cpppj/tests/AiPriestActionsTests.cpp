// 사제 행동의 원본 관문·현재 상태 재조회·요청 순서를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiPriestActions.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 본체와 슬롯에 명시 입력을 넣고 선택된 하위 호출만 관찰한다.
struct AiPriestScene {
    RawAiBrainMemory memory{0x4f000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; // 같은 본체/목록의 실제 저장소다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},list{memory.Allocate(32)}; // 본체와 명시 건물 목록의 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false}; // 원본 PATCH 물리 슬롯을 사용한다.
    Sid priest{5},self{6},dais{7},home{8},building{9},other{10},altar{11}; // 명시 입력의 역할별 SID다.
    double now{100};std::int32_t mapLimit{256};std::uint32_t daisType{165}; // 현재 전역의 참조 입력이다.
    bool vulnerable{},busy{},ritual{},altarEnemy{},lowFunds{};Sid path{};std::int32_t maximum{1000};double distance{2};std::uint32_t selected{86}; // 선택된 하위의 명시 반환값이다.
    AiPriestActionHooks hooks;std::vector<std::string> trace;std::vector<AiPriestSearchRequest> requests; // 호출 순서와 값으로 전달한 초기화 입력을 보존한다.
    // 기본은 건강한 사제/신전/충분한 지갑/미정 목표/완료된 빈 탐색이다.
    AiPriestScene() {
        // 실제 할당은 SID5부터 순서대로 배정하며 각 입력의 주변 바이트를 초기화한다.
        for (std::uint16_t id=5;id<=11;++id) { CHECK(pool.Allocate(2)==Sid{id});auto raw=pool.AllocatedBytes(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=158;Put(raw,26,1000); }
        Position(priest,{20.2F,30.7F});Position(dais,{40,50});Field(0x29c,priest.value);Field(0x294,home.value);Field(0x264,2);Field(0x260,1);Field(0x2d8,1000);Field(0x300,list);Target({-10000,-10000});
        Field(0x270,std::bit_cast<std::uint32_t>(10.0F));Field(0x274,std::bit_cast<std::uint32_t>(20.0F));Field(0x278,std::bit_cast<std::uint32_t>(50.0F));Field(0x27c,std::bit_cast<std::uint32_t>(70.0F));Field(0x2b0,std::bit_cast<std::uint32_t>(25.0F));Field(0x2b4,std::bit_cast<std::uint32_t>(26.0F));
        hooks.scan=[this](auto brain,auto budget) { CHECK(brain==token);trace.push_back("scan:"+std::to_string(budget));Field(0x5a4,Field(0x5a0)); };
        hooks.vulnerable=[this](Sid actor) { CHECK(actor==priest);trace.push_back("vulnerable");return vulnerable; };
        hooks.busy=[this](Sid actor) { CHECK(actor==priest);trace.push_back("busy");return busy; };
        hooks.path=[this](Sid actor) { CHECK(actor==priest);trace.push_back("path");return path; };
        hooks.maxHitPoints=[this](Sid actor) { CHECK(actor==priest);trace.push_back("max");return maximum; };
        hooks.ritual=[this](Sid actor) { trace.push_back("ritual:"+std::to_string(actor.value));return ritual; };
        hooks.altarEnemy=[this](auto enemy,Sid target) { trace.push_back("altar:"+std::to_string(enemy)+":"+std::to_string(target.value));return altarEnemy; };
        hooks.target=[this](auto brain,Sid actor,Sid target,auto sacrifice) { CHECK(brain==token);trace.push_back("target:"+std::to_string(actor.value)+":"+std::to_string(target.value)+":"+std::to_string(sacrifice)); };
        hooks.distance=[this](RawPathPoint point,Sid target) { CHECK(point.x==20.2F && point.y==30.7F);trace.push_back("distance:"+std::to_string(target.value));return distance; };
        hooks.select=[this](auto brain) { CHECK(brain==token);trace.push_back("select");return selected; };
        hooks.home=[this](auto brain,Sid actor) { CHECK(brain==token && actor==priest);trace.push_back("home"); };
        hooks.collect=[this](auto brain,Sid actor) { CHECK(brain==token && actor==priest);trace.push_back("collect"); };
        hooks.deploy=[this](auto brain,auto type,RawPathPoint point) { CHECK(brain==token && type==selected && point.x==20.2F && point.y==30.7F);CHECK(Time(0x5c0)==now+20);trace.push_back("deploy"); };
        hooks.begin=[this](auto brain,const AiPriestSearchRequest& request) { CHECK(brain==token);requests.push_back(request);trace.push_back("begin");Field(0x574,request.type);Field(0x5a4,2);Field(0x5a0,4); };
        hooks.point=[this](auto brain,Sid actor,RawPathPoint point) { CHECK(brain==token && actor==priest);CHECK(point.x==21 && point.y==31);trace.push_back("point"); };
        hooks.enemyLowFunds=[this](auto brain) { CHECK(brain==token);trace.push_back("funds");return lowFunds; };
        hooks.salvage=[this](Sid target) { trace.push_back("salvage:"+std::to_string(target.value)); };
    }
    // 현재 본체의 지정 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 현재 본체의 지정 DWORD만 입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 QWORD 기한을 정확한 double로 읽는다.
    double Time(std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
    // 명시 기한 입력이며 두 DWORD 이외를 바꾸지 않는다.
    void Time(std::size_t at,double value) { const auto raw=std::bit_cast<std::uint64_t>(value);Field(at,static_cast<std::uint32_t>(raw));Field(at+4,static_cast<std::uint32_t>(raw>>32)); }
    // 목표 좌표 입력을 같은 ad0/ad4에 쓴다.
    void Target(RawPathPoint point) { Field(0xad0,std::bit_cast<std::uint32_t>(point.x));Field(0xad4,std::bit_cast<std::uint32_t>(point.y)); }
    // 물리 슬롯의 현재 두 좌표만 입력한다.
    void Position(Sid actor,RawPathPoint point) { Put(pool.AllocatedBytes(actor),14,std::bit_cast<std::uint32_t>(point.x));Put(pool.AllocatedBytes(actor),18,std::bit_cast<std::uint32_t>(point.y)); }
    // 명시 건물 목록을 실제 backing에 기록한다.
    void Buildings(std::initializer_list<Sid> values) { Field(0x308,static_cast<std::uint32_t>(values.size()));std::size_t at{};for (Sid sid:values) { Put(memory.Bytes(list),at,sid.value);at+=4; } } // 반복문은 명시 목록 순서를 보존한다.
    // 실제 처리기에 현재 입력을 빌려 한 번 호출한다. 이 검사는 실행하지 않는다.
    void Act(Sid actor) { RawAiPriestActions actions(memory,pool,{now,mapLimit,daisType},hooks);CHECK(&actions.Pool()==&pool);actions.Act(token,actor); }
    // 앞선 공통 관문 뒤에 기대하는 정확한 호출 목록을 만든다.
    std::vector<std::string> AfterGates(std::initializer_list<std::string> tail) const { std::vector<std::string> result{"vulnerable","busy","path"};result.insert(result.end(),tail.begin(),tail.end());return result; }
};
}

// 진행 중 탐색은 사제0/생각 기한/TYPE0보다 먼저 처리한다.
TEST_CASE(AiPriestActions_SearchPrefixPrecedesActorAndDeadlines) {
    AiPriestScene scene;scene.Field(0x5a4,0xffffffffU);scene.Field(0x5a0,0);scene.Time(0x5a8,999);scene.Act(Sid{});CHECK(scene.trace==std::vector<std::string>{"scan:0"});CHECK(!scene.Field(0x574));scene.trace.clear();scene.Act(Sid{});CHECK(scene.trace.empty());
}
// 생각 관문의 NaN·상처·예약·이동 중 최대HP 조회·건설 기한을 구분한다.
TEST_CASE(AiPriestActions_OrderedReadinessAndEarlyGates) {
    AiPriestScene scene;scene.Time(0x5a8,std::numeric_limits<double>::quiet_NaN());scene.Act(scene.priest);CHECK(scene.trace.empty());scene.Time(0x5a8,100);scene.vulnerable=true;scene.Act(scene.priest);CHECK(scene.trace==std::vector<std::string>{"vulnerable"});scene.trace.clear();scene.vulnerable=false;scene.busy=true;scene.Act(scene.priest);CHECK(scene.trace==std::vector<std::string>({"vulnerable","busy"}));scene.trace.clear();scene.busy=false;scene.path=scene.other;scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"max"}));scene.trace.clear();scene.path={};scene.Time(0x5c0,std::numeric_limits<double>::quiet_NaN());scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({}));
}
// 의식 구간은 명시 사제 인자 대신 본체29c의 현재 사제를 묻는다.
TEST_CASE(AiPriestActions_RitualUsesStoredSelfAndStops) {
    AiPriestScene scene;scene.Field(0x248,0x400);scene.Field(0x2a4,scene.dais.value);scene.Field(0x29c,scene.self.value);scene.ritual=true;scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"ritual:6"}));
}
// 제단 하위 효과 뒤 실제 희생 요청의 두 SID를 다시 읽는다.
TEST_CASE(AiPriestActions_AltarRequeriesSelfAndDaisBeforeSacrifice) {
    AiPriestScene scene;scene.Field(0x248,0x400);scene.Field(0x2a4,scene.dais.value);scene.hooks.altarEnemy=[&](auto enemy,Sid dais) { CHECK(enemy==2 && dais==scene.dais);scene.trace.push_back("altar");scene.Field(0x29c,scene.self.value);scene.Field(0x2a4,scene.other.value);return true; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"ritual:5","altar","target:6:10:1"}));
}
// 거리2와 NaN은 접근하지 않으며 2초과만 현재 다이스에 이동한다.
TEST_CASE(AiPriestActions_DaisStrictDistanceAndCurrentTarget) {
    AiPriestScene scene;scene.Field(0x248,0x400);scene.Field(0x2a4,scene.dais.value);scene.hooks.distance=[&](RawPathPoint,Sid target) { CHECK(target==scene.dais);scene.trace.push_back("distance");scene.Field(0x2a4,scene.other.value);return 2.1; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"ritual:5","altar:2:7","distance","target:5:10:0"}));
    // 두 비접근 경계는 이후 TYPE 선택과 새 탐색까지 진행해야 한다.
    for (double distance:{2.0,std::numeric_limits<double>::quiet_NaN()}) { AiPriestScene boundary;boundary.Field(0x248,0x400);boundary.Field(0x2a4,boundary.dais.value);boundary.distance=distance;boundary.Act(boundary.priest);CHECK(boundary.trace==boundary.AfterGates({"ritual:5","altar:2:7","distance:7","select","begin"})); }
}
// TYPE 선택이 저자금보다 먼저이며 HP 선저장·현재800h 능력으로 수집을 고른다.
TEST_CASE(AiPriestActions_LowFundsSelectionAndSavedHpFallback) {
    AiPriestScene scene;scene.Field(0x2d8,399);scene.hooks.maxHitPoints=[&](Sid actor) { CHECK(actor==scene.priest);scene.trace.push_back("max");Put(scene.pool.AllocatedBytes(actor),26,0);scene.Field(0x248,0x800);return 1000; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","max","collect"}));CHECK(scene.Time(0x5c8)==0);
    AiPriestScene flying;flying.selected=0;flying.Field(0x2cc,1);flying.Act(flying.priest);CHECK(flying.trace==flying.AfterGates({"select","home"}));
}
// TYPE0은 유효 목표만 지우고 미정 목표에서는 귀환/수집으로 넘어간다.
TEST_CASE(AiPriestActions_NoTypeValidTargetOnlyClears) {
    AiPriestScene scene;scene.selected=0;scene.Target({50,60});scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select"}));CHECK(scene.Field(0xad0)==0xc61c4000U && scene.Field(0xad4)==0xc61c4000U);scene.trace.clear();scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","max","home"}));
}
// 좌표 일치 배치는 계획 기한보다 먼저 20초를 쓰고 raw 좌표를 넘긴 뒤 목표를 지운다.
TEST_CASE(AiPriestActions_DeployUsesRawPositionAndPriorTimer) {
    AiPriestScene scene;scene.Target({21,31});scene.Time(0x5c8,999);scene.hooks.deploy=[&](auto brain,auto type,RawPathPoint point) { CHECK(brain==scene.token && type==86 && point.x==20.2F && point.y==30.7F);CHECK(scene.Time(0x5c0)==120);scene.trace.push_back("deploy");scene.Target({90,91}); };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","deploy"}));CHECK(scene.Field(0xad0)==0xc61c4000U && scene.Field(0xad4)==0xc61c4000U && scene.Time(0x5c8)==999);
}
// 실제 배치 하위 누락은 앞서 쓴20초 기한을 보존하며 목표를 지우지 않는다.
TEST_CASE(AiPriestActions_MissingDeployPreservesPrefix) {
    AiPriestScene scene;scene.Target({21,31});scene.hooks.deploy={};CHECK(Throws([&] { scene.Act(scene.priest); }));CHECK(scene.Time(0x5c0)==120 && std::bit_cast<float>(scene.Field(0xad0))==21);
}
// 계획 기한의 less/unordered는 새 탐색·후보 소비를 수행하지 않는다.
TEST_CASE(AiPriestActions_SearchDeadlineIsOrdered) {
    // 명시 두 기한만 바꾸고 같은 관문까지 도달한다.
    for (double due:{101.0,std::numeric_limits<double>::quiet_NaN()}) { AiPriestScene scene;scene.Time(0x5c8,due);scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select"}) && scene.requests.empty()); }
}
// 완료된 같은TYPE 탐색은 실제 최적좌표를 보정하고 이동하며 3초를 쓰지 않는다.
TEST_CASE(AiPriestActions_CompletedSearchConsumesSnapsAndMoves) {
    AiPriestScene scene;scene.Field(0x574,86);scene.Field(0x57c,std::bit_cast<std::uint32_t>(20.2F));scene.Field(0x580,std::bit_cast<std::uint32_t>(30.7F));scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","point"}));CHECK(scene.Field(0x574)==0 && scene.Time(0x5c8)==0 && std::bit_cast<float>(scene.Field(0xad0))==21);
    AiPriestScene empty;empty.Field(0x574,86);empty.Field(0x57c,0xc61c4000U);empty.Field(0x580,0xc61c4000U);empty.Act(empty.priest);CHECK(empty.trace==empty.AfterGates({"select"}) && empty.Time(0x5c8)==103);CHECK(std::bit_cast<float>(empty.Field(0xad0))==-9999 && !empty.Field(0x574));
}
// 새 탐색은 그 시점의8인자를 전달하고 하위 효과 뒤 현재시각+3을 쓴다.
TEST_CASE(AiPriestActions_NewSearchSnapshotsArgumentsAndCurrentClock) {
    AiPriestScene scene;scene.hooks.begin=[&](auto brain,const AiPriestSearchRequest& request) { CHECK(brain==scene.token);scene.trace.push_back("begin");scene.requests.push_back(request);scene.Field(0x270,0);scene.Field(0x260,3);scene.now=125; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","begin"}) && scene.requests.size()==1 && scene.Time(0x5c8)==128);if (scene.requests.empty()) return;const auto& request=scene.requests.front();CHECK(request.type==86 && request.owner==1 && request.bounds.left==10 && request.bounds.top==20 && request.bounds.right==50 && request.bounds.bottom==70 && request.avoid.x==25 && request.avoid.y==26);
}
// 선택 하위가 탐색을 다시 진행 상태로 바꾸면 완료 후보를 소비하지 않는다.
TEST_CASE(AiPriestActions_CurrentSearchProgressAfterSelect) {
    AiPriestScene scene;scene.Field(0x574,86);scene.hooks.select=[&](auto brain) { CHECK(brain==scene.token);scene.trace.push_back("select");scene.Field(0x5a4,2);scene.Field(0x5a0,4);return 86U; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select"}) && scene.Field(0x574)==86 && scene.Time(0x5c8)==103);
}
// 회수 대상은 signed HP 최대 첫 객체이며 free/dead 추가 필터 없이 원본 요청 순서를 따른다.
TEST_CASE(AiPriestActions_DaisReclaimHighestSignedHpAndRequestOrder) {
    AiPriestScene scene;scene.selected=165;scene.Field(0x574,165);scene.Field(0x57c,0xc61c4000U);scene.Field(0x580,0xc61c4000U);scene.Buildings({scene.building,scene.other,scene.home});Put(scene.pool.AllocatedBytes(scene.building),26,2000);Put(scene.pool.AllocatedBytes(scene.other),26,2000);Put(scene.pool.AllocatedBytes(scene.home),26,0xffffffffU);scene.pool.AllocatedBytes(scene.building)[11]=3;
    scene.hooks.salvage=[&](Sid target) { CHECK(target==scene.building);scene.trace.push_back("salvage:9");CHECK(scene.Time(0x5e8)==0);scene.now=120;scene.Field(0x260,3);scene.Field(0x270,std::bit_cast<std::uint32_t>(11.0F)); };
    scene.hooks.scan=[&](auto brain,auto budget) { CHECK(brain==scene.token && budget==1 && scene.Time(0x5e8)==130);scene.trace.push_back("scan:1");scene.now=140; };
    scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","target:5:9:0","salvage:9","begin","scan:1"}));CHECK(scene.Time(0x5e8)==130 && scene.Time(0x5c8)==143 && scene.requests.size()==1);if (!scene.requests.empty()) CHECK(scene.requests.front().owner==3 && scene.requests.front().bounds.left==11);
}
// 건물 부재 시 적 저자금 확인 뒤 현재 신전을 읽으며 부재/거짓은 기한만 쓴다.
TEST_CASE(AiPriestActions_ReclaimFallbackUsesCurrentTempleAndFunds) {
    AiPriestScene scene;scene.selected=165;scene.Field(0x574,165);scene.Field(0x57c,0xc61c4000U);scene.Field(0x580,0xc61c4000U);scene.hooks.enemyLowFunds=[&](auto brain) { CHECK(brain==scene.token);scene.trace.push_back("funds");scene.Field(0x294,scene.other.value);return true; };scene.Act(scene.priest);CHECK(scene.trace==scene.AfterGates({"select","funds","target:5:10:0","salvage:10","begin","scan:1"}));
    // 자금 조건 거짓과 신전 부재를 각각 같은 완료 탐색으로 검사한다.
    for (bool funds:{false,true}) { AiPriestScene empty;empty.selected=165;empty.Field(0x574,165);empty.Field(0x57c,0xc61c4000U);empty.Field(0x580,0xc61c4000U);empty.Field(0x294,0);empty.lowFunds=funds;empty.Act(empty.priest);CHECK(empty.trace==empty.AfterGates({"select","funds"}) && empty.Time(0x5c8)==103 && empty.Time(0x5e8)==0); }
}
// 회수 기한은 경과 판정이 아닌0 비교다. 과거 기한과 NaN도 재회수를 막는다.
TEST_CASE(AiPriestActions_ReclaimTimerNonzeroAndNan) {
    // 이미 경과한 값/NaN은 요청을 막고 +0/-0만 처음 회수를 허용한다.
    for (double due:{1.0,std::numeric_limits<double>::quiet_NaN(),0.0,-0.0}) { AiPriestScene scene;scene.selected=165;scene.Field(0x574,165);scene.Field(0x57c,0xc61c4000U);scene.Field(0x580,0xc61c4000U);scene.Buildings({scene.building});scene.Time(0x5e8,due);scene.Act(scene.priest);if (due==0.0) CHECK(scene.trace==scene.AfterGates({"select","target:5:9:0","salvage:9","begin","scan:1"}));else CHECK(scene.trace==scene.AfterGates({"select"})); }
}
// 회수 하위 누락 시 대상이동 요청 뒤 기한/재탐색/3초 쓰기는 발생하지 않는다.
TEST_CASE(AiPriestActions_MissingSalvagePreservesTargetRequest) {
    AiPriestScene scene;scene.selected=165;scene.Field(0x574,165);scene.Field(0x57c,0xc61c4000U);scene.Field(0x580,0xc61c4000U);scene.Buildings({scene.building});scene.hooks.salvage={};CHECK(Throws([&] { scene.Act(scene.priest); }));CHECK(scene.trace==scene.AfterGates({"select","target:5:9:0"}) && scene.Time(0x5e8)==0 && scene.Time(0x5c8)==0 && scene.requests.empty());
}
// 원본0.99999 보정은 ceil/반올림과 다르며 mapLimit은 float로 저장한 값을 사용한다.
TEST_CASE(AiPriestActions_PointSnapAndValidityNativeBoundaries) {
    auto point=SnapAiPriestPoint({1.000001F,-2.9F});CHECK(point.x==1 && point.y==-1);point=SnapAiPriestPoint({-10000,std::numeric_limits<float>::quiet_NaN()});CHECK(point.x==-9999 && point.y==0);CHECK(AiPriestPointValid({0.1F,255.9F},256));CHECK(!AiPriestPointValid({0,1},256) && !AiPriestPointValid({1,256},256) && !AiPriestPointValid({std::numeric_limits<float>::quiet_NaN(),1},256));CHECK(!AiPriestPointValid({16777216.0F,1},16777217));
}
// 제단의 ownerBYTE가 아닌 argumentWORD를 전체 enemyDWORD와 비교하고 찾은SID0도 읽는다.
TEST_CASE(AiPriestActions_AltarArgumentWordAndPhysicalZero) {
    AiPriestScene scene;Put(scene.pool.AllocatedBytes(scene.altar),12,2,2);scene.pool.AllocatedBytes(scene.altar)[34]=3;const auto find=[&](float x,float y,auto type) { CHECK(x==40 && y==50 && type==164);return scene.altar; };CHECK(AiAltarContainsEnemy(scene.pool,scene.dais,2,164,find));CHECK(!AiAltarContainsEnemy(scene.pool,scene.dais,0x10002,164,find));const auto argument=Get(scene.pool.Slot(Sid{}),12,2);CHECK(AiAltarContainsEnemy(scene.pool,scene.dais,argument,164,[](float,float,auto) { return Sid{}; }));
}
// 의식 구간10..14는 물리 frame의 첫 signed문자에서 구하며 상태BYTE와 무관하다.
TEST_CASE(AiPriestActions_RitualUsesActualFrameCode) {
    AiPriestScene scene;std::vector<RiftTypeFrames> frames(159,RiftTypeFrames({}));frames[158]=RiftTypeFrames({{'J',0,0,0},{'K',0,0,0},{'O',0,0,0},{'P',0,0,0},{0xff,0,0,0}});scene.pool.AllocatedBytes(scene.priest)[40]=0xe0;
    // 각 물리 프레임을 그대로 사용하며 state 상위비트로 의식 여부를 만들지 않는다.
    for (std::uint32_t frame=0;frame<5;++frame) { Put(scene.pool.AllocatedBytes(scene.priest),36,frame);CHECK(AiPriestInRitual(scene.pool,frames,scene.priest)==(frame==1 || frame==2)); }
}
