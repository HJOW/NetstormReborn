// generic41baa0과 다른 GIF 부모의 exactLength/0fill/Decode-before-close 계약이다.
#include "client/GifFileRead.h"
#include "client/GifImage.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace netstorm::client { // 실제 장치를 주입한 부모와 현재 Windows10/11 파일 연결을 제공한다.
namespace { // 도달한 효과만 요구하며 다른 file reader의 정책을 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("GIF 실제 파일의 선택된 장치 미연결");return function; } // 미선택 장치를 선행 요구하지 않는다.
struct GifFileReader { // 완료 Open의 원문 소멸과 미완성 Open의 현대 OS 자원 경계를 소유한다.
    o::RawBaseFileState state{}; // 현재 File/키 토큰의 호스트 폭을 유지한다.
    o::RawBaseFileIOHooks io; // 순수 FILE/key 경계이며 VFS 검색은 Open에 남긴다.
    std::vector<std::uint8_t> key; // Decode가 끝날 때까지 살아 있는 NUL 포함 키다.
    GifFileReader() { // 현재 FILE과 실제 키 할당만 연결한다.
        o::ConstructBaseFile(state);
        io.open=[](auto name,auto mode) { const auto path=platform::PathFromUtf8(name); // ANSI 원문 이름은 기존 Unicode OS 경계를 통해 연다.
#ifdef _WIN32
            const std::wstring text(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),text.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문은 반환값을 읽지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // signed32 길이 결과를 그대로 부모에 전달한다.
        io.read=[](auto file,auto bytes,auto size,auto count) { static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 원문처럼 짧은 읽기의 남은 입력0을 유지하며 재시도/성공 합성을 하지 않는다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // Decode 뒤에만 정상 닫기를 수행한다.
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; }; // 같은 동기 File이 키를 소유한다.
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("GIF 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("GIF 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("GIF 키 반납 토큰");key.clear(); }; // 원문 Close 다음 키 해제를 유지한다.
        io.assertion=[](auto line) { throw std::logic_error("GIF BaseFile 원문 진단 "+std::to_string(line)); }; // 진단을 빈 정상 이미지로 바꾸지 않는다.
    }
    ~GifFileReader() { o::DestructBaseFile(state,io); } // 원문이 이미 반납한 자원은0이다. Open 도중 예외의 잔여 OS 자원만 현대 소유 경계에서 추가 반납한다.
};
}
std::uint32_t GifFileDimensions(std::span<const std::uint8_t> bytes) { // 실제407171은 descriptor marker/서명을 확인하지 않는다.
    if (bytes.size()<13) throw std::out_of_range("GIF 치수 헤더의 실제 물리 범위");
    const std::size_t descriptor=13+((bytes[10]&0x80) ? (3U<<((bytes[10]&7)+1)) : 0U); // 현재 global palette 길이 다음의 같은 첫 descriptor다.
    if (descriptor>bytes.size() || bytes.size()-descriptor<9) throw std::out_of_range("GIF 치수 descriptor의 실제 물리 범위");
    const auto width=std::uint32_t(bytes[descriptor+5])|(std::uint32_t(bytes[descriptor+6])<<8),height=std::uint32_t(bytes[descriptor+7])|(std::uint32_t(bytes[descriptor+8])<<8);return (width<<16)|height;
}
std::uintptr_t ReadGifFile(o::RawBaseFileState& state,std::string_view name,std::int32_t& width,std::int32_t& height,const o::RawBaseFileIOHooks& io,const GifFileReadHooks& hooks,std::span<std::uint8_t> scratch) { // Open 완료 전에 원문 outer 소멸자가 등록되지 않는다.
    state.key=0;state.keyLength=0;Need(hooks.open)(state,name,0,1);
    std::uintptr_t bitmap{}; // 정상 File 소멸 뒤에만 caller로 반환한다.
    try {
        if (!state.file) Need(hooks.assertion)(0xfb);
        const auto length=static_cast<std::uint32_t>(o::BaseFileLength(state,io));const auto input=Need(hooks.allocateInput)(length);
        if (input.bytes.size()<length || (length && !input.token)) throw std::out_of_range("GIF 입력0 채우기의 실제 할당 범위"); // 원문의 NULL/nonzero 쓰기를 성공으로 대체하지 않는다.
        auto bytes=input.bytes.first(length);std::fill(bytes.begin(),bytes.end(),std::uint8_t{});if (!input.token) Need(hooks.assertion)(0x102);
        o::ReadBaseFile(state,bytes,-1,io,scratch);const auto packed=GifFileDimensions(bytes);const auto currentWidth=packed>>16,currentHeight=packed&0xffffU;
        width=static_cast<std::int32_t>(currentWidth);height=static_cast<std::int32_t>(currentHeight); // 두 출력은 bitmap 할당/Decode가 실패하더라도 이미 기록됐다.
        bitmap=Need(hooks.allocateBitmap)(currentWidth*currentHeight);if (!bitmap) Need(hooks.assertion)(0x10c);
        Need(hooks.decode)(bitmap,bytes,currentWidth,currentHeight);Need(hooks.releaseInput)(input.token);
    } catch (...) { o::DestructBaseFile(state,io);throw; } // 실제SEH는 완료된 File/키만 정리한다. 주입한 입력/bitmap의 예외 반납을 합성하지 않는다.
    o::DestructBaseFile(state,io);return bitmap; // 정상 소멸 예외에서 소멸자를 다시 호출하지 않는다.
}
std::uintptr_t ReadBackgroundGifFile(const o::BaseFileSystem& files,std::string_view name,std::int32_t& width,std::int32_t& height,const GifBitmapHooks& bitmap) { // Probe는 기존 caller가 하고 이 부모는 필수 Open을 실행한다.
    GifFileReader reader;std::vector<std::uint8_t> input;std::array<std::uint8_t,4096> scratch{};GifFileReadHooks hooks; // host 암호작업 미작성tail의0은 원문 미정 stack과 같다는 주장이 아니다.
    hooks.open=[&](auto& state,auto path,auto mode,auto create) { files.Open(state,path,mode,create,reader.io); };
    hooks.allocateInput=[&](auto size) { input.resize(size);return GifFileAllocation{1,input}; }; // 요청0에도 할당 존재 토큰을 길이와 구별한다.
    hooks.releaseInput=[&](auto token) { if (token!=1) throw std::out_of_range("GIF 입력 반납 토큰");std::vector<std::uint8_t>{}.swap(input); }; // 정상 Decode 직후 입력을 반납한다.
    hooks.allocateBitmap=bitmap.allocate;hooks.decode=bitmap.decode;hooks.assertion=[](auto line) { throw std::logic_error("GIF 파일 부모의 원문 진단 "+std::to_string(line)); };
    return ReadGifFile(reader.state,name,width,height,reader.io,hooks,scratch); // host 입력 벡터는 예외에서 현대 RAII로 반납하며 원문 malloc 누수의 재현으로 주장하지 않는다.
}
IndexedImage ReadBackgroundGifFileImage(const o::BaseFileSystem& files,std::string_view name,std::int32_t& width,std::int32_t& height) { // Background의 기존 값형 decoder와 성공한 픽셀 소유권을 유지한다.
    IndexedImage image;GifBitmapHooks hooks;
    hooks.allocate=[&](auto size)->std::uintptr_t { if (size>16U*1024U*1024U) throw std::out_of_range("GIF 기존 decoder의 실제 픽셀 한도");image.width=static_cast<std::size_t>(width);image.height=static_cast<std::size_t>(height);image.indices.resize(size);return 1; }; // 원문 할당 시점은 Decode 전이며 현재 host의 픽셀 한도를 유지한다.
    hooks.decode=[&](auto token,auto bytes,auto currentWidth,auto currentHeight) { if (token!=1) throw std::out_of_range("GIF bitmap 실제 할당 토큰");auto decoded=DecodeBackgroundGif(bytes);if (decoded.width!=currentWidth || decoded.height!=currentHeight || decoded.indices.size()!=image.indices.size()) throw std::logic_error("GIF 현재 descriptor/decoder 치수 불일치");image=std::move(decoded); }; // 기존 LZW/색 번호/투명도 결과를 그대로 인계한다.
    try { static_cast<void>(ReadBackgroundGifFile(files,name,width,height,hooks)); }
    catch (const std::out_of_range& failure) { throw std::runtime_error(failure.what()); } // 기존 Background 값형 decoder의 runtime_error 계약을 유지하며 치수/선행효과는 되돌리지 않는다. Kencloud의 raw 물리 범위 예외는 이 어댑터를 거치지 않는다.
    return image;
}
}
