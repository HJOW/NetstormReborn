namespace Netstorm.Client.Display;

/// <summary>
/// 원본 0043c010의 커서 파괴·불법·글자 그림자 세 색 변환표. 각 표는 현재 파일 논리 팔레트의 256개 소스 번호를 출력 번호로 바꾼다.
/// 근거: cpppj/src/client/PreviewColorMaps.h. (R2c, 2026-10-28)
/// </summary>
public sealed class PreviewColorMaps
{
    /// <summary>파괴 표. 현재 기본 빨강의 하위 바이트로 채운다.</summary>
    public byte[] Destroy { get; set; } = new byte[256];

    /// <summary>불법 표. 빨강만 2.1배/1 제한 후 RGB 제곱 거리로 고른다.</summary>
    public byte[] Illegal { get; set; } = new byte[256];

    /// <summary>글자 그림자 표. 원본처럼 모든 소스 번호를 0으로 바꾼다.</summary>
    public byte[] LabelShadow { get; set; } = new byte[256];
}

/// <summary>
/// 원본 ColorSort와 같은 double 정규 RGB 및 float 최소 거리 저장을 사용한다.
/// 근거: cpppj/src/client/PreviewColorMaps.cpp 의 BuildPreviewColorMaps. (R2c, 2026-10-28)
/// </summary>
public static class PreviewColorMapBuilder
{
    /// <summary>실제 ColorSort와 506708의 double 상수다.</summary>
    private const double Normalize = 1.0 / 255.0;

    /// <summary>불법 표의 빨강 확대 곱.</summary>
    private const double IllegalRed = 2.1;

    /// <summary>
    /// 원본의 blue→green→red 제곱 합과 float 최소값 저장 순서를 보존한다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    /// <param name="red">파괴 표를 채울 현재 기본 빨강(하위 바이트만 쓴다).</param>
    public static PreviewColorMaps BuildPreviewColorMaps(ReadOnlySpan<uint> logical, uint red)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        var result = new PreviewColorMaps();
        Array.Fill(result.Destroy, (byte)red);
        Array.Fill(result.LabelShadow, (byte)0);
        var rgbR = new double[256];
        var rgbG = new double[256];
        var rgbB = new double[256];
        // 예약 바이트를 제외하고 각 성분을 double로 저장한다
        for (int i = 0; i < 256; i++)
        {
            rgbR[i] = (logical[i] & 255) * Normalize;
            rgbG[i] = ((logical[i] >> 8) & 255) * Normalize;
            rgbB[i] = ((logical[i] >> 16) & 255) * Normalize;
        }
        // 각 원본 색의 빨강만 확대하고 나머지 두 성분은 유지한다
        for (int i = 0; i < 256; i++)
        {
            double target = Math.Min(1.0, rgbR[i] * IllegalRed);
            float minimum = 1e6f;
            // 원본은 엄격한 double 비교 뒤 최소 거리만 float로 좁히므로 동률 선택을 단순화하지 않는다
            for (int candidate = 0; candidate < 256; candidate++)
            {
                double dB = rgbB[i] - rgbB[candidate];
                double dG = rgbG[i] - rgbG[candidate];
                double difference = target - rgbR[candidate];
                double distance = dB * dB + dG * dG + difference * difference;
                if (distance < minimum)
                {
                    minimum = (float)distance;
                    result.Illegal[i] = (byte)candidate;
                }
            }
        }
        return result;
    }
}
