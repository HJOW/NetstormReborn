// Config의41baa0 호출을 같은 실제 VFS·Unicode FILE·키 반납에 연결한다.
#include "client/ConfigFileRead.h"
#include "o/RawConfigFileLoad.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace netstorm::client { // 원문 File 의미와 Windows10/11 OS 파일 포인터의 폭을 분리한다.
namespace { // 이 동기 읽기에만 필요한 실제 CRT 장치를 소유한다.
struct ConfigFileReader { // 원문 지역 File 완료 뒤 정리와 호스트의 미완성 Open 자원 정리를 함께 소유한다.
    o::RawBaseFileState state{}; // 원시 필드의 현재 FILE/키 토큰이며24BYTE ABI를 가장하지 않는다.
    o::RawBaseFileIOHooks io; // 실제 rb/seek/tell/read/close와 키 할당 경계다.
    std::vector<std::uint8_t> key; // 현재 File이 끝까지 소유하는 NUL 포함 키 물리다.
    bool complete{true}; // 원문 미정 tail을 성공한 이미지로 노출하지 않기 위한 호스트 부분I/O 경계다.
    ConfigFileReader() { // 반환 길이는 새 원문 규칙으로 재해석하지 않고 CRT의 실제 성공 여부만 별도로 기록한다.
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
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Config 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("Config 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("Config 키 반납 토큰");key.clear(); }; // Close와 키 반납의 원문 순서를 섞지 않는다.
        io.assertion=[](auto line) { throw std::logic_error("Config BaseFile 원문 진단 "+std::to_string(line)); }; // 선택된 미복원 디버그 UI를 성공으로 대체하지 않는다.
    }
    ~ConfigFileReader() { o::DestructBaseFile(state,io); } // raw41baa0이 반납한 FILE/키는0이므로 중복 닫지 않는다. 미완성 Open의 남은 OS 자원 추가 반납은 현대 호스트 경계다.
};
}
// 실제Client는Config파일에만이장치를설치한다.공유Reader의값/snapshot은유지한다.
o::ConfigFileLoadHooks MakeConfigFileReadHooks(const o::BaseFileSystem& files,ConfigFilePathResolver resolve) {
    o::ConfigFileLoadHooks hooks;hooks.probe=[&files,resolve](auto name) { const auto selected=resolve ? resolve(name):std::nullopt;return files.Probe(selected ? platform::PathToUtf8(*selected):std::string(name)); }; // 한국어번들만직접물리경로로바꾸고영어는원래query를그대로쓴다.
    hooks.load=[&files,resolve](o::Config& config,std::string_view name) {
        const auto selected=resolve ? resolve(name):std::nullopt;const auto physicalName=selected ? platform::PathToUtf8(*selected):std::string(name); // 실제FILE/키/해독/정리의원문부모는그대로사용한다.
        ConfigFileReader reader;std::vector<std::uint8_t> allocation;o::BaseFileWholeReadHooks whole;o::ConfigFileLoadBodyHooks body; // Append복귀까지같은물리할당을소유한다.
        whole.probe=[&](auto physical) { return files.Probe({reinterpret_cast<const char*>(physical.data()),physical.size()}); };
        whole.open=[&](auto& file,auto physical,auto mode,auto create) { files.Open(file,{reinterpret_cast<const char*>(physical.data()),physical.size()},mode,create,reader.io); };
        whole.allocate=[&](auto size) { allocation.resize(size);return o::BaseFileWholeAllocation{1,allocation}; }; // 호스트padding0은원문미정바이트와동일하다고주장하지않는다.
        body.buffer=[&](auto token) { if (token!=1) throw std::logic_error("Config 실제 할당 토큰");if (!reader.complete) throw std::runtime_error("Config 전체읽기의 실제 부분 I/O");return std::span(allocation); };
        body.append=[&](auto,std::span<const std::uint8_t> bytes) { config.AppendDecodedBytes(bytes); }; // 원문XOR는40380에서끝났으므로기존LoadBytes로두번해독하지않는다.
        body.release=[&](auto token) { if (token!=1) throw std::logic_error("Config 실제 반납 토큰");std::vector<std::uint8_t>{}.swap(allocation); }; // Append가정상복귀한뒤에만원문free를연결한다.
        std::string input(physicalName.substr(0,physicalName.find('\0')));input+='\0';std::array<std::uint8_t,4096> scratch{};o::LoadConfigFile(reader.state,{reinterpret_cast<const std::uint8_t*>(input.data()),input.size()},reader.io,whole,body,scratch); // 호스트예외의남은할당은지역RAII로추가반납하며원문예외ABI와구별한다.
    };return hooks;
}
// 원문chdir대신검색시점의현재게임CWD를사용하며FindClose/Next는같은Windows핸들을소비한다.
o::FileSpecQueryHooks MakeConfigFileSearchHooks(std::function<std::filesystem::path()> directory) {
    auto hooks=o::FileSpecDirectoryHooks({});hooks.find=[directory=std::move(directory)](auto name) { if (!directory) throw std::logic_error("Config 현재검색root 미연결");return o::FileSpecDirectoryHooks(directory()).find(name); };return hooks;
}
}
