// 설정 시작 순서·접근 함수·저장 내용·미션 조회의 단위 검사.
// 조회·치환 규칙 자체는 ConfigOracleTests.cpp가 원본 x86 기대값으로 검사한다.
#include "TestSupport.h"
#include "o/BaseFile.h"
#include "client/Mission.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <map>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// 검사용 가짜 파일 묶음. 경로는 원본 표기 그대로 비교한다.
using FakeFiles = std::map<std::string, std::string>;

// 가짜 파일을 읽는 함수를 만든다. 없는 파일은 빈 값이다.
ConfigInterface::FileReader Reader(const FakeFiles& files) {
    return [&files](std::string_view path) -> std::optional<std::vector<std::uint8_t>> {
        const auto found = files.find(std::string(path));
        if (found == files.end()) return std::nullopt;
        return std::vector<std::uint8_t>(found->second.begin(), found->second.end());
    };
}
// 평문을 원본 형식(서명 + XOR)으로 인코딩한 문자열을 만든다.
std::string Encoded(std::string_view text) {
    const auto bytes = EncodeConfigFile(std::string(kConfigSignature) + std::string(text));
    return std::string(bytes.begin(), bytes.end());
}
// 원본 설정 파일 구성을 흉내 낸 가짜 파일.
FakeFiles MakeFiles() {
    return {
        {"d\\options.cfg", Encoded("InstallDir = \"old\"\r\nSCREENW = \"1024\"\r\ncurrentLanguage = \"german\"\r\n")},
        {"d\\setup.cfg", Encoded("\r\n// comment\r\nDataDir = \"\\D\"\r\nSCREENW = \"640\"\r\ncurrentLanguage=\"english\"\r\n"
                                 "missionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\r\n"
                                 "LanguageSpec = \"{DataDir}\\config.{local.1}\"\r\n"
                                 "title = \"{@tutorial1.name|none}\"\r\nmissing = \"{@nofile.name|none}\"\r\n"
                                 "zero = \"0\"\r\nseven = \" 7x\"\r\n")},
        {"\\D\\TUTORIAL1.german", "name = \"Erste Schritte\"\r\n"},
        {"\\D\\config.german", "Sun = \"Sonne\"\r\nSCREENW = \"1\"\r\n"},
    };
}
}

// 원본 CRT atol과 `;` 목록 함수의 경계.
TEST_CASE(ConfigInterface_ParseLongAndListItem_FollowOriginalRules) {
    CHECK(ConfigParseLong("") == 0);
    CHECK(ConfigParseLong("  \t-12abc") == -12);
    CHECK(ConfigParseLong("+7") == 7);
    CHECK(ConfigParseLong("x1") == 0);
    CHECK(ConfigParseLong("4294967297") == 1); // 넘침 검사 없이 32비트로 감긴다.
    CHECK(ConfigListItem("a;b;;d", 0) == "a");
    CHECK(ConfigListItem("a;b;;d", 1) == "b");
    CHECK(ConfigListItem("a;b;;d", 2).empty());
    CHECK(ConfigListItem("a;b;;d", 3) == "d");
    CHECK(ConfigListItem("a;b;;d", 4).empty());
    CHECK(ConfigListItem("a;b", -1).empty());
}

// 시작 순서: 버전 줄 → 없는 파일의 섹션 줄 → options → setup → [END] 뒤에 InstallDir·CDDir.
TEST_CASE(ConfigInterface_Startup_BuildsSectionsInOriginalOrder) {
    const auto files = MakeFiles();
    ConfigInterface config(Reader(files));
    ConfigInterface::StartupOptions options;
    options.commandLine = "cheat=1;SCREENW=\"800\"";
    options.majorVersion = 10;
    options.minorVersion = 78;
    options.installDir = "C:\\NS";
    config.Startup(options);
    const auto& text = config.Configuration().Text();
    CHECK(text.starts_with("[ARGS]\r\ncheat=1\r\nSCREENW=\"800\"\r\nMAJOR_VERSION=10\r\nMINOR_VERSION=78\r\nversion=\"v10.78\"\r\n"
                           "[user.cfg]\r\n[options.cfg]\r\nmQdsTInstallDir = \"old\"\r\n"));
    CHECK(text.ends_with("[END]\r\nInstallDir = \"C:\\NS\"\r\nCDDir = \"C:\\NS\"\r\n"));
    CHECK(text.find("[dev.cfg]\r\n[guild.cfg]\r\n[setup.cfg]\r\nmQdsT\r\n// comment") != std::string::npos);
    // 먼저 읽은 것이 우선한다: 명령줄 > options > setup.
    CHECK(config.GetInt("SCREENW") == 800);
    CHECK(config.Configuration().Get("currentLanguage") == "german");
    CHECK(config.Configuration().Get("version") == "v10.78");
    // 서명이 붙은 options.cfg 첫 줄은 죽은 줄이고, 시작 순서가 쓴 값이 조회된다.
    CHECK(config.Configuration().Get("InstallDir") == "C:\\NS");
    CHECK(config.PathSpec("missionSpec", "TEST01") == "\\D\\TEST01.german");
    CHECK(config.PathSpec("missionSpec") == "\\D\\{LOCAL.1}.german");
    CHECK(config.SubstituteLocal("{local.1}-{local.2|x}-{DataDir}", "a") == "a-x-\\D");
    // 임시 local 객체는 조회가 끝나면 목록에서 빠진다.
    CHECK(config.Registry().Objects().size() == 2);
}

// 정수·문자열 읽기와 "0에서 달라졌는가" 판정.
TEST_CASE(ConfigInterface_TypedAccessors_KeepOriginalQuirks) {
    const auto files = MakeFiles();
    ConfigInterface config(Reader(files));
    config.Startup({});
    std::int32_t width = 640;
    CHECK(config.ReadInt("SCREENW", width) && width == 1024);
    CHECK(!config.ReadInt("SCREENW", width)); // 같은 값이면 거짓.
    CHECK(!config.ReadInt("nokey", width) && width == 1024);
    std::string language = "english";
    CHECK(config.ReadString("currentLanguage", language) && language == "german");
    CHECK(!config.ReadString("currentLanguage", language));
    CHECK(config.IntEquals("seven", 7));
    CHECK(!config.IntEquals("seven", 8));
    CHECK(!config.IntEquals("zero", 0)); // 0과 같은지는 원본에서 항상 거짓이다.
    CHECK(config.GetInt("nokey") == 0);
}

// `{@미션.키}`는 missionSpec 경로의 파일을 임시 객체로 읽는다. 키는 대문자로 바뀐 뒤 경로에 들어간다.
TEST_CASE(ConfigInterface_MissionLookup_ReadsMissionScriptThroughSpec) {
    const auto files = MakeFiles();
    ConfigInterface config(Reader(files));
    config.Startup({});
    CHECK(config.Configuration().Get("title") == "Erste Schritte");
    CHECK(config.Configuration().Get("missing") == "none");
    CHECK(config.Configuration().Substitute("{@tutorial1.nokey}") == "{@TUTORIAL1}"); // '.'에서 잘린 키가 남는다.
    CHECK(config.Configuration().Substitute("{@nodot}").empty());
    CHECK(config.Registry().Objects().size() == 2);
    // 용어표는 전역 설정보다 먼저 등록되어 조회에서는 뒤 순위다.
    config.LoadLanguage(*config.Configuration().Get("currentLanguage"));
    CHECK(config.Configuration().Get("Sun") == "Sonne");
    CHECK(config.GetInt("SCREENW") == 1024);
}

// 값 쓰기는 버퍼 전체에서 같은 키를 지우고 [END] 뒤에 붙인다. 저장 내용은 options 본문 + [END] 본문이다.
TEST_CASE(ConfigInterface_SetAndSave_RoundTripsThroughOriginalFormat) {
    auto files = MakeFiles();
    ConfigInterface config(Reader(files));
    ConfigInterface::StartupOptions options;
    options.installDir = "C:\\NS";
    config.Startup(options);
    int notifications = 0;
    config.onChanged = [&notifications] { ++notifications; };
    CHECK(config.Set("SCREENW", "800"));
    CHECK(!config.SetInt("newKey", -3));
    CHECK(config.Set("currentLanguage", "fran\xc3\xa7" "ais")); // 내부 UTF-8, 원본 파일에서는 Windows-1252.
    CHECK(notifications == 3 && config.Configuration().changed);
    CHECK(config.GetInt("SCREENW") == 800);
    const auto saved = config.SaveText("d\\options.cfg");
    CHECK(!config.Configuration().changed);
    CHECK(saved == "mQdsTInstallDir = \"old\"\r\nInstallDir = \"C:\\NS\"\r\nCDDir = \"C:\\NS\"\r\n"
                   "SCREENW = \"800\"\r\nnewKey = \"-3\"\r\ncurrentLanguage = \"fran\xc3\xa7" "ais\"\r\n");
    // 저장한 바이트를 options.cfg로 다시 읽으면 같은 값이 나온다. setup.cfg의 SCREENW·currentLanguage 줄은 지워졌었다.
    const auto bytes = EncodeConfigFile(saved);
    CHECK(bytes.size() >= 3 && bytes[0] == 0 && bytes[2] == 0); // 원본의 XOR 감지 조건.
    files["d\\options.cfg"] = std::string(bytes.begin(), bytes.end());
    ConfigInterface reloaded(Reader(files));
    reloaded.Startup(options);
    CHECK(reloaded.GetInt("SCREENW") == 800);
    CHECK(reloaded.GetInt("newKey") == -3);
    CHECK(reloaded.Configuration().Get("currentLanguage") == "fran\xc3\xa7" "ais");
    CHECK(reloaded.Configuration().Text().find("fran\xc3\xa7" "ais") != std::string::npos);
}

// 256개 원본 바이트 모두 역변환되며 저장 서명 뒤에 UTF-8 BOM이 추가되지 않아야 한다.
TEST_CASE(ConfigEncoding_PreservesEveryWindows1252Byte) {
    std::vector<std::uint8_t> original;
    // 정의되지 않은 Windows-1252 제어 바이트도 그대로 왕복시킨다.
    for (int i = 0; i < 256; ++i) original.push_back(static_cast<std::uint8_t>(i));
    CHECK(EncodeOriginalText(DecodeOriginalText(original)) == original);
    const auto encoded = EncodeConfigFile("mQdsTName = \"caf\xc3\xa9 \xe2\x82\xac\"\r\n");
    auto decoded = encoded;
    ApplyXor(decoded);
    CHECK(std::string(decoded.begin(), decoded.end()) == "mQdsTName = \"caf\xe9 \x80\"\r\n");
}

// 파일 쓰기·인코딩 실패 때 설정 변경을 보존하고, 성공 뒤에만 변경 표시를 끈다.
TEST_CASE(ConfigInterface_SaveFile_PreservesChangesOnFailure) {
    const auto files = MakeFiles();
    ConfigInterface config(Reader(files));
    config.Startup({});
    config.SetInt("SCREENW", 800);
    bool failed = false;
    try {
        // 디스크 쓰기 오류를 재현한다.
        config.SaveFile("options.cfg", [](std::string_view, std::span<const std::uint8_t>) {
            throw std::runtime_error("write failure");
        });
    } catch (const std::runtime_error&) { failed = true; }
    CHECK(failed && config.Configuration().changed);
    std::vector<std::uint8_t> saved;
    // 성공한 쓰기에서 실제로 전달한 바이트와 이름을 기록한다.
    config.SaveFile("options.cfg", [&saved](std::string_view name, std::span<const std::uint8_t> bytes) {
        CHECK(name == "options.cfg");
        saved.assign(bytes.begin(), bytes.end());
    });
    CHECK(!saved.empty() && !config.Configuration().changed);
    config.Set("name", "\xed\x95\x9c"); // 원본 Windows-1252에 없는 한글.
    failed = false;
    try {
        // 인코딩이 실패하면 파일 쓰기 자체가 시작되지 않아야 한다.
        config.SaveFile("options.cfg", [](std::string_view, std::span<const std::uint8_t>) { CHECK(false); });
    } catch (const std::invalid_argument&) { failed = true; }
    CHECK(failed && config.Configuration().changed);
}

// 인자 처리: 첫 `@` 인자 전까지는 글, 그 뒤는 파일. 글 안의 `@낱말`은 그 이름('@' 포함)의 파일을 먼저 읽는다.
TEST_CASE(ConfigInterface_Load_SwitchesFromTextToFilesAtFirstAtSign) {
    const FakeFiles files{{"@extra", "E = \"from file\"\r\n"}, {"a.cfg", "A = \"1\"\r\n"}, {"x\\b.cfg", "B = \"2\"\r\n"}};
    ConfigInterface config(Reader(files));
    const std::vector<std::string> arguments{"K=1;@extra L=2", "@a.cfg", "x\\b.cfg", "", "@missing.cfg"};
    config.Load(arguments);
    CHECK(config.Configuration().Text() == "[ARGS]\r\nE = \"from file\"\r\nK=1\r\n L=2\r\n[a.cfg]\r\nA = \"1\"\r\n"
                                           "[b.cfg]\r\nB = \"2\"\r\n[missing.cfg]\r\n[END]\r\n");
    CHECK(config.Configuration().environmentEnabled);
    CHECK(config.Configuration().SectionNames("") == "ARGS;a.cfg;b.cfg;missing.cfg;");
    CHECK(config.Configuration().Section("a.cfg") == "A = \"1\"\r\n");
    CHECK(!config.Configuration().Section("END")); // 본문이 없는 마지막 섹션은 찾지 못한 것으로 다룬다.
}

// 미션 시작(원본 00482fb0): `mission` 설정 객체, 추가 설정, loadFort·missionType, fortSpec 경로.
TEST_CASE(MissionScript_ResolvesScriptFortAndType) {
    FakeFiles files{
        {"d\\setup.cfg", "DataDir = \"\\D\"\r\ncurrentLanguage=\"english\"\r\n"
                         "missionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\r\nfortSpec = \"{DataDir}\\{local.1}.fort\"\r\n"
                         "banner = \"{mission.title|none}\"\r\n"},
        {"\\D\\first.english", "[Header]\r\nmissionType=\"Tutorial\"\r\ntitle=\"First {DataDir}\"\r\nmyStartMoney=3000 // comment\r\n"},
        {"\\D\\second.english", "missionType=\"Battle\"\r\nloadFort=\"shared\"\r\n"},
    };
    ConfigInterface config(Reader(files));
    config.Startup({});
    CHECK(config.Configuration().Get("banner") == "none"); // 미션 객체가 없을 때.
    {
        netstorm::client::MissionScript first(config, "first");
        CHECK(first.Loaded() && first.ScriptPath() == "\\D\\first.english");
        CHECK(first.MissionType() == "Tutorial" && first.FortName() == "first" && first.FortPath() == "\\D\\first.fort");
        CHECK(first.Get("title") == "First \\D"); // 스크립트 값도 전역 설정으로 치환된다.
        CHECK(first.Get("myStartMoney") == "3000" && !first.Get("nokey"));
        // 미션 객체가 살아 있는 동안 다른 설정에서 `{mission.키}`를 쓸 수 있다.
        CHECK(config.Configuration().Get("banner") == "First \\D");
        CHECK(config.Registry().Objects().size() == 3);
    }
    CHECK(config.Registry().Objects().size() == 2 && config.Configuration().Get("banner") == "none");
    // loadFort가 있으면 그 요새를 읽는다. 추가 설정 글은 스크립트보다 먼저 들어가 우선한다.
    netstorm::client::MissionScript second(config, "second", "missionType=\"Override\";extra=1");
    CHECK(second.FortName() == "shared" && second.FortPath() == "\\D\\shared.fort");
    CHECK(second.MissionType() == "Override" && second.Get("extra") == "1");
    // 스크립트가 없으면 미션 이름이 요새 이름이 된다.
    netstorm::client::MissionScript missing(config, "nofile");
    CHECK(!missing.Loaded() && missing.MissionType().empty() && missing.FortPath() == "\\D\\nofile.fort");
}
