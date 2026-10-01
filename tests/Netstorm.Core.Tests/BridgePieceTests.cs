using System.Text.Json;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>다리 조각 모양·추첨·회전·생산 칸 규칙 테스트 (docs/exe/bridge-pieces.md)</summary>
public sealed class BridgePieceTests
{
    /// <summary>원본 난수: 시드 0 이면 0x0BAD0BAD 에서 시작해 상태 × 0x10003 + 3</summary>
    [Fact]
    public void NetstormRandom_MatchesOriginalFormula()
    {
        var random = new NetstormRandom();
        uint expected = unchecked(NetstormRandom.DefaultSeed * 0x10003u + 3u);
        int first = random.Next(10000);
        Assert.Equal(expected, random.State);
        Assert.Equal((int)((expected >> 16) % 10000), first);
        // 범위 버전도 같은 상태 전이를 쓴다.
        int ranged = random.Next(100, 200);
        uint second = unchecked(expected * 0x10003u + 3u);
        Assert.Equal((int)((second >> 16) % 100) + 100, ranged);
    }

    /// <summary>모양 26개, 가중치 합 287, 폭·높이가 원본 표와 같다</summary>
    [Fact]
    public void Catalog_HasOriginalTableShape()
    {
        Assert.Equal(26, BridgePatternCatalog.Patterns.Count);
        Assert.Equal(287, BridgePatternCatalog.TotalWeight);
        Assert.Equal((2, 3), (BridgePatternCatalog.Patterns[4].Width, BridgePatternCatalog.Patterns[4].Height));
        Assert.Equal((4, 2), (BridgePatternCatalog.Patterns[12].Width, BridgePatternCatalog.Patterns[12].Height));
        // 원본 칸 배열은 15칸이므로 모든 모양이 그 안에 들어가야 한다.
        Assert.All(BridgePatternCatalog.Patterns, p => Assert.InRange(p.Width * p.Height, 1, 15));
    }

    /// <summary>추첨 경계: r=0 → 0번(한 칸), 1~30 → 1번, 31~40 → 2번, 가중치 0 인 11·12번은 경계에서만</summary>
    [Fact]
    public void Draw_UsesCumulativeLessOrEqual()
    {
        Assert.Equal(0, BridgePatternCatalog.Draw(0));
        Assert.Equal(1, BridgePatternCatalog.Draw(1));
        Assert.Equal(1, BridgePatternCatalog.Draw(30));
        Assert.Equal(2, BridgePatternCatalog.Draw(31));
        Assert.Equal(2, BridgePatternCatalog.Draw(40));
        Assert.Equal(3, BridgePatternCatalog.Draw(41));
        // 누적 182(10번까지)에서 끝나는 r 은 10번. 가중치 0 인 11·12번은 뽑히지 않고 13번으로 넘어간다.
        Assert.Equal(10, BridgePatternCatalog.Draw(182));
        Assert.Equal(13, BridgePatternCatalog.Draw(183));
        // 나머지 연산: 287 은 0 과 같다.
        Assert.Equal(0, BridgePatternCatalog.Draw(287));
        Assert.Equal(25, BridgePatternCatalog.Draw(286));
    }

    /// <summary>회전 표(VA 0x531590)는 연결 비트를 시계 방향으로 돌린 결과와 같다</summary>
    [Fact]
    public void RotationTable_EqualsClockwiseBitRotation()
    {
        // 모든 회전 번호와 글자 조합을 비트 회전과 비교한다
        for (int rotation = 0; rotation < BridgeDirections.RotationCount; rotation++)
        {
            for (char letter = BridgeDirections.FirstLetter; letter <= BridgeDirections.LastLetter; letter++)
            {
                BridgeLinks rotated = BridgeDirections.RotateLinks(BridgeDirections.ToLinks(letter), rotation);
                Assert.Equal(BridgeDirections.ToLetter(rotated), BridgeDirections.Rotate(letter, rotation));
            }
        }
        Assert.Equal(BridgeLinks.North | BridgeLinks.South, BridgeDirections.ToLinks('J'));
        Assert.Equal(BridgeLinks.East | BridgeLinks.West, BridgeDirections.ToLinks('K'));
    }

    /// <summary>모든 모양·회전에서 조각 안의 이웃 칸끼리 연결이 서로 맞는다 (회전 식 검증)</summary>
    [Fact]
    public void AllPiecesAndRotations_HaveConsistentInternalLinks()
    {
        BridgeLinks[] directions = [BridgeLinks.North, BridgeLinks.East, BridgeLinks.South, BridgeLinks.West];
        // 모양 26개 × 회전 4개를 모두 검사한다
        foreach (BridgePattern pattern in BridgePatternCatalog.Patterns)
        {
            for (int rotation = 0; rotation < BridgeDirections.RotationCount; rotation++)
            {
                var piece = new BridgePiece(pattern.Index, rotation);
                var byPosition = piece.Cells().ToDictionary(c => (c.Dx, c.Dy), c => c.Cell);
                Assert.Equal(pattern.FilledCount, byPosition.Count);
                // 각 칸의 네 방향 이웃이 조각 안에 있으면 양쪽 연결이 같아야 한다
                foreach (((int x, int y), BridgeCell cell) in byPosition)
                {
                    Assert.InRange(x, 0, piece.Width - 1);
                    Assert.InRange(y, 0, piece.Height - 1);
                    foreach (BridgeLinks direction in directions)
                    {
                        (int dx, int dy) = BridgeDirections.Offset(direction);
                        if (byPosition.TryGetValue((x + dx, y + dy), out BridgeCell neighbor))
                        {
                            bool here = cell.Links.HasFlag(direction);
                            bool there = neighbor.Links.HasFlag(BridgeDirections.Opposite(direction));
                            Assert.True(here == there, $"조각 {pattern.Index} 회전 {rotation} ({x},{y}) {direction}");
                        }
                    }
                }
            }
        }
    }

    /// <summary>T 자 조각 4번을 시계 방향으로 돌리면 가로 막대 아래로 가지가 난다</summary>
    [Fact]
    public void Piece4_RotatesClockwise()
    {
        var piece = new BridgePiece(4);
        piece.RotateClockwise();
        Assert.Equal((3, 2), (piece.Width, piece.Height));
        string layout = string.Join(" ", piece.Cells().Select(c => $"{c.Dx},{c.Dy}:{c.Cell}"));
        Assert.Equal("0,0:K1 1,0:C1 2,0:K1 1,1:J1", layout);
        piece.RotateCounterclockwise();
        Assert.Equal(0, piece.Rotation);
    }

    /// <summary>모든 모양의 모든 회전 칸이 원본 bridge.type 에서 보통·금 간·단단한 프레임을 찾는다</summary>
    [Fact]
    public void AllPieceCells_HaveBridgeFrames()
    {
        TypeFrameTable frames = OriginalData.RequireTypes().Find("bridge")!.Definition.Frames;
        // 모양·회전마다 칸의 세 상태 프레임을 찾는다
        foreach (BridgePattern pattern in BridgePatternCatalog.Patterns)
        {
            for (int rotation = 0; rotation < BridgeDirections.RotationCount; rotation++)
            {
                foreach (PlacedBridgeCell placed in new BridgePiece(pattern.Index, rotation).Cells())
                {
                    Assert.True(BridgeFrames.Find(frames, placed.Cell) >= 0, $"{placed.Cell}");
                    Assert.True(BridgeFrames.Find(frames, placed.Cell, BridgeCondition.Cracked) >= 0, $"{placed.Cell} cracked");
                    Assert.True(BridgeFrames.Find(frames, placed.Cell, BridgeCondition.Hard) >= 0, $"{placed.Cell} hard");
                }
            }
        }
        // 저장 다리 값 규칙(docs/exe/terrain-and-bridges.md)과 같은 번호: J01 = 35, K01 = 41
        Assert.Equal(35, BridgeFrames.Find(frames, new BridgeCell('J', 1)));
        Assert.Equal(41, BridgeFrames.Find(frames, new BridgeCell('K', 1)));
    }

    /// <summary>모양·회전 표를 원본 VA 0x52f998·0x531590에서 독립적으로 추출한 고정 자료와 비교한다.</summary>
    [Fact]
    public void Catalog_MatchesCapturedOriginalTables()
    {
        using JsonDocument tables = JsonDocument.Parse(File.ReadAllText(
            Path.Combine(AppContext.BaseDirectory, "Fixtures", "bridge-tables.json")));
        JsonElement patterns = tables.RootElement.GetProperty("patterns");
        Assert.Equal(BridgePatternCatalog.Patterns.Count, patterns.GetArrayLength());
        // 원본에서 추출한 모양마다 가중치·크기·모든 칸을 비교한다.
        foreach (BridgePattern pattern in BridgePatternCatalog.Patterns)
        {
            JsonElement expected = patterns[pattern.Index];
            Assert.Equal(expected.GetProperty("weight").GetInt32(), pattern.Weight);
            Assert.Equal(expected.GetProperty("width").GetInt32(), pattern.Width);
            Assert.Equal(expected.GetProperty("height").GetInt32(), pattern.Height);
            JsonElement cells = expected.GetProperty("cells");
            Assert.Equal(pattern.Width * pattern.Height, cells.GetArrayLength());
            // 빈 칸은 null이며 나머지 칸은 방향 글자·변형 번호의 문자열이다.
            for (int i = 0; i < pattern.Width * pattern.Height; i++)
            {
                Assert.Equal(cells[i].GetString(), pattern.Cells[i]?.ToString());
            }
        }
        JsonElement rotations = tables.RootElement.GetProperty("rotations");
        Assert.Equal(BridgeDirections.RotationCount, rotations.GetArrayLength());
        // 회전 4종마다 A~P 글자의 원본 변환 값을 비교한다.
        for (int r = 0; r < BridgeDirections.RotationCount; r++)
        {
            string expected = rotations[r].GetString()!;
            Assert.Equal(16, expected.Length);
            // 각 방향 글자의 회전 결과를 검사한다.
            for (int i = 0; i < 16; i++)
            {
                Assert.Equal(expected[i], BridgeDirections.Rotate((char)('A' + i), r));
            }
        }
    }

    /// <summary>원본 조작: 기본 회전은 시계 방향, 반대 회전(C)이 켜지면 반시계 방향 (2026-09-29 원본 실행)</summary>
    [Fact]
    public void RotateByPlayer_FollowsReverseSetting()
    {
        // 원본에서 들었던 5번 조각: 반대 회전 켜짐 → 회전 3 (가로 4칸 위로 가지), 기본 → 회전 0 → 회전 1 (아래로 가지)
        var piece = new BridgePiece(5);
        piece.RotateByPlayer(reverseRotation: true);
        Assert.Equal(3, piece.Rotation);
        Assert.Equal("1,0:J1 0,1:K1 1,1:E1 2,1:K1 3,1:K1", string.Join(" ", piece.Cells().Select(c => $"{c.Dx},{c.Dy}:{c.Cell}")));
        piece.RotateByPlayer(reverseRotation: false);
        Assert.Equal(0, piece.Rotation);
        piece.RotateByPlayer(reverseRotation: false);
        Assert.Equal("0,0:K1 1,0:K1 2,0:C1 3,0:K1 2,1:J1", string.Join(" ", piece.Cells().Select(c => $"{c.Dx},{c.Dy}:{c.Cell}")));
    }

    /// <summary>커서 → 조각 왼쪽 위 칸: x 는 기준점 −7~+8, y 는 기준점~+10 (원본 측정 경계)</summary>
    [Fact]
    public void BridgeCursor_MatchesMeasuredBoundaries()
    {
        // 칸 50 의 기준점 x = 800: 793~808 이 칸 50, 792 는 49, 809 는 51
        Assert.Equal(49, BridgeCursor.TopLeftCell(792, 0).X);
        Assert.Equal(50, BridgeCursor.TopLeftCell(793, 0).X);
        Assert.Equal(50, BridgeCursor.TopLeftCell(808, 0).X);
        Assert.Equal(51, BridgeCursor.TopLeftCell(809, 0).X);
        // 칸 13 의 기준점 y = 143: 143~153 이 칸 13, 154 부터 칸 14
        Assert.Equal(12, BridgeCursor.TopLeftCell(0, 142).Y);
        Assert.Equal(13, BridgeCursor.TopLeftCell(0, 143).Y);
        Assert.Equal(13, BridgeCursor.TopLeftCell(0, 153).Y);
        Assert.Equal(14, BridgeCursor.TopLeftCell(0, 154).Y);
    }

    /// <summary>생산 칸: 1초 간격, 칸 수 제한, 첫 추첨은 한 칸 조각, 템플을 잃으면 비운다</summary>
    [Fact]
    public void Tray_RefillsLikeOriginal()
    {
        var tray = new BridgeTray(capacity: 2, new NetstormRandom());
        Assert.Null(tray.Update(0.0, hasTemple: false));
        BridgePiece? first = tray.Update(0.0, hasTemple: true);
        Assert.NotNull(first);
        // 0번째 추첨은 칸에 한 칸 조각이 없으므로 한 칸 조각이 된다.
        Assert.Equal(BridgePatternCatalog.SinglePiece, first.Pattern.Index);
        Assert.Null(tray.Update(0.5, hasTemple: true));
        Assert.NotNull(tray.Update(1.0, hasTemple: true));
        // 칸이 가득 차면 추첨하지 않지만 시각은 다시 예약한다.
        Assert.Null(tray.Update(2.0, hasTemple: true));
        Assert.Equal(2, tray.DrawCount);
        tray.Take(0);
        Assert.Null(tray.Update(2.5, hasTemple: true));
        Assert.NotNull(tray.Update(3.0, hasTemple: true));
        // 템플을 잃는 순간 칸이 비고, 템플이 없으면 채우지 않는다.
        Assert.Null(tray.Update(10.0, hasTemple: false));
        Assert.Empty(tray.Pieces);
    }

    /// <summary>5번째 추첨마다 한 칸 조각을 강제하되, 칸에 이미 한 칸 조각이 있으면 원래 추첨을 쓴다</summary>
    [Fact]
    public void Tray_ForcesSinglePieceEveryFifthDrawOnlyWhenAbsent()
    {
        var random = new NetstormRandom();
        var tray = new BridgeTray(capacity: 6, random) { TimerEnabled = false };
        var expectedRandom = new NetstormRandom();
        // 6번 채우며 각 추첨이 원본 규칙과 같은지 따라가 본다
        for (int draw = 0; draw < 6; draw++)
        {
            bool singleAlready = tray.Pieces.Any(p => p.Pattern.Index == BridgePatternCatalog.SinglePiece);
            int drawn = BridgePatternCatalog.Draw(expectedRandom.Next(BridgePatternCatalog.DrawRange));
            int expected = draw % BridgeTray.SinglePiecePeriod == 0 && !singleAlready ? BridgePatternCatalog.SinglePiece : drawn;
            BridgePiece piece = tray.Update(draw, hasTemple: true)!;
            Assert.Equal(expected, piece.Pattern.Index);
        }
        Assert.Equal(expectedRandom.State, random.State);
    }

    /// <summary>
    /// 칸에 들어온 조각은 6초(원본 타이머 0x3c × 0.1초) 동안 금 간 품질이고 그 뒤 보통 품질이 된다.
    /// 타이머는 0.1초 단위 정수로 비교한다. 칸 밖에서 만든 조각(시험·편집기)은 처음부터 보통이다.
    /// </summary>
    [Fact]
    public void Tray_NewPieceIsCrackedForSixSeconds()
    {
        var tray = new BridgeTray(capacity: 2, new NetstormRandom());
        BridgePiece piece = tray.Update(0.5, hasTemple: true)!;
        Assert.Equal(BridgeCondition.Cracked, BridgeTray.QualityAt(piece, 0.5));
        Assert.Equal(BridgeCondition.Cracked, BridgeTray.QualityAt(piece, 6.49));
        Assert.Equal(BridgeCondition.Normal, BridgeTray.QualityAt(piece, 6.5));
        Assert.Equal(BridgeCondition.Normal, BridgeTray.QualityAt(new BridgePiece(BridgePatternCatalog.SinglePiece), 0.0));
    }
}
