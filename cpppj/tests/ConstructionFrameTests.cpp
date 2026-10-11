// 세 PE의 건설 표시 whole body 관찰을 재생하고 생성 이후 연속 갱신/Draw 연결을 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionFrame.h"
#include <cstdio>

namespace {
using namespace netstorm::test::construction;
// 실행기의 합성 부모/팔레트와 판본별 독립 행 수다.
constexpr Sid kParent{50};
constexpr std::uint32_t kPalette=0x12000200;
constexpr std::size_t kPatchRows=435,kCdRows=441;
// 입력은 DWORD와 시각 double의 64비트 패턴을 함께 포함한다.
enum Input:std::size_t { Instant,Pattern,Random,Capacity,Total,Active,Selected,Hp,Maximum,First,Now,Last,Elapsed,
    Rate,X,Y,ScaleX,ScaleY,CameraX,CameraY,Pixel,Life,Colors,Mutate,Free,Parent,Source,MaxLife,InputCount };
// CTest는 저장 관찰만 읽으며 원본 게임/PE/Python/장치를 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONFRAME_FIXTURE).rows;return rows; }
// float 입력의 하위 DWORD만 그대로 읽는다.
float F(std::uint64_t value) { return Float(static_cast<std::uint32_t>(value)); }
// 콜백 사건 또는 필드를 순서대로 이어 붙인다.
std::string Join(const std::vector<std::string>& values,char separator) {
    std::string result;
    // 입력 순서와 빈 열을 보존한다.
    for (const auto& value:values) { if (!result.empty()) result+=separator;result+=value; }return result.empty() ? "-" : result;
}
// 실제 풀/시각/입자/팔레트와 원본과 같은 입력 경계를 소유한다.
struct FrameScene {
    bool patch; // 원본 raw HP 폭과 가상 표 판본이다.
    SidPool pool; // 부모 raw 저장소다.
    GameRandom random; // 실제 공유 난수다.
    ConstructionVisualState visualState; // 현재 표시 배율이다.
    ConstructionFrameState state; // 현재 시각/카메라/배율이다.
    std::vector<std::uint32_t> colors; // 조회 가능한 경계 뒤 DWORD까지 포함한다.
    ConstructionFramePalette palette; // count와 실제 읽기 배열을 구분한다.
    std::vector<std::uint64_t> input; // 저장 관찰의 입력 값이다.
    std::vector<std::string> events; // 외부 호출 순서다.
    std::size_t dirtyCount{}; // 첫 dirty 경계 변경 시나리오용이다.
    RawConstructionFrame frame; // 실제 갱신/Draw 몸체다.
    // 같은 풀/난수/현재 전역을 모든 몸체와 경계가 공유한다.
    explicit FrameScene(OriginalEdition edition):patch(edition==OriginalEdition::Patch1078),pool(edition,32768,false),
        frame(pool,visualState,state,palette,random,Hooks()) {}
    // 최대 HP·dirty 큐·장치·Kill의 명시 입력/관찰 경계다.
    ConstructionFrameHooks Hooks() {
        ConstructionFrameHooks hooks;
        // 원본과 같은 경계 중 HP/좌표/시각 변경이다.
        hooks.maxHitPoints=[this](Sid parent) {
            CHECK(parent==kParent);events.push_back("H:50");auto raw=InputRaw(pool,kParent);
            if (input[Mutate]==1) Put(raw,26,1);if (input[Mutate]==2) Put(raw,26,100);
            if (input[Mutate]==3) { Put(raw,14,Bits(-7.75F));state.now=30; }
            return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(input[Maximum]));
        };
        // 이전/새 사각형 사이에 카메라/부모 abstract가 바뀔 수 있다.
        hooks.invalidate=[this](ConstructionFrameRect rectangle) {
            events.push_back("I:"+std::to_string(rectangle.left)+':'+std::to_string(rectangle.top)+':'+std::to_string(rectangle.right)+':'+std::to_string(rectangle.bottom));
            if (!dirtyCount && input[Mutate]==4) state.cameraX=100.25F;
            if (!dirtyCount && input[Mutate]==5) InputRaw(pool,kParent)[patch ? 40 : 35]=0;++dirtyCount;
        };
        // 현재 출력 버퍼 span은 호출 중에만 검사하고 보관하지 않는다.
        hooks.draw=[this](const ConstructionFrameDraw& request) {
            CHECK(request.pixels.size()==64 && request.pitch==8 && request.clip.left==0 && request.clip.top==0 && request.clip.right==8 && request.clip.bottom==8);
            events.push_back("D:"+std::to_string(request.x)+':'+std::to_string(request.y)+':'+std::to_string(request.pitch)+":0:0:8:8");
        };
        // Kill의 실제 삭제/해제는 외부 소유자에게 연결할 다음 경계다.
        hooks.kill=[this](std::uint32_t flags) { CHECK(!flags);events.push_back("K:0"); };return hooks;
    }
    // 기대 프레임 결과 대신 raw/전역/팔레트 입력만 준비한다.
    void Prepare(const std::string& text) {
        input.clear();events.clear();dirtyCount=0;
        // 64비트 시각과 음수 픽셀/HP의 비트를 그대로 보존한다.
        for (const auto& value:Split(text,',')) input.push_back(std::stoull(value));CHECK(input.size()==InputCount);
        auto raw=InputRaw(pool,kParent);std::fill(raw.begin(),raw.end(),std::uint8_t{0});Put(raw,0,patch ? 0x501dd0 : 0x5058d8);
        raw[10]=100;raw[11]=static_cast<std::uint8_t>(input[Free]);raw[patch ? 40 : 35]=1;
        Put(raw,14,static_cast<std::uint32_t>(input[X]));Put(raw,18,static_cast<std::uint32_t>(input[Y]));Put(raw,26,static_cast<std::uint32_t>(input[Hp]));
        random.SetState(static_cast<std::uint32_t>(input[Random]));visualState.scaleX=F(input[ScaleX]);visualState.scaleY=F(input[ScaleY]);
        state={std::bit_cast<double>(input[Now]),F(input[Rate]),F(input[CameraX]),F(input[CameraY])};colors.clear();
        // 색 수와 인접 DWORD를 구분하기 위한 고유 DWORD 입력이다.
        for (std::uint32_t i=0;i<=input[Colors];++i) colors.push_back(0x81000000U+i*257+7);
        palette={kPalette,static_cast<std::int32_t>(input[Colors]),colors};
    }
    // 실행기의 8×8 소스 입력과 이미 선택된 세 입자를 공급한다.
    ConstructionFrameFields Initial() const {
        ConstructionFrameFields fields;auto& v=fields.visual;v.parent=Sid{static_cast<std::uint16_t>(input[Parent])};v.instant=input[Instant]!=0;
        v.left=1.1F;v.top=-2.1F;v.right=3.1F;v.bottom=4.1F;v.lastTime=std::bit_cast<double>(input[Last]);v.elapsed=std::bit_cast<double>(input[Elapsed]);
        fields.delta=13;fields.scaledDelta=17;v.palette=kPalette;v.image={8,8,-3,2,{}};v.output.assign(64,std::uint8_t{255});
        // 아직 선택하지 않은 픽셀의 입력 패턴만 만든다.
        for (std::uint32_t i=0;i<64;++i) v.image.pixels.push_back(input[Pattern]==2 && i%3==0 ? 255 : static_cast<std::uint8_t>((i*13+17)%255));
        v.lastRow=56;v.minimum=v.instant ? 2.0F : Float(0x3fa66666);v.maximum=F(input[MaxLife]);v.selected=static_cast<std::uint32_t>(input[Selected]);
        v.opaque=64;v.firstDraw=input[First]!=0;v.active=static_cast<std::uint32_t>(input[Active]);v.capacity=static_cast<std::uint32_t>(input[Capacity]);
        v.total=static_cast<std::uint32_t>(input[Total]);v.particles.resize(v.capacity);
        // 이미 선택된 입자 위치/수명/색은 입력이며 알고리즘으로 계산하지 않는다.
        for (std::uint32_t i=0;i<v.total;++i) {
            const auto pixel=27+i*8;v.particles[i]={static_cast<std::uint8_t>(i<v.active),v.image.pixels[pixel],F(input[Life])*(i+1),pixel};v.image.pixels[pixel]=255;
        }
        if (!input[Source]) v.image.pixels.clear();return fields;
    }
};
// 실행기의 전체 필드/입자/두 버퍼/난수 직렬화 순서다. 기대 갱신 결과는 재계산하지 않는다.
std::vector<std::string> Snapshot(const ConstructionFrameFields& fields,const GameRandom& random) {
    const auto& v=fields.visual;std::vector<std::string> values,particles;
    // 원본 DWORD 필드를 비트 그대로 비교한다.
    for (std::uint32_t value:{Bits(v.left),Bits(v.top),Bits(v.right),Bits(v.bottom),Bits(fields.delta),Bits(fields.scaledDelta),
        v.selected,v.opaque,static_cast<std::uint32_t>(v.firstDraw),v.active,v.capacity,v.total}) values.push_back(std::to_string(value));
    values.push_back(std::to_string(std::bit_cast<std::uint64_t>(v.lastTime)));values.push_back(std::to_string(std::bit_cast<std::uint64_t>(v.elapsed)));
    // 미사용 칸의 잔여 값도 원본의 두 번 복사한 순서와 비교한다.
    for (const auto& particle:v.particles) particles.push_back(std::to_string(particle.active)+':'+std::to_string(particle.color)+':'+std::to_string(Bits(particle.life))+':'+std::to_string(particle.pixel));
    return {Join(values,':'),Hex(v.image.pixels),Hex(v.output),Join(particles,';'),std::to_string(random.State())};
}
// 한 판본의 whole body/중간 관찰을 실제 C++ 몸체와 대조한다.
void Replay(std::string_view edition) {
    FrameScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);
    // 입력만 구성하며 기대 필드/입자/출력은 저장 PE 관찰에서 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==11);scene.Prepare(row[2]);auto fields=scene.Initial();std::uint32_t result=0;std::vector<std::string> history;
        if (row[1]=="Run") scene.frame.Run(fields);
        else if (row[1]=="Expand") scene.frame.Expand(fields,static_cast<std::uint32_t>(scene.input[Pixel]));
        else if (row[1]=="Append") { if (scene.frame.Append(fields,static_cast<std::int32_t>(scene.input[Pixel]))) result=fields.visual.total; }
        else if (row[1]=="Color") result=scene.frame.Color(F(scene.input[Life]),F(scene.input[MaxLife]));
        else if (row[1]=="Clock") scene.frame.Clock(fields);
        else if (row[1]=="Draw") scene.frame.Draw(fields);
        else {
            // 원본과 같은 외부 HP/시각 입력으로 모든 중간 상태를 검사한다.
            for (int step=0;step<5;++step) {
                scene.state.now=step*.0625;Put(InputRaw(scene.pool,kParent),26,step*25);
                if (step==4) InputRaw(scene.pool,kParent)[scene.patch ? 40 : 35]=0;
                scene.frame.Run(fields);scene.frame.Draw(fields);history.push_back(Join(Snapshot(fields,scene.random),'|'));
            }
        }
        auto observed=Snapshot(fields,scene.random);
        // source=null의 조기 반환에서는 입력 메모리의 원래 이미지가 따로 남는다.
        if (!scene.input[Source]) { auto saved=scene.input;scene.input[Source]=1;observed[1]=Hex(scene.Initial().visual.image.pixels);scene.input=std::move(saved); }
        const std::vector<std::string> expected(row.begin()+4,row.begin()+9);
        const bool same=Join(scene.events,';')==row[3] && observed==expected && result==Number(row[9]) && Join(history,'~')==row[10];CHECK(same);
        if (!same) {
            std::printf("건설 프레임 %s 행 %zu %s 불일치\n",std::string(edition).c_str(),count,row[1].c_str());
            // 실패한 관찰 열만 출력해 부동소수점/상태 차이를 찾는다.
            for (std::size_t i=0;i<observed.size();++i) if (observed[i]!=expected[i]) std::printf("열 %zu 실제 %s 예상 %s\n",i,observed[i].c_str(),expected[i].c_str());
            break;
        }++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
// 계약 검사에서 쓰는 유한 입력이다. 결과 계산은 포함하지 않는다.
std::string Default() {
    return Join({"0","0","17","12","3","0","3","50","101","1",std::to_string(std::bit_cast<std::uint64_t>(.025)),"0",
        std::to_string(std::bit_cast<std::uint64_t>(1000000.125)),std::to_string(Bits(.7F)),std::to_string(Bits(20.1F)),std::to_string(Bits(-3.2F)),
        std::to_string(Bits(16.0F)),std::to_string(Bits(11.0F)),std::to_string(Bits(.75F)),std::to_string(Bits(-.25F)),"27",
        std::to_string(Bits(.25F)),"11","0","0","50","1",std::to_string(Bits(2.0F))},',');
}
}
// 패치판의 전체 명령 관찰을 비교한다.
TEST_CASE(ConstructionFrame_PatchNative) { Replay("originals"); }
// CD판의 전체 명령 관찰을 비교한다.
TEST_CASE(ConstructionFrame_CdNative) { Replay("originalCD"); }
// 추가 판본을 별도 PE로 관찰한 결과를 비교한다.
TEST_CASE(ConstructionFrame_AdditionalNative) { Replay("original1037"); }

// 안전하지 않은 원본 assert/포인터 입력은 실제 쓰기 전에 호스트 예외로 거부한다.
TEST_CASE(ConstructionFrame_RejectsInvalidStorageAndPalette) {
    FrameScene scene(OriginalEdition::Patch1078);scene.Prepare(Default());auto fields=scene.Initial();const auto source=fields.visual.image.pixels;
    fields.visual.lastRow=63;CHECK(Throws([&] { scene.frame.Run(fields); }) && fields.visual.image.pixels==source && scene.events.empty());
    fields=scene.Initial();fields.visual.output.pop_back();CHECK(Throws([&] { scene.frame.Run(fields); }));
    fields=scene.Initial();fields.visual.particles[0].pixel=64;CHECK(Throws([&] { scene.frame.Expand(fields,27); }));
    fields=scene.Initial();CHECK(Throws([&] { scene.frame.Append(fields,64); }) && fields.visual.total==3 && scene.random.State()==17);
    fields.visual.parent={};CHECK(Throws([&] { scene.frame.Run(fields); }));
    scene.palette.entries=std::span<const std::uint32_t>(scene.colors).first(11);
    CHECK(Throws([&] { scene.frame.Color(2.0F,2.0F); }));CHECK(Throws([&] { scene.frame.Color(1.0F,0.0F); }));
}

// 종료/장치 경계는 누락할 수 없고 용량 0 Draw는 부모/장치에 접근하지 않는다.
TEST_CASE(ConstructionFrame_RequiresHooksAndSkipsEmptyCapacity) {
    FrameScene scene(OriginalEdition::Patch1078);scene.Prepare(Default());auto hooks=scene.Hooks();hooks.kill={};
    CHECK(Throws([&] { RawConstructionFrame invalid(scene.pool,scene.visualState,scene.state,scene.palette,scene.random,hooks); }));
    ConstructionFrameFields fields;scene.frame.Draw(fields);CHECK(scene.events.empty());
    fields.visual.capacity=1;fields.visual.total=2;CHECK(Throws([&] { scene.frame.Draw(fields); }));
}

// 실제 표시 생성의 소유 상태를 연속 Run/Draw에 넘기고 원래 픽셀 복구와 abstract 종료 요청을 확인한다.
TEST_CASE(ConstructionFrame_CreateThenRunDrawAndFinish) {
    // 두 생성 방식과 두 판본을 실제 복원 몸체로 연결한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        FrameScene scene(edition);scene.Prepare(Default());auto raw=InputRaw(scene.pool,kParent);raw[scene.patch ? 34 : 32]=1;
        scene.visualState.palette=kPalette;auto input=scene.Initial().visual.image;
        // 실제 SHP처럼 투명 테두리가 있는 선택 전 캡처 입력을 공급한다.
        for (std::size_t i=0;i<input.pixels.size();++i) input.pixels[i]=(i/8==0 || i/8==7 || i%8==0 || i%8==7) ? 255 : static_cast<std::uint8_t>((i*13+17)%255);
        ConstructionVisualHooks hooks;
        hooks.reserve=[](std::uint32_t) { return true; };hooks.attach=[](ConstructionVisualFields&,std::uint32_t) {};
        hooks.createPalette=[] { return kPalette; };hooks.gradient=[](std::uint32_t,std::array<double,3>,std::array<double,3>,double) {};
        hooks.buildPalette=[](std::uint32_t) {};hooks.capture=[&](Sid) { return input; };hooks.releaseParticles=[] {};
        RawConstructionVisual visual(scene.pool,scene.visualState,scene.random,hooks);
        // opaque=36, 최대 HP=21에서는 원본 목표의 정수 절단 후에도 모든 선택 입자가 활성화된다.
        scene.input[Maximum]=21;
        // 첫 프레임 초기화 뒤 HP 100%에서 모든 선택 입자를 충분한 시간에 걸쳐 만료시킨다.
        for (bool instant:{false,true}) {
            Put(raw,26,0);
            ConstructionFrameFields fields;fields.visual=visual.Create(kParent,instant);scene.events.clear();scene.dirtyCount=0;scene.state.now=0;
            const auto opaque=fields.visual.opaque;CHECK(fields.visual.total>0 && fields.visual.capacity==500);
            scene.frame.Run(fields);scene.frame.Draw(fields);CHECK(fields.visual.firstDraw && fields.delta==0 && !scene.events.empty());
            Put(raw,26,21);
            // 앞 프레임의 확장이 다음 프레임에도 계속될 수 있으므로 실제 상태로 여러 번 갱신한다.
            for (int i=1;i<=20;++i) { scene.state.now=i*1.0;scene.frame.Run(fields); }
            CHECK(fields.visual.total==0 && fields.visual.active==0 && fields.visual.selected==opaque);
            CHECK(fields.visual.output==input.pixels);
            const auto count=scene.events.size();scene.frame.Draw(fields);CHECK(scene.events.size()==count+1);
            raw[scene.patch ? 40 : 35]=0;scene.frame.Run(fields);CHECK(scene.events.back()=="K:0");raw[scene.patch ? 40 : 35]=1;
        }
    }
}
