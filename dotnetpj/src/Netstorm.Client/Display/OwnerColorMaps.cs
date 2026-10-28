using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>
/// owner 색 번호 0~8 각각의 현재 256색 변환표. 0번은 항등이다.
/// 근거: cpppj/src/client/OwnerColorMaps.h 의 OwnerColorMaps. (R2c, 2026-10-28)
/// </summary>
public sealed class OwnerColorMaps
{
    /// <summary>소유자 9명의 변환표. 각 표는 256바이트다.</summary>
    public byte[][] Tables { get; } = [new byte[256], new byte[256], new byte[256], new byte[256], new byte[256], new byte[256], new byte[256], new byte[256], new byte[256]];
}

/// <summary>
/// 원본의 항등 9색표와 일반 플레이어 색 띠·다리 특례, 전용 5종의 색 표를 복원한다.
/// 근거: cpppj/src/client/OwnerColorMaps.cpp (원본 0043b950·0043ba00·0043b750). (R2c, 2026-10-28)
/// </summary>
public static class OwnerColorMapBuilder
{
    /// <summary>
    /// 보호 10.78 PE의 005319e8..00531b07 원문. 첫 8개 0은 색 0 후보이며 변환표 0 자체는 항등이다.
    /// </summary>
    private static readonly uint[] Candidates =
    [
        0, 0, 0, 0, 0, 0, 0, 0, 81, 80, 76, 145, 146, 147, 4, 10, 94, 100, 170, 169, 204, 249, 217, 227,
        195, 93, 185, 35, 31, 149, 70, 15, 207, 131, 133, 128, 136, 2, 142, 143, 189, 190, 253, 150, 148, 70, 14, 10,
        113, 218, 219, 3, 52, 59, 67, 213, 9, 85, 155, 156, 6, 72, 144, 143, 220, 221, 222, 223, 224, 63, 227, 213,
    ];

    /// <summary>원본 정적 DWORD 표의 읽기 전용 보기다. 반환 저장소는 프로그램 전체 수명이다.</summary>
    public static IReadOnlyList<uint> OriginalOwnerColorCandidates() => Candidates;

    /// <summary>
    /// 0043bc40/00443be30이 넘기는 전용 소스 순서. 94·97의 지면 소스, 111·141·137의 소스다. 종료 인자 ff는 포함하지 않는다.
    /// 등록되지 않은 타입은 거부한다.
    /// </summary>
    /// <param name="type">자산 타입 번호(111·141·137·94·97).</param>
    public static IReadOnlyList<uint> SpecialOwnerColorSources(uint type)
    {
        uint[] surface = [44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 3, 177, 200, 203, 224, 220, 221, 222, 223, 114, 201, 226];
        uint[] first = [86, 188, 85, 247, 84, 31, 34, 75, 82, 74, 83, 156, 153, 70, 72, 71, 73, 152, 13, 144];
        uint[] second = [87, 88, 85, 86, 84, 156, 75, 82, 153, 74, 73, 83, 155, 72, 71, 44, 152, 154, 70, 13, 144];
        uint[] third = [218, 219, 220, 52, 53, 54, 55, 56, 57, 58, 59, 66, 161, 210, 19, 21, 214, 141, 18];
        return type switch
        {
            111 => first,
            141 => second,
            137 => third,
            94 or 97 => surface,
            _ => throw new ArgumentException("전용 소유자 색표 타입 범위 오류", nameof(type)),
        };
    }

    /// <summary>
    /// 항등 초기화 뒤 228~245의 띠와 현재 다리 타입의 색 2·7 특례를 적용한다.
    /// 원문에서 색 0 행은 변경하지 않고 1~8행만 실제 띠를 바꾼다.
    /// </summary>
    /// <param name="type">자산 타입 번호.</param>
    /// <param name="bridgeType">다리 타입 번호.</param>
    /// <param name="candidates">실제 DWORD 후보. 없으면 원문 표를 쓴다. 원문처럼 낮은 바이트만 변환표에 저장한다.</param>
    public static OwnerColorMaps BuildStandardOwnerColorMaps(uint type, uint bridgeType, IReadOnlyList<uint>? candidates = null)
    {
        candidates ??= Candidates;
        if (candidates.Count < 72)
        {
            throw new ArgumentException("소유자 색표 후보는 72개여야 합니다", nameof(candidates));
        }
        var result = new OwnerColorMaps();
        // 모든 비대상 색·색 0의 원래 번호를 유지한다
        foreach (byte[] row in result.Tables)
        {
            for (int color = 0; color < 256; color++)
            {
                row[color] = (byte)color;
            }
        }
        // 플레이어 색 번호 1~8 각각의 명도 후보를 사용한다
        for (int owner = 1; owner < 9; owner++)
        {
            // 228~237은 밝은 0, 238~245는 역순 7~0이며 다리의 색 2·7은 후반 명도를 고정한다
            for (int color = 228; color <= 245; color++)
            {
                uint shade = color < 238 ? 0u : (uint)(245 - color);
                if (color >= 238 && type == bridgeType && (owner == 2 || owner == 7))
                {
                    shade = owner == 2 ? 4u : 3u;
                }
                result.Tables[owner][color] = (byte)candidates[(int)(owner * 8 + shade)];
            }
        }
        return result;
    }

    /// <summary>
    /// 실제 지정 소스만 바꾸며 등록되지 않은 소스·중립 0표는 항등을 유지한다. 장치 없는 독립 adapter다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    /// <param name="type">자산 타입 번호(111·141·137·94·97).</param>
    /// <param name="candidates">실제 DWORD 후보. 없으면 원문 표를 쓴다.</param>
    public static OwnerColorMaps BuildSpecialOwnerColorMaps(ReadOnlySpan<uint> logical, uint type, IReadOnlyList<uint>? candidates = null)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        const double normalize = 1.0 / 255.0;
        var light = new double[256];
        // 기존 RGB→명도 계산 순서를 그대로 유지한다
        for (int i = 0; i < 256; i++)
        {
            double red = (logical[i] & 255) * normalize;
            double green = ((logical[i] >> 8) & 255) * normalize;
            double blue = ((logical[i] >> 16) & 255) * normalize;
            light[i] = (Math.Max(red, Math.Max(green, blue)) + Math.Min(red, Math.Min(green, blue))) * 0.5;
        }
        return BuildSpecialOwnerFromLightness(i => light[i], type, candidates);
    }

    /// <summary>
    /// 실제 소비자는 같은 primary의 현재 +8 double을 읽고 RGB에서 명도를 재계산하지 않는다.
    /// </summary>
    /// <param name="primary">완성한 ColorSort primary.</param>
    /// <param name="type">자산 타입 번호(111·141·137·94·97).</param>
    /// <param name="candidates">실제 DWORD 후보. 없으면 원문 표를 쓴다.</param>
    public static OwnerColorMaps BuildSpecialOwnerColorMaps(RawColorSortPrimary primary, uint type, IReadOnlyList<uint>? candidates = null)
    {
        ArgumentNullException.ThrowIfNull(primary);
        return BuildSpecialOwnerFromLightness(primary.Lightness, type, candidates);
    }

    /// <summary>
    /// 기존 소스·후보·보정 본문을 공유한다. 실제 변환은 소유자 1~8에만 쓰고 동일 거리 후보는 앞의 번호를 유지한다.
    /// </summary>
    private static OwnerColorMaps BuildSpecialOwnerFromLightness(Func<int, double> light, uint type, IReadOnlyList<uint>? candidates)
    {
        candidates ??= Candidates;
        if (candidates.Count < 72)
        {
            throw new ArgumentException("소유자 색표 후보는 72개여야 합니다", nameof(candidates));
        }
        double[] offsets = [-0.1, -0.17, 0.15, 0, 0, 0.05, -0.2, 0];
        const double otherOffset = -0.15000000596046448;
        IReadOnlyList<uint> sources = SpecialOwnerColorSources(type);
        var result = new OwnerColorMaps();
        // 중립을 포함한 9표를 원본 0043b950처럼 항등으로 시작한다
        foreach (byte[] map in result.Tables)
        {
            for (int i = 0; i < 256; i++)
            {
                map[i] = (byte)i;
            }
        }
        for (int owner = 1; owner < 9; owner++)
        {
            double offset = type == 97 ? offsets[owner - 1] : (type == 141 ? otherOffset : 0.0);
            // 실제 호출자의 지정 소스 순서대로 같은 표를 수정한다
            foreach (uint source in sources)
            {
                double target = light((int)source) + offset;
                uint selected = 0;
                double minimum = Math.Min(99.0, Math.Abs(target - light((int)candidates[owner * 8])));
                // 후보 1~7은 엄격히 더 가까울 때만 교체하며 float로 최소값을 좁히지 않는다
                for (int shade = 1; shade < 8; shade++)
                {
                    double distance = Math.Abs(target - light((int)candidates[owner * 8 + shade]));
                    if (distance < minimum)
                    {
                        minimum = distance;
                        selected = (uint)shade;
                    }
                }
                result.Tables[owner][source] = (byte)candidates[(int)(owner * 8 + selected)];
            }
        }
        return result;
    }
}
