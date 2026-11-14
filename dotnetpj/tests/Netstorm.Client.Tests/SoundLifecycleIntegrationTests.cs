using System.Buffers.Binary;
using Netstorm.Client.Audio;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 공간·표시·후처리·프로세스·이름 표·재생 계층을 한 수명으로 합성한다.
namespace Netstorm.Client.Tests;

/// <summary>장치만 원본 관측 상태표로 두고 실제 보호막/소리 수명과 현재 카메라를 검증한다.</summary>
public sealed partial class SoundPlayerOracleTests
{
    /// <summary>같은 실제 SHP 표시 경계가 무효화한 영역 수를 관측한다.</summary>
    private sealed class DisplaySink : SquidDisplaySink
    {
        /// <summary>실제 영역 무효화 수다.</summary>
        public int Changes;
        /// <summary>표시를 억제하지 않는다.</summary>
        public override bool Suppressed() => false;
        /// <summary>실제 표시 계산기가 전달한 영역을 센다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => Changes++;
    }
    /// <summary>같은DWORD 필드 폭으로 실제 raw 입력만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);

    /// <summary>실제 보호막 Pop/Regular·SHP/프레임·소리/Kernel이 생성·위치갱신·화면밖정지·최종삭제까지 이어진다.</summary>
    [Fact]
    public void ActualShieldSpaceFramesKernelAndSoundLifetimeCompose()
    {
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash(); var spots = new byte[65536];
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        // 실제 내장 프로세스/보호막 생성자와 읽는 TYPE 필드만 원본 입력으로 공급한다.
        foreach (int type in new[] { 2, 167 }) types[type].ConstructorAddress = OriginalConstructors.Address(type);
        types[167].MaxHitPoints = 100; types[167].Cost = 37; types[167].FootX = types[167].FootY = types[158].FootX = types[158].FootY = 1; types[158].Flags2 = 0x210000;
        int priest = pool.Allocate(SidAllocation.Client); Span<byte> raw = pool.AllocatedBytes(priest); raw.Clear(); Put(raw, 0, 0x0050f210); raw[10] = 158; raw[34] = 1; raw[33] = 1;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f)); hash.Bucket(1, 20.75f, 21.9f) = (ushort)priest;
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); frames[167] = new RiftTypeFrames([new RiftFrameCode(65, 80, 1, 0), new RiftFrameCode(65, 80, 2, 0)]);
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray(); shapes[167].Loaded = true; shapes[167].FrameCount = 2;
        shapes[167].Frames.AddRange([new SquidDisplayFrame { Width = 12, Height = 20, HotspotX = 3, HotspotY = 18, CellWidth = 1, CellHeight = 1 }, new SquidDisplayFrame { Width = 16, Height = 21, HotspotX = 4, HotspotY = 19, CellWidth = 1, CellHeight = 1 }]);
        shapes[158].Loaded = true; shapes[158].FrameCount = 1; shapes[158].Frames.Add(new SquidDisplayFrame { Width = 12, Height = 20, HotspotX = 3, HotspotY = 18, CellWidth = 1, CellHeight = 1 });
        var sink = new DisplaySink(); var display = new SquidDisplay(pool.Edition, types, shapes, sink, new SquidDisplayView { Viewport = new DisplayRect(0, 0, 640, 480) });
        var unpop = new RawUnpop(pool, hash, spots, display); var book = new RawPostPopState { LocalOwner = 1 }; var common = new RawPostPop(pool, types, book); var pop = new RawPop(pool, hash, spots, display, common);
        pop.BindDiagnostics(new SquidPopDiagnostics { Types = types });
        var destroy = new RawSquidDestroy(pool, unpop, types); var lifecycle = new RawSquidDestroyLifecycle(pool, types, book, new SquidDeletionState { UnitLostSuppressed1 = true }, destroy,
            new SquidDeletionHooks { Emit = _ => { }, Forward = _ => Assert.Fail("미선택 공통 삭제 전달") });
        var kernel = new Kernel(); var clock = new SquidProcessState { Now = 12.5 }; RawForcefieldRegular? regular = null;
        var host = new SquidProcessHost(pool, types, kernel, destroy, clock, (sid, ev, count, payload) => regular!.Handle(sid, ev, count, payload), lifecycle.Hooks());
        var factory = new RawSquidFactory(pool, types); host.BindFormFactory(factory);
        var prefix = new RawForcefieldPostPop(pool, ForcefieldLifecycle.MakeForcefieldPostPopProcessHooks(pool, host, () => true)); common.SetForcefieldPostPop(prefix);
        var setter = new SquidFrame(pool, types, SquidFrameBinding.MakeSquidFrameHooks(pool, types, display, unpop, pop)); var advance = new RawFrameAdvance(pool, frames, setter);
        regular = new RawForcefieldRegular(pool, hash, types, new ForcefieldRegularState(), ForcefieldLifecycle.MakeForcefieldFrameHooks(pool, advance, new ForcefieldRegularHooks { Destroy = (sid, flags) => destroy.Destroy(sid, flags, host.Hooks()) }));
        var device = new Device { LoadAttenuation = 300 }; var sounds = new SoundList(pool.Edition); sounds.Initialize();
        var state = new SoundState { Initialized = true, Device = true, Enabled = true, MasterVolume = -200 }; var player = new SoundPlayer(sounds, state, device.Hooks());
        var processState = new SoundProcessState { Frame = 100 }; var sound = new SoundProcessSystem(host, processState, SoundProcessSystem.MakeSoundProcessHooks(player, () => 3));
        var lookup = new RawPriestForcefield(pool, hash, types, new PriestForcefieldState()); int shield = 0; string? told = null;
        PriestShieldHooks hooks = SoundProcessSystem.MakePriestShieldNoticeHooks(player,
            SoundProcessSystem.MakePriestShieldSoundHooks(pool, sounds, sound, RawPriestShield.MakePriestShieldCreationHooks(pool, factory, new SquidOwner(pool, types, book, new SquidOwnerMode()), new PriestShieldHooks
            {
                Pop = (sid, x, y, flags) => { shield = sid; Assert.Equal(RawPopResult.Registered, pop.PopCurrent(sid, x, y, flags)); },
                ReserveSound = () => true, Translate = key => key, Tell = text => told = text,
            })));
        var create = new RawPriestShield(pool, lookup, new PriestShieldState { LocalPlayer = 1 }, hooks); uint before = pool.FreeCount; create.Ensure(priest);
        uint shieldSound = sounds.Lookup("priestForceField.wav"), noticeSound = sounds.Lookup("ourPriestImmobile.wav");
        Assert.Equal(3, kernel.Size()); Assert.Equal(SoundConstants.ListBase + 44, shieldSound); Assert.Equal(SoundConstants.ListBase + 93, noticeSound); Assert.Equal("PriestImmobile", told);
        Assert.Equal("F:ourPriestImmobile.wav;Q:#0;V:#0:-500;N:#0:0;P:#0:0:0:0", device.Take()); Assert.Equal(1u, sounds.Field(noticeSound, SoundField.Priority)); Assert.Equal(1, state.Playing);
        SoundPoint point = SoundGeometry.ToScreen(state.View, 20.75f, 21.9f); Assert.Equal(new SoundPoint(332, 241), point);
        int gain = SoundGeometry.Volume(state.View, point) - 500, pan = SoundGeometry.Pan(state.View, point, false);
        kernel.RunFrame(); Assert.Equal($"F:priestForceField.wav;Q:#1;V:#1:-500;N:#1:0;P:#1:0:0:1;Q:#1;V:#1:{gain};N:#1:{pan}", device.Take());
        Assert.Equal(2, state.Playing); Assert.Equal(100u, processState.TypeFrames[167]); Assert.True(sink.Changes > 0);
        processState.Frame++; kernel.RunFrame(); Assert.Equal($"Q:#1;Q:#1;V:#1:{gain};N:#1:{pan}", device.Take()); Assert.Equal(2, state.Playing);
        state.View = state.View with { CameraX = 5000 }; kernel.RunFrame(); Assert.Equal("Q:#1;Q:#1;X:#1;C:#1:0", device.Take()); Assert.Equal(1, state.Playing);
        state.View = state.View with { CameraX = 0 }; processState.Frame++; kernel.RunFrame(); Assert.Equal($"Q:#1;V:#1:-500;N:#1:0;P:#1:0:0:1;Q:#1;V:#1:{gain};N:#1:{pan}", device.Take()); Assert.Equal(2, state.Playing);
        double due = host.FindEvent(shield, 2)!.Time(); unpop.Unpop(priest, types[158]); clock.Now = due; kernel.RunFrame(); string deleted = device.Take();
        Assert.Equal(0, kernel.Size()); Assert.Equal(before, pool.FreeCount); Assert.EndsWith("Q:#1;Q:#1;X:#1;C:#1:0;Q:#1", deleted); Assert.Equal(1, state.Playing); Assert.Equal(0u, device.Status[1]);
        Action<int, float, float> fall = SoundProcessSystem.MakePriestFallSound(player); fall(priest, 20, 21);
        Assert.Equal($"F:priestFall.wav;Q:#2;V:#2:{SoundGeometry.Volume(state.View, SoundGeometry.ToScreen(state.View, 20, 21)) - 500};N:#2:0;P:#2:0:0:0", device.Take());
        fall(priest, 200, 21); Assert.Equal("-", device.Take()); Assert.Equal(2, state.Playing);
    }

    /// <summary>현재 지도는 투영·가시성·좌우·음량 위치마다 따로 읽고 앞서 투영한 화면 점만 보존한다.</summary>
    [Fact]
    public void CurrentViewIsRequeriedAtFourOriginalConsumptionPoints()
    {
        var device = new Device(); var list = new SoundList(OriginalEdition.Patch1078); list.Initialize(); uint entry = list.Lookup("moving.wav");
        list.SetField(entry, SoundField.Buffer, 0x16001000); device.Status.Add(0); var state = new SoundState { Initialized = true, Device = true, Enabled = true };
        SoundView[] views = [new(100, 20, 1000, 1000, 1100, 1100), new(900, 900, 280, 180, 320, 220), new(900, 900, 0, 0, 400, 400), new(900, 900, 0, 0, 800, 400)]; int reads = 0;
        state.CurrentView = () => views[reads++]; var player = new SoundPlayer(list, state, device.Hooks());
        Assert.Equal(entry, player.PlayOnceAt(25, 20, entry, 0, 0)); Assert.Equal(4, reads); Assert.Equal("Q:#0;V:#0:-62;N:#0:2500;P:#0:0:0:0", device.Take());
        state.CurrentView = () => { reads++; return new SoundView(0, 0, 1000, 1000, 1100, 1100); }; Assert.Equal(0u, player.PlayOnceAt(25, 20, entry, 0, 0)); Assert.Equal(6, reads); Assert.Equal("-", device.Take());
        state.Enabled = false; Assert.Equal(0u, player.PlayLoopAt(25, 20, entry, 0)); Assert.Equal(6, reads);
    }
    /// <summary>장치 조회/재생이 지도를 바꿔도 처음 투영한 화면 점은 보존하고 후반 현재 좌우/음량을 읽는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void LoopRereadsCurrentGeometryAfterDeviceEffects(bool onPlay)
    {
        var device = new Device(); device.Status.Add(onPlay ? 0u : 1u); var list = new SoundList(OriginalEdition.Patch1078); list.Initialize(); uint entry = list.Lookup("moving.wav"); list.SetField(entry, SoundField.Buffer, 0x16001000);
        var state = new SoundState { Initialized = true, Device = true, Enabled = true }; SoundView current = new(0, 0, 0, 0, 800, 600); int reads = 0;
        state.CurrentView = () => { reads++; return current; }; Action change = () => current = new SoundView(1000, 2000, 0, 0, 400, 600);
        if (onPlay) device.AfterPlay = change; else device.AfterStatus = change;
        var player = new SoundPlayer(list, state, device.Hooks()); Assert.Equal(entry, player.PlayLoopAt(25, 300f / 11f, onPlay ? 0 : entry, onPlay ? entry : 0));
        Assert.Equal(4, reads); Assert.Equal(onPlay ? "Q:#0;V:#0:0;N:#0:0;P:#0:0:0:1;Q:#0;V:#0:-1000;N:#0:10000" : "Q:#0;Q:#0;V:#0:-1000;N:#0:10000", device.Take());
    }

}
