// 새로 쓰는 실행 진입점: 복원된 데이터 계층을 실제 원본/CD 파일로 검증한다.
// 화면·게임 전체 루프는 아직 구현하지 않았다.
#include <cstdio>
#include <cstdlib>
#include <bit>
#include <exception>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "app/InspectView.h"
#include "client/ClientMain.h"
#include "client/GameAssets.h"
#include "client/Mission.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include "o/ConfigInterface.h"
#include "o/Islandbuilder.h"
#include "o/OriginalText.h"
#include "o/Template.h"
#include "o/Xlat.h"
#include "platform/Console.h"
#include "platform/Bitmap.h"
#include "platform/FileOutput.h"

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
    std::printf("  --config-dump <game-dir> [--cd]\n  --config-get <game-dir> <key> [--cd]\n");
    std::printf("  --config-spec <game-dir> <key> [arg1 [arg2 [arg3]]] [--cd]\n");
    std::printf("  --config-save <game-dir> <output-file> [key=value ...] [--cd]\n");
    std::printf("  --dump-types <game-dir> [--cd]\n  --inspect-fort <game-dir> <mission-or-path> [--cd]\n  --dump-forts <game-dir> [--cd]\n");
    std::printf("  --inspect-mission <game-dir> <mission> [--cd]\n  --dump-territories <game-dir> [--cd]\n");
    std::printf("  --run <game-dir> [--view types|<mission>] [--window] [--frames N] [--screenshot out.bmp] [--set \"k=v;k=v\"] [--cd]\n");
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

// 환경 변수를 읽는다. 없으면 빈 값이다(설정의 `{_이름}` 조회와 시작 순서의 IDENTITY·USER에 쓴다).
std::optional<std::string> Environment(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    if (!value) return std::nullopt;
    return std::string(value);
}

// 인자 끝의 선택적인 --cd를 떼어 내고 판본을 정한다.
netstorm::o::OriginalEdition TakeEdition(std::vector<std::string>& arguments) {
    if (!arguments.empty() && arguments.back() == "--cd") { arguments.pop_back(); return netstorm::o::OriginalEdition::Cd1072; }
    return netstorm::o::OriginalEdition::Patch1078;
}

// 원본 시작 순서(FUN_00441790)대로 실제 설정 파일을 읽는다. 버전 값은 실행 파일의 상수(10.78, CD 10.72)다.
void StartConfiguration(netstorm::o::ConfigInterface& config, const std::filesystem::path& root, netstorm::o::OriginalEdition edition) {
    config.Registry().environment = Environment;
    netstorm::o::ConfigInterface::StartupOptions options;
    options.majorVersion = 10;
    options.minorVersion = edition == netstorm::o::OriginalEdition::Cd1072 ? 72 : 78;
    auto identity = Environment("IDENTITY");
    if (!identity || identity->empty()) identity = Environment("USER");
    options.identity = identity.value_or(std::string());
    options.installDir = std::filesystem::absolute(root).lexically_normal().string();
    config.Startup(options);
    // 원본은 시작할 때 `currentLanguage`의 용어표를 읽는다(FUN_00441db0).
    config.LoadLanguage(config.Configuration().Get("currentLanguage").value_or(std::string()));
}

// 설정에서 계산한 전투 팔레트 경로(`GamePalSpec`에 `battlePal`을 넣은 값)로 자산을 읽는다.
netstorm::client::GameAssets LoadAssets(const std::filesystem::path& root, netstorm::o::OriginalEdition edition) {
    const auto files = Files(root);
    netstorm::o::ConfigInterface config([&files](std::string_view path) { return files.TryRead(path); });
    StartConfiguration(config, root, edition);
    const auto palette = config.PathSpec("GamePalSpec", config.Configuration().Get("battlePal").value_or(std::string()));
    return netstorm::client::GameAssets(files, edition, palette);
}

// 타입 번호를 이름으로 적는다. 이름 없는 타입은 번호로 구분한다.
std::string TypeLabel(const netstorm::o::RiftTypeTable& types, int type) {
    if (type >= 0 && static_cast<std::size_t>(type) < types.Types().size()) {
        const auto& record = types.Types()[static_cast<std::size_t>(type)];
        // .type 타입은 파일 이름으로 적는다. 원본 구조체의 이름은 20글자를 넘으면 설명과 이어져 읽힌다.
        if (!record.assetName.empty()) return record.assetName;
        if (!record.name.empty()) return record.name;
    }
    return "#" + std::to_string(type);
}

// 전체 타입 표를 한 줄에 하나씩 적는다: 번호, 이름, 이름 해시, 플래그 1·2, 그룹, 종류, 목록 플래그, 발자국.
void DumpTypes(const netstorm::o::RiftTypeTable& types) {
    std::string text;
    char line[256];
    // 번호순으로 모든 타입을 적는다.
    for (std::size_t i = 0; i < types.Types().size(); ++i) {
        const auto& type = types.Types()[i];
        std::snprintf(line, sizeof line, "%zu\t%s\t%08x\t%08x\t%08x\t%d\t%s\t%x\t%x\t%d\t%d\n", i,
            type.name.empty() ? "-" : type.name.c_str(), netstorm::o::RiftTypeTable::NameHash(type.name), type.flags1, type.flags2,
            type.group, type.kind.empty() ? "-" : type.kind.c_str(), type.containerListFlags, type.contentListFlags, type.footX, type.footY);
        text += line;
    }
    WriteText(text);
}

// 내용물 목록을 한 줄로 적는다: (타입 수량 [lf=파일에서 읽은 목록 플래그] [중첩 목록]) ...
void AppendContents(std::string& text, const netstorm::o::RiftTypeTable& types, const std::vector<netstorm::o::FortContent>& items, bool complete) {
    text += complete ? "[" : "[!";
    // 항목을 저장 순서대로 적는다.
    for (const auto& item : items) {
        text += "(" + TypeLabel(types, item.type) + " " + std::to_string(item.quantity);
        if (item.listFlagsStored) text += " lf=" + std::to_string(item.listFlags);
        // container 타입은 비어 있어도 중첩 목록을 적는다.
        if ((types.Types()[static_cast<std::size_t>(item.type)].flags1 & netstorm::o::TypeFlag1::kContainer) != 0) {
            text += " ";
            AppendContents(text, types, item.contents, item.contentsComplete);
        }
        text += ")";
    }
    text += "]";
}

// 선택 필드를 `이름=값` 꼴로 붙인다.
template <typename Value>
void AppendField(std::string& text, const char* name, const std::optional<Value>& value) {
    if (value) text += std::string(" ") + name + "=" + std::to_string(static_cast<int>(*value));
}

// 청크 섹션 하나를 적는다. 청크 번호는 섹션 안의 저장 순서다.
void AppendChunkSection(std::string& text, std::string_view name, const netstorm::o::FortChunkSection& section, const netstorm::o::RiftTypeTable& types) {
    if (!section.present) return;
    text += "SEC " + std::string(name) + " v=" + std::to_string(section.version) + " chunks=" + std::to_string(section.chunks.size()) + "\n";
    // 청크마다 오브젝트를 저장 순서대로 적는다.
    for (std::size_t chunk = 0; chunk < section.chunks.size(); ++chunk) {
        // 빈 청크는 줄을 만들지 않는다.
        for (const auto& object : section.chunks[chunk].objects) {
            text += "O " + std::to_string(chunk) + " " + std::to_string(object.CellX()) + "," + std::to_string(object.CellY())
                + " " + std::to_string(object.storedType) + " " + TypeLabel(types, object.type);
            AppendField(text, "sh", object.shorthandOwner);
            AppendField(text, "frame", object.frame);
            AppendField(text, "qa", object.quantityByte);
            AppendField(text, "qb", object.quantityWord);
            AppendField(text, "bridge", object.bridgeFrame);
            AppendField(text, "legacy", object.legacyState);
            AppendField(text, "factory", object.factoryState);
            AppendField(text, "owner", object.owner);
            AppendField(text, "skip", object.islandByte);
            if (object.contents) { text += " contents="; AppendContents(text, types, object.contents->items, object.contents->complete); }
            text += "\n";
        }
    }
}

// 파일 하나의 구조 전체를 글로 적는다. 독립 Python 판독기의 결과와 줄 단위로 비교하는 데 쓴다.
std::string DescribeFort(std::string_view name, const netstorm::o::FortTemplate& fort, const netstorm::o::RiftTypeTable& types) {
    std::string text = "FORT " + std::string(name) + " flag=" + std::to_string(fort.flag) + " sections=" + std::to_string(fort.sections.size())
        + " trailing=" + std::to_string(fort.trailingBytes) + "\n";
    char line[128];
    if (fort.subscriber) {
        std::string hex;
        // 이름은 원본 바이트를 16진수로 적어 인코딩 차이를 피한다.
        for (const unsigned char c : fort.subscriber->name) { std::snprintf(line, sizeof line, "%02x", c); hex += line; }
        text += "SUB " + std::to_string(fort.subscriber->id) + " " + (hex.empty() ? "-" : hex) + "\n";
    }
    if (fort.money) { std::snprintf(line, sizeof line, "MONEY %08x\n", std::bit_cast<std::uint32_t>(*fort.money)); text += line; }
    text += "TYPES " + std::to_string(fort.typeHashes.size()) + "\n";
    if (!fort.Section("Technology").empty()) {
        text += "TECH " + std::to_string(fort.technology.items.size()) + " trailing=" + std::to_string(fort.technologyTrailingBytes) + " ";
        AppendContents(text, types, fort.technology.items, fort.technology.complete);
        text += "\n";
    }
    if (!fort.Section("Deck").empty()) {
        text += "DECK " + std::to_string(fort.deck.size());
        // 덱 항목: 타입 chance power 남은횟수.
        for (const auto& entry : fort.deck)
            text += " (" + TypeLabel(types, entry.type) + " " + std::to_string(entry.chance) + " " + std::to_string(entry.power) + " " + std::to_string(entry.remaining) + ")";
        text += "\n";
    }
    AppendChunkSection(text, "Chaff", fort.chaff, types);
    // 영역 섹션은 번호순이다.
    for (std::size_t i = 0; i < fort.territories.size(); ++i) {
        std::snprintf(line, sizeof line, "Terr%02zu", i);
        AppendChunkSection(text, line, fort.territories[i], types);
    }
    return text;
}

// 요새 하나의 요약: 섹션, 자원, 덱, 기술, 타입별 오브젝트 수.
void InspectFort(std::string_view name, const netstorm::o::FortTemplate& fort, const netstorm::o::RiftTypeTable& types) {
    std::printf("Fort: %s\n  sections: %zu, trailing bytes: %zu, flag: 0x%02x\n", std::string(name).c_str(), fort.sections.size(), fort.trailingBytes, fort.flag);
    if (fort.subscriber) std::printf("  subscriber: %u \"%s\"\n", fort.subscriber->id, fort.subscriber->name.c_str());
    if (fort.money) std::printf("  storm power: %g\n", static_cast<double>(*fort.money));
    std::printf("  type names: %zu, technology: %zu, deck: %zu\n", fort.typeHashes.size(), fort.technology.items.size(), fort.deck.size());
    // 섹션마다 타입별 개수를 센다.
    const auto report = [&types](std::string_view section, const netstorm::o::FortChunkSection& data) {
        if (!data.present) return;
        std::map<std::string, std::size_t> counts;
        std::size_t total = 0;
        // 청크와 오브젝트를 모두 훑는다.
        for (const auto& chunk : data.chunks) for (const auto& object : chunk.objects) { ++counts[TypeLabel(types, object.type)]; ++total; }
        std::printf("  %s: version %u, %zu chunks, %zu objects\n", std::string(section).c_str(), data.version, data.chunks.size(), total);
        // 이름순으로 출력한다.
        for (const auto& [type, count] : counts) std::printf("    %-24s %zu\n", type.c_str(), count);
    };
    report("Chaff", fort.chaff);
    char label[16];
    // 내용이 있는 영역만 출력한다.
    for (std::size_t i = 0; i < fort.territories.size(); ++i) { std::snprintf(label, sizeof label, "Terr%02zu", i); report(label, fort.territories[i]); }
}

// 요새 하나의 영역 배치를 적는다: 영역마다 레코드 값, 놓인 청크(y·x 순서), `TerrNN`의 청크 레코드 수.
std::string DescribeTerritories(std::string_view name, const netstorm::o::FortTemplate& fort, const netstorm::o::RiftTypeFrames& pieceFrames) {
    std::string text = "FORT " + std::string(name) + "\n";
    if (fort.territory.empty()) return text;
    netstorm::o::ChunkMap map;
    netstorm::o::IslandList islands;
    netstorm::o::IslandBuilder builder(map, islands, pieceFrames, 1);
    builder.PlaceAll(fort.territory);
    // 영역 번호순으로 적는다.
    for (int territory = 0; territory < netstorm::o::kTerritoryCount; ++territory) {
        const auto* record = &fort.territory[static_cast<std::size_t>(territory) * netstorm::o::kTerritoryRecordBytes];
        const auto chunks = netstorm::o::TerritoryChunks(map, islands, 1, territory);
        const auto& section = fort.territories[static_cast<std::size_t>(territory)];
        if (chunks.empty() && !section.present) continue;
        text += "T " + std::to_string(territory) + " shape=" + std::to_string(record[0] & 0x3f) + " dir=" + std::to_string(record[0] >> 6)
            + " flags=" + std::to_string(record[1]) + " pos=" + std::to_string(record[2] & 0xf) + "," + std::to_string(record[2] >> 4)
            + " stored=" + std::to_string(section.chunks.size()) + " chunks=";
        // 청크 좌표를 순회 순서대로 적는다.
        for (const auto& chunk : chunks) text += std::to_string(chunk.x) + "," + std::to_string(chunk.y) + map.At(chunk.x, chunk.y).side + ";";
        text += "\n";
    }
    return text;
}

// `.fort` 검사 명령.
int RunFortCommand(const std::string& command, std::vector<std::string> arguments) {
    const auto edition = TakeEdition(arguments);
    if (arguments.empty()) throw std::invalid_argument("Missing game directory");
    const std::filesystem::path root = arguments[0];
    const auto files = Files(root);
    const auto assets = LoadAssets(root, edition);
    const auto& types = assets.TypeTable();
    if (command == "--dump-types" && arguments.size() == 1) { DumpTypes(types); return 0; }
    if (command == "--inspect-fort" && arguments.size() == 2) {
        std::string path = arguments[1];
        // 경로 구분자나 확장자가 없으면 미션 이름으로 보고 설정의 fortSpec으로 경로를 만든다.
        if (path.find_first_of("\\/.") == std::string::npos) {
            netstorm::o::ConfigInterface config([&files](std::string_view file) { return files.TryRead(file); });
            StartConfiguration(config, root, edition);
            path = config.PathSpec("fortSpec", path);
        }
        InspectFort(path, netstorm::o::FortTemplate::Parse(files.Read(path), types), types);
        return 0;
    }
    if (command == "--inspect-mission" && arguments.size() == 2) {
        netstorm::o::ConfigInterface config([&files](std::string_view file) { return files.TryRead(file); });
        StartConfiguration(config, root, edition);
        netstorm::client::MissionScript mission(config, arguments[1]);
        std::printf("Mission: %s\n  script: %s (%s)\n", mission.Name().c_str(), mission.ScriptPath().c_str(), mission.Loaded() ? "loaded" : "missing");
        std::printf("  mission type: %s\n  fort: %s -> %s\n", mission.MissionType().c_str(), mission.FortName().c_str(), mission.FortPath().c_str());
        // 머리 값: 플레이어 설정과 AI 2~8의 설정. 스크립트에 있는 것만 적는다.
        std::vector<std::string> keys{"title", "missionNumber", "moreGeysers", "myStartMoney", "myTech", "myAllyList"};
        // AI 번호마다 같은 이름 꼴의 키가 있다.
        for (int ai = 2; ai <= 8; ++ai)
            // 키 이름은 "ai" + 번호 + 접미어다.
            for (const char* suffix : {"Name", "Tech", "StartMoney", "AllyList", "color", "Ability"})
                keys.push_back("ai" + std::to_string(ai) + suffix);
        // 찾은 키만 `키 = 값`으로 적는다.
        for (const auto& key : keys) if (const auto value = mission.Get(key)) std::printf("  %s = %s\n", key.c_str(), value->c_str());
        const auto fortPath = mission.FortPath();
        if (const auto bytes = files.TryRead(fortPath)) InspectFort(fortPath, netstorm::o::FortTemplate::Parse(*bytes, types), types);
        else std::printf("Fort: %s (missing)\n", fortPath.c_str());
        return 0;
    }
    if ((command == "--dump-forts" || command == "--dump-territories") && arguments.size() == 1) {
        const bool territories = command == "--dump-territories";
        const auto pieceFrames = assets.Find("puzzlePiece").definition.FrameTable();
        // 파일 하나를 고른 형식으로 적는다.
        const auto describe = [&](std::string_view name, const netstorm::o::FortTemplate& fort) {
            return territories ? DescribeTerritories(name, fort, pieceFrames) : DescribeFort(name, fort, types);
        };
        std::string text;
        std::size_t count = 0;
        // 데이터 폴더의 낱개 파일을 이름순으로 읽는다(폴더 이름의 대소문자는 판본마다 다르다).
        for (const auto* folder : {"d", "D"}) {
            const auto directory = root / folder;
            if (!std::filesystem::is_directory(directory)) continue;
            std::map<std::string, std::filesystem::path> found;
            // 확장자는 대소문자를 구분하지 않는다.
            for (const auto& entry : std::filesystem::directory_iterator(directory))
                if (entry.is_regular_file() && netstorm::o::AsciiLower(entry.path().extension().string()) == ".fort")
                    found[entry.path().filename().string()] = entry.path();
            // 파일마다 구조를 적는다. 형식 오류는 그 파일의 줄에 남기고 계속한다.
            for (const auto& [name, path] : found) {
                try { text += describe(name, netstorm::o::FortTemplate::Parse(netstorm::o::ReadFileBytes(path), types)); }
                catch (const std::exception& error) { text += "FORT " + name + " ERROR " + error.what() + "\n"; }
                ++count;
            }
            break; // Windows에서는 d와 D가 같은 폴더다.
        }
        // 아카이브 안의 .fort.
        for (const auto* archiveName : {"netstorm.tarc"}) {
            const auto archive = netstorm::o::TaffArchive::Open(root / archiveName);
            // 디렉터리의 원본 순서대로 읽는다.
            for (const auto& entry : archive.Entries()) {
                if (!netstorm::o::AsciiLower(entry.name).ends_with(".fort")) continue;
                const auto name = "tarc:" + entry.name;
                try { text += describe(name, netstorm::o::FortTemplate::Parse(archive.Read(entry.name), types)); }
                catch (const std::exception& error) { text += "FORT " + name + " ERROR " + error.what() + "\n"; }
                ++count;
            }
        }
        WriteText(text);
        std::fprintf(stderr, "Dumped %zu fort files\n", count);
        return 0;
    }
    throw std::invalid_argument("Invalid fort command arguments");
}

// 원본 방식의 창을 띄운다. Renderer를 옮기기 전까지는 검사용 화면(InspectView)이 내용을 그린다.
int RunClient(std::vector<std::string> arguments) {
    netstorm::client::ClientOptions options;
    options.edition = TakeEdition(arguments);
    if (arguments.empty()) throw std::invalid_argument("Missing game directory");
    options.gameDirectory = arguments[0];
    std::string view;
    // 나머지 인자: 원본 명령줄의 "window"에 해당하는 것과 검사용 옵션.
    for (std::size_t i = 1; i < arguments.size(); ++i) {
        const auto& argument = arguments[i];
        const auto value = [&]() -> const std::string& {
            if (i + 1 >= arguments.size()) throw std::invalid_argument("Missing value for " + argument);
            return arguments[++i];
        };
        if (argument == "--window") options.forceWindow = true;
        else if (argument == "--frames") options.frameLimit = std::stoull(value());
        else if (argument == "--screenshot") options.screenshot = value();
        else if (argument == "--set") options.settings = value();
        else if (argument == "--view") view = value();
        else throw std::invalid_argument("Unknown --run option: " + argument);
    }
    netstorm::client::Client client(std::move(options));
    std::optional<netstorm::app::InspectView> inspect;
    if (!view.empty()) inspect.emplace(client, view);
    return client.Run();
}

// 설정 검사 명령: 버퍼 전체, 키 조회, 경로 지정값, 값 쓰기 뒤 저장 내용.
int RunConfigCommand(const std::string& command, std::vector<std::string> arguments) {
    const auto edition = TakeEdition(arguments);
    if (arguments.empty()) throw std::invalid_argument("Missing game directory");
    const std::filesystem::path root = arguments[0];
    const auto files = Files(root);
    netstorm::o::ConfigInterface config([&files](std::string_view path) { return files.TryRead(path); });
    StartConfiguration(config, root, edition);
    if (command == "--config-dump" && arguments.size() == 1) { WriteText(config.Configuration().Text()); return 0; }
    if (command == "--config-get" && arguments.size() == 2) {
        const auto value = config.Configuration().Get(arguments[1]);
        if (!value) return 2;
        WriteText(*value); return 0;
    }
    if (command == "--config-spec" && arguments.size() >= 2 && arguments.size() <= 5) {
        std::optional<std::string_view> values[3];
        // 준 인자만 local.1~local.3에 넣는다.
        for (std::size_t i = 2; i < arguments.size(); ++i) values[i-2] = arguments[i];
        WriteText(config.PathSpec(arguments[1], values[0], values[1], values[2])); return 0;
    }
    if (command == "--config-save" && arguments.size() >= 2) {
        const auto output = std::filesystem::absolute(arguments[1]).lexically_normal();
        const auto protectedRoot = std::filesystem::absolute(root).lexically_normal();
        // 원본 게임 폴더 안에는 쓰지 않는다(AGENTS.md: 원본 파일 수정 금지).
        const auto relative = output.lexically_relative(protectedRoot);
        if (!relative.empty() && *relative.begin() != "..") throw std::invalid_argument("Refusing to write inside the game directory");
        // `키=값` 인자를 차례로 쓴다.
        for (std::size_t i = 2; i < arguments.size(); ++i) {
            const auto equals = arguments[i].find('=');
            if (equals == std::string::npos) throw std::invalid_argument("Expected key=value");
            config.Set(arguments[i].substr(0, equals), arguments[i].substr(equals + 1));
        }
        const auto bytes = netstorm::o::EncodeConfigFile(config.SaveText("options.cfg"));
        netstorm::platform::WriteFileBytes(output, bytes);
        std::printf("Saved %zu bytes -> %s\n", bytes.size(), output.string().c_str());
        return 0;
    }
    throw std::invalid_argument("Invalid config command arguments");
}
}

// 게임 프로세스를 시작하지 않고 복원된 C++ 데이터 모듈을 실행한다.
int main(int argc, char** argv) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) { PrintBuildInfo(); return 0; }
        const std::string command = argv[1];
        if (command.starts_with("--config-")) return RunConfigCommand(command, std::vector<std::string>(argv + 2, argv + argc));
        if (command == "--run") return RunClient(std::vector<std::string>(argv + 2, argv + argc));
        if (command == "--dump-types" || command == "--inspect-fort" || command == "--dump-forts" || command == "--inspect-mission"
            || command == "--dump-territories")
            return RunFortCommand(command, std::vector<std::string>(argv + 2, argv + argc));
        if (argc == 3 && command == "--inspect-data") { InspectData(argv[2]); return 0; }
        if (argc == 3 && command == "--dump-archive") { DumpArchive(argv[2]); return 0; }
        if ((argc == 3 || argc == 4) && (command == "--inspect-assets" || command == "--dump-assets")) {
            const auto assets = LoadAssets(argv[2], Edition(argc, argv, 3));
            if (command == "--inspect-assets") InspectAssets(assets); else DumpAssets(assets);
            return 0;
        }
        if ((argc == 7 || argc == 8) && command == "--export-frame") {
            const auto assets = LoadAssets(argv[2], Edition(argc, argv, 7));
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
