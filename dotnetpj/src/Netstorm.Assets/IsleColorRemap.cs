namespace Netstorm.Assets;

/// <summary>원본 전투 화면의 isle 팔레트 변환표를 구성한다 (0043b750, 0043bc40).</summary>
public sealed class IsleColorRemap
{
    /// <summary>변환표 한 행의 팔레트 인덱스 수.</summary>
    private const int RowSize = Palette.ColorCount;

    /// <summary>색상 1~8의 명도 보정값 (VA 0x5066d0 등).</summary>
    private static readonly double[] LightnessOffsets = [-0.1, -0.17, 0.15, 0, 0, 0.05, -0.2, 0];

    /// <summary>각 색상의 목적지 후보 8개 (VA 0x531a08).</summary>
    private static readonly byte[][] TargetColors =
    [
        [81, 80, 76, 145, 146, 147, 4, 10],
        [94, 100, 170, 169, 204, 249, 217, 227],
        [195, 93, 185, 35, 31, 149, 70, 15],
        [207, 131, 133, 128, 136, 2, 142, 143],
        [189, 190, 253, 150, 148, 70, 14, 10],
        [113, 218, 219, 3, 52, 59, 67, 213],
        [9, 85, 155, 156, 6, 72, 144, 143],
        [220, 221, 222, 223, 224, 63, 227, 213]
    ];

    /// <summary>isle의 0x2c~0x40 및 별도 색상 인덱스 (0043bc40).</summary>
    private static readonly byte[] SourceColors =
    [
        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
        3, 0xb1, 200, 0xcb, 0xe0, 0xdc, 0xdd, 0xde, 0xdf, 0x72, 0xc9, 0xe2
    ];

    private readonly byte[][] _tables = new byte[TargetColors.Length + 1][];

    /// <summary>게임 팔레트의 HSL 명도를 기준으로 소유자 색상별 변환표를 만든다.</summary>
    public IsleColorRemap(Palette palette)
    {
        // 0번은 중립색으로 모든 팔레트 인덱스를 그대로 유지한다.
        for (int color = 0; color < _tables.Length; color++)
        {
            byte[] table = new byte[RowSize];
            // 변환 대상이 아닌 픽셀은 원본 색을 유지한다.
            for (int index = 0; index < RowSize; index++)
            {
                table[index] = (byte)index;
            }
            _tables[color] = table;
        }

        // 원본은 지정된 지면 색상 인덱스에만 8개 플레이어 색상을 계산한다.
        foreach (byte source in SourceColors)
        {
            double originalLightness = Lightness(palette[source]);
            // 각 소유자 색상에서 가장 가까운 명도의 목적지 색을 찾는다.
            for (int color = 1; color < _tables.Length; color++)
            {
                double desired = originalLightness + LightnessOffsets[color - 1];
                byte[] candidates = TargetColors[color - 1];
                byte chosen = candidates[0];
                double distance = Math.Abs(desired - Lightness(palette[chosen]));
                // 같은 거리라면 원본의 엄격한 비교처럼 앞선 후보를 유지한다.
                for (int candidate = 1; candidate < candidates.Length; candidate++)
                {
                    byte next = candidates[candidate];
                    double nextDistance = Math.Abs(desired - Lightness(palette[next]));
                    if (nextDistance < distance)
                    {
                        chosen = next;
                        distance = nextDistance;
                    }
                }
                _tables[color][source] = chosen;
            }
        }
    }

    /// <summary>소유자 색상의 256바이트 변환표를 돌려준다. 범위 밖은 중립색이다.</summary>
    public ReadOnlyMemory<byte> Table(int color) => _tables[color >= 1 && color < _tables.Length ? color : 0];

    /// <summary>원본 RGB→HSL 변환에서 사용하는 명도인 최댓값과 최솟값의 평균을 계산한다.</summary>
    private static double Lightness(Rgb color)
    {
        int highest = Math.Max(color.R, Math.Max(color.G, color.B));
        int lowest = Math.Min(color.R, Math.Min(color.G, color.B));
        return (highest + lowest) / 510.0;
    }
}
