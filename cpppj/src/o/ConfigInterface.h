// 원본 Configinterface.cpp: 전역 설정 `theConfiguration`(00558de8)·언어 용어표(00558e00)와 접근 함수.
// 전역 변수 대신 객체 하나로 묶는다. 근거·검증 범위: docs/exe/cpp-config-reconstruction.md
#pragma once
#include "o/Config.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 원본 CRT atol(MSVC): 앞 공백·부호·숫자만 읽고 넘침은 검사하지 않는다(32비트로 감긴다).
std::int32_t ConfigParseLong(std::string_view text);
// 원본 FUN_004411e0 ↔ CD 004a9560: `;`로 나눈 목록의 index번째 항목. 범위를 벗어나면 빈 문자열.
std::string ConfigListItem(std::string_view list, int index);
// 설정 버퍼의 한 부분을 원본 형식 파일 바이트로 만든다: XOR 인코딩, 서명 `mQdsT`.
// 새 확장: ASCII가 아닌 글자가 있으면 서명 뒤에 UTF-8 BOM을 넣는다(Config::LoadBytes가 읽는다).
std::vector<std::uint8_t> EncodeConfigFile(std::string_view text);

class ConfigInterface {
public:
    // 게임 경로 표기(`d\options.cfg`, `\D\config.english`)의 파일을 읽는다. 없으면 빈 값.
    using FileReader = std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;

    // 시작 순서(원본 FUN_00441790 ↔ CD 004a8b10의 설정 부분)에 필요한 값.
    struct StartupOptions {
        std::string commandLine;      // 실행 인자(원본 param_2). `키=값;키=값` 꼴.
        int majorVersion{};           // MAJOR_VERSION
        int minorVersion{};           // MINOR_VERSION
        std::string dataPrefix{"d\\"}; // 설정 파일이 있는 폴더(원본은 setup.cfg를 찾은 위치로 정한다).
        std::string identity;         // 환경 변수 IDENTITY 또는 USER. 비어 있으면 그 파일은 건너뛴다.
        std::string installDir;       // InstallDir에 쓸 값(원본은 데이터 폴더의 상위 폴더).
        std::string cdDir;            // CDDir에 쓸 값(원본은 CD를 찾지 못하면 InstallDir과 같다).
    };

    // 원본 정적 초기화 순서대로 언어 용어표, 전역 설정을 등록한다(004fc8c0, 004fc8e0).
    explicit ConfigInterface(FileReader reader);

    // 원본 FUN_00440e00 ↔ CD 004a9120: `[ARGS]`, 인자(첫 `@` 인자 전까지는 글, 그 뒤는 파일), `[END]`.
    void Load(std::span<const std::string> arguments);
    // 원본 FUN_00441790의 설정 부분: 버전 → 사용자별 → user → options → dev → guild → setup 순으로 읽고
    // InstallDir·CDDir을 쓴다.
    void Startup(const StartupOptions& options);

    // 원본 FUN_00441090 ↔ CD 004a9400: 치환한 값을 정수로. 없으면 0.
    std::int32_t GetInt(std::string_view key);
    // 원본 FUN_004410f0 ↔ CD 004a9460: `local.1`~`local.3`에 인자를 넣고 키를 조회한다(경로 지정값).
    std::string PathSpec(std::string_view key, std::optional<std::string_view> argument1 = std::nullopt,
        std::optional<std::string_view> argument2 = std::nullopt, std::optional<std::string_view> argument3 = std::nullopt);
    // 원본 FUN_00441270: 키가 있고 값이 지금 값과 다르면 바꾸고 참. 설정으로 기본값을 덮어쓸 때 쓴다.
    bool ReadInt(std::string_view key, std::int32_t& value);
    // 원본 FUN_00441300 ↔ CD 004a96e0: 문자열판. 키가 있고 값이 다르면 바꾸고 참.
    bool ReadString(std::string_view key, std::string& value);
    // 원본 FUN_00441470 ↔ CD 004a98d0: 0에서 시작해 ReadInt가 값을 바꿨고 그 값이 expected면 참.
    // 그래서 expected가 0이면 항상 거짓이다(원본 그대로).
    bool IntEquals(std::string_view key, std::int32_t expected);
    // 원본 FUN_004414d0 ↔ CD 004a9930: 값을 쓰고 변경 알림을 부른다. 이전에 있었으면 참.
    bool Set(std::string_view key, std::string_view value);
    // 원본 FUN_00441520 ↔ CD 004a9980: 정수를 "%d"로 써서 Set.
    bool SetInt(std::string_view key, std::int32_t value);
    // 원본 FUN_00441660/00441740 ↔ CD 004a9ae0/004a9bd0: `local.1`~`local.3`을 넣고 글 전체를 치환한다.
    std::string SubstituteLocal(std::string_view source, std::optional<std::string_view> argument1 = std::nullopt,
        std::optional<std::string_view> argument2 = std::nullopt, std::optional<std::string_view> argument3 = std::nullopt);
    // 원본 FUN_00441db0 ↔ CD 004a93d0: 용어표를 비우고 `LanguageSpec` 경로의 파일을 읽는다.
    void LoadLanguage(std::string_view language);
    // 원본 FUN_00440f30 ↔ CD 004a9270: `[파일이름]` 섹션과 `[END]` 섹션을 이어 저장할 평문을 만든다.
    // 서명이 없으면 붙인다. 변경 표시를 끈다. 파일에 쓰는 일은 호출자가 한다(원본 폴더에 쓰지 않는다).
    std::string SaveText(std::string_view fileName);

    // 원본 DAT_00558dd0: 값을 쓸 때마다 부르는 알림.
    std::function<void()> onChanged;
    // 전역 설정(theConfiguration).
    Config& Configuration();
    // 언어 용어표.
    Config& Language();
    // 설정 객체 목록.
    ConfigRegistry& Registry();
    // 파일 읽기 함수. 미션 스크립트처럼 다른 설정 객체에 파일을 읽어 넣을 때 쓴다.
    const FileReader& Reader() const;
private:
    // 파일이 있으면 읽어 설정 객체에 더한다(원본 FUN_00440380).
    void LoadFile(Config& config, std::string_view path);

    ConfigRegistry registry_;
    FileReader reader_;
    Config language_;
    Config configuration_;
};
}
