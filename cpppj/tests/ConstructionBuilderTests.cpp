// 세 원본 PE의 생성·이동/공급원 소비 관찰과 실제 factory/소유자/Regular 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionBuilder.h"
#include "o/RawEnergySupply.h"

namespace {
using namespace netstorm::test::construction;
// 실행기 입력 열의 순서다. 기대 계산에 사용하지 않고 경계 응답과 장면 자료만 공급한다.
enum Input : std::size_t { Player,Anchor,Owner,Mask,Genus,X,Y,FootX,FootY,TimeLow,TimeHigh,NowLow,NowHigh,
    Path,Speed,Sources,Allocation,Frame,Mutate,Bridge,Count };
// 각 PE의 독립 원본 행 수와 고정 입력 타입 번호다. 실제 게임 타입 역할을 추정하지 않는다.
constexpr std::size_t kRows=370;
constexpr std::uint32_t kBuilder=134,kCarrier=105,kChangedCarrier=106;
// 저장 관찰만 읽는다. CTest에서 원본 게임·PE·Python은 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONBUILDER_FIXTURE).rows;return rows; }
// 시간 입력 비트를 손실 없이 double로 조합한다.
double Time(std::uint32_t low,std::uint32_t high) { return std::bit_cast<double>((static_cast<std::uint64_t>(high)<<32)|low); }
// 실행기와 같은 네 항목 목록 입력이다. 소비 결과는 C++ 모듈에서만 얻는다.
std::vector<std::uint32_t> SourceInput(std::uint32_t profile) {
    return std::array<std::vector<std::uint32_t>,4>{{{0,0,0,0},{70,0,0,0},{0,70,71,70},{71,0,70,0}}}.at(profile);
}
// 실제 풀과 입력 메타 자료, 동작 밖 명시 효과의 관찰 장면이다.
struct BuilderScene {
    SidPool pool; // 모든 모듈과 경계가 공유하는 월드다.
    std::vector<RiftTypeRecord> types; // 현재 타입의 발자국/genus다.
    std::vector<ConstructionBuilderType> metadata; // 현재 타입의 속도/프레임 입력이다.
    ConstructionBuilderState state; // 전역/공급원 공유 상태다.
    ConstructionBuilderPath path{0,{10.0,12.5}}; // 첫 이동의 입력 경로와 두 단계 시각이다.
    std::vector<std::uint32_t> input; // fixture의 21개 입력 DWORD다.
    std::vector<std::string> events; // 기대 순서를 만들지 않고 실제 훅 호출을 저장한다.
    std::uint16_t born{}; // 합성 factory가 반환한 새 객체 개수다.
    bool patch; // 판본별 raw 쓰기 위치와 원본 경로 인자 수를 고른다.
    // 원본 실행기의 합성 SID를 미리 확보한다. 행마다 다시 할당할 필요가 없다.
    explicit BuilderScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        metadata(types.size()),patch(edition==OriginalEdition::Patch1078) {
        // 생성/직접 쓰기의 호스트 계약을 만족하도록 입력 슬롯을 실제로 확보한다.
        for (std::uint16_t sid=5;sid<=127;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 소유자 BYTE의 판본별 위치다.
    std::size_t OwnerOffset() const { return patch ? 34 : 32; }
    // 실제 호출 인자들을 실행기와 같은 비트 문자열로 기록한다.
    void Event(char letter,std::initializer_list<std::uint32_t> values) {
        std::string text(1,letter);
        // 인자 순서와 DWORD 폭을 그대로 보존한다.
        for (auto value:values) text+=':'+std::to_string(value);
        events.push_back(std::move(text));
    }
    // raw 입력만 공급한다. 이동 결과나 기대 중심은 계산하지 않는다.
    void Node(Sid sid,std::uint32_t type,float x,float y) {
        auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        raw[10]=static_cast<std::uint8_t>(type);raw[OwnerOffset()]=static_cast<std::uint8_t>(input[Owner]);
        Put(raw,14,Bits(x));Put(raw,18,Bits(y));
    }
    // 이전 행의 상태를 제거하고 실제 PE에 공급했던 입력 자료로 채운다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);CHECK(input.size()==21);events.clear();born=0;path={0,{10.0,12.5}};
        state={};state.builderType=kBuilder;state.carrierType=kCarrier;state.bridgeType=input[Bridge];
        state.approachMask=input[Mask];state.timerPayload=std::bit_cast<float>(0x25bU);state.now=Time(input[NowLow],input[NowHigh]);
        state.sources={SourceInput(input[Sources]),input[Count]};
        // helper가 조회할 모든 타입에 발자국/속도/프레임 입력을 준비한다.
        for (auto type:{100U,83U,84U,85U,kBuilder,kCarrier,kChangedCarrier}) {
            auto& record=types[type];record={};record.flags2=input[Genus];
            record.footX=type==100 ? static_cast<int>(input[FootX]) : type>=83 && type<=85 ? 3 : 1;
            record.footY=type==100 ? static_cast<int>(input[FootY]) : type>=83 && type<=85 ? 2 : 1;
            auto& meta=metadata[type];meta.speed=Float(input[Speed]);meta.sourceFrameIndex=type==84 ? 1 : type==85 ? 2 : 0;
            // 변경된 운반 타입은 서로 다른 표로 현재 타입 재조회를 확인한다.
            for (std::uint32_t index=0;index<meta.firstFrames.size();++index)
                meta.firstFrames[index]=std::bit_cast<std::int32_t>(input[Frame]+index+(type==kChangedCarrier ? 256U : 0U));
        }
        Node(Sid{50},100,Float(input[X]),Float(input[Y]));Node(Sid{60},83,10.25F,11.5F);
        Node(Sid{70},84,30.25F,31.5F);Node(Sid{71},85,40.5F,41.75F);
    }
    // 메타 계산·소비를 대체하지 않고 경로/가상 공간/자원 정책/할당 응답만 공급한다.
    ConstructionBuilderHooks Hooks() {
        ConstructionBuilderHooks hooks;
        // 기준점 선택은 이번 root 비교의 명시 경계다. 실제 조회 연결은 별도 통합 검사에서 실행한다.
        hooks.anchor=[this](std::uint32_t player,std::uint32_t type,float x,float y) {
            Event('A',{player,type,Bits(x),Bits(y)});return input[Anchor];
        };
        // 원본 factory 경계와 같은 raw 패턴으로 주변 바이트 보존을 비교한다.
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            const Sid sid{static_cast<std::uint16_t>(100+born++)};auto raw=pool.AllocatedBytes(sid);
            // 판본별 stride 전체에 관찰 가능한 바이트를 공급한다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*37+11);
            Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(input[Mutate]==6 && born>1 ? kChangedCarrier : type);raw[11]=4;
            Event('C',{type,flags,sid.value});return sid;
        };
        // 현재 소유자를 쓰고 첫 대상 좌표가 경계 뒤 바뀌는 입력도 공급한다.
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) {
            Event('O',{sid.value,owner});pool.AllocatedBytes(sid)[OwnerOffset()]=static_cast<std::uint8_t>(owner);
            if (input[Mutate]==3 && sid.value==100) { auto raw=pool.AllocatedBytes(Sid{50});Put(raw,14,Bits(22.5F));Put(raw,18,Bits(24.5F)); }
        };
        // 접근점 정책이 받은 실제 기준점 발자국과 목표/플래그를 관찰한다.
        hooks.approach=[this](const std::array<float,4>& rect,float x,float y,std::uint32_t flags) {
            Event('P',{Bits(x),Bits(y),flags,Bits(rect[0]),Bits(rect[1]),Bits(rect[2]),Bits(rect[3])});return ConstructionNoticePoint{10.25F,11.5F};
        };
        // 가상 공간 Pop은 입력 인자만 저장한다.
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event('B',{sid.value,Bits(x),Bits(y),flags}); };
        // 최초 경로만 돌려주며 운반 경로 실패 뒤에도 원본 root가 계속하는지 비교한다.
        hooks.move=[this](Sid sid,float x,float y) -> ConstructionBuilderPath* {
            if (patch) Event('M',{sid.value,Bits(x),Bits(y),0,0,0,0,0});else Event('M',{sid.value,Bits(x),Bits(y),0,0,0,0});
            if (input[Mutate]==2 && sid.value!=100) path.currentStep=1;
            return sid.value==100 && input[Path] ? &path : nullptr;
        };
        // 속도 적용 뒤 첫 경로의 단계가 바뀌는 원본 입력 경계다.
        hooks.speed=[this](ConstructionBuilderPath& route,float speed,std::uint32_t flags) {
            CHECK(&route==&path);Event('V',{Bits(speed),flags});if (input[Mutate]==1) path.currentStep=1;
        };
        // 자원 선택 정책은 공유 목록을 공급하며 현재 대상/기준점 소유자도 바꿀 수 있다.
        hooks.chooseSources=[this](float x,float y,std::uint32_t type,std::uint32_t owner) {
            Event('Q',{Bits(x),Bits(y),type,owner});state.sources.entries=SourceInput(input[Sources]);
            if (input[Mutate]==4) { auto raw=pool.AllocatedBytes(Sid{50});Put(raw,14,Bits(25.5F));Put(raw,18,Bits(26.5F));pool.AllocatedBytes(Sid{60})[OwnerOffset()]=8; }
        };
        // 정렬 응답으로 순서/현재 운반 타입 또는 공급원 현재 좌표의 변경을 공급한다.
        hooks.sortSources=[this](float x,float y) {
            Event('S',{Bits(x),Bits(y)});
            if (input[Mutate]==7) { state.sources.entries={71,70,0,0};state.carrierType=kChangedCarrier; }
            if (input[Mutate]==8) { auto raw=pool.AllocatedBytes(Sid{70});Put(raw,14,Bits(32.75F));Put(raw,18,Bits(33.25F)); }
        };
        // 현재 타입이 현재 다리 전역과 같을 때만 실제 단어 helper가 호출하는 경계다.
        hooks.notifySurface=[this](Sid sid) { Event('N',{sid.value}); };
        // 할당 실패와 할당 중 이벤트 변경을 입력으로 공급한다.
        hooks.reserveTimer=[this] { Event('L',{40});if (input[Mutate]==5) state.timerPayload=std::bit_cast<float>(0x12345678U);return input[Allocation]!=0; };
        // 원본 생성자의 실제 인자 순서(event,parent,payload)로 호출 비트를 기록한다.
        hooks.addTimer=[this](Sid sid,std::uint32_t event,float payload) { Event('T',{event,sid.value,Bits(payload)}); };
        return hooks;
    }
    // 관찰 문자열을 구분자로 묶는다. 빈 관찰은 fixture의 '-'로 표시한다.
    std::string Events() const {
        std::string out;
        // 실제 호출 순서대로 직렬화한다.
        for (const auto& event:events) { if (!out.empty()) out+=';';out+=event; }
        return out.empty() ? "-" : out;
    }
    // 실제 생성 raw 전체를 판본별 stride로 직렬화한다.
    std::string Raws() const {
        std::string out;
        // 새 객체 번호 순서대로 읽는다.
        for (std::uint16_t i=0;i<born;++i) { if (!out.empty()) out+=';';out+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(100+i)})); }
        return out.empty() ? "-" : out;
    }
    // 최종 공유 목록은 개수 밖 꼬리도 그대로 비교한다.
    std::string SourcesText() const {
        std::string out;
        // 저장소 전체 네 항목을 읽는다.
        for (auto value:state.sources.entries) { if (!out.empty()) out+=',';out+=std::to_string(value); }
        return out;
    }
};
// 같은 입력의 반환 비트·사건 순서/인자·생성 raw·소비 목록을 독립 PE 관찰과 비교한다.
void Replay(std::string_view edition) {
    BuilderScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,scene.Hooks());std::size_t count=0;
    CHECK(Fixture().size()==3*kRows);
    // 저장 행마다 장면만 공급하며 기대 반환/기하/프레임은 재계산하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==7);scene.Prepare(Inputs(row[2]));std::string result;
        if (row[1]=="Move") {
            const auto value=builder.Move(scene.input[Player],Sid{50},scene.input[TimeLow],scene.input[TimeHigh]);
            std::array<std::uint8_t,8> bytes{};const auto bits=std::bit_cast<std::uint64_t>(value);
            Put(bytes,0,static_cast<std::uint32_t>(bits));Put(bytes,4,static_cast<std::uint32_t>(bits>>32));result=Hex(bytes);
        } else { const auto sid=builder.TakeSource();result=std::string(sid ? "1:" : "0:")+std::to_string(sid); }
        const bool same=scene.Events()==row[3] && result==row[4] && scene.Raws()==row[5] && scene.SourcesText()==row[6] && scene.state.sources.count==scene.input[Count];
        CHECK(same);if (!same) { std::printf("건설 생성/이동 %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
// 기본 장면의 입력만 제공한다. 유효 대상/기준점과 공급원 두 종류다.
std::vector<std::uint32_t> DefaultInput() {
    return {3,60,3,4,4,Bits(20.5F),Bits(21.25F),3,2,0,0,0,0,1,Bits(1.5F),2,1,0x12345678,0,82,4};
}
}

// 패치 전체 원본 명령의 관찰을 재생한다.
TEST_CASE(ConstructionBuilder_ReplaysOriginals) { Replay("originals"); }
// CD 전체 원본 명령의 관찰을 재생한다.
TEST_CASE(ConstructionBuilder_ReplaysCd) { Replay("originalCD"); }
// 별도 10.37 PE의 독립 관찰을 재생한다.
TEST_CASE(ConstructionBuilder_ReplaysExtra1037) { Replay("original1037"); }

// 공급원은 signed 개수 조기 반환을 보존하고 실제 접근할 손상 목록/타입/경로는 거부한다.
TEST_CASE(ConstructionBuilder_GuardsCurrentListsMetadataAndPaths) {
    BuilderScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());auto hooks=scene.Hooks();
    RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,hooks);
    scene.state.sources={{},0xffffffff};CHECK(builder.TakeSource()==0);scene.state.sources.count=1;CHECK(Throws([&] { builder.TakeSource(); }));
    scene.Prepare(DefaultInput());scene.metadata[84].sourceFrameIndex=16;CHECK(Throws([&] { builder.Move(3,Sid{50},0,0); }));
    scene.Prepare(DefaultInput());scene.path.currentStep=2;CHECK(Throws([&] { builder.Move(3,Sid{50},0,0); }));
    scene.Prepare(DefaultInput());scene.input[Anchor]=0x1003c;CHECK(Throws([&] { builder.Move(3,Sid{50},0,0); }));
    hooks.addTimer={};CHECK(Throws([&] { RawConstructionBuilder invalid(scene.pool,scene.types,scene.metadata,scene.state,hooks); }));
}

// 기준점 실패는 생성 전 반환하며 첫 경로 실패는 앞선 생성/소유자/Pop 효과를 유지한다.
TEST_CASE(ConstructionBuilder_FailureKeepsEarlierEffectsAndTimerFailureStillMoves) {
    BuilderScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,scene.Hooks());
    scene.input[Anchor]=0;CHECK(builder.Move(3,Sid{50},0,0)==5 && scene.born==0 && scene.events.size()==1);
    scene.Prepare(DefaultInput());scene.input[Path]=0;CHECK(builder.Move(259,Sid{50},0,0)==5 && scene.born==1 && scene.events.size()==6);
    scene.Prepare(DefaultInput());scene.input[Allocation]=0;CHECK(builder.Move(3,Sid{50},0,0)==10 && scene.born==4);
    CHECK(std::count_if(scene.events.begin(),scene.events.end(),[](const std::string& event) { return event.starts_with("M:"); })==4);
    CHECK(std::none_of(scene.events.begin(),scene.events.end(),[](const std::string& event) { return event.starts_with("T:"); }));
}

// 로컬/서버 통지의 기존 이동 훅을 같은 전체 복원으로 연결하고 시각 비트를 그대로 전달한다.
TEST_CASE(ConstructionBuilder_LocalAndNoticeAdaptersRunCompleteMove) {
    BuilderScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,scene.Hooks());
    auto local=MakeConstructionBuilderLocalHooks(scene.pool,builder,{});local.moveBuilder(3,Sid{50},0,0);CHECK(scene.born==4);
    scene.Prepare(DefaultInput());auto notice=MakeConstructionBuilderNoticeHooks(scene.pool,builder,SilentNoticeHooks());
    notice.moveBuilder(3,Sid{50},0,0x402e0000);CHECK(scene.born==4);
    CHECK(std::find(scene.events.begin(),scene.events.end(),"V:"+std::to_string(Bits(2.25F))+":0")!=scene.events.end());
}

// 실제 작업장 조회/factory/일반 소유자와 Regular Form/Kernel까지 동일한 월드에서 실행한다.
TEST_CASE(ConstructionBuilder_ActualAnchorFactoryOwnerAndRegularHost) {
    // 두 판본 모두 client 생성 영역과 실제 부착 이벤트를 확인한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        BuilderScene scene(edition);scene.Prepare(DefaultInput());scene.state.timerPayload=0.05F;scene.types[100].flags2=4;scene.types[83].flags1=0x800;
        // 실제 생성자 표를 입력하고 가상 소유자에서 필요한 HP/소유 장부 조건을 공급한다.
        for (auto type:{kBuilder,kCarrier}) { scene.types[type].constructorAddress=TypeConstructorAddress(edition,type);scene.types[type].flags1=0;scene.types[type].flags2=0; }
        scene.pool.AllocatedBytes(Sid{60})[scene.patch ? 30 : 28]=7;
        std::vector<std::uint16_t> surfaces(65536,60);SquidPostPopState books;books.ownerFactories[3]={{60},1};
        SquidPostPopList additional;ContainedFinderState contained;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
        RawPlayerPlacementAnchor anchor(scene.pool,scene.types,surfaces,books,additional,contained,anchorState);
        SquidFactory factory(scene.pool,scene.types);SquidOwnerMode ownerMode;SquidOwner owner(scene.pool,scene.types,books,ownerMode);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(scene.pool,hash,spots);
        RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel;SquidProcessState processState;processState.now=2.5;
        SquidProcessHost process(scene.pool,scene.types,kernel,destroy,processState);std::vector<Sid> created;
        auto hooks=MakeConstructionBuilderHooks(scene.pool,anchor,factory,owner,scene.Hooks(),&process);
        // 실제 factory 반환을 관찰하고 최초 객체의 실제 경로 응답만 공급한다.
        const auto create=hooks.create;hooks.create=[&](std::uint32_t type,std::uint32_t flags) { const auto sid=create(type,flags);created.push_back(sid);return sid; };
        // 공간 Pop은 명시 경계이므로 호출 기록 뒤 정상 Pop의 void 해제 결과를 공급한다.
        const auto pop=hooks.pop;hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            pop(sid,x,y,flags);scene.pool.AllocatedBytes(sid)[11]&=static_cast<std::uint8_t>(~4U);
        };
        hooks.move=[&](Sid sid,float,float) -> ConstructionBuilderPath* { return !created.empty() && sid==created.front() ? &scene.path : nullptr; };
        RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,hooks);CHECK(builder.Move(3,Sid{50},0,0)==10);CHECK(created.size()==4);
        // 생성 객체의 현재 소유자와 Regular 부착/현재 시각을 검사한다.
        for (std::size_t i=0;i<created.size();++i) {
            CHECK(created[i].value<scene.pool.Layout().serverFirst && scene.pool.Slot(created[i])[scene.OwnerOffset()]==3);
            if (!i) continue;auto* timer=process.FindEvent(created[i],0);CHECK(timer!=nullptr);
            if (timer) CHECK(timer->Parent()==created[i] && timer->Payload()==0.05F && timer->Time()==2.5 && timer->Form().value!=0);
        }
        kernel.RunFrame();
        // 부모 event 몸체는 payload 반환 경계다. 실제 Regular의 count/다음 시각 갱신을 검사한다.
        for (std::size_t i=1;i<created.size();++i) {
            auto* timer=process.FindEvent(created[i],0);CHECK(timer && timer->Count()==1 && timer->Payload()==0.05F);
            if (timer) CHECK_NEAR(timer->Time(),2.5+static_cast<double>(0.05F),1e-12);
        }
        // host/몸체가 살아 있는 동안 실제 Kernel 소유권을 먼저 해제한다.
        kernel.Clear();
    }
}

// 다른 풀을 연결하면 첫 효과 전에 거부한다.
TEST_CASE(ConstructionBuilder_AdaptersRejectForeignPools) {
    BuilderScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,scene.Hooks());
    SidPool other(scene.pool.Edition(),kCapacity,false);
    CHECK(Throws([&] { MakeConstructionBuilderLocalHooks(other,builder,{}); }));
    CHECK(Throws([&] { MakeConstructionBuilderNoticeHooks(other,builder,SilentNoticeHooks()); }));
}

// 실제 원본 Builder→Regular 생성자의 event0/payload/current time/count를 같은 C++ Host 필드와 대조한다.
TEST_CASE(ConstructionBuilder_ActualRegularConstructorFields) {
    const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONTIMER_FIXTURE).rows;CHECK(rows.size()==192);std::size_t count=0;
    // 고유 입력마다 실제 ProcessForm/Kernel을 새로 구성하여 이전 수명/등록이 섞이지 않게 한다.
    for (const auto& row:rows) {
        CHECK(row.size()==7);BuilderScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
        scene.Prepare(Inputs(row[1]));scene.state.timerPayload=std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[2])));
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(scene.pool,hash,spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);
        Kernel kernel;SquidProcessState processState;processState.now=scene.state.now;SquidProcessHost host(scene.pool,scene.types,kernel,destroy,processState);
        auto hooks=scene.Hooks();
        // native factory 경계의 패턴은 종속 머리까지 임의 값이다. 실제 Host 부착에는 새 객체의 빈 머리를 공급한다.
        const auto create=hooks.create;hooks.create=[&](std::uint32_t type,std::uint32_t flags) {
            const auto sid=create(type,flags);auto raw=scene.pool.AllocatedBytes(sid);raw[6]=0;raw[7]=0;return sid;
        };
        // native 생성자의 actual 필드 관찰과 같은 순서로 실제 Host가 소유하는 프로세스를 읽는다.
        hooks.addTimer=[&](Sid sid,std::uint32_t event,float payload) {
            scene.Event('T',{event,sid.value,Bits(payload)});auto* timer=host.AddRegular(sid,event,payload);CHECK(timer!=nullptr);
            if (!timer) return;const auto time=std::bit_cast<std::uint64_t>(timer->Time());
            scene.Event('J',{timer->Event(),Bits(timer->Payload()),timer->Parent().value,static_cast<std::uint32_t>(time),static_cast<std::uint32_t>(time>>32),timer->Count()});
        };
        RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,hooks);
        const auto value=builder.Move(scene.input[Input::Player],Sid{50},scene.input[TimeLow],scene.input[TimeHigh]);std::array<std::uint8_t,8> bytes{};
        const auto result=std::bit_cast<std::uint64_t>(value);Put(bytes,0,static_cast<std::uint32_t>(result));Put(bytes,4,static_cast<std::uint32_t>(result>>32));
        // actual Host는 parent의 종속 머리를 추가로 쓴다. native 부착 경계 밖인 그 링크는 raw 기록 대조에서 제외한다.
        const bool same=scene.Events()==row[3] && Hex(bytes)==row[4] && scene.SourcesText()==row[6];
        CHECK(same);if (!same) std::printf("건설 실제 Regular %s 행 %zu 불일치\n",row[0].c_str(),count);
        // 다음 입력의 scope 정리 전에 host가 유효할 때 프로세스를 먼저 해제한다.
        kernel.Clear();
        ++count;
    }
    CHECK(count==192);
}

// 실제 에너지 계산→정렬→같은 목록 소비를 Builder 생성/프레임/현재 시각 흐름과 합친다.
TEST_CASE(ConstructionBuilder_SharedEnergySelectSortAndCurrentClock) {
    BuilderScene scene(OriginalEdition::Patch1078);scene.Prepare(DefaultInput());const auto x=Float(scene.input[X]),y=Float(scene.input[Y]);
    scene.Node(Sid{70},84,x+4,y+4);scene.Node(Sid{71},85,x+2,y+2);scene.types[84].theme=0;scene.types[85].theme=1;scene.metadata[84].sourceFrameIndex=0;scene.metadata[85].sourceFrameIndex=1;scene.metadata[kBuilder].speed=2;
    SquidPostPopList providers{{70,71},2},selected{std::vector<std::uint32_t>(30),0};std::array<std::uint32_t,81> alliances{};std::int32_t limit=256;EnergySupplyState supplyState;
    EnergySupplyHooks energyHooks;energyHooks.providerEntry=[](std::uint32_t) { return 0x41c170U; };energyHooks.requirement=[](std::uint32_t type) { CHECK(type==100);return std::string("wr"); };
    RawEnergySupply supply(scene.pool,scene.types,providers,selected,alliances,limit,supplyState,std::move(energyHooks));
    scene.state.sources={{0xdeadbeefU},1};scene.state.now=-500;auto hooks=scene.Hooks();
    hooks.chooseSources=[](float,float,std::uint32_t,std::uint32_t) { throw std::logic_error("분리된 공급 선택 사용"); };hooks.sortSources=[](float,float) { throw std::logic_error("분리된 공급 정렬 사용"); };
    std::size_t clockReads{};ConstructionBuilderReads reads;reads.now=[&] { ++clockReads;return 5.0; };
    RawConstructionBuilder builder(scene.pool,scene.types,scene.metadata,scene.state,std::move(hooks),&supply,std::move(reads));const auto requested=std::bit_cast<std::uint64_t>(15.0);
    CHECK(builder.Move(scene.input[Input::Player],Sid{50},static_cast<std::uint32_t>(requested),static_cast<std::uint32_t>(requested>>32))==10.0);
    CHECK(scene.born==3 && clockReads==1 && selected.count==2 && selected.entries[0]==0 && selected.entries[1]==0 && builder.TakeSource()==0);
    CHECK(scene.state.sources.entries==std::vector<std::uint32_t>{0xdeadbeefU} && scene.state.sources.count==1);
    // 가까운71이 첫 운반 frame에,70이 다음 frame에 사용된다. 기존 명시 경로 효과는 실제 호출만 기록한다.
    CHECK(Get(scene.pool.Slot(Sid{101}),36)==std::bit_cast<std::uint32_t>(scene.metadata[kCarrier].firstFrames[1]));CHECK(Get(scene.pool.Slot(Sid{102}),36)==std::bit_cast<std::uint32_t>(scene.metadata[kCarrier].firstFrames[0]));
    CHECK(std::find(scene.events.begin(),scene.events.end(),std::string("V:")+std::to_string(Bits(4.0f))+":0")!=scene.events.end());
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);CHECK(Throws([&] { RawConstructionBuilder invalid(other,scene.types,scene.metadata,scene.state,scene.Hooks(),&supply); }));
}
