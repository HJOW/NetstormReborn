namespace Netstorm.Core.Rules;

/// <summary>
/// 원본 마우스 커서 모양 18가지. **값은 원본 커서 번호(1~18)** 이며 원본 표(10.78 005423a8)의 순서와 같다.
/// 번호 0(아직 모양을 정하지 않음)은 클론이 쓰지 않으므로 멤버가 없다. 리소스 번호는 <see cref="GameCursors"/> 에서 얻는다.
/// 쓰임이 확인된 것은 Arrow·Place·Temple·Forbidden·Move 다섯 가지(TEST02 원본 커서 기록과 RT_CURSOR 픽셀 대조)이고,
/// 나머지 이름은 **그림의 생김새**만 적은 것이다. 어떤 상황에 어떤 번호를 쓰는지는 원본 입력 처리(UserInput 004d62b0)가
/// 복원된 뒤에 연결한다 (LEFT_JOBS.dotnetpj.md 5-4·8절).
/// </summary>
/// <remarks>2026-10-10: 다섯 가지에서 원본 표 전체 18가지로 넓히고 값을 원본 번호로 맞췄다.</remarks>
public enum GameCursor
{
    /// <summary>기본 흰 화살표 (번호 1, 그룹 113).</summary>
    Arrow = 1,
    /// <summary>건물을 든 상태의 × (번호 2, 그룹 110).</summary>
    Place = 2,
    /// <summary>선택한 사제의 내 완공 템플: 안쪽을 향한 화살표 네 개 (번호 3, 그룹 111).</summary>
    Temple = 3,
    /// <summary>좌우 양방향 화살표 (번호 4, 그룹 108). 쓰임 미확인.</summary>
    ArrowsHorizontal = 4,
    /// <summary>상하 양방향 화살표 (번호 5, 그룹 107). 쓰임 미확인.</summary>
    ArrowsVertical = 5,
    /// <summary>사제의 이동 불가 대상·허공: 금지 표시 (번호 6, 그룹 109).</summary>
    Forbidden = 6,
    /// <summary>좌우 양방향 화살표 가운데에 위쪽 삼각형 (번호 7, 그룹 115). 쓰임 미확인.</summary>
    ArrowsHorizontalPeak = 7,
    /// <summary>움켜쥐는 손 (번호 8, 그룹 116). 쓰임 미확인.</summary>
    Grab = 8,
    /// <summary>가로 막대 위의 위쪽 삼각형 (번호 9, 그룹 131). 쓰임 미확인.</summary>
    BarUp = 9,
    /// <summary>속이 찬 아래쪽 삼각형 두 개 (번호 10, 그룹 117). 쓰임 미확인.</summary>
    DownFilled = 10,
    /// <summary>금지 표시 위아래에 삼각형 (번호 11, 그룹 130). 쓰임 미확인.</summary>
    ForbiddenVertical = 11,
    /// <summary>윤곽만 있는 아래쪽 삼각형 두 개 (번호 12, 그룹 129). 쓰임 미확인.</summary>
    DownOutline = 12,
    /// <summary>움켜쥐는 손 (번호 13, 그룹 132). 그림과 핫스팟이 번호 8과 같다. 쓰임 미확인.</summary>
    GrabAlternate = 13,
    /// <summary>받침 위의 위쪽 삼각형 (번호 14, 그룹 133). 쓰임 미확인.</summary>
    PedestalUp = 14,
    /// <summary>가로 막대를 지나는 상하 화살표 (번호 15, 그룹 134). 쓰임 미확인.</summary>
    BarVertical = 15,
    /// <summary>가리키는 손 (번호 16, 그룹 136). 쓰임 미확인.</summary>
    PointingHand = 16,
    /// <summary>번개 모양 날이 달린 단검 (번호 17, 그룹 141). 쓰임 미확인.</summary>
    Dagger = 17,
    /// <summary>선택한 사제의 지면 이동 목표: × (번호 18, 그룹 148).</summary>
    Move = 18,
}

/// <summary>
/// 원본 커서 표: 커서 번호 → 실행 파일의 커서 그룹(RT_GROUP_CURSOR) 번호 → 그 그룹의 그림(RT_CURSOR) 번호.
/// 클론은 그림 번호로 동봉 파일 <c>cursors/RT_CURSOR_&lt;번호&gt;.bin</c> 을 읽는다.
/// 사용법: <c>GameCursors.ImageResource(GameCursor.Move)</c> → 20, <c>GameCursors.All</c> 로 18가지를 번호순으로 돈다.
/// </summary>
/// <remarks>
/// 그룹 번호는 원본 10.78 의 표 005423a8(CD판 00516c60, 기준 구현 cpppj/src/client/Cursor.h)이고,
/// 그림 번호는 originals/Netstorm.exe 의 RT_GROUP_CURSOR 리소스에서 읽은 값이다 (그룹마다 그림 하나, 32×32 단색).
/// 2026-10-10 추가 (LEFT_JOBS.dotnetpj.md 5-4).
/// </remarks>
public static class GameCursors
{
    /// <summary>원본 커서 모양의 수 (번호 1~18).</summary>
    public const int Count = 18;

    /// <summary>번호 순서(색인 = 번호 − 1)의 커서 그룹 리소스 번호.</summary>
    private static readonly int[] Groups =
        [113, 110, 111, 108, 107, 109, 115, 116, 131, 117, 130, 129, 132, 133, 134, 136, 141, 148];

    /// <summary>번호 순서(색인 = 번호 − 1)의 커서 그림 리소스 번호.</summary>
    private static readonly int[] Images = [8, 6, 7, 4, 3, 5, 9, 10, 14, 11, 13, 12, 15, 16, 17, 18, 19, 20];

    /// <summary>원본 번호 순서의 모든 커서 모양.</summary>
    public static IReadOnlyList<GameCursor> All { get; } = [.. Enumerable.Range(1, Count).Select(number => (GameCursor)number)];

    /// <summary>커서 모양의 그룹 리소스 번호 (RT_GROUP_CURSOR).</summary>
    /// <param name="cursor">커서 모양 (번호 1~18)</param>
    public static int GroupResource(GameCursor cursor) => Groups[Index(cursor)];

    /// <summary>커서 모양의 그림 리소스 번호 (RT_CURSOR). 동봉 파일 이름에 쓴다.</summary>
    /// <param name="cursor">커서 모양 (번호 1~18)</param>
    public static int ImageResource(GameCursor cursor) => Images[Index(cursor)];

    /// <summary>커서 번호를 표 색인으로 바꾼다. 번호 0 과 18 초과는 원본처럼 거부한다.</summary>
    /// <param name="cursor">커서 모양</param>
    private static int Index(GameCursor cursor)
    {
        int number = (int)cursor;
        if (number < 1 || number > Count)
        {
            throw new ArgumentOutOfRangeException(nameof(cursor), cursor, "원본 커서 번호는 1~18 입니다.");
        }
        return number - 1;
    }
}
