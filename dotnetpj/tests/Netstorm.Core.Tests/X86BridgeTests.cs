using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>10.78 다리 추첨과 전역 난수를 실제 원본 x86 출력에 대조한다.</summary>
public sealed class X86BridgeTests
{
    /// <summary>CD판 열은 비교 자료이며 C#의 기준은 패치판 가중치 287이다.</summary>
    [Fact]
    public void Draw_MatchesOriginal1078()
    {
        string[][] rows = X86Fixture.Read("bridge").Where(row => row[0] == "Draw").ToArray();
        Assert.Equal(10005, rows.Length);
        X86Fixture.CheckRows(rows, row => BridgePatternCatalog.Draw(X86Fixture.Int(row[1])) == X86Fixture.Int(row[2])
            && BridgePatternCatalog.TotalWeight == X86Fixture.Int(row[3]));
    }

    /// <summary>반환값과 32비트 오버플로 뒤 난수 상태를 함께 대조한다.</summary>
    [Fact]
    public void Random_MatchesOriginalStateAndResult()
    {
        string[][] rows = X86Fixture.Read("bridge").Where(row => row[0] == "Random").ToArray();
        Assert.Equal(1056, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var random = new NetstormRandom(X86Fixture.UInt(row[1]));
            return random.Next(X86Fixture.Int(row[2])) == X86Fixture.Int(row[3]) && random.State == X86Fixture.UInt(row[4]);
        });
    }

    /// <summary>다리 26개·영역 68개×모든 회전, 누락 프레임, 소수 좌표의 752개 전체 출력 목록.</summary>
    [Fact]
    public void CanonDecoder_MatchesOriginalX86()
    {
        string[][] all = X86Fixture.Read("bridge");
        var frameTables = new Dictionary<string, FrameCode[]>();
        // 기대값 파일의 원본 코드 표를 한 번만 읽는다.
        foreach (string[] row in all.Where(row => row[0] == "Frames"))
        {
            byte[] bytes = X86Fixture.Bytes(row[2]);
            var codes = new List<FrameCode>();
            // 방향·변형·번호·플래그의 네 바이트 순서다.
            for (int i = 0; i < bytes.Length; i += 4)
                codes.Add(new FrameCode((char)bytes[i], (char)bytes[i + 1], bytes[i + 2], (FrameCodeFlags)bytes[i + 3]));
            frameTables.Add(row[1], codes.ToArray());
        }
        string[][] rows = all.Where(row => row[0] == "Decode").ToArray();
        Assert.Equal(752, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            FrameCode[] codes = frameTables[row[1]];
            if (row[4] == "1") codes = codes.Where(code => code.Side is not ('F' or 'L')).ToArray();
            int shape = X86Fixture.Int(row[2]);
            CanonicalPattern pattern = row[1] == "B" ? BridgePatternCatalog.Patterns[shape].Canonical : FortMap.TerritoryPattern(shape);
            IReadOnlyList<DecodedPatternCell> actual = PatternDecoder.Decode(pattern, new TypeFrameTable(codes),
                X86Fixture.Int(row[3]), X86Fixture.FloatBits(row[5]), X86Fixture.FloatBits(row[6]));
            if (row[1] == "B")
            {
                var frames = new TypeFrameTable(codes);
                // 게임에서 쓰는 기존 BridgePiece.Cells도 같은 출력에 대조한다. 누락 프레임은 원본처럼 제외한다.
                DecodedPatternCell[] pieceCells = new BridgePiece(shape, X86Fixture.Int(row[3]) / 2).Cells()
                    .Select(cell => new DecodedPatternCell(frames.Find(cell.Cell.Letter, 'P', cell.Cell.Variation),
                        X86Fixture.FloatBits(row[5]) + (float)cell.Dx, X86Fixture.FloatBits(row[6]) + (float)cell.Dy, -97, cell.Cell.Letter))
                    .Where(cell => cell.Frame >= 0).ToArray();
                if (!actual.SequenceEqual(pieceCells)) return false;
            }
            string[] expected = row[7] == "-" ? [] : row[7].Split(';');
            if (expected.Length != actual.Count) return false;
            // 반환 순서뿐 아니라 단정밀도 좌표의 비트·번호·방향도 직접 대조한다.
            for (int i = 0; i < expected.Length; i++)
            {
                string[] fields = expected[i].Split(',');
                DecodedPatternCell cell = actual[i];
                if (cell.Frame != X86Fixture.Int(fields[0]) || BitConverter.SingleToUInt32Bits(cell.X) != X86Fixture.UInt(fields[1])
                    || BitConverter.SingleToUInt32Bits(cell.Y) != X86Fixture.UInt(fields[2]) || cell.Label != X86Fixture.Int(fields[3])
                    || cell.Side != X86Fixture.Int(fields[4])) return false;
            }
            return true;
        });
    }
}
