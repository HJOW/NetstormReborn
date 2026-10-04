// 새로 쓰는 코드(원본에 없음): 검사용 화면. 원본 Renderer의 동작을 옮긴 것이 아니다.
#include "app/InspectView.h"
#include "client/Mission.h"
#include "client/VFXDraw.h"
#include "o/ChunkMap.h"
#include "o/Islandbuilder.h"
#include "o/Template.h"
#include <algorithm>
#include <span>

namespace netstorm::app {
namespace {
// 월드 칸 하나의 화면 크기(원본 화면 대조로 확인된 값: 가로 16, 세로 11 — docs/formats/fort.md 4절).
constexpr int kCellPixelsX = 16;
constexpr int kCellPixelsY = 11;
// 타입 표 화면의 칸 크기와 한 줄의 칸 수.
constexpr int kTypeCellWidth = 64;
constexpr int kTypeCellHeight = 96;
// 화살표 키를 누르고 있을 때 프레임마다 움직이는 픽셀 수.
constexpr int kScrollStep = 12;
// 바탕색 팔레트 번호(0 = 검정, 원본이 고정하는 항목).
constexpr std::uint8_t kBackground = 0;
// 검사 화면에서 영역의 소유 플레이어로 쓰는 번호(요새 파일은 한 플레이어의 영역 배치를 담는다).
constexpr int kOwner = 1;
// 영역 패턴을 그리는 타입의 파일 이름(원본 DAT_00541204 = 107 = 70 + 37).
constexpr const char* kPieceType = "puzzlePiece";
// Windows 가상 키 코드(화살표)와 Esc 글자.
constexpr std::uint32_t kKeyLeft = 0x25, kKeyUp = 0x26, kKeyRight = 0x27, kKeyDown = 0x28;
constexpr std::uint32_t kEscapeCharacter = 0x1b;
}

// 임시 연결점에 이 화면의 함수를 건다.
InspectView::InspectView(netstorm::client::Client& client, std::string view) : view_(std::move(view)) {
    client.ready = [this](netstorm::client::Client& c) { Ready(c); };
    client.draw = [this](netstorm::client::Client& c, std::uint8_t* buffer) { Draw(c, buffer); };
    client.input = [this](netstorm::client::Client& c) { return Input(c); };
}

// 미션이면: 스크립트 → 요새 경로 → 요새 읽기 → 영역 배치 → 오브젝트의 월드 칸 좌표.
void InspectView::Ready(netstorm::client::Client& client) {
    if (view_ == "types") return;
    const auto& assets = client.Assets();
    const auto& types = assets.TypeTable();
    netstorm::client::MissionScript mission(client.Configuration(), view_);
    const auto fort = netstorm::o::FortTemplate::Parse(client.Files().Read(mission.FortPath()), types);
    netstorm::o::ChunkMap map;
    netstorm::o::IslandList islands;
    const auto pieceFrames = assets.Find(kPieceType).definition.FrameTable();
    netstorm::o::IslandBuilder builder(map, islands, pieceFrames, kOwner);
    if (!fort.territory.empty()) builder.PlaceAll(fort.territory);
    // 섹션의 청크마다 오브젝트를 월드 칸 좌표로 옮긴다.
    const auto place = [&](const netstorm::o::FortChunkSection& section, const std::vector<netstorm::o::ChunkCoordinate>& chunks) {
        // 청크 레코드와 월드 청크는 같은 순서다.
        for (std::size_t i = 0; i < section.chunks.size() && i < chunks.size(); ++i) {
            // 그 청크의 오브젝트.
            for (const auto& object : section.chunks[i].objects) {
                if (object.type < netstorm::o::kFirstAssetTypeNumber || object.type == types.IslandType()) continue;
                const auto asset = static_cast<std::size_t>(object.type - netstorm::o::kFirstAssetTypeNumber);
                if (asset >= assets.Types().size()) continue;
                const auto& definition = assets.Types()[asset].definition;
                // 저장된 프레임(다리는 클러스터 번호)이 있으면 그것을, 없으면 타입의 기본 프레임을 쓴다.
                std::size_t frame = static_cast<std::size_t>(std::max(definition.specialFrames.defaultFrame, 0));
                if (object.bridgeFrame) frame = *object.bridgeFrame;
                else if (object.frame) frame = *object.frame;
                if (frame >= definition.clusters.size()) frame = 0;
                placed_.push_back({chunks[i].x * netstorm::o::kCellsPerChunk + object.CellX(),
                                   chunks[i].y * netstorm::o::kCellsPerChunk + object.CellY(), asset, frame});
            }
        }
    };
    // Chaff: 청크 번호 i → (i % 16, i / 16).
    std::vector<netstorm::o::ChunkCoordinate> all;
    // 월드 전체를 y 바깥, x 안쪽으로.
    for (int i = 0; i < netstorm::o::kChunkMapSide * netstorm::o::kChunkMapSide; ++i)
        all.push_back({i % netstorm::o::kChunkMapSide, i / netstorm::o::kChunkMapSide});
    place(fort.chaff, all);
    // 영역 섹션: 그 영역이 차지한 청크를 y·x 순서로.
    for (int territory = 0; territory < netstorm::o::kTerritoryCount; ++territory)
        place(fort.territories[static_cast<std::size_t>(territory)], netstorm::o::TerritoryChunks(map, islands, kOwner, territory));
    // 아래쪽 것이 위에 오도록 y, x 순으로 그린다(원본의 그리기 순서가 아니다).
    std::stable_sort(placed_.begin(), placed_.end(), [](const Placed& a, const Placed& b) {
        return a.cellY != b.cellY ? a.cellY < b.cellY : a.cellX < b.cellX; });
    if (placed_.empty()) return;
    // 처음에는 오브젝트들의 가운데를 화면 가운데에 둔다.
    int minX = placed_[0].cellX, maxX = minX, minY = placed_[0].cellY, maxY = minY;
    // 놓인 범위를 구한다.
    for (const auto& item : placed_) {
        minX = std::min(minX, item.cellX); maxX = std::max(maxX, item.cellX);
        minY = std::min(minY, item.cellY); maxY = std::max(maxY, item.cellY);
    }
    scrollX_ = (minX + maxX) * kCellPixelsX / 2 - client.GetScreen().Width() / 2;
    scrollY_ = (minY + maxY) * kCellPixelsY / 2 - client.GetScreen().Height() / 2;
}

// 화면을 바탕색으로 지우고 고른 내용을 그린다.
void InspectView::Draw(netstorm::client::Client& client, std::uint8_t* buffer) {
    auto& screen = client.GetScreen();
    screen.FillRect({0, 0, screen.Width(), screen.Height()}, kBackground);
    if (view_ == "types") DrawTypes(client, buffer);
    else DrawFort(client, buffer);
}

// 타입마다 기본 프레임(레이어 0)을 칸 가운데에 그린다.
void InspectView::DrawTypes(netstorm::client::Client& client, std::uint8_t* buffer) {
    auto& screen = client.GetScreen();
    const auto& assets = client.Assets();
    const std::span<std::uint8_t> pixels(buffer, static_cast<std::size_t>(screen.Pitch()) * static_cast<std::size_t>(screen.Height()));
    const int columns = std::max(screen.Width() / kTypeCellWidth, 1);
    // 로딩 순서대로 왼쪽 위부터 채운다.
    for (std::size_t i = 0; i < assets.Types().size(); ++i) {
        const auto& type = assets.Types()[i];
        const auto& frames = assets.Shapes().Blocks()[type.block].frames;
        const auto index = static_cast<std::size_t>(std::max(type.definition.specialFrames.defaultFrame, 0));
        if (index >= frames.size() || frames[index].IsSpecial()) continue;
        const int cellX = static_cast<int>(i) % columns * kTypeCellWidth - scrollX_;
        const int cellY = static_cast<int>(i) / columns * kTypeCellHeight - scrollY_;
        const auto& rect = frames[index].rect;
        // 그림 상자의 가운데가 칸 가운데에 오게 기준점을 정한다. 칸 밖은 자른다.
        // DrawShape의 좌표는 pane 원점 기준이므로(원본 VFX 규칙) 칸 안의 좌표를 넘긴다.
        const netstorm::client::ShapeRect pane{cellX, cellY, cellX + kTypeCellWidth - 1, cellY + kTypeCellHeight - 1};
        netstorm::client::DrawShape(pixels, screen.Pitch(), screen.Height(), assets.Shapes(), type.block, index,
            kTypeCellWidth / 2 - (rect.left + rect.right) / 2, kTypeCellHeight / 2 - (rect.top + rect.bottom) / 2, pane);
    }
}

// 오브젝트의 칸 좌표를 화면 점으로 바꿔 그 점에 그림의 기준점을 둔다.
void InspectView::DrawFort(netstorm::client::Client& client, std::uint8_t* buffer) {
    auto& screen = client.GetScreen();
    const auto& assets = client.Assets();
    const std::span<std::uint8_t> pixels(buffer, static_cast<std::size_t>(screen.Pitch()) * static_cast<std::size_t>(screen.Height()));
    const netstorm::client::ShapeRect pane{0, 0, screen.Width() - 1, screen.Height() - 1};
    // 정렬해 둔 순서대로 그린다.
    for (const auto& item : placed_) {
        const auto& type = assets.Types()[item.asset];
        const auto& frames = assets.Shapes().Blocks()[type.block].frames;
        if (item.frame >= frames.size() || frames[item.frame].IsSpecial()) continue;
        netstorm::client::DrawShape(pixels, screen.Pitch(), screen.Height(), assets.Shapes(), type.block, item.frame,
            item.cellX * kCellPixelsX - scrollX_, item.cellY * kCellPixelsY - scrollY_, pane);
    }
}

// 큐의 사건을 모두 꺼낸다. 화살표 키는 눌려 있는 동안 화면을 옮긴다.
bool InspectView::Input(netstorm::client::Client& client) {
    bool quit = false;
    // 쌓인 사건을 비운다.
    while (!client.Input().Empty()) {
        const auto event = client.Input().Pop(false);
        if ((event.code & 0xffffu) == kEscapeCharacter && (event.code & netstorm::client::InputCode::kRelease) == 0) quit = true;
    }
    if (!client.Active()) return quit;
    // 키가 눌려 있으면 뗌 표시가 없다.
    const auto held = [&client](std::uint32_t key) {
        return (client.Poll(key << netstorm::client::InputCode::kVirtualKeyShift).code & netstorm::client::InputCode::kRelease) == 0;
    };
    if (held(kKeyLeft)) scrollX_ -= kScrollStep;
    if (held(kKeyRight)) scrollX_ += kScrollStep;
    if (held(kKeyUp)) scrollY_ -= kScrollStep;
    if (held(kKeyDown)) scrollY_ += kScrollStep;
    return quit;
}
}
