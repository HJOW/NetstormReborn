// 원본 측정 관찰을 실제 FontStore에서 재생하고 돌 버튼 생성/부착/수명의 같은 저장소 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpText.h"
#include "client/StoneButtonGump.h"
#include "client/GumpHelp.h"
#include <chrono>
#include <fstream>
#include <memory>

namespace {
using namespace netstorm::test::gumpraw;
// 검사 전용 파일 이름의 원본 슬롯 높이/굵기/스타일 문자열이다. 실제 원본 폴더에는 쓰지 않는다.
constexpr std::array<int,7> kHeights{14,13,0,20,48,14,12},kWeights{700,0,0,700,0,0,0};
constexpr std::array<const char*,5> kStyles{"normal","italic","bold","strikeout","underline"};
// 가상 저장소 토큰은 안정된 호스트 객체를 가리킨다. PE 포인터나 호스트 주소로 재해석하지 않는다.
constexpr std::uint32_t kRoot=1,kStone=2,kBackground=3;
// 임시 캐시 파일을 만들고 해당 범위만 정리한다. fixture는 입력 폭 표/관찰 결과만 제공한다.
struct FontFixture {
    std::filesystem::path directory;
    netstorm::o::BaseFileSystem files;
    FontStore fonts;
    std::vector<std::vector<std::string>> cases;
    // 소유한 임시 디렉터리를 만들며 생성 실패 시 기존 경로를 사용하거나 지우지 않는다.
    static std::filesystem::path MakeDirectory() {
        const auto ticks=std::chrono::steady_clock::now().time_since_epoch().count();
        auto path=std::filesystem::temp_directory_path()/std::filesystem::path("netstorm-gumptext-"+std::to_string(ticks));
        if (!std::filesystem::create_directory(path)) throw std::runtime_error("GumpText 임시 경로 생성 실패");
        return path;
    }
    // 임의 캐시 높이와 원본 폭 표 입력으로 FontStore 파일 조회까지 실제로 연결한다.
    FontFixture():directory(MakeDirectory()),files(directory),fonts(files,"Arial") {
        try {
            std::filesystem::create_directory(directory/"d");
            std::ifstream input(NETSTORM_GUMPTEXT_FIXTURE);
            if (!input) throw std::runtime_error("GumpText fixture 없음");
            std::string line;
            // 폰트 입력 행은 캐시로 저장하고 관찰 행은 기대값 계산 없이 그대로 보관한다.
            while (std::getline(input,line)) {
                if (line.empty() || line[0]=='#') continue;
                auto parts=Split(line,'\t');
                if (parts.at(0)=="Font") WriteFont(parts);
                else if (parts.at(0)=="Case") cases.push_back(std::move(parts));
                else throw std::runtime_error("GumpText fixture 행 종류 오류");
            }
        } catch (...) { std::filesystem::remove_all(directory);throw; }
    }
    // 소유권을 얻은 검사 경로만 정리하며 소멸 중 파일 시스템 예외는 던지지 않는다.
    ~FontFixture() { std::error_code error;std::filesystem::remove_all(directory,error); }
    // 네 폭 표 중 첫 표만 입력대로 만들고 모든 글리프 포인터/블록은 빈 캐시로 둔다.
    void WriteFont(const std::vector<std::string>& row) const {
        const auto slot=Number(row.at(1)),style=Number(row.at(2)),height=Number(row.at(3));
        const auto widths=Bytes(row.at(4));
        if (slot>=7 || style>=5 || widths.size()!=1024) throw std::runtime_error("GumpText 폰트 입력 폭 오류");
        std::vector<std::uint8_t> cache(4132);
        // .chfnt의 머리 서명은 마지막 NUL을 포함한 16바이트다.
        constexpr std::string_view signature{"BitmapFontData\x1a\0",16};
        std::copy(signature.begin(),signature.end(),cache.begin());
        Put(cache,16,1);Put(cache,20,height);Put(cache,24,height-5);Put(cache,28,5);Put(cache,32,256);
        std::copy(widths.begin(),widths.end(),cache.begin()+36);
        const std::string face=slot==1 ? "Courier New" : "Arial";
        const auto name="!"+face+'.'+kStyles[style]+'.'+std::to_string(kHeights[slot])+'.'+std::to_string(kWeights[slot])+".chfnt";
        std::ofstream output(directory/"d"/name,std::ios::binary);
        output.write(reinterpret_cast<const char*>(cache.data()),static_cast<std::streamsize>(cache.size()));
        if (!output) throw std::runtime_error("GumpText 임시 캐시 저장 실패");
    }
};
// 실제 기본 생성·트리·수명·버튼과 글꼴 측정이 모두 같은 기본/파생 노드를 사용한다.
struct StoneScene {
    GumpCreateState create;
    GumpTreeState treeState;
    GumpLifetimeState lifeState;
    GumpEventState eventState;
    GumpInputState inputState;
    ButtonGumpState buttonState;
    StoneButtonGumpState stoneState;
    GumpCreateNode root;
    StoneButtonGumpNode node;
    GumpBackgroundNode background;
    std::int8_t backgroundColor{-4};
    bool failAllocation{};
    std::vector<std::uint32_t> released;
    std::unique_ptr<GumpCreate> factory;
    std::unique_ptr<GumpCollection> collection;
    std::unique_ptr<GumpTree> tree;
    std::unique_ptr<GumpLifetime> lifetime;
    std::unique_ptr<GumpEvents> events;
    std::unique_ptr<GumpInput> input;
    std::unique_ptr<ButtonGump> buttons;
    std::unique_ptr<GumpBackgroundCreate> backgrounds;
    std::unique_ptr<StoneButtonGump> stones;
    // 모든 기본/파생 저장소의 안정된 기본 주소를 반환한다.
    GumpCreateNode* Find(std::uint32_t token) {
        if (token==kRoot) return &root;if (token==kStone) return &node;
        if (token==kBackground) return &background;return nullptr;
    }
    // 순환 참조는 처리기 포인터를 실행 시 조회하여 생성 뒤 연결한다.
    explicit StoneScene(const GumpText& text,netstorm::o::OriginalEdition edition=netstorm::o::OriginalEdition::Patch1078) {
        create.screenWidth=640;create.screenHeight=480;create.collection.edition=edition;
        treeState.mapView={0,0,640,480};stoneState.color=-7;stoneState.palette={9,243,57};
        const auto lookup=[this](auto token) { return Find(token); };
        GumpCreateHooks make;make.node=lookup;make.allocate=[](auto size) { CHECK(size==kGumpBaseSize);return kRoot; };
        factory=std::make_unique<GumpCreate>(create,make);factory->InitializeRoot();
        collection=std::make_unique<GumpCollection>(create.collection,factory->CollectionHooks());
        GumpTreeHooks branch;branch.node=lookup;branch.redraw=[](auto,bool) {};
        branch.push=[this](auto token) { tree->Push(token); };
        branch.attach=[this](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); };
        branch.kill=[this](auto token) { lifetime->Kill(token); };
        GumpLifetimeHooks life;life.node=lookup;life.push=branch.push;
        life.deletingDestructor=[this](auto token,auto flags) { lifetime->DeleteScalar(token,flags); };
        life.release=[this](auto token) { released.push_back(token); };
        BindStoneButtonGump(branch,life,[this](auto token) { return token==kStone ? stones.get() : nullptr; });
        tree=std::make_unique<GumpTree>(create,treeState,branch);
        lifetime=std::make_unique<GumpLifetime>(create,lifeState,life);
        GumpEventHooks e;e.node=lookup;
        events=std::make_unique<GumpEvents>(create,lifeState,eventState,*collection,e);
        GumpInputHooks i;i.node=lookup;
        input=std::make_unique<GumpInput>(create,lifeState,inputState,*collection,*events,i);
        ButtonGumpHooks b;b.node=[this](auto token) { return token==kStone ? &node : nullptr; };b.baseNode=lookup;
        b.backgroundFlags=[this](auto token) { return token==kBackground ? &background.backgroundFlags : nullptr; };
        buttons=std::make_unique<ButtonGump>(create,buttonState,*factory,*collection,*events,*input,*lifetime,b);
        GumpBackgroundCreateHooks bg;bg.node=[this](auto token) { return token==kBackground ? &background : nullptr; };
        backgrounds=std::make_unique<GumpBackgroundCreate>(create,*factory,backgroundColor,bg);
        StoneButtonGumpHooks s;s.node=[this](auto token) { return token==kStone ? &node : nullptr; };
        s.allocate=[this](auto size) { CHECK(size==0xb8);return failAllocation ? 0U : kBackground; };
        s.push=branch.push;s.attach=branch.attach;BindStoneButtonText(s,text);
        stones=std::make_unique<StoneButtonGump>(create,stoneState,*factory,*buttons,*backgrounds,*tree,*lifetime,s);
    }
};
}

// 실제 원본의 전체 슬롯/스타일·길이·정수 감김·출력 alias를 실제 FontStore 공급으로 대조한다.
TEST_CASE(GumpText_NativeMeasurementsAndDelegationMatchThreeEditions) {
    FontFixture fixture;GumpText patch(fixture.fonts),cd(fixture.fonts,netstorm::o::OriginalEdition::Cd1072);
    std::array<std::size_t,3> counts{};
    // 915개의 원본 관찰 행을 입력 그대로 재생한다. 파일/Python/PE 실행이 필요 없는 검사다.
    for (const auto& row:fixture.cases) {
        CHECK(row.size()==13);if (row.size()!=13) continue;
        const auto edition=row[1]=="originals" ? 0U : row[1]=="originalCD" ? 1U : 2U;
        const auto raw=Bytes(row[10]);const std::string bytes(raw.begin(),raw.end());
        GumpTextContext context{Number(row[5]),Number(row[3]),0,Number(row[4]),0,0,
            static_cast<std::int32_t>(std::stoll(row[6])),static_cast<std::int32_t>(std::stoll(row[7]))};
        std::int32_t width=17,height=19;const bool alias=Number(row[9])!=0;
        (edition==0 ? patch : cd).Measure(std::span(bytes.data(),bytes.size()),context,width,alias ? width : height,
            static_cast<std::int32_t>(std::stoll(row[8])));
        CHECK(width==std::stoll(row[11]));CHECK((alias ? width : height)==std::stoll(row[12]));++counts[edition];
    }
    CHECK((counts==std::array<std::size_t,3>{305,305,305}));
    CHECK(fixture.fonts.Get(0).Height()==31 && FontStore::LogicalHeight(0)==14);
}

// 잘못된 바이트 범위를 읽거나 출력 일부를 쓴 뒤 예외를 내는 일을 방지한다.
TEST_CASE(GumpText_BoundedContractAndSignedLengthPreserveOutputs) {
    std::array<std::int32_t,256> widths{};widths[0]=23;
    std::array<char,2> bytes{'A','B'};GumpTextContext context{2,0,0,0,0,0,-3,5};
    std::int32_t width=17,height=19;bool caught=false;
    try { MeasureGumpText(bytes,widths,14,context,width,height); } catch (const std::invalid_argument&) { caught=true; }
    CHECK(caught && width==17 && height==19);caught=false;
    try { MeasureGumpText(bytes,widths,14,context,width,height,3); } catch (const std::invalid_argument&) { caught=true; }
    CHECK(caught && width==17 && height==19);
    MeasureGumpText({},widths,14,context,width,height,-2);CHECK(width==-3 && height==19);
    bytes[0]=0;MeasureGumpText(bytes,widths,14,context,width,height,1);CHECK(width==20 && height==19);
    FontFixture fixture;GumpText text(fixture.fonts);context.font=0xffffffffU;caught=false;width=17;
    try { text.Measure({},context,width,height,0); } catch (const std::invalid_argument&) { caught=true; }
    CHECK(caught && width==17 && height==19);
}

// 기본 문맥의 그림자 높이를 실제 측정해 부모 크기를 정하고 배경/자식 깊이를 가상 부착에서 맞춘다.
TEST_CASE(StoneButtonText_ActualFontMeasureAttachAndChildFirstDestroyShareStorage) {
    FontFixture fixture;GumpText text(fixture.fonts);StoneScene scene(text);
    scene.stoneState.defaultContext={2,5,7,4,0,9,1,1};
    // 파생 생성 전에 있던 미작성 문맥/측정값/라벨 꼬리가 보존되는지 함께 확인한다.
    for (auto& context:scene.node.contexts) { context.extra=0x81726354;context.shadowColor=29;context.shadowX=37;context.shadowY=43; }
    scene.node.label[63]='Z';scene.node.textWidth=81;scene.node.textHeight=82;
    CHECK(scene.stones->Construct(kStone,75,19,"",0xffffffffU)==kStone);
    CHECK(scene.create.count==3 && scene.create.serial==3 && scene.node.background==kBackground);
    CHECK(scene.node.label[63]=='Z' && scene.node.textWidth==81 && scene.node.textHeight==82);
    CHECK(scene.node.buttonFlags==0 && scene.node.command==0xfffffff9U);
    CHECK(scene.node.contexts[0].extra==0x81726354 && scene.node.contexts[2].shadowX==37 && scene.node.contexts[2].shadowY==43);
    CHECK(scene.background.parent==kStone && scene.background.width==73 && scene.background.height==17);
    CHECK(scene.node.textContext==scene.stoneState.defaultContext);
    scene.stones->FitToText(kStone);
    CHECK(scene.node.textWidth==1 && scene.node.textHeight==15 && scene.node.width==13 && scene.node.height==21);
    CHECK(scene.background.width==73 && scene.background.height==17);
    // 부모의 가상 Attach는 배경 크기를 먼저 바꾸고 기본 트리의 자식 재배치까지 연결한다.
    scene.tree->Move(kStone,40,50,kRoot);
    CHECK(scene.node.parent==kRoot && scene.node.x==40 && scene.node.y==50);
    CHECK(scene.background.width==11 && scene.background.height==19 && scene.background.parent==kStone);
    CHECK(scene.background.depth==scene.node.depth+1 && scene.root.child==kStone);
    scene.lifetime->Kill(kStone);CHECK(scene.lifeState.killCount==2);
    scene.lifetime->Cleanup();CHECK(scene.create.count==1 && scene.create.entries[0]==kRoot);
    CHECK((scene.released==std::vector<std::uint32_t>{kBackground,kStone}));
}

// 라벨 변경은 측정만 수행한다. null 생성/짧은 복사/긴 문자열의 NUL 범위 계약을 각각 확인한다.
TEST_CASE(StoneButtonText_LabelChangesDoNotResizeAndLongTailIsNotInvented) {
    FontFixture fixture;GumpText text(fixture.fonts);StoneScene scene(text);
    scene.node.label.fill('Z');scene.stones->Construct(kStone,75,19,nullptr);
    CHECK(scene.node.label[0]==0 && scene.node.label[1]=='Z' && scene.node.label[63]=='Z');
    scene.stones->SetLabel(kStone,"");CHECK(scene.node.textWidth==0 && scene.node.textHeight==14);
    CHECK(scene.node.width==75 && scene.background.width==73 && scene.node.label[62]==0 && scene.node.label[63]=='Z');
    // 같은 원본 관찰의 첫 행은 슬롯 0/normal·그림자 없음·별도 출력의 "Ab" 폭이다.
    const auto& native=fixture.cases.front();
    CHECK(native[1]=="originals" && native[3]=="0" && native[4]=="0" && native[5]=="0" && native[9]=="0");
    const auto expected=static_cast<std::int32_t>(std::stoll(native[11]));
    scene.stones->SetLabel(kStone,"Ab");CHECK(scene.node.textWidth==expected && scene.node.textHeight==14);
    CHECK(scene.node.width==75 && scene.background.width==73);
    const std::string longLabel(80,'A');bool caught=false;
    try { scene.stones->SetLabel(kStone,longLabel.c_str()); } catch (const std::invalid_argument&) { caught=true; }
    CHECK(caught && scene.node.label[62]=='A' && scene.node.label[63]=='Z');
    CHECK(scene.node.textWidth==expected && scene.node.textHeight==14);
}

// 생성자가 배경 할당 실패 전에 완료한 부모 등록/라벨 쓰기를 되돌리지 않는 호스트 경계다.
TEST_CASE(StoneButtonText_AllocationFailurePreservesRegisteredParent) {
    FontFixture fixture;GumpText text(fixture.fonts);StoneScene scene(text);scene.failAllocation=true;bool caught=false;
    try { scene.stones->Construct(kStone,75,19,"Fail"); } catch (const std::runtime_error&) { caught=true; }
    CHECK(caught && scene.create.count==2 && scene.node.background==0 && scene.node.label[0]=='F');
    CHECK(scene.create.entries[scene.node.id]==kStone && scene.node.parent==0);
}

// 도움말의 앞선 메뉴 콜백이 바꾼 현재 제목/본문/주제/루트를 다음 경계가 실제로 읽는지 확인한다.
TEST_CASE(GumpHelp_CurrentCallbacksAndEditionArgumentsKeepOrder) {
    // 패치와 CD의 마지막 항목 인자 차이를 모두 검사한다.
    for (auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        GumpCreateState create;create.collection.edition=edition;create.entries[0]=91;
        GumpCreateNode node;node.alternateTextPointer=5;GumpHelpState state;state.body=11;state.event=12;
        std::vector<std::string> calls;GumpHelpHooks hooks;hooks.node=[&](auto) { return &node; };
        hooks.text=[](auto token) { CHECK(token==5);return std::string("priest\0ignored",14); };
        hooks.beginMenu=[&](auto value) { CHECK(value==0);calls.push_back("begin");node.textPointer=7; };
        hooks.title=[&](auto title) { CHECK(title==7);calls.push_back("title");state.body=21;state.event=22;state.topic="changedHelp"; };
        hooks.entry=[&](auto body,auto event,const auto& topic,auto a,auto b,auto extra) {
            CHECK(body==21 && event==22 && topic=="changedHelp" && a==1 && b==1);
            CHECK(edition==netstorm::o::OriginalEdition::Patch1078 ? extra==std::optional<std::uint32_t>{0} : !extra);
            calls.push_back("entry");create.entries[0]=93;
        };
        hooks.attachMenu=[&](auto x,auto y,auto parent) { CHECK(x==-7 && y==13 && parent==93);calls.push_back("attach"); };
        hooks.sound=[&](auto name) { CHECK(name=="opengump.wav");calls.push_back("sound"); };
        GumpHelp help(create,state,hooks);ButtonGumpHooks button;BindGumpHelp(button,help);
        CHECK(button.baseEvent(1,InputCode::kRightButton,0xfffffff9U,13)==1);
        CHECK((calls==std::vector<std::string>{"begin","title","entry","attach","sound"}));
        CHECK(help.Handle(1,InputCode::kRightButton|InputCode::kRelease,0,0)==0);
        node.alternateTextPointer=0;CHECK(help.Handle(1,InputCode::kRightButton,0,0)==0);
        CHECK(calls.size()==5);
    }
}
