// 실제 Help 읽기 검사의 입력만 새 Unicode 작업 폴더에 작성한다.
#pragma once
#include "RawSceneSupport.h"
#include "client/ClientMain.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace netstorm::test::helpread { // 원본/기존fixture를 수정하지 않는 검사 입력 생성기다.
inline std::filesystem::path Work() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-quick-help-file-767/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/std::filesystem::path(u8"도움말😀"); } // 각 검사는 별개의 실제 Unicode 루트를 쓴다.
inline void Write(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { std::filesystem::create_directories(path.parent_path());std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(output.good()); } // 검사 소유의 명시 바이트만 기록한다.
inline void Text(const std::filesystem::path& path,std::string_view text) { Write(path,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // NUL/빈 입력도 잘라내지 않는다.
inline client::ClientOptions Options(const std::filesystem::path& root) { client::ClientOptions result;result.gameDirectory=root;result.noAudio=true;return result; } // 창/Run 없이 같은 Client 파일 소유자를 만든다.
inline std::vector<std::uint8_t> Archive(std::string name,std::span<const std::uint8_t> payload,bool encrypted=false) { // 한 항목의 TAFF metadata와 원문 키 payload를 독립 입력으로 만든다.
    std::replace(name.begin(),name.end(),'/','\\');if (!name.starts_with('\\')) name.insert(name.begin(),'\\');const auto directorySize=(8+name.size()+1+3)&~std::size_t{3};const auto dataOffset=0x30+directorySize;std::vector<std::uint8_t> bytes(dataOffset+payload.size());const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());
    rawscene::Put(bytes,0x14,1);rawscene::Put(bytes,0x18,encrypted ? 2U:0U);rawscene::Put(bytes,0x1c,0x2c);rawscene::Put(bytes,0x20,0x30);rawscene::Put(bytes,0x24,static_cast<std::uint32_t>(directorySize));rawscene::Put(bytes,0x28,static_cast<std::uint32_t>(dataOffset));rawscene::Put(bytes,0x34,static_cast<std::uint32_t>(payload.size()));std::copy(name.begin(),name.end(),bytes.begin()+0x38);
    constexpr std::string_view key="mydoghasfleas"; // 실제 Open 부모가 설치하는 고정 TAFF 키다.
    for (std::size_t index=0;index<payload.size();++index) bytes[dataOffset+index]=static_cast<std::uint8_t>(payload[index]^(encrypted ? static_cast<std::uint8_t>(key[index%key.size()]):0U)); // 기대 데이터와 별개인 저장 입력만 암호화한다.
    return bytes;
}
}
