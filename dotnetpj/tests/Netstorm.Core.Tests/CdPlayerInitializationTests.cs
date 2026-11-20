using System.Buffers.Binary;
using Netstorm.Core.Original;

// CD00406200의172바이트 행과 현재base 재조회 위치를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>CD 원문이 캡처한 행·동맹 인자와 실제 메모리의 전체 부분 효과를 확인한다.</summary>
public sealed class CdPlayerInitializationTests
{
    /// <summary>호스트 포인터와 무관한 세 실제 블록의 식별자다.</summary>
    private static readonly uint[] Bases = [0x12000000, 0x12001000, 0x12002000];
    /// <summary>독립 기대 DWORD만 기록한다.</summary>
    private static void Put(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);

    /// <summary>new가 전역base를 바꿔도 기존 행의 헤더와 현재 행의 동맹 쓰기가 구별된다.</summary>
    [Fact]
    public void NewCallbackKeepsCapturedRowButWritesAllianceIntoCurrentBase()
    {
        var blocks = new[] { new byte[1548], Enumerable.Repeat((byte)0x55, 1548).ToArray(), Enumerable.Repeat((byte)0x77, 1548).ToArray() };
        uint current = 0; int reads = 0, factories = 0; var trace = new List<string>();
        var globals = new PlayerInitializationGlobals { PlayerBase = () => { reads++; return current; }, WritePlayerBase = value => current = value };
        var hooks = new PlayerInitializationHooks
        {
            Memory = (address, bytes) => blocks[(address - Bases[0]) / 0x1000].AsMemory((int)((address - Bases[0]) % 0x1000), (int)bytes),
            MallocPlayers = bytes => { Assert.Equal(1548u, bytes); return Bases[0]; },
            NewFactory = bytes => { Assert.Equal(120u, bytes); factories++; if (factories <= 2) current = Bases[factories]; return 0x20000000u + (uint)factories * 0x100; },
            Alliance = (owner, mask) => trace.Add($"{owner}:{mask}"),
        };
        RawPlayerInitialization.InitializeCdPlayerRecords(globals, hooks);
        var expected = new[] { new byte[1548], Enumerable.Repeat((byte)0x55, 1548).ToArray(), Enumerable.Repeat((byte)0x77, 1548).ToArray() };
        // 기존 행의 다섯 헤더 쓰기와 현재base의마스크 쓰기를 독립 기대 물리에 반영한다.
        for (int owner = 0; owner < 9; owner++)
        {
            int headerBlock = owner < 2 ? owner : 2, maskBlock = owner == 0 ? 1 : 2, row = owner * 172;
            Put(expected[headerBlock], row, 0x20000100u + (uint)owner * 0x100); Put(expected[headerBlock], row + 4, 30);
            Put(expected[headerBlock], row + 0x70, 127); Put(expected[headerBlock], row + 8, 0); Put(expected[headerBlock], row + 0x2c, 0);
            Put(expected[maskBlock], row + 0x54, 1u << owner);
        }
        // 현재base는 첫1회와각행2회만 읽는다. PATCH 전용 +64/+58·mask재조회는 없다.
        Assert.Equal(19, reads); Assert.Equal(9, factories); Assert.Equal(Bases[2], current);
        for (int block = 0; block < 3; block++) Assert.Equal(expected[block], blocks[block]);
        Assert.Equal(Enumerable.Range(0, 9).Select(owner => $"{owner}:{1u << owner}"), trace);
    }

    /// <summary>free는 첫 캡처base를 사용하고 매복귀 뒤 마지막행까지 현재base를 다시 읽는다.</summary>
    [Fact]
    public void FreeReloadsAfterEveryFactoryIncludingLastBeforePlayerFree()
    {
        var blocks = new[] { new byte[1548], new byte[1548], new byte[1548] };
        // 각블록/행의토큰을서로구별하여실제선택한현재주소를검사한다.
        for (int block = 0; block < 3; block++) for (int owner = 0; owner < 9; owner++) Put(blocks[block], owner * 172, 100u + (uint)block * 10 + (uint)owner);
        uint current = Bases[0]; int loads = 0; var freed = new List<uint>(); var events = new List<string>();
        var fort = new UIntBox { Value = 7 }; var last = new UIntBox { Value = 8 };
        var globals = new PlayerInitializationGlobals(fort, last)
        {
            PlayerBase = () => { events.Add("R"); loads++; return current; }, WritePlayerBase = value => { current = value; events.Add($"W:{value}"); },
        };
        var hooks = new PlayerInitializationHooks
        {
            Memory = (address, bytes) => blocks[(address - Bases[0]) / 0x1000].AsMemory((int)((address - Bases[0]) % 0x1000), (int)bytes),
            FreeFactory = token => { Assert.Equal(0u, fort.Value); Assert.Equal(0u, last.Value); freed.Add(token); events.Add("F"); current = Bases[freed.Count % 3]; },
            FreePlayers = value => { Assert.Equal(Bases[0], value); events.Add("P"); },
            MallocPlayers = _ => throw new IOException("해제이후실제malloc실패"),
        };
        Assert.Throws<IOException>(() => RawPlayerInitialization.InitializeCdPlayerRecords(globals, hooks));
        Assert.Equal(new uint[] { 100, 111, 122, 103, 114, 125, 106, 117, 128 }, freed); Assert.Equal(10, loads); Assert.Equal(0u, current);
        Assert.Equal("R" + string.Concat(Enumerable.Repeat("FR", 9)) + "PW:0", string.Concat(events));
    }

    /// <summary>NULL new도 그대로 헤더와 동맹에 반영하며 다음 장치 누락 시 앞선 물리를 보존한다.</summary>
    [Fact]
    public void NullFactoryAndMissingAllianceKeepReachedRowAndSharedCounters()
    {
        byte[] bytes = new byte[1548]; uint current = 0; var fort = new UIntBox { Value = 7 }; var last = new UIntBox { Value = 8 };
        var globals = new PlayerInitializationGlobals(fort, last) { PlayerBase = () => current, WritePlayerBase = value => current = value };
        var hooks = new PlayerInitializationHooks { Memory = (address, count) => bytes.AsMemory((int)(address - Bases[0]), (int)count), MallocPlayers = _ => Bases[0], NewFactory = _ => 0 };
        Assert.Throws<InvalidOperationException>(() => RawPlayerInitialization.InitializeCdPlayerRecords(globals, hooks));
        byte[] expected = new byte[1548]; Put(expected, 4, 30); Put(expected, 0x70, 127); Put(expected, 0x54, 1);
        Assert.Equal(expected, bytes); Assert.Equal(Bases[0], current); Assert.Equal(0u, fort.Value); Assert.Equal(0u, last.Value);
    }
}
