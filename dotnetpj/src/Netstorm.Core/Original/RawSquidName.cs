using System.Buffers.Binary;

// 현재 TYPE 설명과 실제 가상 이름을 원본의 좁은 BYTE 문자열로 처리한다.
namespace Netstorm.Core.Original;

/// <summary>같은 현재 언어·번역·설정 치환·가상 레벨 장치다. 문자열은 표시 경계 전의 BYTE 뷰다.</summary>
public sealed record SquidNameHooks
{
    /// <summary>원문 위치에서 한 번 또는 두 번 읽는 현재 언어 번호다.</summary>
    public Func<uint>? Language { get; init; }
    /// <summary>현재 표시 BYTE 문자열을 실제 용어표로 번역한다.</summary>
    public Func<ReadOnlyMemory<byte>, byte[]>? Translate { get; init; }
    /// <summary>현재80BYTE 이름에 실제 설정 치환을 수행한다.</summary>
    public Func<ReadOnlyMemory<byte>, byte[]>? Substitute { get; init; }
    /// <summary>현재 Factory의 실제 가상80 레벨을 읽는다.</summary>
    public Func<int, uint>? Level { get; init; }
}

/// <summary>RawSquidName.cpp의004ac550/0049af80과 두 실제 가상68 이름 본체다.</summary>
public sealed partial class RawSquidName
{
    /// <summary>TYPE와 가상 표를 읽는 같은 PATCH 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>언어 콜백 뒤에도 현재 설명 필드를 읽는 같은188TYPE다.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types { get; }
    /// <summary>생성 시 복사한 함수 값이며 선택되지 않은 장치는 요구하지 않는다.</summary>
    private readonly SquidNameHooks _hooks;
    /// <summary>같은 PATCH/188TYPE만 확인하고 이름·외부 효과를 미리 실행하지 않는다.</summary>
    public RawSquidName(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, SquidNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188) throw new ArgumentException("객체 이름의 같은 PATCH/188TYPE 계약");
        Pool = pool; Types = types; _hooks = hooks with { };
    }
    /// <summary>선택된 실제 효과만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("객체 이름의 선택된 실제 하위 미연결");
    /// <summary>첫NUL까지만 보관하고 원문 버퍼의 끝NUL 공간을 확인한다.</summary>
    private static byte[] Text(ReadOnlyMemory<byte> bytes, int capacity)
    {
        int zero = bytes.Span.IndexOf((byte)0); int count = zero < 0 ? bytes.Length : zero;
        if (count >= capacity) throw new ArgumentOutOfRangeException(nameof(bytes), "객체 이름의 원본 BYTE 버퍼 범위");
        return bytes.Span[..count].ToArray();
    }
    /// <summary>선택한 TYPE 번호는 유지하고 언어 효과 뒤 현재 설명→번역→80BYTE→치환을 실행한다.</summary>
    public byte[] BaseName(int sid)
    {
        byte type = Pool.Slot(sid)[10]; if (type >= Types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "객체 이름의 TYPE 범위");
        uint first = Need(_hooks.Language)(); bool translated = first != 0 && Need(_hooks.Language)() != 1;
        byte[] description = ExtendedText.EncodeDisplayText(Types[type].Description);
        int zero = description.AsSpan().IndexOf((byte)0); if (zero >= 0) description = description.AsSpan(0, zero).ToArray();
        if (translated) description = Need(_hooks.Translate)(description);
        return Text(Need(_hooks.Substitute)(Text(description, 80)), 80);
    }
    /// <summary>원문 표의I..X와 범위 밖ff 한BYTE를 돌려준다.</summary>
    public static byte[] LevelName(uint level) => level switch
    {
        0 => "I"u8.ToArray(), 1 => "II"u8.ToArray(), 2 => "III"u8.ToArray(), 3 => "IV"u8.ToArray(), 4 => "V"u8.ToArray(),
        5 => "VI"u8.ToArray(), 6 => "VII"u8.ToArray(), 7 => "VIII"u8.ToArray(), 8 => "IX"u8.ToArray(), 9 => "X"u8.ToArray(), _ => [0xff],
    };
    /// <summary>실제 단일char* 인자의%s·폭·정밀도·%%를256BYTE 접미사에 적용한다. 추가 인자를 요구하는 서식은 미정의 읽기로 실패한다.</summary>
    private static byte[] Suffix(ReadOnlyMemory<byte> pattern, byte[] level)
    {
        ReadOnlySpan<byte> source = pattern.Span; int zero = source.IndexOf((byte)0); if (zero >= 0) source = source[..zero];
        var output = new List<byte>(); bool consumed = false;
        // 원문 CRT의 좁은 서식을 순서대로 소비하며 결과가 지역 버퍼를 넘는 위치에서 실패한다.
        for (int index = 0; index < source.Length; index++)
        {
            byte current = source[index];
            if (current != '%') output.Add(current);
            else
            {
                if (++index >= source.Length) throw new ArgumentException("Factory 이름의 불완전한 CRT 서식");
                if (source[index] == '%') output.Add((byte)'%');
                else
                {
                    bool left = false;
                    // 문자열 폭에 영향을 주는 왼쪽 정렬과 CRT가 허용하는 플래그를 소비한다.
                    while (index < source.Length && source[index] is (byte)'-' or (byte)'+' or (byte)' ' or (byte)'#' or (byte)'0')
                    { left |= source[index] == '-'; index++; }
                    int width = 0;
                    // 가변 인자 폭을 새 값으로 추측하지 않으며 정적 십진 폭만 읽는다.
                    while (index < source.Length && source[index] >= '0' && source[index] <= '9')
                    { width = checked(width * 10 + source[index++] - '0'); if (width >= 256) throw new ArgumentOutOfRangeException(nameof(pattern), "Factory 접미사 폭 범위"); }
                    int precision = int.MaxValue;
                    if (index < source.Length && source[index] == '.')
                    {
                        precision = 0; index++;
                        // 정밀도는 문자열 BYTE 수에 적용하며 추가 인자를 읽지 않는다.
                        while (index < source.Length && source[index] >= '0' && source[index] <= '9') precision = checked(precision * 10 + source[index++] - '0');
                    }
                    if (index >= source.Length || source[index] != 's' || consumed) throw new ArgumentException("Factory 이름의 미정의 추가 CRT 인자");
                    consumed = true; int count = Math.Min(precision, level.Length), padding = Math.Max(0, width - count);
                    if (!left) output.AddRange(Enumerable.Repeat((byte)' ', padding)); output.AddRange(level.AsSpan(0, count).ToArray());
                    if (left) output.AddRange(Enumerable.Repeat((byte)' ', padding));
                }
            }
            if (output.Count >= 256) throw new ArgumentOutOfRangeException(nameof(pattern), "Factory 접미사의256BYTE 범위");
        }
        return output.ToArray();
    }
    /// <summary>가상 표를 먼저 선택하고 기본 이름 뒤 현재 번역 패턴→실제 레벨→접미사를 결합한다.</summary>
    public byte[] Name(int sid)
    {
        uint entry = Entry(BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid))); byte[] name = BaseName(sid);
        if (entry == 0x4ace20) return name;
        byte[] pattern = Need(_hooks.Translate)(" Level %s"u8.ToArray()).ToArray(); byte[] level = LevelName(Need(_hooks.Level)(sid));
        return Text(name.Concat(Suffix(pattern, level)).ToArray(), 80);
    }
}
