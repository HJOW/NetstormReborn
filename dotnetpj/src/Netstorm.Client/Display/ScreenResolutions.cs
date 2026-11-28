namespace Netstorm.Client.Display;

/// <summary>
/// 해상도 메뉴가 고르는 논리 화면 크기 목록. 원본 4:3 세 항목과 Windows 10/11 호환 출력의 16:9·16:10 추가 선택을 두 메뉴에서 공유한다.
/// 모니터 장치 모드를 바꾸는 목록이 아니라 실제 논리 화면 크기다.
/// 근거: cpppj/src/client/ScreenResolutions.h. (2026-10-28 R1에서 이식)
/// </summary>
public static class ScreenResolutions
{
    /// <summary>
    /// 앞 세 항목은 원본 순서다. 뒤 네 항목은 최대 폭1280의16:9/16:10/4:3/5:4다.
    /// </summary>
    public static readonly (uint Width, uint Height)[] Choices =
    [
        (640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1280, 960), (1280, 1024),
    ];

    /// <summary>원문 몸체의 기본 입력/조회 순서를 유지하는 세 4:3 행이다.</summary>
    public static ReadOnlySpan<(uint Width, uint Height)> Original => Choices.AsSpan(0, 3);

    /// <summary>같은1280 입력 폭에서 유지하는 네 화면비다.</summary>
    public static ReadOnlySpan<(uint Width, uint Height)> Compatibility => Choices.AsSpan(3, 4);
    /// <summary>실제 FSR 장치가 있을 때만 허용하는 세 출력 폭/네 화면비의12행이다.</summary>
    public static readonly (uint Width, uint Height)[] FsrChoices =
    [
        (1920, 1080), (1920, 1200), (1920, 1440), (1920, 1536),
        (2560, 1440), (2560, 1600), (2560, 1920), (2560, 2048),
        (3840, 2160), (3840, 2400), (3840, 2880), (3840, 3072),
    ];
    /// <summary>FSR 입력의 고정 폭이다. 높이는 목표 화면비에서 계산한다.</summary>
    public const uint FsrLogicalWidth = 1280;
    /// <summary>FSR16:9 입력의 기본 높이다. 다른 화면비는 LogicalSize가 계산한다.</summary>
    public const uint FsrLogicalHeight = 720;
    /// <summary>원문과 같은 목록 순서로 FSR 전용 표시 크기를 판정한다.</summary>
    public static bool IsFsr(long width, long height)
    {
        // 폭과 높이를 함께 비교하며 임의의 큰 화면을 FSR로 간주하지 않는다.
        foreach (var size in FsrChoices) if (size.Width == width && size.Height == height) return true;
        return false;
    }
    /// <summary>기본7행과 실제 장치가 허용한 FSR12행만 지원한다.</summary>
    public static bool IsSupported(long width, long height, bool fsrAvailable = false)
    {
        // 원본 세 행과 호환 네 행의 순서를 유지한다.
        foreach (var size in Choices) if (size.Width == width && size.Height == height) return true;
        return fsrAvailable && IsFsr(width, height);
    }
    /// <summary>FSR 표시 크기를 같은 화면비의1280 입력으로 바꾸고 일반 크기는 그대로 반환한다.</summary>
    public static (uint Width, uint Height) LogicalSize(long width, long height) => IsFsr(width, height)
        ? (FsrLogicalWidth, unchecked((uint)(height * FsrLogicalWidth / width))) : (unchecked((uint)width), unchecked((uint)height));
    /// <summary>지원되지 않는 작은 크기는1024×768, 옛 큰 크기는 가장 가까운1280 화면비로 복구한다.</summary>
    public static (uint Width, uint Height) Normalize(long width, long height, bool fsrAvailable)
    {
        if (IsSupported(width, height, fsrAvailable)) return (unchecked((uint)width), unchecked((uint)height));
        if (width <= 0 || height <= 0 || (width <= 1280 && height <= 1024)) return (1024, 768);
        (uint Width, uint Height) best = (1280, 720); double distance = 1e100;
        // 동률은 먼저 나온 화면비를 유지하며 CPP와 같은 double 연산 순서로 비교한다.
        foreach (var size in Compatibility)
        {
            double difference = (double)height * 1280 / width - size.Height;
            double absolute = difference < 0 ? -difference : difference;
            if (absolute < distance) { distance = absolute; best = size; }
        }
        return best;
    }
}
