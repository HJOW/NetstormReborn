using System.Buffers.Binary;

namespace Netstorm.Client.Display; // 원본 비트맵 글꼴 캐시와 플랫폼 생성 경계 영역이다.

/// <summary>원본 문맥의 스타일 번호다. 슬롯2는 비어 있다.</summary>
public enum FontStyle
{
    /// <summary>보통.</summary>
    Normal,
    /// <summary>기울임.</summary>
    Italic,
    /// <summary>굵게.</summary>
    Bold,
    /// <summary>취소선.</summary>
    Strikeout,
    /// <summary>밑줄.</summary>
    Underline,
}

/// <summary>각 글자의 독립 측정 폭·ABC A·출력 폭과 실제 VFX 포인터다.
/// 근거: cpppj/src/client/BitmapFont.h 의 FontGlyph. 특수 빈 프레임과 포인터 없음은 구별한다.</summary>
public sealed class FontGlyph
{
    /// <summary>첫 표의 측정 전진 폭.</summary>
    public int Advance { get; init; }
    /// <summary>둘째 표의 ABC A. 출력 x에 다시 더하지 않는다.</summary>
    public int Bearing { get; init; }
    /// <summary>실제 출력 반복이 더하는 셋째 표의 전진 폭.</summary>
    public int DrawAdvance { get; init; }
    /// <summary>실제 글리프 블록. null일 때만 출력과 전진을 모두 생략한다.</summary>
    public ShapeDatabase? Shape { get; init; }
}

/// <summary>원본 .chfnt의 네 정수 표와256개 독립 VFX를 그대로 보존한다.
/// 근거: cpppj/src/client/BitmapFont.cpp, 004a3240/CD00425980. GDI 생성은 Windows 호스트에 둔다.</summary>
public sealed class BitmapFont
{
    /// <summary>NUL을 포함하는16바이트 원본 캐시 서명.</summary>
    public static ReadOnlySpan<byte> Signature => "BitmapFontData\x1a\0"u8;
    /// <summary>다섯 헤더 DWORD 뒤 첫 표.</summary>
    public const int TableStart = 36;
    /// <summary>한 폭/크기 표의 바이트 수.</summary>
    public const int TableBytes = 1024;
    /// <summary>네 표 뒤 첫 글리프 블록.</summary>
    public const int PayloadStart = 4132;
    /// <summary>파일의 실제 tmHeight.</summary>
    private readonly int _height;
    /// <summary>파일의 실제 tmAscent.</summary>
    private readonly int _ascent;
    /// <summary>파일의 실제 tmDescent.</summary>
    private readonly int _descent;
    /// <summary>0~255바이트에 직접 대응하는 안정된 글리프 표.</summary>
    private readonly FontGlyph[] _glyphs = new FontGlyph[256];

    /// <summary>비정렬 DWORD를 읽고 잘린 입력은 C++ runtime_error와 같은 파일 오류로 거부한다.</summary>
    private static uint Read32(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length - 4) throw new InvalidDataException("잘린 비트맵 글꼴"); return BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    }

    /// <summary>서명/메트릭·블록 경계·단일 프레임·실제 RLE와 마지막 바이트까지 검사한다.</summary>
    public BitmapFont(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < PayloadStart || !bytes[..16].SequenceEqual(Signature) || Read32(bytes, 16) != 1 || Read32(bytes, 32) != 256) throw new InvalidDataException("비트맵 글꼴 헤더 오류");
        _height = unchecked((int)Read32(bytes, 20)); _ascent = unchecked((int)Read32(bytes, 24)); _descent = unchecked((int)Read32(bytes, 28));
        if (_height <= 0 || _height > 512 || _ascent < 0 || _descent < 0 || _ascent > _height || _descent > _height) throw new InvalidDataException("비트맵 글꼴 메트릭 오류");
        int position = PayloadStart;
        // 각 코드의 독립 VFX 포인터와 세 폭을 입력 순서대로 읽는다.
        for (int code = 0; code < _glyphs.Length; code++)
        {
            uint size = Read32(bytes, TableStart + TableBytes * 3 + code * 4); if (size > (uint)(bytes.Length - position)) throw new InvalidDataException("잘린 글리프 블록");
            ShapeDatabase? shape = null;
            if (size != 0)
            {
                shape = new(bytes.Slice(position, (int)size).ToArray());
                if (shape.Blocks().Count != 1 || shape.Blocks()[0].Frames.Count != 1) throw new InvalidDataException("글리프 단일 블록/프레임 오류");
                if (!shape.Blocks()[0].Frames[0].IsSpecial()) shape.Decode(0, 0);
            }
            _glyphs[code] = new() { Advance = unchecked((int)Read32(bytes, TableStart + code * 4)), Bearing = unchecked((int)Read32(bytes, TableStart + TableBytes + code * 4)), DrawAdvance = unchecked((int)Read32(bytes, TableStart + TableBytes * 2 + code * 4)), Shape = shape };
            position += (int)size;
        }
        if (position != bytes.Length) throw new InvalidDataException("글꼴 뒤 불필요한 바이트");
    }

    /// <summary>Windows-1252 바이트의 첫 폭 표를 합한다. Gump 측정의 DWORD 감김과 별개인 C++ 안전 폭 API다.</summary>
    public int Measure(ReadOnlySpan<byte> bytes)
    {
        long width = 0;
        // UTF-8 변환은 호출자가 맡으며 모든 바이트를 부호 없이 읽는다.
        foreach (byte code in bytes) width += _glyphs[code].Advance;
        if (width < int.MinValue || width > int.MaxValue) throw new OverflowException("글꼴 측정 폭 초과"); return (int)width;
    }
    /// <summary>코드의 안정된 글리프 객체.</summary>
    public FontGlyph Glyph(byte code) => _glyphs[code];
    /// <summary>캐시 실제 높이.</summary>
    public int Height() => _height;
    /// <summary>캐시 기준선 위 높이.</summary>
    public int Ascent() => _ascent;
    /// <summary>캐시 기준선 아래 높이.</summary>
    public int Descent() => _descent;
}

/// <summary>원본 슬롯/스타일 조합을 동일 이름의 캐시와 메모리 생성 경계에 연결한다.
/// 근거: cpppj/src/client/BitmapFont.cpp 의 FontStore. BaseFileSystem과 GDI는 플랫폼 경계의 조회/생성자로 전달한다.</summary>
public sealed class FontStore
{
    /// <summary>원본 CreateFontA 높이. 슬롯2는0이다.</summary>
    private static readonly int[] Heights = [14, 13, 0, 20, 48, 14, 12];
    /// <summary>원본 슬롯 굵기.</summary>
    private static readonly int[] Weights = [700, 0, 0, 700, 0, 0, 0];
    /// <summary>캐시 파일 이름에 쓰는 원본 스타일 순서.</summary>
    private static readonly string[] Styles = ["normal", "italic", "bold", "strikeout", "underline"];
    /// <summary>보호 원본/아카이브의 읽기 전용 파일 공급 경계.</summary>
    private readonly Func<string, byte[]?> _read;
    /// <summary>캐시가 없거나 손상됐을 때 원본 GDI와 같은 메모리 글꼴을 생성하는 플랫폼 경계.</summary>
    private readonly Func<string, int, int, FontStyle, BitmapFont> _generate;
    /// <summary>설정의 fontFaceName. 빈 값은 Arial이다.</summary>
    private readonly string _face;
    /// <summary>7슬롯×5스타일의 안정된 글꼴 참조.</summary>
    private readonly BitmapFont?[,] _fonts = new BitmapFont?[7, 5];

    /// <summary>읽기/생성 경계를 빌린다. 생성 결과를 원본 캐시에 쓰지 않는다.</summary>
    public FontStore(Func<string, byte[]?> read, string face, Func<string, int, int, FontStyle, BitmapFont> generate)
    {
        ArgumentNullException.ThrowIfNull(read); ArgumentNullException.ThrowIfNull(generate); _read = read; _face = face.Length == 0 ? "Arial" : face; _generate = generate;
    }
    /// <summary>캐시 tmHeight와 별개인 원본의 논리 슬롯 높이.</summary>
    public static int LogicalHeight(int slot) => slot >= 0 && slot < Heights.Length ? Heights[slot] : throw new ArgumentException("Gump 글꼴 슬롯 범위 오류");
    /// <summary>원본10.78 초기화가 실제 만드는 조합. 나머지를 생성으로 채우지 않는다.</summary>
    public static bool Available(int slot, FontStyle style) => slot >= 0 && slot < 7 && slot != 2 && (uint)style < Styles.Length && ((slot != 1 && slot != 3 && slot != 4) || style == FontStyle.Normal);
    /// <summary>캐시 파일의 원본 경로. Bold도 원래 슬롯 굵기로 파일 이름을 만든다.</summary>
    public string CachePath(int slot, FontStyle style = FontStyle.Normal)
    {
        if (!Available(slot, style)) throw new ArgumentException("없는 글꼴 슬롯/스타일");
        return $"d/!{(slot == 1 ? "Courier New" : _face)}.{Styles[(int)style]}.{Heights[slot]}.{Weights[slot]}.chfnt";
    }
    /// <summary>한 번만 캐시를 읽거나 생성한다. 손상 캐시의 파일 오류는 메모리 생성으로 대체한다.</summary>
    public BitmapFont Get(int slot = 0, FontStyle style = FontStyle.Normal)
    {
        if (!Available(slot, style)) throw new ArgumentException("없는 글꼴 슬롯/스타일"); int index = (int)style; BitmapFont? font = _fonts[slot, index];
        if (font is null)
        {
            if (_read(CachePath(slot, style)) is { } bytes)
            {
                try { font = new(bytes); } catch (InvalidDataException) { /* 원본도 손상 캐시를 GDI로 다시 만든다. */ }
            }
            font ??= _generate(slot == 1 ? "Courier New" : _face, Heights[slot], style == FontStyle.Bold ? Weights[slot] + (slot == 0 ? 100 : 700) : Weights[slot], style);
            _fonts[slot, index] = font;
        }
        return font;
    }
}
