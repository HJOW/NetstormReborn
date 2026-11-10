// 원문 Help의41baa0 호출을 같은 실제 VFS·Unicode FILE·키 반납에 연결한다.
#include "client/QuickHelpFileRead.h"
#include "client/ClientMain.h"
#include "client/KoreanResources.h"
#include "platform/Utf8Path.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace netstorm::client { // 원문 File 의미와 Windows10/11 OS 파일 포인터의 폭을 분리한다.
namespace { // 이 동기 읽기에만 필요한 실제 CRT 장치를 소유한다.
struct HelpFileReader { // 원문 지역 File 완료 뒤 정리와 호스트의 미완성 Open 자원 정리를 함께 소유한다.
    o::RawBaseFileState state{}; // 원시 필드의 현재 FILE/키 토큰이며24BYTE ABI를 가장하지 않는다.
    o::RawBaseFileIOHooks io; // 실제 rb/seek/tell/read/close와 키 할당 경계다.
    std::vector<std::uint8_t> key; // 현재 File이 끝까지 소유하는 NUL 포함 키 물리다.
    bool complete{true}; // 원문 미정 tail을 성공한 문자열로 노출하지 않기 위한 호스트 부분I/O 경계다.
    HelpFileReader() { // 반환 길이는 새 원문 규칙으로 재해석하지 않고 CRT의 실제 성공 여부만 별도로 기록한다.
        o::ConstructBaseFile(state);
        io.open=[](std::string_view name,std::string_view mode) { // 원문 ANSI 대신 Unicode W CRT로 연결하는 Windows10/11 경계다.
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문의 signed32 fseek 실패 반환은 사용하지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // -1도 원문 length 계산에 그대로 전달하고 이후 물리 경계에서 거부한다.
        io.read=[this](auto file,auto bytes,auto size,auto count) { const auto actual=std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file));complete=complete && (!size || !count || actual==count); }; // 부분 읽기는 재시도하지 않으며 암호 scratch 처리 뒤에도 실패 표시를 보존한다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 원문은 fclose의 반환값을 읽지 않는다.
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; }; // 할당 예외는 이미 열린 FILE을 되돌리지 않고 바깥 소유자가 반납한다.
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Help 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Help 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("Help 키 반납 토큰");key.clear(); }; // Close와 키 반납의 원문 순서를 섞지 않는다.
        io.assertion=[](auto line) { throw std::logic_error("Help BaseFile 원문 진단 "+std::to_string(line)); }; // 선택된 미복원 디버그 UI를 성공으로 대체하지 않는다.
    }
    ~HelpFileReader() { o::DestructBaseFile(state,io); } // raw41baa0이 반납한 FILE/키는0이므로 중복 닫지 않는다. 미완성 Open의 남은 OS 자원 추가 반납은 현대 호스트 경계다.
};
}
// 원문4c5809의 고정 인수와 초기0인 출력 지역을 실제 전체 읽기 부모에 전달한다.
std::uintptr_t ReadQuickHelpFileBytes(o::RawBaseFileState& state,std::span<const std::uint8_t> name,std::int32_t& length,const o::RawBaseFileIOHooks& io,const o::BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch) {
    length=0;return o::ReadWholeBaseFile(state,name,0,&length,2,io,hooks,scratch);
}
// 실제 Help만 명시 실시간 읽기를 사용한다. 기존 Client 값형 Files().Read/TryRead의 snapshot·optional 계약은 유지한다.
std::optional<std::string> ReadClientQuickHelpFile(const Client& client,std::string_view name) {
    HelpFileReader reader;std::vector<std::uint8_t> allocation;std::int32_t length{};o::BaseFileWholeReadHooks whole; // 추가2/최소4 물리는 본문과 구별해 소유한다.
    whole.open=[&](auto& file,auto physical,auto mode,auto create) { client.Files().Open(file,{reinterpret_cast<const char*>(physical.data()),physical.size()},mode,create,reader.io); };
    whole.allocate=[&](auto size) { allocation.resize(size);return o::BaseFileWholeAllocation{1,allocation}; }; // vector의 초기0 padding은 반환하지 않으며 원문 미정 할당의 값으로 주장하지 않는다.
    const auto selected=client.ResolveKoreanBuiltinFile(name);const auto physical=selected ? platform::PathToUtf8(*selected):std::string(name); // 기존Help원문파일이있으면resolver는빈값이다.
    std::string input(physical.substr(0,physical.find('\0')));input+='\0';std::array<std::uint8_t,4096> scratch{}; // 짧은 암호 읽기의 원문 미정 scratch는 호스트에서 초기화하되 결과 문자열은 거부한다.
    const auto result=ReadQuickHelpFileBytes(reader.state,{reinterpret_cast<const std::uint8_t*>(input.data()),input.size()},length,reader.io,whole,scratch);
    if (!reader.complete) throw std::runtime_error("Help 전체읽기의 실제 부분 I/O");if (!result) return {};
    return DecodeQuickHelpDisplaySource({reinterpret_cast<const char*>(allocation.data()),static_cast<std::size_t>(length)}); // 기존 창이 LF를 한 번 더하고 std::string의 NUL로 원문 추가2BYTE의 논리 끝을 보존한다.
}
}
