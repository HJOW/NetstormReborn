namespace Netstorm.Assets;

/// <summary>팔레트 색 하나 (8비트 RGB)</summary>
/// <param name="R">빨강</param>
/// <param name="G">초록</param>
/// <param name="B">파랑</param>
public readonly record struct Rgb(byte R, byte G, byte B);

/// <summary>
/// 256색 팔레트. 원본 게임 팔레트는 d/GIFCLOUD.COL (docs/formats/shp.md "색상")
/// </summary>
public sealed class Palette
{
    /// <summary>팔레트 색 수</summary>
    public const int ColorCount = 256;

    /// <summary>Animator Pro .COL 헤더 크기 (파일 크기 u32 + 매직 u16 + 버전 u16)</summary>
    private const int ColHeaderSize = 8;

    /// <summary>.COL 파일 전체 크기 (헤더 + 256×RGB). 원본 로더도 이 크기(0x308)로 형식을 판별한다</summary>
    private const int ColFileSize = ColHeaderSize + ColorCount * 3;

    /// <summary>RGBX 형식 팔레트 파일 크기 (256×4바이트). 원본 로더가 지원하는 두 번째 형식</summary>
    private const int RgbxFileSize = ColorCount * 4;

    /// <summary>게임 기본 팔레트 파일 이름 (setup.cfg: fortPal = "gifcloud")</summary>
    public const string GameCol = "GIFCLOUD.COL";

    private readonly Rgb[] _colors;

    /// <summary>인덱스로 색을 얻는다</summary>
    /// <param name="index">팔레트 인덱스 0~255</param>
    public Rgb this[int index] => _colors[index];

    /// <summary>색 배열로부터 만든다</summary>
    /// <param name="colors">256개 색</param>
    public Palette(Rgb[] colors)
    {
        if (colors.Length != ColorCount)
        {
            throw new ArgumentException($"팔레트는 {ColorCount}색이어야 합니다.", nameof(colors));
        }
        _colors = colors;
    }

    /// <summary>
    /// 팔레트 파일 내용을 해석한다. 원본 로더(Screen.cpp)와 같이 0x308 바이트(.COL) 또는 0x400 바이트(RGBX)만 받는다.
    /// </summary>
    /// <param name="data">파일 내용</param>
    public static Palette Parse(ReadOnlySpan<byte> data)
    {
        var colors = new Rgb[ColorCount];
        if (data.Length == ColFileSize)
        {
            ReadOnlySpan<byte> rgb = data[ColHeaderSize..];
            // 256색을 RGB 3바이트씩 읽는다
            for (int i = 0; i < ColorCount; i++)
            {
                colors[i] = new Rgb(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
            }
        }
        else if (data.Length == RgbxFileSize)
        {
            // 256색을 RGBX 4바이트씩 읽는다 (마지막 바이트 무시)
            for (int i = 0; i < ColorCount; i++)
            {
                colors[i] = new Rgb(data[i * 4], data[i * 4 + 1], data[i * 4 + 2]);
            }
        }
        else
        {
            throw new InvalidDataException($"지원하지 않는 팔레트 크기: {data.Length} 바이트");
        }
        return new Palette(colors);
    }

    /// <summary>팔레트 파일을 읽는다</summary>
    /// <param name="path">.COL 파일 경로</param>
    public static Palette Load(string path) => Parse(File.ReadAllBytes(path));
}
