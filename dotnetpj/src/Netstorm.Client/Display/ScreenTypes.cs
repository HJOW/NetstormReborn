namespace Netstorm.Client.Display;

/// <summary>
/// 화면 좌표의 사각형. Right·Bottom은 포함하지 않는다(원본 004a1800의 인자). 값 비교가 가능한 불변 구조체다.
/// 근거: cpppj/src/client/Screen.h 의 ScreenRect. (2026-10-28 R1에서 추가)
/// </summary>
/// <param name="Left">사각형의 왼쪽 경계.</param>
/// <param name="Top">사각형의 위쪽 경계.</param>
/// <param name="Right">사각형의 오른쪽 경계(포함하지 않음).</param>
/// <param name="Bottom">사각형의 아래쪽 경계(포함하지 않음).</param>
public readonly record struct ScreenRect(int Left, int Top, int Right, int Bottom);

/// <summary>화면 또는 창의 좌표 한 점. 근거: cpppj/src/client/Screen.h 의 ScreenPoint. (2026-10-28 R1에서 추가)</summary>
/// <param name="X">X 좌표.</param>
/// <param name="Y">Y 좌표.</param>
public readonly record struct ScreenPoint(int X, int Y);

/// <summary>
/// Windows RGBQUAD와 같은 순서의 색(파랑·초록·빨강·예약). 현재 팔레트와 저장 팔레트가 이 형식으로 256개씩 있다.
/// 근거: cpppj/src/client/Screen.h 의 ScreenColor. (2026-10-28 R1에서 추가)
/// </summary>
/// <param name="Blue">RGB 색의 파랑 성분(0~255).</param>
/// <param name="Green">RGB 색의 초록 성분(0~255).</param>
/// <param name="Red">RGB 색의 빨강 성분(0~255).</param>
/// <param name="Reserved">예약 바이트. BGRX 팔레트 파일의 네 번째 바이트를 보존한다.</param>
public readonly record struct ScreenColor(byte Blue, byte Green, byte Red, byte Reserved);

/// <summary>팔레트 색 하나의 RGB 성분. 근거: cpppj/src/client/Screen.h 의 PaletteColor. (2026-10-28 R1에서 추가)</summary>
/// <param name="Red">RGB 색의 빨강 성분(0~255).</param>
/// <param name="Green">RGB 색의 초록 성분(0~255).</param>
/// <param name="Blue">RGB 색의 파랑 성분(0~255).</param>
public readonly record struct PaletteColor(byte Red, byte Green, byte Blue);

/// <summary>
/// 화면 모드 플래그(원본 DAT_005c78d4). 설정 windowScreenFlags·workingFullScreenFlags의 값이며 모드가 정해지면 global.dd… 설정으로도 적힌다.
/// 근거: cpppj/src/client/Screen.h 의 ScreenMode. (2026-10-28 R1에서 추가)
/// </summary>
public static class ScreenMode
{
    /// <summary>DirectDraw 전체화면(global.ddFullScreen). 이 클론에서는 비율 유지 borderless 출력을 뜻한다.</summary>
    public const uint FullScreen = 0x01;

    /// <summary>그리기 표면이 DIB 섹션(global.ddBackBufferDib). 이 클론에서는 8비트 인덱스 버퍼를 뜻한다.</summary>
    public const uint BackBufferDib = 0x02;

    /// <summary>페이지 플리핑(global.ddFlipping). 원본 독점 DirectDraw 전용이라 이 클론은 요청하지 않는다.</summary>
    public const uint Flipping = 0x04;

    /// <summary>구름("parallax without clouds" 검사의 짝).</summary>
    public const uint Clouds = 0x08;

    /// <summary>구름 시차(global.ddParallax).</summary>
    public const uint Parallax = 0x10;

    /// <summary>구름 움직임.</summary>
    public const uint Animating = 0x20;

    /// <summary>커서를 직접 그림(global.ddSoftwareMouse).</summary>
    public const uint SoftwareMouse = 0x40;

    /// <summary>모드를 정할 때 항상 켜는 비트.</summary>
    public const uint ModeSet = 0x80;

    /// <summary>DirectDraw가 없는데 전체화면을 요구하면 원본이 대신 쓰는 값(004a4fe0의 param_1 = 10).</summary>
    public const uint FallbackWindowed = BackBufferDib | Clouds;
}
