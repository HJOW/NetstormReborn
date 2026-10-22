// Bulf41의 독립 원본314입력과 같은 현재 경로/부모/효과를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBulfFrame.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

namespace {
// 원본과 같은 구분자로 외부 효과의 모든 DWORD를 보존한다.
void BulfEvent(std::string& output,std::string_view name,std::initializer_list<std::uint32_t> values={}) {
    if (!output.empty()) output+=';';output+=name;
    // 각 효과 인자의 낮은 DWORD 전체를 출력한다.
    for (const auto value:values) output+=':'+std::to_string(value);
}
// 원본 관측의 합성8/8 및 실제 Bulf와 같은 보행8/회전24 표를 독립 입력으로 만든다.
RiftTypeFrames BulfFrames(bool originalCounts) {
    std::vector<FrameCode> codes;
    // 실제 TYPE의 여섯 구간 저장 규칙만 독립 입력으로 사용한다.
    for (std::uint8_t side='A';side<='F';++side) {
        // 실제 Bulf의 E/F는 각24프레임이며 보행 A~D는 각8프레임이다.
        for (std::uint8_t frame=0;frame<(originalCounts && side>='E' ? 24 : 8);++frame) codes.push_back({side,0,frame,0});
    }
    return RiftTypeFrames(std::move(codes));
}
}

// 생성자 전체 이미지·프레임 분기·회전 helper·공간/난수/음향 호출을 실제 x86 원문과 비교한다.
TEST_CASE(BulfPath_IndependentNativeBodies) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bulf-path-x86.tsv").string();
    std::size_t rows{};
    // 새 원문 행은 변경하지 않으며 두 x87 정밀도의 결과를 모두 확인한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==9);if (row.size()!=9) continue;
        const auto values=Split(row[0],',');CHECK(values.size()==12);if (values.size()!=12) continue;
        std::array<std::uint32_t,12> a{};
        // 입력 DWORD를 그대로 읽어 NaN/음수0도 보존한다.
        for (std::size_t index=0;index<a.size();++index) a[index]=Number(values[index]);
        // 두 정밀도는 같은 실제 저장값과 효과 순서를 남겨야 한다.
        for (std::size_t index=1;index<=4;++index) CHECK(row[index]==row[index+4]);
        if (!a[0]) {
            const auto checksum=a[2] ? Adler(RawPathRuntime(OriginalEdition::Patch1078,false,static_cast<std::uint8_t>(a[1]),false,false,false,false,true).Bytes()) : Adler(std::vector<std::uint8_t>(0x186f0,static_cast<std::uint8_t>(a[1])));
            CHECK(checksum==Number(row[2]) && row[1]=="N:100080");++rows;continue;
        }
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50};
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(parent).data()),50);
        std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=135;
        Put(raw,14,std::bit_cast<std::uint32_t>(10.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));Put(raw,36,a[3]);
        RawPathRuntime runtime(OriginalEdition::Patch1078,false,0,false,false,false,false,true);
        const std::array<RawPathPoint,1> points{{{0,0}}};const std::array<std::int32_t,1> directions{static_cast<std::int32_t>(a[4])};
        runtime.SetPath(points,directions);runtime.SetAttachment(Sid{},parent);runtime.RestoreStep(0,std::bit_cast<float>(a[1]),a[8]==2);
        std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));frames[135]=BulfFrames(a[11]!=0);
        std::vector<SquidDisplayShape> shapes(188);shapes[135].frames.resize(a[11] ? 80 : 48);
        // 메타 경계가 반환한 동일한 보행 간격을 현재 프레임에 공급한다.
        for (auto& shape:shapes[135].frames) shape.walkStepX=std::bit_cast<float>(a[7]);
        std::string effects;std::uint32_t result{};
        if (a[0]==2) result=static_cast<std::uint32_t>(BulfTurningFrame(frames[135],std::bit_cast<float>(a[2]),a[4]));
        else {
            BulfFrameHooks hooks;
            hooks.immobile=[&] { BulfEvent(effects,"I");return a[8]==1; };
            hooks.halt=[&] { BulfEvent(effects,"H");return a[8]==3; };
            hooks.step=[&] {
                BulfEvent(effects,"S");const auto angle=std::bit_cast<float>(a[2]);runtime.RestoreStep(0,angle,false);
                return RawPathSample{{10.0F+std::bit_cast<float>(a[5]),20.0F+std::bit_cast<float>(a[6])},angle,a[8]==4};
            };
            hooks.arrive=[&] { BulfEvent(effects,"A");runtime.SetFinished(true); };
            hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==parent);BulfEvent(effects,"U",{flags}); };
            hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) {
                CHECK(sid==parent);BulfEvent(effects,"P",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),flags});
                Put(raw,14,std::bit_cast<std::uint32_t>(a[10] ? 33.0F : point.x));Put(raw,18,std::bit_cast<std::uint32_t>(a[10] ? 44.0F : point.y));
            };
            hooks.randomInt=[&](std::int32_t low,std::int32_t high) { BulfEvent(effects,"R",{static_cast<std::uint32_t>(low),static_cast<std::uint32_t>(high)});return static_cast<std::int32_t>(a[9]); };
            hooks.sound=[&](float x,float y,std::string_view sound) { BulfEvent(effects,"W",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});effects+=':'+std::string(sound)+":0"; };
            RunBulfFrame(pool,parent,runtime,frames,shapes,hooks,std::make_shared<bool>(true));
        }
        if ((effects.empty() ? "-" : effects)!=row[1] || Adler(runtime.Bytes())!=Number(row[2]) || Hex(raw)!=row[3] || result!=Number(row[4])) {
            std::cout<<"Bulf 원문 행"<<rows<<" 입력"<<row[0]<<" 효과"<<effects<<"/원본"<<row[1]<<"/체크섬"<<Adler(runtime.Bytes())<<"/원본"<<row[2]<<"/반환"<<result<<"/원본"<<row[4]<<'\n';
            CHECK(false);
        }
        ++rows;
    }
    CHECK(rows==314);
}
