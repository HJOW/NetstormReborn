// 43c4e0의 기존 부모를 실제 CRT/현재VFS와 같은 장기9표에 연결한다. 생성 파일은 사용자Cache 경로만 받는다.
#include "client/ClientColorSortInput.h"
#include "o/RawColorMapCache.h"
#include "o/RawColorSortPrimary.h"
#include "o/RawBaseFileIO.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <cstdio>
#include <bit>
#include <vector>

namespace netstorm::client { // 실제 저장소 수명은 ClientColorSortInput이고 FILE/키는 동기 초기화 동안만 소유한다.
namespace { // 원문 주소 토큰과64비트 CRT 포인터를 별도 상태로 관리한다.
constexpr std::array<std::uint32_t,8> kOwnerColors{84,249,184,140,150,112,156,222}; // PE005319c8의 현재 초기8DWORD다.
struct ColorMapFile { // reader와 writer는 서로 다른 실제 FILE/키 소유자다.24BYTE ABI를 가장하지 않는다.
    o::RawBaseFileState state{}; // 실제 CRT 주소와 부분파일/키 상태다.
    o::RawBaseFileIOHooks io; // 현재 디스크/TAFF의 같은 열기·위치·읽기·쓰기·반납 장치다.
    std::vector<std::uint8_t> key; // 이 FILE의 실제 NUL 포함 복호화 키다.
    ColorMapFile() { // 반환 길이를 재시도 조건으로 바꾸지 않는 원문 장치를 제공한다.
        o::ConstructBaseFile(state);
        io.open=[](auto name,auto mode)->std::uintptr_t { const auto path=platform::PathFromUtf8(name);const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str())); }; // Windows10/11의Unicode 경계다.
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); };
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); };
        io.read=[](auto file,auto bytes,auto size,auto count) { if (size && count) static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 짧은 읽기는 목적 표의 접두만 바꾼다.
        io.write=[](auto file,auto bytes,auto size,auto count) { if (size && count) static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 반환은 원문43c4e0처럼 무시한다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); };
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 1; };
        io.mutableKey=[this](auto token,auto size) { if (token!=1 || size>key.size()) throw std::out_of_range("색표 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=1 || size>key.size()) throw std::out_of_range("색표 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=1) throw std::out_of_range("색표 키 반납 토큰");key.clear(); };
        io.assertion=[](auto line) { throw std::logic_error("색표 BaseFile 원문 진단 "+std::to_string(line)); }; // 선택된 원문 진단을 빈 성공으로 바꾸지 않는다.
    }
    ~ColorMapFile() { o::DestructBaseFile(state,io); } // 미완성 열기의 호스트 자원도 회수하며 정상Close 뒤에는FILE0이다.
};
}

// 출력은 처음부터 같은 실제 표이며 정상 hit에서 buildMaps나 writer를 호출하지 않는다.
void ClientColorSortInput::InitializeCache(const o::BaseFileSystem& files,const std::filesystem::path& path,std::uint32_t currentRed,const std::function<void()>& initializeOwners) {
    static_cast<void>(Primary());if (built_ || !initializeOwners) throw std::logic_error("색표 캐시 최초 초기화 계약");
    o::ColorMapCacheGlobals globals{[this] { return Primary()->Bytes(); },kOwnerColors,ownerHighlight_,ownerSolid_,
        {maps_[2],maps_[7],maps_[6],maps_[4],maps_[3],maps_[5],maps_[1],maps_[0],maps_[8]}}; // 원문 파일 순서는 메모리 주소 순서와 다르다.
    ColorMapFile reader,writer;std::array<std::uint8_t,24> readObject,writeObject;std::array<std::uint8_t,4096> scratch{}; // raw 부모의 지역 두 객체와 암호 묶음 물리를 분리한다.
    auto name=platform::PathToUtf8(path);name+='\0';o::ColorMapCacheHooks hooks; // 5차 요구로 설치DataDir 대신 사용자Cache의 실제 이름을 사용한다.
    hooks.path=[&] { return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()); };
    hooks.owners=initializeOwners;hooks.buildMaps=[&] { static_cast<void>(Rebuild(currentRed)); }; // 같은 최초 primary와 현재 빨강으로만9표를 생성한다.
    hooks.openRead=[&](auto,auto,auto mode,auto create) { files.Open(reader.state,std::string_view(name.data(),name.size()-1),mode,create,reader.io); };
    hooks.openWrite=[&](auto,auto,auto mode,auto create) { files.Open(writer.state,std::string_view(name.data(),name.size()-1),mode,create,writer.io); };
    hooks.read=[&](auto,auto table,auto offset) { o::ReadBaseFile(reader.state,table,std::bit_cast<std::int32_t>(offset),reader.io,scratch);return 0U; }; // 실제fread 반환을 읽힌 BYTE 길이로 합성하지 않는다.
    hooks.write=[&](auto,auto table,auto offset) { o::WriteBaseFile(writer.state,table,std::bit_cast<std::int32_t>(offset),writer.io);return 0U; };
    hooks.close=[&](auto object) { auto& file=object.data()==readObject.data() ? reader:writer;o::DestructBaseFile(file.state,file.io); };
    o::InitializeColorMapCache(globals,hooks,readObject,writeObject);built_=true; // 짧은 파일도 원문처럼 선행 표/부분 읽기 효과를 남긴다.
}
std::span<const std::uint8_t,2048> ClientColorSortInput::OwnerHighlight() const { static_cast<void>(Maps());return ownerHighlight_; } // 실제 초기화 뒤의 같은 명도 표다.
std::span<const std::uint8_t,2048> ClientColorSortInput::OwnerSolid() const { static_cast<void>(Maps());return ownerSolid_; } // 실제 초기화 뒤의 같은 고정색 표다.
}
