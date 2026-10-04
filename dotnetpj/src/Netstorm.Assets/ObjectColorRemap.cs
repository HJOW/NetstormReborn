namespace Netstorm.Assets;

/// <summary>
/// 원본 Colorsort.cpp의 타입별 소유자 색 변환표(0043ba00·0043bbe0·0043be30).
/// 타입 번호 전역 변수 0x541170부터의 초기값은 70 + TypeLoadOrder 순서다.
/// </summary>
public sealed class ObjectColorRemap
{
    /// <summary>소유자 색 수. 0은 원래 색이고 1~8은 플레이어 색이다.</summary>
    private const int ColorRows = 9;

    /// <summary>기본 변환 대상의 시작 인덱스. 228~237은 가장 밝은 색, 238~245는 어두움→밝음이다.</summary>
    private const int BandStart = 0xe4;

    /// <summary>기본 변환 대상의 마지막 인덱스.</summary>
    private const int BandEnd = 0xf5;

    /// <summary>원본 0x531a08의 색 번호별 밝음→어두움 팔레트 후보.</summary>
    private static readonly byte[][] Targets =
    [
        [81, 80, 76, 145, 146, 147, 4, 10],
        [94, 100, 170, 169, 204, 249, 217, 227],
        [195, 93, 185, 35, 31, 149, 70, 15],
        [207, 131, 133, 128, 136, 2, 142, 143],
        [189, 190, 253, 150, 148, 70, 14, 10],
        [113, 218, 219, 3, 52, 59, 67, 213],
        [9, 85, 155, 156, 6, 72, 144, 143],
        [220, 221, 222, 223, 224, 63, 227, 213],
    ];

    /// <summary>변환 대상 외에는 그대로 쓰는 256색 표.</summary>
    private readonly byte[] _identity = Identity();

    /// <summary>타입 로딩 번호별 9행 변환표. 등록되지 않은 타입은 원래 색을 유지한다.</summary>
    private readonly Dictionary<int, byte[][]> _tables = [];

    /// <summary>지면·받침은 기존에 복원한 isle 명도 보정표를 공유한다.</summary>
    private readonly IsleColorRemap _isle;

    /// <summary>팔레트의 명도와 원본 지정 인덱스로 타입별 변환표를 한 번 만든다.</summary>
    public ObjectColorRemap(Palette palette)
    {
        _isle = new IsleColorRemap(palette);
        // 0043be30의 기본 규칙 열두 타입. bridge의 빨강·연파랑만 별도 명도 한 가지를 쓴다.
        foreach (string name in new[] { "bridge", "bridgeConnector", "priest", "altar", "bulf", "sunWalker",
                     "rainWalker", "sunFlyer", "fenceShield", "rainBalloon", "rainFlyer", "outpost" })
        {
            byte[][] rows = Rows();
            // 플레이어 색별 228~245 인덱스를 원본과 같은 순서로 치환한다.
            for (int color = 1; color < ColorRows; color++)
            {
                // 228~237은 후보 0, 238~245는 후보 7~0에 대응한다.
                for (int source = BandStart; source <= BandEnd; source++)
                {
                    int shade = source < 0xee ? 0 : BandEnd - source;
                    if (name == "bridge" && source >= 0xee && color is 2 or 7) shade = color == 2 ? 4 : 3;
                    rows[color][source] = Targets[color - 1][shade];
                }
            }
            _tables[TypeLoadOrder.IndexOf(name)] = rows;
        }
        AddLightness(palette, "sunBalloon", 0,
            [0x56, 0xbc, 0x55, 0xf7, 0x54, 0x1f, 0x22, 0x4b, 0x52, 0x4a, 0x53, 0x9c,
                0x99, 0x46, 0x48, 0x47, 0x49, 0x98, 0x0d, 0x90]);
        // 0x5066d8의 double은 원래 float -0.15를 확장한 값이므로 그대로 보존한다.
        AddLightness(palette, "windBalloon", -0.15000000596046448,
            [0x57, 0x58, 0x55, 0x56, 0x54, 0x9c, 0x4b, 0x52, 0x99, 0x4a, 0x49, 0x53,
                0x9b, 0x48, 0x47, 0x2c, 0x98, 0x9a, 0x46, 0x0d, 0x90]);
        AddLightness(palette, "windWalker", 0,
            [0xda, 0xdb, 0xdc, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
                0x42, 0xa1, 0xd2, 0x13, 0x15, 0xd6, 0x8d, 0x12]);
    }

    /// <summary>타입·색 번호에 맞는 표. 중립·범위 밖 색과 미지정 타입은 원래 색이다.</summary>
    public ReadOnlyMemory<byte> Table(int typeIndex, int color)
    {
        if (color <= 0 || color >= ColorRows) return _identity;
        // 지면 변환 경로는 기존 받침·가장자리 장식 구현의 색을 보존한다.
        if (typeIndex == TypeLoadOrder.IndexOf("isle") || typeIndex == TypeLoadOrder.IndexOf("isleBig")
            || typeIndex == TypeLoadOrder.IndexOf("edgeFarm") || typeIndex == TypeLoadOrder.IndexOf("island")
            || typeIndex == TypeLoadOrder.IndexOf("islandStalag")) return _isle.Table(color);
        return _tables.TryGetValue(typeIndex, out byte[][]? rows) ? rows[color] : _identity;
    }

    /// <summary>오브젝트 소유자 색 예열이 필요한 타입인지 판정한다.</summary>
    public bool AppliesTo(int typeIndex) => _tables.ContainsKey(typeIndex);

    /// <summary>0043b5a0처럼 지정 픽셀의 명도에 가장 가까운 후보를 고른다. 동률은 앞선 후보를 유지한다.</summary>
    private void AddLightness(Palette palette, string name, double offset, byte[] sources)
    {
        byte[][] rows = Rows();
        // 소유자색으로 바꾸기로 지정된 팔레트 인덱스만 다룬다.
        foreach (byte source in sources)
        {
            double desired = Lightness(palette[source]) + offset;
            // 소유자 색별로 가장 가까운 여덟 후보를 비교한다.
            for (int color = 1; color < ColorRows; color++)
            {
                byte chosen = Targets[color - 1][0];
                double distance = Math.Abs(desired - Lightness(palette[chosen]));
                // 원본의 엄격한 비교로 후보 순서를 보존한다.
                foreach (byte candidate in Targets[color - 1].Skip(1))
                {
                    double next = Math.Abs(desired - Lightness(palette[candidate]));
                    if (next >= distance) continue;
                    chosen = candidate; distance = next;
                }
                rows[color][source] = chosen;
            }
        }
        _tables[TypeLoadOrder.IndexOf(name)] = rows;
    }

    /// <summary>전체 인덱스를 그대로 유지하는 표를 만든다.</summary>
    private static byte[] Identity()
    {
        var row = new byte[Palette.ColorCount];
        // 팔레트 0~255를 같은 번호로 연결한다.
        for (int i = 0; i < row.Length; i++) row[i] = (byte)i;
        return row;
    }

    /// <summary>중립을 포함한 아홉 색 모두 원래 색으로 초기화한다.</summary>
    private static byte[][] Rows() => Enumerable.Range(0, ColorRows).Select(_ => Identity()).ToArray();

    /// <summary>원본 RGB→HSL의 명도는 최대·최소 RGB의 평균이다.</summary>
    private static double Lightness(Rgb rgb) =>
        (Math.Max(rgb.R, Math.Max(rgb.G, rgb.B)) + Math.Min(rgb.R, Math.Min(rgb.G, rgb.B))) / 510.0;
}
