// 원문의 직접 읽기전용 매핑과 현대 Win32 자원 반납을 구별한다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "client/NativeShapeFile.h"
#include "client/ReadOnlyFileMap.h"
#include "client/TechnicalHelp.h"
#include "o/BaseFile.h"
#include "o/FileSpecQuery.h"
#include "o/RawBaseFileReadPolicy.h"
#include "o/RawFileName.h"
#include "o/OriginalText.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <optional>
#include <stdexcept>

namespace netstorm::client { // 실제 자산이 같은 view의 마지막 소유자다.
namespace { // 이름 선택 장치와 W API/핸들을 동기 호출 안에서 연결한다.
struct ShapeMapping { // 원문이 누락한 파일/매핑핸들 정리를 현대 소유권으로 수행한다.
    HANDLE file{INVALID_HANDLE_VALUE}; // CreateFileW의 현재 핸들이며 실제호스트 INVALID와 비교한다.
    HANDLE mapping{}; // CreateFileMappingW의 현재 섹션 핸들이다.
    const void* view{}; // MapViewOfFile의 같은 읽기전용 주소다.
    ~ShapeMapping() { if (view) UnmapViewOfFile(view);if (mapping) CloseHandle(mapping);if (file && file!=INVALID_HANDLE_VALUE) CloseHandle(file); } // 마지막 ShapeDatabase 복사까지 살아 있다가 view→mapping→file 순으로 반납한다.
};
std::filesystem::path DiskPath(std::string_view value,const std::filesystem::path& primary) { // 원본 chdir 대신 같은 게임root의 상대/같은드라이브 상대만 보정한다.
    auto path=platform::PathFromUtf8(value);const auto root=std::filesystem::absolute(primary).lexically_normal();
    if (!path.is_absolute()) {
        if (!path.has_root_name()) path=root/path;
        else if (!path.has_root_directory() && o::AsciiLower(platform::PathToUtf8(path.root_name()))==o::AsciiLower(platform::PathToUtf8(root.root_name()))) path=root/path.relative_path();
    }
    return path;
}
std::string RootText(const std::filesystem::path& root) { auto text=platform::PathToUtf8(std::filesystem::absolute(root).lexically_normal());std::replace(text.begin(),text.end(),'/','\\');return text; } // 원문 현재root의 UTF-8/W 확장이다.
}
ShapeDatabase LoadNativeShapeDatabase(const o::BaseFileSystem& files,const std::filesystem::path& primary,std::string_view literal) { // 원문 빈root/d/literal을 같은 고리에 먼저 작성한다.
    const auto names=files.FileNameOwner();const std::array<o::FileNamePart,3> parts{{{-2,""},{-2,"d"},{-3,literal}}};const std::string query(names->Format(parts));std::optional<std::string> selected;
    { // 원문41ad10의 FileSpec은 직접map 호출 전에 정상/예외 반납한다.
    o::FileSpecQuery spec(query,o::FileSpecDirectoryHooks(primary));ExternalFileHooks lookup;
    lookup.device=[&](auto) { return !names->Device(spec.Text()).empty(); }; // 원문41ad10의 같은 FileSpec/Device칸이다.
    lookup.path=[&](bool secondary,auto name) { return o::JoinBaseFileRoot(RootText(secondary ? files.SecondaryDirectory():primary),name,*names); };
    lookup.readable=[&](auto name) { const auto path=DiskPath(name,primary);auto* file=::_wfopen(path.c_str(),L"rb");if (!file) return false;static_cast<void>(std::fclose(file));return true; }; // 현재 rb 열기/닫기이며 payload/길이는 읽지 않는다.
    lookup.archive=[&](bool secondary,auto name) { return files.ContainsArchiveOnly(name,secondary); };
    selected=ResolveExternalFilePath(query,nullptr,lookup);
    }
    if (!selected) throw std::logic_error("원문 Rifttype.cpp168: filename"); // 원문선택실패를빈SHP/추출성공으로바꾸지않는다.
    auto owner=std::make_shared<ShapeMapping>();ReadOnlyFileMapHooks hooks;
    hooks.createFile=[&](auto name,auto access,auto share,auto security,auto creation,auto flags,auto previous) { // 원문security12 논리필드를 실제64비트OS 구조체로 옮긴다.
        SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),reinterpret_cast<void*>(security.descriptor),security.inherit ? TRUE:FALSE};const auto path=DiskPath(name,primary);
        owner->file=CreateFileW(path.c_str(),access,share,&attributes,creation,flags,reinterpret_cast<HANDLE>(previous));return reinterpret_cast<std::uintptr_t>(owner->file);
    };
    hooks.createMapping=[&](auto file,auto security,auto protection,auto high,auto low,auto name) { owner->mapping=CreateFileMappingW(reinterpret_cast<HANDLE>(file),reinterpret_cast<SECURITY_ATTRIBUTES*>(security),protection,high,low,reinterpret_cast<LPCWSTR>(name));return reinterpret_cast<std::uintptr_t>(owner->mapping); }; // 이름/보안NULL을 유지한다.
    hooks.mapView=[&](auto mapping,auto access,auto high,auto low,auto count) { owner->view=MapViewOfFile(reinterpret_cast<HANDLE>(mapping),access,high,low,count);return reinterpret_cast<std::uintptr_t>(owner->view); }; // 원문전체view0 길이다.
    hooks.assertion=[](auto condition,auto source,auto line) { throw std::logic_error("원문 셰이프 매핑 진단 "+std::string(source)+":"+std::to_string(line)+": "+std::string(condition)); };
    static_cast<void>(MapReadOnlyFile(*selected,reinterpret_cast<std::uintptr_t>(INVALID_HANDLE_VALUE),hooks));LARGE_INTEGER length{};
    if (!GetFileSizeEx(owner->file,&length) || length.QuadPart<=0 || static_cast<std::uint64_t>(length.QuadPart)>std::numeric_limits<std::size_t>::max()) throw std::runtime_error("셰이프 매핑의 호스트 실제 길이 조회 실패"); // 현대 파서의 실제 span 범위만 얻으며 원문map 호출 순서는 바꾸지 않는다.
    const std::span<const std::uint8_t> bytes{static_cast<const std::uint8_t*>(owner->view),static_cast<std::size_t>(length.QuadPart)}; // 인자 평가 순서에 따라 owner 이동 뒤 주소를 읽지 않게 먼저 확정한다.
    return ShapeDatabase(bytes,std::move(owner));
}
}
