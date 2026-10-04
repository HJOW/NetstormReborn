// 원본: 00440e00(시작 읽기) ↔ CD 004a9120, 00441790(시작 순서) ↔ CD 004a8b10, 00440f30(저장) ↔ CD 004a9270,
//       00441090/004410f0/004411e0/00441270/00441300/00441470/004414d0/00441520/00441660/00441db0(접근 함수).
// 범위: 설정 읽기 순서·접근 함수·저장할 내용 만들기. 설정 파일 위치 탐색(6개 후보 경로)·작업 폴더 변경·
//       CD 찾기·`*.tarc` 등록·로그 출력은 옮기지 않았다(호출자가 경로와 파일 읽기를 준다).
// 검증: 조회·치환 자체는 Config.cpp의 x86 기대값으로 검증한다. 이 파일의 순서·서식은 정적 대조와 단위 검사다.
#include "o/ConfigInterface.h"
#include "o/BaseFile.h"
#include <algorithm>

namespace netstorm::o {
namespace {
// 시작 읽기가 버퍼 앞뒤에 두는 섹션 이름(원본 문자열 00506be4, 00506bc4).
constexpr std::string_view kArgumentsSection = "ARGS";
constexpr std::string_view kEndSection = "END";
// `local.1`~`local.3`을 담는 임시 설정 객체의 이름(원본 "local").
constexpr std::string_view kLocalObjectName = "local";
// 저장할 때 서명 뒤에 넣는 UTF-8 BOM(새 확장).
constexpr std::string_view kUtf8Bom = "\xef\xbb\xbf";

// 원본 FUN_00459b20(경로, 1): 마지막 `\` 또는 `:` 뒤의 파일 이름. `/`도 받는 것은 새 확장이다.
std::string_view FileNamePart(std::string_view path) {
    const auto separator = path.find_last_of("\\:/");
    return separator == std::string_view::npos ? path : path.substr(separator + 1);
}
// 임시 `local` 객체에 `번호="값"` 줄을 넣는다(원본 서식 "1=\"%s\"" 등).
void AppendLocalArgument(Config& local, int number, const std::optional<std::string_view>& argument) {
    if (argument) local.Append(std::to_string(number) + "=\"" + std::string(*argument) + "\"");
}
}

// CRT의 "C" 로케일 isspace 뒤에 부호와 숫자를 읽는다. 곱셈·덧셈은 부호 없는 32비트로 감아 계산한다.
std::int32_t ConfigParseLong(std::string_view text) {
    std::size_t pos = 0;
    // 앞의 공백류(공백, \t, \n, \v, \f, \r)를 건너뛴다.
    while (pos < text.size() && (text[pos] == ' ' || (text[pos] >= '\t' && text[pos] <= '\r'))) ++pos;
    bool negative = false;
    if (pos < text.size() && (text[pos] == '-' || text[pos] == '+')) negative = text[pos++] == '-';
    std::uint32_t total = 0;
    // 숫자가 아닌 글자에서 멈춘다.
    for (; pos < text.size() && text[pos] >= '0' && text[pos] <= '9'; ++pos)
        total = total * 10u + static_cast<std::uint32_t>(text[pos] - '0');
    return static_cast<std::int32_t>(negative ? 0u - total : total);
}
// 원본은 복사본에서 `;`를 NUL로 바꿔 가며 index번째 조각의 시작을 찾는다.
std::string ConfigListItem(std::string_view list, int index) {
    std::size_t start = 0;
    // 앞 조각을 index개 건너뛴다.
    for (; index > 0; --index) {
        const auto separator = list.find(';', start);
        if (separator == std::string_view::npos) return {};
        start = separator + 1;
    }
    if (index < 0) return {};
    const auto end = list.find(';', start);
    return std::string(list.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
}
// 원본 저장은 파일 전체에 키 "mydoghasfleas"를 XOR 한다(0041a620).
std::vector<std::uint8_t> EncodeConfigFile(std::string_view text) {
    std::string plain;
    if (!text.starts_with(kConfigSignature)) plain += kConfigSignature;
    plain += text;
    // ASCII 밖의 글자가 있으면 서명 바로 뒤에 BOM을 넣어 UTF-8임을 표시한다.
    const bool ascii = std::all_of(plain.begin(), plain.end(), [](char c) { return static_cast<unsigned char>(c) < 0x80; });
    if (!ascii) plain.insert(kConfigSignature.size(), kUtf8Bom);
    std::vector<std::uint8_t> bytes(plain.begin(), plain.end());
    ApplyXor(bytes);
    return bytes;
}

// 멤버 선언 순서가 등록 순서다: 용어표가 먼저, 전역 설정이 나중이라 조회는 전역 설정을 먼저 본다.
ConfigInterface::ConfigInterface(FileReader reader)
    : reader_(std::move(reader)), language_(registry_), configuration_(registry_) {
    // `{@미션.키}`: 원본 00440760은 missionSpec 경로의 미션 스크립트를 임시 객체에 읽는다.
    registry_.loadMission = [this](Config& temporary, std::string_view mission) {
        LoadFile(temporary, PathSpec("missionSpec", mission));
    };
}
// 파일이 없으면 아무것도 더하지 않는다.
void ConfigInterface::LoadFile(Config& config, std::string_view path) {
    if (!reader_) return;
    if (const auto bytes = reader_(path)) config.LoadBytes(*bytes);
}
// 원본 00440e00. 첫 글자가 `@`인 인자가 나오기 전까지는 인자를 글로 넣고, 그 뒤로는 모두 파일로 읽는다.
void ConfigInterface::Load(std::span<const std::string> arguments) {
    configuration_.Clear();
    configuration_.environmentEnabled = true;
    configuration_.AppendSection(kArgumentsSection);
    bool inlineMode = true;
    // 인자를 받은 순서대로 처리한다.
    for (const auto& argument : arguments) {
        std::string_view path = argument;
        if (!path.empty() && path.front() == '@') path.remove_prefix(1);
        else if (inlineMode) {
            configuration_.AppendArguments(argument, reader_);
            continue;
        }
        inlineMode = false;
        if (path.empty()) continue;
        // 파일마다 `[파일이름]` 섹션 줄을 먼저 넣는다. 파일이 없어도 섹션 줄은 남는다.
        configuration_.AppendSection(FileNamePart(path));
        LoadFile(configuration_, path);
    }
    configuration_.AppendSection(kEndSection);
}
// 원본 00441790이 00440e00에 넘기는 인자 순서 그대로다. demo.cfg는 경로만 만들고 읽지 않는다(원본 그대로).
void ConfigInterface::Startup(const StartupOptions& options) {
    const auto major = std::to_string(options.majorVersion), minor = std::to_string(options.minorVersion);
    std::vector<std::string> arguments{
        options.commandLine,
        "MAJOR_VERSION=" + major + ";MINOR_VERSION=" + minor + ";version=\"v" + major + "." + minor + "\";",
        "@",
        options.identity.empty() ? std::string() : options.dataPrefix + options.identity + ".cfg",
        options.dataPrefix + "user.cfg",
        options.dataPrefix + "options.cfg",
        options.dataPrefix + "dev.cfg",
        options.dataPrefix + "guild.cfg",
        options.dataPrefix + "setup.cfg"};
    Load(arguments);
    Set("InstallDir", options.installDir);
    Set("CDDir", options.cdDir.empty() ? options.installDir : options.cdDir);
}
// 값이 없으면 빈 문자열을 읽어 0이 된다.
std::int32_t ConfigInterface::GetInt(std::string_view key) {
    return ConfigParseLong(configuration_.Get(key).value_or(std::string()));
}
// 임시 객체는 이 함수가 끝날 때 목록에서 빠진다. 인자가 하나도 없으면 임시 객체의 버퍼는 없다.
std::string ConfigInterface::PathSpec(std::string_view key, std::optional<std::string_view> argument1,
    std::optional<std::string_view> argument2, std::optional<std::string_view> argument3) {
    Config local(registry_, kLocalObjectName);
    AppendLocalArgument(local, 1, argument1);
    AppendLocalArgument(local, 2, argument2);
    AppendLocalArgument(local, 3, argument3);
    return configuration_.Get(key).value_or(std::string());
}
// 원본은 "찾았고 지금 값과 다르다"일 때만 1을 돌려준다.
bool ConfigInterface::ReadInt(std::string_view key, std::int32_t& value) {
    const auto text = configuration_.Get(key);
    if (!text) return false;
    const auto parsed = ConfigParseLong(*text);
    if (parsed == value) return false;
    value = parsed;
    return true;
}
// 문자열 비교는 대소문자를 구분한다(원본의 바이트 비교).
bool ConfigInterface::ReadString(std::string_view key, std::string& value) {
    const auto text = configuration_.Get(key);
    if (!text || *text == value) return false;
    value = *text;
    return true;
}
// 원본 00441470의 지역 변수 초기값은 0이다.
bool ConfigInterface::IntEquals(std::string_view key, std::int32_t expected) {
    std::int32_t value = 0;
    return ReadInt(key, value) && value == expected;
}
// 값을 쓴 뒤 알림을 부른다.
bool ConfigInterface::Set(std::string_view key, std::string_view value) {
    const bool existed = configuration_.Set(key, value);
    if (onChanged) onChanged();
    return existed;
}
// 원본 서식 "%d".
bool ConfigInterface::SetInt(std::string_view key, std::int32_t value) { return Set(key, std::to_string(value)); }
// 경로 지정값과 같은 임시 객체를 쓰되 키 하나가 아니라 글 전체를 치환한다.
std::string ConfigInterface::SubstituteLocal(std::string_view source, std::optional<std::string_view> argument1,
    std::optional<std::string_view> argument2, std::optional<std::string_view> argument3) {
    Config local(registry_, kLocalObjectName);
    AppendLocalArgument(local, 1, argument1);
    AppendLocalArgument(local, 2, argument2);
    AppendLocalArgument(local, 3, argument3);
    return configuration_.Substitute(source);
}
// 언어를 바꿀 때마다 다시 읽는다. 파일이 없으면 용어표는 빈 채로 남는다.
void ConfigInterface::LoadLanguage(std::string_view language) {
    language_.Clear();
    LoadFile(language_, PathSpec("LanguageSpec", language));
}
// 원본은 `[파일이름]` 본문이 서명으로 시작하지 않으면 서명을 먼저 쓰고, 이어서 `[END]` 본문을 쓴다.
std::string ConfigInterface::SaveText(std::string_view fileName) {
    std::string result;
    if (const auto body = configuration_.Section(FileNamePart(fileName))) {
        if (!body->starts_with(kConfigSignature)) result += kConfigSignature;
        result += *body;
    }
    if (const auto end = configuration_.Section(kEndSection)) result += *end;
    configuration_.changed = false;
    return result;
}
// 전역 설정을 돌려준다.
Config& ConfigInterface::Configuration() { return configuration_; }
// 언어 용어표를 돌려준다.
Config& ConfigInterface::Language() { return language_; }
// 설정 객체 목록을 돌려준다.
ConfigRegistry& ConfigInterface::Registry() { return registry_; }
// 파일 읽기 함수를 돌려준다.
const ConfigInterface::FileReader& ConfigInterface::Reader() const { return reader_; }
}
