// 실제 VFS/ACP 이름·현재 파일·FontStore 생성 캐시의 재사용을 검사한다.
#include "TestSupport.h"
#include "FileProbeTestSupport.h"
#include "client/BitmapFont.h"
#include "client/FontFaceEncoding.h"
#include "client/NativeFontCacheFile.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::fileprobe;
namespace platform=netstorm::platform; // 검사 파일의 UTF-8 경로 함수는 제품의 같은 OS 경계를 사용한다.
namespace { // 기존 보호 자료와 독립적인 정상 CHFNT 입력만 작성한다.
std::vector<std::uint8_t> CacheInput(std::uint32_t width) { // 그림 없는256글리프와 명시 A폭의 유효 입력이다.
    std::vector<std::uint8_t> bytes(4132);constexpr std::string_view signature{"BitmapFontData\x1a\0",16};std::copy(signature.begin(),signature.end(),bytes.begin());
    netstorm::test::rawscene::Put(bytes,16,1);netstorm::test::rawscene::Put(bytes,20,14);netstorm::test::rawscene::Put(bytes,24,10);netstorm::test::rawscene::Put(bytes,28,4);netstorm::test::rawscene::Put(bytes,32,256);netstorm::test::rawscene::Put(bytes,36+'A'*4,width);return bytes;
}
template<class Operation> bool Failed(Operation operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 기존 host validator의 실패를 실제성공으로 합산하지 않는다.
}

TEST_CASE(FontCacheNative_CurrentArchiveAndOriginalSignatureBoundary) { // 등록 snapshot과 current FILE·원문 마지막서명BYTE/후행무시를 구별한다.
    const auto root=Work()/"font-current-789";const std::string name="d/!Input.normal.14.700.chfnt";const auto first=CacheInput(7),next=CacheInput(19);Write(root/"font.tarc",Taff(name,first));BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"font.tarc"));Write(root/"font.tarc",Taff(name,next));
    const auto loaded=LoadNativeFontCache(files,name);CHECK(loaded && *loaded==next && files.Read(name)==first);if (loaded) { BitmapFont font(*loaded);CHECK(font.Measure("A")==19 && font.CacheBytes()==next); }
    auto loose=next;loose[15]=255;loose.insert(loose.end(),{1,2,3,4,5});Write(root/platform::PathFromUtf8(name),loose);const auto normalized=LoadNativeFontCache(files,name);CHECK(normalized && *normalized==next && Failed([&] { BitmapFont legacy(loose); })); // 기존 strict값형 파서는 변하지 않는다.
    FontStore fonts(files,"Input",[](const auto&) {});CHECK(fonts.Get().Measure("A")==19 && fonts.Get().CacheBytes()==next); // 실제 FontStore도 새 현재FILE입구를 선택한다.
    auto damaged=next;netstorm::test::rawscene::Put(damaged,36+3*1024,0xffffffff);Write(root/"d/bad.chfnt",damaged);CHECK(Failed([&] { static_cast<void>(LoadNativeFontCache(files,"d/bad.chfnt")); }) && fonts.Get().Measure("A")==19); // 실제 짧은 파일의 거대 크기표를 먼저 거부하며 기존 font는 유지한다.
}

TEST_CASE(FontCacheNative_ActualStoreGeneratesPersistsAndReloadsSameGlyphBytes) { // 실제 GDI 생성물의 모든 표/블록을 저장하고 새 저장소가 재생성 없이 읽는다.
    const auto root=Work()/"font-generate-789";std::filesystem::create_directories(root/"d");BaseFileSystem files(root);FontStore first(files,"Arial",[](const auto&) {});const auto& font=first.Get();const auto bytes=font.CacheBytes();const auto path=root/"d/!Arial.normal.14.700.chfnt";
    CHECK(std::filesystem::is_regular_file(path) && ReadFileBytes(path)==bytes && font.Measure("Hello")>0 && font.Glyph('A').shape);FontStore next(files,"Arial",[](const auto&) {});const auto& reloaded=next.Get();CHECK(reloaded.CacheBytes()==bytes && reloaded.Measure("Hello")==font.Measure("Hello"));
    CHECK(font.Glyph('A').shape->Decode(0,0).indices==reloaded.Glyph('A').shape->Decode(0,0).indices); // 새 렌더링 기대값을 만들지 않고 같은 보관 블록의 재사용을 확인한다.
}

TEST_CASE(FontCacheNative_RawAcpDiskAndArchivePreserveUnicodeRoot) { // raw TAFF 키와 Unicode 디스크 query를 현재ACP에서 분리한다.
    std::string raw,utf8;constexpr std::array<std::u8string_view,5> candidates{u8"Font_글꼴",u8"Font_明朝",u8"Font_Café",u8"Font_Шрифт",u8"Font_字型"};
    for (const auto candidate:candidates) { try { const std::string text(candidate.begin(),candidate.end());auto encoded=FontFaceFromUtf8(text);if (std::any_of(encoded.begin(),encoded.end(),[](unsigned char byte) { return byte>=128; })) { utf8=text;raw=std::move(encoded);break; } }catch (const std::invalid_argument&) {} } // OS코드페이지를변경하지않고표현가능한첫입력만선택한다.
    CHECK(!raw.empty());if (raw.empty()) return;const auto root=Work()/"font-acp-789";const auto name="d/!"+raw+".normal.14.700.chfnt",disk="d/!"+utf8+".normal.14.700.chfnt";const auto archive=CacheInput(7),loose=CacheInput(29);Write(root/"font.tarc",Taff(name,archive));Write(root/platform::PathFromUtf8(disk),loose);BaseFileSystem files(root);CHECK(files.RegisterArchive(root/"font.tarc") && files.ContainsArchiveOnly(name) && (name==disk || !files.ContainsArchiveOnly(disk))); // ACP가65001이면 두 BYTE이름은 같을 수 있다.
    CHECK(files.ProbeWithDiskName(name,[](auto query) { return FontCacheDiskName(query); })!=0);FontStore fonts(files,raw,[](const auto&) {});CHECK(fonts.Get().Measure("A")==29 && fonts.Get().CacheBytes()==loose);std::filesystem::remove(root/platform::PathFromUtf8(disk));const auto read=LoadNativeFontCache(files,name);CHECK(read && *read==archive && files.Read(name)==archive); // Probe의TAFF우선과Open의disk우선차이를유지한다.
}

TEST_CASE(FontCacheNative_WriteUnavailableAndProtectedRootsKeepGeneratedData) { // 실제 보호 원본 쓰기를 시도하지 않고 경로관문/새캐시차단과 일반 실패를 구별한다.
    const auto root=Work()/"font-no-directory-789";std::filesystem::create_directories(root);BaseFileSystem files(root);const auto bytes=CacheInput(31);CHECK(!SaveNativeFontCache(files,"d/!not-created.chfnt",bytes) && !std::filesystem::exists(root/"d"));
    const auto source=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();CHECK(IsProtectedFontCachePath(source/"originals/Netstorm.exe") && IsProtectedFontCachePath(source/"originalCD/new-cache.chfnt") && !IsProtectedFontCachePath(root/"d/new-cache.chfnt"));
    const auto name="d/!font-cache-guard-789-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".chfnt";const auto path=source/"originals"/platform::PathFromUtf8(name);CHECK(!std::filesystem::exists(path));if (std::filesystem::exists(path)) return;BaseFileSystem original(source/"originals");CHECK(!SaveNativeFontCache(original,name,bytes) && !std::filesystem::exists(path)); // 기존 원본을 쓰기 대상으로 삼지 않으며 새 이름도 CRT 전에 거부한다.
    FontStore memory(files,"Arial",[](const auto&) {});CHECK(memory.Get().Measure("A")>0 && !std::filesystem::exists(root/"d")); // 캐시 폴더 부재에도 기존 메모리 글꼴은 유지한다.
}
