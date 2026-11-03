// 실제 두 판본의 연소 갱신 부모만 CPU로 호출하고 검색·생성·삭제·소리·진단 경계를 관측한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 보호 PE는 읽기 전용이며 입력과 패치는 이 관측기의 전용 payload에만 놓는다.
constexpr std::uint32_t kPool=0x600000,kTypes=0x610000,kVtable=0x61f000,kCreated=0x61e000; // 실제 함수/전역과 겹치지 않는 입력 공간이다.
std::uint32_t slotSize{},burnAddress{},bulfAddress{},first{},second{},created{}; // 현재 판본/검색/생성 장치의 명시 입력이다.
std::int32_t maximum{}; // 최대HP 장치의 현재 입력이며 부모가 먼저 저장한 값과 구별한다.
bool patch{};std::string trace; // 판본 선택과 모든 외부 경계의 순서다.
// 비정렬 raw 필드의 원시 DWORD를 기록한다.
void Store(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); }
// 현재 피해자 슬롯의 절대주소다.
std::uint32_t Victim() { return kPool+50U*slotSize; }
// 입력 슬롯의 모든 바이트를 그대로 직렬화한다.
std::string Bytes(const std::uint8_t* bytes,std::uint32_t length) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 기대값을 계산하지 않는 출력 표다.
    for (std::uint32_t i=0;i<length;++i) { result+=digits[bytes[i]>>4];result+=digits[bytes[i]&15]; } // 예약 필드도 보존한다.
    return result;
}
// TYPE getter는 현재 raw TYPE의 damageEffect 입력만 반환한다.
void* __fastcall ObserveType(std::uint32_t victim,void*) { return reinterpret_cast<void*>(kTypes+*reinterpret_cast<const std::uint8_t*>(victim+10)*0x100U); }
// 실제 최대HP getter의 반환 입력이며 절반 계산은 원문이 수행한다.
std::int32_t __fastcall ObserveMaximum(void*,void*) { return maximum; }
// 진단 뒤의 현재 TYPE/위치/전역 변경과 이미 저장된 HP/첫 검색 결과를 구별한다.
void __cdecl ObserveBurnAssertion(const char* expression,const char* file,std::uint32_t line) {
    trace+="D,"+std::to_string(line)+':'+expression+':'+file+';';
    if (action&2U) { *checking=0;*reinterpret_cast<std::uint8_t*>(Victim()+10)=135;Store(Victim()+14,0x40700000);Store(Victim()+18,0x40900000);Store(Victim()+26,0x7fffffff);maximum=static_cast<std::int32_t>(0x80000000U);Store(bulfAddress,135);Store(burnAddress,61); }
    if (action&4U) { *reinterpret_cast<std::uint8_t*>(Victim()+11)|=1;*checking=0; }
    if (action&8U) first=first ? 0U : 90U;
}
// 첫 검색이 바꾼 현재 checking/TYPE 전역은 두 번째 검색에서 다시 읽어야 한다.
std::uint32_t __cdecl ObserveFind(std::uint32_t victim,std::uint32_t type,std::uint32_t index,std::uint32_t flags) {
    if (victim!=50 || index>1 || flags) throw std::runtime_error("연소 finder 인자");const auto result=index ? second : first;trace+="F,"+std::to_string(victim)+','+std::to_string(type)+','+std::to_string(index)+','+std::to_string(result)+';';
    if (!index && (action&1U)) { *checking=0x80000000;Store(burnAddress,58);Store(Victim()+26,0x7fffffff);maximum=0; }return result;
}
// 실제 생성자 경계는 process 포인터 또는 null을 반환하며 부모가 +4의 Form SID를 읽는다.
void* __cdecl ObserveCreate(std::uint32_t victim) { if (victim!=50) throw std::runtime_error("연소 생성 인자");trace+="C,50,"+std::to_string(created)+';';return created ? reinterpret_cast<void*>(kCreated) : nullptr; }
// 현재 좌표의 원시 float 비트를 보존하고 실제 파일명/loop0 요청을 기록한다.
void __cdecl ObserveSound(float x,float y,const char* name,std::uint32_t loop) { trace+="S,50,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+name+','+std::to_string(loop)+';'; }
// 실제 VT+0x10 thiscall의 현재 Form SID/flags를 관측한다.
void __fastcall ObserveDestroy(std::uint32_t object,void*,std::uint32_t flags) { trace+="X,"+std::to_string((object-kPool)/slotSize)+','+std::to_string(flags)+';'; }
// 외부 장치 진입점만 독립 복사본 안에서 바꾼다.
void BindBurn(std::uint32_t address,const void* function) {
    if (address<0x410000 || address+5>0x620000) throw std::runtime_error("연소 관측 복사 범위");auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(function)-address-5);std::memcpy(target+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("연소 코드 캐시 실패");
}
// 명시 ABI로 호출해 관측 callback 전후 volatile 레지스터를 안전하게 보존한다.
__declspec(noinline) std::uint32_t InvokeBurn() { return reinterpret_cast<std::uint32_t (__thiscall*)(void*)>(patch ? 0x44c060 : 0x462480)(reinterpret_cast<void*>(Victim())); }
}
// 입력 격자만 받고 결과는 실제 원문의 반환/메모리/장치 요청에서 읽는다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=3) throw std::runtime_error("판본과 보호 PE 경로가 필요합니다");patch=std::wstring(arguments[1])==L"originals";MappedPe image(arguments[2]);slotSize=patch ? 50U : 36U;checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;burnAddress=patch ? 0x5410a8 : 0x51c998;bulfAddress=patch ? 0x541274 : 0x51cb60;
        Store(patch ? 0x5c8464 : 0x5395dc,kPool);Store(kVtable+0x10,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveDestroy)));
        BindBurn(patch ? 0x4ac550 : 0x4abf50,reinterpret_cast<const void*>(&ObserveType));BindBurn(patch ? 0x49fce0 : 0x444720,reinterpret_cast<const void*>(&ObserveMaximum));BindBurn(patch ? 0x4b1a90 : 0x4eb310,reinterpret_cast<const void*>(&ObserveFind));BindBurn(patch ? 0x424090 : 0x4a6720,reinterpret_cast<const void*>(&ObserveCreate));BindBurn(patch ? 0x4a9d70 : 0x438c00,reinterpret_cast<const void*>(&ObserveSound));if (patch) BindBurn(0x4e0620,reinterpret_cast<const void*>(&ObserveBurnAssertion));
        std::uint32_t index{},hp{},gate{},type{},free{},effect{};std::int32_t inputMaximum{}; // 행마다 독립인 명시 입력이다.
        while (std::cin>>index>>hp>>inputMaximum>>first>>second>>created>>gate>>type>>free>>effect>>action) { // 초기 상태를 복제하며 결과/절반/분기를 계산하지 않는다.
            maximum=inputMaximum;*checking=gate;Store(burnAddress,14);Store(bulfAddress,135);trace.clear();auto* raw=reinterpret_cast<std::uint8_t*>(Victim());
            for (std::uint32_t byte=0;byte<slotSize;++byte) raw[byte]=static_cast<std::uint8_t>(byte*7U+19U); // 모든 예약 바이트도 비교한다.
            Store(Victim(),kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(free);Store(Victim()+14,0x3fa00000);Store(Victim()+18,0xc0200000);if (patch) Store(Victim()+26,hp);else { const auto word=static_cast<std::uint16_t>(hp);std::memcpy(raw+26,&word,2); }
            Store(kTypes+type*0x100U+0xd4,effect);Store(kTypes+135U*0x100U+0xd4,effect);Store(kPool+90U*slotSize,kVtable);Store(kCreated+4,created);
            std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<hp<<'\t'<<inputMaximum<<'\t'<<first<<'\t'<<second<<'\t'<<created<<'\t'<<gate<<'\t'<<type<<'\t'<<free<<'\t'<<effect<<'\t'<<action;
            const auto result=InvokeBurn();std::cout<<'\t'<<result<<'\t'<<Bytes(raw,slotSize)<<'\t'<<*checking<<'\t'<<*reinterpret_cast<std::uint32_t*>(burnAddress)<<'\t'<<*reinterpret_cast<std::uint32_t*>(bulfAddress)<<'\t'<<maximum<<'\t'<<first<<'\t'<<(trace.empty() ? "-" : trace)<<'\n';
        }return 0;
    } catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
