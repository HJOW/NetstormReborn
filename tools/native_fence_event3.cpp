// 10.78의 선택된 Regular/펜스 몸체만 별도32비트 검사 프로세스에서 실제 CPU로 실행한다.
// 원본 파일은 읽기만 하며 private PE 이미지의 명시 외부 경계만 검사 함수로 연결한다.
#define NOMINMAX // Windows min/max 매크로가 표준 헤더를 바꾸지 않게 한다.
#include <windows.h>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// asm 관찰기는 원래 CALL/FST를 그대로 수행하며 레지스터/플래그와 x87 환경을 보존한다.
std::uint32_t gContinue=0x496d84; // 원래 FCOMP 명령의 재개 주소다.
std::uint32_t gReturnBits{},gReturnCalls{}; // 실제 FST32가 저장한 값과 선택된 호출 수다.
std::array<std::uint8_t,28> gReturnEnvironment{}; // FST 직전 x87 환경이며 FNSTENV/FLDENV 쌍으로 보존한다.

// 원문의 간접 CALL3BYTE와 FST4BYTE를 같은 CPU에서 실행하고 저장값만 기록한다.
__declspec(naked) void ObserveReturn() {
    __asm {
        call dword ptr [edx+5ch]
        fnstenv gReturnEnvironment
        fldenv gReturnEnvironment
        fst dword ptr [esp+8]
        push eax
        mov eax,dword ptr [esp+12]
        mov gReturnBits,eax
        pushfd
        inc gReturnCalls
        popfd
        pop eax
        jmp dword ptr [gContinue]
    }
}

// 원본의 물리 데이터와 명시 경계를 소유하는 검사 전용 저장소다.
namespace {
constexpr std::uintptr_t kBase=0x400000,kMappedBase=0x30000000,kFrame=0x496cb0; // 원본 논리주소/검사 전용 재배치 주소/실제 Regular 진입점이다.
// Python이 주소 참조만 옮긴 private 이미지의 실제 주소를 반환한다.
void* Mapped(std::uintptr_t original) { return reinterpret_cast<void*>(kMappedBase+original-kBase); }
std::array<std::uint8_t,55*50> pool; // 부모50/이웃51~54의 실제50BYTE 기록이다.
std::array<std::uint8_t,40> current,newRegular; // 현재 Regular와 할당 성공 시 원본 생성자가 쓰는40BYTE다.
std::array<std::uint8_t,188*500> types; // 실제 타입 표이며 선택된136의 genus 입력을 제공한다.
std::array<std::uint32_t,12> input; // 기존 펜스 도구와 같은 독립 입력12DWORD다.
std::string trace; // 원문이 실제 선택한 명시 하위 호출 순서다.
std::uint32_t markers{},reserves{},connectionCalls{}; // 하위 효과가 적용되는 현재 호출 누계다.
bool created{},killed{}; // 실제 생성자 진입과 현재 Regular 삭제 요청의 관측이다.

// 검증 실패는 private 원본 SEH를 통한 예외 전파 없이 해당 도우미만 종료한다.
[[noreturn]] void Fail(const char* message) { std::cerr<<message<<'\n';ExitProcess(2); }
// 비정렬 원문 필드를 같은 폭으로 읽고 쓴다.
template<class T> T Read(const void* source,std::size_t offset=0) { T value;std::memcpy(&value,static_cast<const std::uint8_t*>(source)+offset,sizeof value);return value; }
template<class T> void Put(void* target,std::size_t offset,T value) { std::memcpy(static_cast<std::uint8_t*>(target)+offset,&value,sizeof value); }
// 검사 프로세스의 실제32비트 포인터를 원본 DWORD에 담는다.
std::uint32_t Pointer(const void* value) { return reinterpret_cast<std::uint32_t>(value); }
// 원문 선택 순서를 기대값 계산 없이 문자열로 보존한다.
void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
// 실제 지정 SID의 raw 주소이며 검사 입력 범위 밖을 허용하지 않는다.
std::uint8_t* Slot(std::uint32_t sid) { if (sid>=55) Fail("검사 SID 범위 오류");return pool.data()+sid*50; }

// 실제 네 방향 검색의 외부 결과와 검색 효과 뒤 현재 owner 입력이다.
std::uint32_t __fastcall Neighbor(void* self,void*,std::uint32_t direction) {
    if (self!=Slot(50) || direction>6 || direction%2) Fail("이웃 경계 인수 오류");Event("N:"+std::to_string(direction));
    if (input[9]==2 && !direction) Slot(50)[34]=6;
    return input[7]&(1U<<(direction/2)) ? 51+direction/2 : 0;
}
// 첫 표식 삭제 효과에서만 같은 좌표 입력을 바꾼다.
void __fastcall Markers(void* self,void*,std::uint32_t other) {
    if (self!=Slot(50)) Fail("표식 경계 부모 오류");Event("M:"+std::to_string(other));
    if (++markers==1 && input[9]==3) { Put(Slot(51),18,std::bit_cast<std::uint32_t>(125.0F));Put(Slot(50),14,std::bit_cast<std::uint32_t>(120.0F)); }
}
// 원문 mode0의 네 float 비트와 뒤 다섯0을 그대로 관측한다.
void __cdecl Line(std::uint32_t x,std::uint32_t y,std::uint32_t otherX,std::uint32_t otherY,std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d,std::uint32_t e) {
    if (a||b||c||d||e) Fail("선 경계 mode0 인수 오류");Event("L:"+std::to_string(x)+','+std::to_string(y)+','+std::to_string(otherX)+','+std::to_string(otherY));
}
// 각 new40의 명시 성공 여부만 공급하며 실제496e80은 우회하지 않는다.
void* __cdecl Allocate(std::uint32_t size) {
    if (size!=40) Fail("할당 경계 크기 오류");Event("R");const auto success=(input[8]>>reserves)&1U;
    if (++reserves==1 && input[9]==4) { Put(Slot(50),46,77U);Slot(50)[34]=7; }
    return success ? newRegular.data() : nullptr;
}
// 공통 부착만 경계로 두고 원본 Regular 생성자의 필드/시각/x87 명령은 모두 실행한다.
void* __fastcall Attach(void* self,void*,std::uint32_t type,std::uint32_t parent,std::uint32_t parentForm,std::uint32_t flags) {
    if (self!=newRegular.data() || type!=46 || parent!=50 || parentForm || flags!=0x50) Fail("공통 부착 경계 인수 오류");
    Event("A:2:50:"+std::to_string(0x3dcccccdU));created=true;Put(self,8,parent);return self;
}
// 보호bit20의 원문 Damageable 위임은 전달 payload를 반환하는 기존 명시 경계다.
float __fastcall Damageable(void* self,void*,std::uint32_t event,std::uint32_t count,std::uint32_t payload) {
    if (self!=Slot(50)) Fail("Damageable 부모 오류");Event("D:"+std::to_string(event)+','+std::to_string(count)+','+std::to_string(payload));
    if (input[9]==8) Put(Slot(50),12,std::uint16_t{0x8888});return std::bit_cast<float>(payload);
}
// 실제 호출자가 반환0을 소비할 때의 삭제 요청만 기록한다.
void __fastcall Kill(void* self,void*,std::uint32_t flags) { if (self!=current.data() || flags) Fail("삭제 경계 인수 오류");killed=true; }
// 사건3이 선택하지 않아야 하는 하위에 닿으면 관측을 실패시킨다.
void __cdecl Unexpected() { ++connectionCalls;Fail("사건3 미선택 하위 호출 오류"); }

// private PE의 명시 외부 진입점만 검사 콜백으로 연결한다.
template<class F> void Jump(std::uintptr_t address,F function,std::size_t width=5) {
    address=kMappedBase+address-kBase;auto* target=reinterpret_cast<std::uint8_t*>(address);target[0]=0xe9;Put(target,1,Pointer(reinterpret_cast<const void*>(function))-static_cast<std::uint32_t>(address)-5U);
    for (std::size_t i=5;i<width;++i) target[i]=0x90; // 원래 CALL/FST7BYTE 중 남은 바이트만 채운다.
}
// 보호 EXE는 읽기만 하며 현재 검사 프로세스의 별도 메모리에 검사 전용 주소에 배치한다.
void Image(const char* path) {
    // 큰 원천 버퍼가 검사 전용 고정주소를 먼저 차지하지 않도록 작은 헤더로 필요한 영역부터 확보한다.
    std::ifstream file(path,std::ios::binary);IMAGE_DOS_HEADER firstDos{};IMAGE_NT_HEADERS32 firstNt{};
    file.read(reinterpret_cast<char*>(&firstDos),sizeof firstDos);if (!file || firstDos.e_magic!=IMAGE_DOS_SIGNATURE || firstDos.e_lfanew<0) Fail("원본 DOS 헤더 읽기 실패");
    file.seekg(firstDos.e_lfanew);file.read(reinterpret_cast<char*>(&firstNt),sizeof firstNt);if (!file || firstNt.Signature!=IMAGE_NT_SIGNATURE || firstNt.OptionalHeader.ImageBase!=kBase) Fail("원본 NT 헤더 읽기 실패");
    auto* image=static_cast<std::uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(kMappedBase),firstNt.OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if (image!=reinterpret_cast<void*>(kMappedBase)) { MEMORY_BASIC_INFORMATION memory{};VirtualQuery(reinterpret_cast<void*>(kMappedBase),&memory,sizeof memory);char module[MAX_PATH]{};GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase),module,MAX_PATH);std::cerr<<"고정주소 오류 "<<GetLastError()<<' '<<memory.AllocationBase<<' '<<memory.RegionSize<<' '<<GetModuleHandleW(nullptr)<<' '<<module<<'\n';Fail("private 원본 이미지 할당 실패"); }
    file.seekg(0);std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
    if (bytes.size()<sizeof(IMAGE_DOS_HEADER)) Fail("원본 PE 읽기 실패");const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || static_cast<std::size_t>(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size()) Fail("DOS 헤더 오류");
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);
    if (nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC || nt->OptionalHeader.ImageBase!=kBase) Fail("원본 PE32 주소 오류");
    if (nt->OptionalHeader.SizeOfHeaders>bytes.size() || nt->OptionalHeader.SizeOfImage!=firstNt.OptionalHeader.SizeOfImage) Fail("private 원본 이미지 크기 오류");
    std::memcpy(image,bytes.data(),nt->OptionalHeader.SizeOfHeaders);const auto* section=IMAGE_FIRST_SECTION(nt);
    for (unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) { // 실제 section 원천 바이트만 복사하며 BSS는 VirtualAlloc의 초기값을 사용한다.
        if (static_cast<std::size_t>(section[i].PointerToRawData)+section[i].SizeOfRawData>bytes.size() || static_cast<std::size_t>(section[i].VirtualAddress)+section[i].SizeOfRawData>nt->OptionalHeader.SizeOfImage) Fail("원본 section 범위 오류");
        std::memcpy(image+section[i].VirtualAddress,bytes.data()+section[i].PointerToRawData,section[i].SizeOfRawData);
    }
    Jump(0x456220,Neighbor);Jump(0x4566d0,Markers);Jump(0x455b20,Line);Jump(0x4e4391,Allocate);Jump(0x41bd20,Attach);Jump(0x44b900,Damageable);Jump(0x41bf10,Kill);
    Jump(0x4573d0,Unexpected);Jump(0x4578f0,Unexpected);Jump(0x457460,Unexpected);Jump(0x4acee0,Unexpected);Jump(0x4e0620,Unexpected);
    const std::array<std::uint8_t,7> call{0xff,0x52,0x5c,0xd9,0x54,0x24,0x08};if (std::memcmp(Mapped(0x496d7d),call.data(),call.size())) Fail("원래 CALL/FST 바이트 오류");
    gContinue=Pointer(Mapped(0x496d84));Jump(0x496d7d,ObserveReturn,7);if (!FlushInstructionCache(GetCurrentProcess(),image,nt->OptionalHeader.SizeOfImage)) Fail("명령 캐시 갱신 실패");
}
// 실제 관측 바이트를16진 문자열로 보존한다.
std::string Hex(const std::uint8_t* bytes,std::size_t count) {
    constexpr char digits[]="0123456789abcdef"; // 원문 바이트의 순서만 표기하는 문자 표다.
    std::string result;result.reserve(count*2);
    for (std::size_t i=0;i<count;++i) { result+=digits[bytes[i]>>4];result+=digits[bytes[i]&15]; } // 두 문자씩 원래 주소 순서로 기록한다.
    return result;
}
// 입력행마다 실제 원문250BYTE/현재 Regular/타입/현재시각을 준비한다.
void Prepare(std::uint64_t now) {
    for (std::uint32_t sid=50;sid<=54;++sid) { // 네 이웃의 좌표와 두 캐시를 서로 독립 입력으로 보존한다.
        auto* raw=Slot(sid);std::memset(raw,0xa5,50);Put(raw,0,Pointer(Mapped(0x509470)));Put(raw,4,std::uint16_t{0});Put(raw,6,static_cast<std::uint16_t>(sid==50 ? 81 : 0));Put(raw,8,std::uint16_t{0});raw[10]=136;raw[11]=0;Put(raw,12,std::uint16_t{114});
        Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(sid)+.25F));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(sid)+.5F));raw[33]=1;raw[34]=1;raw[40]=static_cast<std::uint8_t>(sid==50 ? input[1] : 0);Put(raw,36,input[2]);Put(raw,42,input[3]);Put(raw,46,input[4]);
    }
    std::memset(current.data(),0xa5,current.size());std::memset(newRegular.data(),0x5a,newRegular.size());Put(current.data(),0,Pointer(Mapped(0x50f420)));Put(current.data(),8,50U);Put(current.data(),16,now);Put(current.data(),24,input[0]);Put(current.data(),28,input[10]);Put(current.data(),32,input[11]);
    Put(Mapped(0x5c8464),0,Pointer(pool.data()));Put(Mapped(0x59ab20),0,Pointer(types.data()));Put(Mapped(0x55b4d0),0,now);
    Put(Mapped(0x5318ec),0,0U);Put(Mapped(0x5e4794),0,0U);Put(Mapped(0x541348),0,188U);Put(Mapped(0x541128),0,46U);Put(Mapped(0x541240),0,122U);
    trace.clear();markers=reserves=connectionCalls=gReturnBits=gReturnCalls=0;created=killed=false;gReturnEnvironment.fill(0);
}
}

// 각 입력을 두 x87 정밀도에서 실제 실행하고 TSV로 최초 결과만 출력한다.
int main(int argc,char** argv) {
    static_assert(sizeof(void*)==4); // 원래 명령/호출 규약을 실행하는32비트 전용 도우미다.
    if (argc!=4) Fail("사용법: native_fence_event3.exe 선택이미지 입력TSV 출력TSV");Image(argv[1]);std::ifstream source(argv[2]);std::ofstream output(argv[3],std::ios::binary);if (!source || !output) Fail("입출력 경로 오류");std::string line;
    while (std::getline(source,line)) { // Python이 준비한 독립 입력과 정밀도만 읽는다.
        if (line.empty() || line[0]=='#') continue;std::istringstream row(line);std::string values;std::uint64_t now{};unsigned control{};row>>values>>now>>control;
        std::istringstream fields(values);std::string value;std::size_t count{};
        while (std::getline(fields,value,',')) { if (count>=input.size()) Fail("입력 DWORD 초과");input[count++]=static_cast<std::uint32_t>(std::stoull(value)); } // 입력의 비트만 변환한다.
        if (count!=input.size() || (control!=0x27f && control!=0x37f)) Fail("입력/정밀도 오류");Prepare(now);const auto cw=static_cast<std::uint16_t>(control);
        __asm {
            fninit
            fldcw cw
        }
        reinterpret_cast<void(__thiscall*)(void*)>(Mapped(kFrame))(current.data());
        if (gReturnCalls!=1 || Read<std::uint16_t>(gReturnEnvironment.data())!=cw) Fail("실제 부모 반환 호출 수/정밀도 오류");const auto returned=gReturnBits;const auto tag=Read<std::uint16_t>(gReturnEnvironment.data(),8);const auto before=current;const auto beforeTrace=trace;std::string repeated="-";
        if (!input[1]) { // 같은 실제 객체의 다음 호출은 NaN 예약 때문에 사건을 다시 선택하지 않아야 한다.
            auto next=std::bit_cast<double>(now)+1.0;Put(Mapped(0x55b4d0),0,std::bit_cast<std::uint64_t>(next));reinterpret_cast<void(__thiscall*)(void*)>(Mapped(kFrame))(current.data());
            if (gReturnCalls!=1 || current!=before || trace!=beforeTrace) Fail("원문 NaN 예약의 재호출 차이");repeated="1";
        }
        __asm { fninit }
        output<<values<<'\t'<<now<<'\t'<<Hex(Slot(50),250)<<'\t'<<returned<<'\t'<<(trace.empty() ? "-" : trace)<<'\t'<<Read<std::uint64_t>(current.data(),16)<<','<<Read<std::uint32_t>(current.data(),28)<<','<<Read<std::uint32_t>(current.data(),32)<<','<<killed<<'\t';
        if (created) output<<Read<std::uint32_t>(newRegular.data(),8)<<','<<Read<std::uint32_t>(newRegular.data(),24)<<','<<Read<std::uint32_t>(newRegular.data(),28)<<','<<Read<std::uint32_t>(newRegular.data(),32)<<','<<Read<std::uint64_t>(newRegular.data(),16);else output<<'-';
        output<<'\t'<<tag<<'\t'<<repeated<<'\n';
    }
    std::cerr<<"실제32비트 CPU 관측 완료\n";return 0;
}
