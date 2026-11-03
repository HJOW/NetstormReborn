// 두 원본의 공급원 목록 등록만 실제 CPU로 호출하고 중복 진단의 정상 복귀를 관측한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 원본 파일은 읽기 전용이며 모든 입력/패치는 독립 관측기의 payload에만 둔다.
constexpr std::uint32_t kListA=0x601000,kListB=0x602000,kWords=16; // 현재/교체 목록과 stale 꼬리를 위한 유효 입력 공간이다.
std::uint32_t pointerAddress{},countAddress{},capacityAddress{},dirtyAddress{},value{}; // 현재 판본의 전역 주소와 호출 인자다.
// 실제 비정렬 DWORD 쓰기이며 목록 판단이나 카운터 결과를 계산하지 않는다.
void StoreMana(std::uint32_t address,std::uint32_t word) { std::memcpy(reinterpret_cast<void*>(address),&word,4); }
// 현재 원문 메모리의 DWORD를 그대로 읽는다.
std::uint32_t ReadMana(std::uint32_t address) { std::uint32_t word{};std::memcpy(&word,reinterpret_cast<const void*>(address),4);return word; }
// 진단 장치가 바꾼 현재 목록/카운터/owner와 부모에 저장된 중복 여부를 구별한다.
void __cdecl ObserveManaAssertion(const char* expression,const char* file,std::uint32_t line) {
    events+="D,"+std::to_string(line)+':'+expression+':'+file+';';
    if (action&1U) *checking=0;
    if (action&2U) { StoreMana(pointerAddress,kListB);StoreMana(countAddress,0);StoreMana(capacityAddress,2); }
    if (action&4U) StoreMana(dirtyAddress,0xffffffff);
    if (action&8U) { for (std::uint32_t index=0;index<kWords;++index) if (ReadMana(kListA+index*4)==value) StoreMana(kListA+index*4,0xdead0000U+index); } // 중복 원소를 지워도 이미 저장한 중복 판정을 다시 계산하지 않는다.
}
// 원문 진단 진입점의 독립 복사본만 외부 장치에 연결한다.
void BindManaAssertion() { auto* target=reinterpret_cast<std::uint8_t*>(0x4e0620);target[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveManaAssertion)-0x4e0625);std::memcpy(target+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("마나 진단 코드 캐시 실패"); }
// 명시 cdecl ABI로 호출해 외부 진단 장치와 원문의 레지스터 계약을 지킨다.
__declspec(noinline) void InvokeMana(bool patch) { reinterpret_cast<void (__cdecl*)(std::uint32_t)>(patch ? 0x472f70 : 0x48a360)(value); }
// 전체16DWORD의 stale 꼬리까지 원시 바이트로 기록한다.
std::string ManaBytes(std::uint32_t address) { constexpr char digits[]="0123456789abcdef";std::string result;const auto* bytes=reinterpret_cast<const std::uint8_t*>(address);for (std::uint32_t index=0;index<kWords*4;++index) { result+=digits[bytes[index]>>4];result+=digits[bytes[index]&15]; }return result; } // 기대값 모델 없는 HEX 출력 표다.
}
// 행마다 새 명시 입력만 쓰고 실제 반환 뒤 메모리/진단 순서를 출력한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=3) throw std::runtime_error("판본과 보호 PE 경로가 필요합니다");const bool patch=std::wstring(arguments[1])==L"originals";MappedPe image(arguments[2]);checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;pointerAddress=patch ? 0x592f84 : 0x567648;capacityAddress=pointerAddress+4;countAddress=pointerAddress+8;dirtyAddress=patch ? 0x55a4a4 : 0x520c10;if (patch) BindManaAssertion();
        std::uint32_t index{},used{},capacity{},pattern{},dirty{},gate{}; // 원문에 전달할 독립 DWORD 입력이다.
        while (std::cin>>index>>value>>used>>capacity>>pattern>>dirty>>gate>>action) { // 결과나 중복 여부는 계산하지 않는다.
            if (used>4) throw std::runtime_error("마나 관측 활성 입력 한계");StoreMana(pointerAddress,kListA);StoreMana(countAddress,used);StoreMana(capacityAddress,capacity);StoreMana(dirtyAddress,dirty);*checking=gate;events.clear();
            for (std::uint32_t at=0;at<kWords;++at) { StoreMana(kListA+at*4,0x11220000U+at);StoreMana(kListB+at*4,0x33440000U+at); } // 두 물리 목록의 stale 꼬리도 각각 구별한다.
            if (used && (pattern&1U)) StoreMana(kListA,value);if (used && (pattern&2U)) StoreMana(kListA+(used-1)*4,value);
            InvokeMana(patch);std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<value<<'\t'<<used<<'\t'<<capacity<<'\t'<<pattern<<'\t'<<dirty<<'\t'<<gate<<'\t'<<action<<'\t'<<(ReadMana(pointerAddress)==kListB ? 1 : 0)<<'\t'<<ReadMana(countAddress)<<'\t'<<ReadMana(capacityAddress)<<'\t'<<ReadMana(dirtyAddress)<<'\t'<<*checking<<'\t'<<ManaBytes(kListA)<<'\t'<<ManaBytes(kListB)<<'\t'<<(events.empty() ? "-" : events)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
