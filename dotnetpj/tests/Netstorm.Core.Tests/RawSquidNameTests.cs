using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 언어/TYPE·설정·가상 표·좁은 CRT 서식의 실제 선택 순서를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>RawSquidName.cpp의 같은 현재 자료와 BYTE 경계를 검사한다.</summary>
public sealed class RawSquidNameTests
{
    /// <summary>BYTE 이름을 변환 없이 읽는 테스트용 표기다.</summary>
    private static string Text(byte[] bytes) => Encoding.Latin1.GetString(bytes);
    /// <summary>설명85와 실제 base 가상 표를 가진 같은 풀을 만든다.</summary>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, int Sid) Scene()
    {
        var pool = new RawSidPool(32768, true); int sid = pool.Allocate(SidAllocation.Client);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        Span<byte> raw = pool.RawSlot(sid); raw.Clear(); raw[10] = 85; BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501dd0); types[85].Description = "raw";
        return (pool, types, sid);
    }
    /// <summary>언어0/1/그 외의 조회 횟수와 설명 번역→설정 치환을 구별한다.</summary>
    [Fact]
    public void CurrentLanguageReadOrderAndSelectedTranslation()
    {
        var (pool, types, sid) = Scene(); uint language = 0; var trace = new List<string>();
        var names = new RawSquidName(pool, types, new()
        {
            Language = () => { trace.Add("language"); return language; },
            Translate = text => { trace.Add("translate:" + Text(text.ToArray())); return "translated:raw"u8.ToArray(); },
            Substitute = text => { trace.Add("substitute"); return "cfg:"u8.ToArray().Concat(text.ToArray()).ToArray(); },
        });
        Assert.Equal("cfg:raw", Text(names.Name(sid))); Assert.Equal(new[] { "language", "substitute" }, trace);
        trace.Clear(); language = 1; Assert.Equal("cfg:raw", Text(names.Name(sid))); Assert.Equal(new[] { "language", "language", "substitute" }, trace);
        trace.Clear(); language = 2; Assert.Equal("cfg:translated:raw", Text(names.Name(sid))); Assert.Equal(new[] { "language", "language", "translate:raw", "substitute" }, trace);
    }
    /// <summary>TYPE 번호는 언어 효과 전에 보관하고 같은 번호의 현재 설명을 효과 뒤 읽는다.</summary>
    [Fact]
    public void LanguageEffectsKeepSelectedTypeButReadCurrentDescription()
    {
        var (pool, types, sid) = Scene(); types[86].Description = "other";
        var names = new RawSquidName(pool, types, new()
        {
            Language = () => { types[85] = new() { Description = "current\0ignored" }; pool.RawSlot(sid)[10] = 86; return 0; },
            Substitute = text => text.ToArray(),
        });
        Assert.Equal("current", Text(names.BaseName(sid)));
    }
    /// <summary>가상68은 먼저 고정하며 base 효과가 VT를 바꿔도 이미 선택한 Factory 접미사를 유지한다.</summary>
    [Fact]
    public void FactorySelectsEntryBeforeBaseAndReadsLevelAfterPattern()
    {
        var (pool, types, sid) = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x509148); var trace = new List<string>();
        var names = new RawSquidName(pool, types, new()
        {
            Language = () => 0, Substitute = text => { trace.Add("base"); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x501dd0); return text.ToArray(); },
            Translate = text => { Assert.Equal(" Level %s", Text(text.ToArray())); trace.Add("pattern"); pool.RawSlot(sid)[11] = 64; return text.ToArray(); },
            Level = selected => { Assert.Equal(sid, selected); trace.Add("level"); return (uint)(pool.Slot(selected)[11] >> 5); },
        });
        Assert.Equal("raw Level III", Text(names.Name(sid))); Assert.Equal(new[] { "base", "pattern", "level" }, trace);
    }
    /// <summary>기본 이름은 Factory 가상 표와 무관하며 번역 불필요 경로에 접미사/레벨 장치를 요구하지 않는다.</summary>
    [Fact]
    public void BaseNameDoesNotDispatchVtableOrRequireFactoryEffects()
    {
        var (pool, types, sid) = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0);
        var names = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = text => text.ToArray() });
        Assert.Equal("raw", Text(names.BaseName(sid))); Assert.Throws<InvalidOperationException>(() => names.Name(sid));
    }
    /// <summary>설명과 치환 출력은 첫NUL 뒤를 제외하며 각80BYTE 경계를 별도로 확인한다.</summary>
    [Fact]
    public void DescriptionAndSubstitutionHaveIndependentByteBounds()
    {
        var (pool, types, sid) = Scene(); int substituted = 0;
        var names = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = text => { substituted++; return text.ToArray(); } });
        types[85].Description = new string('x', 79) + "\0tail"; Assert.Equal(79, names.BaseName(sid).Length); Assert.Equal(1, substituted);
        types[85].Description = new string('x', 80); Assert.Throws<ArgumentOutOfRangeException>(() => names.BaseName(sid)); Assert.Equal(1, substituted);
        types[85].Description = "raw";
        var tooLong = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = _ => new byte[80].Select(_ => (byte)'x').ToArray() });
        Assert.Throws<ArgumentOutOfRangeException>(() => tooLong.BaseName(sid));
        var nul = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = _ => "ok\0tail"u8.ToArray() }); Assert.Equal("ok", Text(nul.BaseName(sid)));
    }
    /// <summary>원문의83개 표 중 두 Factory와 base 대표를 분배하며 로마 숫자와ff sentinel을 유지한다.</summary>
    [Fact]
    public void VtableEntriesAndRomanSentinelAreExact()
    {
        Assert.Equal(0x4ace20u, RawSquidName.Entry(0x501dd0)); Assert.Equal(0x454d30u, RawSquidName.Entry(0x509148)); Assert.Equal(0x454d30u, RawSquidName.Entry(0x509210));
        Assert.Throws<InvalidOperationException>(() => RawSquidName.Entry(0)); Assert.Equal("I", Text(RawSquidName.LevelName(0))); Assert.Equal("X", Text(RawSquidName.LevelName(9)));
        Assert.Equal(new byte[] { 0xff }, RawSquidName.LevelName(10)); Assert.Equal(new byte[] { 0xff }, RawSquidName.LevelName(uint.MaxValue));
    }
    /// <summary>단일char* 서식의 폭·정밀도·왼쪽 정렬·리터럴%와 실제BYTE sentinel을 확인한다.</summary>
    [Fact]
    public void FactoryCrtFormattingKeepsWidthPrecisionPercentAndRawByte()
    {
        var (pool, types, sid) = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x509148); byte[] pattern = " %% [%4s]"u8.ToArray(); uint level = 0;
        var names = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = text => text.ToArray(), Translate = _ => pattern, Level = _ => level });
        Assert.Equal("raw % [   I]", Text(names.Name(sid))); pattern = "[%-4.2s]"u8.ToArray(); level = 2; Assert.Equal("raw[II  ]", Text(names.Name(sid)));
        pattern = "%s"u8.ToArray(); level = 10; Assert.Equal(new byte[] { (byte)'r', (byte)'a', (byte)'w', 0xff }, names.Name(sid));
        pattern = "constant\0%s"u8.ToArray(); Assert.Equal("rawconstant", Text(names.Name(sid)));
    }
    /// <summary>패턴 길이 실패는 레벨 조회 뒤 발생하고 최종80BYTE·미정의 추가 가변 인자도 구별한다.</summary>
    [Fact]
    public void FactoryBoundsKeepPriorEffectsAndDoNotInventExtraArguments()
    {
        var (pool, types, sid) = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x509148); int read = 0; byte[] pattern = Enumerable.Repeat((byte)'x', 256).ToArray();
        var names = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = text => text.ToArray(), Translate = _ => pattern, Level = _ => { read++; return 0; } });
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Name(sid)); Assert.Equal(1, read);
        pattern = Enumerable.Repeat((byte)'x', 77).ToArray(); Assert.Throws<ArgumentOutOfRangeException>(() => names.Name(sid)); Assert.Equal(2, read);
        pattern = "%s%s"u8.ToArray(); Assert.Throws<ArgumentException>(() => names.Name(sid)); Assert.Equal(3, read);
    }
    /// <summary>번역이 돌려준 패턴은 원문의 값 문자열처럼 보유해 레벨 효과의 외부 배열 변경과 분리한다.</summary>
    [Fact]
    public void FactoryKeepsReturnedPatternValueAcrossLevelEffect()
    {
        var (pool, types, sid) = Scene(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x509148); byte[] pattern = " [%s]"u8.ToArray();
        var names = new RawSquidName(pool, types, new() { Language = () => 0, Substitute = text => text.ToArray(), Translate = _ => pattern,
            Level = _ => { pattern.AsSpan().Fill((byte)'x'); return 1; } });
        Assert.Equal("raw [II]", Text(names.Name(sid))); Assert.Equal("xxxxx", Text(pattern));
    }
}
