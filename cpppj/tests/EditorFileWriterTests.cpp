// 실제Unicode작업본에서Template전용r+b저장과일반잘라쓰기의경계를검사한다.보호게임파일은수정하지않는다.
#include "TestSupport.h"
#include "client/EditorFiles.h"
#include "client/CampaignTemplateStorage.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // Win32최소/최대매크로가표준함수이름과충돌하지않게한다.
#endif
#include <windows.h>
#endif

namespace { // 모든실제파일은검사가만드는새고유디렉터리에만둔다.
using namespace netstorm::client; // 실제EditorFiles와저장소를그대로사용한다.
using namespace netstorm::o; // 원본이미지/파일읽기의공통계약이다.
using namespace netstorm::platform; // UTF-8문자열과Windowswide경로의실제변환이다.
// 이전실패자료를삭제하거나덮어쓰지않고실제Unicode작업루트를한개만든다.
std::filesystem::path WriterRoot(std::string_view label) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();return std::filesystem::temp_directory_path()/std::filesystem::path(u8"Netstorm 저장 😀")/(std::string(label)+'-'+std::to_string(stamp));
}
// 보호대상도검사소유입력파일이며게임originals/originalCD에는쓰지않는다.
void WriterSeed(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!file) throw std::runtime_error("Template쓰기검사의입력파일실패");
}
// 실제거부를관측하고앞선파일내용이나이름을제품에맞춰바꾸지않는다.
template<class Function> bool WriterRejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
// 지정raw DWORD만원본little endian순서로쓴다.
void WriterField(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 현재네BYTE밖의manager/current/이름은보존한다.
    for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 실제Template헤더의manager토큰을호스트주소로해석하지않고네BYTE로읽는다.
std::uint32_t WriterToken(std::span<const std::uint8_t> bytes) {
    std::uint32_t value{};
    // 같은manager포인터DWORD의낮은BYTE부터조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[index])<<(index*8);return value;
}
// 같은manager+c의NUL이름만바꾸어파일저장부모가편의이름사본을쓰는지구별한다.
void WriterManagerName(std::span<std::uint8_t> manager,std::string_view name) {
    if (name.size()>=256) throw std::length_error("Template쓰기검사의manager이름범위");std::copy(name.begin(),name.end(),manager.begin()+12);manager[12+name.size()]=0;
}
}

TEST_CASE(EditorFileWriter_ExistingTailMissingCreationAndGeneralTruncation) { //기존이미지보다짧게써도이전tail을보존하고부재파일만새로생성한다.
    const auto root=WriterRoot("tail"),source=root/std::filesystem::path(u8"보호 자산"),work=root/std::filesystem::path(u8"편집 작업본");EditorFiles files(work,{source});std::vector<std::uint8_t> initial(37,0x5a);const std::array<std::uint8_t,3> prefix{1,0,255};
    const auto name=PathToUtf8(std::filesystem::path(u8"기존 맵 😀"));const auto path=files.Prepare(name,initial);const auto absolute=PathToUtf8(path);const auto writer=files.Writer();writer.permissions(absolute,0x180);writer.write(absolute,prefix);
    auto expected=initial;std::copy(prefix.begin(),prefix.end(),expected.begin());CHECK(ReadFileBytes(path)==expected);writer.write(absolute,{});CHECK(ReadFileBytes(path)==expected); //빈한element쓰기도기존파일의끝을자르지않는다.
    std::vector<std::uint8_t> longer(53,0x73);writer.write(absolute,longer);CHECK(ReadFileBytes(path)==longer);
    const auto created=files.ResolvePath(PathToUtf8(std::filesystem::path(u8"d/새 맵 😀.fort")));CHECK(!std::filesystem::exists(created));writer.write(PathToUtf8(created),prefix);CHECK(ReadFileBytes(created)==std::vector<std::uint8_t>(prefix.begin(),prefix.end()));
    const auto empty=files.ResolvePath("d/empty.fort");writer.write(PathToUtf8(empty),{});CHECK(std::filesystem::exists(empty) && ReadFileBytes(empty).empty());
    const std::array<std::uint8_t,1> shortened{9};files.Write(absolute,shortened);CHECK(ReadFileBytes(path)==std::vector<std::uint8_t>{9}); //일반복사/준비의기존잘라쓰기계약은유지한다.
}

TEST_CASE(EditorFileWriter_ActualSaveUsesCurrentManagerNameSignatureAndTail) { //실제직렬화/두이름조회/권한/Writer가같은manager의현재서명과이름을소비한다.
    const auto root=WriterRoot("manager"),source=root/std::filesystem::path(u8"보호 원본"),work=root/std::filesystem::path(u8"쓰기 작업본");EditorFiles files(work,{source});const std::vector<std::uint8_t> sentinel(256,0xa7);const auto first=files.Prepare(PathToUtf8(std::filesystem::path(u8"첫 맵 😀")),sentinel),second=files.ResolvePath(PathToUtf8(std::filesystem::path(u8"d/다음 맵 😀.fort")));
    FortTemplate fort;fort.sections.resize(35);fort.sections[1]={7,8,9};fort.flag=0;CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"stale-name.fort");auto manager=storage.ManagerBytes(WriterToken(storage.Headers().subspan(20,4)));WriterManagerName(manager,PathToUtf8(first));WriterField(manager,0x110,1);
    std::uint32_t readOnly{},owner=1;std::array<std::uint8_t,9> hidden{};FortImageHooks image;image.typeCount=[] { return 188U; };image.assertion=[](auto) { throw std::logic_error("Template쓰기검사의예상밖이미지진단"); };
    storage.SaveFile(1,{readOnly,owner,hidden},image,files.Writer());const auto saved=ReadFileBytes(first);CHECK(saved.size()==sentinel.size() && saved[0]=='F' && saved[1]==0xfe && saved[6]==7 && saved[7]==8 && saved[8]==9);
    CHECK(std::all_of(saved.begin()+75,saved.end(),[](auto byte) { return byte==0xa7; }) && storage.CurrentSection(1)=="Terr19" && storage.Name(1)==PathToUtf8(first)); //72BYTE헤더/길이표와3BYTE State뒤기존tail전체를보존한다.
    WriterManagerName(manager,PathToUtf8(second));WriterField(manager,0x110,0);storage.Template(1).flag=0xfe;storage.SaveFile(1,{readOnly,owner,hidden},image,files.Writer());const auto current=ReadFileBytes(second);
    CHECK(current.size()==75 && current[0]=='F' && !current[1] && ReadFileBytes(first)==saved && storage.Name(1)==PathToUtf8(second)); //편의flag/이름사본과반대인실제manager필드만쓴다.
}

TEST_CASE(EditorFileWriter_ProtectedAndAbsentParentFailBeforeMutation) { //보호경로/작업본밖과없는부모의실제열기실패를성공으로바꾸지않는다.
    const auto root=WriterRoot("guards"),source=root/std::filesystem::path(u8"보호 원본 😀"),work=root/std::filesystem::path(u8"작업본 😀");const std::array<std::uint8_t,4> original{3,1,4,1};const auto protectedFile=source/"d/kept.fort";WriterSeed(protectedFile,original);EditorFiles files(work,{source});files.EnsureDirectory("d");const auto writer=files.Writer();
    CHECK(WriterRejects([&] { writer.permissions(PathToUtf8(protectedFile),0x180); }) && WriterRejects([&] { writer.write(PathToUtf8(protectedFile),{}); }));CHECK(ReadFileBytes(protectedFile)==std::vector<std::uint8_t>(original.begin(),original.end()));
    const auto outside=root/"outside.fort";CHECK(WriterRejects([&] { writer.write(PathToUtf8(outside),original); }) && !std::filesystem::exists(outside));
    const auto missing=files.ResolvePath("absent/child.fort");CHECK(WriterRejects([&] { writer.write(PathToUtf8(missing),original); }) && !std::filesystem::exists(missing.parent_path()));
}

TEST_CASE(EditorFileWriter_ExclusiveOpenFailurePreservesExistingBytes) { //실제쓰기공유거부는r+b와생성fallback모두실패하며기존내용을자르지않는다.
#ifdef _WIN32
    const auto root=WriterRoot("sharing"),source=root/std::filesystem::path(u8"보호"),work=root/std::filesystem::path(u8"공유 작업본 😀");EditorFiles files(work,{source});const std::array<std::uint8_t,5> original{2,7,1,8,2};const auto path=files.Prepare(PathToUtf8(std::filesystem::path(u8"잠긴 맵 😀")),original);
    struct LockedFile { //예외/검사종료에서도이검사가연핸들한개만반납한다.
        HANDLE handle{INVALID_HANDLE_VALUE}; //실제쓰기공유를허용하지않는읽기핸들이다.
        ~LockedFile() { if (handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); } //다른프로세스/사용자핸들은건드리지않는다.
    } lock{CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)};
    CHECK(lock.handle!=INVALID_HANDLE_VALUE);if (lock.handle==INVALID_HANDLE_VALUE) return;const auto writer=files.Writer();CHECK(WriterRejects([&] { writer.write(PathToUtf8(path),{}); }));CHECK(ReadFileBytes(path)==std::vector<std::uint8_t>(original.begin(),original.end()));
#endif
}
