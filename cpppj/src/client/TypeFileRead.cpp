// TYPE의 필수 전체읽기만 같은 VFS·Unicode FILE·키 수명에 연결한다.
#include "client/TypeFileRead.h"
#include "o/OriginalText.h"
#include "platform/Utf8Path.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace netstorm::client { // 32비트 원문 토큰과 현재 OS 포인터의 폭을 구별한다.
namespace { // 동기 TYPE 한 파일의 실제 장치만 이 범위에서 소유한다.
struct TypeFileReader { // 원문 File 완료 정리와 미완성 Open의 호스트 자원 정리를 소유한다.
    o::RawBaseFileState state{}; // 현재 파일·키와 원문 필드이며24BYTE ABI를 가장하지 않는다.
    o::RawBaseFileIOHooks io; // 동일 FILE의 열기/위치/읽기/닫기 및 키 장치다.
    std::vector<std::uint8_t> key; // SetKey가 만든 NUL 포함 현재 키의 실제 물리다.
    bool complete{true}; // 실제 짧은 fread의 미정 tail을 정상 TYPE로 해석하지 않기 위한 호스트 경계다.
    TypeFileReader() { // 반환값을 새 원문 성공 규칙으로 바꾸지 않고 CRT의 부분 읽기만 별도 기록한다.
        o::ConstructBaseFile(state);
        io.open=[](std::string_view name,std::string_view mode) { // 원문 ANSI 대신 Windows10/11 Unicode CRT를 사용하는 호환 경계다.
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문의 signed32 fseek 반환은 사용하지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // -1도 원문 계산 뒤 물리 경계에서 거부한다.
        io.read=[this](auto file,auto bytes,auto size,auto count) { const auto actual=std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file));complete=complete && (!size || !count || actual==count); }; // 부분 읽기를 재시도하지 않으며 복호화 뒤에도 실패 표시를 보존한다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 원문은 fclose 반환을 읽지 않는다.
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; }; // 할당 예외의 남은 FILE은 바깥 소유자가 정리한다.
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("TYPE 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("TYPE 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("TYPE 키 반납 토큰");key.clear(); }; // Close 뒤 키 반납의 원문 순서를 유지한다.
        io.assertion=[](auto line) { throw std::logic_error("TYPE BaseFile 원문 진단 "+std::to_string(line)); }; // 미연결 디버그 UI를 성공으로 대신하지 않는다.
    }
    ~TypeFileReader() { o::DestructBaseFile(state,io); } // raw41baa0이 반납한0토큰은 중복 정리하지 않으며 미완성 Open 자원만 추가 정리한다.
};
}
// 원문 전역540cf8의 현재 복원된 값 d와 실제 구분자 부품을 같은 owner에 전달한다.
std::string_view FormatTypeFileName(o::RawFileName& names,std::string_view typeName) {
    const std::array<o::FileNamePart,4> parts{{{-2,"\\"},{-2,"d"},{-3,typeName},{-4,".type"}}};return names.Format(parts);
}
// 49c160은 전역 curFileSize를 미리0으로 쓰지 않으며 check/extra는 모두0이다.
std::uintptr_t ReadTypeFileBytes(o::RawBaseFileState& state,std::span<const std::uint8_t> name,std::int32_t& length,const o::RawBaseFileIOHooks& io,const o::BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch) {
    return o::ReadWholeBaseFile(state,name,0,&length,0,io,hooks,scratch);
}
// 현재 Native 시작의 TYPE만 명시 실행하며 기존 parser/Decode와 값형 snapshot을 유지한다.
o::RiftTypeDefinition ReadNativeTypeDefinition(const o::BaseFileSystem& files,std::string_view typeName) {
    TypeFileReader reader;std::vector<std::uint8_t> allocation;std::int32_t length{};o::BaseFileWholeReadHooks whole; // 동기 한 파일의 새 호스트 길이는0이며 최소4 할당은 본문과 구별한다.
    whole.open=[&](auto& file,auto physical,auto mode,auto create) { files.Open(file,{reinterpret_cast<const char*>(physical.data()),physical.size()},mode,create,reader.io); };
    whole.allocate=[&](auto size) { allocation.resize(size);return o::BaseFileWholeAllocation{1,allocation}; }; // vector 초기0 padding은 파서에 노출하지 않고 원문 미정 값으로 주장하지 않는다.
    const auto names=files.FileNameOwner();static_cast<void>(FormatTypeFileName(*names,typeName));std::array<std::uint8_t,4096> scratch{}; // 원문 고리의 같은 실제 NUL 포함 칸을 Open에 빌려 주며 별도 문자열로 복제하지 않는다.
    const auto result=ReadTypeFileBytes(reader.state,names->CurrentBytes(),length,reader.io,whole,scratch);
    if (!reader.complete) throw std::runtime_error("TYPE 전체읽기의 실제 부분 I/O");
    if (!result || length==0) throw std::runtime_error("TYPE 원문 진단 0x51c: curFileBuffer != NULL && curFileSize != 0"); // 필수 파일의 빈 본문을 정상 빈 정의로 합성하지 않는다.
    const auto decoded=o::DecodeOriginalText(std::span<const std::uint8_t>(allocation).first(static_cast<std::size_t>(length)));
    return o::RiftTypeDefinition::Parse(decoded); // 원문49c0f0 scanner/read-ahead는 재현하지 않는다. 기존 Parse가 완료/실패할 때 두 소유 버퍼를 RAII로 정리하는 호스트 수명 경계다.
}
}
