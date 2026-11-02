// 글꼴 이름의 ACP/UTF-8 경계만 창·GDI 생성·OS 글꼴 변경 없이 검사한다.
#include "GumpRawSupport.h"
#include "client/BitmapFont.h"
#include "client/ClientOptionsFile.h"
#include "client/FontFaceEncoding.h"
#include "client/PresentationCommandInput.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace { //검사 입력과 임시 파일만 소유하며 원본이나 실제 옵션을 쓰지 않는다.
using namespace netstorm::client;using namespace netstorm::o;using namespace netstorm::test::gumpraw; //제품 변환/설정/VFS와 동결BYTE 해독 도구만 공유한다.
//독립 UTF-8 문자 상수를 원래BYTE로 가져온다.
std::string FontUtf8(std::u8string_view text) { return {text.begin(),text.end()}; }
//NLS 참고값의16진 입력을 같은BYTE 문자열로 읽는다.
std::string FontBytes(std::string_view text) { if (text=="-") return {};const auto bytes=Bytes(std::string(text));return {bytes.begin(),bytes.end()}; }
//명시 입력 header/표에 little endian DWORD를 쓴다.
void FontPut(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes.at(offset+index)=static_cast<std::uint8_t>(value>>(8*index)); } //실제4BYTE 폭만 변경한다.
//독립 CHFNT는 그림 없는 유효256문자와 A 전진폭12345만 보유한다. GDI에 내려가지 않는다.
std::vector<std::uint8_t> FontCacheBytes() {
    std::vector<std::uint8_t> bytes(4132);constexpr std::string_view signature{"BitmapFontData\x1a\0",16}; //실제16BYTE 서명이다.
    std::copy(signature.begin(),signature.end(),bytes.begin());FontPut(bytes,16,1);FontPut(bytes,20,14);FontPut(bytes,24,10);FontPut(bytes,28,4);FontPut(bytes,32,256);FontPut(bytes,36+'A'*4,12345);return bytes;
}
//한 raw이름/내용을 가진 최소TAFF를 만들며 UTF-8 변환은 하지 않는다.
std::vector<std::uint8_t> FontArchive(std::string_view name,std::uint8_t value) {
    const auto size=static_cast<std::uint32_t>(name.size()+9);std::vector<std::uint8_t> bytes(48+size+1);constexpr std::string_view magic="TAFF v0.2\x1a"; //인덱스와 디렉터리의 입력 배치다.
    std::copy(magic.begin(),magic.end(),bytes.begin());FontPut(bytes,0x14,1);FontPut(bytes,0x1c,44);FontPut(bytes,0x20,48);FontPut(bytes,0x24,size);FontPut(bytes,0x28,48+size);FontPut(bytes,44,0);FontPut(bytes,48,0);FontPut(bytes,52,1);std::copy(name.begin(),name.end(),bytes.begin()+56);bytes.back()=value;return bytes;
}
//생성에 성공한 새 임시 루트만 반납한다. 현재 경로가 바뀌면 재귀 삭제하지 않는다.
struct FontDirectory {
    std::filesystem::path parent=std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()); //OS 임시 부모의 실제 절대 경로다.
    std::filesystem::path root; //기존 사용자 파일과 겹치지 않는 검사 소유 루트다.
    FontDirectory() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt=0;attempt<128;++attempt) { const auto candidate=parent/("netstorm-font-774-"+std::to_string(stamp)+'-'+std::to_string(attempt));if (std::filesystem::create_directory(candidate)) { root=candidate;return; } } //동시 검사는 새 접미사로 분리한다.
        throw std::runtime_error("글꼴 인코딩 검사 임시 루트 확보 실패");
    }
    FontDirectory(const FontDirectory&)=delete;FontDirectory& operator=(const FontDirectory&)=delete; //반납 소유권은 복제하지 않는다.
    ~FontDirectory() { std::error_code error;const auto actual=std::filesystem::weakly_canonical(root,error);if (!error && !root.empty() && actual.parent_path()==parent && actual.filename().native().starts_with(L"netstorm-font-774-")) std::filesystem::remove_all(actual,error); } //검증한 자체 루트만 지운다.
};
//검사의 새 입력 파일만 쓴다.
void FontWrite(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!out) throw std::runtime_error("글꼴 인코딩 검사 입력 쓰기 실패"); }
//UTF-8 상대 이름을 native wide 파일로 만든다.
std::filesystem::path FontPath(std::string_view text) { return std::filesystem::path(std::u8string(text.begin(),text.end())); }
//원문18조합의 파일을 전부 공급하여 실제 FontStore.Get/Rebuild도 GDI를 호출하지 않는다.
void FontWriteCaches(const std::filesystem::path& root,std::string_view utf8) {
    constexpr std::array<int,7> heights{14,13,0,20,48,14,12},weights{700,0,0,700,0,0,0}; //원문 슬롯의 고정 요청값이다.
    constexpr std::array<std::string_view,5> styles{"normal","italic","bold","strikeout","underline"}; //실제 FontStyle 파일 이름 순서다.
    const auto bytes=FontCacheBytes();
    for (int slot=0;slot<7;++slot) for (std::size_t style=0;style<5;++style) { if (!FontStore::Available(slot,static_cast<FontStyle>(style))) continue;const auto face=slot==1 ? std::string("Courier New"):std::string(utf8);FontWrite(root/FontPath("d/!"+face+'.'+std::string(styles[style])+'.'+std::to_string(heights[slot])+'.'+std::to_string(weights[slot])+".chfnt"),bytes); } //유효 파일만 UTF-8/W 경로로 쓴다.
}
}

//명시 코드 페이지의 실제 Windows NLS24관측으로 엄격 변환/firstNUL/표현불가를 고정한다.
TEST_CASE(FontFaceEncoding_WindowsNls24RowsKeepCodePagesAndRejectLoss) {
    const auto rows=LoadFixture(NETSTORM_FONT_FACE_ENCODING_FIXTURE).rows;CHECK(rows.size()==24);
    for (const auto& row:rows) { CHECK(row.size()==5);if (row.size()!=5) continue;const auto source=FontBytes(row[2]);std::string output;bool rejected{};try { output=row[0]=="to" ? FontFaceToUtf8(source,Number(row[1])):FontFaceFromUtf8(source,Number(row[1])); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected==(row[3]=="throw"));if (!rejected) CHECK(output==FontBytes(row[4])); } //참고값은 원문 출력이나 C++ 계산에서 만들지 않은 실제NLS 결과다.
}

//ACP949/1252 이름을 실제 Config 저장reader로 왕복하며 비1252는 기존UTF-8 BOM 정책을 따른다.
TEST_CASE(FontFaceEncoding_ActualConfigSaveReloadPreservesAcpNamesAndChanged) {
    const std::array<std::pair<std::uint32_t,std::string>,2> inputs{{{949,std::string("\xb1\xdb\xb2\xc3",4)},{1252,std::string("Caf\xe9",4)}}}; //각각 글꼴/Café의 명시 실제ACP 바이트다.
    for (const auto& [page,raw]:inputs) { //동일 Config serializer/reader를 거쳐 현재 이름을 되돌린다.
        ConfigInterface config({});config.Startup({});const auto utf8=FontFaceToUtf8(raw,page);config.Set("fontFaceName",utf8);std::vector<std::uint8_t> bytes;std::uint32_t writes{};
        SaveClientOptions(config,[&](auto name,auto data) { CHECK(name=="d\\options.cfg");++writes;bytes.assign(data.begin(),data.end()); });CHECK(writes==1 && !config.Configuration().changed && ConfigText::FromBytes(bytes).GetRaw("fontFaceName")==utf8);
        auto plain=bytes;ApplyXor(plain);CHECK((plain.size()>=8 && plain[5]==0xef && plain[6]==0xbb && plain[7]==0xbf)==(page==949));ConfigRegistry registry;Config reloaded(registry);reloaded.LoadBytes(bytes);const auto restored=reloaded.Get("fontFaceName");CHECK(restored.has_value());if (restored) CHECK(*restored==utf8 && FontFaceFromUtf8(*restored,page)==raw);
        config.Set("fontFaceName",utf8+"x");bool failed{};try { SaveClientOptions(config,[&](auto,auto) { throw std::runtime_error("명시 실제 writer 실패"); }); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && config.Configuration().changed);
    }
}

//디스크만UTF-8로 옮기며 rawTAFF 이름·네단계우선순위·같은10칸 고리 사용수를 유지한다.
TEST_CASE(FontFaceEncoding_ActualVfsDiskAdapterPreservesRawArchiveOrderAndRing) {
    FontDirectory folder;const auto base=folder.root/L"기본😀",secondary=folder.root/L"보조😀";std::filesystem::create_directories(base);std::filesystem::create_directories(secondary);const std::string raw="d/!Caf\xe9.normal.14.700.chfnt",utf8=FontFaceToUtf8(raw,1252);const auto diskBase=base/FontPath(utf8),diskSecondary=secondary/FontPath(utf8);
    FontWrite(base/"one.tarc",FontArchive(raw,'A'));FontWrite(secondary/"one.tarc",FontArchive(raw,'Z'));FontWrite(diskBase,std::array<std::uint8_t,1>{'P'});FontWrite(diskSecondary,std::array<std::uint8_t,1>{'S'});auto names=std::make_shared<RawFileName>();BaseFileSystem files(base,secondary,names);CHECK(files.RegisterArchive(base/"one.tarc") && files.RegisterArchive(secondary/"one.tarc",true));std::uint32_t converted{};
    const auto convert=[&](auto name) { CHECK(name==raw);++converted;return FontCacheDiskName(name,1252); };const auto read=[&](std::uint8_t expected,std::uint32_t steps) { const auto before=names->Index();const auto actual=files.TryReadWithDiskName(raw,convert);CHECK(actual==std::vector<std::uint8_t>{expected} && names->Index()==(before+steps)%10); }; //전체VFS가같은장기고리로방문한장치만센다.
    read('P',2);CHECK(converted==1);std::filesystem::remove(diskBase);read('A',2);CHECK(converted==2);CHECK(files.TryReadArchiveOnly(raw)==std::vector<std::uint8_t>{'A'} && !files.TryReadArchiveOnly(utf8));
    auto otherNames=std::make_shared<RawFileName>();BaseFileSystem withoutPrimary(base,secondary,otherNames);CHECK(withoutPrimary.RegisterArchive(secondary/"one.tarc",true));const auto before=otherNames->Index();CHECK(withoutPrimary.TryReadWithDiskName(raw,convert)==std::vector<std::uint8_t>{'S'} && otherNames->Index()==(before+3)%10);std::filesystem::remove(diskSecondary);const auto again=otherNames->Index();CHECK(withoutPrimary.TryReadWithDiskName(raw,convert)==std::vector<std::uint8_t>{'Z'} && otherNames->Index()==(again+3)%10);
    const auto unchanged=names->Index();CHECK(files.TryReadWithDiskName(raw,[](auto)->std::optional<std::string> { return {}; })==std::vector<std::uint8_t>{'A'} && names->Index()==(unchanged+2)%10); //잘린ACP의디스크실패도root고리와뒤rawTAFF를유지한다.
}

//현재ACP의비ASCII 이름으로실제18캐시를읽되목록/current는원문31BYTE그대로다.
TEST_CASE(FontFaceEncoding_ActualFontStoreReadsUnicodeDiskAndKeepsRawCatalog) {
    std::string utf8,raw;const std::array<std::u8string_view,5> candidates{u8"Font_글꼴",u8"Font_明朝",u8"Font_Café",u8"Font_Шрифт",u8"Font_字型"}; //설치글꼴이아닌명시캐시이름후보이며OS설정은변경하지않는다.
    for (const auto candidate:candidates) { try { auto value=FontFaceFromUtf8(FontUtf8(candidate));if (value.size()<=31 && std::any_of(value.begin(),value.end(),[](unsigned char byte) { return byte>=128; })) { raw=std::move(value);utf8=FontUtf8(candidate);break; } }catch (const std::invalid_argument&) {} } //현재ACP에서손실없이표현가능한첫독립입력만선택한다.
    CHECK(!raw.empty());if (raw.empty()) return;FontDirectory folder;FontWriteCaches(folder.root,utf8);BaseFileSystem files(folder.root);FontStore fonts(files,raw,[&](const auto& visit) { visit(raw,4,0); });CHECK(fonts.FaceAt(0)==raw && fonts.CurrentFace()==raw && fonts.Get().Measure("A")==12345);fonts.Rebuild(fonts.FaceAt(0));CHECK(fonts.CurrentFace()==raw && FontFaceToUtf8(fonts.CurrentFace())==utf8);
    for (int slot=0;slot<7;++slot) for (std::size_t style=0;style<5;++style) if (FontStore::Available(slot,static_cast<FontStyle>(style))) CHECK(fonts.Get(slot,static_cast<FontStyle>(style)).Measure("A")==12345); //모든18파일이GDI대체없이현재UTF-8디스크에서읽힌다.
    const auto partial=std::string(30,'A')+"\xb0";FontStore cut(files,partial,[&](const auto& visit) { visit(partial+"tail",4,0); });CHECK(cut.CurrentFace()==partial && cut.FaceAt(0)==partial && !FontCacheDiskName(partial,949)); //31BYTE절삭자체를Unicode문자경계로임의보정하지않는다.
}

//원문dirty뒤현재값조회/Config순서에서미완성DBCS는거짓저장성공으로바꾸지않는다.
TEST_CASE(FontFaceEncoding_PartialDbcsKeepsOriginalParentEffectsAndRejectsConfig) {
    const auto partial=std::string(30,'A')+"\xb0";PresentationCommandHooks hooks;std::string trace;std::uint32_t writes{};hooks.fontFace=[&](auto) { return std::optional<std::string_view>(partial); };hooks.rebuildFonts=[&](auto) { trace+='R'; };hooks.width=[] { return 100; };hooks.height=[] { return 80; };hooks.invalidate=[&](auto,auto,auto,auto,auto) { trace+='D'; };hooks.currentFontFace=[&] { trace+='G';return std::string_view(partial); };hooks.set=[&](auto,auto value) { const auto utf8=FontFaceToUtf8(value,949);static_cast<void>(utf8);++writes; };
    bool rejected{};try { DispatchPresentationCommandInput({kChangeFontFaceCommand,0,0},hooks); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && trace=="RDG" && !writes);CHECK(!FontCacheDiskName(partial,949));
    bool unavailable{};try { static_cast<void>(FontFaceFromUtf8(FontUtf8(u8"😀"),1252)); }catch (const std::invalid_argument&) { unavailable=true; }CHECK(unavailable); //표현불가설정의물음표/다른글꼴성공을합성하지않는다.
}

// 원문 raw31BYTE가 맞는 긴 ASCII root에서도 Unicode 디스크 경로 확장이 raw TAFF를 막지 않는다.
TEST_CASE(FontFaceEncoding_Raw31ByteLongAsciiRootKeepsRawRingAndTaffFallback) {
    FontDirectory folder;const auto prefix=folder.root.root_path().generic_u8string(),unique=folder.root.filename().generic_u8string();
    std::string root(prefix.begin(),prefix.end());root.append(unique.begin(),unique.end()); // 실제 디스크 생성은 짧은 검사 소유 폴더에만 하며 긴 base는 읽기 후보로만 사용한다.
    const bool ascii=std::all_of(root.begin(),root.end(),[](unsigned char byte) { return byte<128; });CHECK(ascii && root.size()<190);if (!ascii || root.size()>=190) return;
    root.resize(190,'A');std::string face;
    for (int index=0;index<15;++index) face.append("\xb1\xdb",2); // 보관된949 NLS의 완전한 글 BYTE쌍15개와 ASCII1개로 정확31BYTE를 만든다.
    face+='A';const auto raw="d/!"+face+".normal.14.700.chfnt",utf8=FontFaceToUtf8(raw,949);CHECK(face.size()==31 && raw.size()==54 && utf8.size()==69);
    const auto archive=folder.root/"raw-long-path.tarc";FontWrite(archive,FontArchive(raw,'T'));auto names=std::make_shared<RawFileName>();BaseFileSystem files(FontPath(root),{},names);CHECK(files.RegisterArchive(archive));const auto before=names->Index();
    CHECK(files.TryReadWithDiskName(raw,[](auto name) { return FontCacheDiskName(name,949); })==std::vector<std::uint8_t>{'T'} && names->Index()==(before+2)%10);
    const auto slot=names->CurrentBytes();const auto end=std::find(slot.begin(),slot.end(),std::uint8_t{});const std::string stored(reinterpret_cast<const char*>(slot.data()),static_cast<std::size_t>(end-slot.begin()));
    CHECK(stored.size()==245 && stored.ends_with(face+".normal.14.700.chfnt")); // 고리는 raw245BYTE를 보관하고 UTF-8의260BYTE를 저장하지 않는다.
}
