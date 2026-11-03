// 실제 주변음 부모/난수/선택 바람은 보호 PE 바이트로 실행하고 시계·소리·화면 요청만 기록한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain

namespace { // 원본 게임 초기화 없이 독립 콘솔의 전용 payload만 사용한다.
struct AmbientLocations { // 두 판본의 현재 전역/외부 장치 주소다.
    std::uint32_t entry,reset,clock,sound,flash,deadline,kind,forced,dialog,random,wind,x,y,angle; // 주소는 모두 읽기 전용 파일에서 복사한 원문이다.
};
constexpr AmbientLocations kPatchAmbient{0x4180f0,0x4180d0,0x460d70,0x4a9cb0,0x470fa0,0x544d58,0x544d50,0x544d54,0x565dd4,0x532710,0x4dd2b0,0x5e18a8,0x5435b0,0x5e18b0}; //10.78의 실제 부모/전역이다.
constexpr AmbientLocations kCdAmbient{0x4eca20,0x4ec9e0,0x4011b0,0x438b50,0x4b10d0,0x5b7ba0,0x546210,0x546214,0x535e8c,0x5308ec,0x4503f0,0x51fbf8,0x51fbfc,0x51fc00}; //10.72의 실제 부모/전역이다.
AmbientLocations locations{}; // 현재 입력 판본의 실제 주소이며 출력 결과는 계산하지 않는다.
double currentWall{},wallIncrement{}; // 외부 시계의 명시 입력 시퀀스다.
std::uint32_t wallCalls{},ambientAction{}; // 경계 호출 횟수/현재값 변경 입력이다.
// 시계 경계는 요청한 double 물리에 명시 입력만 저장한다. 두 번째 시계 뒤의 현재 종류/강제값 읽기도 관측한다.
double* __cdecl ObserveWall(double* target) {
    *target=currentWall;currentWall+=wallIncrement;++wallCalls;events+="W,"+std::to_string(std::bit_cast<std::uint64_t>(*target))+";";
    if ((ambientAction&4U) && wallCalls==2) { *reinterpret_cast<std::int32_t*>(locations.forced)=1;*reinterpret_cast<std::int32_t*>(locations.kind)=2; }
    return target;
}
// 실제 여섯 음향 인자를 그대로 기록하며 소리 장치/파일은 열지 않는다.
std::uint32_t __cdecl ObserveSound(const char* name,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit) {
    events+="S,"+std::string(name)+','+std::to_string(loop)+','+std::to_string(volume)+','+std::to_string(pan)+','+std::to_string(priority)+','+std::to_string(limit)+";";
    if (ambientAction&1U) *reinterpret_cast<std::uint32_t*>(locations.dialog)=0;
    if (ambientAction&2U) *reinterpret_cast<std::uint32_t*>(locations.dialog)=0x80000000U;
    if (ambientAction&8U) *reinterpret_cast<double*>(locations.deadline)=-4.25;
    return 0;
}
// 화면 프로세스 생성 대신 요청 인자만 기록한다. 실제 부모의 모달 관문은 대체하지 않는다.
void __cdecl ObserveFlash(std::int32_t brightness) { events+="F,"+std::to_string(brightness)+";"; }
// 관측기 자신의 복사본에만 장치 경계의 상대 JMP를 설치한다.
void BindAmbientDevice(std::uint32_t address,const void* observer) {
    if (address<0x410000 || address+5>0x620000) throw std::runtime_error("주변음 전용 복사 공간 범위");
    auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(target+1,&delta,4);
    if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("주변음 장치 경계 캐시 갱신 실패");
}
// CD 시계004011b0은 전용 복사 공간 밖이므로 실제 세 CALL 지점만 동일 시계 경계로 연결한다.
void BindAmbientClock(std::uint32_t call) {
    auto* target=reinterpret_cast<std::uint8_t*>(call);if (call<0x410000 || call+5>0x620000 || target[0]!=0xe8) throw std::runtime_error("주변음 실제 시계 CALL 범위");
    const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveWall)-call-5);std::memcpy(target+1,&delta,4);
    if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("주변음 시계 CALL 캐시 갱신 실패");
}
// 원문 void 부모를 그대로 호출한다. 난수·기한·종류·바람 결과를 host 모델로 대신하지 않는다.
void InvokeAmbient(std::uint32_t address) { __asm { mov eax,address } __asm { call eax } }
}
// 입력 행마다 최초 전역과 외부 시계만 준비한 뒤 실제 메모리 결과/요청 순서를 출력한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
        if (count!=3) throw std::runtime_error("판본과 보호 PE 경로가 필요합니다");const bool patch=std::wstring(arguments[1])==L"originals";locations=patch ? kPatchAmbient : kCdAmbient;
        MappedPe image(arguments[2]);checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;*checking=0;action=0;BindAssertion(patch ? 0x4e0620 : 0x490040);
        if (patch) { BindAmbientClock(0x4180d7);BindAmbientClock(0x4180f8);BindAmbientClock(0x418117); }
        else { BindAmbientClock(0x4ec9e8);BindAmbientClock(0x4eca28);BindAmbientClock(0x4eca48); }
        BindAmbientDevice(locations.sound,reinterpret_cast<const void*>(&ObserveSound));BindAmbientDevice(locations.flash,reinterpret_cast<const void*>(&ObserveFlash));
        std::uint32_t index{},seed{},dialog{},steps{},mode{};std::int32_t kind{},forced{};std::uint64_t deadlineBits{},wallBits{},incrementBits{};
        // 행 사이 원본 상태를 가져오지 않고 행 안의 시계/난수/바람은 끝까지 이어간다.
        while (std::cin>>index>>seed>>deadlineBits>>kind>>forced>>dialog>>wallBits>>incrementBits>>steps>>mode>>ambientAction) {
            if (steps>32 || mode>3) throw std::runtime_error("주변음 CPU 입력 범위");events.clear();wallCalls=0;currentWall=std::bit_cast<double>(wallBits);wallIncrement=std::bit_cast<double>(incrementBits);
            *reinterpret_cast<double*>(locations.deadline)=std::bit_cast<double>(deadlineBits);*reinterpret_cast<std::int32_t*>(locations.kind)=kind;*reinterpret_cast<std::int32_t*>(locations.forced)=forced;*reinterpret_cast<std::uint32_t*>(locations.dialog)=dialog;*reinterpret_cast<std::uint32_t*>(locations.random)=seed;
            *reinterpret_cast<float*>(locations.x)=0;*reinterpret_cast<float*>(locations.y)=1;*reinterpret_cast<float*>(locations.angle)=0;const std::uint16_t control=0x027f;__asm { fninit } __asm { fldcw control }
            if (mode&1U) InvokeAmbient(locations.reset);
            // 결합 모드는 원본 Client처럼 주변음 바로 뒤에서 같은 게임 난수를 쓰는 바람을 호출한다.
            for (std::uint32_t step=0;step<steps;++step) { InvokeAmbient(locations.entry);if (mode&2U) InvokeAmbient(locations.wind); }
            std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<seed<<'\t'<<deadlineBits<<'\t'<<kind<<'\t'<<forced<<'\t'<<dialog<<'\t'<<wallBits<<'\t'<<incrementBits<<'\t'<<steps<<'\t'<<mode<<'\t'<<ambientAction<<'\t'<<std::bit_cast<std::uint64_t>(*reinterpret_cast<const double*>(locations.deadline))<<'\t'<<*reinterpret_cast<const std::int32_t*>(locations.kind)<<'\t'<<*reinterpret_cast<const std::int32_t*>(locations.forced)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.dialog)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.random)<<'\t'<<wallCalls<<'\t'<<(events.empty() ? "-" : events)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.x)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.y)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.angle)<<'\n';
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
