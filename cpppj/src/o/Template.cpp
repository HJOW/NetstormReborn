// 원본: Datamanager.cpp 0044d380 ↔ CD 0044c750(섹션 읽기), Template.cpp 004bd800 ↔ CD 00427e00(타입 번호 변환),
//       004bdc60 ↔ CD 00428320(오브젝트), 004bd130 ↔ CD 00426dd0(내용물), 004be160 ↔ CD 004288a0(청크),
//       004bf190 ↔ CD 00429670(덱), 004bd450(Subscriber), 00483300(Money).
// 범위: 파일을 구조로 읽는 데까지. 원본은 읽으면서 곧바로 오브젝트를 만들어 월드에 놓는다 — 그 부분과
//       `State`·`Mission`·`CoreData`·`Badges`·`CompressedData` 섹션의 해석은 옮기지 않았다(원시 바이트로 보존).
// 검증: 원본 `.fort` 전체를 기존 Python 판독기와 필드 단위로 대조한다(tools/cpp_fort_smoke.py).
#include "o/Template.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 파일 첫 바이트('F'). 원본은 다르면 파일을 버린다.
constexpr std::uint8_t kFortMagic = 0x46;
// 청크 레코드의 시작 표지('c').
constexpr std::uint8_t kChunkMark = 99;
// 타입 바이트가 이 값 이상이면 다리 약식 표기다(소유자 = 0xff - 값).
constexpr std::uint8_t kBridgeShorthandMin = 0xf6;
// 섹션 길이 필드의 크기. 길이에는 이 2바이트가 포함된다.
constexpr std::size_t kSectionLengthBytes = 2;
// `Territory` 섹션의 고정 길이(6바이트 × 20, 원본 004bd660).
constexpr std::size_t kTerritoryBytes = 0x78;
// 소유자 바이트의 유효 범위 끝(플레이어 1~8).
constexpr int kMaxOwner = 8;

// 섹션 이름(패치 005146b0의 15개 + "Terr%02d" 20개).
constexpr std::array<std::string_view, FortTemplate::kSectionCount> kSectionNames{
    "Subscriber", "State", "Mission", "CoreData", "Chaff", "Badges", "Territory", "TypeNames", "CompressedData",
    "Technology", "Money", "Deck", "Reserved2", "Reserved3", "Reserved4",
    "Terr00", "Terr01", "Terr02", "Terr03", "Terr04", "Terr05", "Terr06", "Terr07", "Terr08", "Terr09",
    "Terr10", "Terr11", "Terr12", "Terr13", "Terr14", "Terr15", "Terr16", "Terr17", "Terr18", "Terr19"};

// 원본 memstream: 리틀 엔디언으로 읽고 끝을 넘으면 예외를 던진다(원본은 assert "pos<=end").
class Stream {
public:
    // 섹션 이름은 오류 메시지에만 쓴다.
    Stream(std::span<const std::uint8_t> bytes, std::string_view name) : bytes_(bytes), name_(name) {}
    // 1바이트.
    std::uint8_t U8() { Require(1); return bytes_[position_++]; }
    // 부호 없는 16비트.
    std::uint16_t U16() {
        Require(2);
        const auto value = static_cast<std::uint16_t>(bytes_[position_] | (bytes_[position_ + 1] << 8));
        position_ += 2;
        return value;
    }
    // 부호 없는 32비트.
    std::uint32_t U32() { const std::uint32_t low = U16(); return low | (static_cast<std::uint32_t>(U16()) << 16); }
    // 남은 바이트 수.
    std::size_t Remaining() const { return bytes_.size() - position_; }
    // 형식 오류를 섹션 이름·위치와 함께 알린다.
    [[noreturn]] void Fail(std::string_view message) const {
        throw std::runtime_error("fort " + std::string(name_) + " @" + std::to_string(position_) + ": " + std::string(message));
    }
private:
    // 읽을 만큼 남았는지 확인한다.
    void Require(std::size_t count) const { if (Remaining() < count) Fail("pos<=end"); }
    std::span<const std::uint8_t> bytes_;
    std::string_view name_;
    std::size_t position_{};
};

// 변환한 타입 번호의 타입 정보를 찾는다. 표 밖의 번호는 형식 오류다.
const RiftTypeRecord& TypeOf(const RiftTypeTable& types, int type, const Stream& stream) {
    if (type < 0 || static_cast<std::size_t>(type) >= types.Types().size()) stream.Fail("type number outside the type table");
    return types.Types()[static_cast<std::size_t>(type)];
}

// 원본 004bd130. containerListFlags는 그릇 타입의 +0xfc 값이다.
FortContentList ReadContents(Stream& stream, const FortTemplate& fort, const RiftTypeTable& types, std::uint32_t containerListFlags) {
    FortContentList list;
    // 개수 바이트만큼 항목을 읽는다.
    for (auto count = stream.U8(); count != 0; --count) {
        FortContent item;
        item.storedType = stream.U8();
        item.type = fort.ConvertType(item.storedType);
        if (item.type == 0) { list.complete = false; break; } // 원본은 여기서 읽기를 그만두고 0을 돌려준다.
        const auto& type = TypeOf(types, item.type, stream);
        if ((type.flags1 & TypeFlag1::kSaveQA) != 0) item.quantity = stream.U8();
        if ((type.flags1 & TypeFlag1::kSaveQB) != 0) item.quantity = std::bit_cast<std::int16_t>(stream.U16());
        item.listFlags = type.contentListFlags;
        if (item.listFlags == 0) item.listFlags = containerListFlags;
        if (item.listFlags == 0) { item.listFlags = stream.U8(); item.listFlagsStored = true; }
        if ((type.flags1 & TypeFlag1::kContainer) != 0) {
            auto nested = ReadContents(stream, fort, types, type.containerListFlags);
            item.contents = std::move(nested.items);
            item.contentsComplete = nested.complete;
            list.items.push_back(std::move(item));
            if (!list.items.back().contentsComplete) { list.complete = false; break; } // 원본: 중첩이 실패하면 0.
            continue;
        }
        list.items.push_back(std::move(item));
    }
    return list;
}

// 원본 004bdc60의 읽기 부분.
FortObject ReadObject(Stream& stream, const FortTemplate& fort, const RiftTypeTable& types, std::uint8_t version) {
    FortObject object;
    object.position = stream.U8();
    object.storedType = stream.U8();
    if (object.storedType < kBridgeShorthandMin) object.type = fort.ConvertType(object.storedType);
    else {
        object.shorthandOwner = 0xff - object.storedType;
        object.type = types.BridgeType();
    }
    if (object.type == 0) {
        if (stream.U8() != 0) stream.Fail("num == 0"); // 원본 assert.
        return object;
    }
    if (object.type == types.IslandType()) {
        object.islandByte = stream.U8();
        return object;
    }
    const auto& type = TypeOf(types, object.type, stream);
    if ((type.flags1 & TypeFlag1::kSaveFrame) != 0) object.frame = stream.U8();
    if ((type.flags1 & TypeFlag1::kSaveQA) != 0) object.quantityByte = stream.U8();
    if ((type.flags1 & TypeFlag1::kSaveQB) != 0) object.quantityWord = std::bit_cast<std::int16_t>(stream.U16());
    if ((type.flags2 & TypeFlag2::kBridge) != 0) object.bridgeFrame = stream.U8();
    // 버전 0: 원본 FUN_004ad0a0은 `or edx, 0x4000000` 뒤의 분기 때문에 항상 참이라 모든 타입에서 1바이트를 읽는다.
    if (version == 0) object.legacyState = stream.U8();
    if (version > 1 && (type.flags2 & TypeFlag2::kFactory) != 0) object.factoryState = stream.U8();
    if (version > 0 && (type.flags2 & TypeFlag2::kOwnerSavedMask) != 0) object.owner = stream.U8();
    if ((type.flags1 & TypeFlag1::kContainer) != 0) object.contents = ReadContents(stream, fort, types, type.containerListFlags);
    return object;
}

// `Chaff`·`TerrNN`: 버전 바이트 뒤로 섹션이 끝날 때까지 청크 레코드를 읽는다.
// 원본은 월드 청크를 y 바깥·x 안쪽으로 돌며 해당 청크마다 레코드 하나를 읽는다. 어느 청크인지는 영역 배치가 정한다.
FortChunkSection ReadChunkSection(std::span<const std::uint8_t> bytes, std::string_view name, const FortTemplate& fort, const RiftTypeTable& types) {
    FortChunkSection section;
    if (bytes.empty()) return section;
    Stream stream(bytes, name);
    section.version = stream.U8();
    section.present = stream.Remaining() != 0;
    // 청크 레코드마다 표지와 오브젝트 수가 온다.
    while (stream.Remaining() != 0) {
        if (stream.U8() != kChunkMark) stream.Fail("data.readByte() == 99"); // 원본 assert.
        FortChunk chunk;
        // 빈 레코드(타입 0)도 개수에 들어 있으므로 그대로 보존한다.
        for (auto count = stream.U16(); count != 0; --count) chunk.objects.push_back(ReadObject(stream, fort, types, section.version));
        section.chunks.push_back(std::move(chunk));
    }
    return section;
}
}

// 0이거나 8보다 크면 1로 바꾼다(원본 004bdc60).
std::optional<int> FortObject::NormalizedOwner() const {
    if (!owner) return std::nullopt;
    return *owner == 0 || *owner > kMaxOwner ? 1 : static_cast<int>(*owner);
}
// 저장 순서의 섹션 이름.
std::span<const std::string_view> FortTemplate::SectionNames() { return kSectionNames; }
// 이름은 대소문자를 구분하지 않는다(원본 stricmp).
std::span<const std::uint8_t> FortTemplate::Section(std::string_view name) const {
    // 이름 표의 위치가 곧 파일 안의 섹션 순서다.
    for (std::size_t i = 0; i < kSectionNames.size() && i < sections.size(); ++i)
        if (AsciiLower(kSectionNames[i]) == AsciiLower(name)) return sections[i];
    return {};
}
// 원본은 0xf6 미만의 번호만 변환표로 바꾼다. 변환표가 없으면(섹션이 비었으면) 그대로 쓴다.
int FortTemplate::ConvertType(std::uint8_t storedType) const {
    if (storedType >= kBridgeShorthandMin || !hasTypeConversion) return storedType;
    return typeConversion[storedType];
}
// 원본은 섹션을 이름 목록 순서로 35개 읽는다. 파일이 먼저 끝나면 거기서 멈춘다(새 구현의 보호 처리).
FortTemplate FortTemplate::Parse(std::span<const std::uint8_t> bytes, const RiftTypeTable& types) {
    if (bytes.size() < 2 || bytes[0] != kFortMagic) throw std::runtime_error("fort: first byte is not 0x46");
    FortTemplate fort;
    fort.flag = bytes[1];
    std::size_t position = 2;
    // 길이(부호 있는 16비트, 길이 필드 포함)와 내용이 이어진다.
    while (fort.sections.size() < kSectionCount && bytes.size() - position >= kSectionLengthBytes) {
        const auto length = static_cast<std::size_t>(bytes[position] | (bytes[position + 1] << 8));
        if (length < kSectionLengthBytes || length > 0x7fff || length > bytes.size() - position) break;
        const auto body = bytes.subspan(position + kSectionLengthBytes, length - kSectionLengthBytes);
        fort.sections.emplace_back(body.begin(), body.end());
        position += length;
    }
    fort.trailingBytes = bytes.size() - position;

    // Subscriber: ID 4바이트 + 문자열(길이 16비트 + 글자).
    if (const auto section = fort.Section("Subscriber"); !section.empty()) {
        Stream stream(section, "Subscriber");
        FortSubscriber subscriber;
        subscriber.id = stream.U32();
        // 이름은 길이만큼의 바이트다.
        for (auto length = stream.U16(); length != 0; --length) subscriber.name.push_back(static_cast<char>(stream.U8()));
        fort.subscriber = std::move(subscriber);
    }
    // Money: 32비트 실수.
    if (const auto section = fort.Section("Money"); section.size() >= 4) {
        Stream stream(section, "Money");
        fort.money = std::bit_cast<float>(stream.U32());
    }
    // Territory: 120바이트를 그대로 둔다. 해석은 영역 배치 단계에서 한다.
    if (const auto section = fort.Section("Territory"); section.size() >= kTerritoryBytes)
        fort.territory.assign(section.begin(), section.begin() + static_cast<std::ptrdiff_t>(kTerritoryBytes));
    // TypeNames: 원본 004bd800. 현재 타입마다 이름 해시가 같은 첫 파일 번호에 자기 번호를 적는다.
    if (const auto section = fort.Section("TypeNames"); !section.empty()) {
        Stream stream(section, "TypeNames");
        // 개수 바이트 뒤로 해시가 이어진다.
        for (auto count = stream.U8(); count != 0; --count) fort.typeHashes.push_back(stream.U32());
        fort.hasTypeConversion = true;
        const auto current = types.Types();
        // 같은 해시의 현재 타입이 여럿이면 번호가 큰 것이 남는다(이름 없는 타입의 해시 0이 그렇다).
        for (std::size_t type = 0; type < current.size(); ++type) {
            const auto hash = RiftTypeTable::NameHash(current[type].name);
            // 파일의 번호 가운데 해시가 같은 첫 번호만 고친다.
            for (std::size_t stored = 0; stored < fort.typeHashes.size(); ++stored) {
                if (fort.typeHashes[stored] != hash) continue;
                if (stored < fort.typeConversion.size()) fort.typeConversion[stored] = static_cast<std::uint8_t>(type);
                break;
            }
        }
    }
    // Technology: 플레이어를 그릇으로 한 내용물 목록이다. 그 타입(PlayerSquid)의 +0xfc는 0이라 목록 플래그를 파일에서 읽는다.
    if (const auto section = fort.Section("Technology"); !section.empty()) {
        Stream stream(section, "Technology");
        fort.technology = ReadContents(stream, fort, types, 0);
        fort.technologyTrailingBytes = stream.Remaining();
    }
    // Deck: 개수 + (타입, chance, power, 남은 횟수) 4바이트 항목.
    if (const auto section = fort.Section("Deck"); !section.empty()) {
        Stream stream(section, "Deck");
        // 변환한 타입이 0인 항목도 보존한다(원본은 덱에 넣지 않는다).
        for (auto count = stream.U8(); count != 0; --count) {
            FortDeckEntry entry;
            entry.storedType = stream.U8();
            entry.type = fort.ConvertType(entry.storedType);
            entry.chance = stream.U8();
            entry.power = std::bit_cast<std::int8_t>(stream.U8());
            entry.remaining = stream.U8();
            fort.deck.push_back(entry);
        }
    }
    fort.chaff = ReadChunkSection(fort.Section("Chaff"), "Chaff", fort, types);
    // 영역 섹션 20개.
    for (std::size_t i = 0; i < kTerritoryCount; ++i) {
        const auto name = kSectionNames[kSectionCount - kTerritoryCount + i];
        fort.territories[i] = ReadChunkSection(fort.Section(name), name, fort, types);
    }
    return fort;
}
}
