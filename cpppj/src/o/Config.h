// 원본 Config.cpp의 파일 복호화·첫 일치 조회와 설정 객체 층·치환·값 쓰기·섹션.
// 근거·검증 범위: docs/exe/cpp-config-reconstruction.md
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 원본 DAT_00532538~3e(CD 0051846c~)의 설정 문법 특수 문자. 두 판본이 같다.
inline constexpr char kConfigEscape = '`';        // 이스케이프
inline constexpr char kConfigOpenBrace = '{';     // 치환 시작
inline constexpr char kConfigCloseBrace = '}';    // 치환 끝
inline constexpr char kConfigDefaultSeparator = '|'; // 기본값 구분
inline constexpr char kConfigOpenSection = '[';   // 섹션 줄 시작
inline constexpr char kConfigCloseSection = ']';  // 섹션 줄 끝
inline constexpr char kConfigObjectSeparator = '.'; // 객체 접두어 구분
// 원본 인코딩 설정 파일의 평문 서명(DAT_00532564).
inline constexpr std::string_view kConfigSignature = "mQdsT";

// 원본 FUN_0043fd70 ↔ CD 0042a270: 버퍼에서 첫 일치 키의 위치를 찾는다. NUL 이후는 보지 않는다.
std::optional<std::size_t> ConfigFindKey(std::string_view text, std::string_view key);
// 원본 FUN_00440150 ↔ CD 0042ab70: 따옴표 제거, 따옴표 밖 주석 제거한 원시 값.
std::optional<std::string> ConfigGetRaw(std::string_view text, std::string_view key);

class ConfigText {
public:
    // UTF-8 설정 버퍼를 받아 원본 줄 순서를 그대로 유지한다.
    explicit ConfigText(std::string text);
    // 원본 FUN_00440380: 첫째·셋째 바이트 0으로 XOR 파일을 감지한다. 서명은 떼어 낸다(검사 도구용).
    static ConfigText FromBytes(std::span<const std::uint8_t> bytes);
    // 원본 FUN_0043fd70 ↔ CD 0042a270: 첫 일치 키의 줄 앞 위치를 찾는다.
    std::optional<std::size_t> FindKey(std::string_view key) const;
    // 원본 FUN_00440150 ↔ CD 0042ab70: 따옴표 제거, 따옴표 밖 주석 제거.
    std::optional<std::string> GetRaw(std::string_view key) const;
    // 원본 FUN_0043ffa0: 입력 끝의 줄바꿈을 CRLF 하나로 정리해 이어 붙인다. 앞 파일의 값이 우선한다.
    void Append(std::string_view text);
    // 복원·검증 도구가 읽은 버퍼를 확인할 수 있게 한다.
    const std::string& Text() const;
private:
    std::string text_;
};

class Config;

// 원본 전역 배열 DAT_00557e00(패치 1000칸, CD 40칸)·개수 DAT_00558da0을 전역 대신 객체로 둔다.
// 조회는 마지막에 등록한 설정 객체부터 한다.
class ConfigRegistry {
public:
    // 패치판 0043fc20의 `totalConfigs < MAX_CONFIGS` 한도. CD판 0042a010은 40이다.
    static constexpr std::size_t kMaxConfigs = 1000;
    // 등록 순서(오래된 것부터)의 설정 객체 목록.
    std::span<Config* const> Objects() const;
    // 원본 FUN_0043fb90 ↔ CD 0042a0b0: 이름("이름.")이 같은 첫 객체. 없으면 nullptr.
    Config* FindByName(std::string_view name) const;

    // `{_이름}` 환경 변수 조회(원본 getenv). 비어 있으면 찾지 못한 것으로 처리한다.
    std::function<std::optional<std::string>(std::string_view)> environment;
    // `{&경로}` 레지스트리 조회(원본 FUN_004dda10). 비어 있으면 찾지 못한 것으로 처리한다.
    std::function<std::optional<std::string>(std::string_view)> registryValue;
    // `{@미션.키}`: 임시 설정 객체에 미션 스크립트를 읽어 넣는다(원본은 missionSpec 경로를 읽는다).
    std::function<void(Config&, std::string_view)> loadMission;
private:
    friend class Config;
    // 원본 0043fc20 끝부분: 배열 끝에 넣는다.
    void Register(Config* config);
    // 원본 FUN_0043fab0 ↔ CD 0042a050: 같은 포인터를 모두 빼고 순서를 유지한다.
    void Unregister(Config* config);
    std::vector<Config*> objects_;
};

// 원본 설정 객체(0x18바이트): +0 버퍼, +8 환경 변수 허용, +0xc "이름.", +0x10 이름 길이, +0x14 변경 표시.
class Config {
public:
    // 원본 FUN_0043fc20 ↔ CD 0042a150: 이름이 있으면 "이름."을 접두어로 삼고 전역 목록에 등록한다.
    explicit Config(ConfigRegistry& registry, std::optional<std::string_view> name = std::nullopt);
    // 원본 FUN_0043fcd0 ↔ CD 0042a1e0: 목록에서 뺀다.
    ~Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 버퍼가 한 번이라도 만들어졌는가(원본 `*this != 0`, "theConfiguration.isValid()").
    bool IsValid() const;
    // 원본 FUN_0043fd40 ↔ CD 0042a210: 버퍼를 버린다. 이름·등록은 그대로다.
    void Clear();
    // 새 API(검사·도구용): 버퍼를 주어진 내용 그대로 바꾼다. 원본에서는 버퍼 포인터를 직접 다루는 것에 해당한다.
    void Assign(std::string text);
    // 원본 FUN_0043ffa0 ↔ CD 0042a430: 끝의 CR/LF를 정리하고 CRLF 하나를 붙여 버퍼 끝에 더한다.
    void Append(std::string_view text);
    // 원본 FUN_00440330 ↔ CD 0042ad20: `[이름]` 섹션 줄을 더한다.
    void AppendSection(std::string_view name);
    // 원본 FUN_00440380 ↔ CD 0042ad70의 복호화: XOR 감지 후 버퍼에 더한다. 서명 `mQdsT`는 원본처럼 남는다.
    void LoadBytes(std::span<const std::uint8_t> bytes);
    // 원본 FUN_00440410 ↔ CD 0042ae60: 명령줄 형식. `;`는 줄바꿈, `@파일`은 그 파일을 읽어 넣는다.
    void AppendArguments(std::string_view text,
        const std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>& readFile);

    // 이 객체의 버퍼에서만 찾는 원시 값(원본 FUN_00440150). 버퍼가 없으면 찾지 못한다.
    std::optional<std::string> GetRaw(std::string_view key) const;
    // 원본 FUN_004409d0 ↔ CD 0042a930: 모든 등록 객체에서 찾아 치환한 값. 없으면 빈 값이다.
    std::optional<std::string> Get(std::string_view key);
    // 원본 FUN_00440b60 ↔ CD 0042a960: 이스케이프·`{키|기본값}` 치환, 끝 공백 제거.
    std::string Substitute(std::string_view source);
    // 원본 FUN_00440240 ↔ CD 0042ac50: 같은 키의 줄을 모두 지운다. 하나라도 있었으면 참.
    bool RemoveKey(std::string_view key);
    // 원본 FUN_004404e0 ↔ CD 0042af30: 같은 키를 지우고 끝에 `키 = "값"`을 더한다. 이전에 있었으면 참.
    bool Set(std::string_view key, std::string_view value);

    // 원본 FUN_00440570 ↔ CD 0042afd0: 섹션 줄 다음부터 다음 섹션 줄 앞까지의 본문.
    std::optional<std::string_view> Section(std::string_view name) const;
    // 원본 FUN_00440640 ↔ CD 0042b090: 이름이 접두어로 시작하는 섹션 이름을 `이름;` 꼴로 이어 붙인다.
    std::string SectionNames(std::string_view prefix) const;

    // 원본 +8: 참이면 `{_이름}`을 환경 변수에서 찾는다.
    bool environmentEnabled{};
    // 원본 +0x14: Set이 켠다. 저장 여부 판단은 호출자 몫이다.
    bool changed{};
    // 접두어("이름." 또는 빈 문자열).
    const std::string& Prefix() const;
    // 검사 도구용 버퍼. 버퍼가 없으면 빈 문자열이다.
    const std::string& Text() const;
private:
    friend class ConfigRegistry;
    // 원본 FUN_00440b60의 본체: dest 끝에 이어 쓴다.
    void SubstituteInto(std::string& dest, std::string_view source, int depth);
    // 원본 FUN_00440a00 ↔ CD 0042a540: `{키|기본값}` 한 개를 풀어 dest 끝에 쓴다.
    void ExpandBrace(std::string& dest, std::string_view source, std::size_t& pos, int depth);
    // 원본 FUN_00440760 ↔ CD 0042a6b0: 키를 찾아 치환한 값을 dest 끝에 쓴다.
    bool Lookup(std::string key, std::string_view fallback, std::string& dest, bool reportMissing, int depth);

    ConfigRegistry& registry_;
    std::optional<std::string> text_; // 비어 있음 = 원본의 널 버퍼.
    std::string prefix_;
};

// 원본 FUN_0043fe60 ↔ CD 0042a320: `[이름` 으로 시작하는 섹션 줄을 찾아 그 다음 줄의 위치를 돌려준다.
// 찾지 못하면 빈 값. matchedName에는 `[` 다음부터 `]` 앞까지를 넣는다.
std::optional<std::size_t> ConfigFindSection(std::string_view text, std::size_t from,
    std::string_view header, std::string* matchedName = nullptr);
}
