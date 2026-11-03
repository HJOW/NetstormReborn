// 기존 실제CPU PE 복사/진단 장치만 재사용하고 Wind/Random/fpatan 부모는 원본 바이트를 직접 실행한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain

namespace { // 원문 Wind의 판본별 절대 주소만 입력으로 연결한다.
// 실제 부모/세 float/공유 게임 난수의 주소다. 관측기가 결과를 계산하지 않는다.
struct WindLocations {
    std::uint32_t entry,x,y,angle,random; // 각 주소는 복사한 보호 PE 내부이며 host float 객체가 아니다.
};
constexpr WindLocations kPatchWind{0x4dd2b0,0x5e18a8,0x5435b0,0x5e18b0,0x532710}; //10.78의 실제 현재 전역이다.
constexpr WindLocations kCdWind{0x4503f0,0x51fbf8,0x51fbfc,0x51fc00,0x5308ec}; //10.72의 같은 원문 부모/전역이다.
// 시퀀스 시작만 x87을53비트로 초기화한다. 이후 각 프레임은 원문 상태를 이어 읽는다.
void BeginWindSequence() { const std::uint16_t control=0x027f;__asm { fninit } __asm { fldcw control } }
// 원문 void 부모의 레지스터/FPU 연산을 그대로 실행하며 OS/게임 초기화는 실행하지 않는다.
void InvokeWind(std::uint32_t address) { __asm { mov eax,address } __asm { call eax } }
}
// stdin은 명시 시작 상태/반복 수만 제공하며 결과는 실제CPU 메모리에서 읽는다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
        if (count!=3) throw std::runtime_error("판본과 보호 PE 경로가 필요합니다");const bool patch=std::wstring(arguments[1])==L"originals";const auto locations=patch ? kPatchWind : kCdWind;
        MappedPe image(arguments[2]);checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;BindAssertion(patch ? 0x4e0620 : 0x490040);
        std::uint32_t index{},seed{},steps{},x{},y{},angle{},gate{};
        // 모든 행을 독립 입력으로 준비하고 행 안에서는 같은 난수/바람을 계속 전진시킨다.
        while (std::cin>>index>>seed>>steps>>x>>y>>angle>>gate) {
            if (steps>4096) throw std::runtime_error("CPU 관측 반복 범위");events.clear();action=0;*checking=gate;
            *reinterpret_cast<std::uint32_t*>(locations.random)=seed;*reinterpret_cast<std::uint32_t*>(locations.x)=x;*reinterpret_cast<std::uint32_t*>(locations.y)=y;*reinterpret_cast<std::uint32_t*>(locations.angle)=angle;
            BeginWindSequence();
            // 실제 부모를 매 프레임 호출하며 중간 결과를 정규화하거나 재시드하지 않는다.
            for (std::uint32_t step=0;step<steps;++step) InvokeWind(locations.entry);
            std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<index<<'\t'<<seed<<'\t'<<steps<<'\t'<<x<<'\t'<<y<<'\t'<<angle<<'\t'<<gate<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.x)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.y)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.angle)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(locations.random)<<'\t'<<*checking<<'\t'<<(events.empty() ? "-" : events)<<'\n';
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
