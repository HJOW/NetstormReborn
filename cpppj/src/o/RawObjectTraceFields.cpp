// 원문이공통출력뒤파생필드를읽는시점과short/DWORD부호확장을보존한다.
#include "o/RawObjectTraceFields.h"
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 실제 CRT 출력 경계의 최종 바이트를 한국어/UTF-8 변환 없이 작성한다.
namespace { // C문자열/서식의 정상 범위를 보존하고 정의되지 않은 호스트 접근만 거부한다.
// TYPE의 전체 C문자열을 첫NUL에서 끝내며 Base는 이후 따로20BYTE로 자른다.
std::string_view CString(std::string_view name) { return name.substr(0,name.find('\0')); }
// 원문 DWORD SID를 줄이기 전에 물리 범위를 확인한다. free/dead 상태는 여기서 거부하지 않는다.
std::span<const std::uint8_t> Raw(const SidPool& pool,std::uint32_t sid) {
    if (sid>=pool.Capacity()) throw std::out_of_range("객체 서식 물리 SID 범위");return pool.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// 정렬되지 않은 WORD/DWORD를 원문 little endian 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value{}; // 더 넓은 상위 바이트를 섞지 않는 현재 결과다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8); // 요청한 물리 폭만 읽는다.
    return value;
}
// 부호 있는 %03d/%04d의0패딩을 현재 CRT에 맡긴다. 정수 범위 안의 최대문자수는12이하다.
std::string Number(std::int32_t value,int width=0) { char bytes[32]{};std::snprintf(bytes,sizeof(bytes),"%0*d",width,value);return bytes; }
// TYPE의 이름 포인터는 범위를 검사하되20BYTE제한을 적용하지 않는다.
std::string_view Name(std::span<const RiftTypeRecord> types,std::uint32_t type) {
    if (type>=types.size()) throw std::out_of_range("객체 서식 현재 TYPE 범위");return CString(types[type].name);
}
// Content의 sprintf(name,"XXX")를 정상 단일%s/%%서식에만 적용한다. 추가varargs/80BYTE초과는원문미정의경로라거부한다.
std::string ContentName(std::string_view format) {
    std::string result;bool consumed{}; // 정상한개문자열인자와현재80BYTE임시버퍼의내용이다.
    for (std::size_t i=0;i<format.size();) { // C문자열 전체를 읽고각변환의너비/정밀도를그대로CRT에전달한다.
        if (format[i]!='%') { result+=format[i++]; }
        else if (i+1<format.size() && format[i+1]=='%') { result+='%';i+=2; }
        else {
            const auto start=i++; // 현재 변환의 시작이며 한번의%s만인자를소비한다.
            while (i<format.size() && std::string_view("-+ #0").find(format[i])!=std::string_view::npos) ++i; // 원본printf플래그를보존한다.
            while (i<format.size() && format[i]>='0' && format[i]<='9') ++i; // 상수너비만지원한다.
            if (i<format.size() && format[i]=='.') { ++i;while (i<format.size() && format[i]>='0' && format[i]<='9') ++i; } // 문자열정밀도의상수BYTE한도다.
            if (consumed || i>=format.size() || format[i]!='s') throw std::logic_error("Content trace의 추가/미정의 CRT인자");
            consumed=true;const std::string spec(format.substr(start,++i-start));char bytes[80]{}; // 원본80BYTE임시버퍼와같은크기다.
            const auto written=std::snprintf(bytes,sizeof(bytes),spec.c_str(),"XXX");
            if (written<0 || written>=static_cast<int>(sizeof(bytes))) throw std::length_error("Content trace80BYTE임시버퍼범위");result+=bytes;
        }
        if (result.size()>=80) throw std::length_error("Content trace80BYTE임시버퍼범위");
    }
    return result;
}
// 이미발생한공통출력을취소하지않고실제로그장치부재를거부한다.
void Log(const ObjectTraceFieldLog& log,const std::string& line) { if (!log) throw std::logic_error("객체 서식 실제 로그 미연결");log(29,line); }
}
// 공통Base의현재TYPE/링크/상태를기록한뒤선택된파생필드를다시조회한다.
void TraceRawObjectFields(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& suppress,
    std::uint32_t sid,ObjectTraceFieldKind kind,const ObjectTraceFieldLog& log) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("객체 서식 PATCH 풀/TYPE 필요");
    const auto raw=Raw(pool,sid);const auto name=Name(types,raw[10]);std::string line="~1~y"+Number(static_cast<std::int32_t>(sid),4)+"~n Base type="+Number(raw[10]);
    if (!name.empty()) line+="/~y"+std::string(name.substr(0,20))+"~n";line+=' ';
    if (Read(raw,4,2) && !suppress) line+="next=~y"+Number(static_cast<std::int32_t>(Read(raw,4,2)),4)+"~n ";
    if (Read(raw,6,2) && !suppress) line+="head=~y"+Number(static_cast<std::int32_t>(Read(raw,6,2)),4)+"~n ";
    if (raw[11]&1U) line+="~rFREE~n ";if (raw[11]&2U) line+="~rDEAD~n ";if (raw[11]&4U) line+="~rVOID~n ";if (raw[11]&8U) line+="CONTAINED ";
    line+='\n';Log(log,line);
    switch (kind) {
    case ObjectTraceFieldKind::Base:return;
    case ObjectTraceFieldKind::Form: {
        std::string previous; // 파생Form의현재이전WORD는덤프모드에서도표시한다.
        if (Read(raw,14,2)) previous="prev=~y"+Number(static_cast<std::int32_t>(Read(raw,14,2)),4)+"~n ";
        Log(log,"~1     Form parent=~y"+Number(static_cast<std::int32_t>(Read(raw,16,2)),4)+"~n "+previous+"\n");return;
    }
    case ObjectTraceFieldKind::Process:Log(log,"~1     Process #~w"+Number(std::bit_cast<std::int32_t>(Read(raw,18,4)),3)+"~n\n");return;
    case ObjectTraceFieldKind::Depend:Log(log,"~1     Depend watcherFid=~y"+Number(static_cast<std::int32_t>(Read(raw,18,2)),4)+"~n watchedFid=~y"+Number(static_cast<std::int32_t>(Read(raw,20,2)),4)+"~n\n");return;
    case ObjectTraceFieldKind::Content: {
        const auto type=Read(raw,18,2);const auto content=ContentName(Name(types,type));char flags[16]{}; // flags는 원래의 소문자16진수로 기록한다.
        std::snprintf(flags,sizeof(flags),"%x",Read(raw,20,2));
        Log(log,"~1     ContentType="+Number(static_cast<std::int32_t>(type))+"/~y"+content.substr(0,20)+"~n q="+Number(static_cast<std::int32_t>(Read(raw,12,2)))+" flags="+flags+"\n");return;
    }
    }
    throw std::invalid_argument("객체 서식 미등록 본체");
}
}
