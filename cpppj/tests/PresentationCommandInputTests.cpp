// 새 원문 관측과 실제 FontStore/Config/음량 수명을 장치·게임 없이 집중 확인한다.
#include "RawSceneSupport.h"
#include "client/PresentationCommandInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/BitmapFont.h"
#include "client/ClientAudio.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
using namespace netstorm::client; //현재 글꼴/설정/음향 부모를 실제 제품과 공유한다.
using namespace netstorm::test::rawscene; //동결 fixture 해독과 독립 BYTE 입력 작성기다.

namespace { //모든 검사 파일/목록은 명시 입력이며 보호 원본 경로에는 쓰지 않는다.
// 새 TSV를 실제 소스 파일 위치 기준으로 읽어 실행 cwd에 의존하지 않는다.
FixtureData PresentationRows(std::string_view name) { const auto root=std::filesystem::path(__FILE__).parent_path();return LoadFixture((root/"fixtures"/name).string().c_str()); }
// 원문 raw 이름/문자열은 UTF-8 치환을 거치지 않고 첫 NUL도 보존한다.
std::string PresentationBytes(std::string_view hex) { if (hex=="-") return {};std::string result;for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(i,2)),nullptr,16)));return result; } //각 fixture BYTE를 같은 자리에 복원한다.
// 효과 문자열은 원문 BYTE의16진수로 비교한다.
std::string PresentationHex(std::string_view text) { constexpr char digits[]="0123456789abcdef";std::string result;for (const auto byte:text) { const auto value=static_cast<unsigned char>(byte);result+=digits[value>>4];result+=digits[value&15]; }return result; } //부호 있는 char를 직접 색인하지 않는다.
// 명시 호출 순서를 원문 trace와 같은 세미콜론 형식으로 축적한다.
void PresentationTrace(std::string& trace,std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
// Native 부모에서 실제 child 내부 Clear/Create/Cache와 부모의 Dirty/Set을 구별한다.
std::string PresentationParentTrace(std::string_view trace) { std::string result;for (const auto& item:Split(trace,';')) if (item.starts_with("Dirty:") || item.starts_with("Set:")) PresentationTrace(result,item);return result; } //관측한 부모 순서를 바꾸지 않는다.
// 4a3cc4의 최대31BYTE/current first-NUL 저장이다. 실제 캐시 별도 검사가 같은 결과를 확인한다.
std::string PresentationCurrent(std::string_view source) { return std::string(source.substr(0,std::min<std::size_t>(31,source.find('\0')))); }
}

// 원문 이름표/파서/FIFO21행을 실제 제품의 이름 및80칸 문자열 고리로 재생한다.
TEST_CASE(PresentationCommand_Native21NamesAndQueue) {
    const auto fixture=PresentationRows("presentation-command-names-x86.tsv");std::size_t count{};
    for (const auto& row:fixture.rows) { //원래/소문자/대문자와 미등록의 진단/고리 순서를 모두 읽는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::vector<std::size_t> diagnostics;
        QueueScriptCommand(PresentationBytes(row[1]),queue,strings,{[](auto name) { CHECK(GameMenuScriptCode(name)==HtmlScriptCommandValue(name));return GameMenuScriptCode(name); },[&](auto) { diagnostics.push_back(strings.Index()); }});
        CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") CHECK(queue.Empty());else {
            const auto expected=Split(row[6],',');const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && queue.Empty());
            if (expected[1].front()=='s') CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.x))==PresentationBytes(expected[1].substr(1)));else CHECK(std::bit_cast<std::uint32_t>(event.x)==Number(expected[1].substr(1)));
        }
        CHECK(diagnostics.size()==static_cast<std::size_t>(std::count(row[8].begin(),row[8].end(),'L')));++count;
    }
    CHECK(count==21);CHECK(GameMenuCommandValue(0x542764U)==kChangeFontFaceCommand);CHECK(GameMenuCommandValue(0x542a68U)==kChangeSoundQualityCommand);
}

// 음질32입력의 선행 종료·실제 signed%d Config·priestmove.wav와release를 대조한다.
TEST_CASE(PresentationCommand_SoundNative32RowsAndPartialFailure) {
    const auto fixture=PresentationRows("sound-quality-command-x86.tsv");std::size_t count{};
    for (const auto& row:fixture.rows) { //수식/outside도 release 이외의 새 관문으로 쓰지 않는다.
        CHECK(row.size()==4);if (row.size()!=4) continue;std::string trace;PresentationCommandHooks hooks;hooks.closeSound=[&] { PresentationTrace(trace,"Close"); };hooks.config=[&](auto text) { PresentationTrace(trace,"Config:"+PresentationHex(text)); };
        hooks.sound=[&](auto name,auto a,auto b,auto c,auto d,auto e) { CHECK(!(a|b|c|d|e));PresentationTrace(trace,"Sound:"+PresentationHex(name)); };
        CHECK(DispatchPresentationCommandInput({Number(row[0]),37,std::bit_cast<std::int32_t>(Number(row[1]))},hooks));CHECK((trace.empty()?"-":trace)==row[2]);++count;
    }
    CHECK(count==32);CHECK(!DispatchPresentationCommandInput({'D',0,0},{}));CHECK(DispatchPresentationCommandInput({kChangeFontFaceCommand|InputCode::kRelease,0,0},{}));
    std::string trace;PresentationCommandHooks hooks;hooks.closeSound=[&] { trace+="C"; };hooks.config=[&](auto) { trace+="S";throw std::runtime_error("명시 Config 실패"); };hooks.sound=[&](auto,auto,auto,auto,auto,auto) { trace+="P"; };
    CHECK(Throws([&] { DispatchPresentationCommandInput({kChangeSoundQualityCommand,0,-1},hooks); }));CHECK(trace=="CS"); //종료 뒤 Config가 실패하면 되돌림/음향을 합성하지 않는다.
}

// 원문 옵션192입력의 장치 자연 반환→두 음량→현재 곡 순서와 실패 enabled 보존을 확인한다.
TEST_CASE(PresentationCommand_AudioNative192RowsAndFailureOrder) {
    const auto fixture=PresentationRows("audio-reinterpret-x86.tsv");std::size_t count{};
    for (const auto& row:fixture.rows) { //실제 OS 대신 이 몸체가 사용하는 장치 반환만 명시한다.
        CHECK(row.size()==7);if (row.size()!=7) continue;ClientAudioOptions options;options.sound=Number(row[0])!=0;options.quality=std::bit_cast<std::int32_t>(Number(row[1]));options.soundVolumeStep=std::bit_cast<std::int32_t>(Number(row[2]));options.musicVolumeStep=std::bit_cast<std::int32_t>(Number(row[3]));options.music=Number(row[4])!=0;
        std::string trace;bool globals=false,enabled=false;ClientAudioInterpretHooks hooks;hooks.globals=[&](const auto& current) { globals=true;enabled=current.sound; };
        hooks.open=[&](auto quality) { CHECK(globals);PresentationTrace(trace,"Open:"+std::to_string(std::bit_cast<std::uint32_t>(quality)));enabled=Number(row[5])!=0; };hooks.close=[&] { CHECK(globals);PresentationTrace(trace,"Close");enabled=false; };
        hooks.effectVolume=[&](auto value) { PresentationTrace(trace,"EffectVolume:"+std::to_string(std::bit_cast<std::uint32_t>(value))); };hooks.musicVolume=[&](auto value) { PresentationTrace(trace,"MusicVolume:"+std::to_string(std::bit_cast<std::uint32_t>(value))); };hooks.selectMusic=[&](auto on) { PresentationTrace(trace,"Music:"+std::to_string(on ? 1 : 0)); };
        ReinterpretClientAudioOptions(options,hooks);CHECK(trace==row[6]);CHECK(enabled==(options.sound && Number(row[5])!=0));++count;
    }
    CHECK(count==192);
    ClientAudioOptions options;ClientAudioInterpretHooks hooks;std::string trace;hooks.globals=[&](const auto&) { trace+="G"; };hooks.open=[&](auto) { trace+="O";throw std::runtime_error("명시 장치 예외"); };hooks.effectVolume=[&](auto) { trace+="V"; };
    CHECK(Throws([&] { ReinterpretClientAudioOptions(options,hooks); }));CHECK(trace=="GO"); //예외는 자연 실패 반환과 구별하고 전역 쓰기 뒤 음량을 실행하지 않는다.
}

// 실제 장치가 없는 ClientAudio도 같은 Scene 기한/음소거 깊이/예약 음량을 보존한다.
TEST_CASE(PresentationCommand_ActualAudioMutedOptionsKeepDeadlineAndClock) {
    std::size_t clocks{};ClientAudio audio({},netstorm::o::OriginalEdition::Patch1078,"english",[&] { ++clocks;return 41.25; },1);audio.WriteSongEndBits(0x0123456789abcdefULL);audio.PushMute();
    ClientAudioOptions options;options.sound=false;options.music=false;options.quality=-1;options.soundVolumeStep=1;options.musicVolumeStep=4;audio.Interpret(0,options);
    CHECK(std::bit_cast<std::uint64_t>(audio.Scene().songEnd)==0x0123456789abcdefULL && clocks==0);const auto held=audio.Describe();CHECK(held.find("muteDepth\t1\n")!=held.npos && held.find("pendingMasterVolume\t-4000\n")!=held.npos && held.find("pendingMusicVolume\t-500\n")!=held.npos);
    audio.PopMute();const auto released=audio.Describe();CHECK(released.find("muteDepth\t0\n")!=released.npos && released.find("masterVolume\t-4000\n")!=released.npos && released.find("musicVolume\t-500\n")!=released.npos);CHECK(!audio.DeviceReady() && clocks==0);
}

// 원문 Enum315입력의 정확type/charset 및 first-NUL/31BYTE를 실제 FontStore 목록에 재생한다.
TEST_CASE(PresentationCommand_FontCatalogNative315RowsAndLookupBoundary) {
    netstorm::o::BaseFileSystem files(std::filesystem::path{});std::size_t count{};
    for (const auto& row:PresentationRows("font-face-enum-x86.tsv").rows) { //가짜 OS의 동일 두 번 열거만 제공하며 GDI/캐시를 열지 않는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;const auto name=PresentationBytes(row[2]);std::size_t enumerations{};FontStore fonts(files,"Arial",[&](const auto& visitor) { ++enumerations;visitor(name,Number(row[0]),static_cast<std::uint8_t>(Number(row[1]))); });
        CHECK(enumerations==2);const auto face=fonts.FaceAt(0);const auto selected=Number(row[5])==18;CHECK(face.has_value()==selected);
        if (face) { const auto record=PresentationBytes(row[8]);CHECK(*face==std::string_view(record.data(),record.find('\0'))); }CHECK(!fonts.FaceAt(1));++count;
    }
    CHECK(count==315);std::size_t lookups{};
    for (const auto& row:PresentationRows("font-face-lookup-x86.tsv").rows) { //손상된 signed 음수 count는 정상 목록의 제품 입력으로 재해석하지 않는다.
        CHECK(row.size()==6);if (row.size()!=6 || (Number(row[1])&0x80000000U)) continue;const auto available=row[2]=="1" ? Number(row[1]) : 0U;
        FontStore fonts(files,"Arial",[&](const auto& visitor) { for (std::uint32_t i=0;i<available;++i) visitor("Arial",4,0); }); //원문 정상 count/table 조합만 현재 저장소로 만든다.
        const auto index=std::bit_cast<std::int32_t>(Number(row[0]));if (index<0) { CHECK(Number(row[4])==1);CHECK(Throws([&] { static_cast<void>(fonts.FaceAt(index)); })); }else CHECK(fonts.FaceAt(index).has_value()==(Number(row[3])!=0));++lookups;
    }
    CHECK(lookups==54);
}

// font parent의 현재 H/W·dirty 뒤 이름 재조회와 각 효과 실패 꼬리를 원문에서 읽는다.
TEST_CASE(PresentationCommand_FontParentNativeRowsAndLateReaders) {
    std::size_t count{};for (const auto& row:PresentationRows("font-face-rebuild-x86.tsv").rows) { //실제 child의 내부 글꼴/캐시 효과와 일반 입력 부모를 구별한다.
        CHECK(row.size()==9);if (row.size()!=9 || row[1]!="1") continue;const auto source=row[0]=="NULL" ? std::string{} : PresentationBytes(row[0]);std::string current="PreviousFace",trace;std::int32_t width=640,height=480;PresentationCommandHooks hooks;
        hooks.fontFace=[&](auto index)->std::optional<std::string_view> { CHECK(index==0);return row[0]=="NULL" ? std::nullopt : std::optional<std::string_view>(source); };
        hooks.rebuildFonts=[&](auto face) { if (row[2]=="Clear" || row[2]=="Create" || row[2]=="Cache") throw std::runtime_error("명시 child 중단");current=PresentationCurrent(face ? *face : "Arial");if (row[3]=="1") { width=1280;height=720; } };
        hooks.width=[&] { return width; };hooks.height=[&] { return height; };hooks.invalidate=[&](auto x,auto y,auto w,auto h,auto flags) { PresentationTrace(trace,"Dirty:"+std::to_string(x)+":"+std::to_string(y)+":"+std::to_string(w)+":"+std::to_string(h)+":"+std::to_string(flags));if (row[2]=="Dirty") throw std::runtime_error("명시 dirty 중단");if (row[4]=="1") current="ChangedAtDirty"; };
        hooks.currentFontFace=[&] { return std::string_view(current); };hooks.set=[&](auto key,auto value) { PresentationTrace(trace,"Set:"+PresentationHex(key)+":"+PresentationHex(value));if (row[2]=="Set") throw std::runtime_error("명시 Config 중단"); };
        bool completed=true;try { CHECK(DispatchPresentationCommandInput({kChangeFontFaceCommand,37,0},hooks)); }catch (const std::runtime_error&) { completed=false; }
        CHECK(completed==(row[5]=="1"));CHECK(trace==PresentationParentTrace(row[8]));const auto expected=PresentationBytes(row[6]);CHECK(current==std::string(expected.data(),expected.find('\0')));++count;
    }
    CHECK(count==15);
}

namespace { //전체18개 원본 이름의 최소 CHFNT를 검사 전용 경로에만 만든다.
// 새 임시 CHFNT와 같은 FontStore를 소유하는 파일 검사 장치다.
struct PresentationFonts {
    std::filesystem::path root; //이 객체가 새로 소유한 임시 경로만 소멸 시 제거한다.
    netstorm::o::BaseFileSystem files; //실제 VFS로 파일 이름과 캐시 재읽기를 확인한다.
    FontStore fonts; //동일 인스턴스 참조가 Rebuild 뒤에도 유지되어야 한다.
    static std::filesystem::path NewRoot() { const auto path=std::filesystem::temp_directory_path()/("netstorm-font764-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));if (!std::filesystem::create_directory(path)) throw std::runtime_error("검사 글꼴 경로 생성 실패");return path; } //기존 경로는 재사용하지 않는다.
    // 기존 경로를 재사용하지 않고 실제 저장소와 가짜 OS 목록을 연결한다.
    PresentationFonts():root(NewRoot()),files(root),fonts(files,"Arial",[](const auto& visitor) { visitor("Arial",4,0);visitor("NextFace",4,0); }) { std::filesystem::create_directory(root/"d"); }
    ~PresentationFonts() { std::error_code ignored;std::filesystem::remove_all(root,ignored); } //보호 원본/사용자 자산 경로에는 접근하지 않는다.
    // 유효18개 캐시에 독립 전진 폭을 기록해 재적재를 확인한다.
    void Write(std::string_view face,std::uint32_t width) {
        constexpr std::array<int,7> heights{14,13,0,20,48,14,12},weights{700,0,0,700,0,0,0};constexpr std::array<const char*,5> styles{"normal","italic","bold","strikeout","underline"}; //원본 파일 이름의 요청 높이/굵기다.
        for (int slot=0;slot<7;++slot) for (std::size_t style=0;style<styles.size();++style) { //유효18개 조합만 기록해 GDI fallback을 필요로 하지 않는다.
            if (!FontStore::Available(slot,static_cast<FontStyle>(style))) continue;std::vector<std::uint8_t> cache(4132);constexpr std::string_view signature{"BitmapFontData\x1a\0",16};std::copy(signature.begin(),signature.end(),cache.begin());Put(cache,16,1);Put(cache,20,14);Put(cache,24,10);Put(cache,28,4);Put(cache,32,256);Put(cache,36+'A'*4,slot==1 ? 3U : width);
            const auto path=root/"d"/("!"+std::string(slot==1 ? "Courier New" : face)+"."+styles[style]+"."+std::to_string(heights[slot])+"."+std::to_string(weights[slot])+".chfnt");std::ofstream stream(path,std::ios::binary);stream.write(reinterpret_cast<const char*>(cache.data()),static_cast<std::streamsize>(cache.size()));if (!stream) throw std::runtime_error("검사 CHFNT 쓰기 실패");
        }
    }
};
}

// 실제 저장소의 참조·CHFNT 우선·동일face 재읽기·NULL/빈face를 구별한다. GDI/게임은 실행하지 않는다.
TEST_CASE(PresentationCommand_ActualFontStoreClearsAndReloadsChfntInPlace) {
    PresentationFonts input;input.Write("Arial",7);input.Write("NextFace",11);input.Write("",17);auto* identity=&input.fonts;CHECK(input.fonts.Get().Measure("A")==7);
    input.fonts.Rebuild(input.fonts.FaceAt(1));CHECK(&input.fonts==identity && input.fonts.CurrentFace()=="NextFace");
    for (int slot=0;slot<7;++slot) for (std::size_t style=0;style<5;++style) if (FontStore::Available(slot,static_cast<FontStyle>(style))) CHECK(input.fonts.Get(slot,static_cast<FontStyle>(style)).Measure("A")== (slot==1 ? 3 : 11)); //모든18개 캐시를 새 이름으로 읽었다.
    input.Write("NextFace",13);CHECK(input.fonts.Get().Measure("A")==11);input.fonts.Rebuild(input.fonts.FaceAt(1));CHECK(input.fonts.Get().Measure("A")==13);
    input.fonts.Rebuild(std::nullopt);CHECK(input.fonts.CurrentFace()=="Arial" && input.fonts.Get().Measure("A")==7);input.fonts.Rebuild(std::string_view{});CHECK(input.fonts.CurrentFace().empty() && input.fonts.Get().Measure("A")==17);
}
