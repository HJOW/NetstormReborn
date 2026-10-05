// 두 판본 원본 x86 기계어에서 생성한 기대값으로 설정 객체 층·치환·줄 삭제·값 쓰기·섹션 검색을 검사한다.
// 기대값 생성: tools/decomp_config_oracle.py
#include "TestSupport.h"
#include "o/Config.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// 기대값 생성 도구의 getenv 대체와 같은 고정 환경 변수 표(이름은 ASCII 대소문자 무시).
const std::map<std::string, std::string> kOracleEnvironment = {{"nsenv", "env{A}val"}, {"nsempty", ""}};
// 기대값 파일의 줄 수(Def 줄 제외). 도구가 보고한 개수와 같아야 한다.
constexpr std::size_t kExpectedRows = 2440;
// 두 판본 저장·XOR 쓰기 기계어가 같은 출력을 낸 입력 수(실제 설정 버퍼 2개 포함).
constexpr std::size_t kExpectedOptionsRows = 16;

// TSV의 16진수 칸을 바이트 문자열로 바꾼다. '-'는 빈 문자열이다.
std::string Unhex(const std::string& text) {
    if (text == "-") return {};
    if (text.size() % 2 != 0) throw std::runtime_error("Invalid fixture hex");
    std::string result;
    // 두 자리씩 1바이트로 변환한다.
    for (std::size_t pos = 0; pos < text.size(); pos += 2) result.push_back(static_cast<char>(std::stoul(text.substr(pos, 2), nullptr, 16)));
    return result;
}

// 서명·파일 이름 섹션·END·4KB XOR 경계·비ASCII를 원본 두 판본이 저장한 바이트와 비교한다.
TEST_CASE(ConfigSave_MatchesBothOriginalX86Editions) {
    std::ifstream file(NETSTORM_OPTIONS_FIXTURE);
    CHECK(file.is_open());
    std::string line;
    std::size_t count = 0;
    // 기대값은 tools/decomp_options_oracle.py가 원본 기계어로 만들었다.
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line);
        std::string nameHex, textHex, expectedHex;
        stream >> nameHex >> textHex >> expectedHex;
        CHECK(!stream.fail());
        const auto raw = Unhex(textHex);
        const std::vector<std::uint8_t> bytes(raw.begin(), raw.end());
        ConfigInterface config({});
        config.Configuration().Assign(DecodeOriginalText(bytes));
        config.Configuration().changed = true;
        const auto result = EncodeConfigFile(config.SaveText(Unhex(nameHex)));
        CHECK(std::string(result.begin(), result.end()) == Unhex(expectedHex));
        CHECK(!config.Configuration().changed);
        ++count;
    }
    CHECK(count == kExpectedOptionsRows);
}

// 한 입력의 설정 객체 묶음. 등록 순서가 기대값의 객체 순서다.
struct Stack {
    ConfigRegistry registry;
    std::vector<std::unique_ptr<Config>> objects;
    std::vector<bool> hadText;
};

// TSV에서 `개수 {이름 버퍼 환경}×개수`를 읽어 객체를 만든다. '~'는 없음, '@n'은 Def 줄의 버퍼다.
void ReadStack(std::istringstream& stream, const std::map<std::string, std::string>& defined, Stack& stack) {
    std::size_t count = 0;
    stream >> count;
    stack.registry.environment = [](std::string_view name) -> std::optional<std::string> {
        const auto found = kOracleEnvironment.find(AsciiLower(name));
        if (found == kOracleEnvironment.end()) return std::nullopt;
        return found->second;
    };
    // 객체를 등록 순서대로 만든다.
    for (std::size_t i = 0; i < count; ++i) {
        std::string name, text;
        int environment = 0;
        stream >> name >> text >> environment;
        const std::string decodedName = name == "~" ? std::string() : Unhex(name);
        auto config = name == "~" ? std::make_unique<Config>(stack.registry)
                                  : std::make_unique<Config>(stack.registry, std::string_view(decodedName));
        if (text != "~") config->Assign(text[0] == '@' ? defined.at(text.substr(1)) : Unhex(text));
        config->environmentEnabled = environment != 0;
        stack.hadText.push_back(text != "~");
        stack.objects.push_back(std::move(config));
    }
}
}

// Python 모델이 아니라 원본 기계어의 결과가 기대값이다. 두 판본이 같은 결과를 낸 입력만 들어 있다.
TEST_CASE(ConfigObjectLayer_MatchesBothOriginalX86Editions) {
    std::ifstream file(NETSTORM_CONFIG_FIXTURE);
    CHECK(file.is_open());
    std::map<std::string, std::string> defined;
    std::string line;
    std::size_t count = 0;
    // TSV 한 줄이 하나의 입력이다.
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line);
        std::string kind;
        stream >> kind;
        const int failuresBefore = netstorm::test::FailureCount();
        if (kind == "Def") {
            std::string id, hex;
            stream >> id >> hex;
            defined[id] = Unhex(hex);
            continue;
        }
        if (kind == "Subst" || kind == "Get") {
            std::size_t self = 0;
            stream >> self;
            Stack stack;
            ReadStack(stream, defined, stack);
            if (kind == "Subst") {
                std::string source, expected;
                stream >> source >> expected;
                const auto actual = stack.objects.at(self)->Substitute(Unhex(source));
                if (actual != Unhex(expected)) std::fprintf(stderr, "  source [%s] expected [%s] actual [%s]\n",
                    Unhex(source).c_str(), Unhex(expected).c_str(), actual.c_str());
                CHECK(actual == Unhex(expected));
            } else {
                std::string key, expected;
                int found = 0;
                stream >> key >> found >> expected;
                const auto value = stack.objects.at(self)->Get(Unhex(key));
                CHECK(value.has_value() == (found != 0));
                // 찾지 못했을 때 원본 출력 버퍼는 비어 있다(기본값·표시 없음).
                CHECK(value.value_or(std::string()) == Unhex(expected));
            }
        } else if (kind == "Remove") {
            std::string text, key, after;
            int found = 0;
            stream >> text >> key >> found >> after;
            ConfigRegistry registry;
            Config config(registry);
            config.Assign(Unhex(text));
            CHECK(config.RemoveKey(Unhex(key)) == (found != 0));
            CHECK(config.Text() == Unhex(after));
        } else if (kind == "Set") {
            std::size_t self = 0, target = 0;
            stream >> self;
            Stack stack;
            ReadStack(stream, defined, stack);
            std::string key, value, outKey, outValue, after;
            int existed = 0;
            stream >> key >> value >> existed >> target >> outKey >> outValue >> after;
            CHECK(stack.objects.at(self)->Set(Unhex(key), Unhex(value)) == (existed != 0));
            // 변경 표시는 고른 대상 객체에만 켜진다.
            for (std::size_t i = 0; i < stack.objects.size(); ++i) CHECK(stack.objects[i]->changed == (i == target));
            // 대상 버퍼 = 원본이 줄을 지운 뒤의 버퍼 + 서식 줄(원본 서식 "%s = \"%s\"" + CRLF).
            const std::string appended = Unhex(outKey) + " = \"" + Unhex(outValue) + "\"\r\n";
            CHECK(stack.objects.at(target)->Text() == (after == "~" ? std::string() : Unhex(after)) + appended);
        } else if (kind == "FindSection") {
            std::string text, header, expectedName;
            int wantName = 0;
            long long expected = 0;
            stream >> text >> header >> wantName >> expected >> expectedName;
            const std::string buffer = Unhex(text);
            std::string name;
            const auto found = ConfigFindSection(buffer, 0, Unhex(header), wantName != 0 ? &name : nullptr);
            CHECK((found ? static_cast<long long>(*found) : -1) == expected);
            if (found && wantName != 0) CHECK(name == Unhex(expectedName));
        } else if (kind == "Section") {
            std::string text, name;
            long long expected = 0;
            std::size_t length = 0;
            stream >> text >> name >> expected >> length;
            ConfigRegistry registry;
            Config config(registry);
            config.Assign(Unhex(text));
            const auto body = config.Section(Unhex(name));
            CHECK((body ? static_cast<long long>(body->data() - config.Text().data()) : -1) == expected);
            CHECK((body ? body->size() : 0) == length);
        } else throw std::runtime_error("Unknown config oracle fixture kind");
        CHECK(!stream.fail());
        if (netstorm::test::FailureCount() != failuresBefore) std::fprintf(stderr, "Config oracle row %zu (%s) failed\n", count + 1, kind.c_str());
        ++count;
    }
    CHECK(count == kExpectedRows);
}
