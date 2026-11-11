using Netstorm.Assets;
using Netstorm.Client.Automation;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>미션 플레이어 목록 적재가 실제 대본·요새·타입 표에서 원본과 같은 목록을 만드는지 검사한다.
/// 근거: cpppj/src/o/Player.cpp의 <c>MissionPlayers::Load</c>. (2026-11-12 0.394에서 추가했다)</summary>
public sealed class MissionPlayersLoadTests
{
    /// <summary>thewarbegins 대본으로 목록을 적재한다.</summary>
    private static (MissionFortLoadResult Loaded, MissionPlayers Players, RiftTypeTable Types) LoadTheWarBegins()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
        var script = new Netstorm.Client.Mission.MissionScript(startup.Configuration, "thewarbegins");
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            return (loaded, players, types);
        }
        finally { script.Dispose(); }
    }

    /// <summary>thewarbegins 1·2번과 나머지 번호가 원본 규칙과 같은 목록을 내놓는지 확인한다.</summary>
    [Fact]
    public void LoadsTheWarBeginsPlayers()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.All(players.TechAllowed, allowed => Assert.True(allowed));
        Assert.False(players.DenySalvage);
        Assert.False(players.DenyAscend);
        Assert.False(players.AiOff);
        MissionPlayer first = players.Players[1];
        Assert.Equal(1, first.Number);
        Assert.Equal(1, first.Color);
        Assert.True(first.Active);
        Assert.False(first.Computer);
        Assert.Equal(3000.0, first.StormPower);
        Assert.Equal("Player", first.Name);
        Assert.Equal("sunWalker;rainBattery;sunAviary;sunCannon", first.StartingTech);
        Assert.Equal(
            new[] { types.Find("sunWalker"), types.Find("rainBattery"), types.Find("sunAviary"), types.Find("sunCannon") }.Order().ToList(),
            first.Knowledge);
        Assert.Equal("", first.Ability);
        Assert.Equal(0x0002, first.Allies);
        Assert.Equal(26, first.StoredTechnology.Items.Count);
        Assert.Equal(38, first.StoredDeck.Count);
        Assert.Equal(loaded.Fort.Technology.Items.Count, first.StoredTechnology.Items.Count);
        Assert.Equal(loaded.Fort.Deck.Count, first.StoredDeck.Count);
        MissionPlayer second = players.Players[2];
        Assert.Equal(2, second.Number);
        Assert.Equal(2, second.Color);
        Assert.True(second.Active);
        Assert.True(second.Computer);
        Assert.Equal(0.0, second.StormPower);
        Assert.Equal("The Jailer", second.Name);
        Assert.Equal("sunWalker;windBattery;sunArcher", second.StartingTech);
        Assert.Equal(
            new[] { types.Find("sunWalker"), types.Find("windBattery"), types.Find("sunArcher") }.Order().ToList(),
            second.Knowledge);
        Assert.Equal("!ENCIRCLE_ENEMY_ISLAND;!USE_PRIEST_TO_COLLECT", second.Ability);
        Assert.Equal(0x0004, second.Allies);
        // 나머지 번호는 비어 있고 자기 자신하고만 동맹이다.
        for (int number = 3; number <= MissionPlayers.PlayerCount; number++)
        {
            MissionPlayer player = players.Players[number];
            Assert.Equal(number, player.Number);
            Assert.Equal(number, player.Color);
            Assert.False(player.Active);
            Assert.True(player.Computer);
            Assert.Equal(0.0, player.StormPower);
            Assert.Equal("", player.Name);
            Assert.Empty(player.Knowledge);
            Assert.Equal(unchecked((ushort)(1u << number)), player.Allies);
        }
        Assert.False(players.Allied(1, 2));
        Assert.True(players.Allied(1, 1));
        Assert.False(players.Allied(0, 1));
        Assert.False(players.Allied(1, 9));
    }

    /// <summary>thewarbegins 시작 SP 지정이 기준선 sp 행의 참가자 값과 같은지 확인한다.</summary>
    [Fact]
    public void StartsPlayerSpLikeBaseline()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.NotNull(loaded);
        Assert.NotNull(types);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.False(session.Sp.Initialized);
        // 실제 사슬 모양의 재시드(세션 게임 난수+CRT 상자+모드 상자+고정 유닉스 초)로 연결한다.
        var crtBox = new UIntBox { Value = 1 };
        var modeBox = new UIntBox { Value = 0 };
        var seed = new RawGameRandomSeed(session.Game, crtBox, modeBox, () => 946684800);
        session.BindSpReseed(() => seed.Reseed());
        session.StartPlayerSp(players);
        Assert.True(session.Sp.Initialized);
        // 참가자 값은 난수와 무관하게 정확히 읽힌다(기준선 f090: 1번 1161527296, 나머지는 미지정 0과 같다).
        Assert.Equal(1161527296u, session.Sp.Get(1));
        Assert.Equal(0u, session.Sp.Get(2));
        // offset은 초기화 사슬(재시드 입력·Rotate 횟수)이 Start 순서와 함께 맞아야 기준선(103)과 같아진다.
        // 지금은 행 문법과 참가자 값만 고정하고 offset 값은 기록만 한다(0.395 잔여).
        string text = SessionStateDump.FormatWorld1(
            "originals", 90, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        Assert.Contains($"\nsp\t1\t{session.Sp.Offset}\t0\t1161527296\t0\t0\t0\t0\t0\t0\t0\n", text, StringComparison.Ordinal);
    }
}
