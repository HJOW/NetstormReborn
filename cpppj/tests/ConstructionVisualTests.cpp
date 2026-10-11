// 세 실제 PE의 건설 표시 초기화 관찰과 건설 시작의 표시/시각 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionVisual.h"
#include "o/RawConstructionProcess.h"
#include <cstdio>

namespace {
using namespace netstorm::test::construction;
// 실행기와 같은 부모·공유 색 표 항목 및 판본별 독립 입력 수다.
constexpr Sid kParent{50};
constexpr std::uint32_t kPalette=0x12000200;
constexpr std::size_t kRows=592;
// fixture 입력의 DWORD 열 순서다. 기대 입자를 계산하지 않는다.
enum Input:std::size_t { Instant,Width,Height,Pattern,Random,Cold,Mutate,Z,X,Y,ScaleX,ScaleY,Selected,Old,InputCount };
// 저장 관찰만 읽는다. 기본 CTest에서 원본 PE/게임/Python은 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONVISUAL_FIXTURE).rows;return rows; }
// 실제 풀/표시/난수와 명시 경계의 입력을 공급한다.
struct VisualScene {
    bool patch; // raw zOrder와 최종 가상 표의 판본을 선택한다.
    SidPool pool; // 부모 raw 자료다.
    GameRandom random; // 실제 공유 난수 몸체다.
    ConstructionVisualState state; // 팔레트 캐시/배율/프로세스 타입이다.
    std::vector<std::uint32_t> input; // 관찰 행의 입력 DWORD다.
    std::vector<std::string> events; // 실제 외부 요청 순서다.
    std::uint32_t failBytes{}; // 별도 계약 검사에서 실패시킬 확보 크기다.
    RawConstructionVisual visual; // 실제 표시 초기화 몸체다.
    // 장면의 수명 동안 실제 풀과 난수/경계를 공유한다.
    explicit VisualScene(OriginalEdition edition):patch(edition==OriginalEdition::Patch1078),pool(edition,kCapacity,false),visual(pool,state,random,Hooks()) {}
    // 부모 자료와 공유 상태만 설정한다. 이미지와 입자 결과는 생성하지 않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);CHECK(input.size()==InputCount);events.clear();failBytes=0;random.SetState(input[Random]);
        state={};state.palette=input[Cold] ? 0 : kPalette;state.scaleX=Float(input[ScaleX]);state.scaleY=Float(input[ScaleY]);
        auto raw=InputRaw(pool,kParent);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kVtable);raw[10]=100;
        raw[patch ? 34 : 32]=1;raw[patch ? 35 : 33]=static_cast<std::uint8_t>(input[Z]);Put(raw,14,input[X]);Put(raw,18,input[Y]);
    }
    // 각 패턴은 PE 실행에 공급한 캡처 입력이다. 입자 선택 기대값은 아니다.
    ConstructionVisualImage Image() const {
        ConstructionVisualImage image;image.width=static_cast<std::int32_t>(input[Width]);image.height=static_cast<std::int32_t>(input[Height]);
        image.hotspotX=-3;image.hotspotY=2;image.pixels.resize(static_cast<std::size_t>(image.width)*image.height);
        // 하단/구멍/중앙 열/마지막 픽셀/색 0을 구별하는 일곱 입력을 만든다.
        for (std::size_t i=0;i<image.pixels.size();++i) {
            auto color=static_cast<std::uint8_t>((i*13+17)%255);const auto pattern=input[Pattern];
            if (pattern==1) color=255;
            if (pattern==2 && i%3==0) color=255;
            if (pattern==3 && i/input[Width]>input[Height]/3) color=255;
            if (pattern==4 && i%input[Width]!=input[Width]/2) color=255;
            if (pattern==5 && i+1!=image.pixels.size()) color=255;
            if (pattern==6) color=0;
            image.pixels[i]=color;
        }
        return image;
    }
    // 실제 부착/캡처/팔레트/확보 요청을 기록하고 실행기와 같은 경계 변경만 공급한다.
    ConstructionVisualHooks Hooks() {
        ConstructionVisualHooks hooks;
        hooks.reserve=[this](std::uint32_t size) { events.push_back("A:"+std::to_string(size));return size!=failBytes; };
        hooks.attach=[this](ConstructionVisualFields& fields,std::uint32_t type) {
            events.push_back("B:"+std::to_string(fields.parent.value)+':'+std::to_string(type));
            if (input[Mutate]==1) state.palette=kPalette;
            if (input[Mutate]==2) Put(InputRaw(pool,kParent),14,Bits(7.25F));
        };
        hooks.createPalette=[this] { events.push_back("P");return kPalette; };
        hooks.gradient=[this](std::uint32_t count,std::array<double,3> from,std::array<double,3> to,double noise) {
            // 실제 경계에서 관찰한 시작/끝 RGB다.
            const std::array<double,3> blue{0,0,1},white{1,1,1},gray{128.0/255.0,128.0/255.0,128.0/255.0};
            CHECK(noise==2.4 && ((count==8 && from==blue && to==white) || (count==3 && from==white && to==gray)));
            events.push_back("G:"+std::to_string(count));
        };
        hooks.buildPalette=[this](std::uint32_t palette) { CHECK(palette==kPalette);events.push_back("J"); };
        hooks.capture=[this](Sid parent) {
            events.push_back("C:"+std::to_string(parent.value));
            if (input[Mutate]==3) { state.scaleX=-4.5F;Put(InputRaw(pool,kParent),14,Bits(-1.25F)); }
            return Image();
        };
        hooks.releaseParticles=[this] { events.push_back("F"); };return hooks;
    }
    // 직접 Seed/Sync 검사에 필요한 기존 필드만 입력으로 공급한다.
    ConstructionVisualFields Initial() const {
        ConstructionVisualFields fields;fields.parent=kParent;fields.instant=input[Instant]!=0;fields.zOrder=123;
        fields.left=1;fields.top=2;fields.right=3;fields.bottom=4;fields.palette=kPalette;fields.image=Image();
        fields.output.assign(fields.image.pixels.size(),255);fields.lastRow=(fields.image.height-1)*fields.image.width;
        fields.minimum=fields.instant ? 2.0F : Float(0x3fa66666);fields.maximum=fields.instant ? 9.0F : 2.0F;
        fields.selected=input[Selected];fields.firstDraw=true;fields.active=3;
        if (input[Old]) { fields.capacity=9;fields.particles.resize(9); }return fields;
    }
    // 외부 호출의 순서만 합친다.
    std::string Events() const {
        std::string text;
        // 같은 호출 순서를 유지한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }return text;
    }
};
// 실제 필드를 실행기의 DWORD 열 순서로 직렬화한다. 음수는 하위 DWORD 비트로 보존한다.
std::string Fields(const ConstructionVisualFields& fields,bool patch) {
    const auto table=patch ? (fields.instant ? 0x503680U : 0x5036acU) : (fields.instant ? 0x507410U : 0x507440U);
    const auto& image=fields.image;std::string text;
    // 포인터 대신 복원한 의미 있는 필드만 비교한다.
    for (std::uint32_t value:{table,static_cast<std::uint32_t>(fields.parent.value),static_cast<std::uint32_t>(fields.zOrder),
        Bits(fields.left),Bits(fields.top),Bits(fields.right),Bits(fields.bottom),fields.palette,
        static_cast<std::uint32_t>(image.width),static_cast<std::uint32_t>(image.height),static_cast<std::uint32_t>(image.hotspotX),
        static_cast<std::uint32_t>(image.hotspotY),static_cast<std::uint32_t>(fields.lastRow),Bits(fields.minimum),Bits(fields.maximum),
        fields.selected,fields.opaque,static_cast<std::uint32_t>(fields.firstDraw),fields.active,fields.capacity,fields.total}) {
        if (!text.empty()) text+=':';text+=std::to_string(value);
    }
    return text;
}
// 입자 목적지는 출력 버퍼의 픽셀 번호로 정규화한다. 미사용 칸은 0임을 별도로 확인한다.
std::string Particles(const ConstructionVisualFields& fields) {
    std::string text;CHECK(fields.particles.size()==fields.capacity);
    // 사용한 항목의 active·원래 색·수명 비트·목적지 순서를 보존한다.
    for (std::size_t i=0;i<fields.particles.size();++i) {
        const auto& particle=fields.particles[i];
        if (i>=fields.total) { CHECK(!particle.active && !particle.color && !Bits(particle.life) && !particle.pixel);continue; }
        if (!text.empty()) text+=';';text+=std::to_string(particle.active)+':'+std::to_string(particle.color)+':'+std::to_string(Bits(particle.life))+':'+std::to_string(particle.pixel);
    }
    return text.empty() ? "-" : text;
}
// 한 판본의 전체 원본 관찰을 읽고 C++ 몸체의 사건/필드/두 버퍼/입자/난수/캐시에 대조한다.
void Replay(std::string_view edition) {
    VisualScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;CHECK(Fixture().size()==3*kRows);
    // 입력만 구성하고 기대 선택/수명/좌표는 실제 PE 관찰에서 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==10);scene.Prepare(Inputs(row[2]));ConstructionVisualFields fields;
        if (row[1]=="Create") fields=scene.visual.Create(kParent,scene.input[Instant]!=0);
        else { fields=scene.Initial();if (row[1]=="Seed") scene.visual.Seed(fields);else scene.visual.Synchronize(fields); }
        const bool same=scene.Events()==row[3] && Fields(fields,scene.patch)==row[4] && Hex(fields.image.pixels)==row[5] && Hex(fields.output)==row[6] &&
            Particles(fields)==row[7] && scene.random.State()==Number(row[8]) && scene.state.palette==Number(row[9]);CHECK(same);
        if (!same) { std::printf("건설 표시 %s 행 %zu %s 불일치\n",std::string(edition).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==kRows);
}
// 계약/통합 검사에서 사용할 기본 입력이다. 결과 계산은 포함하지 않는다.
std::vector<std::uint32_t> Default() { return {0,7,9,0,0,0,0,128,Bits(20.5F),Bits(-3.25F),Bits(16.0F),Bits(11.0F),0,0}; }
}
// 패치판의 실제 생성/초기화 명령과 대조한다.
TEST_CASE(ConstructionVisual_PatchNative) { Replay("originals"); }
// CD판의 실제 생성/초기화 명령과 대조한다.
TEST_CASE(ConstructionVisual_CdNative) { Replay("originalCD"); }
// 추가 판본의 독립 PE 관찰과 대조한다.
TEST_CASE(ConstructionVisual_AdditionalNative) { Replay("original1037"); }

// 원본의 null 접근 대신 확보 실패를 예외로 처리하며 잘못된 이미지/출력은 쓰기 전에 거부한다.
TEST_CASE(ConstructionVisual_RejectsAllocationAndInvalidBuffers) {
    VisualScene scene(OriginalEdition::Patch1078);scene.Prepare(Default());scene.failBytes=136;
    CHECK(Throws([&] { scene.visual.Create(kParent,false); }) && scene.Events()=="A:136");
    auto cold=Default();cold[Cold]=1;scene.Prepare(cold);scene.failBytes=8;
    CHECK(Throws([&] { scene.visual.Create(kParent,false); }) && scene.Events()=="A:136;B:50:17;A:8" && !scene.state.palette);
    scene.Prepare(Default());scene.failBytes=63;CHECK(Throws([&] { scene.visual.Create(kParent,false); }) && scene.random.State()==0);
    scene.Prepare(Default());scene.failBytes=6000;auto unallocated=scene.Initial();
    CHECK(Throws([&] { scene.visual.Seed(unallocated); }) && unallocated.image.pixels==scene.Image().pixels && scene.random.State()==0);
    auto fields=scene.Initial();fields.image.height=0;CHECK(Throws([&] { scene.visual.Seed(fields); }));
    fields=scene.Initial();fields.output.pop_back();CHECK(Throws([&] { scene.visual.Seed(fields); }) && fields.image.pixels==scene.Image().pixels);
    scene.Prepare(Default());auto raw=InputRaw(scene.pool,kParent);raw[11]=4;CHECK(Throws([&] { scene.visual.Create(kParent,true); }) && scene.events.empty());
}

// 필수 경계와 어댑터의 같은 풀/소유 저장소 조건을 확인한다.
TEST_CASE(ConstructionVisual_RejectsMissingHooksAndForeignAdapter) {
    VisualScene scene(OriginalEdition::Patch1078);scene.Prepare(Default());auto hooks=scene.Hooks();hooks.capture={};
    CHECK(Throws([&] { RawConstructionVisual invalid(scene.pool,scene.state,scene.random,hooks); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { MakeConstructionVisualHooks(other,scene.visual,[](ConstructionVisualFields) {},{}); }));
    CHECK(Throws([&] { MakeConstructionVisualHooks(scene.pool,scene.visual,{},{}); }));
}

// 건설 시작은 표시 초기화 이후의 시각으로 실제 건설 필드를 계산한다. 팔레트는 두 번째 생성에서 재사용한다.
TEST_CASE(ConstructionVisual_StartStoresInitialStateBeforeConstructionTime) {
    VisualScene scene(OriginalEdition::Patch1078);scene.Prepare(Default());ConstructionProcessState state;ConstructionProcessFields fields;
    std::vector<ConstructionVisualFields> visuals;ConstructionProcessHooks hooks;
    hooks.reserve=[] { return true; };hooks.loadSound=[](std::string_view) { return 0U; };hooks.reserveSound=[] { return false; };
    hooks.attachSound=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {};hooks.refresh=[] {};hooks.maxHitPoints=[](Sid) { return 100; };
    hooks.updateDisplay=[](Sid) {};hooks.removeContained=[](Sid,std::uint32_t) {};hooks.playAt=[](float,float,std::string_view,std::uint32_t) {};
    hooks.readySound=[](std::uint32_t) { return 0U; };hooks.play=[](std::uint32_t) {};hooks.unpop=[](Sid,std::uint32_t) {};
    hooks.repop=[](Sid,std::uint32_t) {};hooks.push=[](float,float,std::uint32_t,std::uint32_t) {};
    hooks.attach=[&](const ConstructionProcessFields& value,std::uint32_t,Sid,std::uint32_t) { CHECK(visuals.size()==1);fields=value;return &fields; };
    auto visualHooks=scene.Hooks();visualHooks.capture=[&](Sid) { state.now=5.5;return scene.Image(); };
    RawConstructionVisual visual(scene.pool,scene.state,scene.random,visualHooks);
    auto connected=MakeConstructionVisualHooks(scene.pool,visual,[&](ConstructionVisualFields value) { visuals.push_back(std::move(value)); },hooks);
    std::vector<RiftTypeRecord> types(188);RawConstructionProcess process(scene.pool,types,state,connected);
    process.Start(kParent,0);CHECK(visuals.size()==1 && visuals[0].instant && visuals[0].minimum==2.0F && visuals[0].maximum==9.0F);
    CHECK(fields.start==5.5 && fields.end==15.5 && visuals[0].total>0);
    scene.state.palette=0;auto first=visual.Create(kParent,false);const auto size=scene.events.size();auto second=visual.Create(kParent,false);
    CHECK(first.palette==second.palette && scene.events.size()==size+4);
}
