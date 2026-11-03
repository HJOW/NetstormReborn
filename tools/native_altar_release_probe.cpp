// 실제 패치00448bd0만 CPU로 호출하며 해방/공간/방향/진단 경계를 관측한다. 사제 본체는 실행하지 않는다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 보호 PE는 읽기 전용이며 모든 입력/패치는 관측기의 전용 payload다.
constexpr std::uint32_t kPool=0x600000,kAltar=kPool+50*50,kTable=0x610000,kVtable=0x61f000; // 다른 실제 코드/전역과 겹치지 않는 객체/TYPE/가상표 입력 공간이다.
std::uint32_t found{},frameBase{},directions{};std::int32_t firstDirection{},secondDirection{}; // 최초 finder 결과·현재 프레임 표·두 방향 getter의 명시 입력이다.
std::string releaseEvents; // 실제 호출 순서를 기록하며 결과를 모델로 만들지 않는다.
// 비정렬 원문 DWORD 입력을 쓴다.
void StoreRelease(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); }
// 실제 메모리의 DWORD를 그대로 읽는다.
std::uint32_t ReadRelease(std::uint32_t address) { std::uint32_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value; }
// 현재 TYPE이 가리키는 입력 record다. 프레임 표는 원문 부모가 직접 읽는다.
std::uint32_t ReleaseType(std::uint32_t type) { if (type<164 || type>167) throw std::runtime_error("해방 TYPE 입력 범위");return kTable+(type-160)*0x200U; }
// 원문 typed finder를 관측하며 진단 이전에 결과 SID가 저장되도록 한다.
std::uint32_t __cdecl ObserveReleaseFind(float x,float y,std::uint32_t type) { const auto result=found;releaseEvents+="F,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(type)+','+std::to_string(result)+';';if (action&1U) { *checking=0x80000000;StoreRelease(kAltar+14,0x40b00000);StoreRelease(kAltar+18,0x40d00000); }return result; }
// 첫 방향이12가 아니면 두 번째 getter를 호출하는 원문 순서를 관측한다.
std::int32_t __fastcall ObserveReleaseDirection(std::uint32_t object,void*) { if (object!=kAltar) throw std::runtime_error("해방 방향 this 오류");const auto result=directions ? secondDirection : firstDirection;releaseEvents+="G,"+std::to_string(directions)+','+std::to_string(result)+';';if (!directions && (action&2U)) *checking=0;++directions;return result; }
// 진단 뒤 current checking/유효 범위/TYPE/위치와 저장 finder 결과를 구별한다.
void __cdecl ObserveReleaseAssertion(const char* expression,const char* file,std::uint32_t line) { releaseEvents+="A,"+std::to_string(line)+':'+expression+':'+file+';';if (action&4U) { *checking=0;StoreRelease(0x5c847c,2);*reinterpret_cast<std::uint8_t*>(kAltar+10)=165;StoreRelease(kAltar+14,0x40700000);StoreRelease(kAltar+18,0x40900000); }if (action&32U) found=found ? 0U : 127U; }
// 실제 사제 해방은 실행하지 않고 저장 다이스 포인터와 후속 현재 제단 변화를 관측한다.
void __fastcall ObserveReleaseDais(std::uint32_t object,void*) { releaseEvents+="R,"+std::to_string((object-kPool)/50U)+';';if (action&8U) { *reinterpret_cast<std::uint8_t*>(kAltar+10)=166;StoreRelease(kAltar+14,0x40900000);StoreRelease(0x5c847c,1);found=0; } }
// 가상 Unpop 뒤 TYPE이 바뀌면 이후 plain P frame은 현재 TYPE에서 가져와야 한다.
void __fastcall ObserveReleaseUnpop(std::uint32_t object,void*,std::uint32_t flags) { if (object!=kAltar || flags) throw std::runtime_error("해방 Unpop 인자");releaseEvents+="U,50,0;";if (action&16U) { *reinterpret_cast<std::uint8_t*>(kAltar+10)=167;StoreRelease(kAltar+18,0xc0d00000); } }
// 표시 프레임 쓰기 뒤의 같은 가상 Repop을 관측한다.
void __fastcall ObserveReleaseRepop(std::uint32_t object,void*,std::uint32_t flags) { if (object!=kAltar || flags) throw std::runtime_error("해방 Repop 인자");releaseEvents+="P,50,0;"; }
// 실제 TYPE getter의 반환 입력과 호출 순서만 관측한다.
void* __fastcall ObserveReleaseType(std::uint32_t object,void*) { if (object!=kAltar) throw std::runtime_error("해방 TYPE this");const auto type=*reinterpret_cast<const std::uint8_t*>(object+10);const auto record=ReleaseType(type);releaseEvents+="T,"+std::to_string(type)+','+std::to_string(ReadRelease(record+0x16c))+';';return reinterpret_cast<void*>(record); }
// 외부 경계 진입점의 독립 복사본만 JMP로 바꾼다.
void BindRelease(std::uint32_t address,const void* observer) { auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(target+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("해방 코드 캐시 실패"); }
// 명시 thiscall ABI로 원문을 호출하며 상위 게임/의식 초기화는 실행하지 않는다.
__declspec(noinline) void InvokeRelease() { reinterpret_cast<void (__thiscall*)(void*)>(0x448bd0)(reinterpret_cast<void*>(kAltar)); }
// 전체50BYTE의 예약 필드까지 그대로 직렬화한다.
std::string ReleaseBytes() { constexpr char digits[]="0123456789abcdef";std::string result;const auto* raw=reinterpret_cast<const std::uint8_t*>(kAltar);for (std::uint32_t index=0;index<50;++index) { result+=digits[raw[index]>>4];result+=digits[raw[index]&15]; }return result; } // 출력 전용 HEX 표/바이트 순회다.
}
// 독립 입력 격자와 실제 반환 뒤 raw/전역/외부 호출만 stdout으로 기록한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호 패치 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);StoreRelease(0x5c8464,kPool);StoreRelease(0x5412ec,165);StoreRelease(kVtable+0x48,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveReleaseUnpop)));StoreRelease(kVtable+0x4c,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveReleaseRepop)));BindRelease(0x4b1fa0,reinterpret_cast<const void*>(&ObserveReleaseFind));BindRelease(0x4ad710,reinterpret_cast<const void*>(&ObserveReleaseDirection));BindRelease(0x4e0620,reinterpret_cast<const void*>(&ObserveReleaseAssertion));BindRelease(0x448500,reinterpret_cast<const void*>(&ObserveReleaseDais));BindRelease(0x4ac550,reinterpret_cast<const void*>(&ObserveReleaseType));
        std::uint32_t index{},gate{},capacity{}; // 행마다 독립인 명시 DWORD 입력이다.
        while (std::cin>>index>>firstDirection>>secondDirection>>gate>>found>>capacity>>frameBase>>action) { // 기대 분기/프레임 결과는 계산하지 않는다.
            *checking=gate;StoreRelease(0x5c847c,capacity);directions=0;releaseEvents.clear();auto* raw=reinterpret_cast<std::uint8_t*>(kAltar);for (std::uint32_t byte=0;byte<50;++byte) raw[byte]=static_cast<std::uint8_t>(byte*7U+19U); // 예약 바이트의 같은 명시 입력이다.
            StoreRelease(kAltar,kVtable);raw[10]=164;raw[11]=0;StoreRelease(kAltar+14,0x3fa00000);StoreRelease(kAltar+18,0xc0200000);StoreRelease(kAltar+36,0x55667788);
            for (std::uint32_t type=164;type<=167;++type) StoreRelease(ReleaseType(type)+0x16c,frameBase+type*17U); // TYPE getter가 제공할 서로 다른 현재 P 첫번호 입력이다.
            std::cout<<index<<'\t'<<firstDirection<<'\t'<<secondDirection<<'\t'<<gate<<'\t'<<found<<'\t'<<capacity<<'\t'<<frameBase<<'\t'<<action;InvokeRelease();std::cout<<'\t'<<ReleaseBytes()<<'\t'<<*checking<<'\t'<<found<<'\t'<<ReadRelease(0x5c847c)<<'\t'<<directions<<'\t'<<(releaseEvents.empty() ? "-" : releaseEvents)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
