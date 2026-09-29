namespace Netstorm.Core.Bridges;

/// <summary>다리 칸이 이어지는 방향 (원본 연결 비트: 북 1, 동 2, 남 4, 서 8)</summary>
[Flags]
public enum BridgeLinks : byte
{
    /// <summary>연결 없음</summary>
    None = 0,
    /// <summary>북쪽 (월드 y − 1)</summary>
    North = 1,
    /// <summary>동쪽 (월드 x + 1)</summary>
    East = 2,
    /// <summary>남쪽 (월드 y + 1)</summary>
    South = 4,
    /// <summary>서쪽 (월드 x − 1)</summary>
    West = 8,
    /// <summary>네 방향 모두</summary>
    All = North | East | South | West,
}

/// <summary>
/// 다리·지면 클러스터 이름의 방향 글자 'A'~'P' 와 연결 비트의 대응, 다리 조각 회전 규칙.
/// 글자 → 비트 표는 원본 VA 0x52f910 (지면 연결 문자와 같은 표, docs/exe/terrain-and-bridges.md),
/// 회전 표는 원본 VA 0x531590 (Canondecoder.cpp FUN_00425860 이 회전 번호 × 16 + 글자로 찾는다).
/// </summary>
public static class BridgeDirections
{
    /// <summary>첫 방향 글자</summary>
    public const char FirstLetter = 'A';

    /// <summary>마지막 방향 글자</summary>
    public const char LastLetter = 'P';

    /// <summary>빈 칸 표시 글자 (원본 조각 표의 '.')</summary>
    public const char EmptyLetter = '.';

    /// <summary>회전 번호 개수 (0°, 90°, 180°, 270°)</summary>
    public const int RotationCount = 4;

    /// <summary>원본 VA 0x52f910: 'A'~'P' 순서의 연결 비트</summary>
    private static readonly byte[] LetterBits = [15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0];

    /// <summary>
    /// 원본 VA 0x531590: 회전 번호별로 'A'~'P' 가 바뀌는 글자. 회전 1 은 모든 연결을 시계 방향으로 90° 돌린다
    /// (예: B 북·동·남 → C 동·남·서, J 남북 → K 동서).
    /// </summary>
    private static readonly string[] RotationTable =
    [
        "ABCDEFGHIJKLMNOP",
        "ACDEBGHIFKJMNOLP",
        "ADEBCHIFGJKNOLMP",
        "AEBCDIFGHKJOLMNP",
    ];

    /// <summary>방향 글자인지 ('A'~'P')</summary>
    /// <param name="letter">검사할 글자</param>
    public static bool IsLetter(char letter) => letter is >= FirstLetter and <= LastLetter;

    /// <summary>방향 글자의 연결 비트</summary>
    /// <param name="letter">'A'~'P'</param>
    public static BridgeLinks ToLinks(char letter)
    {
        if (!IsLetter(letter))
        {
            throw new ArgumentOutOfRangeException(nameof(letter), letter, "방향 글자는 'A'~'P' 여야 합니다");
        }
        return (BridgeLinks)LetterBits[letter - FirstLetter];
    }

    /// <summary>연결 비트에 해당하는 방향 글자</summary>
    /// <param name="links">연결 비트</param>
    public static char ToLetter(BridgeLinks links)
    {
        byte bits = (byte)(links & BridgeLinks.All);
        // 표에서 같은 비트를 가진 글자를 찾는다 (16개가 모두 달라 하나만 맞는다)
        for (int i = 0; i < LetterBits.Length; i++)
        {
            if (LetterBits[i] == bits)
            {
                return (char)(FirstLetter + i);
            }
        }
        throw new InvalidOperationException("연결 비트 표에 없는 값입니다");
    }

    /// <summary>회전 번호만큼 돌린 방향 글자 (원본 회전 표)</summary>
    /// <param name="letter">'A'~'P'</param>
    /// <param name="rotation">회전 번호 (음수·4 이상도 4로 나눈 나머지로 쓴다)</param>
    public static char Rotate(char letter, int rotation)
    {
        if (!IsLetter(letter))
        {
            throw new ArgumentOutOfRangeException(nameof(letter), letter, "방향 글자는 'A'~'P' 여야 합니다");
        }
        return RotationTable[NormalizeRotation(rotation)][letter - FirstLetter];
    }

    /// <summary>연결 비트를 시계 방향으로 90° × 횟수만큼 돌린다 (북 → 동 → 남 → 서)</summary>
    /// <param name="links">연결 비트</param>
    /// <param name="rotation">회전 번호</param>
    public static BridgeLinks RotateLinks(BridgeLinks links, int rotation)
    {
        int bits = (int)(links & BridgeLinks.All);
        // 한 번 돌릴 때마다 비트를 왼쪽으로 한 칸 옮기고 넘친 서쪽 비트를 북쪽으로 되돌린다
        for (int i = 0; i < NormalizeRotation(rotation); i++)
        {
            bits = ((bits << 1) | (bits >> 3)) & (int)BridgeLinks.All;
        }
        return (BridgeLinks)bits;
    }

    /// <summary>한 방향의 반대 방향</summary>
    /// <param name="direction">북·동·남·서 중 하나</param>
    public static BridgeLinks Opposite(BridgeLinks direction) => RotateLinks(direction, 2);

    /// <summary>한 방향으로 한 칸 옮길 때의 월드 좌표 변화</summary>
    /// <param name="direction">북·동·남·서 중 하나</param>
    public static (int Dx, int Dy) Offset(BridgeLinks direction) => direction switch
    {
        BridgeLinks.North => (0, -1),
        BridgeLinks.East => (1, 0),
        BridgeLinks.South => (0, 1),
        BridgeLinks.West => (-1, 0),
        _ => throw new ArgumentOutOfRangeException(nameof(direction), direction, "한 방향만 지정해야 합니다"),
    };

    /// <summary>회전 번호를 0~3 으로 맞춘다</summary>
    /// <param name="rotation">회전 번호</param>
    public static int NormalizeRotation(int rotation) => ((rotation % RotationCount) + RotationCount) % RotationCount;
}
