// 원문 위치 음향의 투영·signed 범위·DWORD 감김·idiv를 보존한다.
namespace Netstorm.Client.Audio;

/// <summary>CPP SoundToScreen/OnScreen/Pan/Volume과 공통 카메라 투영의 순수 계산이다.</summary>
public static class SoundGeometry
{
    /// <summary>원문 CRT의64비트 절삭 뒤 하위DWORD를 보존하며 비유한/범위 밖은0이다.</summary>
    private static int FtolLow(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0) return 0;
        return unchecked((int)(long)value);
    }
    /// <summary>같은 SoundView snapshot의 현재 원점을 축별 ftol 뒤 DWORD로 뺀다.</summary>
    public static SoundPoint ToScreen(SoundView view, float x, float y) => new(unchecked(FtolLow((double)x * 16 + 0.5) - view.CameraX), unchecked(FtolLow((double)y * 11 + 0.5) - view.CameraY));
    /// <summary>원문의 signed 경계에 좌우40/상하30 여유를 DWORD로 더하고 비교한다.</summary>
    public static bool OnScreen(SoundView view, SoundPoint point) => point.X >= unchecked(view.Left - 40) && point.X <= unchecked(view.Right + 40) && point.Y >= unchecked(view.Top - 30) && point.Y <= unchecked(view.Bottom + 30);
    /// <summary>범위 뺄셈을 DWORD로 감고0쪽 절삭으로 반을 나눈다.</summary>
    private static int Half(int low, int high) { int span = unchecked(high - low); return (span - (span >> 31)) >> 1; }
    /// <summary>원문 idiv의0/몫 넘침 예외를 명시하고0방향으로 나눈다.</summary>
    private static int Divide(int dividend, int divisor)
    {
        if (divisor == 0 || (divisor == -1 && dividend == int.MinValue)) throw new ArgumentException("소리 위치 계산의 화면 폭이 너무 작습니다");
        return dividend / divisor;
    }
    /// <summary>위치 계산 결과를 원문의1/100dB 범위로 자른다.</summary>
    private static int Limit(int value) => Math.Clamp(value, SoundConstants.Minimum, SoundConstants.Maximum);
    /// <summary>현재 가로 중심 거리의 signed 제곱 비율과 좌우 교환을 원문DWORD 곱으로 계산한다.</summary>
    public static int Pan(SoundView view, SoundPoint point, bool swap)
    {
        int half = Half(view.Left, view.Right); uint distance = unchecked((uint)(point.X - view.Left - half)); if (swap) distance = unchecked(0u - distance);
        int offset = unchecked((int)distance); if (offset == 0) return 0;
        int scaled = unchecked((int)(distance * distance * (uint)(offset < 0 ? SoundConstants.Minimum : SoundConstants.Maximum)));
        return Limit(Divide(scaled, unchecked(half * half)));
    }
    /// <summary>가로/세로 중심 거리 제곱의 합을 가로 절반 제곱으로 나누며 가운데에서도 나눗셈한다.</summary>
    public static int Volume(SoundView view, SoundPoint point)
    {
        int hw = Half(view.Left, view.Right), hh = Half(view.Top, view.Bottom); uint dx = unchecked((uint)(point.X - view.Left - hw)), dy = unchecked((uint)(point.Y - hh - view.Top));
        int scaled = unchecked((int)((dy * dy + dx * dx) * (uint)-1000)); return Limit(Divide(scaled, unchecked(hw * hw)));
    }
}
