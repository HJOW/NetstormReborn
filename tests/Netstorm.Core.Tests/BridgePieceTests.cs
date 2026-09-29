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

    /// <summary>C# 로 옮긴 모양 표·회전 표가 원본 Netstorm.exe 의 VA 0x52f998·0x531590 값과 같다</summary>
    [Fact]
    public void Catalog_MatchesOriginalExecutable()
    {
        OriginalData.RequireResources();
        string exe = Path.Combine(GameDataLocator.FindDataDirectory()!, "Netstorm.exe");
        Assert.SkipWhen(!File.Exists(exe), "Netstorm.exe 가 없어 건너뜀");
        byte[] image = File.ReadAllBytes(exe);
        // 모양 26개: 72바이트 항목 (가중치, 폭, 높이, 칸 15개 — 칸 = 글자<<24 | 방향<<8 | 변형 문자)
        foreach (BridgePattern pattern in BridgePatternCatalog.Patterns)
        {
            int offset = FileOffset(image, 0x52f998u + (uint)pattern.Index * 72);
            Assert.Equal(pattern.Weight, BitConverter.ToInt32(image, offset));
            Assert.Equal(pattern.Width, BitConverter.ToInt32(image, offset + 4));
            Assert.Equal(pattern.Height, BitConverter.ToInt32(image, offset + 8));
            // 칸마다 방향 글자와 변형 번호를 비교한다 ('.' 은 빈 칸)
            for (int i = 0; i < pattern.Width * pattern.Height; i++)
            {
                uint raw = BitConverter.ToUInt32(image, offset + 12 + i * 4);
                char letter = (char)((raw >> 8) & 0xff);
                BridgeCell? cell = pattern.Cells[i];
                if (letter == BridgeDirections.EmptyLetter)
                {
                    Assert.Null(cell);
                }
                else
                {
                    Assert.Equal(new BridgeCell(letter, (int)(raw & 0xff) - '0'), cell);
                }
            }
        }
        // 회전 표: 회전 번호 × 16 + 글자 순서의 32비트 글자 코드
        int rotation = FileOffset(image, 0x531590u);
        for (int r = 0; r < BridgeDirections.RotationCount; r++)
        {
            for (int i = 0; i < 16; i++)
            {
                char expected = (char)BitConverter.ToInt32(image, rotation + (r * 16 + i) * 4);
                Assert.Equal(expected, BridgeDirections.Rotate((char)('A' + i), r));
            }
        }
    }

    /// <summary>PE32 이미지의 가상 주소를 파일 위치로 바꾼다 (섹션 표 사용)</summary>
    private static int FileOffset(byte[] image, uint virtualAddress)
    {
        int pe = BitConverter.ToInt32(image, 0x3c);
        int sectionCount = BitConverter.ToUInt16(image, pe + 6);
        int optionalSize = BitConverter.ToUInt16(image, pe + 20);
        uint imageBase = BitConverter.ToUInt32(image, pe + 24 + 28);
        uint rva = virtualAddress - imageBase;
        int table = pe + 24 + optionalSize;
        // 주소를 포함하는 섹션을 찾아 원시 데이터 위치로 옮긴다
        for (int i = 0; i < sectionCount; i++)
        {
            int section = table + i * 40;
            uint start = BitConverter.ToUInt32(image, section + 12);
            uint size = Math.Max(BitConverter.ToUInt32(image, section + 8), BitConverter.ToUInt32(image, section + 16));
            if (rva >= start && rva < start + size)
            {
                return (int)(rva - start + BitConverter.ToUInt32(image, section + 20));
            }
        }
        throw new InvalidDataException($"VA 0x{virtualAddress:x} 를 포함하는 섹션이 없습니다");
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
}
