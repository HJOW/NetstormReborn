// Windows10/11의 한글 경로는 원본 ANSI와 달리 wide CRT로 보존하며 파일/바이트 정책은 원문을 따른다.
#include "client/IndexedBitmapFile.h"
#include "o/RawBaseFileIO.h"
#include "platform/Utf8Path.h"
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 실제FILE은 한 동기 출력 호출 동안만 소유한다.
namespace { // 원문 포인터와 별개인 지역 입력 토큰/CRT 수명을 관리한다.
constexpr std::uint32_t kBitmapPixels=1,kBitmapPalette=2; // 두 현재 span을 구별하는 비소유 토큰이다.
struct File { // 현재FILE/키는 기존 공통 BaseFile 본체로 정상/예외 모두 반납한다.
    o::RawBaseFileState state{}; // 실제 FILE 주소 토큰이며24BYTE 객체 ABI로 노출하지 않는다.
    o::RawBaseFileIOHooks hooks; // 열기/쓰기/닫기만 쓰는 실제 CRT 장치다.
    File() { // 같은 BaseFile 모드와 단일 element 쓰기를 제공한다.
        o::ConstructBaseFile(state);
        hooks.open=[](auto name,auto mode)->std::uintptr_t {
            if (mode!="r+b" && mode!="w+b") throw std::logic_error("8비트 BMP 파일의 원문 쓰기 모드 범위");const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str())); // 원본 A 경로 대신 Windows10/11의UTF-8/W 경계다.
#else
            const std::string narrow(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),narrow.c_str()));
#endif
        };
        hooks.write=[](auto token,auto bytes,auto size,auto count) { static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(token))); }; // Native는 부분 쓰기 반환으로 재시도/새 실패를 만들지 않는다.
        hooks.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); }; // fclose 반환도 원문처럼 사용하지 않는다.
    }
    ~File() { o::DestructBaseFile(state,hooks); } // 본체가 이미 반납했으면FILE0이므로 이중 fclose를 하지 않는다.
};
}
void WriteIndexedBitmapFile(const std::filesystem::path& path,std::span<const std::uint8_t> pixels,IndexedBitmapArea area,
    std::span<const std::uint8_t> logicalRgbPalette,IndexedBitmapScratch& scratch) {
    File file;IndexedBitmapHooks hooks;
    hooks.construct=[&](auto name,auto mode,auto create) { if (!o::OpenBaseFileDisk(file.state,name,mode,create,file.hooks)) throw std::runtime_error("8비트 BMP의 실제 파일 열기 실패"); }; // 장치 경로 쓰기 실패에서 읽기용TAFF/보조 경로로 우회하지 않는다.
    hooks.isOpen=[&] { return file.state.file ? 1U:0U; };hooks.write=[&](auto bytes,auto offset) { o::WriteBaseFile(file.state,bytes,offset,file.hooks); };hooks.destroy=[&] { o::DestructBaseFile(file.state,file.hooks); };
    hooks.memory=[&](auto token,auto offset,auto length) { const auto bytes=token==kBitmapPixels ? pixels:token==kBitmapPalette ? logicalRgbPalette:throw std::out_of_range("8비트 BMP의 현재 입력 토큰");if (offset>bytes.size() || length>bytes.size()-offset) throw std::out_of_range("8비트 BMP의 실제 입력 span 범위");return bytes.subspan(offset,length); };
    const IndexedBitmapInput input{kBitmapPixels,area.x,area.y,area.width,area.height,area.pitch,kBitmapPalette};WriteRawIndexedBitmap(platform::PathToUtf8(path),input,scratch,hooks);
}
}
