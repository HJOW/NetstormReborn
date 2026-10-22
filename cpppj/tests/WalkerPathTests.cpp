// Walker38의 생성자 전체 이미지/프레임/예약/공유 counter를 독립 x86 원문과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawWalkerFrame.h"
#include <filesystem>
#include <cmath>
#include <iostream>
#include <limits>
using namespace netstorm::test::rawscene;

namespace {
// 원본 외부 효과의 DWORD 순서와 동일한 표기를 만든다.
void WalkerEvent(std::string& output,std::string_view name,std::initializer_list<std::uint32_t> values={}) {
    if (!output.empty()) output+=';';output+=name;
    // 값은 부호 없는 네 바이트 전체를 보존한다.
    for (const auto value:values) output+=':'+std::to_string(value);
}
// 실제 TYPE helper가 읽는 A/B 구간을 독립 명시 메타 입력으로 만든다.
RiftTypeFrames WalkerFrames(bool alternate) {
    std::vector<FrameCode> codes;
    const auto append=[&](std::uint8_t side,std::uint8_t count) {
        // 코드의 물리 순서가 Run.first/count가 된다.
        for (std::uint8_t index=0;index<count;++index) codes.push_back({side,0,index,0});
    }; // 합성 구간에는 보행 이전/사이의 다른 side도 포함한다.
    if (alternate) { append('C',3);append('A',5);append('D',3);append('B',7); }
    else { append('A',8);append('B',8); }
    return RiftTypeFrames(std::move(codes));
}
// 예약 경계는 now10초의 인접 double과 unordered도 포함한다.
double WalkerTime(std::uint32_t code) {
    const std::array<double,7> values{0,10,std::nextafter(10.0,11.0),std::numeric_limits<double>::quiet_NaN(),
        std::nextafter(10.0,9.0),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};
    return values.at(code);
}
}

// 전체100080바이트·부모50바이트·효과 순서·공유 전역을 두 정밀도와 비교한다.
TEST_CASE(WalkerPath_IndependentNativeBodies) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/walker-path-x86.tsv").string();std::size_t rows{};
    // 새 동결 원문은 변경하지 않고 모든 입력을 실행한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==9);if (row.size()!=9) continue;
        const auto input=Split(row[0],',');CHECK(input.size()==12);if (input.size()!=12) continue;
        std::array<std::uint32_t,12> a{};
        // NaN/음수0/overflow counter도 원래 DWORD로 읽는다.
        for (std::size_t index=0;index<a.size();++index) a[index]=Number(input[index]);
        // x87 53/64비트 정밀도에서 저장 결과와 효과가 같아야 한다.
        for (std::size_t index=1;index<=4;++index) CHECK(row[index]==row[index+4]);
        if (!a[0]) {
            const auto checksum=a[2] ? Adler(RawPathRuntime(OriginalEdition::Patch1078,false,static_cast<std::uint8_t>(a[1]),false,false,false,false,false,true).Bytes()) : Adler(std::vector<std::uint8_t>(0x186f0,static_cast<std::uint8_t>(a[1])));
            CHECK(checksum==Number(row[2]) && row[1]=="N:100080");++rows;continue;
        }
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50};
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(parent).data()),50);
        std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(a[3]);
        Put(raw,14,std::bit_cast<std::uint32_t>(10.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));Put(raw,36,a[5]);
        RawPathRuntime runtime(OriginalEdition::Patch1078,false,0,false,false,false,false,false,true);
        const std::array<RawPathPoint,1> points{{{0,0}}};const std::array<std::int32_t,1> directions{0};
        runtime.SetPath(points,directions);runtime.SetAttachment(Sid{},parent);runtime.RestoreStep(0,0,a[9]==2);runtime.SetNextFrame(WalkerTime(a[10]));
        std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));frames[156]=WalkerFrames(a[11]!=0);
        std::string effects;auto counter=a[6];WalkerFrameHooks hooks;
        hooks.immobile=[&] { WalkerEvent(effects,"I");return a[9]==1; };
        hooks.halt=[&] { WalkerEvent(effects,"H");return a[9]==3; };
        hooks.step=[&] {
            WalkerEvent(effects,"S");const auto angle=std::bit_cast<float>(a[4]);
            // 실제 Step 경계가 쓴 angle DWORD를 직접 이관하여 유한값 전용 RestoreStep API와 구별한다.
            auto image=std::span<std::uint8_t>(const_cast<std::uint8_t*>(runtime.Bytes().data()),runtime.Bytes().size());
            Put(image,RawPathRuntime::Layout(OriginalEdition::Patch1078).angle,a[4]);
            return RawPathSample{{10.0F+std::bit_cast<float>(a[7]),20.0F+std::bit_cast<float>(a[8])},angle,a[9]==4};
        };
        hooks.groundStop=[&](RawPathPoint point) { WalkerEvent(effects,"G",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)});return a[9]==5; };
        hooks.arrive=[&] { WalkerEvent(effects,"A");runtime.SetFinished(true); };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==parent);WalkerEvent(effects,"U",{flags}); };
        hooks.pop=[&](Sid sid,RawPathPoint point,std::uint32_t flags) {
            CHECK(sid==parent);WalkerEvent(effects,"P",{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y),flags});
            Put(raw,14,std::bit_cast<std::uint32_t>(point.x));Put(raw,18,std::bit_cast<std::uint32_t>(point.y));
        };
        RunWalkerFrame(pool,parent,runtime,10,frames,137,156,counter,hooks,std::make_shared<bool>(true));
        if ((effects.empty() ? "-" : effects)!=row[1] || Adler(runtime.Bytes())!=Number(row[2]) || Hex(raw)!=row[3] || counter!=Number(row[4])) {
            std::cout<<"Walker 원문 행"<<rows<<" input"<<row[0]<<" effects "<<effects<<"/native "<<row[1]<<" adler "<<Adler(runtime.Bytes())<<"/native "<<row[2]<<" counter "<<counter<<"/native "<<row[4]<<'\n';CHECK(false);
        }
        ++rows;
    }
    CHECK(rows==348);
}

// 큰 하위가 현재 경로를 삭제한 뒤에는 부모나 다음 공간 효과를 읽지 않는다.
TEST_CASE(WalkerPath_DeletionBoundaries) {
    // Halt/Step/지면/Unpop 각각에서 삭제 통지를 발생시킨다.
    for (unsigned stage=0;stage<4;++stage) {
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50};
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(parent).data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=137;
        RawPathRuntime runtime(OriginalEdition::Patch1078,false,0,false,false,false,false,false,true);
        std::uint32_t counter{};const auto watch=std::make_shared<bool>(true);std::array<unsigned,6> calls{};WalkerFrameHooks hooks;
        hooks.immobile=[] { return false; };
        hooks.halt=[&] { ++calls[0];if (stage==0) *watch=false;return false; };
        hooks.step=[&] { ++calls[1];if (stage==1) *watch=false;return RawPathSample{{10,20},0,false}; };
        hooks.groundStop=[&](RawPathPoint) { ++calls[2];if (stage==2) *watch=false;return false; };
        hooks.arrive=[&] { ++calls[3]; };
        hooks.unpop=[&](Sid,std::uint32_t) { ++calls[4];if (stage==3) *watch=false; };
        hooks.pop=[&](Sid,RawPathPoint,std::uint32_t) { ++calls[5]; };
        RunWalkerFrame(pool,parent,runtime,10,{},137,156,counter,hooks,watch);
        CHECK(!*watch && calls[5]==0 && calls[3]==0 && calls[0]==1);
        CHECK(calls[1]==(stage>=1 ? 1U : 0U) && calls[2]==(stage>=2 ? 1U : 0U) && calls[4]==(stage==3 ? 1U : 0U));
    }
}
