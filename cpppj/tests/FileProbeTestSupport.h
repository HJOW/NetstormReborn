// 세실제exists부모의metadata/loose충돌과Unicode작업파일을검사입력으로구성한다.
#pragma once
#include "RawSceneSupport.h"
#include "client/GifScreenshot.h"
#include "platform/Utf8Path.h"
#include "o/RawFileName.h" // 실제Client/VFS의같은고리번호를관측한다.
#include <chrono>
#include <fstream>

namespace netstorm::test::fileprobe { // 보호원본/기존fixture를수정하지않는공통입력작성기다.
inline std::filesystem::path Work() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-file-probe-765/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/std::filesystem::path(u8"존재😀"); } // 각검사의새Unicode출력부모다.
inline void Write(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { std::filesystem::create_directories(path.parent_path());std::ofstream stream(path,std::ios::binary);stream.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(stream.good()); } // scratch입력만작성하고폴더/파일을숨겨서성공시키지않는다.
inline std::vector<std::uint8_t> Taff(std::string name,std::span<const std::uint8_t> payload) { // 한이름의정상metadata와명시payload를독립입력으로작성한다.
    std::replace(name.begin(),name.end(),'/','\\');if (!name.starts_with('\\')) name.insert(name.begin(),'\\');const auto directorySize=(8+name.size()+1+3)&~std::size_t{3};const auto dataOffset=0x30+directorySize;std::vector<std::uint8_t> bytes(dataOffset+payload.size());const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());
    rawscene::Put(bytes,0x14,1);rawscene::Put(bytes,0x1c,0x2c);rawscene::Put(bytes,0x20,0x30);rawscene::Put(bytes,0x24,static_cast<std::uint32_t>(directorySize));rawscene::Put(bytes,0x28,static_cast<std::uint32_t>(dataOffset));rawscene::Put(bytes,0x34,static_cast<std::uint32_t>(payload.size()));std::copy(name.begin(),name.end(),bytes.begin()+0x38);std::copy(payload.begin(),payload.end(),bytes.begin()+static_cast<std::ptrdiff_t>(dataOffset));return bytes;
}
inline std::vector<std::uint8_t> Gif(std::uint32_t width,std::uint32_t height,std::uint8_t color) { // 순수encoder를입력작성에만사용하며기대인덱스는명시색상이다.
    const std::vector<std::uint8_t> pixels(std::size_t(width)*height,color);const std::array<std::uint8_t,768> palette{};return client::EncodeScreenshotGif(width,height,width,pixels,palette);
}
}
