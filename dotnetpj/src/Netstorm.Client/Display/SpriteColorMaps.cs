namespace Netstorm.Client.Display;

/// <summary>
/// 원본 일반 자산 출력의 추상/건설/그림자 소스 색 변환표. 세 표는 동일한 파일 팔레트에서 만들며 일시적인 날씨 RGB 변경에는 다시 계산하지 않는다.
/// 근거: cpppj/src/client/SpriteColorMaps.h. (R2c, 2026-10-28)
/// </summary>
public sealed class SpriteColorMaps
{
    /// <summary>밝음 표. 명암 표의 밝은 쪽을 그대로 쓴다.</summary>
    public byte[] Bright { get; set; } = new byte[256];

    /// <summary>건설(어두움) 표. 명암 표의 어두운 쪽을 그대로 쓴다.</summary>
    public byte[] Dead { get; set; } = new byte[256];

    /// <summary>그림자 표. 소스 팔레트 번호를 제곱 거리로 변환한다.</summary>
    public byte[] Shadow { get; set; } = new byte[256];
}

/// <summary>
/// 논리 RGB 256색을 원본 거리·후보·float 최소값 순서로 변환한다.
/// 근거: cpppj/src/client/SpriteColorMaps.cpp 의 BuildSpriteColorMaps (원본 0043c010). (R2c, 2026-10-28)
/// </summary>
public static class SpriteColorMapBuilder
{
    /// <summary>원본 정규화 double과 승격한 float 0.6이다.</summary>
    private const double Normalize = 1.0 / 255.0;

    /// <summary>그림자 목표의 어두움 곱.</summary>
    private const double ShadowFactor = 0.6000000238418579;

    /// <summary>
    /// 기존 명암 두 표를 재사용하고 독립 그림자 표를 같은 RGB로 계산한다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    public static SpriteColorMaps BuildSpriteColorMaps(ReadOnlySpan<uint> logical)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        GumpShadeMaps shade = PaletteShade.BuildGumpShadeMaps(logical);
        var result = new SpriteColorMaps { Bright = shade.Bright, Dead = shade.Dark, Shadow = new byte[256] };
        var red = new double[256];
        var green = new double[256];
        var blue = new double[256];
        // 예약 바이트를 제외하고 실제 원본 번호 순서로 정규화한다
        for (int i = 0; i < 256; i++)
        {
            red[i] = (logical[i] & 255) * Normalize;
            green[i] = ((logical[i] >> 8) & 255) * Normalize;
            blue[i] = ((logical[i] >> 16) & 255) * Normalize;
        }
        // 각각의 소스 RGB를 어둡게 만든 뒤 모든 후보를 원본 순서로 검사한다
        for (int i = 0; i < 256; i++)
        {
            double targetR = red[i] * ShadowFactor;
            double targetG = green[i] * ShadowFactor;
            double targetB = blue[i] * ShadowFactor;
            float minimum = 1e6f;
            for (int candidate = 0; candidate < 256; candidate++)
            {
                double dB = targetB - blue[candidate];
                double dG = targetG - green[candidate];
                double dR = targetR - red[candidate];
                double distance = dB * dB + dG * dG + dR * dR;
                if (distance < minimum)
                {
                    minimum = (float)distance;
                    result.Shadow[i] = (byte)candidate;
                }
            }
        }
        // 원본 0043c010 끝에서 18회에 걸쳐 228~245를 항등으로 덮는다. 증분 전 종료값을 사용한다
        for (int i = 228; i <= 245; i++)
        {
            result.Shadow[i] = (byte)i;
        }
        result.Shadow[254] = 254;
        result.Shadow[255] = 255;
        return result;
    }
}
