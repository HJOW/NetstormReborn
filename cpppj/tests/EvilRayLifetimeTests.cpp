// 실제 PE의 생성/중단/소멸과 색 결과를 게임 없이 대조한다.
#include "TestSupport.h"
#include "o/RawEvilRayLifetime.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace { // fixture 판독과 원문 의미 필드 직렬화만 이 검사 파일에 한정한다.
std::vector<std::string> Split(std::string_view text,char delimiter) { // 마지막 빈 열도 보존해 고정 폭 입력을 읽는다.
    std::vector<std::string> result;std::size_t start=0; // 현재 열의 시작과 이미 읽은 열이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 모든 구분자를 한 번씩 소비한다.
}
std::vector<std::uint8_t> Hex(std::string_view text) { // 원본 전체 비트를 허용 오차 없이 읽는다.
    if (text.size()%2) throw std::runtime_error("EvilRay hex 폭 오류");std::vector<std::uint8_t> result; // 읽은 BYTE를 원본 순서로 보관한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 두 ASCII 문자마다 한 BYTE다.
    return result;
}
void Put(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) { // 호스트 구조체 배치와 무관한 little endian 직렬화다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8))); // 원문 폭만 추가한다.
}
std::vector<std::uint8_t> Fields(const EvilRayFields& fields,std::size_t capacity) { // 포인터/미작성 패딩을 제외한 원문80BYTE 의미 필드다.
    std::vector<std::uint8_t> result;const auto& reg=fields.registration; // 현재 등록을 복사하지 않고 직렬화한다.
    Put(result,static_cast<std::uint32_t>(reg.form));Put(result,static_cast<std::uint32_t>(reg.parent));
    for (const auto value:{reg.left,reg.top,reg.right,reg.bottom}) Put(result,std::bit_cast<std::uint32_t>(value)); // 네 float의 전체 비트다.
    Put(result,static_cast<std::uint32_t>(reg.slot));Put(result,static_cast<std::uint32_t>(reg.zOrder));Put(result,capacity);Put(result,static_cast<std::uint32_t>(fields.total));Put(result,std::bit_cast<std::uint64_t>(fields.lastTime),8);Put(result,std::bit_cast<std::uint64_t>(fields.elapsed),8);Put(result,std::bit_cast<std::uint32_t>(fields.delta));Put(result,std::bit_cast<std::uint32_t>(fields.scaledDelta));Put(result,fields.first ? 1 : 0);Put(result,std::bit_cast<std::uint32_t>(fields.nextEmission));Put(result,fields.source);Put(result,fields.stopping);return result;
}
std::string Slots(const RawRenderRegistry& registry) { // 빈 슬롯과 기존 항목의 원문 순서를 유지한다.
    std::string result; // 쉼표로 연결한 현재 토큰이다.
    for (const auto token:registry.Slots()) { if (!result.empty()) result+=',';result+=std::to_string(token); } // 구멍0을 압축하지 않는다.
    return result.empty() ? "-" : result;
}
std::ifstream Fixture() { return std::ifstream(std::filesystem::path(__FILE__).parent_path()/"fixtures/evilray-lifetime-x86.tsv"); } // 같은 새 원문 파일만 읽고 수정하지 않는다.
}
TEST_CASE(EvilRayLifetime_NativeCtorPaletteAndDestruction) { // 기본53비트12행을 원문 필드/3색/등록/반납과 비교한다. 비교 정밀도12행은 별도로 센다.
    auto input=Fixture();CHECK(input.good());std::string line;std::size_t count=0,extended=0; // 원문 행과 판본별 실제 재생 수다.
    while (std::getline(input,line)) { // 생성 행만 원문 입력 그대로 재생한다.
        if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');if (row[0]!="create") continue;CHECK(row.size()==16);if (row.size()!=16) return;if (std::stoul(row[7])==0x037f) { ++extended;continue; }
        const auto parent=static_cast<std::uint16_t>(std::stoul(row[3]));const bool warm=std::stoul(row[2])!=0,mutate=std::stoul(row[4])!=0;SidPool pool(OriginalEdition::Patch1078,32768,false);EvilRayLifetimeState state;state.palette=warm ? 1 : 0;state.processType=static_cast<std::uint32_t>(std::stoul(row[5]));state.source=static_cast<std::uint32_t>(std::stoul(row[6])); // 기대값이 아닌 독립 원문 입력이다.
        const_cast<std::uint8_t*>(pool.Slot(Sid{}).data())[35]=255;const_cast<std::uint8_t*>(pool.Slot(Sid{50}).data())[35]=127;const_cast<std::uint8_t*>(pool.Slot(Sid{51}).data())[35]=64;
        std::vector<std::uint32_t> initial;std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 요청 순서만 관찰한다.
        for (std::uint32_t index=0;index<40;++index) initial.push_back(index==20 ? 0U : 100+index); // 원문 입력의 유일한 빈칸20을 준비한다.
        RenderRegistryHooks registration;registration.reserve=[&](auto bytes) { event("N:"+std::to_string(bytes));return true; };registration.releaseArray=[&] { event("F:R"); }; // 외부 확보/반납 경계다.
        registration.attach=[&](auto& fields,auto type,auto target,auto flags) { event("B:"+std::to_string(type)+":"+std::to_string(target.value)+":0:"+std::to_string(flags)+":"+std::to_string(fields.slot)+":"+std::to_string(fields.zOrder));fields.form=7;fields.parent=mutate ? 51 : parent;if (mutate) { state.processType=88;state.source=0xffffffffU;const_cast<std::uint8_t*>(pool.Slot(Sid{parent}).data())[35]=128; } };
        RawRenderRegistry registry(pool,std::move(registration),std::move(initial));EvilRayLifetimeHooks hooks;std::vector<std::uint32_t> colors{31,32,33}; // warm의 외부 기존3색 입력이다.
        hooks.reserve=[&](auto bytes) { event("N:"+std::to_string(bytes));return true; };hooks.createPalette=[] { return 1U; };const auto rgbx=Hex(row[8]);CHECK(rgbx.size()==1024);std::array<std::uint32_t,256> palette{}; // 원문 COL 바이트를 그대로 재구성한다.
        for (std::size_t index=0;index<palette.size();++index) { const auto offset=index*4;palette[index]=std::uint32_t(rgbx[offset])|(std::uint32_t(rgbx[offset+1])<<8)|(std::uint32_t(rgbx[offset+2])<<16)|(std::uint32_t(rgbx[offset+3])<<24); } // RGBX DWORD 순서다.
        hooks.buildPalette=[&](auto token) { CHECK(token==1);event("N:12");colors=BuildEvilRayPalette(palette); };RawEvilRayLifetime body(pool,registry,state,std::move(hooks));EvilRayFields fields;fields.delta=fields.scaledDelta=std::bit_cast<float>(0xa5a5a5a5U);body.Initialize(fields,1,Sid{parent});const auto capacity=fields.particles.size();CHECK(capacity==50 && Fields(fields,capacity)==Hex(row[9]) && Slots(registry)==row[11] && events==row[12]);
        for (const auto& particle:fields.particles) CHECK(particle.x==0 && particle.y==0 && particle.velocityX==0 && particle.velocityY==0 && particle.life==0 && particle.spare==0); // 미사용 용량 전체의 실제 초기값이다.
        std::vector<std::uint8_t> colorBytes;for (const auto color:colors) Put(colorBytes,color);CHECK(colorBytes==Hex(row[10])); // 인접 DWORD를 추가하지 않고 실제3색만 비교한다.
        events.clear();body.Destroy(fields,1,[&](auto kind) { if (kind==EvilRayRelease::Particles) { CHECK(registry.Slots()[20]==1);event("F:P"); }else { CHECK(registry.Slots()[20]==0);event("F:O"); } });CHECK(fields.particles.empty() && Fields(fields,capacity)==Hex(row[13]) && Slots(registry)==row[14] && events==row[15] && state.palette==1);++count;
    }
    CHECK(count==12 && extended==12);
}
TEST_CASE(EvilRayLifetime_NativeStopDiagnosticsAndWholeDword) { // mode3/다른 mode·이미 중단된 전체 DWORD의 실제 진단과 반환값을 비교한다.
    auto input=Fixture();CHECK(input.good());std::string line;std::size_t count=0,extended=0; // 기본/비교 정밀도 행을 구별한다.
    SidPool pool(OriginalEdition::Patch1078,32768,false);RenderRegistryHooks registration;registration.reserve=[](auto) { return true; };registration.releaseArray=[] {};registration.attach=[](auto&,auto,auto,auto) {};RawRenderRegistry registry(pool,std::move(registration));EvilRayLifetimeState state; // Stop은 등록/확보를 호출하지 않는다.
    while (std::getline(input,line)) { // 원문 Stop10행 중 기본53비트5행만 같은 제품 정밀도로 비교한다.
        if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');if (row[0]!="stop") continue;CHECK(row.size()==6);if (row.size()!=6) return;if (std::stoul(row[3])==0x037f) { ++extended;continue; }
        EvilRayLifetimeHooks hooks;std::string diagnostics="[]";hooks.assertion=[&](auto expression,auto file,auto number) { CHECK(file==".\\Evilray.cpp");diagnostics="[[\""+std::string(expression)+"\",\".\\\\Evilray.cpp\","+std::to_string(number)+"]]"; };RawEvilRayLifetime body(pool,registry,state,std::move(hooks));EvilRayFields fields;fields.stopping=static_cast<std::uint32_t>(std::stoul(row[2]));body.Stop(fields,static_cast<std::uint32_t>(std::stoul(row[1])));CHECK(fields.stopping==std::stoul(row[4]) && diagnostics==row[5]);++count;
    }
    CHECK(count==5 && extended==5 && registry.Slots().empty());
}
TEST_CASE(EvilRayLifetime_CurrentReadOrderAndReachedDeviceFailure) { // 실제 등록 뒤 source/확보 앞 TYPE와 선택 장치 누락 때 앞선 효과를 대조한다.
    SidPool pool(OriginalEdition::Patch1078,32768,false);std::uint32_t type=57,source=77,attached=0;RenderRegistryHooks registration;registration.reserve=[](auto) { return true; };registration.releaseArray=[] {};registration.attach=[&](auto& fields,auto value,auto,auto) { attached=value;type=88;source=99;fields.form=7;fields.parent=50; };RawRenderRegistry registry(pool,std::move(registration));EvilRayLifetimeState state;state.palette=1;state.sharedProcessType=&type;state.sharedSource=&source;EvilRayLifetimeHooks hooks;hooks.reserve=[&](auto bytes) { CHECK(bytes==1200);source=101;return true; };RawEvilRayLifetime body(pool,registry,state,std::move(hooks));EvilRayFields fields;body.Initialize(fields,1,Sid{50});CHECK(attached==57 && fields.source==99 && source==101 && fields.particles.size()==50);body.Stop(fields,3);CHECK(fields.stopping==1);body.Destroy(fields,0,[](auto) {});
    state.palette=0;std::vector<std::uint32_t> requested;EvilRayLifetimeHooks missing;missing.reserve=[&](auto bytes) { requested.push_back(bytes);return true; };RawEvilRayLifetime partial(pool,registry,state,std::move(missing));EvilRayFields unfinished;bool failed=false;try { partial.Initialize(unfinished,2,Sid{50}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && (requested==std::vector<std::uint32_t>{1200,8}) && unfinished.particles.size()==50 && unfinished.registration.slot>=0 && state.palette==0);partial.Destroy(unfinished,1,[](auto) {});
}
