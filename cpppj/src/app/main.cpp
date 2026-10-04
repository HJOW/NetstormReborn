// 새로 쓰는 실행 진입점: 복원된 데이터 계층을 실제 원본/CD 파일로 검증한다.
// 화면·게임 전체 루프는 아직 구현하지 않았다.
#include <cstdio>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "client/ClientMain.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include "o/OriginalText.h"
#include "o/Xlat.h"
#include "platform/Console.h"

namespace {
// 복원의 기준이 되는 원본 판본.
constexpr const char* kTargetOriginalVersion = "10.78";

// 실행 가능한 공용 계층의 명령과 아직 구현하지 않은 범위를 출력한다.
void PrintBuildInfo() {
    std::printf("NetstormCpp (reconstructed core; game UI pending)\n");
    std::printf("  reconstructed from: NetStorm %s (decompiled)\n", kTargetOriginalVersion);
    std::printf("  default maxFPS: %d (frame interval %.6f s, 1ms clock: %u ms)\n",
        netstorm::client::kDefaultMaxFps, netstorm::client::FrameIntervalSeconds(netstorm::client::kDefaultMaxFps),
        netstorm::client::QuantizedFrameMilliseconds(netstorm::client::kDefaultMaxFps));
    std::printf("  --inspect-data <game-dir>\n  --read-archive <tarc> <entry>\n  --dump-archive <tarc>\n  --read-game <game-dir> <entry>\n");
    std::printf("  --config <file> <key>\n  --translate-game <game-dir> <language-number> <original-text>\n");
}

// 실제 자산을 읽는 VFS를 만든다. 아카이브 등록 순서는 명시적으로 지정한다.
netstorm::o::BaseFileSystem Files(const std::filesystem::path& root) {
    netstorm::o::BaseFileSystem files(root);
    files.RegisterArchive(root / "netstorm.tarc");
    return files;
}

// 원본 바이트를 손실 없이 출력하여 Python 도구와 바이트 단위 대조를 가능하게 한다.
void WriteBytes(const std::vector<std::uint8_t>& bytes) {
    netstorm::platform::UseBinaryStdout();
    if (!bytes.empty() && std::fwrite(bytes.data(), 1, bytes.size(), stdout) != bytes.size())
        throw std::runtime_error("Unable to write stdout");
}

// UTF-8 텍스트도 줄바꿈 변환 없이 출력한다.
void WriteText(const std::string& text) { WriteBytes(std::vector<std::uint8_t>(text.begin(), text.end())); }

// 일괄 검사 출력의 길이는 원본과 같이 리틀 엔디언 32비트로 쓴다.
void WriteLength(std::uint32_t value) {
    std::vector<std::uint8_t> bytes(4);
    // 낮은 바이트부터 순서대로 기록한다.
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(value >> (i*8));
    WriteBytes(bytes);
}

// 검사 전용 프레임 형식: 개수, 각 이름 길이·이름·내용 길이·내용. 원본 TAFF 형식을 바꾸지 않는다.
void DumpArchive(const std::filesystem::path& path) {
    const auto archive = netstorm::o::TaffArchive::Open(path);
    WriteLength(static_cast<std::uint32_t>(archive.Entries().size()));
    // 모든 엔트리를 한 프로세스에서 읽어 검사기의 반복 실행 비용을 없앤다.
    for (const auto& entry : archive.Entries()) {
        WriteLength(static_cast<std::uint32_t>(entry.name.size())); WriteText(entry.name);
        const auto data = archive.Read(entry.name);
        WriteLength(static_cast<std::uint32_t>(data.size())); WriteBytes(data);
    }
}

// 복호화된 설정·번역 자산을 읽어 실행 가능한 공용 계층의 상태를 보고한다.
void InspectData(const std::filesystem::path& root) {
    const auto archive = netstorm::o::TaffArchive::Open(root / "netstorm.tarc");
    auto files = Files(root);
    const auto config = netstorm::o::ConfigText::FromBytes(files.Read("d/setup.cfg"));
    const auto translations = netstorm::o::XlatTable(netstorm::o::DecodeOriginalText(files.Read("d/xlat.german")));
    std::printf("TAFF entries: %zu\n", archive.Entries().size());
    const auto major = config.GetRaw("gamemaster"); const auto minor = config.GetRaw("gameminor");
    if (major && minor) std::printf("Version declared in setup.cfg: %s.%s\n", major->c_str(), minor->c_str());
    else std::printf("Version not declared in setup.cfg\n");
    std::printf("German translation keys: %zu\n", translations.Size());
}
}

// 게임 프로세스를 시작하지 않고 복원된 C++ 데이터 모듈을 실행한다.
int main(int argc, char** argv) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) { PrintBuildInfo(); return 0; }
        const std::string command = argv[1];
        if (argc == 3 && command == "--inspect-data") { InspectData(argv[2]); return 0; }
        if (argc == 3 && command == "--dump-archive") { DumpArchive(argv[2]); return 0; }
        if (argc == 4 && command == "--read-archive") { WriteBytes(netstorm::o::TaffArchive::Open(argv[2]).Read(argv[3])); return 0; }
        if (argc == 4 && command == "--read-game") { WriteBytes(Files(argv[2]).Read(argv[3])); return 0; }
        if (argc == 4 && command == "--config") {
            const auto value = netstorm::o::ConfigText::FromBytes(netstorm::o::ReadFileBytes(argv[2])).GetRaw(argv[3]);
            if (!value) return 2;
            WriteText(*value); return 0;
        }
        if (argc == 5 && command == "--translate-game") {
            const int language = std::stoi(argv[3]);
            // 원본 언어 번호만 읽는다. 한국어 UTF-8 언어 파일 연결은 후속 작업이다.
            const std::vector<std::string> languages{"english","english","french","german","spanish","japanese","portuguese"};
            if (language < 0 || language >= static_cast<int>(languages.size())) throw std::runtime_error("Unsupported language number");
            const auto text = Files(argv[2]).Read("d/xlat." + languages[static_cast<std::size_t>(language)]);
            WriteText(netstorm::o::XlatTable(netstorm::o::DecodeOriginalText(text)).Translate(argv[4], language)); return 0;
        }
        PrintBuildInfo(); return 2;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
