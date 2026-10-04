namespace Netstorm.Core.Rules;

/// <summary>화면 픽셀 사각형 (왼쪽·위·폭·높이).</summary>
/// <param name="X">왼쪽 끝</param>
/// <param name="Y">위쪽 끝</param>
/// <param name="Width">폭</param>
/// <param name="Height">높이</param>
public readonly record struct PixelRect(int X, int Y, int Width, int Height);

/// <summary>
/// 선택한 오브젝트의 괄호 표시와 체력 막대의 모양 규칙. 원본 렌더러(0x498841 부근)와 막대 함수 <c>FUN_004972f0</c> 를 옮겼다.
/// 기준은 오브젝트가 지금 그리는 셰이프 프레임의 그림 상자(헤더의 폭·높이·기준점)다.
/// <list type="bullet">
/// <item><description>괄호: 상자의 아래 두 모서리에만, 2픽셀 두께. 가로 길이 = 폭 ÷ 4, 세로 길이 = (높이 − 1) ÷ 4.</description></item>
/// <item><description>막대: 상자 위 끝 바로 위. 검은 바탕 안에 색 막대. 추가 막대는 3픽셀씩 위로 쌓인다.</description></item>
/// <item><description>체력 색: 기본 초록 (95,188,92), 최대의 절반 이하 노랑 (255,255,22), 4분의 1 이하 빨강 (255,22,22).</description></item>
/// </list>
/// 2026-10-03 TEST01 녹화의 선택 장면(연청록 괄호, 그림 위 초록 막대)과 모양이 맞는다.
/// </summary>
public static class SelectionMarks
{
    /// <summary>색 번호(0~8)별 괄호의 팔레트 번호 (exe VA 0x5319c4). 파랑(1)은 84번 = (125,176,179) 연청록이다.</summary>
    private static readonly byte[] BracketPaletteIndexes = [84, 84, 249, 184, 140, 150, 112, 156, 222];

    /// <summary>체력이 절반을 넘을 때의 막대 색 (exe 0x5b5dc4).</summary>
    public static readonly (byte R, byte G, byte B) HealthyColor = (0x5f, 0xbc, 0x5c);

    /// <summary>체력이 절반 이하일 때의 막대 색 (exe 0x59afd8).</summary>
    public static readonly (byte R, byte G, byte B) HurtColor = (0xff, 0xff, 0x16);

    /// <summary>체력이 4분의 1 이하일 때의 막대 색 (exe 0x5b5e58).</summary>
    public static readonly (byte R, byte G, byte B) CriticalColor = (0xff, 0x16, 0x16);

    /// <summary>막대 하나가 위로 쌓이는 간격(픽셀).</summary>
    private const int BarPitch = 3;

    /// <summary>플레이어 색 번호의 괄호 팔레트 번호 (범위 밖은 중립 0번 값).</summary>
    /// <param name="color">색 번호 (<see cref="PlayerColors"/>)</param>
    public static byte BracketPaletteIndex(int color) =>
        BracketPaletteIndexes[color >= 0 && color < BracketPaletteIndexes.Length ? color : 0];

    /// <summary>괄호의 가로 길이(폭 ÷ 4)와 세로 길이((높이 − 1) ÷ 4). 원본처럼 정수 나눗셈이다.</summary>
    /// <param name="width">그림 상자 폭</param>
    /// <param name="height">그림 상자 높이</param>
    public static (int Horizontal, int Vertical) BracketArms(int width, int height) => (width / 4, (height - 1) / 4);

    /// <summary>체력에 맞는 막대 색. 원본처럼 정수 체력과 정수 나눗셈으로 비교한다.</summary>
    /// <param name="hitPoints">현재 체력</param>
    /// <param name="maxHitPoints">최대 체력</param>
    public static (byte R, byte G, byte B) HealthColor(int hitPoints, int maxHitPoints)
    {
        if (hitPoints <= maxHitPoints / 4) return CriticalColor;
        return hitPoints <= maxHitPoints / 2 ? HurtColor : HealthyColor;
    }

    /// <summary>막대의 검은 바탕: 상자보다 좌우 1픽셀 넓고 위 끝에서 5픽셀 위부터 4픽셀 높이.</summary>
    /// <param name="left">그림 상자 왼쪽 끝</param>
    /// <param name="top">그림 상자 위쪽 끝</param>
    /// <param name="width">그림 상자 폭</param>
    /// <param name="slot">막대 순번 (0 = 체력, 그 위로 1, 2…)</param>
    public static PixelRect BarBackground(int left, int top, int width, int slot = 0) =>
        new(left - 1, top - slot * BarPitch - 5, width + 2, 4);

    /// <summary>막대의 채움: 바탕 안쪽 2픽셀 높이, 길이 = 값 × 폭 ÷ 최대 (값은 0~최대로 자른다).</summary>
    /// <param name="left">그림 상자 왼쪽 끝</param>
    /// <param name="top">그림 상자 위쪽 끝</param>
    /// <param name="width">그림 상자 폭</param>
    /// <param name="value">현재 값</param>
    /// <param name="maximum">최대 값 (0 이하이면 1로 본다)</param>
    /// <param name="slot">막대 순번</param>
    public static PixelRect BarFill(int left, int top, int width, int value, int maximum, int slot = 0)
    {
        int clamped = Math.Clamp(value, 0, Math.Max(0, maximum));
        return new PixelRect(left, top - slot * BarPitch - 4, clamped * width / Math.Max(1, maximum), 2);
    }
}
