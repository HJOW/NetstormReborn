using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 실제 제품 장치/풀/공통 삭제/표면 수명으로 파편과35h의 부분 효과를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>보호SHP/실제Host/Kernel/감시/삭제 본체를 조립하는 창 없는 제품 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>검사에 명시한 표면/더미 클래스의 실제 공통/파생 삭제를 같은 Host 깊이에 연결한다.</summary>
    private static RawSquidFormDispatch BindShrapnelDeletion(BurningContext scene)
    {
        HostSession session = scene.Session; RawSquidDestroy body = session.PlayerProcesses.Destroy();
        Assert.False(session.TerrainBookkeeping.GraphsEnabled); // 이 독립 장면은 아직 Graph Pop을 켜지 않은 원문 입력이다.
        RawSquidFormDispatch? dispatch = null;
        var common = new RawSquidDestroyLifecycle(session.Pool, scene.Types, session.TerrainBookkeeping, new(), body, new()
        {
            LocalOwner = () => session.MissionPlayer.BattleOwner,
            Emit = effect => { if (effect.Effect == SquidDeletionEffect.UnitLostSound) scene.Audio.PlayGlobal("unitLost.wav"); else throw new InvalidOperationException("검사 미선택 UI 삭제 효과"); },
            Forward = effect => throw new InvalidOperationException($"검사 미선택 외부 삭제 효과:{effect.Effect}"),
        }, reward: session.Reward);
        IReadOnlyList<IReadOnlyList<FrameCode>> frames = session.WorldFrames.Select(frame => (IReadOnlyList<FrameCode>)frame.Codes.Select(code => new FrameCode((char)code.Side, (char)code.Variant, code.Number, (FrameCodeFlags)code.Flags)).ToArray()).ToArray();
        var neighbors = new RawSquidNeighbors(session.Pool, session.TerrainPopHash!, session.TerrainPopSpots!, scene.Types, frames);
        var surface = new RawSurfaceLifecycle(session.Pool, neighbors, session.BridgeState.ConnectorType, common.Hooks(), new()
        {
            Destroy = (sid, flags) => session.DestroyWorldObject(sid, flags),
            ScheduleFall = (sid, x, y) => { _ = RawBridgeFallParent.ScheduleChecked(session.PlayerProcesses, session.Pool, scene.Types, sid, x, y, new(() => session.AiChecking.Read())); },
            FallWalker = sid => session.PriestWorld.FallWalker(sid),
            BridgeEffect = effect => { if (effect.Effect == BridgeLifecycleEffect.NotifyRemoval) _ = session.AddWorldShrapnel(effect.Sid, ShrapnelKind.Variant2); else if (effect.Effect == BridgeLifecycleEffect.FallSound) scene.Audio.PlayEffectAt(effect.X, effect.Y, "bridgeFall.wav", 0); else throw new InvalidOperationException("검사 다른 다리 표시 효과"); },
        });
        dispatch = new(session.Pool, body, surface.Hooks(), (sid, flags, hooks) =>
        {
            // 다른 실제 파생을 공통 Pre/Post로 통과시키지 않고 명시한 실제 클래스만 검사한다.
            uint table = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(sid));
            if (table is not (0x50eb98 or 0x506e50 or 0x506f08 or 0x506d98 or 0x501dd0)) throw new InvalidOperationException($"검사 전체 삭제의 미선택 클래스:{table:x}");
            body.Destroy(sid, flags, hooks); return true;
        });
        dispatch.Bind(session.PlayerProcesses); dispatch.Bind(session.WorldDependencies); session.BindWorldDeletion(dispatch); return dispatch;
    }
    /// <summary>실제 공장/현재 소유자/공간으로 낙하 원천을 배치한다. Graph 미설치 입력의 실제 Pop 플래그다.</summary>
    private static int PlaceShrapnelSource(BurningContext scene, uint type = 95, uint owner = 1, float x = 4, float y = 5)
    {
        HostSession session = scene.Session; TerrainStampHooks stamp = session.BuildTerrainStampHooks(scene.Types);
        int sid = stamp.Create!(type, 2); session.SetFortObjectOwner(sid, owner); stamp.Pop!(sid, x, y, 0x02000061); return sid;
    }
    /// <summary>수신 부모 입력을 만들며 호출자 원문 헤더와 DWORD 폭을 유지한다.</summary>
    private static byte[] DropPacket(int x, int y, uint flags, uint owner)
    {
        byte[] packet = new byte[20]; BattlePacketWriter.WriteDropIslandPacket(packet, unchecked((uint)x), unchecked((uint)y), flags, owner); return packet;
    }

    /// <summary>세 생성기가 실제 더미Pop/감시/캡처/픽셀/자연소진과 같은 전체 삭제를 사용한다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData(ShrapnelKind.Explosion)]
    [InlineData(ShrapnelKind.Collapse)]
    [InlineData(ShrapnelKind.Variant2)]
    public void ProductShrapnelCreatesDrawsAndExpiresThroughFullDeletion(ShrapnelKind kind)
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; BindShrapnelDeletion(scene); int source = PlaceShrapnelSource(scene);
        scene.Visuals.SetView(-3.25f, -2.5f, new(0, 0, 1024, 768)); scene.Visuals.PrepareFrame(2.25, 1); session.ProcessTime.Value = 100;
        byte[] screen = scene.Screen.Lock(); Array.Fill(screen, (byte)199); scene.Screen.Unlock();
        ShrapnelVisualProcess process = Assert.IsType<ShrapnelVisualProcess>(session.AddWorldShrapnel(source, kind));
        int form = process.Form(), dummy = process.Parent(); uint token = process.Fields.Registration.Token;
        Assert.Equal(16, session.Pool.Slot(form)[10]); Assert.Equal(110, session.Pool.Slot(dummy)[10]); Assert.True(session.WorldDependencies.Has(form, dummy));
        Assert.Equal(2.25, process.Fields.LastTime); Assert.Same(session.PlayerProcesses, session.WorldRendering.Host); Assert.Same(process, session.PlayerProcesses.FromForm(form));
        Assert.True(process.Fields.Total > 0); Assert.Contains(process.Fields.Image.Pixels, color => color != 255);
        session.WorldRendering.Draw(); Assert.Contains(screen, color => color != 199);
        session.DestroyWorldObject(source, 0x10); Assert.Equal(1, session.WorldShrapnel.Size);
        scene.Visuals.PrepareFrame(100, 2); session.Kernel.RunFrame(); Assert.Equal(1, session.WorldShrapnel.Size);
        scene.Visuals.PrepareFrame(101, 3); session.Kernel.RunFrame();
        Assert.Equal(0, session.WorldShrapnel.Size); Assert.Equal(0, session.Kernel.Size()); Assert.Null(session.WorldRendering.Lookup(token));
        Assert.Equal(3, session.Pool.Slot(form)[11] & 3); Assert.Equal(3, session.Pool.Slot(dummy)[11] & 3); Assert.Equal(0u, session.PlayerProcesses.Destroy().PreDepth); Assert.Equal(0u, session.PlayerProcesses.Destroy().PostDepth);
    }
    /// <summary>뷰 미설치 캡처 실패도 이미 만든 실제 Form/더미/감시 쌍을 반납한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductShrapnelMissingViewCleansAttachedLifetime()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; BindShrapnelDeletion(scene); int source = PlaceShrapnelSource(scene);
        Assert.Contains("viewport", Assert.Throws<InvalidOperationException>(() => session.AddWorldShrapnel(source, ShrapnelKind.Variant2)).Message);
        Assert.Equal(0, session.WorldShrapnel.Size); Assert.Equal(0, session.Kernel.Size()); Assert.Equal(0, session.WorldRendering.Size); Assert.Equal(0u, session.PlayerProcesses.Destroy().PreDepth); Assert.Equal(0u, session.PlayerProcesses.Destroy().PostDepth);
    }
    /// <summary>픽셀 장치는 현재 clip 끝을 제외하며 실제 palette byte만 현재 표면에 쓴다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductShrapnelPixelUsesCurrentClipAndPaletteByte()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768)); scene.Screen.SetClip(3, 4, 10, 12);
        byte[] pixels = scene.Screen.Lock(); Array.Fill(pixels, (byte)199); scene.Screen.Unlock();
        scene.Visuals.Shrapnel.Draw(new(3, 4, 0x123456ab)); scene.Visuals.Shrapnel.Draw(new(10, 4, 0xff)); scene.Visuals.Shrapnel.Draw(new(3, 12, 0xff));
        Assert.Equal(0xab, pixels[4 * scene.Screen.Pitch + 3]); Assert.Equal(199, pixels[4 * scene.Screen.Pitch + 10]); Assert.Equal(199, pixels[12 * scene.Screen.Pitch + 3]);
    }
    /// <summary>전체 삭제 장치 미설치와 반납 뒤 수신은 실제 호출 위치에서 명시 실패한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDropAndShrapnelRequireActualDeletionAndLiveRuntime()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; int source = PlaceShrapnelSource(scene);
        Assert.Contains("가상 삭제", Assert.Throws<InvalidOperationException>(() => session.DestroyWorldObject(source, 0)).Message); Assert.Equal(0, session.Pool.Slot(source)[11] & 3);
        session.ReleasePlayerRecordViews(); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(0, 0, 0, 0)));
        Assert.Throws<InvalidOperationException>(() => session.WorldShrapnel);
    }
    /// <summary>현재 억제DWORD는 로딩 깊이와 별개이며 임의 좌표도 전파/탐색 전에 자연 반환한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDropReadsLongSuppressionAndPreparesCurrentBuilding()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session;
        Assert.Equal(96u, session.GameAux.IslandStumpTypeBox.Value); session.GameAux.IslandDropBlockedBox.Value = 1;
        session.ProcessTime.Value = 7.25; session.MissionPlayer.BattleOwner = 8;
        byte[] packet = DropPacket(int.MinValue, int.MaxValue, 0, uint.MaxValue); packet[0] = packet[1] = 0xee;
        session.ReceiveFixedGameplayPacket(0x46e940, packet);
        Assert.Equal(7.25, session.WorldConstructionProgressState.Now); Assert.Equal(8u, session.WorldConstructionProgressState.LocalPlayer); Assert.Equal(0, session.WorldShrapnel.Size);
        session.GameAux.IslandDropBlockedBox.Value = 0; session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0x50, 0x100));
        Assert.Equal(0, session.WorldShrapnel.Size);
    }
    /// <summary>같은 Pool이라도 Host와 다른 삭제 깊이 본체를 설치하지 않는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductShrapnelRejectsDifferentDeletionBodyBeforeBinding()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session;
        var separate = new RawSquidDestroy(session.Pool, new(session.Pool, session.TerrainPopHash!, session.TerrainPopSpots!), scene.Types);
        var dispatch = new RawSquidFormDispatch(session.Pool, separate, new() { Selected = static () => 0, Emit = static _ => throw new InvalidOperationException("잘못된 본체가 호출됨") });
        Assert.Contains("깊이 불일치", Assert.Throws<ArgumentException>(() => session.BindWorldDeletion(dispatch)).Message);
        Assert.Null(session.PlayerProcesses.Runtime().Asset);
        _ = BindShrapnelDeletion(scene); Assert.NotNull(session.PlayerProcesses.Runtime().Asset);
    }
    /// <summary>원격 표 미설치 진단은 실제 후보를 삭제하기 전이며 서버/50h 억제는 현재값이다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDropBroadcastPrecedesCandidatesAndUsesCurrentAuthority()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; BindShrapnelDeletion(scene);
        int source = PlaceShrapnelSource(scene, 96); session.BridgeState.Mission = false; session.Gameplay.Authority.Server = true;
        Assert.Contains("원격 연결 표", Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0, 1))).Message);
        Assert.Equal(0, session.Pool.Slot(source)[11] & 3);
        session.Gameplay.Authority.Server = false; session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0, 1));
        Assert.Equal(1, session.Pool.Slot(source)[11] & 3); Assert.Equal(0, session.WorldShrapnel.Size);
    }
    /// <summary>정상9/1/1 섬은 실제 공간/표면 삭제를 끝내고1000h가 파편과 낙하음을 생략한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDropDeletesCompleteIslandWithoutParticlesWhenSuppressed()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; BindShrapnelDeletion(scene);
        session.BuildIslandPostPopDevices(scene.Types, session.WorldFrames);
        var sources = new List<int>();
        // 실제3곱하기3 발자국의 표면9개를 현재 공장과 Pop으로 배치한다.
        for (int y = 3; y <= 5; y++)
        {
            // 각 줄의 세 칸을 원본 좌표 폭으로 등록한다.
            for (int x = 2; x <= 4; x++) sources.Add(PlaceShrapnelSource(scene, 157, 1, x, y));
        }
        sources.Add(PlaceShrapnelSource(scene, 94));
        var finder = new RawSquidFinder(session.Pool, session.TerrainPopHash!, scene.Types);
        // 섬의 실제 Prefix가 만든 종유석을 조회한다. 검사에서 두 번째 조각을 만들지 않는다.
        for (int sid = finder.Begin(new(2, 3, 4, 5)); sid != 0; sid = finder.Next())
            if (session.Pool.Slot(sid)[10] == 95) sources.Add(sid);
        Assert.Equal(11, sources.Count); Assert.Single(sources, sid => session.Pool.Slot(sid)[10] == 95);
        session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0x1050, 0x100));
        Assert.All(sources, sid => Assert.Equal(0, session.Pool.Slot(sid)[11] & 3));
        session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0x1050, 1));
        Assert.All(sources, sid => Assert.Equal(3, session.Pool.Slot(sid)[11] & 3));
        Assert.Equal(0, session.WorldShrapnel.Size); Assert.Equal(0, session.Kernel.Size());
        Assert.Equal(0u, session.PlayerProcesses.Destroy().PreDepth); Assert.Equal(0u, session.PlayerProcesses.Destroy().PostDepth);
    }
    /// <summary>실제 파편/음향/전체 표면 삭제 뒤9/1/1진단의 첫 실패를 유지한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDropExecutesRealFallAndFullDeleteBeforeIslandCountDiagnostic()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session; BindShrapnelDeletion(scene); int source = PlaceShrapnelSource(scene);
        scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768)); scene.Visuals.PrepareFrame(1.5, 1);
        Exception failure = Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x46e940, DropPacket(4 << 16, 5 << 16, 0x50, 1)));
        Assert.Contains("noIslandTypeCount == 9", failure.Message); Assert.Contains("Islanddropper.cpp:7b", failure.Message);
        Assert.Equal(3, session.Pool.Slot(source)[11] & 3); Assert.Equal(1, session.WorldShrapnel.Size);
        ShrapnelVisualProcess process = Assert.IsType<ShrapnelVisualProcess>(Enumerable.Range(1, (int)session.Kernel.Capacity()).Select(pid => session.Kernel.Get((uint)pid)).Single(item => item is ShrapnelVisualProcess));
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3f99999a), process.Fields.Gravity); Assert.Equal(1.5, process.Fields.LastTime); Assert.True(process.Fields.Total > 0);
    }
}
