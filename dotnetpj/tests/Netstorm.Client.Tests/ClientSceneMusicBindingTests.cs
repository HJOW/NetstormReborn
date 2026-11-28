using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// Game가 사용하는 같은 현재 owner 바인딩을 창·음향 장치·감독 실행 없이 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 조회/공유 DWORD와 연결 전 정체성 거부를 실제 장기 소유자에서 검증한다.</summary>
public sealed class ClientSceneMusicBindingTests
{
    /// <summary>바인딩에서는 읽히면 안 되는 시계와 같은 실제 Client 소유자를 준비한다.</summary>
    private static (ClientGameplayGlobals Game, ClientSurfaceGlobals Surface, ClientLocalStateGlobals Local,
        ClientConnectionGlobals Connections) Owners()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        return (game, new(game, new(local), new(), () => throw new IOException("장면 결선 중 시각 조회 금지")), local, new(local));
    }

    /// <summary>현재 Player/battle을 소비 위치에서 읽으며 실제 State와 Cloud의 같은 DWORD를 값 복사 없이 빌린다.</summary>
    [Fact]
    public void GameBindingReadsCurrentPlayerBattleAndSameCloudBoxWithoutResettingValues()
    {
        var (game, surface, local, connections) = Owners(); var resources = new ClientStateResources(); var cloud = new ClientKencloudGlobals();
        SceneMusicState music = resources.Music; UIntBox result = music.ResultStateBox, originalDirty = music.PaletteDirtyBox;
        music.PlayersReady = 71; music.LocalPlayer = 72; music.Battle = 73; music.PaletteDirty = 74; cloud.WorldRedraw = 75;
        music.ResultState = 0x81234567; music.SongEnd.Value = BitConverter.UInt64BitsToDouble(0x7ff8000012345678);
        game.Player.PlayerKnown = 0x80000000; game.Player.BattleOwnerBox.Value = uint.MaxValue;
        resources.BindCurrentMusicOwners(game, surface, local, connections, cloud);
        Assert.Same(cloud.WorldRedrawBox, music.PaletteDirtyBox); Assert.Equal(75u, music.PaletteDirty); Assert.Equal(74u, originalDirty.Value);
        Assert.Equal(0x80000000u, music.CurrentPlayersReady!()); Assert.Equal(uint.MaxValue, music.CurrentLocalPlayer!()); Assert.Equal(0u, music.CurrentBattle!());
        game.Player.PlayerKnown = 7; game.Player.BattleOwnerBox.Value = 8; surface.Storage.OwnerMode.Battle = true;
        Assert.Equal(7u, music.CurrentPlayersReady!()); Assert.Equal(8u, music.CurrentLocalPlayer!()); Assert.Equal(1u, music.CurrentBattle!());
        var globals = ClientStateWorldGlobals.Bind(resources.Context(surface, local, connections, cloud, new()));
        globals.Write32!(0x59a8b0, 0x92345678); Assert.Equal(0x92345678u, music.PaletteDirty);
        music.PaletteDirty = uint.MaxValue; Assert.Equal(uint.MaxValue, globals.Read32!(0x59a8b0));
        Assert.Equal(71u, music.PlayersReady); Assert.Equal(72u, music.LocalPlayer); Assert.Equal(73u, music.Battle);
        Assert.Same(result, music.ResultStateBox); Assert.Equal(0x81234567u, music.ResultState);
        Assert.Equal(0x7ff8000012345678UL, BitConverter.DoubleToUInt64Bits(music.SongEnd.Value));
    }

    /// <summary>startup이 없는 실제 호스트의 경로는 Player/battle만 연결하고 독립 dirty 물리를 유지한다.</summary>
    [Fact]
    public void MissingStartupCloudBindsOnlyExistingPlayerAndBattle()
    {
        var (game, surface, local, connections) = Owners(); var resources = new ClientStateResources();
        UIntBox dirty = resources.Music.PaletteDirtyBox; dirty.Value = 0x81234567;
        resources.BindCurrentMusicOwners(game, surface, local, connections, null);
        game.Player.PlayerKnown = 3; game.Player.BattleOwnerBox.Value = 6; surface.Storage.OwnerMode.Battle = true;
        Assert.Equal(3u, resources.Music.CurrentPlayersReady!()); Assert.Equal(6u, resources.Music.CurrentLocalPlayer!());
        Assert.Equal(1u, resources.Music.CurrentBattle!()); Assert.Same(dirty, resources.Music.PaletteDirtyBox); Assert.Equal(0x81234567u, dirty.Value);
    }

    /// <summary>한 Client처럼 보이는 다른 물리를 섞으면 기존 getter/상자/값을 바꾸기 전에 거부한다.</summary>
    [Theory]
    [InlineData("gameplay")]
    [InlineData("local")]
    [InlineData("connections")]
    [InlineData("clock")]
    public void MixedOwnersFailBeforeChangingAnyBinding(string mismatch)
    {
        var (game, surface, local, connections) = Owners(); var resources = new ClientStateResources(); var cloud = new ClientKencloudGlobals();
        Func<uint> ready = () => throw new IOException("실패한 결선에서 이전 ready 실행 금지");
        Func<uint> player = () => throw new IOException("실패한 결선에서 이전 local 실행 금지");
        Func<uint> battle = () => throw new IOException("실패한 결선에서 이전 battle 실행 금지");
        resources.Music.CurrentPlayersReady = ready; resources.Music.CurrentLocalPlayer = player; resources.Music.CurrentBattle = battle;
        UIntBox dirty = resources.Music.PaletteDirtyBox, result = resources.Music.ResultStateBox; dirty.Value = 71; result.Value = 72; cloud.WorldRedraw = 73;
        // 실제 생성자가 허용하는 서로 다른 owner 조합만 공급하며 내부 readonly 물리는 수정하지 않는다.
        switch (mismatch)
        {
            case "gameplay": game = new(); break;
            case "local": local = new(new(), new()); break;
            case "connections": connections = new(new(game, new())); break;
            case "clock": surface = new(game, new(new(game, new())), new(), () => throw new IOException("실패한 결선에서 시각 조회 금지")); break;
            default: throw new ArgumentOutOfRangeException(nameof(mismatch));
        }
        Assert.Throws<ArgumentException>(() => resources.BindCurrentMusicOwners(game, surface, local, connections, cloud));
        Assert.Same(ready, resources.Music.CurrentPlayersReady); Assert.Same(player, resources.Music.CurrentLocalPlayer); Assert.Same(battle, resources.Music.CurrentBattle);
        Assert.Same(dirty, resources.Music.PaletteDirtyBox); Assert.Equal(71u, dirty.Value); Assert.Equal(73u, cloud.WorldRedraw);
        Assert.Same(result, resources.Music.ResultStateBox); Assert.Equal(72u, result.Value);
    }

    /// <summary>Game의 음향 공개 관문은 실제 준비 음향의 같은 Scene만 받으며 다른 Client의 장면을 거부한다.</summary>
    [Fact]
    public void ActualPreparedAudioMustBorrowSameSceneBeforePublication()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var resources = new ClientStateResources();
        resources.Music.ResultState = 0x81234567;
        using var audio = new HostMissionAudio(new HostStartupScene(data), () => throw new IOException("장면 확인 중 벽시각 조회 금지"), resources.Music);
        resources.RequireMusicOwner(audio.Scene); Assert.Same(resources.Music, audio.Scene);
        Assert.Throws<ArgumentException>(() => new ClientStateResources().RequireMusicOwner(audio.Scene));
        Assert.Throws<ArgumentNullException>(() => resources.RequireMusicOwner(null!));
        Assert.False(audio.DeviceReady); Assert.Empty(audio.CurrentMusicName); Assert.Equal(0x81234567u, resources.Music.ResultState);
    }
}
