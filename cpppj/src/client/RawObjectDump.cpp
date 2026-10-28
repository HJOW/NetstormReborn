// 원문 최초/현재 끝 번호와 파일 후처리의 서로 다른 읽기 시점을 유지한다.
#include "client/RawObjectDump.h"
#include "o/RawObjectTraceMain.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace netstorm::client { // 현재 모드는 선택된 정상/typed 복귀 경로에서만 복구한다.
namespace { // 원문 물리 읽기와 선택된 장치만 확인한다.
// 실제 선택된 하위가 없으면 앞선 효과를 유지한 채 거부한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("객체 덤프 실제 하위 미연결");return value; }
// DWORD SID를 줄이기 전에 물리 풀 범위를 확인한다.
std::span<const std::uint8_t> Raw(const o::SidPool& pool,std::uint32_t sid) { if (sid>=pool.Capacity()) throw std::out_of_range("객체 덤프 물리 SID 범위");return pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); }
// 정렬되지 않은 float32의 원문 비트를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits{}; // 낮은BYTE부터 조립한 실제 좌표다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8); // 원문DWORD폭만읽는다.
    return std::bit_cast<float>(bits);
}
// _ftol의int64절삭 뒤 하위DWORD만 %d에 전달한다. 범위밖/비유한의indefinite도 하위0이다.
std::int32_t Ftol(float value) {
    const double number=std::trunc(static_cast<double>(value));
    if (!std::isfinite(number) || number<-9223372036854775808.0 || number>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(number))));
}
// 원문4자리SID는 최소폭이며10000이상도잘리지않는다.
std::string Number(std::uint32_t sid) { char bytes[24]{};std::snprintf(bytes,sizeof(bytes),"%04u",sid);return bytes; }
}
// 같은현재풀/전역에서모든가상본문과재귀의END/free줄을동일한로그로연결한다.
void TraceRawObjectToLog(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const std::uint32_t& suppress,
    const o::ObjectTraceGlobals& globals,TraceLogState& log,const TraceLogHooks& logHooks,std::uint32_t sid,const std::function<void()>& deadParent) {
    const auto write=[&](std::int32_t level,std::string_view line) {
        std::size_t prefix{}; // 이복원된trace본문은항상고정format의선행~쌍으로시작한다.
        while (prefix+1<line.size() && line[prefix]=='~') prefix+=2; // 인자이름은접두뒤본문에있으므로추가쌍으로세지않는다.
        WriteRawTraceLog(log,level,line,prefix,logHooks);
    };
    o::ObjectTraceHooks hooks;hooks.trace=[&](o::Sid current) { o::TraceRawObjectVirtual(pool,types,suppress,current.value,[&](auto level,auto line,auto) { write(level,line); }); };
    hooks.indent=[&](auto value) { log.indent=value; };hooks.log=write;hooks.deadParent=deadParent;
    o::TraceRawObject(pool,globals,hooks,sid);
}
// 단순TYPE만좌표줄을쓰고나머지는실제재귀뒤구분개행을항상기록한다.
void DumpRawObjects(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,TraceLogState& log,
    const ObjectDumpGlobals& globals,const TraceLogHooks& logHooks,const ObjectDumpHooks& hooks,std::span<std::uint8_t,24> file) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("객체 덤프 PATCH 풀/TYPE 필요");
    const auto priorLevel=log.level;log.level=30;globals.suppress=1;
    const auto path=Need(hooks.path)();log.stream=Need(hooks.open)(path,"wt");
    if (!log.stream) { Need(hooks.openFailure)();if (!log.stream) return; }
    const auto snapshot=globals.count;std::uint32_t current=2; // 원문은cursor1의Next를먼저호출하므로SID1은방문하지않는다.
    while (current<snapshot && (Raw(pool,current)[11]&1U)) ++current; // 최초iterator의free건너뛰기다.
    for (;;) { // 바깥유효범위는매번현재count를읽지만free건너뛰기는최초snapshot을사용한다.
        if (!current || current>=globals.count) break;
        const auto raw=Raw(pool,current);const auto type=static_cast<std::uint32_t>(raw[10]);
        if (type && !(raw[11]&8U)) {
            if (type==globals.landType || type==globals.fringeType || type==globals.simpleType) {
                if (type>=types.size()) throw std::out_of_range("객체 덤프 현재 TYPE 범위");
                const auto name=std::string_view(types[type].name).substr(0,types[type].name.find('\0'));
                WriteRawTraceLog(log,29,Number(current)+" "+std::string(name)+" ("+std::to_string(Ftol(Float(raw,14)))+","+std::to_string(Ftol(Float(raw,18)))+")\r\n",0,logHooks);
            } else { Need(hooks.trace)(current);WriteRawTraceLog(log,29,"\r\n",0,logHooks); }
        }
        ++current;
        while (current<snapshot && (Raw(pool,current)[11]&1U)) ++current; // 실제iterator는새끝번호로snapshot을갱신하지않는다.
    }
    Need(hooks.close)(log.stream);log.stream=0;
    const auto input=Need(hooks.read)(path,0,0);
    if (input.size()>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("객체 덤프 원문DWORD파일길이범위");
    const auto size=static_cast<std::uint32_t>(input.size());const auto signedSize=std::bit_cast<std::int32_t>(size);auto output=Need(hooks.allocate)(signedSize<4 ? 4U : size);
    std::size_t used{}; // 첫NUL뒤의원문입력도필터링하며실제쓰기길이만나중에첫NUL로정한다.
    if (signedSize>0) for (std::size_t i=0;i<input.size();++i) { // 원문은~를보면다음한BYTE를무조건건너뛴다.
        if (input[i]=='~') { ++i;continue; }if (used>=output.size()) throw std::length_error("객체 덤프 후처리할당범위");output[used++]=input[i];
    }
    if (used>=output.size()) throw std::length_error("객체 덤프 마지막NUL물리범위");output[used]=0;
    Need(hooks.construct)(file);
    try {
        Need(hooks.openWrite)(file,path,2,0);
        const auto end=std::find(output.begin(),output.begin()+static_cast<std::ptrdiff_t>(used+1),std::uint8_t{});
        static_cast<void>(Need(hooks.write)(file,std::span<const std::uint8_t>(output.data(),static_cast<std::size_t>(end-output.begin())),0xffffffffU));
    } catch (const ObjectDumpUnableToOpen&) { // 52564c의정확EX_UNABLE_TO_OPEN_FILE handler만6275로복귀한다.
    } catch (...) { Need(hooks.destruct)(file);throw; } // 다른예외는0상태의원본unwind로같은File을반납하고전파한다.
    globals.suppress=0;log.level=priorLevel;Need(hooks.destruct)(file); // 원문은모드복구뒤정상소멸을호출하며그예외를중복정리하지않는다.
}
}
