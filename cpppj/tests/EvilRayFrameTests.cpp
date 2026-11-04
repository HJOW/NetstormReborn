// 실제 PE의 Frame/Draw 상태·입자·음향/dirty/타원 인자를 격리 대조한다.
#include "TestSupport.h"
#include "o/RawEvilRayFrame.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace { // 원문 BYTE 판독과 의미 필드/요청 직렬화만 검사에 한정한다.
std::vector<std::string> Split(std::string_view text,char delimiter) { // TSV/인자 열을 마지막 빈 열까지 읽는다.
    std::vector<std::string> result;std::size_t start=0; // 이미 읽은 열과 현재 시작이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 각 구분자를 한 번씩 소비한다.
}
std::vector<std::uint8_t> Hex(std::string_view value) { // 원본 hex를 바꾸지 않고 BYTE로 읽는다.
    std::vector<std::uint8_t> result;if (value=="-") return result;if (value.size()%2) throw std::runtime_error("EvilRay Frame hex 폭"); // 빈 입자 배열은-다.
    for (std::size_t index=0;index<value.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(value.substr(index,2)),nullptr,16))); // 두 ASCII 문자마다 한 BYTE다.
    return result;
}
std::string Encoded(std::span<const std::uint8_t> bytes) { // Native 출력과 같은 소문자 hex다.
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 원문 BYTE를 표시하는 고정 문자표다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15]; } // 전체 BYTE를 순서대로 보존한다.
    return result;
}
std::uint64_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) { // 호스트 정렬/배치에 의존하지 않는 little endian 판독이다.
    std::uint64_t result=0;for (std::size_t index=0;index<width;++index) result|=std::uint64_t(bytes[offset+index])<<(index*8);return result; // 지정 폭만 읽는다.
}
float Single(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(static_cast<std::uint32_t>(Read(bytes,offset))); } // float 전체 비트다.
double Double(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<double>(Read(bytes,offset,8)); } // double 전체 비트다.
void Put(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) { for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8))); } // 원문 폭만 직렬화한다.
std::string Blob(std::initializer_list<std::uint32_t> values) { std::vector<std::uint8_t> bytes;for (const auto value:values) Put(bytes,value);return Encoded(bytes); } // 외부 요청 DWORD 배열의 실제 바이트다.
std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); } // signed0/NaN도 정규화하지 않는다.
std::vector<std::uint8_t> Fields(const EvilRayFields& fields) { // Native semantic과 같은80BYTE다. 호스트 포인터/패딩은 제외한다.
    const auto& reg=fields.registration;std::vector<std::uint8_t> bytes;Put(bytes,static_cast<std::uint32_t>(reg.form));Put(bytes,static_cast<std::uint32_t>(reg.parent)); // 등록 앞부분이다.
    for (const auto value:{reg.left,reg.top,reg.right,reg.bottom}) Put(bytes,Bits(value)); // 네 영역 float다.
    Put(bytes,static_cast<std::uint32_t>(reg.slot));Put(bytes,static_cast<std::uint32_t>(reg.zOrder));Put(bytes,fields.particles.size());Put(bytes,static_cast<std::uint32_t>(fields.total));Put(bytes,std::bit_cast<std::uint64_t>(fields.lastTime),8);Put(bytes,std::bit_cast<std::uint64_t>(fields.elapsed),8);Put(bytes,Bits(fields.delta));Put(bytes,Bits(fields.scaledDelta));Put(bytes,fields.first ? 1 : 0);Put(bytes,Bits(fields.nextEmission));Put(bytes,fields.source);Put(bytes,fields.stopping);return bytes;
}
std::vector<std::uint8_t> Particles(const EvilRayFields& fields) { // 사용 범위 밖을 포함한 전체24BYTE 입자 배열이다.
    std::vector<std::uint8_t> bytes;for (const auto& particle:fields.particles) { Put(bytes,Bits(particle.x));Put(bytes,Bits(particle.y));Put(bytes,Bits(particle.velocityX));Put(bytes,Bits(particle.velocityY));Put(bytes,Bits(particle.life));Put(bytes,particle.spare); }return bytes; // 미작성 속도/마지막 DWORD도 비교한다.
}
}
TEST_CASE(EvilRayFrame_NativeCurrentStateParticlesAndOrderedRequests) { // 기본53비트18행의 상태/모든 입자/요청 순서를 비교하며 비교 정밀도18행은 별도 보존한다.
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/evilray-frame-x86.tsv");CHECK(input.good());std::string line;std::size_t count=0,extended=0; // 원문 재생 행 수다.
    while (std::getline(input,line)) { // 독립 원문 입력으로만 각 몸체를 실행한다.
        if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==11);if (row.size()!=11) return;if (std::stoul(row[2])==0x037f) { ++extended;continue; }
        const auto raw=Hex(row[3]),particles=Hex(row[4]),parents=Hex(row[5]),stateBytes=Hex(row[6]);const auto parameters=Split(row[7],',');SidPool pool(OriginalEdition::Patch1078,32768,false);std::size_t offset=0; // 원문 pointer는 호스트 주소로 실행하지 않는다.
        for (const auto sid:{0,50,51}) { auto slot=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(Sid{static_cast<std::uint16_t>(sid)}).data()),50);std::copy_n(parents.begin()+static_cast<std::ptrdiff_t>(offset),50,slot.begin());offset+=50; } // 세 현재 물리 슬롯만 원문 입력으로 덮는다.
        EvilRayFields fields;auto& reg=fields.registration;reg.form=static_cast<std::int32_t>(Read(raw,4));reg.parent=static_cast<std::int32_t>(Read(raw,8));reg.left=Single(raw,12);reg.top=Single(raw,16);reg.right=Single(raw,20);reg.bottom=Single(raw,24);reg.slot=static_cast<std::int32_t>(Read(raw,28));reg.zOrder=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(raw,32)));fields.total=static_cast<std::int32_t>(Read(raw,40));fields.lastTime=Double(raw,48);fields.elapsed=Double(raw,56);fields.delta=Single(raw,64);fields.scaledDelta=Single(raw,68);fields.first=Read(raw,72)!=0;fields.nextEmission=Single(raw,80);fields.source=static_cast<std::uint32_t>(Read(raw,84));fields.stopping=static_cast<std::uint32_t>(Read(raw,88));
        for (std::size_t at=0;at<particles.size();at+=24) fields.particles.push_back({Single(particles,at),Single(particles,at+4),Single(particles,at+8),Single(particles,at+12),Single(particles,at+16),static_cast<std::uint32_t>(Read(particles,at+20))}); // 입력 전체 용량/미사용 비트다.
        FieryTailFrameState state;state.now=Double(stateBytes,0);state.pixelScaleX=Single(stateBytes,8);state.pixelScaleY=Single(stateBytes,12);state.rate=Single(stateBytes,16);state.cameraX=Single(stateBytes,20);state.cameraY=Single(stateBytes,24);const std::array<std::uint32_t,3> colors{17,0x12345678,0xabcdef01};std::string events;bool killed=false;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 외부 경계의 인자/순서만 관찰한다.
        EvilRayFrameHooks hooks;hooks.shapeWidth=[&](Sid sid) { const auto current=pool.Slot(sid);event("M:"+std::to_string(current[10])+":"+std::to_string(current[40]&1)+":"+std::to_string(Read(current,36)));return static_cast<std::int16_t>(std::stoi(parameters[0])); };hooks.soundId=[&](Sid sid,bool end) { event("T:"+std::to_string(sid.value)+":"+std::to_string(end ? 1 : 0));return static_cast<std::uint32_t>(std::stoul(parameters[end ? 3 : sid.value==51 ? 2 : 1])); };
        hooks.sound=[&](float x,float y,std::uint32_t id) { event("S:"+Blob({Bits(x),Bits(y),id,0,0})); };hooks.invalidate=[&](auto rect) { event("D:"+Blob({static_cast<std::uint32_t>(rect.left),static_cast<std::uint32_t>(rect.top),static_cast<std::uint32_t>(rect.right),static_cast<std::uint32_t>(rect.bottom),0})); };hooks.kill=[&](auto flags) { event("K:"+std::to_string(flags));killed=true; };
        hooks.clip=[&](auto rect) { event("C:"+std::to_string(rect[0])+","+std::to_string(rect[1])+","+std::to_string(rect[2])+","+std::to_string(rect[3])); };hooks.circle=[&](auto circle) { event("R:"+Blob({static_cast<std::uint32_t>(circle.x),static_cast<std::uint32_t>(circle.y),static_cast<std::uint32_t>(circle.radius),circle.color})); };hooks.assertion=[&](auto expression,auto file,auto number) { CHECK(file==".\\Evilray.cpp");event("A:"+std::to_string(number)+":"+std::string(expression));if (std::stoul(parameters[4])) fields.source=51; };
        RawEvilRayFrame body(pool,state,colors,std::move(hooks));if (row[1]=="frame") body.Step(fields);if (!killed) body.Draw(fields);CHECK(Fields(fields)==Hex(row[8]) && Particles(fields)==Hex(row[9]) && (events.empty() ? "-" : events)==row[10]);++count;
    }
    CHECK(count==18 && extended==18);
}
TEST_CASE(EvilRayFrame_ParentDiagnosticUsesSavedPhysicalParentAndDrawDevicesAreLazy) { // 원문 부모 getter 진단 뒤 저장 물리/현재 TYPE와 Draw 진단→clip→물리 실패 순서를 관찰한다.
    SidPool pool(OriginalEdition::Patch1078,32768,false);auto* parent=const_cast<std::uint8_t*>(pool.Slot(Sid{51}).data());parent[6]=parent[7]=0;FieryTailFrameState state;state.now=1;EvilRayFields fields;fields.source=50;fields.registration.parent=51;fields.first=true;fields.lastTime=1;std::uint32_t diagnoses=0,dirty=0;EvilRayFrameHooks hooks; // 실제 게임/음향/Host 없이 선택 경계만 설치한다.
    hooks.shapeWidth=[](auto) { return std::int16_t{9}; };hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="parent->getHead() != 0" && file=="\\Ns\\O\\Baseprocess.cpp" && line==0xa4);++diagnoses;parent[10]=74;fields.registration.parent=52; };hooks.soundId=[&](Sid sid,bool end) { CHECK(!end);if (sid==Sid{50}) return 0U;CHECK(sid==Sid{51} && pool.Slot(sid)[10]==74);return 17U; };hooks.sound=[](auto,auto,auto id) { CHECK(id==17); };hooks.invalidate=[&](auto) { ++dirty; };RawEvilRayFrame body(pool,state,{},std::move(hooks));body.Step(fields);CHECK(diagnoses==1 && dirty==2 && fields.nextEmission==15 && fields.total==0);
    EvilRayFields invalid;invalid.total=1;std::vector<int> order;EvilRayFrameHooks selected;selected.assertion=[&](auto expression,auto,auto line) { CHECK(expression=="totalParticles <= maxParticles" && line==0x8c);order.push_back(1); };selected.clip=[&](auto rect) { CHECK((rect==std::array<std::int32_t,4>{0,0,639,479}));order.push_back(2); };RawEvilRayFrame draw(pool,state,{},std::move(selected));bool failed=false;try { draw.Draw(invalid); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && (order==std::vector<int>{1,2}));
}
TEST_CASE(EvilRayFrame_ExactZeroDeathSoundPrecedesNextEmptyStopKill) { // 실제 FCOMP 분기의<=0과 이번 양수 입자를 활성으로 센 뒤 다음 호출에 종료하는 순서를 확인한다.
    SidPool pool(OriginalEdition::Patch1078,32768,false);FieryTailFrameState state;state.now=1.1;EvilRayFields fields;fields.source=50;fields.registration.parent=51;fields.first=true;fields.lastTime=1;fields.stopping=1;fields.total=1;fields.particles.push_back({1,2,0,0,1,0});std::uint32_t sounds=0,dirty=0,killed=0;EvilRayFrameHooks hooks; // 순수 시계/끝 음향과 종료 요청만 관찰한다.
    hooks.shapeWidth=[](auto) { return std::int16_t{9}; };hooks.soundId=[](Sid sid,bool end) { CHECK(sid==Sid{51} && end);return 31U; };hooks.sound=[&](auto,auto,auto id) { CHECK(id==31);++sounds; };hooks.invalidate=[&](auto) { ++dirty; };hooks.kill=[&](auto flags) { CHECK(flags==0);++killed; };RawEvilRayFrame body(pool,state,{},std::move(hooks));body.Step(fields);CHECK(fields.particles[0].life==0 && sounds==1 && dirty==2 && killed==0);body.Step(fields);CHECK(sounds==1 && dirty==2 && killed==1);
}
