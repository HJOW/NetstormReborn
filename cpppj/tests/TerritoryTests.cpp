// 영역 배치(CanonDecoder·ChunkMap·Islandbuilder)의 단위 검사. 원본 파일 없이 puzzlePiece와 같은 꼴의 타입 글을 쓴다.
// 실제 두 판본의 `.fort` 전체 대조는 tools/cpp_window_smoke.py가 한다.
#include "TestSupport.h"
#include "o/CanonDecoder.h"
#include "o/ChunkMap.h"
#include "o/Islandbuilder.h"
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// 형식 오류가 예외로 보고되는지 검사한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}

// 원본 puzzlePiece.type과 같은 클러스터 구성(A01~P01, A02~P02)의 프레임 코드 표를 만든다.
RiftTypeFrames PieceFrames() {
    std::string text = "typename puzzlepiece typeflags ; { foot_x = 1; foot_y = 1; }\n";
    // 번호 01, 02 순서로.
    for (int number = 1; number <= 2; ++number) {
        // 방향 글자 A~P.
        for (char side = 'A'; side <= 'P'; ++side) {
            text += std::string(1, side) + "0" + std::to_string(number) + " : " + (side == 'A' && number == 1 ? "default" : "")
                + ": \"null.gif\" #00;\n";
        }
    }
    return RiftTypeDefinition::Parse(text).FrameTable();
}

// 반복자가 내는 칸을 "x,y글자" 꼴로 이어 적는다.
std::string Walk(const RiftTypeFrames& frames, std::size_t shape, int direction, float x, float y) {
    std::string text;
    // 유효한 칸이 남은 동안.
    for (CanonDecoder decoder(frames, TerritoryPatterns()[shape], direction, x, y); decoder.Valid(); decoder.Advance())
        text += std::to_string(static_cast<int>(decoder.X())) + "," + std::to_string(static_cast<int>(decoder.Y())) + decoder.Side() + " ";
    return text;
}

// `Territory` 레코드 하나(6바이트)를 만든다: 모양·방향, 플래그, 위치(하위 니블 x, 상위 니블 y).
std::vector<std::uint8_t> Record(int shape, int direction, int flags, int x, int y) {
    return {static_cast<std::uint8_t>(shape | (direction << 6)), static_cast<std::uint8_t>(flags),
            static_cast<std::uint8_t>(x | (y << 4)), 0, 0, 0};
}

// 영역의 청크를 "x,y글자;" 꼴로 적는다(`--dump-territories`와 같은 꼴).
std::string Chunks(const ChunkMap& map, const IslandList& islands, int owner, int territory) {
    std::string text;
    // 순회 순서대로.
    for (const auto& chunk : TerritoryChunks(map, islands, owner, territory))
        text += std::to_string(chunk.x) + "," + std::to_string(chunk.y) + map.At(chunk.x, chunk.y).side + ";";
    return text;
}
}

// 표는 실행 파일의 68개 그대로다. 앞의 것은 3 × 3 ".L.FAGINH".
TEST_CASE(CanonPatterns_TableShape_MatchesExecutable) {
    const auto patterns = TerritoryPatterns();
    CHECK(patterns.size() == 68);
    CHECK(patterns[0].width == 3 && patterns[0].height == 3 && patterns[0].first == 1);
    CHECK(patterns[0].cells[0][1] == '.' && patterns[0].cells[1][1] == 'L' && patterns[0].cells[4][1] == 'A');
    CHECK(patterns[7].width == 2 && patterns[7].height == 2);
    CHECK(patterns[4].width == 3 && patterns[4].height == 1);
    // 21번부터는 번호 글자가 붙은 'P' 패턴이다.
    CHECK(patterns[21].width == 4 && patterns[21].height == 1 && patterns[21].cells[0][1] == 'P' && patterns[21].cells[3][3] == 'd');
}

// 회전 0: 줄 우선으로 훑고 빈 칸('.')은 건너뛴다.
TEST_CASE(CanonDecoder_Unrotated_WalksRowMajorSkippingEmptyCells) {
    const auto frames = PieceFrames();
    CHECK(Walk(frames, 0, 0, 2.0f, 7.0f) == "3,7L 2,8F 3,8A 4,8G 2,9I 3,9N 4,9H ");
    CHECK(Walk(frames, 7, 0, 10.0f, 6.0f) == "10,6F 11,6G 10,7I 11,7H ");
    CHECK(Walk(frames, 4, 0, 0.0f, 0.0f) == "0,0O 1,0K 2,0M ");
    // 번호 글자: 'a'를 뺀 값. 번호가 없는 칸은 -0x61이다.
    CanonDecoder labelled(frames, TerritoryPatterns()[21], 0, 0.0f, 0.0f);
    CHECK(labelled.Valid() && labelled.Label() == 0 && labelled.Side() == 'P');
    labelled.Advance();
    CHECK(labelled.Label() == 1 && labelled.X() == 1.0f);
    CanonDecoder plain(frames, TerritoryPatterns()[7], 0, 0.0f, 0.0f);
    CHECK(plain.Label() == -0x61);
    // 프레임 번호는 프레임 코드 표의 위치다(F는 여섯째).
    CHECK(plain.Frame() == 5 && plain.Side() == 'F');
}

// 회전(방향 값 / 2): 폭과 높이가 바뀌고 방향 글자도 회전 표로 바뀐다. 칸 수는 그대로다.
TEST_CASE(CanonDecoder_Rotated_SwapsExtentsAndSides) {
    const auto frames = PieceFrames();
    // 3 × 1 "OKM"을 한 번 돌리면 1 × 3이 된다.
    CHECK(Walk(frames, 4, 2, 0.0f, 0.0f) == "0,0L 0,1J 0,2N ");
    // 두 번 돌리면 순서가 뒤집힌다.
    CHECK(Walk(frames, 4, 4, 0.0f, 0.0f) == "0,0O 1,0K 2,0M ");
    // 방향 값 0·1은 같은 회전이다.
    CHECK(Walk(frames, 0, 1, 2.0f, 7.0f) == Walk(frames, 0, 0, 2.0f, 7.0f));
    // 2 × 2 "FGIH"는 어느 회전에서도 네 칸이다.
    for (int direction = 0; direction < 8; direction += 2) {
        int cells = 0;
        // 칸 수를 센다.
        for (CanonDecoder decoder(frames, TerritoryPatterns()[7], direction, 0.0f, 0.0f); decoder.Valid(); decoder.Advance()) ++cells;
        CHECK(cells == 4);
    }
    CHECK(Throws([&] { CanonDecoder(frames, TerritoryPatterns()[0], 8, 0.0f, 0.0f); }));
}

// 청크 지도: 처음에는 섬이 없고, 범위 밖 좌표는 가장자리로 당겨진다.
TEST_CASE(ChunkMap_DefaultsAndClamping_MatchOriginal) {
    ChunkMap map;
    CHECK(map.XLength() == 16 && map.YLength() == 16);
    CHECK(!map.HasIsland(0, 0) && map.At(15, 15).island == kNoIsland);
    map.At(3, 4).island = 2;
    CHECK(map.HasIsland(3, 4) && !map.HasIsland(4, 3));
    // 원본 00432180: assert 뒤에 좌표를 가장자리로 당긴다.
    CHECK(&map.At(-1, 99) == &map.At(0, 15));
    map.At(5, 6).island = 2;
    const auto bounds = map.Bounds(2);
    CHECK(bounds.left == 3 && bounds.top == 4 && bounds.right == 6 && bounds.bottom == 7);
    // 없는 섬: 원본 그대로 (16, 16, 1, 1).
    const auto missing = map.Bounds(9);
    CHECK(missing.left == 16 && missing.top == 16 && missing.right == 1 && missing.bottom == 1);
    const auto rect = DefaultChunkRect();
    CHECK(rect.left == 1 && rect.top == 1 && rect.right == 15 && rect.bottom == 15);
    CHECK(Throws([] { ChunkMap(17, 16); }));
    CHECK(Throws([] { ChunkMap(0, 16); }));
}

// 섬 목록: 번호는 등록 순서, 소유자와 영역 번호가 모두 맞아야 일치한다.
TEST_CASE(IslandList_AddAndMatch) {
    IslandList islands;
    CHECK(!islands.Exists(0) && !islands.Matches(0, 1, 0));
    CHECK(islands.Add(1, 3) == 0 && islands.Add(2, 3) == 1);
    CHECK(islands.Exists(1) && !islands.Exists(2) && !islands.Exists(-1));
    CHECK(islands.Matches(0, 1, 3) && !islands.Matches(0, 2, 3) && !islands.Matches(0, 1, 4));
    CHECK(!islands.Matches(kNoIsland, 1, 3));
    CHECK(islands.Islands().size() == 2);
}

// 원본 0046dba0·0046dd70: 실제 파일(TEST01, Save the Island)에서 확인한 배치.
TEST_CASE(IslandBuilder_PlacesTerritoriesAtStoredPositions) {
    const auto frames = PieceFrames();
    ChunkMap map;
    IslandList islands;
    IslandBuilder builder(map, islands, frames, 1);
    // TEST01의 영역 0~3과, 놓지 않는 레코드 둘(플래그 0, 플래그 bit 1).
    std::vector<std::uint8_t> territories;
    // 레코드를 번호순으로 붙인다.
    for (const auto& record : {Record(0, 0, 9, 1, 6), Record(5, 0, 9, 10, 2), Record(2, 0, 9, 9, 10), Record(0, 0, 9, 6, 3),
                               Record(7, 0, 0, 0, 0), Record(7, 0, 3, 0, 0)})
        territories.insert(territories.end(), record.begin(), record.end());
    territories.resize(kTerritoryRecordBytes * kTerritoryCount);
    builder.PlaceAll(territories);
    CHECK(islands.Islands().size() == 4);
    CHECK(Chunks(map, islands, 1, 0) == "3,7L;2,8F;3,8A;4,8G;2,9I;3,9N;4,9H;");
    CHECK(Chunks(map, islands, 1, 1) == "11,3O;12,3K;13,3G;13,4N;");
    CHECK(Chunks(map, islands, 1, 2) == "10,11F;11,11C;12,11G;10,12B;11,12A;12,12D;10,13I;11,13N;12,13H;");
    CHECK(Chunks(map, islands, 1, 3) == "8,4L;7,5F;8,5A;9,5G;7,6I;8,6N;9,6H;");
    CHECK(Chunks(map, islands, 1, 4).empty() && Chunks(map, islands, 1, 5).empty());
    // 다른 소유자의 영역으로는 찾지 않는다.
    CHECK(Chunks(map, islands, 2, 0).empty());
    // 청크 항목: 섬 번호, 레코드 6바이트, 영역 안의 순서.
    const auto& entry = map.At(3, 8);
    CHECK(entry.island == 0 && entry.side == 'A' && entry.ordinal == 2);
    CHECK(entry.record[0] == 0 && entry.record[1] == 9 && entry.record[2] == 0x61);
    const auto bounds = map.Bounds(1);
    CHECK(bounds.left == 11 && bounds.top == 3 && bounds.right == 14 && bounds.bottom == 5);
    CHECK(!map.HasIsland(2, 7) && map.HasIsland(3, 7));
}

// 한 레코드 놓기: 위치 바이트 무시, 범위의 원점, 잘못된 입력.
TEST_CASE(IslandBuilder_PlaceOptionsAndErrors) {
    const auto frames = PieceFrames();
    ChunkMap map;
    IslandList islands;
    IslandBuilder builder(map, islands, frames, 3);
    // Save the Island의 영역 7: 2 × 2, 위치 바이트 0x59 → (10,6)부터.
    const auto record = Record(7, 0, 9, 9, 5);
    CHECK(record[2] == 0x59);
    CHECK(builder.Place(record, 7) == 0);
    CHECK(Chunks(map, islands, 3, 7) == "10,6F;11,6G;10,7I;11,7H;");
    // 위치를 무시하면 범위의 왼쪽 위(1,1)에 놓는다.
    CHECK(builder.Place(record, 8, true) == 1);
    CHECK(Chunks(map, islands, 3, 8) == "1,1F;2,1G;1,2I;2,2H;");
    // 범위를 달리 주면 그 원점에서 센다.
    ChunkMap other;
    IslandList otherIslands;
    IslandBuilder shifted(other, otherIslands, frames, 1, {4, 5, 15, 15});
    shifted.Place(Record(4, 0, 9, 1, 2), 0);
    CHECK(Chunks(other, otherIslands, 1, 0) == "5,7O;6,7K;7,7M;");
    // 나중에 놓은 영역이 겹친 청크를 차지한다.
    shifted.Place(Record(4, 0, 9, 3, 2), 1);
    CHECK(Chunks(other, otherIslands, 1, 0) == "5,7O;6,7K;" && Chunks(other, otherIslands, 1, 1) == "7,7O;8,7K;9,7M;");
    // 모양 번호는 6비트(최대 63)라 68개 표를 벗어나지 않는다. 레코드가 짧으면 예외다.
    CHECK(!Throws([&] { builder.Place(Record(0x3f, 0, 9, 0, 0), 0); }));
    CHECK(Throws([&] { builder.Place(std::vector<std::uint8_t>{0, 9, 0}, 0); }));
    const std::vector<std::uint8_t> territories(kTerritoryRecordBytes * kTerritoryCount);
    CHECK(Throws([&] { builder.PlaceAll(territories, 3, 3); })); // 원본 assert "iStart != iEnd".
}
