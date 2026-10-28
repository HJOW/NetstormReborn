namespace Netstorm.Client.Display;

/// <summary>
/// 해상도 메뉴가 고르는 논리 화면 크기 목록. 원본 4:3 세 항목과 Windows 10/11 호환 출력의 16:9·16:10 추가 선택을 두 메뉴에서 공유한다.
/// 모니터 장치 모드를 바꾸는 목록이 아니라 실제 논리 화면 크기다.
/// 근거: cpppj/src/client/ScreenResolutions.h. (2026-10-28 R1에서 이식)
/// </summary>
public static class ScreenResolutions
{
    /// <summary>
    /// 앞 세 항목은 원본 순서다. 뒤 세 항목은 원본과 다른 Windows 10/11 DIB/화면비 지원 요구(AGENTS.md)에 따라 cpppj가 추가한 것을 그대로 따른다.
    /// </summary>
    public static readonly (uint Width, uint Height)[] Choices =
    [
        (640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1366, 768),
    ];

    /// <summary>원문 몸체의 기본 입력/조회 순서를 유지하는 세 4:3 행이다.</summary>
    public static ReadOnlySpan<(uint Width, uint Height)> Original => Choices.AsSpan(0, 3);

    /// <summary>호환 Client만 명시적으로 추가하는 16:9·16:10·DIB 홀수 폭 세 행이다.</summary>
    public static ReadOnlySpan<(uint Width, uint Height)> Compatibility => Choices.AsSpan(3, 3);
}
