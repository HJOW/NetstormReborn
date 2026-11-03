// 실제00489ff0의 부모 Halt/진단/재지정을 CPU로 호출한다. 게임·GUI·통신은 실행하지 않는다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 원문 stack watch 값 자체는 출력하지 않고 삭제/호출/현재 필드를 관측한다.
constexpr std::uint32_t kPath=0x570000,kPool=0x600000,kTable=0x5a0000,kVtable=0x61f000; // 관측기 payload 안의 별도 경로/풀/타입/가상표 입력이다.
std::uint32_t haltResult{},findFirst{},findSecond{},choice{},findCalls{};bool deleted{}; // 명시 외부 반환/삭제 입력과 실제 조회 수다.
std::string haltEvents; // 결과 모델 없이 실제 외부 호출을 기록한다.
// 원문의 비정렬 DWORD 입력을 쓴다.
void HaltPut(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); }
// 실제 현재 DWORD를 읽는다.
std::uint32_t HaltGet(std::uint32_t address) { std::uint32_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value; }
// 원문 부모가 설치한 실제 지역 deadFlag에 삭제를 통지한다.
void MarkHaltDeleted() { const auto flag=HaltGet(kPath+0x186ec);if (!flag) throw std::runtime_error("Halt 원문 지역 watch 미설치");HaltPut(flag,1);deleted=true; }
// 첫 가상 결과 뒤 현재 checking/권한/저장 부모와 경로 부모의 차이를 입력으로 바꾼다.
std::uint32_t __fastcall ObserveParentHalt(std::uint32_t parent,void*) { if (parent!=kPool+50*50) throw std::runtime_error("Halt 최초 부모 오류");haltEvents+="H,50,"+std::to_string(haltResult)+';';if (action&1U) MarkHaltDeleted();if (action&2U) *checking=0x80000000;if (action&4U) *checking=0;if (action&8U) HaltPut(0x540bc4,HaltGet(0x540bc4) ? 0U : 1U);if (action&16U) *reinterpret_cast<std::uint8_t*>(parent+40)^=0x20;if (action&32U) *reinterpret_cast<std::uint8_t*>(parent+10)=2;if (action&64U) HaltPut(kPath+8,51);return haltResult; }
// 실제 진단 문자열/줄을 읽고 정상 복귀 뒤 현재 전역 변경을 남긴다.
void __cdecl ObserveHaltAssertion(const char* expression,const char* file,std::uint32_t line) { haltEvents+="A,"+std::to_string(line)+':'+expression+':'+file+';';if (action&512U) { *checking=0;HaltPut(0x540bc4,0);HaltPut(kPath+8,52); } }
// flags0의 실제 자기 제거 요청이며 상위 Host 검사는 별도 실제 C++ 검사에서 수행한다.
void __fastcall ObserveHaltKill(std::uint32_t path,void*,std::uint32_t flags) { if (path!=kPath || flags) throw std::runtime_error("Halt 제거 인자 오류");haltEvents+="K,"+std::to_string(HaltGet(path+4)) +",0;";deleted=true; }
// 원문 재지정의 현재 내용물 조회 입력과 순서를 관측한다.
std::uint32_t __fastcall ObserveHaltFind(std::uint32_t parent,void*,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { haltEvents+="F,"+std::to_string((parent-kPool)/50U)+','+std::to_string(mask)+','+std::to_string(type)+','+std::to_string(index)+';';return findCalls++ ? findSecond : findFirst; }
// 실제 귀환 후보 선택의 외부 반환이다.
std::uint32_t __fastcall ObserveHaltReturn(std::uint32_t parent,void*) { haltEvents+="C,"+std::to_string((parent-kPool)/50U)+';';return choice; }
// 실제 간헐천 후보 선택의 외부 반환이다.
std::uint32_t __fastcall ObserveHaltGeyser(std::uint32_t parent,void*) { haltEvents+="Y,"+std::to_string((parent-kPool)/50U)+';';return choice; }
// 두 새 이동은 원문 watch를 통해 자신을 삭제할 수 있다.
void HaltGo(char name,std::uint32_t parent,std::uint32_t target) { haltEvents+=name;haltEvents+=','+std::to_string((parent-kPool)/50U)+','+std::to_string(target)+';';if (action&128U) MarkHaltDeleted(); }
void __fastcall ObserveHaltGoReturn(std::uint32_t parent,void*,std::uint32_t target) { HaltGo('R',parent,target); } //004276a0 경계를 관측한다.
void __fastcall ObserveHaltGoGeyser(std::uint32_t parent,void*,std::uint32_t target) { HaltGo('G',parent,target); } //00427600 경계를 관측한다.
// 외부 함수의 프로세스 복사본만 상대 JMP로 연결한다.
void BindHalt(std::uint32_t address,const void* observer) { auto* code=reinterpret_cast<std::uint8_t*>(address);code[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(code+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),code,5)) throw std::runtime_error("Halt 코드 캐시 실패"); }
// 명시 thiscall bool ABI로 원문 부모만 실행한다.
__declspec(noinline) bool InvokePathHalt() { return reinterpret_cast<bool (__thiscall*)(void*)>(0x489ff0)(reinterpret_cast<void*>(kPath)); }
}
// 독립 입력 격자의 실제 반환/현재 전역·필드와 외부 호출만 기록한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);HaltPut(0x5c8464,kPool);HaltPut(0x59ab20,kTable);HaltPut(0x5412c0,154);HaltPut(kVtable+0x38,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveParentHalt)));
        BindHalt(0x4e0620,reinterpret_cast<const void*>(&ObserveHaltAssertion));BindHalt(0x41bf10,reinterpret_cast<const void*>(&ObserveHaltKill));BindHalt(0x4ac9f0,reinterpret_cast<const void*>(&ObserveHaltFind));BindHalt(0x428ea0,reinterpret_cast<const void*>(&ObserveHaltReturn));BindHalt(0x429420,reinterpret_cast<const void*>(&ObserveHaltGeyser));BindHalt(0x4276a0,reinterpret_cast<const void*>(&ObserveHaltGoReturn));BindHalt(0x427600,reinterpret_cast<const void*>(&ObserveHaltGoGeyser));
        std::uint32_t index{},gate{},boss{},form{},forced{},genus{},targets{}; // 행마다 독립인 명시 DWORD 입력이다.
        while (std::cin>>index>>haltResult>>gate>>boss>>form>>forced>>genus>>targets>>findFirst>>findSecond>>choice>>action) { // 원본 분기/반환 결과는 계산하지 않는다.
            std::memset(reinterpret_cast<void*>(kPath),0,0x186f0);HaltPut(kPath+4,form);HaltPut(kPath+8,50);HaltPut(kPath+0x186cc,(targets&1U) ? 60U : 0U);HaltPut(kPath+0x186d0,(targets&2U) ? 61U : 0U);if (action&256U) HaltPut(kPath+0x186ec,0x12345678);HaltPut(0x540bc4,boss);*checking=gate;HaltPut(kTable+500+0xec,genus);HaltPut(kTable+1000+0xec,genus^0x30000U);
            for (const auto sid:{50U,51U,52U,60U,61U}) { const auto raw=kPool+sid*50;std::memset(reinterpret_cast<void*>(raw),0,50);HaltPut(raw,kVtable);*reinterpret_cast<std::uint8_t*>(raw+10)=1;*reinterpret_cast<std::uint8_t*>(raw+11)=sid>=60 ? 2U : 0U; } // 유효 부모/죽은 대상의 실제 바이트 입력이다.
            *reinterpret_cast<std::uint8_t*>(kPool+50*50+40)=static_cast<std::uint8_t>(forced);haltEvents.clear();findCalls=0;deleted=false;
            std::cout<<index<<'\t'<<haltResult<<'\t'<<gate<<'\t'<<boss<<'\t'<<form<<'\t'<<forced<<'\t'<<genus<<'\t'<<targets<<'\t'<<findFirst<<'\t'<<findSecond<<'\t'<<choice<<'\t'<<action;const auto result=InvokePathHalt();std::cout<<'\t'<<result<<'\t'<<deleted<<'\t'<<*checking<<'\t'<<HaltGet(0x540bc4)<<'\t'<<HaltGet(kPath+8)<<'\t'<<HaltGet(kPath+4)<<'\t'<<HaltGet(kPath+0x186cc)<<'\t'<<HaltGet(kPath+0x186d0)<<'\t'<<static_cast<unsigned>(*reinterpret_cast<const std::uint8_t*>(kPool+50*50+10))<<'\t'<<static_cast<unsigned>(*reinterpret_cast<const std::uint8_t*>(kPool+50*50+40))<<'\t'<<findCalls<<'\t'<<(haltEvents.empty() ? "-" : haltEvents)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
