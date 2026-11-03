// 실제004c9350 숫자 확인의 AI/미선택 가지를 호출하며 통신과 게임 초기화는 실행하지 않는다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
#include <cctype>
#include <cstdlib>
namespace { // 모든 메모리 입력은 이 관측기 전용 payload에만 둔다.
constexpr std::uint32_t kDialog=0x600000,kPlayers=0x601000; // 실제 창+d4와9개 Player 입력 저장소다.
std::string inputText,moneyEvents; // 원문 CopyText 입력과 실제 호출 기록이다.
std::uint32_t firstMoney{},secondMoney{},moneyReads{}; // 두 GetSp 응답의 float 비트를 구분한다.
// 원본 전역/창 입력에 DWORD를 그대로 쓴다.
void MoneyPut(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); }
// 실제 현재 전역/창 DWORD를 기록한다.
std::uint32_t MoneyGet(std::uint32_t address) { std::uint32_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value; }
// 외부 텍스트 복사의 ABI와256BYTE 상한만 공급한다.
void __fastcall CopyMoneyText(void*,void*,char* output,std::uint32_t capacity) { if (capacity!=256 || inputText.size()>=capacity) throw std::runtime_error("송금 문자 입력 범위");std::memcpy(output,inputText.c_str(),inputText.size()+1); }
// 원문의 CRT 경계에 같은 ASCII 문자열 검색을 제공한다.
char* __cdecl MoneyFindChar(char* text,int character) { return std::strchr(text,character); }
// 원문은 끝 NUL도 조회하며 여기서는 ASCII 입력만 사용한다.
int __cdecl MoneySpace(int character) { return std::isspace(character); }
// Windows32 long 변환을 제공하며 출력 모델은 만들지 않는다.
long __cdecl MoneyLong(const char* text) { return std::atol(text); }
// 첫/두 번째 실제 SP 조회 뒤 checking/미션/대상 변경 입력을 적용한다.
void __cdecl ReadMoney(std::uint32_t owner,void* output) { const auto index=moneyReads++;const auto bits=index ? secondMoney : firstMoney;moneyEvents+="M,"+std::to_string(owner)+','+std::to_string(bits)+';';std::memcpy(output,&bits,4);if ((!index && (action&1U)) || (index && (action&2U))) { *checking=0x80000000;MoneyPut(0x594fa4,0);MoneyPut(kDialog+0xd4,index ? 4U : 3U); } }
// 실제 진단은 현재 상태를 바꿔도 이미 저장한 EBX 대상 번호를 바꾸지 않는다.
void __cdecl MoneyAssertion(const char* expression,const char* file,std::uint32_t line) { moneyEvents+="A,"+std::to_string(line)+':'+expression+':'+file+';';if (action&4U) { *checking=0;MoneyPut(0x594fa4,1);MoneyPut(kDialog+0xd4,8); } }
// 실제 차감 요청 비트와 그 뒤 원문이 보존할 대상을 관측한다.
void __cdecl MoneyAddLocal(float amount) { moneyEvents+="L,"+std::to_string(std::bit_cast<std::uint32_t>(amount))+';';if (action&8U) MoneyPut(kDialog+0xd4,5); }
// AI 지갑 요청만 관측하며 네트워크/실제 AI 프레임은 진행하지 않는다.
void __cdecl MoneyWallet(std::uint32_t owner,std::int32_t amount) { moneyEvents+="W,"+std::to_string(owner)+','+std::to_string(amount)+';'; }
// 연결 플레이어/패킷 가지는 입력에서 제외하며 도달하면 즉시 실패한다.
void __cdecl RejectMoneyNetwork() { throw std::runtime_error("제외한 멀티플레이 송금 가지 도달"); }
// 외부 함수의 독립 복사본에만 JMP 경계를 둔다.
void BindMoney(std::uint32_t address,const void* observer) { auto* bytes=reinterpret_cast<std::uint8_t*>(address);bytes[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(bytes+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),bytes,5)) throw std::runtime_error("송금 코드 캐시 실패"); }
// payload보다 낮은 원문 함수는 관측기 코드 영역을 덮지 않고 부모 CALL 피연산자만 연결한다.
void BindMoneyCall(std::uint32_t address,const void* observer) { auto* bytes=reinterpret_cast<std::uint8_t*>(address);if (bytes[0]!=0xe8) throw std::runtime_error("송금 실제 CALL 피연산자 오류");const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(bytes+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),bytes,5)) throw std::runtime_error("송금 CALL 코드 캐시 실패"); }
// 숨은 thiscall 인자와 반환값을 같은 ABI로 유지한다.
__declspec(noinline) std::uint32_t InvokeMoney() { return reinterpret_cast<std::uint32_t (__thiscall*)(void*)>(0x4c9350)(reinterpret_cast<void*>(kDialog)); }
// 입력 HEX만 ASCII로 복원하며 분기 결과는 계산하지 않는다.
std::string MoneyText(std::string_view hex) { if (hex=="-") return {};std::string text;for (std::size_t index=0;index<hex.size();index+=2) text+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16));return text; } // 입력의 두 글자를 한BYTE로 순회한다.
}
// 명시 격자의 현재 전역/반환/호출만 출력하며 원본 실행파일을 시작하지 않는다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);MoneyPut(0x59534c,kPlayers);MoneyPut(kDialog+0xd0,0x600800);
        BindMoney(0x451e80,reinterpret_cast<const void*>(&CopyMoneyText));BindMoney(0x4e63e0,reinterpret_cast<const void*>(&MoneyFindChar));BindMoney(0x4e701a,reinterpret_cast<const void*>(&MoneySpace));BindMoney(0x4e66de,reinterpret_cast<const void*>(&MoneyLong));BindMoneyCall(0x4c9404,reinterpret_cast<const void*>(&ReadMoney));BindMoneyCall(0x4c9432,reinterpret_cast<const void*>(&ReadMoney));BindMoney(0x4e0620,reinterpret_cast<const void*>(&MoneyAssertion));BindMoneyCall(0x4c94ac,reinterpret_cast<const void*>(&MoneyAddLocal));BindMoney(0x413bb0,reinterpret_cast<const void*>(&MoneyWallet));BindMoney(0x41dff0,reinterpret_cast<const void*>(&RejectMoneyNetwork));BindMoney(0x4e2dc0,reinterpret_cast<const void*>(&RejectMoneyNetwork));
        std::uint32_t index{},known{},gate{},mission{},flags{},editor{},target{},local{};std::string text; // 행마다 독립인 현재 상태/숫자 문자열이다.
        while (std::cin>>index>>text>>known>>firstMoney>>secondMoney>>gate>>mission>>flags>>editor>>target>>local>>action) { // 연결 플레이어 전용 입력/게임 프레임은 실행하지 않는다.
            if ((flags&1U) && !(flags&0x20U)) throw std::runtime_error("멀티플레이 입력 제외");inputText=MoneyText(text);moneyEvents.clear();moneyReads=0;*checking=gate;MoneyPut(0x595344,known);MoneyPut(0x594fa4,mission);MoneyPut(0x5c85a4,editor);MoneyPut(0x540c70,local);MoneyPut(kDialog+0xd4,target);
            for (std::uint32_t owner=0;owner<9;++owner) MoneyPut(kPlayers+owner*180+0x50,flags); // 대상 변경 후에도 같은 명시 flags 입력을 제공한다.
            std::cout<<index<<'\t'<<text<<'\t'<<known<<'\t'<<firstMoney<<'\t'<<secondMoney<<'\t'<<gate<<'\t'<<mission<<'\t'<<flags<<'\t'<<editor<<'\t'<<target<<'\t'<<local<<'\t'<<action;const auto result=InvokeMoney();std::cout<<'\t'<<result<<'\t'<<*checking<<'\t'<<MoneyGet(0x594fa4)<<'\t'<<MoneyGet(kDialog+0xd4)<<'\t'<<moneyReads<<'\t'<<(moneyEvents.empty() ? "-" : moneyEvents)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
