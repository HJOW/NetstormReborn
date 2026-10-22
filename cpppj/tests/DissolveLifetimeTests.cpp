// 분해의 원본100BYTE/전체 입자/게임 난수/목록/요청과 실제 삭제 순서를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawDissolveLifetime.h"

using namespace netstorm::test::rawscene;
namespace {
// 독립 원문 hex만 바이트로 복원한다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text=="-") return {};std::vector<std::uint8_t> bytes;
    // 두 문자마다 기대값 바이트 하나를 읽는다.
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return bytes;
}
// little endian DWORD/QWORD를 원래 폭으로 저장한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    // 낮은 바이트부터 주어진 폭만 쓴다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
// 원문100바이트의 의미 상태다. bitmap 주소만 비영 토큰으로 정규화한다.
std::vector<std::uint8_t> Fields(const DissolveFrameFields& full) {
    const auto& fields=full.common;const auto& rect=fields.registration;std::vector<std::uint8_t> result;Append(result,static_cast<std::uint32_t>(rect.form));Append(result,static_cast<std::uint32_t>(rect.parent));
    // 실제 표시 영역 네 float를 먼저 저장한다.
    for (const auto value:{rect.left,rect.top,rect.right,rect.bottom}) Append(result,std::bit_cast<std::uint32_t>(value));
    Append(result,static_cast<std::uint32_t>(rect.slot));Append(result,static_cast<std::uint32_t>(rect.zOrder));Append(result,fields.particles.size());Append(result,static_cast<std::uint32_t>(fields.total));Append(result,std::bit_cast<std::uint64_t>(fields.lastTime),8);Append(result,std::bit_cast<std::uint64_t>(fields.elapsed),8);Append(result,std::bit_cast<std::uint32_t>(fields.delta));Append(result,std::bit_cast<std::uint32_t>(fields.scaledDelta));Append(result,fields.first ? 1 : 0);Append(result,fields.image.pixels.empty() ? 0 : 1);
    // 캡처 치수/기준점은 signed DWORD 비트 그대로 보존한다.
    for (const auto value:{fields.image.width,fields.image.height,fields.image.hotspotX,fields.image.hotspotY}) Append(result,static_cast<std::uint32_t>(value));
    Append(result,0);Append(result,full.opaqueCount);Append(result,full.clockStarted ? 1 : 0);return result;
}
// 전체 용량의 미사용0 입자도 원문과 비교한다.
std::vector<std::uint8_t> Particles(const DissolveFrameFields& full) {
    const auto& fields=full.common;std::vector<std::uint8_t> result;
    //24BYTE 필드 순서와 색 offset+1 토큰을 유지한다.
    for (const auto& particle:fields.particles) {
        // 실제 다섯 float의 비트다.
        for (const auto value:{particle.x,particle.y,particle.velocityX,particle.velocityY,particle.life}) Append(result,std::bit_cast<std::uint32_t>(value));
        Append(result,particle.pixel);
    }
    return result;
}
// 현재 원본 목록의 빈칸/토큰을 그대로 직렬화한다.
std::string Slots(const RawRenderRegistry& registry) {
    std::string result;
    // 모든 빈칸과 이전 항목도 출력한다.
    for (const auto token:registry.Slots()) { if (!result.empty()) result+=',';result+=std::to_string(token); }
    return result.empty() ? "-" : result;
}
// 첫 차이를 행/항목/바이트로 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin())) return;
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin(),expected.end());const auto offset=mismatch.first-actual.begin();
    const auto text="분해 수명 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(offset)+" 폭 실제"+std::to_string(actual.size())+" 원본"+std::to_string(expected.size());netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
}
}
// 실제 wrapper/분해 생성자/동일 Prepare와 반납의 고유288개를 비교한다.
TEST_CASE(DissolveLifetime_NativeCreationAndRelease) {
    SidPool pool(OriginalEdition::Patch1078,32768,false);
    // 원본 입력의 source50/dummy60/대체부모61까지 확보한다.
    for (std::uint16_t sid=5;sid<=62;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::size_t count=0;
    // 기대 결과를 공급하지 않고 각 명시 입력으로만 실행한다.
    for (const auto& row:LoadFixture(NETSTORM_DISSOLVE_LIFETIME_FIXTURE).rows) {
        if (row.size()!=16) throw std::runtime_error("분해 수명 fixture 열 수 오류");const auto orientation=static_cast<std::int32_t>(Number(row[0]));const auto capacity=Number(row[1]),mutation=Number(row[4]),flags=Number(row[5]);const auto hole=std::stoi(row[2]);
        auto zero=pool.Slot({});const_cast<std::uint8_t*>(zero.data())[35]=255;
        // 실제 현재 source/두 부모의 raw 입력을 원본과 똑같이 설치한다.
        for (const auto sid:{50,60,61,62}) {
            auto raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12900000);raw[10]=sid==50 ? 144 : sid==62 ? 145 : 110;raw[35]=sid==50 ? static_cast<std::uint8_t>(Number(row[3])) : sid==61 ? 127 : 0;
            Put(raw,14,std::bit_cast<std::uint32_t>(sid==50 ? 2.125F : sid==60 ? 9.75F : sid==61 ? -2.75F : 20.5F));Put(raw,18,std::bit_cast<std::uint32_t>(sid==50 ? 3.25F : sid==60 ? 8.5F : sid==61 ? 7.125F : 30.25F));
        }
        std::vector<std::uint32_t> slots(capacity);
        // 토큰1은 생성할 객체이며 기존100+ 항목은 그대로 유지한다.
        for (std::size_t index=0;index<slots.size();++index) slots[index]=static_cast<int>(index)==hole ? 0 : 100+static_cast<std::uint32_t>(index);
        std::vector<std::uint8_t> events;const auto event=[&](char name,std::initializer_list<std::uint32_t> values) {
            events.push_back(static_cast<std::uint8_t>(name));
            // 원문 요청의 인수를DWORD 폭으로 저장한다.
            for (const auto value:values) Append(events,value);
        };
        RawRenderRegistry registry(pool,{[&](std::uint32_t bytes) { event('N',{bytes});return true; },[&] { event('F',{1}); },[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("parent0 등록의 예상 밖 부착"); }},std::move(slots));
        GameRandom game(0x12345678);ShrapnelFrameState state;state.now=1.125;DissolveFrameFields fields;fields.common.delta=fields.common.scaledDelta=std::bit_cast<float>(0xa5a5a5a5U);ShrapnelFrameHooks frameHooks;
        frameHooks.reserve=[&](std::uint32_t bytes) { event('N',{bytes});return true; };frameHooks.release=[&](bool scratch) { event('F',{scratch ? 5U : 3U}); };
        frameHooks.invalidate=[](FieryTailDirtyRect) { throw std::logic_error("준비의 예상 밖 dirty"); };frameHooks.pixel=[](ShrapnelPixel) { throw std::logic_error("준비의 예상 밖 픽셀"); };frameHooks.removeForm=[] { throw std::logic_error("준비의 예상 밖 소진"); };
        RawDissolveFrame frame(pool,game,state,std::move(frameHooks));DissolveLifetimeHooks hooks;
        hooks.reserve=[&](std::uint32_t bytes) { event('N',{bytes});return true; };
        hooks.create=[&](std::uint32_t type,std::uint32_t value) {
            event('C',{type,value});CHECK(type==110 && value==2);
            if (mutation==1) { auto raw=pool.AllocatedBytes(Sid{50});Put(raw,14,std::bit_cast<std::uint32_t>(-7.75F));Put(raw,18,std::bit_cast<std::uint32_t>(8.25F));raw[35]=128; }return Sid{60};
        };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t value) { event('P',{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),value});auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y)); };
        hooks.attach=[&](RenderProcessFields& rect,std::uint32_t type,Sid dummy,std::uint32_t value) { event('A',{type,dummy.value,0,value,static_cast<std::uint32_t>(rect.slot),static_cast<std::uint32_t>(rect.zOrder)});rect.form=80;rect.parent=mutation==2 ? 61 : dummy.value; };
        hooks.depend=[&](Sid form,Sid dummy) { event('D',{form.value,dummy.value}); };
        hooks.capture=[&](Sid source) {
            event('I',{source.value});ConstructionVisualImage image{3,4,1,3,{}};
            // 실제 캡처가 반환한 명시 bitmap 입력이며 결과 입자는 계산하지 않는다.
            for (std::uint32_t index=0;index<12;++index) image.pixels.push_back(static_cast<std::uint8_t>(Number(row[6]) ? (index*13+7)%255 : 255));
            if (mutation==3) pool.AllocatedBytes(Sid{50})[35]=255;return image;
        };
        hooks.prepare=[&](DissolveFrameFields& current,Sid first,Sid second,std::int32_t xOriented) { frame.Initialize(current,first,second,xOriented); };
        RawDissolveLifetime lifetime(pool,registry,std::move(hooks));CHECK(lifetime.ReserveObject());Sid dummy;lifetime.Initialize(fields,1,Sid{50},Sid{61},Sid{62},orientation,dummy);CHECK(dummy.value==60);
        Compare(Fields(fields),Decode(row[7]),count,"생성100");Compare(Particles(fields),Decode(row[8]),count,"용량 전체");CHECK(game.State()==Number(row[9]) && Number(row[10])==1);CHECK(Slots(registry)==row[11]);Compare(events,Decode(row[12]),count,"생성 요청");
        events.clear();lifetime.Destroy(fields,flags,[&](ShrapnelRelease value) { event('F',{value==ShrapnelRelease::Source ? 2U : value==ShrapnelRelease::Particles ? 3U : 4U});CHECK(registry.Slots()[static_cast<std::size_t>(fields.common.registration.slot)]==(value==ShrapnelRelease::Object ? 0U : 1U)); });
        const auto actual=Fields(fields),expected=Decode(row[13]);
        // 삭제 뒤 원본은 포인터/용량을 그대로 두지만 C++ vector는 실제 반납하므로 그 두 저장소 표현만 제외한다.
        Compare(std::span(actual).first(32),std::span(expected).first(32),count,"삭제 등록");Compare(std::span(actual).subspan(36,32),std::span(expected).subspan(36,32),count,"삭제 시간/꼬리");Compare(std::span(actual).subspan(72),std::span(expected).subspan(72),count,"삭제 메타데이터");CHECK(Slots(registry)==row[14]);Compare(events,Decode(row[15]),count,"반납 요청");CHECK(fields.common.particles.empty() && fields.common.image.pixels.empty());++count;
    }
    CHECK(count==288);
}
