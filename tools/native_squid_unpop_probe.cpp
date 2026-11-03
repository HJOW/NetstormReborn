// 원문 공통 Unpop의 현재 진단/부분 쓰기와 실제 부류·해시·CRT를 CPU로 관측한다.
#define wmain BoardAngleUnusedMain
#include "native_board_angle_probe.cpp"
#undef wmain
namespace { // 이 독립 관측기 자신의 payload만 변경한다.
constexpr std::uint32_t kPool=0x600000,kTypes=0x5a0000,kMap=0x5d0000,kHeads=0x570000,kVt=0x61f000,kHashBytes=172544,kPoolBytes=6450; // 원본 함수와 겹치지 않는 명시 입력 공간이다.
constexpr std::array<std::uint32_t,4> kHeadBases{0x570000,0x590000,0x598000,0x59a000},kScales{1,2,4,16}; // 원문 네 단계의 실제 크기/연속 배열이다.
std::uint32_t sourceSid{},unpopAction{};bool notified{};std::string unpopEvents; // 행별 외부 입력과 실제 요청만 보관한다.
void UnpopPut(std::uint32_t address,std::uint32_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,4); } // 원문 비정렬 DWORD를 쓴다.
std::uint32_t UnpopGet(std::uint32_t address) { std::uint32_t value{};std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value; } // 현재 DWORD를 읽는다.
std::uint32_t UnpopSource() { return kPool+sourceSid*50; } // 최초 this의 실제 물리 주소다.
std::uint32_t UnpopBucket(std::uint32_t level,float x,float y) { const auto scale=kScales[level];return kHeadBases[level]+(static_cast<std::uint32_t>(y)/scale*(256/scale)+static_cast<std::uint32_t>(x)/scale)*2; } // 입력 버킷만 배치하며 원문 조회는 실제004b2a90이 한다.
void UnpopWord(std::uint32_t address,std::uint16_t value) { std::memcpy(reinterpret_cast<void*>(address),&value,2); } // 대상/이전 next의 WORD만 쓴다.
std::uint32_t UnpopAdler(std::uint32_t address,std::uint32_t length) { std::uint32_t a=1,b=0;const auto* bytes=reinterpret_cast<const std::uint8_t*>(address);for (std::uint32_t index=0;index<length;++index) { a=(a+bytes[index])%65521U;b=(b+a)%65521U; }return (b<<16)|a; } // 전체 입력 물리의 실제 변경을 체크섬으로 남긴다.
void __cdecl ObserveUnpopAssertion(const char* expression,const char* file,std::uint32_t line) { // 진단 정상 복귀에서 현재 값/이미 읽은 byte를 구별한다.
    unpopEvents+="A,"+std::to_string(line)+':'+expression+':'+file+';';const auto raw=UnpopSource();
    if (unpopAction==1 && line==0x617) { *checking=0;std::memset(reinterpret_cast<void*>(kMap),0xf0,65536); }
    if (unpopAction==2) { *checking=0x80000000;*reinterpret_cast<std::uint8_t*>(raw+10)=71; }
    if (unpopAction==3) { *checking=0;*reinterpret_cast<std::uint8_t*>(raw+10)=70;*reinterpret_cast<std::uint8_t*>(raw+11)=0; }
    if (unpopAction==8) { *checking=0;*reinterpret_cast<std::uint8_t*>(raw+10)=73; }
    if (unpopAction==10 && line==0x65d) UnpopWord(UnpopBucket(0,std::bit_cast<float>(UnpopGet(raw+14)),std::bit_cast<float>(UnpopGet(raw+18))),static_cast<std::uint16_t>(sourceSid));
    if (unpopAction==11) { *checking=0x80000000;*reinterpret_cast<std::uint8_t*>(raw+10)=70;*reinterpret_cast<std::uint8_t*>(raw+11)&=0xf7; }
}
void __cdecl ObserveUnpopMiniMap(float x,float y,std::uint32_t genus) { // 실제 미니맵 helper의 인자만 관측하고 명시 현재 입력을 바꾼다.
    unpopEvents+="M,"+std::to_string(sourceSid)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(genus)+';';const auto raw=UnpopSource();
    if (unpopAction==4) { *checking=0x80000000;*reinterpret_cast<std::uint8_t*>(raw+10)=71;UnpopPut(raw+14,0x41880000);UnpopPut(raw+18,0x41a80000);UnpopPut(kTypes+70*500+0x1d4,2);UnpopPut(kTypes+70*500+0x1d8,2); }
    if (unpopAction==5) *reinterpret_cast<std::uint8_t*>(raw+40)|=8;
    if (unpopAction==6) *reinterpret_cast<std::uint8_t*>(raw+11)|=8;
    if (unpopAction==12) *checking=0;
}
void __cdecl ObserveUnpopNotify(std::uint32_t sid) { // 실제 인라인 옆면 BYTE 쓰기 뒤 통지만 명시 경계다.
    unpopEvents+="N,"+std::to_string(sid)+';';if (notified) return;notified=true;const auto raw=UnpopSource();
    if (unpopAction==7) { *checking=0x80000000;*reinterpret_cast<std::uint8_t*>(raw+10)=71;UnpopPut(raw+14,0x41880000);UnpopPut(raw+18,0x41a80000); }
    if (unpopAction==9) *reinterpret_cast<std::uint8_t*>(raw+11)|=8;
}
void __fastcall ObserveUnpopDetach(std::uint32_t raw,void*) { unpopEvents+="D,"+std::to_string((raw-kPool)/50)+';'; } // 실제 종속 분리 몸체 자체는 이 부모 관측의 외부 경계다.
void __stdcall ObserveUnpopRefresh(std::uint32_t sid) { unpopEvents+="Q,"+std::to_string(sid)+';'; } // 실제 내용물 UI 종단 요청을 별도로 관측한다.
void UnpopUpdate(std::uint32_t raw,std::uint32_t offset) { unpopEvents+="U,"+std::to_string(offset)+','+std::to_string((raw-kPool)/50)+','+std::to_string(*reinterpret_cast<std::uint8_t*>(raw+10))+','+std::to_string(*reinterpret_cast<std::uint8_t*>(raw+11))+';'; } // 실제 가상 표시 요청의 현재 대상/상태다.
void __fastcall ObserveUnpop88(std::uint32_t raw,void*) { UnpopUpdate(raw,0x88); } // 인자 없는 실제 thiscall 경계다.
void __fastcall ObserveUnpop8c(std::uint32_t raw,void*) { UnpopUpdate(raw,0x8c); } // 선택 flags2000의 별도 가상 경계다.
void BindUnpop(std::uint32_t address,const void* observer) { auto* code=reinterpret_cast<std::uint8_t*>(address);code[0]=0xe9;const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(observer)-address-5);std::memcpy(code+1,&delta,4);if (!FlushInstructionCache(GetCurrentProcess(),code,5)) throw std::runtime_error("Unpop 코드 캐시 실패"); } // payload의 알려진 외부 함수만 연결한다.
void UnpopRelink(std::uint32_t address,std::uint32_t target) { auto* code=reinterpret_cast<std::uint8_t*>(address);if (code[0]!=0xe8) throw std::runtime_error("Unpop 실제 CALL 피연산자 오류");const auto delta=target-address-5;std::memcpy(code+1,&delta,4); } // 실제 상대 CALL의 도착 주소만 보정한다.
void CopyUnpopGrid(const std::filesystem::path& path) { // 0040eaf0은 관측기 자체 영역 아래이므로 검증한 전용 payload에 원문77BYTE만 옮긴다.
    constexpr std::uint32_t rva=0xeaf0,length=0x4d,destination=0x61e000; // 순수 표면 조회의 함수 경계와 비충돌 목적지다.
    std::ifstream input(path,std::ios::binary);std::vector<char> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};if (bytes.size()<sizeof(IMAGE_DOS_HEADER)) throw std::runtime_error("표면 조회 PE 범위");
    IMAGE_DOS_HEADER dos{};std::memcpy(&dos,bytes.data(),sizeof(dos));if (dos.e_lfanew<0 || static_cast<std::size_t>(dos.e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size()) throw std::runtime_error("표면 조회 NT 범위");IMAGE_NT_HEADERS32 header{};std::memcpy(&header,bytes.data()+dos.e_lfanew,sizeof(header));const auto first=static_cast<std::size_t>(dos.e_lfanew)+offsetof(IMAGE_NT_HEADERS32,OptionalHeader)+header.FileHeader.SizeOfOptionalHeader;bool copied{};
    for (std::uint32_t index=0;index<header.FileHeader.NumberOfSections;++index) { // 해당 원문 섹션의 실제 파일 구간만 복사한다.
        const auto at=first+index*sizeof(IMAGE_SECTION_HEADER);if (at+sizeof(IMAGE_SECTION_HEADER)>bytes.size()) throw std::runtime_error("표면 조회 섹션 범위");IMAGE_SECTION_HEADER section{};std::memcpy(&section,bytes.data()+at,sizeof(section));if (rva<section.VirtualAddress || static_cast<std::uint64_t>(rva)+length>static_cast<std::uint64_t>(section.VirtualAddress)+section.SizeOfRawData) continue;const auto offset=static_cast<std::uint64_t>(section.PointerToRawData)+rva-section.VirtualAddress;if (offset+length>bytes.size()) throw std::runtime_error("표면 조회 원문 바이트 범위");std::memcpy(reinterpret_cast<void*>(destination),bytes.data()+offset,length);copied=true;break;
    }
    if (!copied) throw std::runtime_error("표면 조회 원문 섹션 누락");UnpopRelink(destination+0xb,0x4e49c0);UnpopRelink(destination+0x1c,0x4e49c0);UnpopRelink(0x4b0146,destination);if (!FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(destination),length) || !FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(0x4b0146),5)) throw std::runtime_error("표면 조회 복사 캐시 실패");
}
void UnpopType(std::uint32_t number,std::uint32_t first,std::uint32_t genus,std::uint32_t width,std::uint32_t height) { const auto raw=kTypes+number*500;UnpopPut(raw+0xe8,first);UnpopPut(raw+0xec,genus);UnpopPut(raw+0x1d4,width);UnpopPut(raw+0x1d8,height); } // 명시 TYPE 입력만 설정한다.
__declspec(noinline) void InvokeUnpop(std::uint32_t flags) { reinterpret_cast<void (__thiscall*)(void*,std::uint32_t)>(0x4afe50)(reinterpret_cast<void*>(UnpopSource()),flags); } // 게임 시작 없이 공통 부모만 호출한다.
}
int wmain(int count,wchar_t** arguments) { // 표준 입력의 명시 행과 실제 출력만 처리한다.
    try {
        AddVectoredExceptionHandler(1,ObserveFault);SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);if (count!=2) throw std::runtime_error("보호 PE 경로가 필요합니다");MappedPe image(arguments[1]);checking=reinterpret_cast<std::uint32_t*>(0x5e4794);UnpopPut(0x5c8464,kPool);UnpopPut(0x5c847c,128);UnpopPut(0x59ab20,kTypes);UnpopPut(0x5c7c44,kMap);UnpopPut(0x5c84bc,kHeads);UnpopPut(0x531928,256);
        for (std::uint32_t level=0;level<4;++level) { UnpopPut(0x5424f8+level*4,kScales[level]);UnpopPut(0x542514+level*4,kHeadBases[level]); } // 실제 hash와 surface는 같은 배열을 읽는다.
        UnpopPut(kVt+0x88,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveUnpop88)));UnpopPut(kVt+0x8c,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&ObserveUnpop8c)));
        BindUnpop(0x4e0620,reinterpret_cast<const void*>(&ObserveUnpopAssertion));BindUnpop(0x481510,reinterpret_cast<const void*>(&ObserveUnpopMiniMap));BindUnpop(0x4214a0,reinterpret_cast<const void*>(&ObserveUnpopNotify));BindUnpop(0x4ac6b0,reinterpret_cast<const void*>(&ObserveUnpopDetach));BindUnpop(0x4ad230,reinterpret_cast<const void*>(&ObserveUnpopRefresh));CopyUnpopGrid(arguments[1]);
        std::uint32_t index{},gate{},type{},state{},extra{},xb{},yb{},width{},height{},genus{},first{},layout{},flags{},seed{},surface{}; // 행별 초기 입력이며 결과를 예측하지 않는다.
        while (std::cin>>index>>gate>>sourceSid>>type>>state>>extra>>xb>>yb>>width>>height>>genus>>first>>layout>>flags>>unpopAction>>seed>>surface) { // 원문 부류/CRT/해시/진단 분기 자체를 실행한다.
            std::memset(reinterpret_cast<void*>(kPool),0,kPoolBytes);std::memset(reinterpret_cast<void*>(kHeads),0,kHashBytes);std::memset(reinterpret_cast<void*>(kMap),static_cast<int>(seed),65536);*checking=gate;notified=false;unpopEvents.clear();
            UnpopType(70,first,genus,width,height);UnpopType(6,first,genus,width,height);UnpopType(71,0x800,1,3,3);UnpopType(72,0,6,2,2);UnpopType(73,0,0,1,1);
            const auto raw=UnpopSource();UnpopPut(raw,kVt);*reinterpret_cast<std::uint8_t*>(raw+10)=static_cast<std::uint8_t>(type);*reinterpret_cast<std::uint8_t*>(raw+11)=static_cast<std::uint8_t>(state);*reinterpret_cast<std::uint8_t*>(raw+40)=static_cast<std::uint8_t>(extra);UnpopPut(raw+14,xb);UnpopPut(raw+18,yb);UnpopWord(raw+4,51);UnpopWord(kPool+49*50+4,static_cast<std::uint16_t>(sourceSid));*reinterpret_cast<std::uint8_t*>(kPool+70*50+10)=83;*reinterpret_cast<std::uint8_t*>(kPool+70*50+12)=0xff;
            if (surface) for (std::uint32_t y=17;y<=22;++y) for (std::uint32_t x=13;x<=18;++x) UnpopWord(kHeads+(y*256+x)*2,70); // 주변 실제 표면 WORD 입력이다.
            const auto x=std::bit_cast<float>(xb),y=std::bit_cast<float>(yb);if (layout==1 || layout==4) UnpopWord(UnpopBucket(0,x,y),static_cast<std::uint16_t>(sourceSid));if (layout==2) UnpopWord(UnpopBucket(1,x,y),49);if (layout==3 || layout==4) UnpopWord(UnpopBucket(3,x,y),static_cast<std::uint16_t>(sourceSid));
            std::cout<<index<<'\t'<<gate<<'\t'<<sourceSid<<'\t'<<type<<'\t'<<state<<'\t'<<extra<<'\t'<<xb<<'\t'<<yb<<'\t'<<width<<'\t'<<height<<'\t'<<genus<<'\t'<<first<<'\t'<<layout<<'\t'<<flags<<'\t'<<unpopAction<<'\t'<<seed<<'\t'<<surface;InvokeUnpop(flags);std::cout<<'\t'<<*checking<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+10))<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+11))<<'\t'<<static_cast<unsigned>(*reinterpret_cast<std::uint8_t*>(raw+40))<<'\t'<<UnpopGet(raw+14)<<'\t'<<UnpopGet(raw+18)<<'\t'<<UnpopGet(kTypes+70*500+0x1d4)<<'\t'<<UnpopGet(kTypes+70*500+0x1d8)<<'\t'<<UnpopAdler(kPool,kPoolBytes)<<'\t'<<UnpopAdler(kHeads,kHashBytes)<<'\t'<<UnpopAdler(kMap,65536)<<'\t'<<(unpopEvents.empty() ? "-" : unpopEvents)<<'\n';
        }return 0;
    }catch (const std::exception& failure) { std::cerr<<failure.what()<<'\n';return 1; }
}
