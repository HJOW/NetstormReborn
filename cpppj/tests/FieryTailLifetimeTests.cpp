// 실제 PE 생성·등록·색 초기화·삭제의 의미 필드와 사건 순서를 대조한다.
#include "TestSupport.h"
#include "o/RawFieryTailLifetime.h"
#include "o/ParticleGradient.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace netstorm::o;
namespace {
// 기본/비교 정밀도에 각각 존재하는 독립 입력 수다.
constexpr std::size_t kLifetimeCases=31;
// 빈 열도 보존하며 TSV와 내부 인수 목록을 읽는다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t start=0;
    // 마지막 열까지 한 번씩 추가한다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; }
}
// 두16진 문자를 바이트로 바꾸며 원본 비트열을 보존한다.
std::vector<std::uint8_t> Hex(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("Fiery Tail 수명 hex 폭 오류");
    std::vector<std::uint8_t> result;
    // ASCII16진수 두 자리씩 입력 바이트를 복원한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// DWORD/QWORD를 호스트 구조체 배치와 무관하게 little endian으로 기록한다.
void Put(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    // 지정 폭만 기록해 signed 필드의 상위 확장을 막는다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
// float 전체 비트를 허용 오차 없이 보존한다.
void PutFloat(std::vector<std::uint8_t>& bytes,float value) { Put(bytes,std::bit_cast<std::uint32_t>(value)); }
// 원본은 반납 후 capacity를 덮어쓰지 않는다. 호스트 벡터와 별도인 할당 시점의 용량을 받는다.
std::vector<std::uint8_t> Fields(const FieryTailFrameFields& fields,std::size_t allocatedCapacity) {
    std::vector<std::uint8_t> result;const auto& reg=fields.registration;
    Put(result,static_cast<std::uint32_t>(reg.form));Put(result,static_cast<std::uint32_t>(reg.parent));
    PutFloat(result,reg.left);PutFloat(result,reg.top);PutFloat(result,reg.right);PutFloat(result,reg.bottom);
    Put(result,static_cast<std::uint32_t>(reg.slot));Put(result,static_cast<std::uint32_t>(reg.zOrder));Put(result,allocatedCapacity);Put(result,static_cast<std::uint32_t>(fields.total));
    Put(result,std::bit_cast<std::uint64_t>(fields.lastTime),8);Put(result,std::bit_cast<std::uint64_t>(fields.elapsed),8);
    PutFloat(result,fields.delta);PutFloat(result,fields.scaledDelta);Put(result,fields.first ? 1 : 0);Put(result,fields.palette);
    Put(result,static_cast<std::uint32_t>(fields.maximumLife));PutFloat(result,fields.previousX);PutFloat(result,fields.previousY);Put(result,static_cast<std::uint32_t>(fields.offset));return result;
}
// 사용 수가0이어도 원본 전체 용량의24바이트 입자와 비교한다.
std::vector<std::uint8_t> Particles(const FieryTailFrameFields& fields) {
    std::vector<std::uint8_t> result;
    // 모든 미사용 필드와 꼬리 슬롯도 직렬화한다.
    for (const auto& particle:fields.particles) {
        PutFloat(result,particle.x);PutFloat(result,particle.y);PutFloat(result,particle.velocityX);PutFloat(result,particle.velocityY);PutFloat(result,particle.life);Put(result,particle.spare);
    }
    return result;
}
// 현재 표시 슬롯의 빈 칸과 기존 토큰도 순서대로 보존한다.
std::string Slots(const RawRenderRegistry& registry) {
    std::string result;
    // 원본 빈 칸0을 건너뛰지 않는다.
    for (auto value:registry.Slots()) { if (!result.empty()) result+=',';result+=std::to_string(value); }
    return result.empty() ? "-" : result;
}
}
// 기본53비트31행의 필드·120입자·공유 색 표·등록/반납 순서를 원본과 대조한다.
TEST_CASE(FieryTailLifetime_Native027f) {
    std::ifstream input(NETSTORM_FIERYTAIL_LIFETIME_FIXTURE);if (!input) throw std::runtime_error("Fiery Tail 수명 fixture 없음");
    std::string line;std::size_t count=0,extended=0;
    // 비교 정밀도 행은 보존하되 기본 정밀도 완료 수에 중복 가산하지 않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=12) throw std::runtime_error("Fiery Tail 수명 열 수 오류");
        if (std::stoul(row[1])==0x037f) { ++extended;continue; }CHECK(std::stoul(row[1])==0x027f);
        const auto args=Split(row[2],',');if (args.size()!=8) throw std::runtime_error("Fiery Tail 수명 인수 폭 오류");
        const bool warm=std::stoi(args[1])!=0;const auto capacity=std::stoi(args[2]),hole=std::stoi(args[3]),parent=std::stoi(args[4]),depth=std::stoi(args[5]),mutate=std::stoi(args[6]);
        const auto flags=static_cast<std::uint32_t>(std::stoul(args[7]));SidPool pool(OriginalEdition::Patch1078,32768,false);
        auto* zero=const_cast<std::uint8_t*>(pool.Slot(Sid{}).data());zero[35]=255;
        auto* original=const_cast<std::uint8_t*>(pool.Slot(Sid{50}).data());original[35]=static_cast<std::uint8_t>(depth);
        auto* replaced=const_cast<std::uint8_t*>(pool.Slot(Sid{51}).data());replaced[35]=127;
        std::vector<std::uint32_t> initial;
        // 초기 목록만 입력으로 구성한다. 검색/확장은 제품 몸체가 수행한다.
        for (int index=0;index<capacity;++index) initial.push_back(index==hole ? 0U : static_cast<std::uint32_t>(100+index));
        std::string events;
        // 실제 요청을 시간 순서대로 이어 붙인다.
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; };
        RenderRegistryHooks registration;
        registration.reserve=[&](std::uint32_t bytes) { event("N:"+std::to_string(bytes));return true; };
        registration.releaseArray=[&] { event("F:R"); };
        registration.attach=[&](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t attachFlags) {
            event("B:"+std::to_string(type)+":"+std::to_string(target.value)+":0:"+std::to_string(attachFlags)+":"+std::to_string(fields.slot)+":"+std::to_string(fields.zOrder));
            fields.form=7;fields.parent=mutate==2 ? 51 : target.value;if (mutate==1) original[35]=128;
        };
        RawRenderRegistry registry(pool,std::move(registration),std::move(initial));FieryTailLifetimeState state{warm ? 1U : 0U};
        std::vector<std::uint32_t> colors;
        // 이미 존재하는11색은 원본 warm 실험과 같은 외부 입력이다.
        if (warm) for (std::uint32_t color=31;color<42;++color) colors.push_back(color);
        FieryTailLifetimeHooks hooks;hooks.reserve=[&](std::uint32_t bytes) { event("N:"+std::to_string(bytes));return true; };
        hooks.createPalette=[] { return 1U; };
        const auto rgbx=Hex(row[3]);if (rgbx.size()!=1024) throw std::runtime_error("Fiery Tail 입력 팔레트 폭 오류");
        std::array<std::uint32_t,kParticlePaletteColors> palette{};
        // 색 기초 입력의 RGBX DWORD를 원본 순서로 복원한다.
        for (std::size_t index=0;index<palette.size();++index) {
            const auto base=index*4;palette[index]=std::uint32_t(rgbx[base])|(std::uint32_t(rgbx[base+1])<<8)|(std::uint32_t(rgbx[base+2])<<16)|(std::uint32_t(rgbx[base+3])<<24);
        }
        hooks.buildPalette=[&](std::uint32_t token) { CHECK(token==1);event("N:44");colors=BuildFieryTailPalette(palette); };
        RawFieryTailLifetime lifetime(pool,registry,state,std::move(hooks));FieryTailFrameFields fields;
        fields.delta=fields.scaledDelta=std::bit_cast<float>(0xa5a5a5a5U);
        lifetime.Initialize(fields,1,Sid{static_cast<std::uint16_t>(parent)});const auto allocated=fields.particles.size();
        CHECK(Fields(fields,allocated)==Hex(row[4]));CHECK(Particles(fields)==Hex(row[5]));CHECK(Slots(registry)==row[7]);CHECK(events==row[8]);CHECK(state.palette==1);
        std::vector<std::uint8_t> serializedColors;
        // 공유 색 표11개 전체를 비교하며 인접 DWORD를 추가하지 않는다.
        for (auto color:colors) Put(serializedColors,color);
        CHECK(serializedColors==Hex(row[6]));events.clear();
        lifetime.Destroy(fields,flags,[&](FieryTailRelease released) {
            if (released==FieryTailRelease::Particles) { CHECK(registry.Slots()[static_cast<std::size_t>(fields.registration.slot)]==1);event("F:P"); }
            else { CHECK(registry.Slots()[static_cast<std::size_t>(fields.registration.slot)]==0);event("F:O"); }
        });
        CHECK(fields.particles.empty());CHECK(Fields(fields,allocated)==Hex(row[9]));CHECK(Slots(registry)==row[10]);CHECK(events==row[11]);CHECK(state.palette==1);++count;
    }
    CHECK(count==kLifetimeCases);CHECK(extended==kLifetimeCases);
}
