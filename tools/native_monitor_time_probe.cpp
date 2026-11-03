// 원본 시간/임시 Chat 서식 부모를 실제 CPU로 실행하며 시계/달력/제출 장치만 기록한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 원본 초기화/GUI/통신 없이 전용 payload만 사용한다.
struct MonitorLocations { // 판본별 실제 전역/원문 경계다.
    std::uint32_t entry,guard,deadline,monitor,alert,handle,time,local,asctime,format,config,emit,current,submit; // 모든 주소는 보호 PE 복사본 안이다.
};
constexpr MonitorLocations kPatchMonitor{0x4182c0,0x544d68,0x544d60,0x54db9c,0x54dba0,0x540ca8,0x4e4f1f,0x4e4d9f,0x4e4c94,0x4a89c0,0x4410f0,0x42f430,0x548714,0x42da00}; //10.78의 현재 주소다.
constexpr MonitorLocations kCdMonitor{0x4ecb30,0x5b7b98,0x5b7ba8,0x52e964,0x52e968,0x50f6d0,0x4f15c0,0x4f4190,0x4f4080,0x420560,0x4a9460,0x4b2d40,0x51ade0,0x43a9f0}; //10.72의 현재 주소다.
MonitorLocations locations{}; // 현재 판본의 입력 주소만 보관한다.
double wall{},increment{}; // 명시 외부 시계의 현재 입력이다.
std::uint32_t calls{},exitCalls{},changes{}; // 경계 횟수/현재값 변경 입력이다.
const char kFirstName[]="Alpha",kSecondName[]="Bravo",kCalendar[]="Wed Nov  4 01:00:00 2026\n",kFormat[]="~9~0"; // 별도 경계의 고정 ASCII 입력이다.
std::array<std::int32_t,9> calendarFields{0,0,1,4,10,126,3,307,0}; // localtime의 명시 입력이며 원문은9DWORD를 지역 복사한다.
// 원시 바이트를 고정 HEX로 출력해 개행/구분자를 TSV 행에 섞지 않는다.
std::string HexBytes(const std::uint8_t* data,std::size_t size) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 출력만 수행하는16진수 표다.
    for (std::size_t i=0;i<size;++i) { result+=digits[data[i]>>4];result+=digits[data[i]&15]; } // 원문 물리 순서를 그대로 기록한다.
    return result;
}
std::string TextHex(const char* text) { return HexBytes(reinterpret_cast<const std::uint8_t*>(text),std::strlen(text)); } // C문자열의 NUL 전 바이트만 기록한다.
// 복사 공간 안의 실제 장치 또는 CALL 피연산자만 대체한다.
void BindMonitor(std::uint32_t address,const void* observer,bool call=false) {
    if (address<0x410000 || address+5>0x620000) throw std::runtime_error("시간 관측 전용 복사 공간 범위");auto* target=reinterpret_cast<std::uint8_t*>(address);if (call && target[0]!=0xe8) throw std::runtime_error("원문 시계 CALL 불일치");target[0]=call ? 0xe8 : 0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(target+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("시간 관측 코드 캐시 실패");
}
// 현재 시각을 목적8BYTE에 저장하고 원문이 그 뒤 현재 기한을 다시 읽는지 관측한다.
double* __cdecl ObserveMonitorWall(double* output) { *output=wall;wall+=increment;++calls;events+="W,"+std::to_string(std::bit_cast<std::uint64_t>(*output))+";";if ((changes&1U) && calls==1) *reinterpret_cast<double*>(locations.deadline)=10000.0;return output; }
std::int32_t __cdecl ObserveTime(std::int32_t* output) { *output=0x12345678;events+="T;";return *output; } // 실제 time32 경계의 고정 입력이다.
const std::int32_t* __cdecl ObserveLocal(const std::int32_t* input) { if (*input!=0x12345678) throw std::runtime_error("원문 localtime 인자 불일치");events+="L;";return calendarFields.data(); } // 원문의 지역 복사는 직접 실행한다.
const char* __cdecl ObserveAsctime(const std::int32_t* local) { if (!std::equal(calendarFields.begin(),calendarFields.end(),local)) throw std::runtime_error("원문 tm9DWORD 복사 불일치");events+="A;";if (changes&2U) *reinterpret_cast<const char**>(locations.handle)=kSecondName;return kCalendar; } // 현재 handle은 이 복귀 뒤 읽힌다.
// 고정 CRT 문자열 경계의 실제 인자/결과를 기록하며 시간 조건/난수를 계산하지 않는다.
char* __cdecl ObserveLine(char* output,const char* pattern,const char* name,const char* calendar) { events+="P,"+TextHex(pattern)+','+TextHex(name)+','+TextHex(calendar)+";";if (std::snprintf(output,256,pattern,name,calendar)<0) throw std::runtime_error("원문 고정 시간 서식 실패");return output; }
const char* __cdecl ObserveMonitorConfig(const char* key,std::uint32_t a,std::uint32_t b,std::uint32_t c) { events+="Q,"+TextHex(key)+','+std::to_string(a)+','+std::to_string(b)+','+std::to_string(c)+";";if (changes&4U) *reinterpret_cast<const char**>(locations.handle)=kSecondName;return kFormat; } // 시간본문은 네 치환 인자가 아닌 다음 스택 인자로 남는다.
void __cdecl ObserveMonitorEmit(const char* format,const char* line) { events+="E,"+TextHex(format)+','+TextHex(line)+";";if (changes&8U) { *reinterpret_cast<std::int32_t*>(locations.monitor)=0;*reinterpret_cast<std::int32_t*>(locations.alert)=0; }if (changes&16U) *reinterpret_cast<double*>(locations.deadline)=-4.25; } // 실제 통신 대신 두 인자를 관측한다.
int __cdecl ObserveExit(void (__cdecl*)()) { ++exitCalls;return 0; } // CD만 등록하는 무효과 atexit의 OS수명 경계다.
// 임시서식 부모의 실제 thiscall 제출은 현재 front와 본문을 읽은 뒤 선택적으로 current를 바꾼다.
void __fastcall ObserveTemporarySubmit(std::uint32_t manager,void*,const char* line) { const auto offset=locations.current==0x548714 ? 0x14U : 0x10U;events+="S,"+TextHex(reinterpret_cast<const char*>(manager+offset))+','+TextHex(line)+";";if (changes&1U) *reinterpret_cast<std::uint32_t*>(locations.current)=0x602000; }
void InvokeMonitor(std::uint32_t address) { __asm { mov eax,address } __asm { call eax } } // 원문 void 시간 부모를 그대로 호출한다.
void InvokeTemporary(std::uint32_t address,const char* format,const char* line) { __asm { push line } __asm { push format } __asm { mov eax,address } __asm { call eax } __asm { add esp,8 } } // 실제 두 스택 인자를 보존한다.
}
// 모드별 독립 입력을 읽고 실제 원문 메모리/요청만 출력한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=4) throw std::runtime_error("판본/모드/보호PE가 필요합니다");const bool patch=std::wstring(arguments[1])==L"originals",timer=std::wstring(arguments[2])==L"timer";locations=patch ? kPatchMonitor : kCdMonitor;MappedPe image(arguments[3]);checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;*checking=0;action=0;BindAssertion(patch ? 0x4e0620 : 0x490040);
        if (timer) {
            if (patch) { BindMonitor(0x418346,reinterpret_cast<const void*>(&ObserveMonitorWall),true);BindMonitor(0x418360,reinterpret_cast<const void*>(&ObserveMonitorWall),true);BindMonitor(0x4183dd,reinterpret_cast<const void*>(&ObserveMonitorWall),true); }
            else { BindMonitor(0x4ecbcf,reinterpret_cast<const void*>(&ObserveMonitorWall),true);BindMonitor(0x4ecbfb,reinterpret_cast<const void*>(&ObserveMonitorWall),true);BindMonitor(0x4ecc8b,reinterpret_cast<const void*>(&ObserveMonitorWall),true);BindMonitor(0x4f1560,reinterpret_cast<const void*>(&ObserveExit)); }
            BindMonitor(locations.time,reinterpret_cast<const void*>(&ObserveTime));BindMonitor(locations.local,reinterpret_cast<const void*>(&ObserveLocal));BindMonitor(locations.asctime,reinterpret_cast<const void*>(&ObserveAsctime));BindMonitor(locations.format,reinterpret_cast<const void*>(&ObserveLine));BindMonitor(locations.config,reinterpret_cast<const void*>(&ObserveMonitorConfig));BindMonitor(locations.emit,reinterpret_cast<const void*>(&ObserveMonitorEmit));
            std::uint32_t index{},guard{},steps{};std::int32_t monitor{},alert{};std::uint64_t deadline{},first{},delta{};
            // 각 행은 독립 전역이며 한 행 안에서는 모든 시각/현재값을 이어 사용한다.
            while (std::cin>>index>>guard>>deadline>>monitor>>alert>>first>>delta>>steps>>changes) {
                if (steps>32) throw std::runtime_error("시간 관측 반복 범위");events.clear();calls=exitCalls=0;*reinterpret_cast<std::uint32_t*>(locations.guard)=guard;*reinterpret_cast<double*>(locations.deadline)=std::bit_cast<double>(deadline);*reinterpret_cast<std::int32_t*>(locations.monitor)=monitor;*reinterpret_cast<std::int32_t*>(locations.alert)=alert;*reinterpret_cast<const char**>(locations.handle)=kFirstName;wall=std::bit_cast<double>(first);increment=std::bit_cast<double>(delta);const std::uint16_t control=0x027f;__asm { fninit } __asm { fldcw control }
                for (std::uint32_t step=0;step<steps;++step) InvokeMonitor(locations.entry); // 원문 부모를 매번 직접 호출한다.
                std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<guard<<'\t'<<deadline<<'\t'<<monitor<<'\t'<<alert<<'\t'<<first<<'\t'<<delta<<'\t'<<steps<<'\t'<<changes<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.guard)<<'\t'<<std::bit_cast<std::uint64_t>(*reinterpret_cast<const double*>(locations.deadline))<<'\t'<<*reinterpret_cast<const std::int32_t*>(locations.monitor)<<'\t'<<*reinterpret_cast<const std::int32_t*>(locations.alert)<<'\t'<<calls<<'\t'<<exitCalls<<'\t'<<(events.empty() ? "-" : events)<<'\n';
            }
        } else {
            BindMonitor(locations.submit,reinterpret_cast<const void*>(&ObserveTemporarySubmit));std::uint32_t index{},pattern{};
            // 두 실제 manager의 현재 주소/바이트를 입력으로 만들며 wrapper/CRT setter는 원문을 실행한다.
            while (std::cin>>index>>pattern>>changes) {
                events.clear();std::memset(reinterpret_cast<void*>(0x601000),0xab,256);std::memset(reinterpret_cast<void*>(0x602000),0xcd,256);const auto offset=patch ? 0x14U : 0x10U;std::memcpy(reinterpret_cast<char*>(0x601000+offset),"saved-prefix",13);std::memcpy(reinterpret_cast<char*>(0x602000+offset),"second-prefix",14);*reinterpret_cast<std::uint32_t*>(locations.current)=0x601000;std::string format=pattern==0 ? "~9~0" : pattern==1 ? std::string(75,'Z') : std::string();InvokeTemporary(locations.emit,format.c_str(),"Alpha: !calendar\n\n");
                std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<pattern<<'\t'<<changes<<'\t'<<(*reinterpret_cast<const std::uint32_t*>(locations.current)==0x602000 ? 1 : 0)<<'\t'<<HexBytes(reinterpret_cast<const std::uint8_t*>(0x601000),256)<<'\t'<<HexBytes(reinterpret_cast<const std::uint8_t*>(0x602000),256)<<'\t'<<(events.empty() ? "-" : events)<<'\n';
            }
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
