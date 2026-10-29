using System.Buffers.Binary;
using Netstorm.Client.Display;

namespace Netstorm.Tests; // 원본 독립 폭/포인터 입력을 실제 글꼴 캐시에 공급하는 검사 영역이다.

/// <summary>cpppj/tests/GumpGlyphTests.cpp·GumpTextTests.cpp의 입력 캐시 작성기다. 파일 장치 대신 같은 경로의 읽기 전용 메모리 공급을 사용한다.</summary>
internal sealed class GumpFontFixture
{
    /// <summary>원본 파일 이름의 요청 높이.</summary>
    private static readonly int[] Heights = [14, 13, 0, 20, 48, 14, 12];
    /// <summary>원본 파일 이름의 요청 굵기.</summary>
    private static readonly int[] Weights = [700, 0, 0, 700, 0, 0, 0];
    /// <summary>원본 파일 이름의 스타일 문자열.</summary>
    private static readonly string[] Styles = ["normal", "italic", "bold", "strikeout", "underline"];
    /// <summary>독립 입력으로 만든 실제 캐시.</summary>
    public Dictionary<string, byte[]> Caches { get; } = new(StringComparer.Ordinal);
    /// <summary>실제 선택된 글꼴 객체의 정체성. 기대 선택을 계산하지 않는다.</summary>
    public Dictionary<BitmapFont, (uint Slot, uint Style)> Identities { get; } = [];
    /// <summary>실제 파일 이름/파서/슬롯 캐시를 쓰는 저장소.</summary>
    public FontStore Fonts { get; }
    /// <summary>독립 PE 관측 행.</summary>
    public string[][] Cases { get; }

    /// <summary>바이트 배열의 입력 DWORD를 쓴다.</summary>
    public static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);
    /// <summary>실제 캐시 경로를 검사 입력의 슬롯/스타일로 만든다.</summary>
    private static string Path(uint slot, uint style) => $"d/!{(slot == 1 ? "Courier New" : "Arial")}.{Styles[style]}.{Heights[slot]}.{Weights[slot]}.chfnt";

    /// <summary>글리프 포인터/특수 공백·첫 폭99·ABC A-31과 독립 셋째 폭을 가진 입력 캐시를 만든다.</summary>
    public static byte[] GlyphCache(byte[] advances, byte[] mask)
    {
        Assert.Equal(1024, advances.Length); Assert.Equal(256, mask.Length); byte[] header = Header(31); advances.CopyTo(header, 36 + 2048); var shapes = new List<byte>();
        // 입력 포인터가 있는 코드에만 실제 VFX 블록을 붙인다.
        for (int code = 0; code < 256; code++)
        {
            Put(header, 36 + code * 4, 99); Put(header, 36 + 1024 + code * 4, 0xffffffe1);
            if (mask[code] == 0) continue; var shape = new List<byte>(new byte[40]); byte[] raw = shape.ToArray(); Put(raw, 0, 0x30312e31); Put(raw, 4, 1); Put(raw, 8, 16);
            if (code == 32) { Put(raw, 24, 0x7fffffff); Put(raw, 28, 0x7fffffff); Put(raw, 32, 0x80000001); Put(raw, 36, 0x80000001); }
            shape = new(raw); if (code != 32) shape.AddRange([2, 100, 0]); Put(header, 36 + 3072 + code * 4, (uint)shape.Count); shapes.AddRange(shape);
        }
        return [.. header, .. shapes];
    }

    /// <summary>서명과 실제 캐시 높이/상승/하강을 가진 공통 입력 헤더.</summary>
    public static byte[] Header(uint height)
    {
        byte[] header = new byte[4132]; "BitmapFontData\x1a\0"u8.CopyTo(header); Put(header, 16, 1); Put(header, 20, height); Put(header, 24, height - 5); Put(header, 28, 5); Put(header, 32, 256); return header;
    }

    /// <summary>A/공백/코드0만 포인터가 있고 셋째 전진 폭이2/3/1인 독립 한 픽셀 글꼴.</summary>
    public static BitmapFont PixelFont()
    {
        byte[] widths = new byte[1024], mask = new byte[256]; Put(widths, 65 * 4, 2); Put(widths, 32 * 4, 3); Put(widths, 0, 1); mask[65] = mask[32] = mask[0] = 1; return new(GlyphCache(widths, mask));
    }

    /// <summary>두 fixture가 제공한 입력 표만 캐시로 만들고 실제 FontStore 인스턴스의 정체성을 기록한다.</summary>
    public GumpFontFixture(string name)
    {
        string[][] rows = X86Fixture.Read(name); Cases = rows.Where(r => r[0] == "Case").ToArray(); Fonts = new(path => Caches.GetValueOrDefault(path), "Arial", (_, _, _, _) => throw new InvalidOperationException("fixture 글꼴 생성 금지"));
        // 저장된 Font 입력은 실제 캐시 파서에 공급하며 Case 출력은 계산하지 않는다.
        foreach (string[] r in rows.Where(r => r[0] == "Font"))
        {
            uint slot = X86Fixture.UInt(r[1]), style = X86Fixture.UInt(r[2]); byte[] cache;
            if (name == "gumpglyph") cache = GlyphCache(X86Fixture.Bytes(r[3]), X86Fixture.Bytes(r[4]));
            else { cache = Header(X86Fixture.UInt(r[3])); X86Fixture.Bytes(r[4]).CopyTo(cache, 36); }
            Caches.Add(Path(slot, style), cache);
        }
        // 실제 객체별 슬롯/스타일은 출력 콜백의 관측에만 사용한다.
        foreach (string[] r in rows.Where(r => r[0] == "Font")) { uint slot = X86Fixture.UInt(r[1]), style = X86Fixture.UInt(r[2]); Identities.Add(Fonts.Get((int)slot, (FontStyle)style), (slot, style)); }
    }
}
