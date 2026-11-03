// 고정 snapshot 대신 같은 VFS metadata/current FILE·ACP 디스크 이름을 사용한다.
#include "client/NativeFontCacheFile.h"
#include "client/FontCacheFile.h"
#include "client/FontFaceEncoding.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include "o/RawColorMapCache.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <system_error>

namespace netstorm::client { // 같은 File/키와 글꼴 바이트를 동기 호출 동안 소유한다.
namespace { // OS 장치와 캐시 파일의 실제 물리 범위를 한정한다.
constexpr std::array<std::uint8_t,16> kSignature{'B','i','t','m','a','p','F','o','n','t','D','a','t','a',26,0}; // 정상 writer 서명이다.
constexpr std::array<std::uint8_t,4> kCharset{0,1,0,0}; // 256의 원문 DWORD다.
constexpr std::uint32_t kMaxGlyphBytes=16U*1024U*1024U; // 손상 cache의 수GB 할당을 막는 호스트 물리 상한이며 원문 논리 조건으로 합산하지 않는다.
constexpr std::size_t kMaxCacheGlyphBytes=64U*1024U*1024U; // 여러 손상 글리프의 누적 할당도 호스트 메모리 범위 안으로 제한한다.
struct ProtectedCacheWrite final : std::exception {}; // 보호원본에서 새 writer가 fallback/덮어쓰기를 계속하지 않게 하는 현대 호스트 경계다.
std::uint32_t Word(std::span<const std::uint8_t> bytes) { // 작은 DWORD 필드를 정렬 없이 해석한다.
    if (bytes.size()<4) throw std::runtime_error("글꼴 캐시 DWORD 물리 누락");std::uint32_t value{};for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[index])<<(index*8);return value; // 원문 바이트 순서를 유지한다.
}
struct FontCacheStorage { // 실제 순차 reader의 owner와 설치 글리프를 소유한다.
    std::array<std::uint8_t,kFontCacheOwnerBytes> owner{}; // 원문 머리/표의 같은 직접 쓰기 물리다.
    std::array<std::vector<std::uint8_t>,256> glyphs; // 각 슬롯의 현재 할당이다. 호스트는 미정 heap 대신0시드한다.
    std::vector<std::uint8_t> Serialize() const { // 검증된 읽기 또는 기존 BitmapFont의 같은 버퍼를 순서만 바꾸어 반환한다.
        std::vector<std::uint8_t> result(kSignature.begin(),kSignature.end());result.insert(result.end(),owner.begin(),owner.begin()+16);result.insert(result.end(),kCharset.begin(),kCharset.end());result.insert(result.end(),owner.begin()+16,owner.end());
        for (const auto& glyph:glyphs) result.insert(result.end(),glyph.begin(),glyph.end());return result; // 빈 글리프도 표에 기록한0길이를 그대로 유지한다.
    }
    void Assign(std::span<const std::uint8_t> bytes) { // writer 입력의 물리 범위만 검사하며 글리프를 다시 그리거나 재인코딩하지 않는다.
        if (bytes.size()<4132) throw std::runtime_error("글꼴 writer 입력 머리 물리 누락");std::copy_n(bytes.begin()+16,16,owner.begin());std::copy_n(bytes.begin()+36,4096,owner.begin()+16);std::size_t offset=4132;
        for (std::size_t code=0;code<256;++code) { // 각 크기표와 같은 원래 글리프 블록을 복사한다.
            const auto length=Word(std::span(owner).subspan(0xc10+code*4,4));if (length>bytes.size()-offset) throw std::runtime_error("글꼴 writer 입력 글리프 물리 누락");glyphs[code].assign(bytes.begin()+offset,bytes.begin()+offset+length);offset+=length;
        }
        if (offset!=bytes.size()) throw std::runtime_error("글꼴 writer 입력 뒤의 미분류 BYTE");
    }
};
struct FontCacheDevice { // 완료 File 정리와 미완성 Open의 현대 자원 회수를 구별한다.
    o::RawBaseFileState state{}; // 현재 실제 FILE/키 토큰이며32비트로 절단하지 않는다.
    o::RawBaseFileIOHooks io; // Unicode CRT와 키 소유 장치다.
    std::vector<std::uint8_t> key; // 같은 File을 닫은 뒤 해제한다.
    FontCacheDevice() { // 창/게임/PCM 장치를 열지 않는다.
        o::ConstructBaseFile(state);
        io.open=[](auto name,auto mode) { // 변환을 끝낸 UTF-8 디스크 경로만 받으며 raw TAFF 키는 여기서 바꾸지 않는다.
            const auto path=platform::PathFromUtf8(name);if (mode!="rb" && IsProtectedFontCachePath(path)) throw ProtectedCacheWrite{};
#ifdef _WIN32
            const std::wstring text(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),text.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문 CRT 반환을 검사하지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // 암호 위치는 signed32 원문 필드로 계산한다.
        io.read=[](auto file,auto bytes,auto size,auto count) { if (size && count) static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // NULL data의0BYTE 호스트 CRT 호출만 피하며 부분 읽기는 그대로 남긴다.
        io.write=[](auto file,auto bytes,auto size,auto count) { if (size && count) static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 원문처럼 부분 쓰기 반환은 사용하지 않는다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 명시Close 뒤 원문 필드를0으로 만든다.
        io.allocateKey=[this](auto length)->std::uintptr_t { key.resize(length);return 2; }; // 같은 동기 File의 키다.
        io.mutableKey=[this](auto token,auto length) { if (token!=2 || length>key.size()) throw std::out_of_range("글꼴 키 쓰기 물리 범위");return std::span(key).first(length); };
        io.key=[this](auto token,auto length) { if (token!=2 || length>key.size()) throw std::out_of_range("글꼴 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(length); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("글꼴 키 반납 토큰");key.clear(); };
        io.assertion=[](auto line) { throw std::logic_error("글꼴 BaseFile 원문 진단 "+std::to_string(line)); };
    }
    ~FontCacheDevice() { o::DestructBaseFile(state,io); } // 미완성Open의 잔여 OS 자원 추가 회수는 현대 경계다.
};
}
bool IsProtectedFontCachePath(const std::filesystem::path& path) { // 실제 경로/junction을 확인한 뒤 보호 트리의 하위만 차단한다.
    std::error_code error;const auto actual=std::filesystem::weakly_canonical(std::filesystem::absolute(path),error);if (error) return true;
    const auto source=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    for (const auto directory:{"originals","originalCD"}) { // 사용자 지침의 두 보호 원본 트리를 각각 검사한다.
        const auto root=std::filesystem::weakly_canonical(source/directory,error);if (error) return true;auto current=actual.begin();bool inside=true;
        for (const auto& component:root) { // Windows 경로의 ASCII 대소문자 차이로 보호 검사를 우회하지 않는다.
            if (current==actual.end() || o::AsciiLower(platform::PathToUtf8(*current))!=o::AsciiLower(platform::PathToUtf8(component))) { inside=false;break; }++current;
        }
        if (inside) return true;
    }
    return false;
}
std::optional<std::vector<std::uint8_t>> LoadNativeFontCache(const o::BaseFileSystem& files,std::string_view name) { // 같은 현재 backing을 읽으며 등록 snapshot은 변경하지 않는다.
    FontCacheDevice file;FontCacheStorage storage;FontCacheFileHooks hooks;std::array<std::uint8_t,4096> scratch{};std::array<std::uint8_t,16> signature{};std::array<std::uint8_t,4> charset{};std::size_t allocatedGlyphBytes{}; // 같은 읽기 수명의 누적 실제 할당 요청이다.
    const auto convert=[](auto raw) { return FontCacheDiskName(raw); }; // root는이미UTF-8이며 명시 raw query만 현재ACP로 변환한다.
    hooks.exists=[&](auto query) { return files.ProbeWithDiskName(query,convert)!=0; };hooks.open=[&](auto& state,auto query,auto mode,auto create) { files.OpenWithDiskName(state,query,mode,create,file.io,convert); };
    hooks.allocate=[&](auto code,auto length)->std::span<std::uint8_t> { if (length>kMaxGlyphBytes || length>kMaxCacheGlyphBytes-allocatedGlyphBytes) throw std::runtime_error("손상 글꼴 캐시의 호스트 글리프 할당 상한");storage.glyphs.at(code).resize(length);allocatedGlyphBytes+=length;return storage.glyphs.at(code); };
    hooks.assertion=[](auto line) { throw std::runtime_error("글꼴 캐시 원문 진단 "+std::to_string(line)); };
    if (!ReadFontCacheFile(file.state,name,storage.owner,file.io,hooks,scratch,signature,charset)) return {};return storage.Serialize();
}
bool SaveNativeFontCache(const o::BaseFileSystem& files,std::string_view name,std::span<const std::uint8_t> bytes) { // 캐시 쓰기 실패는 이미 만든 글꼴을 제거하거나 시작을 막지 않는다.
    FontCacheDevice file;FontCacheStorage storage;storage.Assign(bytes);FontCacheFileHooks hooks;
    hooks.open=[&](auto& state,auto query,auto mode,auto create) { files.OpenWithDiskName(state,query,mode,create,file.io,[](auto raw) { return FontCacheDiskName(raw); });if (state.flags&1U) throw ProtectedCacheWrite{}; }; // 읽기전용TAFF fallback에는 쓰지 않으며 현재File/키는 현대RAII가 반납한다.
    hooks.glyph=[&](auto code,auto length)->std::span<const std::uint8_t> { const auto& glyph=storage.glyphs.at(code);if (glyph.size()!=length) throw std::out_of_range("글꼴 writer의 같은 크기표/블록 불일치");return glyph; };
    try { return WriteFontCacheFile(file.state,name,storage.owner,file.io,hooks); }
    catch (const ProtectedCacheWrite&) { return false; } // 개발 원본/설치CD 보호 지침을 writer보다 우선한다.
    catch (const o::ColorCacheUnableToOpen&) { return false; } // Windows 읽기전용 설치에서도 기존 메모리 생성 성공을 유지한다. 전체 사용자 경로 이관은5차다.
}
}
