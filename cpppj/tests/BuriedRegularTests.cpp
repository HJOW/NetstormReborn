// 원본 오벨리스크 Handle의 반환/전체 raw/난수와 실제 프레임 지정의 표시 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBuriedRegular.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 구간 표 생성기를 사용해 A8/B가변/C나머지인48프레임 입력을 만든다.
RiftTypeFrames BuriedCodes(std::uint32_t count) {
    std::vector<FrameCode> codes; // 입력한 구간별 코드이며 결과 프레임은 계산하지 않는다.
    for (std::uint32_t index=0;index<48;++index) codes.push_back({static_cast<std::uint8_t>(index<8 ? 'A' : index<8+count ? 'B' : 'C'),'A',static_cast<std::uint8_t>(index),0}); // 원본TYPE 전처리와 같은 실제 구간을 만든다.
    return RiftTypeFrames(std::move(codes));
}
// 원본 입력과 동일한 단일 풀/프레임 지정기/난수의 검사용 조립체다.
struct BuriedScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 부모50과 원본 고정 예측 free list를 모두 포함한 배치다.
    std::vector<RiftTypeRecord> types{188}; // 프레임 지정기의 현재 원본TYPE 입력이다.
    std::vector<RiftTypeFrames> frames; // 실제 A/B 프레임 구간 표다.
    GameRandom random; // 현재 공유 난수다.
    std::vector<std::uint32_t> input; // 원본 관찰의 열10개를 그대로 보관한다.
    std::string trace; // 모든 표시 효과의 호출 당시 현재 프레임이다.
    std::size_t updates{}; // Frame 뒤 명시 변경 입력의 호출 횟수다.
    SquidFrame setter; // 같은 풀에서 StoreLevel/Frame/표시를 실제 수행한다.
    std::unique_ptr<RawBuriedRegular> body; // 조립체의 참조를 사용하는 실제 처리기다.
    // 원본 입력만 공급하고 Handle의 분기/반환/난수 결과는 계산하지 않는다.
    explicit BuriedScene(const std::string& values):frames(188,BuriedCodes(8)),setter(pool,types,{
        [](Sid,std::int32_t) { return std::array<float,2>{1,1}; },
        [this](Sid sid,std::uint32_t flags) {
            CHECK(flags==0);if (!trace.empty()) trace+=';';trace+="U:"+std::to_string(Get(pool.Slot(sid),36));
            if (++updates==2 && input[9]) { random.SetState(0x87654321);Put(pool.AllocatedBytes(sid),12,0xbeef,2); }
        },[](Sid,std::uint32_t) { throw std::logic_error("오벨리스크 합성 입력은 저장 단계1입니다"); },
        [](Sid,float,float,std::uint32_t) { throw std::logic_error("오벨리스크 합성 입력은 재Pop하지 않습니다"); }}) {
        for (const auto& value:Split(values,',')) input.push_back(Number(value)); // 원본 관찰에서 제공한 입력 비트만 읽는다.
        CHECK(input.size()==10);
        for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid}); // 실제 번호50까지 할당한다.
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});
        Put(raw,0,0x50a748);Put(raw,4,0,2);Put(raw,6,0,2);Put(raw,8,0,2);raw[10]=112;raw[11]=0;Put(raw,12,input[8],2);
        Put(raw,14,std::bit_cast<std::uint32_t>(50.25f));Put(raw,18,std::bit_cast<std::uint32_t>(52.5f));raw[33]=1;Put(raw,36,input[2]);
        frames[112]=BuriedCodes(input[5]);random.SetState(input[6]);body=std::make_unique<RawBuriedRegular>(pool,frames,setter,random);
    }
};
// 사건0과 기본 사건은 독립 검사에서 같은 fixture의 전수 결과를 읽는다.
void CompareBuried(bool animated,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_BURIED_REGULAR_FIXTURE);std::size_t count{}; // 실제 비교한 고유 입력 수다.
    for (const auto& row:fixture.rows) { // 결과를 재계산하지 않고 원본에서 기록한 비트와 비교한다.
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto event=Number(Split(row[0],',')[0]);if ((event==0)!=animated) continue;
        BuriedScene scene(row[0]);const auto result=scene.body->Handle(kSource,scene.input[0],scene.input[1],std::bit_cast<float>(scene.input[7]));
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        const bool same=Hex(scene.pool.Slot(kSource))==row[1] && std::bit_cast<std::uint32_t>(result)==Number(row[2]) && scene.random.State()==Number(row[3]) && trace==row[4];
        CHECK(same);++count;if (!same) { std::fprintf(stderr,"오벨리스크 원본 불일치 입력%s 실제%s 기대%s\n",row[0].c_str(),trace.c_str(),row[4].c_str());break; }
    }
    CHECK(count==expected);
}
}
TEST_CASE(BuriedRegular_NativeAnimation288) { CompareBuried(true,288); } // signed 횟수/현재 난수/실제 표시 변경의 전체 결과다.
TEST_CASE(BuriedRegular_NativeBase96) { CompareBuried(false,96); } // QA 범위와 signed count에 무관한 기본 float 반환이다.
// 실제 가상 표/같은 풀/판본을 검증하고 다른 부모의 누락을 성공으로 처리하지 않는다.
TEST_CASE(BuriedRegular_RejectsForeignBindingsAndMissingParent) {
    BuriedScene scene("0,0,0,0,8,8,0,0,114,0");CHECK(scene.body->Handles(kSource));
    auto raw=scene.pool.AllocatedBytes(kSource);Put(raw,0,0x50f540);bool failed{};
    try { scene.body->Handle(kSource,0,0,0); } catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
    failed=false;try { scene.body->Handler()(kSource,0,0,0); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
    CHECK(scene.body->Handler([](Sid,std::uint32_t,std::uint32_t,float payload) { return payload; })(kSource,9,0,3.0f)==3.0f);
    BuriedScene other("0,0,0,0,8,8,0,0,114,0");failed=false;
    try { RawBuriedRegular invalid(scene.pool,scene.frames,other.setter,scene.random); } catch (const std::invalid_argument&) { failed=true; }CHECK(failed);
}
