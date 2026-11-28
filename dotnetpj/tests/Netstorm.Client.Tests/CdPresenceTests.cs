using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 독립 PE의 전체 cache·출력·OS scratch와 실제 호출 순서를 비교한다.
namespace Netstorm.Client.Tests;

/// <summary>CD 루트 열거·현재 캐시 판단·Client 소유자의 선택 경계를 검증한다.</summary>
public sealed class CdPresenceTests
{
    /// <summary>원문 BYTE를 소문자16진수로만 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>콜백 변이 입력을 같은 세128BYTE 물리에 설치한다.</summary>
    [Fact]
    public void MatchesAll492NativeBuffersAndCalls()
    {
        string[][] rows = X86Fixture.Read("cd-presence"); Assert.Equal(492, rows.Length);
        // 최종 기대 버퍼는 새 본체로 재계산하지 않고 모든 독립 원문 행을 비교한다.
        foreach (string[] row in rows)
        {
            byte[] cache = Enumerable.Repeat((byte)205, 128).ToArray(), output = (byte[])cache.Clone(), logical = (byte[])cache.Clone();
            byte[] initial = X86Fixture.Bytes(row[1]); initial.CopyTo(cache, 0); cache[initial.Length] = 0;
            byte[] drives = X86Fixture.Bytes(row[2]);
            uint[] types = row[3] == "-" ? [] : row[3].Split(',').Select(X86Fixture.UInt).ToArray();
            uint logicalResult = X86Fixture.UInt(row[4]), capacity = X86Fixture.UInt(row[5]), mask = X86Fixture.UInt(row[6]), mutation = X86Fixture.UInt(row[7]);
            int typeIndex = 0, probes = 0; bool outputUsed = row[0] == "roots", logicalUsed = false;
            ReadOnlyMemory<byte> pathRoot = default; var events = new List<string>(); var names = new RawFileName();
            var roots = new CdDriveRootsHooks
            {
                LogicalDrives = buffer => { logicalUsed = true; events.Add("L:128"); drives.CopyTo(buffer.Span); return logicalResult; },
                DriveType = root =>
                {
                    events.Add("T:" + Hex(root.Span)); uint type = types[typeIndex++];
                    if (mutation != 0 && typeIndex == 1) MemoryMarshal.AsMemory(root).Span[0] = (byte)'Z';
                    return type;
                },
            };
            var hooks = new CdPresenceHooks
            {
                Path = (root, marker) => { Assert.Equal(CdPresence.Marker, marker); events.Add("P:" + Hex(root.Span)); pathRoot = root; return CdPresence.JoinRootPath(names, root, marker); },
                Exists = path =>
                {
                    events.Add("E:" + Hex(Encoding.Latin1.GetBytes(path))); uint result = (mask >> probes++) & 1u;
                    if (mutation != 0 && probes == 1) MemoryMarshal.AsMemory(pathRoot).Span[0] = (byte)'Z';
                    return result != 0;
                },
                Roots = buffer => { outputUsed = true; return CdPresence.CollectRoots(128, buffer, logical, roots); },
            };
            uint result = row[0] == "roots" ? CdPresence.CollectRoots(capacity, output, logical, roots) : CdPresence.Check(cache, output, hooks);
            Assert.Equal(X86Fixture.UInt(row[8]), result); Assert.Equal(row[9], Hex(cache));
            Assert.Equal(row[10], outputUsed ? Hex(output) : "-"); Assert.Equal(row[11], logicalUsed ? Hex(logical) : "-");
            Assert.Equal(row[12], events.Count == 0 ? "-" : string.Join(';', events));
        }
    }

    /// <summary>캐시 실패의 첫BYTE0·NULL 진단·미선택 OS 경계와 짧은 물리의 전방 쓰기를 확인한다.</summary>
    [Fact]
    public void MissingAndShortBoundariesPreserveCompletedEffects()
    {
        byte[] cache = Enumerable.Repeat((byte)'x', 128).ToArray(), scratch = new byte[128]; "Q:\\\0"u8.CopyTo(cache);
        Assert.Throws<InvalidOperationException>(() => CdPresence.Check(cache, scratch, new() { Path = (_, _) => "marker", Exists = _ => false }));
        Assert.Equal(0, cache[0]); Assert.Equal((byte)':', cache[1]); Assert.Equal((byte)'x', cache[4]);
        int assertions = 0;
        Assert.Equal(0u, CdPresence.CollectRoots(0, Memory<byte>.Empty, scratch, new()
        {
            Assertion = line => { Assert.Equal(0x529u, line); assertions++; }, LogicalDrives = _ => 0,
        }));
        Assert.Equal(1, assertions); Assert.Equal(0u, CdPresence.Check(cache, scratch, new() { Roots = _ => 0 }));
        byte[] shortCache = new byte[2];
        Assert.Throws<ArgumentOutOfRangeException>(() => CdPresence.Check(shortCache, scratch, new()
        {
            Path = (_, _) => "marker", Exists = _ => true,
            Roots = buffer => { "D:\\\0\0"u8.CopyTo(buffer.Span); return 5; },
        }));
        Assert.Equal(new byte[] { (byte)'D', (byte)':' }, shortCache);
    }

    /// <summary>최초 결과·State7 결과·재확인 serial을 분리하고 캐시 성공에서 열거하지 않는다.</summary>
    [Fact]
    public void CurrentCheckPreservesSeparateStartupAndStateOwners()
    {
        var owner = new ClientCdGlobals(); var local = new ClientLocalStateGlobals(new(), new());
        local.CdPresent.Value = 7; owner.RecheckSerial.Value = 31;
        var names = new RawFileName();
        var hooks = new CdPresenceHooks
        {
            Path = (root, marker) => CdPresence.JoinRootPath(names, root, marker), Exists = _ => true,
            Roots = output => { "D:\\\0\0"u8.CopyTo(output.Span); return 5; },
        };
        owner.InitializeStartup(hooks); Assert.Equal(1u, owner.StartupPresent);
        Assert.Equal(1u, owner.Check(new() { Path = hooks.Path, Exists = hooks.Exists, Roots = _ => throw new InvalidOperationException("캐시 성공의 열거 미선택") }));
        Assert.Equal(7u, local.CdPresent.Value); Assert.Equal(31u, owner.RecheckSerial.Value);
        Assert.Throws<IOException>(() => owner.InitializeStartup(new() { Path = (_, _) => { Assert.Equal(0u, owner.StartupPresent); throw new IOException("현재 경로 실패"); } }));
        Assert.Equal(0u, owner.StartupPresent); Assert.Equal(31u, owner.RecheckSerial.Value); Assert.Equal(7u, local.CdPresent.Value);
    }
}
