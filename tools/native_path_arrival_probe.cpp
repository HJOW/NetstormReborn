// 실제 공통 도착/항상참 Baseprocess 진단과 일반 finder 분기를 CPU로 관측한다. 게임은 시작하지 않는다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 원문 지역 watch 주소 자체는 출력하지 않는 독립 payload다.
constexpr std::uint32_t kPath=0x570000,kPool=0x600000,kTypes=0x5a0000,kMap=0x5d0000,kParentVt=0x61f000,kPathVt=0x61f100; // 다른 원문 함수와 겹치지 않는 입력 공간이다.
std::uint32_t parentSid{},halted{},arrivalResult{},arrivalDeleted{},blockers{},mutation{},pointX{},pointY{},robustCalls{},findCalls{},nextCalls{};bool deleted{}; // 명시 외부 응답/현재 좌표와 실제 요청 수다.
std::string arrivalEvents; // 실제 호출 순서만 기록한다.
// 비정렬 원문 DWORD 입력을 쓴다.
void ArrivalPut(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); }
// 현재 원문 DWORD를 읽는다.
std::uint32_t ArrivalGet(std::uint32_t address) { std::uint32_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value; }
// 최초 저장 부모의 현재 위치/타입/owner를 바꾸는 외부 입력이다.
void ChangeArrivalParent(std::uint32_t x,std::uint32_t y,std::uint8_t owner) { const auto parent=kPool+parentSid*50;ArrivalPut(parent+14,x);ArrivalPut(parent+18,y);*reinterpret_cast<std::uint8_t*>(parent+10)=2;*reinterpret_cast<std::uint8_t*>(parent+34)=owner; }
// 진단이 돌아온 뒤 현재값 재조회를 구별한다.
void __cdecl ObserveArrivalAssertion(const char* expression,const char* file,std::uint32_t line) { arrivalEvents+="A,"+std::to_string(line)+':'+expression+':'+file+';';if (mutation==10 || mutation==11) { *checking=mutation==10 ? 0U : 0x80000000U;ArrivalPut(kPath+4,5);ArrivalPut(kPath+8,50);ArrivalPut(kPath+0x186ec,0);ArrivalPut(kPath+0x186cc,95);ArrivalPut(kPath+0x186d0,96);*reinterpret_cast<std::uint8_t*>(kPool+parentSid*50+11)=0;ChangeArrivalParent(0x40700000,0x40900000,7); }if (mutation==12 && line==0x69) { ArrivalPut(kPath+4,5);ArrivalPut(kPath+8,90);ArrivalPut(0x5c847c,64); } }
// 부모의 두 Point 출력/angle 입력만 제공하며 실제 Step 내부는 별도 기존 관측 범위다.
std::uint32_t __fastcall ObserveArrivalStep(std::uint32_t path,void*,void* point,void* angle) { if (path!=kPath) throw std::runtime_error("도착 Step this 오류");std::memcpy(point,&pointX,4);std::memcpy(static_cast<std::uint8_t*>(point)+4,&pointY,4);const std::uint32_t zero{};std::memcpy(angle,&zero,4);arrivalEvents+="S,"+std::to_string(pointX)+','+std::to_string(pointY)+';';if (mutation==1) { *checking=0x80000000;ArrivalPut(kPath+4,128); }return 1; }
// 저장 부모의 현재 가상 공간 요청을 관측한다.
void __fastcall ObserveArrivalUnpop(std::uint32_t parent,void*,std::uint32_t flags) { arrivalEvents+="U,"+std::to_string((parent-kPool)/50U)+','+std::to_string(flags)+';'; }
// Pop이 보존한 최종 좌표를 실제 raw에 쓰고 다음 현재 입력을 적용한다.
void __fastcall ObserveArrivalPop(std::uint32_t parent,void*,float x,float y,std::uint32_t flags) { const auto xb=std::bit_cast<std::uint32_t>(x),yb=std::bit_cast<std::uint32_t>(y);arrivalEvents+="P,"+std::to_string((parent-kPool)/50U)+','+std::to_string(xb)+','+std::to_string(yb)+','+std::to_string(flags)+';';ArrivalPut(parent+14,xb);ArrivalPut(parent+18,yb);if (mutation==2) { *checking=0;ArrivalPut(kPath+4,128); }if (mutation==3) *reinterpret_cast<std::uint8_t*>(parent+11)=3; }
// 실제0041be80을 그대로 호출해 진단과 항상1 반환을 보존한다. 부모 CALL 피연산자만 연결한다.
__declspec(noinline) std::uint32_t __fastcall ObserveArrivalRobust(std::uint32_t path,void*,std::uint32_t live) { ++robustCalls;arrivalEvents+="Q,"+std::to_string(ArrivalGet(path+4))+','+std::to_string(ArrivalGet(path+8))+';';return reinterpret_cast<std::uint32_t (__thiscall*)(void*,std::uint32_t)>(0x41be80)(reinterpret_cast<void*>(path),live); }
// 공통 가상Halt의 명시 결과/현재 watch 해제 및 반환 뒤 입력이다.
std::uint32_t __fastcall ObserveArrivalHalt(std::uint32_t path,void*) { if (path!=kPath) throw std::runtime_error("도착 Halt this 오류");arrivalEvents+="T,"+std::to_string(halted)+';';ArrivalPut(kPath+0x186ec,0);if (mutation==4) *checking=0;if (mutation==5) *checking=0x80000000;if (mutation==6) ArrivalPut(kPath+0x186ec,0x12345678);if (mutation==7) { ArrivalPut(kPath+0x186cc,85);ArrivalPut(kPath+0x186d0,86);ChangeArrivalParent(0x40700000,0x40900000,7); }return halted; }
// 현재 두 target으로 호출한 부모+3c와 원문 지역 watch의 삭제 통지를 관측한다.
std::uint32_t __fastcall ObserveArrivalParent(std::uint32_t parent,void*,std::uint32_t a,std::uint32_t b) { arrivalEvents+="R,"+std::to_string((parent-kPool)/50U)+','+std::to_string(a)+','+std::to_string(b)+','+std::to_string(arrivalResult)+';';if (mutation==8) { *checking=0;ChangeArrivalParent(0x40b00000,0x40d00000,6); }if (mutation==9) { *checking=0x80000000;ArrivalPut(kPath+4,128); }if (arrivalDeleted) { const auto watch=ArrivalGet(kPath+0x186ec);if (!watch) throw std::runtime_error("도착 원문 watch 미설치");ArrivalPut(watch,1);deleted=true; }return arrivalResult; }
// 실제 제거 요청만 관측하며 C++ Host 삭제는 별도 검사에서 실행한다.
void __fastcall ObserveArrivalKill(std::uint32_t path,void*,std::uint32_t flags) { arrivalEvents+="K,"+std::to_string(ArrivalGet(path+4))+','+std::to_string(flags)+';';deleted=true; }
// 원문 iterator의 실제 현재 SID 필드에 명시 후보를 제공한다.
void __fastcall ObserveArrivalFind(std::uint32_t iterator,void*,float x,float y,std::uint32_t flags) { ++findCalls;arrivalEvents+="F,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags)+';';ArrivalPut(iterator+0x34,blockers==4 ? 71U : blockers>=2 ? 70U : 0U); }
// 실제 다음 요청에서만 다음 후보 입력을 제공한다.
void __fastcall ObserveArrivalNext(std::uint32_t iterator,void*) { ++nextCalls;const auto old=ArrivalGet(iterator+0x34);const auto next=old==70 && blockers==3 ? 71U : 0U;ArrivalPut(iterator+0x34,next);arrivalEvents+="N,"+std::to_string(old)+','+std::to_string(next)+';'; }
// 삭제/진단 뒤 저장 부모의 현재 metadata로 호출하는 겹침 정리를 관측한다.
void __cdecl ObserveArrivalClear(float x,float y,std::uint32_t type,std::uint32_t owner) { arrivalEvents+="C,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(type)+','+std::to_string(owner)+';'; }
// 외부 함수의 독립 복사본에만 상대 JMP를 둔다.
void BindArrival(std::uint32_t address,const void* observer) { auto* code=reinterpret_cast<std::uint8_t*>(address);code[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(code+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),code,5)) throw std::runtime_error("도착 코드 캐시 실패"); }
// 원문 robust 함수 자체를 바꾸지 않고 알려진 CALL 피연산자만 연결한다.
void BindArrivalCall(std::uint32_t address,const void* observer) { auto* code=reinterpret_cast<std::uint8_t*>(address);if (code[0]!=0xe8) throw std::runtime_error("도착 CALL 피연산자 오류");const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(code+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),code,5)) throw std::runtime_error("도착 CALL 코드 캐시 실패"); }
// 명시 thiscall ABI로 원문 공통 도착만 호출한다.
__declspec(noinline) void InvokeArrival() { reinterpret_cast<void (__thiscall*)(void*)>(0x48b340)(reinterpret_cast<void*>(kPath)); }
}
// 명시 입력 격자와 실제 요청/현재 필드만 출력한다. 원본 파일/실행 초기화를 변경하지 않는다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);ArrivalPut(0x5c8464,kPool);ArrivalPut(0x59ab20,kTypes);ArrivalPut(0x5c7c44,kMap);ArrivalPut(0x5325b4,0x200);ArrivalPut(kParentVt+0x48,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveArrivalUnpop)));ArrivalPut(kParentVt+0x90,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveArrivalPop)));ArrivalPut(kParentVt+0x3c,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveArrivalParent)));ArrivalPut(kPathVt+0x28,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveArrivalHalt)));
        BindArrival(0x48a330,reinterpret_cast<const void*>(&ObserveArrivalStep));BindArrival(0x4e0620,reinterpret_cast<const void*>(&ObserveArrivalAssertion));BindArrival(0x41bf10,reinterpret_cast<const void*>(&ObserveArrivalKill));BindArrival(0x4202f0,reinterpret_cast<const void*>(&ObserveArrivalFind));BindArrival(0x4b1810,reinterpret_cast<const void*>(&ObserveArrivalNext));BindArrival(0x443800,reinterpret_cast<const void*>(&ObserveArrivalClear));
        for (const auto site:{0x48b3c6U,0x48b3eaU,0x48b4d2U,0x48b5f6U,0x48b626U}) BindArrivalCall(site,reinterpret_cast<const void*>(&ObserveArrivalRobust)); // 실제 다섯 robust CALL만 관측한다.
        std::uint32_t index{},gate{},form{},capacity{},flags{},watch{},finished{}; // 행마다 독립인 현재 DWORD 입력이다.
        while (std::cin>>index>>gate>>parentSid>>form>>capacity>>flags>>halted>>arrivalResult>>arrivalDeleted>>blockers>>mutation>>watch>>finished>>pointX>>pointY) { // 결과 분기를 모델로 계산하지 않는다.
            std::memset(reinterpret_cast<void*>(kPath),0,0x186f0);std::memset(reinterpret_cast<void*>(kMap),0,65536);ArrivalPut(kPath,kPathVt);ArrivalPut(kPath+4,form);ArrivalPut(kPath+8,parentSid);ArrivalPut(kPath+0x186cc,60);ArrivalPut(kPath+0x186d0,61);ArrivalPut(kPath+0x186c8,finished);ArrivalPut(kPath+0x186ec,watch);ArrivalPut(0x5c847c,capacity);*checking=gate;
            for (const auto sid:{0U,5U,50U,70U,71U,90U,128U}) { const auto raw=kPool+sid*50;std::memset(reinterpret_cast<void*>(raw),0,50);ArrivalPut(raw,kParentVt);*reinterpret_cast<std::uint8_t*>(raw+10)=sid==70 ? 2 : sid==71 ? 3 : 1;*reinterpret_cast<std::uint8_t*>(raw+34)=2;ArrivalPut(raw+14,0x3fa00000);ArrivalPut(raw+18,0xc0200000); } // 유효 물리 객체와 의도한 예약0/논리 범위밖 입력이다.
            const auto raw=kPool+parentSid*50;*reinterpret_cast<std::uint8_t*>(raw+11)=static_cast<std::uint8_t>(flags);ArrivalPut(kTypes+500+0xec,0);ArrivalPut(kTypes+1000+0xec,0);ArrivalPut(kTypes+1500+0xec,0x200);*reinterpret_cast<std::uint8_t*>(kMap+21*256+11)=blockers ? 8 : 0;deleted=false;arrivalEvents.clear();robustCalls=findCalls=nextCalls=0;
            std::cout<<index<<'\t'<<gate<<'\t'<<parentSid<<'\t'<<form<<'\t'<<capacity<<'\t'<<flags<<'\t'<<halted<<'\t'<<arrivalResult<<'\t'<<arrivalDeleted<<'\t'<<blockers<<'\t'<<mutation<<'\t'<<watch<<'\t'<<finished<<'\t'<<pointX<<'\t'<<pointY;InvokeArrival();std::cout<<'\t'<<deleted<<'\t'<<*checking<<'\t'<<ArrivalGet(kPath+4)<<'\t'<<ArrivalGet(kPath+8)<<'\t'<<ArrivalGet(0x5c847c)<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+11))<<'\t'<<ArrivalGet(kPath+0x186c8)<<'\t'<<(ArrivalGet(kPath+0x186ec)!=0)<<'\t'<<ArrivalGet(kPath+0x186cc)<<'\t'<<ArrivalGet(kPath+0x186d0)<<'\t'<<ArrivalGet(raw+14)<<'\t'<<ArrivalGet(raw+18)<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+10))<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+34))<<'\t'<<robustCalls<<'\t'<<findCalls<<'\t'<<nextCalls<<'\t'<<(arrivalEvents.empty() ? "-" : arrivalEvents)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
