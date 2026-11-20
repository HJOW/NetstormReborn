using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 진단과 커서의 실제 장기 소유자 및 Session/Surface 공유를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 초기값·주소 정체성·옵션·실제 Runtime 대여 경계 검사다.</summary>
public sealed class ClientAiDebugGlobalsTests
{
    /// <summary>같은 게임 전역을 사용하는 실제 세션이다.</summary>
    private static HostSession Session(ClientGameplayGlobals game, RawSurfaceStorage? storage = null) => new(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: storage?.Pool);
    /// <summary>첫 Session 없이 실제 현재 물리와 시계/커널을 만든다.</summary>
    private static ClientSurfaceGlobals Surface(ClientGameplayGlobals game)
    {
        var clock = new ClientClockGlobals(new ClientLocalStateGlobals(game, new())); return new(game, clock, new Kernel(), () => throw new IOException("관측없는시각조회금지"));
    }
    /// <summary>PE 일곱 주소와 쓰기 참조·BSS0/null이 원문대로 한 번만 구성된다.</summary>
    [Fact]
    public void NativeAddressesOwnMutableWordsAndBssDefaults()
    {
        var game = new ClientGameplayGlobals(); ClientAiDebugGlobals debug = game.AiDebug;
        Assert.Equal(0, debug.Checking.Value); Assert.Null(debug.AssertionDetail.Value); Assert.Equal(1u, game.Cursor.LockDepth); Assert.Equal(0u, game.CursorGlobals.Lifecycle.ReadChecking()); Assert.Equal(0u, game.PlayerRecords.Base);
        uint[] addresses = [0x52f3cc, 0x52f3d0, 0x52f3d4, 0x52f3d8, 0x52f3dc, 0x52f3e0, 0x52f3e4], expected = [99, 99, 29, 99, 99, 99, 29];
        // 각 원문 주소의 직접 참조 쓰기가 같은 현재 읽기에 반영된다.
        for (int index = 0; index < addresses.Length; index++) { Assert.Equal(expected[index], debug.Read(addresses[index])); ref uint word = ref debug.Word(addresses[index]); word = unchecked(0x80000000u + (uint)index); Assert.Equal(word, debug.Read(addresses[index])); }
        Assert.Throws<ArgumentOutOfRangeException>(() => debug.Read(0)); Assert.Throws<ArgumentOutOfRangeException>(() => debug.Read(0x52f3cd));
    }
    /// <summary>커서·Surface·두 Session은 현재 signed 전체비트를 즉시 읽고 별도 세계로 복제하지 않는다.</summary>
    [Fact]
    public void EveryCurrentConsumerReadsSameLongLivedChecking()
    {
        var game = new ClientGameplayGlobals(); ClientSurfaceGlobals surface = Surface(game); HostSession first = Session(game), second = Session(game);
        RawCheckingFlag beforeWorld = surface.Storage.Checking(); RawCheckingFlag cursor = game.CursorGlobals.Lifecycle.Checking!;
        game.AiDebug.Checking.Value = int.MinValue; Assert.Equal(0x80000000u, beforeWorld.Read()); Assert.Equal(0x80000000u, cursor.Read()); Assert.Equal(0x80000000u, first.AiChecking.Read()); Assert.Equal(0x80000000u, second.AiChecking.Read());
        game.AiDebug.Checking.Value = -1; Assert.Equal(uint.MaxValue, first.BuildTerrainGenerationInputs().Growth.Checking.Read());
        game.AiDebug.Checking.Value = 0; Assert.Equal(0u, beforeWorld.Read()); Assert.Equal(0u, cursor.Read()); Assert.Equal(0u, second.AiChecking.Read());
    }
    /// <summary>prepared 공개/반납은 커서·현재 오류 상세·AI 로그를 재초기화하지 않는다.</summary>
    [Fact]
    public void PreparedRuntimePreservesActualCursorAndDiagnostics()
    {
        var game = new ClientGameplayGlobals(); ClientSurfaceGlobals surface = Surface(game); var session = Session(game, surface.Storage);
        CursorDropState drop = game.Cursor; var life = game.CursorGlobals.Lifecycle; uint[] previews = life.Previews;
        game.AiDebug.Checking.Value = -73; game.AiDebug.AssertionDetail.Value = "current"; game.AiDebug.Word(0x52f3d4) = 0xa5;
        drop.LockDepth = 7; drop.Preview = 0xffff1234; life.Previews[29] = 0xaabbccdd; life.PreviewCount = -3;
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage)) { Assert.Same(session, game.RequireSession()); Assert.Equal(7u, drop.LockDepth); }
        Assert.Same(drop, game.Cursor); Assert.Same(life, game.CursorGlobals.Lifecycle); Assert.Same(previews, life.Previews); Assert.Equal(0xffff1234u, drop.Preview); Assert.Equal(0xaabbccddu, previews[29]); Assert.Equal(-3, life.PreviewCount);
        Assert.Equal(unchecked((uint)-73), surface.Storage.Checking().Read()); Assert.Equal("current", game.AiDebug.AssertionDetail.Value); Assert.Equal(0xa5u, game.AiDebug.Read(0x52f3d4));
    }
    /// <summary>일반 fresh만 같은 커서 배열을 명시 초기화하며 기록/회전/checking과 실제 픽셀 참조를 보존한다.</summary>
    [Fact]
    public void GeneralResetPreservesOwnersOptionsAndLiveChecking()
    {
        var game = new ClientGameplayGlobals(); var drop = game.Cursor; var life = game.CursorGlobals.Lifecycle; var placement = game.CursorGlobals.Placement; uint[] previews = life.Previews; int[] bounds = placement.Bounds, pixels = [17, 23];
        drop.LockDepth = 0; drop.Preview = 77; drop.Recording = 5; drop.Recorder = 0x80000000; life.Previews[29] = 77; life.PreviewCount = 30; bounds[3] = 91; placement.SharedScreenOrigin = pixels; game.CursorGlobals.RotationMode.Value = 3; game.CursorGlobals.SnapToGrid.Value = 1; game.AiDebug.Checking.Value = int.MinValue;
        game.ResetCursorForGeneralWorld(); Assert.Same(drop, game.Cursor); Assert.Same(previews, life.Previews); Assert.Same(bounds, placement.Bounds); Assert.Equal(1u, drop.LockDepth); Assert.Equal(0u, drop.Preview); Assert.Equal(5u, drop.Recording); Assert.Equal(0x80000000u, drop.Recorder); Assert.Equal(0u, previews[29]); Assert.Equal(0, bounds[3]); Assert.Equal(3u, game.CursorGlobals.RotationMode.Value); Assert.Equal(0u, game.CursorGlobals.SnapToGrid.Value);
        Assert.Equal(0x80000000u, life.ReadChecking()); game.AiDebug.Checking.Value = -1; Assert.Equal(uint.MaxValue, life.ReadChecking()); pixels[0] = 81; Assert.Equal(81, placement.CurrentOriginX());
    }
    /// <summary>실제 ReadInt는 누락 키의 현재값과 전체 signed DWORD를 보존하며 다른 진단을 지우지 않는다.</summary>
    [Fact]
    public void OptionReadPreservesMissingValuesAndAllBits()
    {
        var debug = new ClientAiDebugGlobals(); var configuration = new ConfigInterface(_ => null); debug.Checking.Value = -7; debug.AssertionDetail.Value = "kept"; debug.Word(0x52f3cc) = 73;
        debug.ReadOptions(configuration); Assert.Equal(-7, debug.Checking.Value); configuration.Set("showSoftAsserts", "4294967295"); debug.ReadOptions(configuration); Assert.Equal(-1, debug.Checking.Value);
        configuration.SetInt("showSoftAsserts", int.MinValue); debug.ReadOptions(configuration); Assert.Equal(int.MinValue, debug.Checking.Value); Assert.Equal("kept", debug.AssertionDetail.Value); Assert.Equal(73u, debug.Read(0x52f3cc));
    }
    /// <summary>실제 표면 진단은 같은 nullable 상세 owner와 명시 중앙 장치를 사용하며 누락은 실패한다.</summary>
    [Fact]
    public void SurfaceDiagnosticsUseActualDetailAndSelectedDevice()
    {
        var game = new ClientGameplayGlobals(); ClientSurfaceGlobals surface = Surface(game); var seen = new List<string>();
        Assert.Throws<InvalidOperationException>(() => surface.Storage.RecordAssertionDetail("before"));
        surface.BindDisplayDiagnostics((expression, file, line) => { seen.Add($"{expression}:{file}:{line}:{game.AiDebug.AssertionDetail.Value}"); game.AiDebug.Checking.Value = int.MinValue; });
        surface.Storage.RecordAssertionDetail("actual"); Assert.Equal("actual", game.AiDebug.AssertionDetail.Value); surface.Storage.AssertDisplay("false", ".\\Squid.cpp", 73); Assert.Equal(["false:.\\Squid.cpp:73:actual"], seen);
        Assert.Equal(0x80000000u, game.CursorGlobals.Lifecycle.ReadChecking()); surface.Storage.RecordAssertionDetail("next"); Assert.Equal("next", game.AiDebug.AssertionDetail.Value);
    }
}
