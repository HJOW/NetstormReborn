namespace Netstorm.Client.Display;

/// <summary>
/// 원본 생산 아이콘의 비활성 및 선행 출력 색표.
/// 근거: cpppj/src/client/ProductionColorMaps.h. (R2c, 2026-10-28)
/// </summary>
public sealed class ProductionColorMaps
{
    /// <summary>비활성 표. 소스 빨강만 2.1배 한 제곱 거리다.</summary>
    public byte[] Disabled { get; set; } = new byte[256];

    /// <summary>선행 표.</summary>
    public byte[] Underlay { get; set; } = new byte[256];
}

/// <summary>
/// 원본은 소스 빨강만 확대한 뒤 1로 제한한다. 선행 표는 원본 memset의 0이다.
/// 근거: cpppj/src/client/ProductionColorMaps.cpp 의 BuildProductionColorMaps. (R2c, 2026-10-28)
/// </summary>
public static class ProductionColorMapBuilder
{
    /// <summary>실제 원본 double 정규화다.</summary>
    private const double Normalize = 1.0 / 255.0;

    /// <summary>비활성 표의 빨강 확대 곱.</summary>
    private const double DisabledRed = 2.1;

    /// <summary>
    /// 후보 번호는 정렬하지 않고 팔레트 순서대로 유지한다. blue→green→red 제곱 합을 float 최소값과 엄격 비교한다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    public static ProductionColorMaps BuildProductionColorMaps(ReadOnlySpan<uint> logical)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        var result = new ProductionColorMaps();
        var rgbR = new double[256];
        var rgbG = new double[256];
        var rgbB = new double[256];
        // 원본 ColorSort의 정규화 성분이다
        for (int i = 0; i < 256; i++)
        {
            rgbR[i] = (logical[i] & 255) * Normalize;
            rgbG[i] = ((logical[i] >> 8) & 255) * Normalize;
            rgbB[i] = ((logical[i] >> 16) & 255) * Normalize;
        }
        for (int i = 0; i < 256; i++)
        {
            double target = Math.Min(1.0, rgbR[i] * DisabledRed);
            float minimum = 1e6f;
            for (int candidate = 0; candidate < 256; candidate++)
            {
                double dB = rgbB[i] - rgbB[candidate];
                double dG = rgbG[i] - rgbG[candidate];
                double dR = target - rgbR[candidate];
                double distance = dB * dB + dG * dG + dR * dR;
                if (distance < minimum)
                {
                    minimum = (float)distance;
                    result.Disabled[i] = (byte)candidate;
                }
            }
        }
        return result;
    }
}
