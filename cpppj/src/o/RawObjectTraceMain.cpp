// 83개 자산의 공통 Main과 Dude Task, 네 내장Form의 진단 분배를 원문에서 복원한다.
#include "o/RawObjectTraceMain.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 기록된 원본 VT 주소는 호스트 함수 포인터로 사용하지 않는다.
namespace { // 진단의 정렬되지 않은 raw 필드와 고정 CRT 서식을 읽는다.
// 원문의 DWORD SID를 WORD로 좁히기 전에 물리 범위를 검사한다.
std::span<const std::uint8_t> Raw(const SidPool& pool,std::uint32_t sid) {
    if (sid>=pool.Capacity()) throw std::out_of_range("Main 진단 물리 SID 범위");return pool.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// 요청한 원문 폭의 little endian 정수만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value{}; // 상위 바이트를 섞지 않는 현재 정수다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8); // 원래 필드 폭만 읽는다.
    return value;
}
// 정수의 부호/0패딩은 같은 C CRT 서식으로 기록한다.
std::string Number(std::int32_t value,int width=0) { char bytes[32]{};std::snprintf(bytes,sizeof(bytes),"%0*d",width,value);return bytes; }
// 현재 TYPE는 로그 콜백 뒤 변경될 수 있으므로 소비 위치마다 다시 조회한다.
const RiftTypeRecord& Type(std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> raw) {
    if (raw[10]>=types.size()) throw std::out_of_range("Main 진단 현재 TYPE 범위");return types[raw[10]];
}
// 원래 로그 호출 한 번으로 바이트와 최종 수치 인자를 함께 전달한다.
void Log(const ObjectTraceMainLog& log,std::int32_t level,std::string_view line,const ObjectTraceMainNumbers* numbers=nullptr) {
    if (!log) throw std::logic_error("Main 진단 실제 로그 미연결");log(level,line,numbers);
}
}
// 실제__fltout2의17자리 가수→__fptostr의5이상 올림→__cftof2의소수점배치를보존한다.
std::string FormatOriginalTraceCoordinate(float value) {
    const auto bits=std::bit_cast<std::uint32_t>(value),absolute=bits&0x7fffffffU;const bool negative=(bits>>31)!=0; // 원문 double승격 전의특수값/부호를구별한다.
    if (!absolute) return "0.00000"; // 원본I10은음의0을양의0과같이출력한다.
    if (absolute>=0x7f800000U) {
        if (absolute==0x7f800000U) return negative ? "-1.#INF0" : "1.#INF0";
        if (negative && absolute==0x7fc00000U) return "-1.#IND0";
        return negative ? "-1.#QNAN" : "1.#QNAN";
    }
    std::array<char,64> scientific{}; // float32의정확한double승격을17자리10진가수로변환한다.
    const double magnitude=static_cast<double>(std::bit_cast<float>(absolute));
    const auto converted=std::to_chars(scientific.data(),scientific.data()+scientific.size(),magnitude,std::chars_format::scientific,16);
    if (converted.ec!=std::errc{}) throw std::logic_error("원본 좌표 진단의17자리 가수 변환 실패");
    const std::string text(scientific.data(),converted.ptr);const auto e=text.find('e');std::string digits=text.substr(0,e);digits.erase(digits.find('.'),1); // 부호/소수점을제외한17개가수다.
    const auto keep=std::stoi(text.substr(e+1))+6;std::string units; // decpt+5자리까지유지한값은고정소수점출력의정수부다.
    if (keep>0) { units=digits.substr(0,static_cast<std::size_t>(keep));if (units.size()<static_cast<std::size_t>(keep)) units.append(static_cast<std::size_t>(keep)-units.size(),'0'); }
    if (units.empty()) units="0";
    if (keep>=0 && static_cast<std::size_t>(keep)<digits.size() && digits[static_cast<std::size_t>(keep)]>='5') {
        auto position=units.size(); // 원문의9연속올림을오른쪽에서왼쪽으로전파한다.
        while (position && units[position-1]=='9') { units[--position]='0'; } // 모든연속9를0으로바꾼다.
        if (position) ++units[position-1];else units.insert(0,1,'1');
    }
    if (units.size()<6) units.insert(0,6-units.size(),'0');units.insert(units.size()-5,1,'.');
    if (negative) units.insert(0,1,'-');return units;
}
// Base가 복귀한 뒤 모든 Main 필드/모드를 현재 물리 주소에서 읽는다.
void TraceRawObjectMain(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,bool task,const ObjectTraceMainLog& log) {
    TraceRawObjectFields(pool,types,suppress,sid,ObjectTraceFieldKind::Base,[&log](auto level,auto line) { Log(log,level,line); });
    const auto raw=Raw(pool,sid);std::string contained,hp,graph,frame,island; // 원문 다섯80BYTE 임시 문자열의 정상 내용이다.
    if (raw[11]&8U) {
        const auto previous=Read(raw,16,2);const auto parent=Read(raw,14,2); // 실제+4(이전) 호출 뒤 +0(부모)를 읽는다. 83개 자산 VT가 같은 두 몸체다.
        contained="(parent=~y"+Number(static_cast<std::int32_t>(parent),4)+"~n prev=~y"+Number(static_cast<std::int32_t>(previous),4)+"~n) ";
    }
    if ((Type(types,raw).flags2&0x400000U) ? (raw[40]&1U)!=0 : (Type(types,raw).flags1&0x10U)!=0)
        hp="HP="+Number(std::bit_cast<std::int32_t>(Read(raw,26,4)))+" ";
    if (Type(types,raw).flags1&0x800U) graph="GrNum="+Number(raw[30])+" "; // 004ad390의 현재 graph BYTE이며 조건상 assert가 발생하지 않는다.
    if (!suppress) frame="fram="+Number(std::bit_cast<std::int32_t>(Read(raw,36,4)))+" ";
    if (Type(types,raw).flags2&2U) island="islandId="+Number(static_cast<std::int32_t>((Type(types,raw).flags2&4U) ? 127U : Read(raw,8,2)))+" "; // 004acda0의 현재 bridge sentinel127/섬WORD다.
    const auto flags=raw[40]; // 원문 Main 상태는 공통 free/dead BYTE와 별개인 +0x28이다.
    const ObjectTraceMainNumbers numbers{static_cast<double>(std::bit_cast<float>(Read(raw,14,4))),static_cast<double>(std::bit_cast<float>(Read(raw,18,4))),
        std::bit_cast<std::int8_t>(raw[33]),Read(raw,12,2),Read(raw,8,2),std::bit_cast<std::int8_t>(raw[35])};
    const auto x=FormatOriginalTraceCoordinate(std::bit_cast<float>(Read(raw,14,4))),y=FormatOriginalTraceCoordinate(std::bit_cast<float>(Read(raw,18,4))); // 원본CRT의실수서식은최신CRT와동률/음의0이다르므로별도복원한다.
    constexpr const char* format="~1     Main( %s %s) hl=%d q=%d r=%d z=%d %splId=%d %s%s\n     Main %s%s%s%s%s%s%s%s%sgenus=%X\n"; // 004b0fb0의실수두개만원본CRT완성문자열로전달하며나머지서식은같다.
    std::array<char,1024> bytes{}; // 유한 float32 좌표 두 개와 모든 고정 상태 문자열을 담는 호스트 출력 버퍼다.
    const auto size=std::snprintf(bytes.data(),bytes.size(),format,x.c_str(),y.c_str(),numbers.highlight,static_cast<int>(numbers.quantity),static_cast<int>(numbers.region),numbers.z,
        frame.c_str(),static_cast<int>(raw[34]),hp.c_str(),graph.c_str(),(flags&8U)?"~rAstral~n ":"",(flags&1U)?"~rAbstract~n ":"",(flags&2U)?"OvDraw ":"",
        (flags&4U)?"Selected ":"",(flags&16U)?"Invisible":"",(flags&32U)?"Paralyzed":"",(flags&128U)?"InWorld ":"~r!InWorld~n",contained.c_str(),island.c_str(),Type(types,raw).flags2);
    if (size<0 || static_cast<std::size_t>(size)>=bytes.size()) throw std::length_error("Main 진단 호스트 서식 범위");
    Log(log,29,std::string_view(bytes.data(),static_cast<std::size_t>(size)),&numbers);
    if (task) {
        constexpr std::array<std::string_view,7> names{"TEND_MANA_PRODUCERS","GO_HOME","GO_TO","FALL","BLEED","RunBurning","\x87YN"}; // 원문 마지막 포인터0051a688의 C문자열도 세BYTE 그대로 보존한다.
        const auto current=std::clamp(Read(raw,12,2),0x5101U,0x5107U); // Main 로그 복귀 뒤 현재 작업WORD를 읽는 원문 상/하한이다.
        Log(log,29,"~1~y     Task \""+std::string(names[current-0x5101U])+"\"\n");
    }
}
// 실제 원본+70 분배표의83개 자산과 네Form만 선택하며 미지 VT를 임의Base로 대체하지 않는다.
void TraceRawObjectVirtual(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,const ObjectTraceMainLog& log) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("가상 진단 PATCH 풀 필요");
    const auto table=Read(Raw(pool,sid),0,4);ObjectTraceFieldKind kind{}; // 네 내장Form의 현재 가상 표 기록값이다.
    switch (table) {
    case 0x512110U:kind=ObjectTraceFieldKind::Depend;break;
    case 0x512098U:kind=ObjectTraceFieldKind::Process;break;
    case 0x512188U:kind=ObjectTraceFieldKind::Form;break;
    case 0x512208U:kind=ObjectTraceFieldKind::Content;break;
    default:
        if (!SquidFactory::SupportsConstructedVtable(pool.Edition(),table)) throw std::logic_error("객체 trace 미등록 가상 표");
        TraceRawObjectMain(pool,types,suppress,sid,table==0x508cc8U,log);return;
    }
    TraceRawObjectFields(pool,types,suppress,sid,kind,[&log](auto level,auto line) { Log(log,level,line); });
}
}
