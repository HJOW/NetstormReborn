// 일반 whole read의 최소폭/추가NUL 없이 같은 Xlat 버퍼와 File 수명을 보존한다.
#include "client/XlatFileRead.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include "o/RawXlat.h"
#include "platform/Utf8Path.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <vector>

namespace netstorm::client { // 파일 부모와 원문 바이트 장부의 실제 연결이다.
namespace { // 같은 동기 FILE/키를 장부 정규화까지 보관한다.
template<class Function> const Function& Need(const Function& function) { // 미선택 장치의 누락은 앞서 검사하지 않는다.
    if (!function) throw std::logic_error("Xlat 파일의 선택된 실제 장치 미연결");return function;
}
struct XlatFileReader { // 원문의 완료 File 정리와 미완성 Open의 현대 OS 회수를 구별한다.
    o::RawBaseFileState state{}; // 현재 호스트 폭 FILE/키 필드다.
    o::RawBaseFileIOHooks io; // 검색/metadata는 VFS에 맡기며 CRT와 키 물리만 소유한다.
    std::vector<std::uint8_t> key; // 정규화와 정상 닫기까지 살아 있는 같은 키다.
    XlatFileReader() { // 생성만으로 파일/장부를 읽지 않는다.
        o::ConstructBaseFile(state);
        io.open=[](auto name,auto mode) { // UTF-8 게임 경로를 기존 wide CRT 경계로 연다.
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring text(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),text.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문처럼 CRT 반환을 검사하지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // 원문의 signed32 -1 길이도 부모에 전달한다.
        io.read=[](auto file,auto bytes,auto size,auto count) { if (!size || !count) return;static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 길이0 vector의 NULL 물리를 CRT에 넘기지 않으며0BYTE의 위치/내용은 바꾸지 않는다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 정규화 뒤·파싱 전에 정상 닫는다.
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; }; // 같은 reader가 키를 동기 소유한다.
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Xlat 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Xlat 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("Xlat 키 반납 토큰");key.clear(); }; // Close 뒤에만 키를 반납한다.
        io.assertion=[](auto line) { throw std::logic_error("Xlat BaseFile 원문 진단 "+std::to_string(line)); };
    }
    ~XlatFileReader() { o::DestructBaseFile(state,io); } // 미완성 Open의 잔여 OS 자원 회수는 현대 소유 경계이며 정상 정리를 합성하지 않는다.
};
}
bool ReadXlatFile(o::RawBaseFileState& state,std::uint32_t which,std::string_view literal,
    const o::RawBaseFileIOHooks& io,const XlatFileReadHooks& hooks,std::span<std::uint8_t> scratch) { // 같은 버퍼/장부를 복제하지 않는 실제 File 부모다.
    if (Need(hooks.hasBuffer)(which)) Need(hooks.assertion)(0xe1);
    const auto path=Need(hooks.path)(literal);
    if (!Need(hooks.exists)(path)) { Need(hooks.log)(XlatFileFailure::Missing,path);return false; }
    state.key=0;state.keyLength=0;Need(hooks.open)(state,path,0,1); // 이 Open이 완료되기 전에는 원문 File cleanup 상태가 아니다.
    bool loaded{};
    try {
        if (!state.file) Need(hooks.assertion)(0xf2);
        const auto length=o::BaseFileLength(state,io);
        if (length==-1) Need(hooks.log)(XlatFileFailure::Length,path);
        else {
            if (Need(hooks.hasBuffer)(which)) Need(hooks.releaseExisting)(which);
            const auto requested=static_cast<std::uint32_t>(length);const auto allocation=Need(hooks.allocate)(which,requested);
            if (!allocation.token) Need(hooks.assertion)(0x103);
            if (allocation.bytes.size()<requested) throw std::out_of_range("Xlat 원문 할당의 실제 읽기 범위");
            o::ReadBaseFile(state,allocation.bytes.first(requested),-1,io,scratch);Need(hooks.normalize)(which);loaded=true;
        }
    } catch (...) { o::DestructBaseFile(state,io);throw; } // 실패한 부분 버퍼/앞선 언어는 되돌리지 않는다.
    o::DestructBaseFile(state,io); // 정상 소멸 실패를 다시 소멸하지 않고 파서도 실행하지 않는다.
    if (loaded) Need(hooks.parse)(which); // 원문은 정규화 File을 닫은 뒤 파싱한다. 파싱 예외에서도 File을 다시 닫지 않는다.
    return loaded;
}
bool LoadNativeXlatFile(o::RawXlat& xlat,const o::BaseFileSystem& files,std::uint32_t which,std::string_view literal,
    const std::function<void(XlatFileFailure,std::string_view)>& log) { // 실제 Client의 현재7칸과 VFS를 한 번의 동기 적재에 빌린다.
    XlatFileReader reader;std::array<std::uint8_t,4096> scratch{};XlatFileReadHooks hooks; // 명시0 작업 물리/할당은 원문 미정 stack·힙 자체와의 동일성 주장이 아니다.
    hooks.hasBuffer=[&](auto index) { return xlat.HasBuffer(index); };
    hooks.path=[&](auto name) { const std::array<o::FileNamePart,2> parts{{{-2,"d"},{-3,name}}};return std::string(files.FileNameOwner()->Format(parts)); }; // 선택 root는 현재 VFS가 담당한다.
    hooks.exists=[&](auto path) { return files.Probe(path)!=0; };hooks.log=log;
    hooks.open=[&](auto& state,auto path,auto mode,auto create) { files.Open(state,path,mode,create,reader.io); };
    hooks.releaseExisting=[](auto) { throw std::logic_error("Xlat 중복 진단 복귀 뒤 기존 노드 포인터 재사용 경계"); }; // 기존711의 미정 재적재 거부 계약을 유지한다.
    hooks.allocate=[&](auto index,auto size) { return XlatFileAllocation{1,xlat.AllocateFileBuffer(index,size)}; }; // 빈 설치도 논리 할당 존재와 구별하며 길이를 늘리지 않는다.
    hooks.normalize=[&](auto index) { xlat.NormalizeFileBuffer(index); };hooks.parse=[&](auto index) { xlat.ParseFileBuffer(index); };
    hooks.assertion=[](auto line) { throw std::logic_error("Xlat 파일 부모의 원문 진단 "+std::to_string(line)); };
    return ReadXlatFile(reader.state,which,literal,reader.io,hooks,scratch);
}
void InitializeNativeXlat(o::RawXlat& xlat,const o::BaseFileSystem& files,
    const std::function<void(XlatFileFailure,std::string_view)>& log) { // 원문 시작의 세 literal/which 순서를 보존한다.
    constexpr std::array<std::pair<std::uint32_t,std::string_view>,3> kSources{{{3,"xlat.german"},{1,"xlat.english"},{6,"xlat.portuguese"}}}; // 다른 언어를 부재 fallback으로 추가하지 않는다.
    for (const auto& [which,literal]:kSources) static_cast<void>(LoadNativeXlatFile(xlat,files,which,literal,log)); // false의 부재/길이실패 뒤에도 다음 파일을 시도한다.
}
}
