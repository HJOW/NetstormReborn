using System.Reflection;
// 실제 준비 음향의 파일/채널 소유자는 값 보정 없이 바인딩 전후로만 관측한다.
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// SceneMusic의 현재 조회/공유DWORD와 실제 State view 사이의 좁은 별칭 계약을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 순서의 소비 위치와 부분 효과만 검사하며 실제 Game/State/Scene 부모 결선 완료를 합성하지 않는다.</summary>
public sealed class SceneMusicCurrentOwnersTests
{
    /// <summary>현재 조회의 순서/실패를 명시하는 독립 장치다. 파일·음향 준비 성공을 제공하지 않는다.</summary>
    private sealed class Probe
    {
        /// <summary>각 소비가 실제로 실행된 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>검사 대상과 같은 장기 전역이다.</summary>
        public readonly SceneMusicState State;
        /// <summary>생성/Next가 소비하지 않는 같은 게임 난수다.</summary>
        public readonly GameRandom Random = new(17);
        /// <summary>모든 검사가 실제로 호출하는 동일 감독이다.</summary>
        public readonly SceneMusic Music;
        /// <summary>각 이름 조회가 현재 읽는 명시 입력이다.</summary>
        public string Name = "before.mus";
        /// <summary>각 이름 조회의 재진입 효과를 명시한다.</summary>
        public Action? OnName;
        /// <summary>현재 localPlayer 인자의 명시 희생 판정이다.</summary>
        public Func<uint, bool> Sacrifice = _ => false;
        /// <summary>날씨 옵션의 명시 입력이다.</summary>
        public bool PaletteEnabled;
        /// <summary>팔레트 함수가 정상 복귀/예외를 내는 현재 효과다.</summary>
        public Action<string>? OnPalette;

        /// <summary>모든 필수 경계를 연결하되 생성 자체는 현재값·시계·난수를 소비하지 않는다.</summary>
        public Probe(SceneMusicState? state = null)
        {
            State = state ?? new();
            Music = new(State, Random, new()
            {
                WallSeconds = () => { Events.Add("clock"); return 100; },
                CurrentName = () => { Events.Add("name"); OnName?.Invoke(); return Name; },
                Select = name => { Events.Add("select:" + name); Name = name; },
                Duration = () => { Events.Add("duration"); return 0; },
                Sacrificing = player => { Events.Add("sacrifice:" + player); return Sacrifice(player); },
                WaitingRoom = () => throw new IOException("이 검사에서 대기실 선택 금지"),
                Refresh = () => Events.Add("refresh"),
                AscendancyPalette = () => { Events.Add("option"); return PaletteEnabled; },
                LoadPalette = name => { Events.Add("palette:" + name); OnPalette?.Invoke(name); },
                ThunderFlash = () => Events.Add("flash:" + State.PaletteDirty),
                WeatherSound = name => Events.Add("sound:" + name + ":" + State.PaletteDirty),
            });
        }
    }

    /// <summary>실제 private owner의 동일성/물리만 읽고 필드나 성공 표식을 수정하지 않는다.</summary>
    private static T Field<T>(object owner, string name) => (T)(owner.GetType()
        .GetField(name, BindingFlags.Instance | BindingFlags.NonPublic)?.GetValue(owner)
        ?? throw new MissingFieldException(owner.GetType().FullName, name));

    /// <summary>실제 State 접근의 쓰기가 Scene의 같은 Player/Cloud/SongEnd에 즉시 보이며 독립 fallback은 덮지 않는다.</summary>
    [Fact]
    public void StateWidthViewsAndSceneShareCurrentOwnersWithoutCopyingValues()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => throw new IOException("별칭 설치 중 시각 읽기 금지"));
        var cloud = new ClientKencloudGlobals(); var resources = new ClientStateResources(); SceneMusicState state = resources.Music;
        var globals = ClientStateWorldGlobals.Bind(resources.Context(surface, local, new(local), cloud, new()));
        state.PlayersReady = 73; state.LocalPlayer = 74; state.PaletteDirty = 75; UIntBox standaloneDirty = state.PaletteDirtyBox;
        cloud.WorldRedraw = 76; game.MapRevision.Value = 0x12345678;
        state.CurrentPlayersReady = () => game.Player.PlayerKnown;
        state.CurrentLocalPlayer = () => game.Player.BattleOwnerBox.Value;
        state.BindPaletteDirty(cloud.WorldRedrawBox);
        Assert.Same(cloud.WorldRedrawBox, state.PaletteDirtyBox); Assert.Equal(76u, state.PaletteDirty); Assert.Equal(75u, standaloneDirty.Value);
        // 전체DWORD의 부호 비트도 bool로 정규화하거나 다른 소유자에 복사하지 않는다.
        foreach (uint bits in new uint[] { 0, 1, 0x80000000, uint.MaxValue })
        {
            globals.Write32!(0x595344, bits); Assert.Equal(bits, state.CurrentPlayersReady!());
            game.Player.BattleOwnerBox.Value = bits; Assert.Equal(bits, globals.Read32!(0x540c70)); Assert.Equal(bits, state.CurrentLocalPlayer!());
            globals.Write32!(0x59a8b0, bits); Assert.Equal(bits, state.PaletteDirty);
            state.PaletteDirty = ~bits; Assert.Equal(~bits, globals.Read32!(0x59a8b0)); Assert.Equal(~bits, cloud.WorldRedraw);
            state.ResultState = bits; Assert.Equal(bits, state.ResultStateBox.Value);
            state.ResultStateBox.Value = ~bits; Assert.Equal(~bits, state.ResultState);
        }
        // State가 쓰는 같은8BYTE의 음수0/NaN payload를 바인딩이 바꾸지 않는다.
        foreach (ulong bits in new ulong[] { 0x8000000000000000, 0x7ff8000012345678 })
        {
            globals.Write64Bits!(0x565e68, bits); Assert.Equal(bits, BitConverter.DoubleToUInt64Bits(state.SongEnd.Value));
        }
        Assert.Equal(73u, state.PlayersReady); Assert.Equal(74u, state.LocalPlayer); Assert.Equal(75u, standaloneDirty.Value);
        Assert.Equal(0x12345678u, game.MapRevision.Value);
        Assert.Throws<ArgumentNullException>(() => state.BindPaletteDirty(null!)); Assert.Same(cloud.WorldRedrawBox, state.PaletteDirtyBox);
    }

    /// <summary>ready0은 색인 증가 뒤 읽히며 local/희생 함수는 선택하지 않는다.</summary>
    [Fact]
    public void ZeroCurrentReadySkipsLocalAndSacrificeAfterStoringIndex()
    {
        var state = new SceneMusicState { PlayersReady = 1, LocalPlayer = 7 }; var probe = new Probe(state);
        state.CurrentPlayersReady = () => { probe.Events.Add("ready:" + state.Index); return 0; };
        state.CurrentLocalPlayer = () => throw new IOException("ready0에서 local 조회 금지");
        probe.Sacrifice = _ => throw new IOException("ready0에서 희생 판정 금지");
        probe.Music.Next();
        Assert.Equal("ready:0", probe.Events[0]); Assert.Contains("select:wind22.mus", probe.Events);
        Assert.Equal(0, state.Index); Assert.Equal(1u, state.PlayersReady); Assert.Equal(7u, state.LocalPlayer); Assert.Equal(17u, probe.Random.State());
    }

    /// <summary>ready의 재진입 효과 뒤에 설치된 local getter를 읽고 다음 호출에서도 현재 owner를 다시 읽는다.</summary>
    [Fact]
    public void ReadyEffectsPrecedeLocalLookupAndEachNextReadsAgain()
    {
        var game = new ClientGameplayGlobals(); game.Player.PlayerKnown = 0x80000000; game.Player.BattleOwnerBox.Value = 2;
        var state = new SceneMusicState { PlayersReady = 0, LocalPlayer = 99 }; var probe = new Probe(state); int readyReads = 0;
        state.CurrentLocalPlayer = () => throw new IOException("ready 이전의 오래된 local 함수");
        state.CurrentPlayersReady = () =>
        {
            probe.Events.Add("ready:" + state.Index);
            if (++readyReads == 1)
            {
                game.Player.BattleOwnerBox.Value = 8;
                state.CurrentLocalPlayer = () => { probe.Events.Add("local"); return game.Player.BattleOwnerBox.Value; };
            }
            return game.Player.PlayerKnown;
        };
        probe.Sacrifice = _ => true;
        probe.Music.Next(); Assert.Equal(new[] { "ready:0", "local", "sacrifice:8" }, probe.Events.Take(3));
        Assert.DoesNotContain("refresh", probe.Events);
        game.Player.BattleOwnerBox.Value = 6; probe.Events.Clear(); probe.Music.Next();
        Assert.Equal(new[] { "ready:1", "local", "sacrifice:6" }, probe.Events.Take(3)); Assert.Equal(2, readyReads);
        Assert.Equal(0x80000000u, game.Player.PlayerKnown); Assert.Equal(0u, state.PlayersReady); Assert.Equal(99u, state.LocalPlayer); Assert.Equal(17u, probe.Random.State());
    }

    /// <summary>getter 예외는 색인 쓰기와 앞선 조회만 남기고 희생/곡/시계/날씨로 진행하지 않는다.</summary>
    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void CurrentGetterFailurePreservesOnlyNativePrefix(bool readyFails)
    {
        var state = new SceneMusicState(); var probe = new Probe(state); var failure = new IOException("선택된 현재 조회 실패");
        state.CurrentPlayersReady = () => { probe.Events.Add("ready"); if (readyFails) throw failure; return 1; };
        state.CurrentLocalPlayer = () => { probe.Events.Add("local"); throw failure; };
        Assert.Same(failure, Assert.Throws<IOException>(probe.Music.Next));
        Assert.Equal(readyFails ? new[] { "ready" } : new[] { "ready", "local" }, probe.Events);
        Assert.Equal(0, state.Index); Assert.Equal(17u, probe.Random.State()); Assert.Equal(0.0, state.SongEnd.Value);
    }

    /// <summary>두 getter를 각각 제거하면 해당 독립 입력만 다시 소비하며 현재 조회가 fallback 값을 덮지 않는다.</summary>
    [Fact]
    public void RemovingCurrentProvidersRestoresIndependentFallbackAtUse()
    {
        var state = new SceneMusicState { PlayersReady = 1, LocalPlayer = 9 }; var probe = new Probe(state);
        state.CurrentPlayersReady = () => 0; state.CurrentLocalPlayer = () => 7; probe.Sacrifice = _ => true;
        probe.Music.Next(); Assert.DoesNotContain(probe.Events, value => value.StartsWith("sacrifice:", StringComparison.Ordinal));
        state.CurrentPlayersReady = null; probe.Events.Clear(); probe.Music.Next(); Assert.Equal("sacrifice:7", probe.Events[0]);
        state.CurrentLocalPlayer = null; probe.Events.Clear(); probe.Music.Next(); Assert.Equal("sacrifice:9", probe.Events[0]);
        Assert.Equal(1u, state.PlayersReady); Assert.Equal(9u, state.LocalPlayer);
    }

    /// <summary>현재 이름 조회가 같은 결과DWORD를 바꾸면 뒤 fanfare 관문이 그 현재값을 소비한다.</summary>
    [Fact]
    public void FanfareReadsSingleResultBoxAfterCurrentNameEffects()
    {
        var state = new SceneMusicState(); var probe = new Probe(state); UIntBox outcome = state.ResultStateBox; int reads = 0;
        probe.OnName = () => { if (++reads == 3) outcome.Value = 1; };
        probe.Music.Request(SceneMusic.FanfareMusic);
        Assert.Same(outcome, state.ResultStateBox); Assert.Equal(1u, state.ResultState);
        Assert.Equal(new[] { "name", "name", "name", "select:fanfare.mus", "duration", "clock", "duration", "clock" }, probe.Events);
        probe.Name = "before.mus"; probe.Events.Clear(); probe.OnName = null; outcome.Value = 2;
        probe.Music.Request(SceneMusic.FanfareMusic); Assert.Equal(new[] { "name", "name", "name" }, probe.Events);
    }

    /// <summary>실제 팔레트 함수가 정상 복귀한 뒤 같은 Cloud DWORD를1로 쓰고 번개/효과음은 그 쓰기 뒤에 실행한다.</summary>
    [Fact]
    public void WeatherWritesBorrowedDirtyAfterPaletteAndBeforeFlash()
    {
        var cloud = new ClientKencloudGlobals(); cloud.WorldRedraw = 0x80000000;
        var state = new SceneMusicState { Index = 2 }; state.Tints[2] = 0x12345678; state.BindPaletteDirty(cloud.WorldRedrawBox);
        var probe = new Probe(state) { PaletteEnabled = true };
        probe.OnPalette = _ => { Assert.Equal(0x12345678u, state.Tint); Assert.Equal(0x80000000u, cloud.WorldRedraw); cloud.WorldRedraw = 7; };
        probe.Music.ApplyWeather();
        Assert.Equal(new[] { "refresh", "option", "palette:thundery.col", "flash:1", "sound:thunderCrack.wav:1" }, probe.Events);
        Assert.Equal(1u, cloud.WorldRedraw); Assert.Same(cloud.WorldRedrawBox, state.PaletteDirtyBox);
    }

    /// <summary>팔레트 예외에서는 먼저 쓴 Tint만 유지하며 같은 dirty/번개/효과음의 뒤 효과는 만들지 않는다.</summary>
    [Fact]
    public void PaletteFailureLeavesBorrowedDirtyAndLaterEffectsUntouched()
    {
        var cloud = new ClientKencloudGlobals(); cloud.WorldRedraw = 0x80000000;
        var state = new SceneMusicState { Index = 2 }; state.Tints[2] = 71; state.BindPaletteDirty(cloud.WorldRedrawBox);
        var probe = new Probe(state) { PaletteEnabled = true }; var failure = new IOException("현재 팔레트 실패"); probe.OnPalette = _ => throw failure;
        Assert.Same(failure, Assert.Throws<IOException>(probe.Music.ApplyWeather));
        Assert.Equal(new[] { "refresh", "option", "palette:thundery.col" }, probe.Events);
        Assert.Equal(71u, state.Tint); Assert.Equal(0x80000000u, cloud.WorldRedraw);
    }

    /// <summary>같은 실제 준비 음향/Scene에 getter·Cloud를 결합해도 시계/RNG/파일/두 raw채널을 소비하지 않는다.</summary>
    [Fact]
    [SupportedOSPlatform("windows")]
    public void BindingPreparedActualOwnersDoesNotTouchClockRandomFilesOrChannels()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data);
        var resources = new ClientStateResources(); SceneMusicState state = resources.Music; int clockReads = 0, ownerReads = 0, commands = 0;
        // 실제 준비 음향과 Scene이 같은 벽시각을 빌리며 바인딩에서는 선택되면 안 된다.
        double Clock() { clockReads++; throw new IOException("바인딩 중 벽시각 소비 금지"); }
        using var audio = new HostMissionAudio(startup, Clock, state);
        MusicChannelState primary = Field<MusicChannelState>(audio, "_primaryMusicState"), secondary = Field<MusicChannelState>(audio, "_secondaryMusicState");
        // 준비/파일/버퍼 필드는0을 유지하고 서로 다른 패딩BYTE만 두어 무의미한 재초기화도 감지한다.
        primary.Raw().Span[0x14] = 0xa5; secondary.Raw().Span[0x44] = 0x5a;
        var files = Field<WindowsMusicFileStore>(audio, "_musicFiles"); byte[] firstRaw = primary.Raw().ToArray(), secondRaw = secondary.Raw().ToArray();
        uint random = startup.Random.State(); UIntBox result = state.ResultStateBox;
        state.ResultState = 0x81234567; state.SongEnd.Value = BitConverter.UInt64BitsToDouble(0x7ff8000012345678); state.PaletteDirty = 71; startup.Cloud.WorldRedraw = 73;
        state.CurrentBattle = () => { ownerReads++; throw new IOException("바인딩 중 battle 소비 금지"); };
        state.CurrentPlayersReady = () => { ownerReads++; throw new IOException("바인딩 중 ready 소비 금지"); };
        state.CurrentLocalPlayer = () => { ownerReads++; throw new IOException("바인딩 중 local 소비 금지"); };
        state.BindPaletteDirty(startup.Cloud.WorldRedrawBox); state.BindPaletteDirty(startup.Cloud.WorldRedrawBox);
        _ = new SceneMusic(state, startup.Random, new()
        {
            WallSeconds = Clock,
            CurrentName = () => { ownerReads++; return audio.CurrentMusicName; },
            Duration = () => { ownerReads++; return audio.CurrentMusicDuration; },
            Select = name => { commands++; audio.SelectMusic(name); },
            WeatherSound = name => { commands++; audio.PlayGlobal(name); },
            Sacrificing = _ => throw new IOException("바인딩 중 희생 판정 금지"), WaitingRoom = () => throw new IOException("바인딩 중 대기실 판정 금지"),
            Refresh = () => throw new IOException("바인딩 중 화면 갱신 금지"), AscendancyPalette = () => throw new IOException("바인딩 중 팔레트 옵션 조회 금지"),
            LoadPalette = _ => throw new IOException("바인딩 중 팔레트 읽기 금지"), ThunderFlash = () => throw new IOException("바인딩 중 번개 부착 금지"),
        });
        Assert.Equal(0, commands); Assert.Equal(0, ownerReads); Assert.Equal(0, clockReads); Assert.Equal(random, startup.Random.State());
        Assert.Same(state, audio.Scene); Assert.Same(result, state.ResultStateBox); Assert.Equal(0x81234567u, state.ResultState); Assert.Equal(73u, state.PaletteDirty);
        Assert.Equal(0x7ff8000012345678UL, BitConverter.DoubleToUInt64Bits(state.SongEnd.Value));
        Assert.Equal(firstRaw, primary.Raw().ToArray()); Assert.Equal(secondRaw, secondary.Raw().ToArray());
        Assert.Empty(Field<Dictionary<uint, nint>>(files, "_files")); Assert.Empty(audio.CurrentMusicName); Assert.False(audio.DeviceReady);
        Assert.False(Field<SoundState>(audio, "_state").MusicInitialized);
        var handles = Field<MusicRuntimeState>(audio, "_musicRuntimeState"); Assert.Equal((nint)0, handles.Thread); Assert.Equal((nint)0, handles.Shutdown);
    }
}
