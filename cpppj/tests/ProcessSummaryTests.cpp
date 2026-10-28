// 독립 원문20입력의 전체 파일 바이트와 현재 슬롯/이름/부모 재조회 순서를 비교한다.
#include "TestSupport.h"
#include "o/RawProcessSummary.h"
#include "o/RiftType.h"
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 기대 바이트는 원문 CRT 인자에서만 읽으며 제품 계산으로 만들지 않는다.
using namespace netstorm::o;
//NUL/탭/LF를 포함한 실제 바이트를 TSV의 단일 열로 보존한다.
std::string Hex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; //한 바이트당 두 자리의 원본 바이트 표현이다.
    for (const unsigned char value:bytes) { result.push_back(digits[value>>4]);result.push_back(digits[value&15]); } //문자 코드 변환 없이 낮은8비트를 쓴다.
    return result;
}
//합성 프로세스의 원본+4/+8에 해당하는 명시 입력 필드다.
struct Record { std::uint32_t form{};std::int32_t parent{}; }; //form은현재Form SID, parent는signed 부착 부모다.
//풀/슬롯/장치의 입력과 원문에서 정한 시점의 외부 변형만 보유한다.
struct SummaryScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; //원문 관측기의 같은 실제50바이트 물리 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //현재 부모의 이름이 들어 있는 원본 TYPE 표다.
    std::array<ProcessSummaryHandle,40001> slots{}; //0/40000은 읽으면 안 되는 sentinel도 포함한다.
    std::array<Record,80> records{}; //불투명 핸들 번호별 현재 두 필드다.
    std::string trace; //실제파일 경계에 전달된 원문 순서의 인자 기록이다.
    std::int32_t mutation{};std::uint32_t file{},lines{},forms{},parents{}; //입력 변형/파일 토큰과 현재 경계 호출 횟수다.
    //검사 입력은 non-const 풀이 소유한 물리 슬롯을 초기화하며 할당 상태도 원문처럼 따로 둔다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(sid*50,50); }
    //원문의 fprintf가 돌아온 순간에만 다음 필드/슬롯을 바꾼다.
    void Mutate() {
        switch (mutation) {
        case 1:slots[2]=0;break;case 2:slots[100]=4;break;case 3:records[2].parent=0;break;
        case 4:Raw(51)[10]=54;break;case 5:Raw(61)[10]=157;break;case 6:slots[2]=3;break;
        case 7:slots[1]=4;break;case 8:Raw(50)[10]=62;break;case 13:slots[39999]=0;break;case 14:slots[3]=4;break;default:break;
        }
    }
    //입력 구성은 원문 도구와 같고 출력 trace는 실행 전 비워 둔다.
    SummaryScene(std::uint32_t scenario,std::int32_t change,std::uint32_t token):mutation(change),file(token) {
        types[82].name="bridge";types[94].name="island";types[157].name="aLongParentNameWith%and\tText";types[0].name=std::string("none\0ignored",12);
        Raw(60)[10]=82;Raw(61)[10]=94;Raw(62)[10]=157;Raw(63)[10]=0;
        constexpr std::array<std::uint8_t,5> kinds{46,10,55,54,0};constexpr std::array<std::int32_t,5> parent{60,61,0,62,63}; //다섯 합성 프로세스의 입력 TYPE/부모다.
        for (std::size_t i=0;i<kinds.size();++i) { records[i+1]={static_cast<std::uint32_t>(50+i),parent[i]};Raw(static_cast<std::uint32_t>(50+i))[10]=kinds[i]; } //원본+4 Form은각각50..54다.
        slots[0]=slots[40000]=5;
        if (scenario==1) { slots[1]=1;records[1].parent=0;Raw(50)[11]=1; }
        else if (scenario==2) { slots[1]=1;slots[2]=2;slots[3]=3;slots[39999]=5; }
        else if (scenario==3) for (std::uint32_t kind=0;kind<63;++kind) { //축약 TYPE 이름으로 대체하면 실패하는 전체63개 원래 이름이다.
            const auto handle=10+kind,sid=100+kind;slots[kind+1]=handle;records[handle]={sid,std::array<std::int32_t,5>{0,60,61,62,63}[kind%5]};Raw(sid)[10]=static_cast<std::uint8_t>(kind);Raw(sid)[11]=static_cast<std::uint8_t>(kind%16);
        }
        else if (scenario==4) { slots[1]=3;slots[39998]=1;slots[39999]=2; }
    }
    //사본 목록이 아니라 원문이 재조회하는 같은 현재 핸들/필드를 공급한다.
    ProcessSummaryHooks Hooks() {
        return {
            [this](std::uint32_t slot) { CHECK(slot>=1 && slot<=39999);return slots.at(slot); },
            [this](ProcessSummaryHandle handle) { const auto form=records.at(handle).form;if (++forms==1 && mutation==9) slots[1]=2;return form; },
            [this](ProcessSummaryHandle handle) { const auto parent=records.at(handle).parent;if (++parents==1) { if (mutation==10) records.at(handle).parent=61;else if (mutation==11) slots[1]=2; }return parent; },
            [this](std::string_view path,std::string_view mode) { trace="O:"+Hex(path)+":"+Hex(mode)+":"+std::to_string(file);return file; },
            [this](ProcessSummaryHandle stream,std::string_view line) { trace+=";W:"+std::to_string(stream)+":"+Hex(line);if (++lines==1) Mutate(); },
            [this](ProcessSummaryHandle stream) { trace+=";C:"+std::to_string(stream); }
        };
    }
};
}

//가상 파일 장치에 전달된 인자/바이트는20개 독립 원문 결과와 모두 같아야 한다.
TEST_CASE(ProcessSummary_IndependentNative20CurrentSlotsAndFullNames) {
    std::ifstream input(NETSTORM_PROCESS_SUMMARY_FIXTURE);CHECK(bool(input));std::string line;std::size_t count{};
    while (std::getline(input,line)) { //모든 동결 입력을 누락 없이 실제39999슬롯으로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        std::stringstream fields(line);std::string scenario,change,file,expected;std::getline(fields,scenario,'\t');std::getline(fields,change,'\t');std::getline(fields,file,'\t');std::getline(fields,expected);
        SummaryScene scene(static_cast<std::uint32_t>(std::stoul(scenario)),std::stoi(change),static_cast<std::uint32_t>(std::stoul(file)));
        WriteRawProcessSummary(scene.pool,scene.types,scene.Hooks());CHECK(scene.trace==expected);++count;
    }
    CHECK(count==20);
}

//호스트 범위/미연결 장치 거부는 이미 발생한 open/출력 효과를 되돌리지 않는다.
TEST_CASE(ProcessSummary_HostBoundariesPreservePriorEffects) {
    SummaryScene scene(2,0,700);auto hooks=scene.Hooks();scene.records[1].form=0xffffffffU;bool threw=false;
    try { WriteRawProcessSummary(scene.pool,scene.types,hooks); }catch (const std::out_of_range&) { threw=true; }
    CHECK(threw && scene.trace=="O:70726f6353756d6d2e747874:7762:700");
    SummaryScene missing(0,0,0);auto absent=missing.Hooks();absent.close={};threw=false;
    try { WriteRawProcessSummary(missing.pool,missing.types,absent); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && missing.trace=="O:70726f6353756d6d2e747874:7762:0");
}
