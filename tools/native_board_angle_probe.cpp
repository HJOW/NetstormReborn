// 실제32비트 CPU에서 보호 PE의 순수 각도/CRT만 격리 호출한다. 게임 초기화/GUI/IAT 서비스는 호출하지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#pragma section(".payload",read,execute)
extern "C" { __declspec(allocate(".payload")) unsigned char peScratch[0x300000]={}; } // 원본 순수 코드 주소를 OS 다른 맵과 충돌 없이 같은 EXE의 빈 공간에 확보한다.

namespace { // 실제 PE 주소는 이 독립32비트 콘솔의 전용 메모리에만 설치한다.
std::uint32_t* checking{}; // 패치의 현재 DWORD 또는 CD 독립 입력이다.
std::uint32_t cdChecking{}; // CD 몸체는 이 값을 읽지 않는다.
std::uint32_t action{}; // 진단 중 checking0 변경을 선택한다.
std::string events; // 원문 진단의 순서/식/파일/행을 보관한다.
// 관측기 내부 범위 오류는 GUI 없이 EIP/주소를 기록하고 OS의 정상 실패 처리로 넘긴다.
LONG CALLBACK ObserveFault(EXCEPTION_POINTERS* error) {
    std::fprintf(stderr,"probe fault code=%08lx EIP=%08lx address=%08lx\\n",error->ExceptionRecord->ExceptionCode,error->ContextRecord->Eip,error->ExceptionRecord->NumberParameters>1 ? static_cast<unsigned long>(error->ExceptionRecord->ExceptionInformation[1]) : 0UL);
    const auto* stack=reinterpret_cast<const std::uint32_t*>(error->ContextRecord->Esp);std::fprintf(stderr,"ESP=%08lx stack=%08x,%08x,%08x,%08x\\n",error->ContextRecord->Esp,stack[0],stack[1],stack[2],stack[3]);
    return EXCEPTION_CONTINUE_SEARCH;
}
// 실제 assertion UI만 대체하며 부모의 FPU 반환값은 계산하거나 바꾸지 않는다.
void __cdecl ObserveAssertion(const char* expression,const char* file,std::uint32_t line) {
    if (!events.empty()) events+='|';events+=std::to_string(line)+':'+expression+':'+file;if (action) *checking=0;
}
// 원본 파일을 읽기 전용으로 복사하며 로더/게임 시작 함수를 실행하지 않는다.
struct MappedPe {
    std::array<void*,33> blocks{}; // 관측기의 전용 빈 코드 공간 안에서만 원문64KB 블록을 빌린다.
    std::array<DWORD,33> protections{}; // 각 빌린 블록의 원래 메모리 보호다.
    explicit MappedPe(const std::filesystem::path& path) {
        const auto firstBlank=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(peScratch));
        // 다른 모듈이나 OS 맵을 반납/변경하지 않고 이 관측기 자신의 검증한 빈 공간만 사용한다.
        for (std::size_t index=0;index<blocks.size();++index) {
            const auto address=0x410000U+static_cast<std::uint32_t>(index)*0x10000U;
            if (address<firstBlank || static_cast<std::uint64_t>(address)+0x10000>firstBlank+sizeof(peScratch)) throw std::runtime_error("관측기 빈 코드 공간 범위 오류 "+std::to_string(firstBlank));
            blocks[index]=reinterpret_cast<void*>(address);
            if (!VirtualProtect(blocks[index],0x10000,PAGE_EXECUTE_READWRITE,&protections[index])) throw std::runtime_error("관측기 빈 코드 공간 보호 변경 실패");
        }
        std::ifstream input(path,std::ios::binary);if (!input) throw std::runtime_error("PE 읽기 실패");
        std::vector<char> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        if (bytes.size()<sizeof(IMAGE_DOS_HEADER)) throw std::runtime_error("PE DOS 범위");
        IMAGE_DOS_HEADER dos{};std::memcpy(&dos,bytes.data(),sizeof(dos));
        if (dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || static_cast<std::size_t>(dos.e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size()) throw std::runtime_error("PE NT 범위");
        IMAGE_NT_HEADERS32 header{};std::memcpy(&header,bytes.data()+dos.e_lfanew,sizeof(header));
        if (header.Signature!=IMAGE_NT_SIGNATURE || header.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC || header.OptionalHeader.ImageBase!=0x400000 || header.OptionalHeader.SizeOfHeaders>bytes.size()) throw std::runtime_error("PE32 헤더 계약");
        const auto first=static_cast<std::size_t>(dos.e_lfanew)+offsetof(IMAGE_NT_HEADERS32,OptionalHeader)+header.FileHeader.SizeOfOptionalHeader;
        // 각 실제 섹션의 파일/가상 범위를 확인하고 메모리에만 복사한다. 파일을 쓰거나 수정하지 않는다.
        for (std::uint32_t index=0;index<header.FileHeader.NumberOfSections;++index) {
            const auto at=first+index*sizeof(IMAGE_SECTION_HEADER);if (at+sizeof(IMAGE_SECTION_HEADER)>bytes.size()) throw std::runtime_error("PE 섹션 헤더 범위");
            IMAGE_SECTION_HEADER section{};std::memcpy(&section,bytes.data()+at,sizeof(section));
            if (static_cast<std::uint64_t>(section.PointerToRawData)+section.SizeOfRawData>bytes.size() || static_cast<std::uint64_t>(section.VirtualAddress)+section.SizeOfRawData>header.OptionalHeader.SizeOfImage) throw std::runtime_error("PE 섹션 물리 범위");
            // 각 섹션과 독립 블록의 교차 구간만 그대로 복사한다. 원문의 바이트/절대주소/상수는 바꾸지 않는다.
            for (auto* block:blocks) {
                if (!block) continue;const auto address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(block));
                const std::uint32_t sourceAddress=0x400000U+section.VirtualAddress,low=std::max<std::uint32_t>(address,sourceAddress),high=std::min<std::uint32_t>(address+0x10000U,sourceAddress+section.SizeOfRawData);
                if (low<high) std::memcpy(reinterpret_cast<void*>(low),bytes.data()+section.PointerToRawData+low-sourceAddress,high-low);
            }
        }
    }
    // 원본 파일에는 쓰지 않고 이 프로세스의 전용 주소 공간만 반납한다.
    ~MappedPe() {
        // 관측기 자신의 빈 공간을 다시0으로 지우고 보호만 되돌린다. 이미지/OS/원본 파일을 반납하지 않는다.
        for (std::size_t index=0;index<blocks.size();++index) if (blocks[index]) { std::memset(blocks[index],0,0x10000);DWORD ignored{};VirtualProtect(blocks[index],0x10000,protections[index],&ignored); }
    }
};
// 명시 진단 진입점의 프로세스 복사본에만 상대 JMP를 연결한다.
void BindAssertion(std::uint32_t address) {
    auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;
    const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveAssertion)-address-5);std::memcpy(target+1,&delta,4);
    if (!FlushInstructionCache(GetCurrentProcess(),target,5)) throw std::runtime_error("진단 코드 캐시 갱신 실패");
}
// 결과 모델 없이 실제 x87 53비트 호출과 fstp qword를 수행한다.
double Invoke(std::uint32_t address,std::uint32_t inputBits) {
    const std::uint16_t control=0x027f;double result{};
    __asm { fninit }
    __asm { fldcw control }
    __asm { push inputBits }
    __asm { mov eax,address }
    __asm { call eax }
    __asm { add esp,4 }
    __asm { fstp qword ptr result }
    return result;
}
// 실제 fstp qword의 여덟 바이트를 고정 소문자 HEX로 출력한다.
std::string DoubleHex(double value) {
    constexpr char digits[]="0123456789abcdef"; // 기대값 계산 없는 바이트 출력 표다.
    const auto bits=std::bit_cast<std::uint64_t>(value);std::string text;
    // 원문 little endian 저장 순서를 유지한다.
    for (std::uint32_t index=0;index<8;++index) { const auto byte=static_cast<std::uint8_t>(bits>>(index*8));text+=digits[byte>>4];text+=digits[byte&15]; }
    return text;
}
}
// stdin 입력 격자를 실제 PE 부모에 전달하고 전체 반환/진단을 stdout으로 기록한다.
int wmain(int count,wchar_t** arguments) {
    try {
        AddVectoredExceptionHandler(1,ObserveFault);
        SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX); // 원본 GUI를 만들지 않는 콘솔 관측에서 OS 오류 상자도 표시하지 않는다.
        if (count!=3) throw std::runtime_error("판본과 PE 경로가 필요합니다");const bool patch=std::wstring(arguments[1])==L"originals";
        MappedPe image(arguments[2]);checking=patch ? reinterpret_cast<std::uint32_t*>(0x5e4794) : &cdChecking;BindAssertion(patch ? 0x4e0620 : 0x490040);
        std::fprintf(stderr,"entry=%08x CRT=%08x flag=%08x\\n",*reinterpret_cast<const std::uint32_t*>(patch ? 0x41cdb0 : 0x43f4e0),*reinterpret_cast<const std::uint32_t*>(patch ? 0x4e5e8a : 0x4f28fa),*reinterpret_cast<const std::uint32_t*>(patch ? 0x5e4acc : 0x546778));
        std::uint32_t bits{},initial{};double modulus{};
        // 모든 결과는 같은 입력을 받은 실제 함수/CRT에서 얻으며 host fmod를 사용하지 않는다.
        while (std::cin>>bits>>modulus>>initial>>action) {
            *checking=initial;events.clear();*reinterpret_cast<double*>(patch ? 0x5022d8 : 0x501730)=modulus;
            const auto value=Invoke(patch ? 0x41cdb0 : 0x43f4e0,bits);
            std::cout<<(patch ? "originals" : "originalCD")<<'\t'<<bits<<'\t'<<static_cast<std::uint32_t>(modulus)<<'\t'<<initial<<'\t'<<action<<'\t'<<DoubleHex(value)<<'\t'<<*checking<<'\t'<<(events.empty() ? "-" : events)<<'\n';
        }
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
