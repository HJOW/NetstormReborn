namespace Netstorm.Assets;

/// <summary>
/// 원본 생산 창(덱) 항목의 색 변환표를 만든다. 원본 팔레트 준비 함수 FUN_0043c010 이
/// 팔레트마다 계산하는 256바이트 표 중 생산 창이 쓰는 두 개다.
/// <list type="bullet">
/// <item><description>어둡게(DAT_00556438): RGB를 모두 0.6배 한 색과 L1 거리가 가장 가까운 팔레트 색.
/// 배치 뒤 재충전 중인 유닛과 집고 있는 다리 조각(그리기 모드 1)에 쓴다.</description></item>
/// <item><description>빨갛게(DAT_00556038): R만 2.1배(최대 1) 한 색과 유클리드 거리가 가장 가까운 팔레트 색.
/// Storm Power 가 비용보다 적은 유닛(그리기 모드 2)에 쓴다.</description></item>
/// </list>
/// 원본은 거리가 같으면 앞선 인덱스를 유지한다(엄격한 &lt; 비교).
/// </summary>
public static class ProductionTintRemap
{
    /// <summary>어둡게 표의 RGB 배율 (VA 0x506700 = 0.6)</summary>
    public const double DarkenFactor = 0.6;

    /// <summary>빨갛게 표의 R 배율 (VA 0x506708 = 2.1, 결과는 1.0 으로 자른다)</summary>
    public const double RedFactor = 2.1;

    /// <summary>재충전 중·집은 항목의 어둡게 변환표를 만든다.</summary>
    /// <param name="palette">게임 팔레트</param>
    public static byte[] Darkened(Palette palette)
    {
        var table = new byte[Palette.ColorCount];
        // 팔레트 색마다 0.6배 색에 가장 가까운 색을 찾는다
        for (int source = 0; source < Palette.ColorCount; source++)
        {
            (double r, double g, double b) = Unit(palette[source]);
            table[source] = Nearest(palette, r * DarkenFactor, g * DarkenFactor, b * DarkenFactor, euclidean: false);
        }
        return table;
    }

    /// <summary>Storm Power 부족 항목의 빨갛게 변환표를 만든다.</summary>
    /// <param name="palette">게임 팔레트</param>
    public static byte[] Reddened(Palette palette)
    {
        var table = new byte[Palette.ColorCount];
        // 팔레트 색마다 R 을 키운 색에 가장 가까운 색을 찾는다
        for (int source = 0; source < Palette.ColorCount; source++)
        {
            (double r, double g, double b) = Unit(palette[source]);
            table[source] = Nearest(palette, Math.Min(1.0, r * RedFactor), g, b, euclidean: true);
        }
        return table;
    }

    /// <summary>목표 색에 가장 가까운 팔레트 인덱스 (거리가 같으면 앞선 인덱스)</summary>
    private static byte Nearest(Palette palette, double r, double g, double b, bool euclidean)
    {
        int best = 0;
        double bestDistance = double.MaxValue;
        // 256색 전체를 원본처럼 차례로 비교한다
        for (int index = 0; index < Palette.ColorCount; index++)
        {
            (double cr, double cg, double cb) = Unit(palette[index]);
            double distance = euclidean
                ? (r - cr) * (r - cr) + (g - cg) * (g - cg) + (b - cb) * (b - cb)
                : Math.Abs(r - cr) + Math.Abs(g - cg) + Math.Abs(b - cb);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = index;
            }
        }
        return (byte)best;
    }

    /// <summary>8비트 RGB 를 0~1 범위로 바꾼다 (원본 팔레트 구조체의 RGB 실수값)</summary>
    private static (double R, double G, double B) Unit(Rgb color) => (color.R / 255.0, color.G / 255.0, color.B / 255.0);
}
