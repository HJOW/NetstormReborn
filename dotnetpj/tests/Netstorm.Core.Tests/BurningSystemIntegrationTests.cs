using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 실제 연소14의 등록/Form/Kernel·부분생성·현재그리기 수명을 같은 월드에서 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>기존 실제 공통 표시 장면에 연소 파생 시스템을 연결한다.</summary>
public sealed partial class RenderProcessSystemTests
{
    /// <summary>실제 부모HP/VT와 등록배율을 준비하고 합성 캡처 입력만 공급하는 연소장면이다.</summary>
    private sealed class BurningScene
    {
        /// <summary>같은 실제Host/Kernel/공통등록 owner다.</summary>
        public Scene World { get; } = new();
        /// <summary>실제 공유 시각/카메라/바람 상태다.</summary>
        public BurningFrameState Clock { get; } = new() { Now = 1, PixelScaleX = 16, PixelScaleY = 11, DeltaMultiplier = .3f, Viewport = new(0, 0, 128, 128) };
        /// <summary>독립 프레임 번호의 같은 주소다.</summary>
        public uint Frame;
        /// <summary>현재프레임 읽기 횟수다.</summary>
        public int FrameReads;
        /// <summary>실제캡처 요청 횟수다.</summary>
        public int Captures;
        /// <summary>현재clip 조회 횟수다.</summary>
        public int Clips;
        /// <summary>공유 팔레트의 현재 토큰이다.</summary>
        public BurningVisualState VisualState { get; } = new() { Palette = 1 };
        /// <summary>동일한 현재 연소점 상태다.</summary>
        public BurningSourcesState Sources { get; } = new();
        /// <summary>인접DWORD까지 명시한 실제 색 표다.</summary>
        public BurningDrawPalette Palette { get; } = new() { Count = 8, Readable = new uint[] { 31, 32, 33, 34, 35, 36, 37, 38, 39 } };
        /// <summary>실제 dirty요청과 반납/소리의 관측순서다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>같은 실제8비트 픽셀 소비자의 요청이다.</summary>
        public List<(int X, int Y, byte Color)> Pixels { get; } = [];
        /// <summary>시스템 구성 때 복사하는 함수입력이다.</summary>
        public BurningSystemHooks Hooks { get; }
        /// <summary>현재 원문 부모와 캡처/메모리 외부 입력을 연결한다.</summary>
        public BurningScene()
        {
            World.Types[71].Flags1 = 16; World.Types[71].Flags2 = 0x400; World.Types[71].MaxHitPoints = 20;
            BinaryPrimitives.WriteUInt32LittleEndian(World.Pool.RawSlot(World.Parent), 0x50b888); BinaryPrimitives.WriteInt32LittleEndian(World.Pool.RawSlot(World.Parent)[26..], 10);
            Hooks = new()
            {
                Visual = new() { Reserve = _ => true },
                Frame = new() { Capture = sid => { Assert.Equal(World.Parent, sid); Captures++; return new() { Width = 12, Height = 10, Pixels = new byte[120] }; }, StopSound = sound => Events.Add($"stop:{sound}") },
                Invalidate = _ => Events.Add("dirty"), Clip = () => { Clips++; return new(0, 0, 128, 128); },
                Pixel = (x, y, color) => Pixels.Add((x, y, color)), Release = kind => Events.Add("release:" + kind),
            };
        }
        /// <summary>단일 실제Host/Kernel과 현재프레임주소를 시스템에 빌린다.</summary>
        public BurningVisualSystem System() => new(World.Rendering, new(), World.Types, VisualState, Sources, Clock, Palette, () => { FrameReads++; return Frame; }, Hooks);
    }
    /// <summary>생성→실제Kernel단계/입자/5픽셀→Host삭제→소리/버퍼/등록까지 한수명이다.</summary>
    [Fact]
    public void ActualBurningFormRunsAllStagesDrawsAndReleasesInOrder()
    {
        var s = new BurningScene(); using BurningVisualSystem system = s.System(); BurningVisualProcess process = system.Add(s.World.Parent, 1); int form = process.Form();
        Assert.Equal(1, system.Size); Assert.Equal(1, s.World.Kernel.Size()); Assert.Same(process, s.World.Host.FromForm(form));
        s.World.Kernel.RunFrame(); Assert.Equal(1u, process.Fields.Stage); Assert.Equal(1, s.Captures);
        s.Clock.Now = 1.02; s.World.Kernel.RunFrame(); Assert.Equal(2u, process.Fields.Stage);
        s.Clock.Now = 1.04; s.World.Kernel.RunFrame(); s.Clock.Now = 1.30; s.World.Kernel.RunFrame();
        s.World.Rendering.Draw(); Assert.NotEmpty(s.Pixels); Assert.Equal(0, s.Pixels.Count % 5); Assert.True(process.Fields.Total > 0);
        process.Fields.Sound = 55; s.Events.Clear(); s.World.Host.Kill(process, 0x10);
        Assert.Equal(new[] { "dirty", "release:Image", "release:Rows", "stop:55", "release:Particles", "release:Object" }, s.Events);
        Assert.Equal(0, system.Size); Assert.Equal(0, s.World.Kernel.Size()); Assert.Equal(0, s.World.Rendering.Size); Assert.Null(s.World.Host.FromForm(form));
        Assert.Equal(-12345, process.Fields.Registration.Parent); Assert.Empty(process.Fields.Particles); Assert.Equal(1u, s.VisualState.Palette);
    }
    /// <summary>실제부착 뒤 확보실패는 미완성dirty를 건너뛰고 Form/토큰/부분버퍼만 반납한다.</summary>
    [Fact]
    public void FailedParticleAllocationAfterActualAttachReturnsFormAndAllowsRetry()
    {
        var s = new BurningScene(); bool allowed = false; s.Hooks.Visual.Reserve = _ => allowed; using BurningVisualSystem system = s.System();
        Assert.Throws<OutOfMemoryException>(() => system.Add(s.World.Parent, 1));
        Assert.Equal(new[] { "stop:0", "release:Object" }, s.Events); Assert.Equal(0, system.Size); Assert.Equal(0, s.World.Kernel.Size()); Assert.Equal(0, s.World.Rendering.Size); Assert.Equal(0u, s.World.Rendering.Slots[0]);
        allowed = true; BurningVisualProcess process = system.Add(s.World.Parent, 1); Assert.Equal(1, system.Size); Assert.Equal(0, process.Fields.Registration.Slot); Assert.Equal(2u, process.Fields.Registration.Token);
    }
    /// <summary>생성 중 Kernel/Draw를 재진입해도 미완성 파생은 공개하지 않고 중첩Add는 거부한다.</summary>
    [Fact]
    public void PendingConstructionRemainsHiddenDuringActualKernelAndDrawingReentry()
    {
        var s = new BurningScene(); BurningVisualSystem? system = null;
        s.Hooks.Visual.Reserve = _ =>
        {
            Assert.Equal(1, s.World.Kernel.Size()); Assert.Null(s.World.Rendering.Lookup(s.World.Rendering.Slots[0])); s.World.Kernel.RunFrame(); s.World.Rendering.Draw();
            Assert.Equal(0, s.Captures); Assert.Equal(0, s.Clips); Assert.Equal(0, s.FrameReads); Assert.Throws<ArgumentException>(() => system!.Add(s.World.Parent, 0)); return true;
        };
        using (system = s.System()) { system.Add(s.World.Parent, 1); s.World.Kernel.RunFrame(); Assert.Equal(1, s.Captures); }
        Assert.Equal(0, s.World.Kernel.Size()); Assert.Equal(0, s.World.Rendering.Size);
    }
    /// <summary>실제slot7은 현재프레임gate가 선택한 뒤에만 배율/프레임을 소비하며 다른효과는 반납하지 않는다.</summary>
    [Fact]
    public void CurrentSlotFrameGatePrecedesScaleCheckAndKeepsForeignRegistrations()
    {
        var s = new BurningScene(); var dummies = new List<RenderProcessFields>();
        // 같은 공통목록 앞7칸을 다른 효과의 실제 등록으로 차지한다.
        for (int i = 0; i < 7; i++)
        {
            var fields = new RenderProcessFields(); uint token = s.World.Rendering.Track(fields, (_, _, _, _) => { }, () => { });
            s.World.Rendering.Registry.Construct(fields, token, 0, 17); s.World.Rendering.Ready(token); dummies.Add(fields);
        }
        using (BurningVisualSystem system = s.System())
        {
            BurningVisualProcess process = system.Add(s.World.Parent, 1); Assert.Equal(7, process.Fields.Registration.Slot); s.Clock.PixelScaleX = 17;
            s.World.Kernel.RunFrame(); Assert.Equal(0, s.Captures); s.Frame = 3; Assert.Throws<InvalidOperationException>(() => s.World.Kernel.RunFrame());
            s.Clock.PixelScaleX = 16; s.World.Kernel.RunFrame(); Assert.Equal(1, s.Captures); Assert.Equal(1u, process.Fields.Stage);
            s.Frame = 4; s.World.Kernel.RunFrame(); Assert.Equal(1u, process.Fields.Stage); s.Frame = 7; s.World.Kernel.RunFrame(); Assert.Equal(2u, process.Fields.Stage);
        }
        Assert.Equal(7, s.World.Rendering.Size); Assert.Equal(0, s.World.Kernel.Size());
        // 각 외부효과는 자기등록만 별도로 반납한다.
        foreach (RenderProcessFields fields in dummies) { s.World.Rendering.Registry.Destroy(fields); s.World.Rendering.Forget(fields.Token); }
    }
    /// <summary>Draw는clip장치 호출 전 카메라값사본을 만들고 팔레트는 현재같은owner를 읽는다.</summary>
    [Fact]
    public void DrawingSnapshotsCameraBeforeClipButReadsCurrentPalette()
    {
        var s = new BurningScene(); s.Clock.CameraX = .25f; s.Clock.CameraY = .5f;
        s.Hooks.Clip = () => { s.Clock.CameraX = s.Clock.CameraY = 1000; s.Palette.Readable = new uint[] { 101, 102, 103, 104, 105, 106, 107, 108, 109 }; return new(0, 0, 128, 128); };
        using BurningVisualSystem system = s.System(); BurningVisualProcess process = system.Add(s.World.Parent, 1); process.Fields.Total = 1; process.Fields.Particles[0] = new(10.75f, 12.5f, 0, 0, 4, false);
        s.World.Rendering.Draw(); Assert.Equal(5, s.Pixels.Count); Assert.Equal((9, 11, (byte)104), s.Pixels[0]); s.World.Rendering.Draw(); Assert.Equal(5, s.Pixels.Count);
    }
    /// <summary>표시 밖에서 처음nonzero소리만 정지하고 미연결장치는 선택된그위치에서 실패한다.</summary>
    [Fact]
    public void OffscreenSoundStopIsSelectedOnceAndMissingDeviceKeepsPartialState()
    {
        var s = new BurningScene(); var registry = new RawRenderRegistry(s.World.Pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (_, _, _, _) => { } });
        var visual = new RawBurningVisual(s.World.Pool, registry, s.VisualState, new() { Reserve = _ => true }); var random = new GameRandom(); var sources = new RawBurningSources(s.World.Pool, s.World.Types, visual, random, s.Sources);
        var sink = new EmptyBurningSink(); var fields = new BurningVisualFields { Capacity = 1, Particles = new BurningParticle[1], Victim = s.World.Parent, Stage = 2, Sound = 55 };
        fields.Registration.Left = fields.Registration.Top = 200; fields.Registration.Right = fields.Registration.Bottom = 210; int stopped = 0;
        var hooks = new BurningFrameHooks { Capture = _ => new(), StopSound = sound => { Assert.Equal(55u, sound); stopped++; } }; var frame = new RawBurningFrame(s.World.Pool, s.World.Types, visual, sources, random, s.Clock, sink, hooks);
        frame.Step(fields, new()); frame.Step(fields, new()); Assert.Equal(1, stopped); Assert.True(fields.Off);
        hooks.StopSound = null; var missing = new RawBurningFrame(s.World.Pool, s.World.Types, visual, sources, random, s.Clock, sink, hooks); fields.Off = false;
        Assert.Throws<InvalidOperationException>(() => missing.Step(fields, new())); Assert.False(fields.Off); fields.Sound = 0; missing.Step(fields, new()); Assert.True(fields.Off);
    }
    /// <summary>표시 밖 입력에서 실제dirty를 요청하면 실패시키는 관측sink다.</summary>
    private sealed class EmptyBurningSink : SquidDisplaySink
    {
        /// <summary>억제를 선행조회하지 않는 원문 경로다.</summary>
        public override bool Suppressed() => false;
        /// <summary>표시 밖 프레임은dirty를 제출하지 않아야 한다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => Assert.Fail("표시 밖 프레임 dirty 미선택");
    }
}
