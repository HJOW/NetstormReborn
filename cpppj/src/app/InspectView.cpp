// 새로 쓰는 코드(원본에 없음): 원본 Renderer 기반에 정적 검사 장면을 공급한다.
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
    client.input = [this](netstorm::client::Client& c) { return Input(c); };
}

// 미션이면: 스크립트 → 요새 경로 → 요새 읽기 → 영역 배치 → 오브젝트의 월드 칸 좌표.
void InspectView::Ready(netstorm::client::Client& client) {
    if (view_ == "types" || view_ == "fonts") { BuildScene(client); client.ShowScene(); return; }
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
    if (placed_.empty()) { BuildScene(client); client.ShowScene(); return; }
    // 처음에는 오브젝트들의 가운데를 화면 가운데에 둔다.
    int minX = placed_[0].cellX, maxX = minX, minY = placed_[0].cellY, maxY = minY;
    // 놓인 범위를 구한다.
    for (const auto& item : placed_) {
        minX = std::min(minX, item.cellX); maxX = std::max(maxX, item.cellX);
        minY = std::min(minY, item.cellY); maxY = std::max(maxY, item.cellY);
    }
    scrollX_ = (minX + maxX) * kCellPixelsX / 2 - client.GetScreen().Width() / 2;
    scrollY_ = (minY + maxY) * kCellPixelsY / 2 - client.GetScreen().Height() / 2;
    BuildScene(client); client.ShowScene();
}

// 원본 Renderer에 넘길 스프라이트·글자 목록을 만든다. 화면 버퍼를 직접 만지지 않는다.
void InspectView::BuildScene(netstorm::client::Client& client) {
    auto& screen = client.GetScreen();
    const auto& assets = client.Assets();
    std::vector<netstorm::client::RenderSprite> sprites;
    std::vector<netstorm::client::RenderText> text;
    if (view_ == "types") {
        const int columns = std::max(screen.Width() / kTypeCellWidth, 1);
        // 기본 프레임을 검사 격자의 칸 가운데에 놓는다.
        for (std::size_t i = 0; i < assets.Types().size(); ++i) {
            const auto& type = assets.Types()[i];
            const auto& frames = assets.Shapes().Blocks()[type.block].frames;
            const auto frame = static_cast<std::size_t>(std::max(type.definition.specialFrames.defaultFrame, 0));
            if (frame >= frames.size() || frames[frame].IsSpecial()) continue;
            const int x = static_cast<int>(i) % columns * kTypeCellWidth - scrollX_;
            const int y = static_cast<int>(i) / columns * kTypeCellHeight - scrollY_;
            const auto rect = frames[frame].rect;
            sprites.push_back({&assets.Shapes(), type.block, frame, x + kTypeCellWidth / 2 - (rect.left + rect.right) / 2,
                y + kTypeCellHeight / 2 - (rect.top + rect.bottom) / 2, {x, y, x + kTypeCellWidth, y + kTypeCellHeight}, {}, {}, false});
        }
    } else if (view_ == "fonts") {
        int y = 20 - scrollY_;
        // 원본에서 존재하는 슬롯·스타일을 모두 보여 준다.
        for (int slot = 0; slot < 7; ++slot) {
            if (slot == 2) continue;
            // 스타일 번호는 원본 글자 문맥의 순서다.
            for (int style = 0; style < ((slot == 1 || slot == 3 || slot == 4) ? 1 : 5); ++style) {
                const auto& font = client.Fonts().Get(slot, static_cast<netstorm::client::FontStyle>(style));
                text.push_back({&font, "Font " + std::to_string(slot) + "/" + std::to_string(style) + ": NetStorm Islands at War 0123456789",
                    20 - scrollX_, y, 255, 0, {1, 1}, true, false});
                y += font.Height() + 12;
            }
        }
    } else {
        // 월드 오브젝트 복원 전까지 저장된 기본 프레임만 제출한다.
        for (const auto& item : placed_) {
            const auto& type = assets.Types()[item.asset];
            const auto& frames = assets.Shapes().Blocks()[type.block].frames;
            if (item.frame >= frames.size() || frames[item.frame].IsSpecial()) continue;
            const int x = item.cellX * kCellPixelsX - scrollX_, y = item.cellY * kCellPixelsY - scrollY_;
            sprites.push_back({&assets.Shapes(), type.block, item.frame, x, y, {0, 0, screen.Width(), screen.Height()},
                {static_cast<float>(x), static_cast<float>(y), 0}, {}, false});
        }
    }
    client.GetRenderer().SetScene(std::move(sprites), std::move(text), view_ != "types");
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
    const int previousX = scrollX_, previousY = scrollY_;
    // 키가 눌려 있으면 뗌 표시가 없다.
    const auto held = [&client](std::uint32_t key) {
        return (client.Poll(key << netstorm::client::InputCode::kVirtualKeyShift).code & netstorm::client::InputCode::kRelease) == 0;
    };
    if (held(kKeyLeft)) scrollX_ -= kScrollStep;
    if (held(kKeyRight)) scrollX_ += kScrollStep;
    if (held(kKeyUp)) scrollY_ -= kScrollStep;
    if (held(kKeyDown)) scrollY_ += kScrollStep;
    if (scrollX_ != previousX || scrollY_ != previousY) BuildScene(client);
    return quit;
}
}
