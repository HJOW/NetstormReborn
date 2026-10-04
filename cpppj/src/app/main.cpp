// 새로 쓰는 실행 진입점: 복원된 데이터 계층을 실제 원본/CD 파일로 검증한다.
// 화면·게임 전체 루프는 아직 구현하지 않았다.
#include <cstdio>
#include <bit>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "client/ClientMain.h"
#include "client/GameAssets.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include "o/OriginalText.h"
#include "o/Xlat.h"
#include "platform/Console.h"
#include "platform/Bitmap.h"

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
    std::printf("  --inspect-assets <game-dir> [--cd]\n  --dump-assets <game-dir> [--cd]\n");
    std::printf("  --export-frame <game-dir> <type> <cluster> <layer> <output.bmp> [--cd]\n");
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

// 선택적인 --cd를 해석한다. 판본은 블록 개수만으로 자동 추정하지 않는다.
netstorm::o::OriginalEdition Edition(int argc, char** argv, int expectedCount) {
    if (argc == expectedCount) return netstorm::o::OriginalEdition::Patch1078;
    if (argc == expectedCount+1 && std::string(argv[expectedCount]) == "--cd") return netstorm::o::OriginalEdition::Cd1072;
    throw std::invalid_argument("Expected optional --cd as the last argument");
}

// 타입·클러스터·그림을 한 번씩 해석하여 실제 로더의 처리 결과를 보고한다.
void InspectAssets(const netstorm::client::GameAssets& assets) {
    std::size_t frames = 0, images = 0, special = 0;
    // 모든 블록을 검사하여 메타데이터와 이미지 레코드 수를 구분한다.
    for (const auto& type : assets.Types()) {
        const auto& block = assets.Shapes().Blocks()[type.block];
        frames += block.frames.size();
        // 실제 압축 해제까지 수행하므로 잘린 픽셀 스트림도 실패로 보고한다.
        for (std::size_t frame = 0; frame < block.frames.size(); ++frame) {
            if (block.frames[frame].IsSpecial()) ++special;
            else { assets.Shapes().Decode(type.block, frame); ++images; }
        }
    }
    std::printf("Loaded asset types: %zu\nShape frames: %zu\nDecoded images: %zu\nSpecial records: %zu\n", assets.Types().size(), frames, images, special);
}

// 길이가 붙은 UTF-8 문자열을 검사 전용 스트림으로 기록한다.
void WriteString(const std::string& value) { WriteLength(static_cast<std::uint32_t>(value.size())); WriteText(value); }
// 숫자형 속성은 리틀 엔디언 IEEE 754 double로 기록하여 텍스트 반올림을 피한다.
void WriteDouble(double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    WriteLength(static_cast<std::uint32_t>(bits)); WriteLength(static_cast<std::uint32_t>(bits >> 32));
}
// 복원된 타입과 모든 픽셀을 한 프로세스에서 독립 Python 판독기와 대조하게 한다.
void DumpAssets(const netstorm::client::GameAssets& assets) {
    WriteLength(1); // 검사 스트림의 스키마 버전이며 원본 파일 형식과는 별개다.
    WriteLength(static_cast<std::uint32_t>(assets.Types().size()));
    // 타입별 속성·클러스터·특수 인덱스·모든 그래픽 프레임을 기록한다.
    for (const auto& asset : assets.Types()) {
        const auto& type = asset.definition;
        WriteString(asset.assetName); WriteString(type.name); WriteString(type.constructor);
        WriteLength(static_cast<std::uint32_t>(type.flags.size()));
        // 미사용 게임 플래그도 원문 이름을 보존한다.
        for (const auto& flag : type.flags) WriteString(flag);
        WriteLength(static_cast<std::uint32_t>(type.properties.size()));
        // 문자열과 숫자형 속성은 서로 다른 태그로 기록한다.
        for (const auto& property : type.properties) {
            WriteString(property.name);
            if (const auto* number = std::get_if<double>(&property.value)) { WriteLength(0); WriteDouble(*number); }
            else { WriteLength(1); WriteString(std::get<std::string>(property.value)); }
        }
        WriteLength(static_cast<std::uint32_t>(type.clusters.size()));
        // 프레임 코드는 side·variant·number·flags의 원래 4바이트다.
        for (const auto& cluster : type.clusters) {
            WriteString(cluster.name);
            WriteBytes({cluster.code.side, cluster.code.variant, cluster.code.number, cluster.code.flags});
            WriteLength(static_cast<std::uint32_t>(cluster.images.size()));
            // GIF 참조를 검증하되 런타임 그래픽 선택에는 쓰지 않는다.
            for (const auto& reference : cluster.images) { WriteString(reference.file); WriteDouble(reference.number); }
        }
        WriteLength(std::bit_cast<std::uint32_t>(type.specialFrames.defaultFrame));
        WriteLength(std::bit_cast<std::uint32_t>(type.specialFrames.gumpFrame));
        WriteLength(std::bit_cast<std::uint32_t>(type.specialFrames.helpFrame));
        WriteLength(std::bit_cast<std::uint32_t>(type.specialFrames.baseFrame));
        const auto foot = type.Footprint();
        WriteLength(std::bit_cast<std::uint32_t>(foot[0])); WriteLength(std::bit_cast<std::uint32_t>(foot[1]));
        const auto& block = assets.Shapes().Blocks()[asset.block];
        WriteLength(static_cast<std::uint32_t>(block.frames.size()));
        // 특수 레코드는 헤더만 기록하고 정상 레코드는 팔레트 번호와 투명 마스크를 기록한다.
        for (std::size_t i = 0; i < block.frames.size(); ++i) {
            const auto& frame = block.frames[i];
            WriteLength(frame.bounds[0]); WriteLength(frame.bounds[1]); WriteLength(frame.origin[0]); WriteLength(frame.origin[1]);
            WriteLength(std::bit_cast<std::uint32_t>(frame.rect.left)); WriteLength(std::bit_cast<std::uint32_t>(frame.rect.top));
            WriteLength(std::bit_cast<std::uint32_t>(frame.rect.right)); WriteLength(std::bit_cast<std::uint32_t>(frame.rect.bottom));
            WriteLength(frame.IsSpecial() ? 1 : 0);
            if (!frame.IsSpecial()) {
                const auto image = assets.Shapes().Decode(asset.block, i);
                WriteLength(static_cast<std::uint32_t>(image.width)); WriteLength(static_cast<std::uint32_t>(image.height));
                WriteBytes(image.indices); WriteBytes(image.opacity);
            }
        }
    }
}

// 자산 로더로 선택한 프레임을 원본 팔레트와 투명도로 BMP에 내보낸다.
void ExportFrame(const netstorm::client::GameAssets& assets, const std::string& typeName,
    const std::string& cluster, const std::string& layerText, const std::filesystem::path& output) {
    if (layerText.empty() || layerText.front() == '-') throw std::invalid_argument("Invalid shape layer");
    std::size_t parsed = 0;
    const auto layer = std::stoull(layerText, &parsed);
    if (parsed != layerText.size()) throw std::invalid_argument("Invalid shape layer");
    const auto& type = assets.Find(typeName);
    const auto frame = assets.FrameIndex(type, cluster, static_cast<std::size_t>(layer));
    const auto image = assets.Shapes().Decode(type.block, frame);
    std::vector<std::uint8_t> rgba(image.indices.size()*4);
    // 투명 여부를 팔레트 번호 0으로 대신하지 않는다.
    for (std::size_t i = 0; i < image.indices.size(); ++i) {
        const auto color = assets.Palette().Color(image.indices[i]);
        rgba[i*4] = color.red; rgba[i*4+1] = color.green; rgba[i*4+2] = color.blue; rgba[i*4+3] = image.opacity[i];
    }
    netstorm::platform::WriteBitmap(output, static_cast<std::uint32_t>(image.width), static_cast<std::uint32_t>(image.height), rgba);
    std::printf("Exported %s/%s layer %zu: %zux%zu -> %s\n", type.assetName.c_str(), cluster.c_str(), static_cast<std::size_t>(layer), image.width, image.height, output.string().c_str());
}
}

// 게임 프로세스를 시작하지 않고 복원된 C++ 데이터 모듈을 실행한다.
int main(int argc, char** argv) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) { PrintBuildInfo(); return 0; }
        const std::string command = argv[1];
        if (argc == 3 && command == "--inspect-data") { InspectData(argv[2]); return 0; }
        if (argc == 3 && command == "--dump-archive") { DumpArchive(argv[2]); return 0; }
        if ((argc == 3 || argc == 4) && (command == "--inspect-assets" || command == "--dump-assets")) {
            const netstorm::client::GameAssets assets(Files(argv[2]), Edition(argc, argv, 3));
            if (command == "--inspect-assets") InspectAssets(assets); else DumpAssets(assets);
            return 0;
        }
        if ((argc == 7 || argc == 8) && command == "--export-frame") {
            const netstorm::client::GameAssets assets(Files(argv[2]), Edition(argc, argv, 7));
            ExportFrame(assets, argv[3], argv[4], argv[5], argv[6]); return 0;
        }
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
