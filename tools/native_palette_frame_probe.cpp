// 실제10.78 팔레트 Frame/도달 불가 진단을 CPU로 실행하고 화면·프로세스 종료 요청만 관측한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 보호 파일을 바꾸지 않고 관측기의 전용 payload만 사용한다.
constexpr std::uint32_t kFrameObject=0x601000; // 다른 PE 전역/관측기 코드와 겹치지 않는 실제 객체 입력 공간이다.
std::string paletteEvents; // 실제 apply/restore/kill 순서이며 assertion 문자열과 구별한다.
bool frameKilled{}; // 외부 Kill 경계에서만 설정하며 뒤 원문 호출을 중단한다.
// 화면 팔레트의 원시 바이트를 고정 HEX로 기록한다.
std::string PaletteBytes(const std::uint8_t* bytes,std::size_t size) {
    constexpr char digits[]="0123456789abcdef";std::string output; // 출력만 수행하는16진수 표다.
    for (std::size_t i=0;i<size;++i) { output+=digits[bytes[i]>>4];output+=digits[bytes[i]&15]; } // 예약 바이트도 그대로 보존한다.
    return output.empty() ? "-" : output;
}
// 실제 색/범위/표시 요청 인자를 기록하며 RGB 계산을 대신하지 않는다.
void __cdecl ObservePalette(std::uint32_t start,std::uint32_t count,const std::uint8_t* colors,std::uint32_t apply) {
    if (apply!=1) throw std::runtime_error("팔레트 Frame apply 인자");
    if (!colors) { if (start!=0 || count!=256) throw std::runtime_error("팔레트 복구 범위");paletteEvents+="R;";return; }
    if (count>256 || colors!=reinterpret_cast<const std::uint8_t*>(kFrameObject+0x40c)) throw std::runtime_error("팔레트 Frame 현재 색 주소");paletteEvents+="A,"+std::to_string(start)+','+std::to_string(count)+','+PaletteBytes(colors,count*4U)+";";
}
// 원문 thiscall Kill의 현재 객체와 flags0을 기록한다. 실제 게임 프로세스는 생성하지 않는다.
void __fastcall ObservePaletteKill(std::uint32_t object,void*,std::uint32_t flags) { if (object!=kFrameObject || flags!=0) throw std::runtime_error("팔레트 Frame Kill 인자");frameKilled=true;paletteEvents+="K,0;"; }
// 실제 두 외부 경계의 관측기 복사본에만 JMP를 설치한다.
void BindPalette(std::uint32_t address,const void* observer) {
    if (address<0x410000 || address+5>0x620000) throw std::runtime_error("팔레트 전용 복사 범위");auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(target+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("팔레트 코드 캐시 실패");
}
__declspec(noinline) void InvokePaletteFrame() { reinterpret_cast<void (__thiscall*)(void*)>(0x471650)(reinterpret_cast<void*>(kFrameObject)); } // 실제 ABI 호출로 외부 장치가 바꾸는 volatile XMM과 시각 변수를 분리한다.
}
// 입력은 시작 전역/객체/시각만 제공하며 결과는 실제 원문 메모리에서 읽는다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호10.78 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);action=0;BindAssertion(0x4e0620);BindPalette(0x4a2640,reinterpret_cast<const void*>(&ObservePalette));BindPalette(0x41bf10,reinterpret_cast<const void*>(&ObservePaletteKill));
        std::uint32_t index{},gate{},mode{},start{},colors{},pattern{},steps{};std::int32_t remaining{};std::uint64_t intervalBits{},lastBits{},nowBits{},incrementBits{};
        // 행은 독립 상태이며 행 안의 last/step/색은 재초기화하지 않는다.
        while (std::cin>>index>>intervalBits>>lastBits>>nowBits>>incrementBits>>remaining>>gate>>mode>>start>>colors>>pattern>>steps) {
            if (colors>256 || start>256-colors || steps>32 || (remaining>65 && std::bit_cast<double>(intervalBits)<=0)) throw std::runtime_error("팔레트 관측 입력 범위");std::memset(reinterpret_cast<void*>(kFrameObject),0,0x840);events.clear();paletteEvents.clear();frameKilled=false;*checking=gate;*reinterpret_cast<std::uint32_t*>(0x5c78d4)=mode;
            // 원래/현재 RGBQUAD는 서로 다른 명시 입력이며 결과를 미리 계산하지 않는다.
            for (std::uint32_t byte=0;byte<1024;++byte) { *reinterpret_cast<std::uint8_t*>(kFrameObject+0xc+byte)=static_cast<std::uint8_t>(byte*37U+11U+pattern);*reinterpret_cast<std::uint8_t*>(kFrameObject+0x40c+byte)=static_cast<std::uint8_t>(byte*13U+143U+pattern*17U); }
            *reinterpret_cast<std::uint64_t*>(kFrameObject+0x810)=intervalBits;*reinterpret_cast<std::uint64_t*>(kFrameObject+0x818)=lastBits;*reinterpret_cast<std::uint32_t*>(kFrameObject+0x820)=start;*reinterpret_cast<std::uint32_t*>(kFrameObject+0x824)=colors;*reinterpret_cast<std::int32_t*>(kFrameObject+0x838)=remaining;*reinterpret_cast<std::uint32_t*>(kFrameObject+0x83c)=1;
            double now=std::bit_cast<double>(nowBits);const double increment=std::bit_cast<double>(incrementBits);const std::uint16_t control=0x027f;__asm { fninit } __asm { fldcw control }std::uint32_t calls{};
            // Kill 요청이 반환해도 뒤 프레임을 실행하지 않아 실제 수명을 보존한다.
            for (std::uint32_t step=0;step<steps && !frameKilled;++step) { *reinterpret_cast<double*>(0x55b4d8)=now;InvokePaletteFrame();++calls;now+=increment; }
            std::cout<<index<<'\t'<<intervalBits<<'\t'<<lastBits<<'\t'<<nowBits<<'\t'<<incrementBits<<'\t'<<remaining<<'\t'<<gate<<'\t'<<mode<<'\t'<<start<<'\t'<<colors<<'\t'<<pattern<<'\t'<<steps<<'\t'<<*reinterpret_cast<const std::uint64_t*>(kFrameObject+0x810)<<'\t'<<*reinterpret_cast<const std::uint64_t*>(kFrameObject+0x818)<<'\t'<<*reinterpret_cast<const std::int32_t*>(kFrameObject+0x838)<<'\t'<<*reinterpret_cast<const std::uint32_t*>(kFrameObject+0x83c)<<'\t'<<PaletteBytes(reinterpret_cast<const std::uint8_t*>(kFrameObject+0xc),colors*4U)<<'\t'<<PaletteBytes(reinterpret_cast<const std::uint8_t*>(kFrameObject+0x40c),colors*4U)<<'\t'<<calls<<'\t'<<*checking<<'\t'<<(events.empty() ? "-" : events)<<'\t'<<(paletteEvents.empty() ? "-" : paletteEvents)<<'\n';
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
