using System.Text;
using Netstorm.Core.Original;

// 현재 서버 행의 바이트 경계와 실제 NSInfo 장기 저장소를 원문 분기 계약으로 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>통신 없이 서버 문자열 판독·출력 보존·부분검색과 체크섬을 검증한다.</summary>
public sealed class ClientNsInfoGlobalsTests
{
    /// <summary>검사 입력의 원문 좁은 문자열을 BYTE로 제공한다.</summary>
    private static byte[] Raw(string value) => Encoding.Latin1.GetBytes(value);
    /// <summary>목적 버퍼의 실제 첫 NUL 전까지만 판독한다.</summary>
    private static string Text(ReadOnlySpan<byte> value)
    {
        int nul = value.IndexOf((byte)0); return Encoding.Latin1.GetString(nul < 0 ? value : value[..nul]);
    }

    /// <summary>실제 초기 DWORD/BYTE와 장기 고정 물리가 처음 한 번만 구성된다.</summary>
    [Fact]
    public void InitialFieldsAndPhysicalOwnersMatchNativeDefaults()
    {
        var info = new ClientNsInfoGlobals(); Assert.Equal(0u, info.Current.Value); Assert.Equal(0u, info.Zone.Value);
        Assert.Equal(40, info.RosterOrder.Length); Assert.Equal(80, info.ServerName.Length);
        Assert.Equal((byte)6, info.NsKind); Assert.Equal((byte)37, info.PlayerKind); Assert.Equal((byte)60, info.BattleKind);
        Assert.Equal(0x50c894u, info.ZoneFormat); Assert.Equal(0x02000006u, info.LeaveMenuCommand); Assert.Equal(0x020000bau, info.KickoffCommand);
        UIntBox zone = info.Zone; zone.Value = 0x80000001; info.LoadServerNames(Raw("a;a;1\n")); Assert.Same(zone, info.Zone); Assert.Equal(0x80000001u, info.Zone.Value);
    }

    /// <summary>같은 실제 옵션이 세 현재DWORD를 원문 순서로 갱신하고 누락키와 다른 전역은 보존한다.</summary>
    [Fact]
    public void OptionReadsKeepMissingCurrentValuesAndFullSignedBits()
    {
        var info = new ClientNsInfoGlobals(); var configuration = new ConfigInterface(_ => null);
        info.ServerSelection.Value = -7; info.RootServerBroadcastPort.Value = 123; info.PassedDiags.Value = 9; info.Zone.Value = 73;
        info.ReadOptions(configuration); Assert.Equal(-7, info.ServerSelection.Value); Assert.Equal(123, info.RootServerBroadcastPort.Value); Assert.Equal(9, info.PassedDiags.Value);
        configuration.SetInt("curServer", -1); configuration.Set("rootServerBroadcastPort", "4294967295"); configuration.SetInt("passedDiags", int.MinValue);
        info.ReadOptions(configuration); Assert.Equal(-1, info.ServerSelection.Value); Assert.Equal(-1, info.RootServerBroadcastPort.Value); Assert.Equal(int.MinValue, info.PassedDiags.Value); Assert.Equal(73u, info.Zone.Value);
    }

    /// <summary>무효 index는 출력/선택값을 보존하고 유효 parser는 출력 전체를 먼저 지운다.</summary>
    [Fact]
    public void InvalidIndexPreservesOutputsWhileValidRecordClearsAndParses()
    {
        var info = new ClientNsInfoGlobals(); info.Servers.Add("host;Displayed; -12tail\n99");
        byte[] endpoint = Enumerable.Repeat((byte)0xa5, 8).ToArray(), name = Enumerable.Repeat((byte)0xb6, 16).ToArray(); var value = new IntBox { Value = 73 };
        info.ReadServerFields(-1, endpoint, name, value); Assert.All(endpoint, item => Assert.Equal(0xa5, item)); Assert.All(name, item => Assert.Equal(0xb6, item)); Assert.Equal(73, value.Value);
        info.ReadServerFields(0, endpoint, name, value); Assert.Equal("host", Text(endpoint)); Assert.Equal("Displayed", Text(name)); Assert.Equal(-12, value.Value); Assert.Equal(0, endpoint[^1]); Assert.Equal(0, name[^1]);
        RootServerRecords.Read(Raw("no delimiter"), endpoint, name, value); Assert.All(endpoint, item => Assert.Equal(0, item)); Assert.All(name, item => Assert.Equal(0, item)); Assert.Equal(0, value.Value);
    }

    /// <summary>첫세미콜론·최대span-1복사·빈출력·첫NUL과별도숫자출력의경계를확인한다.</summary>
    [Fact]
    public void RecordParserPreservesRawByteAndSpanBoundaries()
    {
        byte[] endpoint = new byte[4], name = new byte[5]; var value = new IntBox();
        RootServerRecords.Read(Raw("abcdef;0123456789;2147483648\0;17"), endpoint, name, value);
        Assert.Equal("abc", Text(endpoint)); Assert.Equal("0123", Text(name)); Assert.Equal(int.MinValue, value.Value);
        RootServerRecords.Read(Raw(";\xff;0x20"), [], name, value); Assert.Equal((byte)0xff, name[0]); Assert.Equal(0, value.Value);
        RootServerRecords.Read(Raw("abc;last"), endpoint, name, value); Assert.Equal("abc", Text(endpoint)); Assert.Equal("last", Text(name)); Assert.Equal(0, value.Value);
    }

    /// <summary>첫 성공패턴의host/숫자만사용하며실패콜론을가로질러원래prefix를합치지않는다.</summary>
    [Theory]
    [InlineData("server:123tail", "server", 123u, 0)]
    [InlineData("abc:bad:9suffix", "bad", 9u, 0)]
    [InlineData(":7:8", "", 7u, 0)]
    [InlineData("abc", "abc", 6799u, 1)]
    [InlineData("abc:-1", "abc:-1", 6799u, 1)]
    [InlineData("a:;b:15", ";b", 15u, 0)]
    [InlineData("host:0", "host", 0u, 0)]
    public void EndpointUsesFirstSuccessfulPattern(string input, string expectedHost, uint expectedPort, int expectedReads)
    {
        int reads = 0; var result = RootServerRecords.Resolve(Raw(input), 0, () => { reads++; return 6799; });
        Assert.Equal(expectedHost, result.Host); Assert.Equal(expectedPort, result.Port); Assert.Equal(expectedReads, reads);
    }

    /// <summary>기본포트조회는명시포트덮어쓰기보다앞에있고80BYTE/NUL물리관문은그보다앞에있다.</summary>
    [Fact]
    public void EndpointKeepsDefaultReadBeforeExplicitOverrideAndPhysicalLimit()
    {
        int reads = 0; Assert.Equal(77u, RootServerRecords.Resolve(Raw("host"), 77, () => { reads++; return -1; }).Port); Assert.Equal(1, reads);
        Assert.Throws<InvalidOperationException>(() => RootServerRecords.Resolve(Raw("host"), 77, null));
        Assert.Equal(99u, RootServerRecords.Resolve(Raw("host:12"), 99, null).Port);
        Assert.Throws<ArgumentOutOfRangeException>(() => RootServerRecords.Resolve(new byte[80].Select(_ => (byte)'a').ToArray(), 1, () => 2));
        Assert.Equal("x", RootServerRecords.Resolve(Raw("x\0" + new string('a', 100)), 0, () => 7).Host);
    }

    /// <summary>무효이름조회는80BYTE전체를보존하며LAN은prefix/NUL만쓰고유효행조회는뒤바이트도0으로만든다.</summary>
    [Fact]
    public void ServerNameSelectionUsesDifferentNativeWriteBoundaries()
    {
        var info = new ClientNsInfoGlobals(); Array.Fill(info.ServerName, (byte)0xa5); info.ServerSelection.Value = -2;
        Assert.Equal(80, info.ResolveServerName(null).Length); Assert.All(info.ServerName, item => Assert.Equal(0xa5, item));
        info.ServerSelection.Value = -1; Assert.Equal("LAN", Text(info.ResolveServerName(_ => Raw("LAN\0ignored")).Span)); Assert.Equal(0xa5, info.ServerName[4]);
        byte[] before = info.ServerName.ToArray(); Assert.Throws<ArgumentException>(() => info.ResolveServerName(_ => new byte[80].Select(_ => (byte)'b').ToArray())); Assert.Equal(before, info.ServerName);
        info.Servers.Add("host;Named;1"); info.ServerSelection.Value = 0; Assert.Equal("Named", Text(info.ResolveServerName(null).Span)); Assert.Equal(0, info.ServerName[^1]);
    }

    /// <summary>실제행초기화와CR제거순서·긴행실패의이전목록/합계보존을확인한다.</summary>
    [Fact]
    public void LoadingNamesKeepsOrderChecksumAndFailurePrefix()
    {
        var info = new ClientNsInfoGlobals(); info.LoadServerNames(Raw("a;a;1\r\n\r\nb;b;2\n"));
        Assert.Equal(new[] { "a;a;1", "b;b;2" }, info.Servers); Assert.Equal(735u, info.ServerListChecksum);
        Assert.Throws<ArgumentException>(() => info.LoadServerNames(Raw("new;row;3\n" + new string('\r', 257))));
        Assert.Equal(new[] { "a;a;1", "b;b;2" }, info.Servers); Assert.Equal(735u, info.ServerListChecksum);
        info.LoadServerNames(Raw("a;a;1\0ignored\n")); Assert.Equal(366u, info.ServerListChecksum);
    }

    /// <summary>첫부분일치·기존값보존·signedDWORD와기존행교체의stale합계를확인한다.</summary>
    [Fact]
    public void UpsertUsesFirstSubstringAndPreservesStaleChecksumOnReplacement()
    {
        var info = new ClientNsInfoGlobals(); info.LoadServerNames(Raw("xxhostyy;Old;7\nhost;Second;8\n")); uint original = info.ServerListChecksum;
        info.UpsertServer(Raw("HOST;New;9"), true); Assert.Equal("host;New;7", info.Servers[0]); Assert.Equal(original, info.ServerListChecksum);
        info.SetServerValue(1, uint.MaxValue); Assert.Equal("host;Second;-1", info.Servers[0]); Assert.Equal("host;Second;8", info.Servers[1]); Assert.Equal(original, info.ServerListChecksum);
        info.UpsertServer(Raw("other;Third;10"), false); Assert.Equal(3, info.Servers.Count); Assert.NotEqual(original, info.ServerListChecksum);
        var saved = info.Servers.ToArray(); info.SetServerValue(-1, 0); Assert.Equal(saved, info.Servers);
    }
}
