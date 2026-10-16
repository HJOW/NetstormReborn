// 원본 Handle의 외부 효과 입력/현재 재조회/전체250BYTE를 같은 실제 Frame과 비교한다.
#include "RawSceneSupport.h"
#include "o/RawFenceRegular.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원본 입력과 같은 A~F의8개씩인48프레임 코드 표다.
RiftTypeFrames FenceRegularCodes() {
    std::vector<FrameCode> codes; //결과가 아닌 입력 구간이다.
    for (unsigned index=0;index<48;++index) codes.push_back({static_cast<std::uint8_t>('A'+index/8),'A',static_cast<std::uint8_t>(index),0}); //실제 TYPE 구간 계산기가 다시 센다.
    return RiftTypeFrames(std::move(codes));
}
// 선언한 명시 하위만 관찰하고 실제 부모 판단과 Frame 지정은 제품 모듈로 실행한다.
struct FenceRegularScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; //부모50/이웃51~54와 실제 고정 예측 free list를 포함한다.
    std::vector<RiftTypeRecord> types{188}; //같은 실제 Frame 지정기의 TYPE 입력이다.
    std::vector<RiftTypeFrames> frames{188,FenceRegularCodes()}; //현재 글자/개수 표다.
    std::vector<std::uint32_t> input; //원본 관찰이 제공한12개 입력 비트다.
    std::string trace; //실제 경계의 호출 순서다.
    std::size_t updates{},markers{},reserves{},connections{}; //각 명시 효과에서의 현재 누계다.
    SquidFrame setter; //StoreLevel/프레임 쓰기/표시 갱신을 실제로 수행한다.
    std::unique_ptr<RawFenceRegular> body; //같은 풀/프레임/외부 입력을 사용하는 실제 Handle다.
    // 실제 Frame의 앞/뒤 표시와 원본과 같은 독립 입력을 조립한다.
    explicit FenceRegularScene(const std::string& values):setter(pool,types,{
        [](Sid,std::int32_t) { return std::array<float,2>{1,1}; },
        [this](Sid sid,std::uint32_t flags) { CHECK(flags==0);Event("U:"+std::to_string(Get(pool.Slot(sid),36)));if (++updates==2 && input[9]==1) pool.AllocatedBytes(kSource)[34]=7; },
        [](Sid,std::uint32_t) { throw std::logic_error("바리케이드 합성 입력은 저장 단계1입니다"); },
        [](Sid,float,float,std::uint32_t) { throw std::logic_error("바리케이드 합성 입력은 재Pop하지 않습니다"); }}) {
        for (const auto& value:Split(values,',')) input.push_back(Number(value)); //행의 입력만 읽는다.
        CHECK(input.size()==12);
        for (std::uint16_t sid=5;sid<=54;++sid) CHECK(pool.Allocate(2)==Sid{sid}); //실제 번호를 확보한다.
        for (std::uint16_t number=50;number<=54;++number) { //전체 다섯 입력 객체의 보존 바이트도 비교한다.
            auto raw=pool.AllocatedBytes(Sid{number});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x509470);Put(raw,4,0,2);Put(raw,6,0,2);Put(raw,8,0,2);raw[10]=136;raw[11]=0;Put(raw,12,114,2);
            Put(raw,14,std::bit_cast<std::uint32_t>(float(number)+.25f));Put(raw,18,std::bit_cast<std::uint32_t>(float(number)+.5f));raw[33]=1;raw[34]=1;raw[40]=static_cast<std::uint8_t>(number==50 ? input[1] : 0);Put(raw,36,input[2]);Put(raw,42,input[3]);Put(raw,46,input[4]);
        }
        body=std::make_unique<RawFenceRegular>(pool,frames,setter,Hooks());
    }
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //명시 효과를 실제 순서대로 결합한다.
    // 실제 게임 계산을 흉내 내는 기대값 대신 원본 관찰의 같은 명시 하위 입력/변경만 공급한다.
    FenceRegularHooks Hooks() {
        FenceRegularHooks hooks;
        hooks.neighbor=[this](Sid sid,std::uint32_t direction) { CHECK(sid==kSource);Event("N:"+std::to_string(direction));if (input[9]==2 && direction==0) pool.AllocatedBytes(kSource)[34]=6;return Sid{static_cast<std::uint16_t>(input[7]&(1U<<(direction/2)) ? 51+direction/2 : 0)}; };
        hooks.clearMarkers=[this](Sid sid,Sid other) { CHECK(sid==kSource);Event("M:"+std::to_string(other.value));if (++markers==1 && input[9]==3) { Put(pool.AllocatedBytes(Sid{51}),18,std::bit_cast<std::uint32_t>(125.0f));Put(pool.AllocatedBytes(kSource),14,std::bit_cast<std::uint32_t>(120.0f)); } };
        hooks.clearLine=[this](std::array<float,4> points) {
            std::string event="L:";
            for (std::size_t index=0;index<points.size();++index) { if (index) event+=',';event+=std::to_string(std::bit_cast<std::uint32_t>(points[index])); } //현재 네 좌표의 원본float 비트를 같은 순서로 기록한다.
            Event(std::move(event));
        };
        hooks.connections=[this](Sid sid) { CHECK(sid==kSource);Event("C");if (input[9]==5) pool.AllocatedBytes(kSource)[34]=5;if (input[9]==9) Put(pool.AllocatedBytes(kSource),46,input[5]);return ++connections==1 ? input[5] : input[6]; };
        hooks.reserveRegular=[this] { Event("R");const auto success=(input[8]>>reserves)&1U;if (++reserves==1 && input[9]==4) { Put(pool.AllocatedBytes(kSource),46,77);pool.AllocatedBytes(kSource)[34]=7; }return success!=0; };
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { Event("A:"+std::to_string(event)+':'+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        hooks.refreshNeighbors=[this](Sid sid) { CHECK(sid==kSource);Event("F");MutateEffect(); };
        hooks.rebuild=[this](Sid sid) { CHECK(sid==kSource);Event("B");MutateEffect(); };
        hooks.damageable=[this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { CHECK(sid==kSource);Event("D:"+std::to_string(event)+','+std::to_string(count)+','+std::to_string(std::bit_cast<std::uint32_t>(payload)));if (input[9]==8) Put(pool.AllocatedBytes(sid),12,0x8888,2);return payload; };
        return hooks;
    }
    void MutateEffect() { if (input[9]==6) Put(pool.AllocatedBytes(kSource),12,0x7777,2);if (input[9]==7) Put(pool.AllocatedBytes(kSource),36,22); } //호출자가 명시한 외부 효과 후 변경 입력이다.
    std::string Objects() const { std::string value;for (std::uint16_t sid=50;sid<=54;++sid) value+=Hex(pool.Slot(Sid{sid}));return value; } //원본 전체250BYTE와 비교한다.
};
// 원본 fixture의 범위별 모든 결과 비트를 비교한다.
void CompareFenceRegular(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_REGULAR_FIXTURE);std::size_t count{}; //현재 범위의 전수 비교 수다.
    for (const auto& row:fixture.rows) { //독립 관찰의 결과는 테스트가 재계산하지 않는다.
        CHECK(row.size()==5);if (row.size()!=5 || row[0]!=std::string(1,kind)) continue;FenceRegularScene scene(row[1]);
        const auto result=scene.body->Handle(kSource,scene.input[0],scene.input[11],std::bit_cast<float>(scene.input[10]));
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        const bool same=scene.Objects()==row[2] && std::bit_cast<std::uint32_t>(result)==Number(row[3]) && trace==row[4];CHECK(same);++count;
        if (!same) { std::fprintf(stderr,"바리케이드 사건 불일치 %c 입력%s\n실제%s\n기대%s\n",kind,row[1].c_str(),trace.c_str(),row[4].c_str());break; }
    }
    CHECK(count==expected);
}
}
TEST_CASE(FenceRegular_NativeAnimation1305) { CompareFenceRegular('A',1305); } //두 독립 변경/각 할당 실패/현재 재조회의 전체 결과다.
TEST_CASE(FenceRegular_NativeEffects18) { CompareFenceRegular('E',18); } //사건1/2의 실제 하위 위임/반환0이다.
TEST_CASE(FenceRegular_NativeDamageable49) { CompareFenceRegular('D',49); } //bit20과 기본 사건의 위임/비트 보존이다.
// 미복원 반환/하위와 잘못된 부모는 빈 성공으로 대신하지 않고 앞선 프레임 쓰기를 보존한다.
TEST_CASE(FenceRegular_RequiredBoundariesAndPartialWrites) {
    FenceRegularScene scene("0,0,7,1,0,0,0,0,0,0,0,0");auto hooks=scene.Hooks();hooks.connections={};RawFenceRegular missing(scene.pool,scene.frames,scene.setter,std::move(hooks));bool failed{};
    try { missing.Handle(kSource,0,0,0); } catch (const std::logic_error&) { failed=true; }CHECK(failed && Get(scene.pool.Slot(kSource),36)==0);
    failed=false;try { scene.body->Handle(kSource,3,0,0); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
    scene.pool.AllocatedBytes(kSource)[40]=32;CHECK(scene.body->Handle(kSource,3,0,0.25f)==0.25f);
    Put(scene.pool.AllocatedBytes(kSource),0,0x50a748);failed=false;try { scene.body->Handler()(kSource,0,0,0); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
}
