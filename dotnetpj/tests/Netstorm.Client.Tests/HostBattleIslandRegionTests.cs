// 전투섬 배정 region 갈고리(+0x78 기록)의 live 연결을 검사하는 네임스페이스다.
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 전투섬 배정 region 갈고리(+0x78 기록) 검사를 담는 클래스다. 2026-11-12 0.474에서 R4 전투섬 배정 갈고리 live 연결용으로 추가했다.
public sealed class HostBattleIslandRegionTests
{
    /// <summary>배정 기록이 해당 Player의 첫 지역(+0x78)에 섬 번호를 little endian으로 쓰는지 확인한다.</summary>
    [Fact]
    public void WriteBattleIslandRegionWritesLittleEndianIsland()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        session.WriteBattleIslandRegion(1u, 0u);
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x78]);
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x79]);
        session.WriteBattleIslandRegion(2u, 0x01020304u);
        Assert.Equal(0x04, session.PlayerRecords[2 * 180 + 0x78]);
        Assert.Equal(0x03, session.PlayerRecords[2 * 180 + 0x79]);
        Assert.Equal(0x02, session.PlayerRecords[2 * 180 + 0x7a]);
        Assert.Equal(0x01, session.PlayerRecords[2 * 180 + 0x7b]);
    }

    /// <summary>배정 기록이 127 초기값을 덮어쓰고 다른 기록을 건드리지 않는지 확인한다.</summary>
    [Fact]
    public void WriteBattleIslandRegionOverwritesInitial127Only()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = new MissionPlayers();
        session.StartPlayers(players, "You");
        session.WriteBattleIslandRegion(1u, 0u);
        Assert.Equal(0, session.PlayerRecords[1 * 180 + 0x78]);
        // 다른 8기록은 127 초기값을 유지한다.
        for (int owner = 0; owner <= 8; owner++)
        {
            if (owner == 1) continue;
            Assert.Equal(127, session.PlayerRecords[owner * 180 + 0x78]);
        }
    }

    /// <summary>9번 이상 소유자의 배정 기록을 원문처럼 범위 밖으로 거부하는지 확인한다.</summary>
    [Fact]
    public void WriteBattleIslandRegionRejectsOutOfRangeOwner()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.WriteBattleIslandRegion(9u, 0u));
    }

    /// <summary>갈고리 대리자가 세션 기록에 그대로 쓰는 통로인지 확인한다.</summary>
    [Fact]
    public void RegionHookDelegateWritesThroughSession()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Action<uint, uint> region = session.BuildBattleIslandRegionHook();
        region(2u, 1u);
        Assert.Equal(1, session.PlayerRecords[2 * 180 + 0x78]);
        Assert.Throws<ArgumentOutOfRangeException>(() => region(9u, 0u));
    }

    /// <summary>실제 thewarbegins 요새에는 배틀 플래그가 없어 0번을 고르는지 확인한다.</summary>
    [Fact]
    public void SelectBattleTerritoryFallsBackToZeroForTheWarBegins()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        Assert.True(fort.Fort.Territory.Length >= 120);
        Assert.Equal(0, RawBattleIsland.SelectBattleTerritory(fort.Fort.Territory.AsSpan(0, 120)));
    }

    /// <summary>0x04 플래그가 켜진 첫 영역을 고르는지 합성 바이트로 확인한다.</summary>
    [Fact]
    public void SelectBattleTerritoryPicksFirstBattleFlag()
    {
        var records = new byte[120];
        for (int i = 0; i < 20; i++) records[i * 6 + 1] = 0x03;
        records[1 * 6 + 1] = 0x01;
        records[2 * 6 + 1] = 0x05;
        Assert.Equal(2, RawBattleIsland.SelectBattleTerritory(records));
    }

    /// <summary>짧은 입력의 영역 선택을 거부하는지 확인한다.</summary>
    [Fact]
    public void SelectBattleTerritoryRejectsShortInput()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBattleIsland.SelectBattleTerritory(new byte[119]));
    }

    /// <summary>실제 thewarbegins 요새의 전투 소유자 시작 섬이 0번으로 풀리는지 확인한다.</summary>
    [Fact]
    public void ResolveBattleIslandRegionIsZeroForTheWarBegins()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        Assert.True(HostSession.TryResolveBattleIslandRegion(fort.Fort.Territory[..120], out uint island));
        Assert.Equal(0u, island);
    }

    /// <summary>앞선 배치 가능 레코드 수를 섬 번호로 세는지 합성 바이트로 확인한다.</summary>
    [Fact]
    public void ResolveBattleIslandRegionCountsPrecedingPlaceables()
    {
        var records = new byte[120];
        for (int i = 0; i < 20; i++) records[i * 6 + 1] = 0x03;
        records[1 * 6 + 1] = 0x01;
        records[2 * 6 + 1] = 0x05;
        Assert.True(HostSession.TryResolveBattleIslandRegion(records, out uint island));
        Assert.Equal(1u, island);
    }

    /// <summary>고른 영역이 배치 불가면 127 유지를 위해 거짓을 돌려주는지 확인한다.</summary>
    [Fact]
    public void ResolveBattleIslandRegionRejectsUnplaceableSelection()
    {
        var records = new byte[120];
        for (int i = 0; i < 20; i++) records[i * 6 + 1] = 0x03;
        records[0 * 6 + 1] = 0x04;
        Assert.False(HostSession.TryResolveBattleIslandRegion(records, out _));
        Assert.False(HostSession.TryResolveBattleIslandRegion(new byte[119], out _));
    }
}
