// 실제 Fiery Tail Frame/Draw의 전체 의미 필드·입자·난수·외부 요청을 원본 비트로 대조한다.
#include "TestSupport.h"
#include "o/RawFieryTailFrame.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace netstorm::o;
namespace {
// 기본/비교 정밀도에서 각각 관찰한 고유 입력 수다.
constexpr std::size_t kFieryNativeCases=65;
// TSV 열의 빈 값도 보존한다.
std::vector<std::string> Split(std::string_view text) {
    std::vector<std::string> result;std::size_t start=0;
    // 마지막 열까지 순서대로 읽는다.
    for (;;) { const auto end=text.find('\t',start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; }
}
// 한 hex 문자를 바이트의 상위/하위4비트로 읽는다.
std::uint8_t Digit(char value) {
    if (value>='0' && value<='9') return static_cast<std::uint8_t>(value-'0');
    if (value>='a' && value<='f') return static_cast<std::uint8_t>(value-'a'+10);
    throw std::runtime_error("Fiery Tail hex 문자 오류");
}
// 원본 바이트를 손실 없이 읽는다. '-'는 길이0이다.
std::vector<std::uint8_t> Hex(std::string_view text) {
    if (text=="-") return {};if (text.size()%2) throw std::runtime_error("Fiery Tail hex 길이 오류");
    std::vector<std::uint8_t> result;result.reserve(text.size()/2);
    // 두 문자마다 원본 메모리 바이트 하나다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>((Digit(text[index])<<4)|Digit(text[index+1])));
    return result;
}
// 비정렬 DWORD/QWORD를 호스트 구조체와 무관하게 읽는다.
std::uint64_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset+width>bytes.size() || width>8) throw std::out_of_range("Fiery Tail 원본 비트 범위 오류");
    std::uint64_t result=0;
    // 각 바이트의 실제 little endian 위치를 보존한다.
    for (std::size_t index=0;index<width;++index) result|=static_cast<std::uint64_t>(bytes[offset+index])<<(8*index);
    return result;
}
// 입력 float의 전체32비트를 복원한다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(static_cast<std::uint32_t>(Read(bytes,offset))); }
// 입력 signed DWORD를 부호 변경 없이 복원한다.
std::int32_t Integer(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(bytes,offset))); }
// 출력 의미 필드를 실제 little endian 바이트로 직렬화한다.
void Put(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    // 상위 바이트의 부호 확장 없이 지정 폭만 기록한다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(8*index)));
}
// float는 허용오차 없이 전체 비트를 보존한다.
void PutFloat(std::vector<std::uint8_t>& bytes,float value) { Put(bytes,std::bit_cast<std::uint32_t>(value)); }
// native 객체의68바이트 의미 필드 순서대로 기록한다.
std::vector<std::uint8_t> Fields(const FieryTailFrameFields& fields) {
    std::vector<std::uint8_t> bytes;const auto& rect=fields.registration;
    PutFloat(bytes,rect.left);PutFloat(bytes,rect.top);PutFloat(bytes,rect.right);PutFloat(bytes,rect.bottom);
    Put(bytes,fields.particles.size());Put(bytes,static_cast<std::uint32_t>(fields.total));
    Put(bytes,std::bit_cast<std::uint64_t>(fields.lastTime),8);Put(bytes,std::bit_cast<std::uint64_t>(fields.elapsed),8);
    PutFloat(bytes,fields.delta);PutFloat(bytes,fields.scaledDelta);Put(bytes,fields.first ? 1 : 0);
    Put(bytes,static_cast<std::uint32_t>(fields.maximumLife));PutFloat(bytes,fields.previousX);PutFloat(bytes,fields.previousY);Put(bytes,static_cast<std::uint32_t>(fields.offset));return bytes;
}
// 용량 전체의24바이트 의미 필드를 실제 원본 순서로 기록한다.
std::vector<std::uint8_t> Particles(const FieryTailFrameFields& fields) {
    std::vector<std::uint8_t> bytes;
    // 사용 꼬리 밖의 active 입자와 사용하지 않는 필드도 포함한다.
    for (const auto& particle:fields.particles) {
        PutFloat(bytes,particle.x);PutFloat(bytes,particle.y);PutFloat(bytes,particle.velocityX);PutFloat(bytes,particle.velocityY);PutFloat(bytes,particle.life);Put(bytes,particle.spare);
    }
    return bytes;
}
// 전체 비교에서 최초로 다른 바이트를 입력명/범위와 함께 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,const std::string& context) {
    if (actual.size()!=expected.size()) { const auto message=context+" 길이 불일치";netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str());return; }
    // 같은 행의 후속 비교를 계속해 원인을 숨기지 않는다.
    for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) {
        const auto message=context+" 바이트"+std::to_string(index)+" 불일치 실제"+std::to_string(actual[index])+" 기대"+std::to_string(expected[index]);
        netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str());return;
    }
}
}
// 실제 기본53비트65행의 프레임/Draw 결과를 재계산한 기대값 없이 대조한다.
TEST_CASE(FieryTailFrame_Native027f) {
    std::ifstream input(NETSTORM_FIERYTAIL_FRAME_FIXTURE);if (!input) throw std::runtime_error("Fiery Tail 원본 fixture 없음");
    SidPool pool(OriginalEdition::Patch1078,32768,false);std::string line;std::size_t count=0,extended=0;
    // 비교037f의 원문은 보존하고 제품 기본 설정의 결과만 완료로 센다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line);if (row.size()!=14) throw std::runtime_error("Fiery Tail 원본 열 수 오류");
        const auto precision=std::stoul(row[1]);if (precision==0x037f) { ++extended;continue; }if (precision!=0x027f) throw std::runtime_error("Fiery Tail 원본 정밀도 오류");
        const auto before=Hex(row[2]),particles=Hex(row[3]),parent=Hex(row[4]),globals=Hex(row[5]),colors=Hex(row[6]);
        if (before.size()!=68 || parent.size()!=50 || globals.size()!=40 || colors.size()!=260) throw std::runtime_error("Fiery Tail 원본 입력 폭 오류");
        auto* parentBytes=const_cast<std::uint8_t*>(pool.Slot(Sid{50}).data());std::copy(parent.begin(),parent.end(),parentBytes);
        FieryTailFrameFields fields;auto& rect=fields.registration;rect.parent=50;
        rect.left=Float(before,0);rect.top=Float(before,4);rect.right=Float(before,8);rect.bottom=Float(before,12);
        fields.particles.resize(static_cast<std::size_t>(Read(before,16)));fields.total=Integer(before,20);
        fields.lastTime=std::bit_cast<double>(Read(before,24,8));fields.elapsed=std::bit_cast<double>(Read(before,32,8));
        fields.delta=Float(before,40);fields.scaledDelta=Float(before,44);fields.first=Read(before,48)!=0;
        fields.maximumLife=Integer(before,52);fields.previousX=Float(before,56);fields.previousY=Float(before,60);fields.offset=Integer(before,64);
        if (particles.size()!=fields.particles.size()*24) throw std::runtime_error("Fiery Tail 원본 입자 폭 오류");
        // 현재 입력 입자만 복원하며 기대 출력에는 제품 계산을 사용하지 않는다.
        for (std::size_t index=0;index<fields.particles.size();++index) {
            auto& particle=fields.particles[index];const auto offset=index*24;
            particle={Float(particles,offset),Float(particles,offset+4),Float(particles,offset+8),Float(particles,offset+12),Float(particles,offset+16),static_cast<std::uint32_t>(Read(particles,offset+20))};
        }
        FieryTailFrameState state;state.now=std::bit_cast<double>(Read(globals,0,8));state.pixelScaleX=Float(globals,8);state.pixelScaleY=Float(globals,12);
        state.rate=Float(globals,16);state.cameraX=Float(globals,20);state.cameraY=Float(globals,24);state.diagnostic=Integer(globals,36);
        std::vector<std::uint32_t> readable;
        // 조회 가능한 인접 DWORD도 원본 입력에서만 공급한다.
        for (std::size_t offset=4;offset<colors.size();offset+=4) readable.push_back(static_cast<std::uint32_t>(Read(colors,offset)));
        FieryTailFramePalette palette{Integer(colors,0),readable};GameRandom random(static_cast<std::uint32_t>(std::stoul(row[7])));
        std::vector<std::uint8_t> dirty,lines;FieryTailFrameHooks hooks;
        // 원본 외부 dirty 요청의 flags0을 포함해 기록한다.
        hooks.invalidate=[&](FieryTailDirtyRect value) { Put(dirty,static_cast<std::uint32_t>(value.left));Put(dirty,static_cast<std::uint32_t>(value.top));Put(dirty,static_cast<std::uint32_t>(value.right));Put(dirty,static_cast<std::uint32_t>(value.bottom));Put(dirty,0); };
        // 실제 선 요청의 양 끝 좌표와 색 DWORD를 기록한다.
        hooks.line=[&](FieryTailLine value) { Put(lines,static_cast<std::uint32_t>(value.x1));Put(lines,static_cast<std::uint32_t>(value.y1));Put(lines,static_cast<std::uint32_t>(value.x2));Put(lines,static_cast<std::uint32_t>(value.y2));Put(lines,value.color); };
        RawFieryTailFrame frame(pool,random,state,palette,std::move(hooks));frame.Step(fields);frame.Draw(fields,Integer(globals,28),Integer(globals,32));
        Compare(Fields(fields),Hex(row[8]),row[0]+" 필드");Compare(Particles(fields),Hex(row[9]),row[0]+" 입자");
        auto paletteAfter=colors;
        // 실제 진단 분기가 바꿀 수 있는 색수만 현재 값으로 직렬화한다.
        for (std::size_t index=0;index<4;++index) paletteAfter[index]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(palette.count)>>(8*index));
        Compare(paletteAfter,Hex(row[10]),row[0]+" 색 표");CHECK(random.State()==std::stoul(row[11]));
        Compare(dirty,Hex(row[12]),row[0]+" dirty");Compare(lines,Hex(row[13]),row[0]+" 선 요청");Compare(pool.Slot(Sid{50}),parent,row[0]+" 부모 불변");++count;
    }
    CHECK(count==kFieryNativeCases);CHECK(extended==kFieryNativeCases);
}
// 실제 초기 수명은11번째 뒤의 DWORD를 읽는다. 그 입력이 없으면 가짜 색을 내보내지 않는다.
TEST_CASE(FieryTailFrame_MissingAdjacentColorRejects) {
    SidPool pool(OriginalEdition::Patch1078,32768,false);GameRandom random;FieryTailFrameState state;
    std::array<std::uint32_t,11> colors{};FieryTailFramePalette palette{11,colors};FieryTailFrameFields fields;
    fields.particles.resize(1);fields.particles[0].life=4;fields.total=1;std::size_t output=0;
    FieryTailFrameHooks hooks;hooks.invalidate=[](FieryTailDirtyRect) {};hooks.line=[&](FieryTailLine) { ++output; };
    RawFieryTailFrame frame(pool,random,state,palette,std::move(hooks));bool rejected=false;
    try { frame.Draw(fields,0,0); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);CHECK(output==0);
}
