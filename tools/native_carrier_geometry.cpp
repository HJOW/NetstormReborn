// 보호 PE의 세 기하 함수만 별도 Win32 프로세스에서 실제 x87로 관측한다.
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
static_assert(sizeof(void*)==4,"기하 원문 관측기는 Win32로 빌드해야 합니다"); // 원본 thiscall/주소 폭을 강제한다.
namespace {
constexpr std::uintptr_t kImageBase=0x400000,kSide=0x41d400,kNearest=0x41d530,kDiagnostic=0x4e0620,kInvalid=0x545370; // 원본 절대 주소이며 새 이미지 복사에만 사용한다.
#pragma section(".nsimage",read)
__declspec(allocate(".nsimage")) std::array<std::uint8_t,0x800000> scratch{}; // 낮은 helper 이미지 안의 사용하지 않는 원본 코드 전용 공간이다.
std::uint8_t* image{}; // 격리 복사본의 주소다. 원본 파일을 쓰기 모드로 열지 않는다.
std::uint32_t mutation{},diagnosticCount{}; // 현재 명시 진단 입력과 실제 발생 수다.
std::array<std::uint32_t,8> diagnostics{}; // 한 기하 호출에서 발생한 진단 행 번호만 저장한다.
bool diagnosticFault{}; // 원문 진단 인자 또는 고정 배열 범위가 다르면 관측을 거부한다.
// 원본 cdecl 진단의 세 인자만 검사하고 현재 실패점 변이만 공급한다.
int __cdecl Diagnostic(const char* condition,const char* file,std::uint32_t line) {
    if (std::strcmp(condition,"false") || std::strcmp(file,"\\Ns\\O\\Boardcoord.cpp") || (line!=0x1f9 && line!=0x262) || diagnosticCount>=diagnostics.size()) diagnosticFault=true;
    else diagnostics[diagnosticCount++]=line;
    if (mutation) { const std::array<std::uint32_t,2> values{0x42288000,0x7f800001};std::memcpy(image+kInvalid-kImageBase,values.data(),8); }
    return 0;
}
// 원문의22DWORD 입력을 읽으며 기하 결과는 계산하지 않는다.
std::vector<std::uint32_t> Numbers(const std::string& value) { std::vector<std::uint32_t> result;std::stringstream stream(value);std::string word;
    // 끝까지 원래 입력 순서로 읽는다.
    while (std::getline(stream,word,',')) result.push_back(static_cast<std::uint32_t>(std::stoul(word)));return result;
}
// output 양쪽 canary는 원본 쓰기가 정확한 두 DWORD인지 확인한다.
struct GuardedPoint {
    std::uint32_t before{0xc0dec0de}; // 원본 출력 앞의 보존 canary다.
    std::array<std::uint32_t,2> point{0xa5a5a5a5,0xa5a5a5a5}; // 원본이 직접 쓴 두 float의 원문 비트다.
    std::uint32_t after{0xdec0dec0}; // 원본 출력 뒤의 보존 canary다.
};
// 원본 thiscall과 같은 인자 폭을 사용하며 입력 float는 DWORD 그대로 전달한다.
using SideCall=void* (__thiscall*)(void*,void*,std::uint32_t,std::uint32_t);
using NearestCall=void* (__thiscall*)(void*,void*,std::uint32_t,std::uint32_t,std::uint32_t);
}
// GUI/원본 초기화/원본 엔트리는 실행하지 않고 기하 입력 파일만 관측한다.
int wmain(int argc,wchar_t** argv) {
    if (argc!=3) return 2;
    constexpr std::size_t kReservedBytes=0x400000; // helper의 전용 섹션에 들어가는 원본 절대 주소 범위다.
    image=reinterpret_cast<std::uint8_t*>(kImageBase);if (kImageBase<reinterpret_cast<std::uintptr_t>(scratch.data()) || kImageBase+kReservedBytes>reinterpret_cast<std::uintptr_t>(scratch.data())+scratch.size()) return 6;DWORD previousProtection{};if (!VirtualProtect(image,kReservedBytes,PAGE_EXECUTE_READWRITE,&previousProtection)) return 6;
    std::ifstream source(std::filesystem::path(argv[1]),std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(source)),{});
    if (bytes.size()<sizeof(IMAGE_DOS_HEADER)) return 3;const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || static_cast<std::size_t>(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size()) return 4;
    const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);if (pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || pe->OptionalHeader.ImageBase!=kImageBase || pe->OptionalHeader.SizeOfImage>kReservedBytes) return 5;
    std::memcpy(image,bytes.data(),pe->OptionalHeader.SizeOfHeaders);const auto* section=IMAGE_FIRST_SECTION(pe);
    // 원본 섹션은 격리 실행 페이지에만 복사한다. import/엔트리 초기화를 호출하지 않는다.
    for (std::size_t i=0;i<pe->FileHeader.NumberOfSections;++i) { if (section[i].PointerToRawData+section[i].SizeOfRawData>bytes.size() || section[i].VirtualAddress+section[i].SizeOfRawData>pe->OptionalHeader.SizeOfImage) return 7;std::memcpy(image+section[i].VirtualAddress,bytes.data()+section[i].PointerToRawData,section[i].SizeOfRawData); }
    std::array<std::uint8_t,7> hook{0xb8,0,0,0,0,0xff,0xe0};const auto callback=reinterpret_cast<std::uint32_t>(&Diagnostic);std::memcpy(hook.data()+1,&callback,4);std::memcpy(image+kDiagnostic-kImageBase,hook.data(),hook.size());FlushInstructionCache(GetCurrentProcess(),image,pe->OptionalHeader.SizeOfImage);
    const std::vector<std::uint8_t> frozen(image,image+pe->OptionalHeader.SizeOfImage);std::ifstream input{std::filesystem::path(argv[2])};std::string line;std::size_t ordinal{},returns{};
    // 입력마다 두 x87 정밀도를 실제 CPU에서 독립 관측한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;const auto end=line.find('\t');const auto original=line.substr(0,end);const auto values=Numbers(original);if (values.size()!=22) return 8;const auto current=ordinal++;if (values[0]>1) continue;
        // 각 제어어의 관측은 원래 메모리 입력에서 다시 시작한다.
        for (std::uint16_t control:{std::uint16_t{0x27f},std::uint16_t{0x37f}}) {
            std::array<std::uint32_t,4> rectangle{values[1],values[2],values[3],values[4]};const auto before=rectangle;GuardedPoint output;std::array<std::uint32_t,2> invalid{values[19],values[20]};std::memcpy(image+kInvalid-kImageBase,invalid.data(),8);mutation=values[21];diagnosticCount=0;diagnosticFault=false;std::uint16_t saved{},status{},actualControl{};void* result{};
            __asm {
                fnstcw saved
                fnclex
                fldcw control
            }
            if (values[0]==0) result=reinterpret_cast<SideCall>(kSide)(rectangle.data(),output.point.data(),values[8],values[7]);else result=reinterpret_cast<NearestCall>(kNearest)(rectangle.data(),output.point.data(),values[5],values[6],values[7]);
            __asm {
                fnstsw status
                fnstcw actualControl
                fnclex
                fldcw saved
            }
            if (result!=output.point.data() || output.before!=0xc0dec0de || output.after!=0xdec0dec0 || rectangle!=before || actualControl!=control || (status&0x3800) || diagnosticFault) return 9;
            const auto split=static_cast<std::size_t>(kInvalid-kImageBase);if (std::memcmp(image,frozen.data(),split) || std::memcmp(image+split+8,frozen.data()+split+8,frozen.size()-split-8)) return 10;
            std::memcpy(invalid.data(),image+kInvalid-kImageBase,8);std::printf("%zu\t%u\t",current,control);
            // 원본 진단을 실제 발생 순서로 기록한다.
            for (std::size_t i=0;i<diagnosticCount;++i) std::printf("%s!:%u",i ? ";" : "",diagnostics[i]);if (!diagnosticCount) std::printf("-");
            std::printf("\t%u,%u,%u,%u\t%u\n",output.point[0],output.point[1],invalid[0],invalid[1],status);++returns;
        }
    }
    DWORD restoredProtection{};VirtualProtect(image,kReservedBytes,previousProtection,&restoredProtection);return returns ? 0 : 11;
}
