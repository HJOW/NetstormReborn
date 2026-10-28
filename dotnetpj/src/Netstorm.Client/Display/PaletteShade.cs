namespace Netstorm.Client.Display;

/// <summary>
/// 원본 GUI 질감의 밝음·어두움 색 번호 변환표. 일시 팔레트 효과와 별도로 파일 팔레트에서 만든다.
/// 근거: cpppj/src/client/PaletteShade.h 의 GumpShadeMaps. (R2a, 2026-10-28)
/// </summary>
public sealed class GumpShadeMaps
{
    /// <summary>밝은 쪽 목표 색 번호 256개.</summary>
    public byte[] Bright { get; } = new byte[256];

    /// <summary>어두운 쪽 목표 색 번호 256개.</summary>
    public byte[] Dark { get; } = new byte[256];
}

/// <summary>
/// R/G/B/flags 순서 논리 팔레트의 RGB를 정규화하고 원본 거리 비교·float32 최솟값 저장으로 명암 표를 만든다.
/// 근거: cpppj/src/client/PaletteShade.cpp 의 BuildGumpShadeMaps (원본 0043c010 / CD 00496530). (R2a, 2026-10-28)
/// </summary>
public static class PaletteShade
{
    /// <summary>원본 ColorSort 생성자에 저장된 double 정규화 계수.</summary>
    private const double Normalize = 1.0 / 255.0;

    /// <summary>원본 명암 생성자의 밝음 곱.</summary>
    private const double BrightFactor = 1.5;

    /// <summary>원본 명암 생성자의 어두움 곱(float 0.6을 double로 승격한 값).</summary>
    private const double DarkFactor = 0.6000000238418579;

    /// <summary>밝음 후보에서 제외하는 구름 색 번호 범위(229~245).</summary>
    private const int BrightExcludeFirst = 229;

    /// <summary>밝음 후보 제외 범위의 끝.</summary>
    private const int BrightExcludeLast = 245;

    /// <summary>
    /// 각 소스 번호마다 두 독립 목표 색을 검색한다. 거리 자체는 double로 비교하고 최소 거리만 float로 좁힌다.
    /// 동일 RGB도 저장 반올림에 따라 뒤 번호를 고를 수 있으므로 비교·저장 순서를 유지한다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    public static GumpShadeMaps BuildGumpShadeMaps(ReadOnlySpan<uint> logical)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        // 예약 바이트를 제외한 원본 R/G/B를 ColorSort와 같은 double 값으로 저장한다
        var red = new double[256];
        var green = new double[256];
        var blue = new double[256];
        for (int i = 0; i < 256; i++)
        {
            red[i] = (logical[i] & 255) * Normalize;
            green[i] = ((logical[i] >> 8) & 255) * Normalize;
            blue[i] = ((logical[i] >> 16) & 255) * Normalize;
        }
        var result = new GumpShadeMaps();
        // 소스 번호마다 두 독립 목표 색을 검색한다
        for (int i = 0; i < 256; i++)
        {
            double brightR = Math.Min(1.0, red[i] * BrightFactor);
            double brightG = Math.Min(1.0, green[i] * BrightFactor);
            double brightB = Math.Min(1.0, blue[i] * BrightFactor);
            double darkR = red[i] * DarkFactor;
            double darkG = green[i] * DarkFactor;
            double darkB = blue[i] * DarkFactor;
            float brightMinimum = 1e6f;
            float darkMinimum = 1e6f;
            for (int candidate = 0; candidate < 256; candidate++)
            {
                double brightDistance = Math.Abs(brightR - red[candidate]) + Math.Abs(brightG - green[candidate]) + Math.Abs(brightB - blue[candidate]);
                double darkDistance = Math.Abs(darkR - red[candidate]) + Math.Abs(darkG - green[candidate]) + Math.Abs(darkB - blue[candidate]);
                if (brightDistance < brightMinimum && (candidate < BrightExcludeFirst || candidate > BrightExcludeLast))
                {
                    brightMinimum = (float)brightDistance;
                    result.Bright[i] = (byte)candidate;
                }
                if (darkDistance < darkMinimum)
                {
                    darkMinimum = (float)darkDistance;
                    result.Dark[i] = (byte)candidate;
                }
            }
        }
        return result;
    }
}
