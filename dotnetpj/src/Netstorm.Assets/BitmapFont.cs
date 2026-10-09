using System.Buffers.Binary;

namespace Netstorm.Assets;

/// <summary>비트맵 글꼴의 글리프 하나: 세 가지 전진 폭과 해독된 모양.</summary>
public sealed class BitmapGlyph
{
    /// <summary>측정 폭 (첫 표: ABC B+C). 문자열 폭 재기에 쓴다.</summary>
    public int Advance { get; }

    /// <summary>ABC A (둘째 표). 출력 x 좌표에 다시 더하지 않는다.</summary>
    public int Bearing { get; }

    /// <summary>그리기 전진 폭 (셋째 표: ABC B+C). 실제 출력이 다음 글자로 넘어가는 너비다.</summary>
    public int DrawAdvance { get; }

    /// <summary>해독된 글리프 모양 (빈 글리프(공백)는 null).</summary>
    public IndexedImage? Image { get; }

    /// <summary>그릴 픽셀이 없는 빈 글리프인지.</summary>
    public bool IsBlank => Image == null;

    /// <summary>글리프를 만든다.</summary>
    /// <param name="advance">측정 폭</param>
    /// <param name="bearing">ABC A</param>
    /// <param name="drawAdvance">그리기 전진 폭</param>
    /// <param name="image">해독된 모양 (빈 글리프는 null)</param>
    internal BitmapGlyph(int advance, int bearing, int drawAdvance, IndexedImage? image)
    {
        Advance = advance;
        Bearing = bearing;
        DrawAdvance = drawAdvance;
        Image = image;
    }
}

/// <summary>
/// 원본 비트맵 글꼴 캐시(.chfnt) 읽기. 원본 10.78 의 읽기(004a3240)를 옮겼다
/// (docs/exe/cpp-renderer-reconstruction.md·docs/formats/chfnt.md. 기준 구현은 cpppj/src/client/BitmapFont.cpp).
/// 파일은 서명 `BitmapFontData`·`1A 00`(16바이트), u32 다섯 개(버전 1·높이·어센트·디센트·글자 수 256),
/// i32 256개씩 세 표(측정 폭·ABC A·그리기 전진 폭), u32 256개(글리프 블록 바이트 수) 뒤에
/// 256개의 독립 VFX `1.10` 블록(각 한 프레임)이 온다. 글리프 블록 해독은 <see cref="ShapeDatabase"/> 를 재사용한다.
/// 공백은 특수 빈 프레임이며 폭은 그대로 진행한다. 측정은 첫 표, 실제 출력의 진행은 셋째 표를 쓴다.
/// 글자의 불투명 픽셀은 팔레트 번호 100 이다. 손상된 캐시는 거부한다 (원본은 GDI 로 다시 만든다).
/// 한국어처럼 코드 페이지 밖의 글자는 이 판독기로 그리지 않는다 (D2Coding 을 쓴다).
/// </summary>
public sealed class BitmapFont
{
    /// <summary>파일 앞 16바이트 서명 (`BitmapFontData` + 1A 00).</summary>
    public static ReadOnlySpan<byte> Signature => "BitmapFontData\x1A\0"u8;

    /// <summary>정수 표 시작 (0x24).</summary>
    private const int TableStart = 36;

    /// <summary>표 하나당 바이트 수 (i32·u32 × 256).</summary>
    private const int TableBytes = 1024;

    /// <summary>글리프 블록 시작. 네 표 끝(36 + 1024 × 4 = 4132 = 0x1024)이며 문서/C++ 구현과 같다.</summary>
    private const int PayloadStart = 4132;

    /// <summary>글자 높이 (CreateFontA 의 양수 픽셀 높이).</summary>
    public int Height { get; }

    /// <summary>기준선 위 높이.</summary>
    public int Ascent { get; }

    /// <summary>기준선 아래 높이.</summary>
    public int Descent { get; }

    /// <summary>바이트 코드 0~255의 글리프.</summary>
    public IReadOnlyList<BitmapGlyph> Glyphs { get; }

    /// <summary>파일에서 읽는다.</summary>
    /// <param name="path">.chfnt 경로</param>
    public static BitmapFont Load(string path) => new(File.ReadAllBytes(path));

    /// <summary>메모리에 올린 파일 내용으로부터 만든다. 손상된 캐시는 거부한다.</summary>
    /// <param name="raw">.chfnt 전체 내용</param>
    public BitmapFont(byte[] raw)
    {
        // 서명·버전·글자 수와 전체 최소 길이를 확인한다.
        if (raw.Length < PayloadStart || !raw.AsSpan(0, 16).SequenceEqual(Signature)
            || ReadUInt32(raw, 16) != 1 || ReadUInt32(raw, 32) != 256)
        {
            throw new InvalidDataException("비트맵 글꼴 머리말이 손상되었습니다.");
        }
        Height = (int)ReadUInt32(raw, 20);
        Ascent = (int)ReadUInt32(raw, 24);
        Descent = (int)ReadUInt32(raw, 28);
        // 높이·어센트·디센트 범위를 확인한다 (원본 GDI 생성 범위와 같다).
        if (Height <= 0 || Height > 512 || Ascent < 0 || Descent < 0 || Ascent > Height || Descent > Height)
        {
            throw new InvalidDataException("비트맵 글꼴 크기가 범위를 벗어났습니다.");
        }
        var glyphs = new BitmapGlyph[256];
        int position = PayloadStart;
        // 바이트 코드 0~255의 독립 VFX 블록을 차례로 읽는다.
        for (int c = 0; c < glyphs.Length; c++)
        {
            int advance = ReadInt32(raw, TableStart + c * 4);
            int bearing = ReadInt32(raw, TableStart + TableBytes + c * 4);
            int drawAdvance = ReadInt32(raw, TableStart + TableBytes * 2 + c * 4);
            int size = (int)ReadUInt32(raw, TableStart + TableBytes * 3 + c * 4);
            // 블록이 파일 끝을 넘으면 거부한다.
            if (size < 0 || size > raw.Length - position)
            {
                throw new InvalidDataException($"비트맵 글리프 {c} 블록이 잘렸습니다.");
            }
            IndexedImage? image = null;
            if (size != 0)
            {
                // 한 프레임짜리 VFX 블록 하나를 기존 판독기로 읽는다.
                var block = new ShapeDatabase(raw.AsSpan(position, size).ToArray());
                if (block.Blocks.Count != 1 || block.Blocks[0].Frames.Count != 1)
                {
                    throw new InvalidDataException($"비트맵 글리프 {c} 블록이 한 프레임이 아닙니다.");
                }
                ShapeFrame frame = block.Blocks[0].Frames[0];
                // 특수 빈 프레임(공백)은 모양 없이 폭만 둔다.
                if (!frame.IsSpecial)
                {
                    image = block.Decode(frame);
                }
            }
            glyphs[c] = new BitmapGlyph(advance, bearing, drawAdvance, image);
            position += size;
        }
        // 남는 바이트가 있으면 거부한다.
        if (position != raw.Length)
        {
            throw new InvalidDataException("비트맵 글꼴 끝에 남는 바이트가 있습니다.");
        }
        Glyphs = glyphs;
    }

    /// <summary>바이트 코드의 글리프.</summary>
    /// <param name="code">바이트 코드 (Windows-1252 바이트 값)</param>
    public BitmapGlyph Glyph(byte code) => Glyphs[code];

    /// <summary>
    /// 코드 페이지 바이트열의 측정 폭 (첫 표의 합). 원본 Measure 와 같이 코드를 부호 없이 색인한다.
    /// </summary>
    /// <param name="codes">코드 페이지 바이트열</param>
    public int MeasureBytes(ReadOnlySpan<byte> codes)
    {
        long width = 0;
        // 바이트마다 측정 폭을 누적한다.
        foreach (byte code in codes)
        {
            width += Glyphs[code].Advance;
        }
        // 32비트에 못 들어가는 합은 거부한다.
        if (width < int.MinValue || width > int.MaxValue)
        {
            throw new OverflowException("비트맵 글자 폭 합이 범위를 벗어났습니다.");
        }
        return (int)width;
    }

    /// <summary>리틀 엔디언 u32 읽기.</summary>
    /// <param name="raw">파일 내용</param>
    /// <param name="offset">바이트 오프셋</param>
    private static uint ReadUInt32(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>리틀 엔디언 i32 읽기.</summary>
    /// <param name="raw">파일 내용</param>
    /// <param name="offset">바이트 오프셋</param>
    private static int ReadInt32(byte[] raw, int offset) =>
        BinaryPrimitives.ReadInt32LittleEndian(raw.AsSpan(offset, 4));
}
