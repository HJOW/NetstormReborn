using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>다리 조각 모양의 한 칸: 방향 글자와 변형 번호 (bridge.type 클러스터 "J01" = 글자 J, 변형 1)</summary>
/// <param name="Letter">방향 글자 'A'~'P'</param>
/// <param name="Variation">변형 번호 (원본 표의 '1'·'2' → 1·2)</param>
public readonly record struct BridgeCell(char Letter, int Variation)
{
    /// <summary>이 칸의 연결 방향</summary>
    public BridgeLinks Links => BridgeDirections.ToLinks(Letter);

    /// <summary>개발용 표기 (예: "J1")</summary>
    public override string ToString() => $"{Letter}{Variation}";
}

/// <summary>
/// 다리 조각 모양 하나 (원본 VA 0x52f998 표의 72바이트 항목: 가중치, 폭, 높이, 칸 15개).
/// 칸 배열은 행 우선(y × 폭 + x)이며 빈 칸은 null 이다.
/// </summary>
public sealed class BridgePattern
{
    /// <summary>표 안의 번호 (원본 Canondecoder 가 오브젝트에 저장하는 조각 번호)</summary>
    public int Index { get; }

    /// <summary>추첨 가중치 (0 인 모양도 누적 경계값에서 뽑힐 수 있다 — BridgePatternCatalog.Draw)</summary>
    public int Weight { get; }

    /// <summary>가로 칸 수 (월드 x 방향)</summary>
    public int Width { get; }

    /// <summary>세로 칸 수 (월드 y 방향)</summary>
    public int Height { get; }

    /// <summary>행 우선 칸 배열 (빈 칸 null)</summary>
    public IReadOnlyList<BridgeCell?> Cells { get; }

    /// <summary>모양을 만든다</summary>
    /// <param name="index">표 번호</param>
    /// <param name="weight">추첨 가중치</param>
    /// <param name="rows">행 문자열 (칸은 공백으로 구분, "J1" 같은 글자+변형 또는 빈 칸 "..")</param>
    internal BridgePattern(int index, int weight, params string[] rows)
    {
        Index = index;
        Weight = weight;
        Height = rows.Length;
        string[][] tokens = rows.Select(r => r.Split(' ', StringSplitOptions.RemoveEmptyEntries)).ToArray();
        Width = tokens[0].Length;
        var cells = new BridgeCell?[Width * Height];
        // 행·열 순서로 칸 표기를 해석한다
        for (int y = 0; y < Height; y++)
        {
            if (tokens[y].Length != Width)
            {
                throw new ArgumentException($"조각 {index} 의 {y}행 칸 수가 다릅니다");
            }
            for (int x = 0; x < Width; x++)
            {
                string token = tokens[y][x];
                cells[y * Width + x] = token[0] == BridgeDirections.EmptyLetter
                    ? null
                    : new BridgeCell(token[0], token[1] - '0');
            }
        }
        Cells = cells;
    }

    /// <summary>(x, y) 칸 (범위 밖이면 null)</summary>
    /// <param name="x">0 ~ 폭 − 1</param>
    /// <param name="y">0 ~ 높이 − 1</param>
    public BridgeCell? CellAt(int x, int y) =>
        x < 0 || y < 0 || x >= Width || y >= Height ? null : Cells[y * Width + x];

    /// <summary>다리가 놓인 칸 수</summary>
    public int FilledCount => Cells.Count(c => c != null);

    /// <summary>영역과 다리가 함께 사용하는 원본 CanonDecoder의 셀 표로 변환한다.</summary>
    public CanonicalPattern Canonical => new(Width, Height, Cells.Select(cell =>
        cell is BridgeCell value ? new PatternCell(value.Letter, value.Variation) : new PatternCell('.', 0)).ToArray());
}

/// <summary>
/// 원본 다리 조각 모양 표 26개 (Netstorm.exe VA 0x52f998, Canondecoder.cpp)와 추첨 규칙 (FUN_004257c0).
/// 표 값은 exe 에서 그대로 옮겼다 — 재추출·검증 방법은 docs/exe/bridge-pieces.md.
/// </summary>
public static class BridgePatternCatalog
{
    /// <summary>한 칸짜리 조각 번호 (5번째 추첨마다 강제로 주는 조각)</summary>
    public const int SinglePiece = 0;

    /// <summary>추첨에 쓰는 난수 범위 (원본 FUN_004558c0(10000))</summary>
    public const int DrawRange = 10000;

    /// <summary>원본 표 순서의 모양 26개 (행은 월드 y, 칸은 월드 x 순서)</summary>
    public static IReadOnlyList<BridgePattern> Patterns { get; } =
    [
        new(0, 0, "J1"),
        new(1, 30, "J1", "J2"),
        new(2, 10, "J1", "J1", "J1"),
        new(3, 4, "J1", "J1", "J1", "J1"),
        new(4, 50, "J1 ..", "B1 K1", "J1 .."),
        new(5, 10, "J1 ..", "B1 K1", "J1 ..", "J1 .."),
        new(6, 10, "J1 ..", "J1 ..", "B1 K1", "J1 .."),
        new(7, 20, "J1 .. ..", "B1 K1 K1", "J1 .. .."),
        new(8, 40, "J1 ..", "B2 K1", "J1 .."),
        new(9, 4, "J1 ..", "J1 ..", "I1 K2"),
        new(10, 4, ".. J1", ".. J1", "K2 H1"),
        new(11, 0, "K1 G1 ..", ".. I1 K2"),
        new(12, 0, "K1 G1 .. ..", ".. I1 K2 K1"),
        new(13, 2, "J1 ..", "J1 ..", "J2 ..", "I1 K1"),
        new(14, 2, ".. J1", ".. J2", ".. J1", "K1 H1"),
        new(15, 8, "K2 G1", ".. J1"),
        new(16, 2, ".. F1 K1", ".. J1 ..", "K1 H1 .."),
        new(17, 2, "K1 G1 ..", ".. J1 ..", ".. I1 K1"),
        new(18, 2, ".. F1 K1", ".. J1 ..", ".. J1 ..", "K1 H1 .."),
        new(19, 20, "J1 .. ..", "I1 C1 K1", ".. J1 .."),
        new(20, 20, "J1 .. ..", "I1 C2 K1", ".. J1 .."),
        new(21, 40, ".. .. J1", "K1 C1 H1", ".. J1 .."),
        new(22, 1, ".. .. J1", "K1 C2 H1", ".. J2 ..", ".. J1 .."),
        new(23, 1, "J1 .. ..", "J1 .. ..", "I1 C1 K1", ".. J1 .."),
        new(24, 0, "F1 K1", "J1 ..", "I1 K1"),
        new(25, 5, ".. F1 K1", "K1 D1 ..", ".. I1 K1"),
    ];

    /// <summary>모든 모양의 가중치 합 (원본 DAT_005453d4 에 처음 한 번 계산해 둔다)</summary>
    public static int TotalWeight { get; } = Patterns.Sum(p => p.Weight);

    /// <summary>
    /// 난수 값으로 모양 번호를 고른다 (원본 FUN_004257c0).
    /// r = value % 가중치 합일 때 누적 가중치가 처음으로 r 이상이 되는 모양을 고른다.
    /// 비교가 "이하"라서 r = 0 이면 가중치 0 인 0번(한 칸)이 뽑힌다.
    /// </summary>
    /// <param name="value">부호 있는 원본 입력. 실제 게임의 난수는 0~9999이며 음수 나머지는 첫 조각을 고른다.</param>
    public static int Draw(int value)
    {
        int r = value % TotalWeight;
        int cumulative = 0;
        // 표 순서로 누적 가중치를 더하며 r 이 들어가는 첫 모양을 찾는다
        foreach (BridgePattern pattern in Patterns)
        {
            cumulative += pattern.Weight;
            if (r <= cumulative)
            {
                return pattern.Index;
            }
        }
        // r < 가중치 합이므로 도달하지 않는다. 원본은 이 경우 표 크기(26)를 돌려준다
        return Patterns.Count;
    }
}
