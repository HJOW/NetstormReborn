using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Form/Kernel/감시쌍/표시목록에서원본파편의소유권을검사한다.
namespace Netstorm.Core.Tests;

/// <summary>캡처/표시장치는명시입력이며수명/프레임/공통삭제는실제모듈을사용한다.</summary>
public sealed class ShrapnelVisualSystemTests
{
    /// <summary>같은실제저장소와파편시스템을소유하는검사장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>전체Form/더미가공유하는실제PATCH풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true);
        /// <summary>원본생성자주소를명시한타입입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        /// <summary>픽셀효과의실제소유자다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>실제공통삭제본체다.</summary>
        public readonly RawSquidDestroy Destroy;
        /// <summary>같은타입/풀의실제생성자다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>서로종속된Form과자산을실제분배한다.</summary>
        public readonly RawSquidFormDispatch Dispatch;
        /// <summary>공통Form/Kernel소유권의실제Host다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>실제상호감시쌍이다.</summary>
        public readonly RawDependFormHost Depends;
        /// <summary>모든효과가공유하는현재표시배율이다.</summary>
        public readonly ConstructionVisualState Visual = new();
        /// <summary>같은등록/수집목록이다.</summary>
        public readonly RenderProcessSystem Rendering;
        /// <summary>같은게임난수다.</summary>
        public readonly GameRandom Game = new(0x12345678);
        /// <summary>독립CRT스트림이다.</summary>
        public readonly CrtRandom Crt = new(1);
        /// <summary>현재Kernel시각/카메라다.</summary>
        public readonly ShrapnelFrameState Frame = new() { Now = 1 };
        /// <summary>반납의실제순서다.</summary>
        public readonly List<ShrapnelRelease> Releases = [];
        /// <summary>현재픽셀출력요청이다.</summary>
        public readonly List<ShrapnelPixel> Pixels = [];
        /// <summary>프레임/삭제의dirty영역이다.</summary>
        public readonly List<FieryTailDirtyRect> Dirty = [];
        /// <summary>캡처원천이다.</summary>
        public readonly int Source;
        /// <summary>마지막실제생성더미다.</summary>
        public int Dummy;
        /// <summary>전체투명캡처입력이다.</summary>
        public bool Transparent;
        /// <summary>wrapper확보실패입력이다.</summary>
        public bool FailObject;
        /// <summary>실제더미Pop전예외입력이다.</summary>
        public bool FailPop;
        /// <summary>부착/쌍구성뒤캡처예외입력이다.</summary>
        public bool FailCapture;
        /// <summary>입자또는작업그림확보실패의바이트수다.</summary>
        public uint FailBytes;
        /// <summary>캡처중같은장치의재진입검사다.</summary>
        public Action? Capturing;
        /// <summary>실제검사대상이며Host보다먼저정리한다.</summary>
        public readonly ShrapnelVisualSystem System;

        /// <summary>실제분배와감시통지를묶고합성원천하나를생성한다.</summary>
        public Scene()
        {
            RawUnpop unpop = new(Pool, new RawSquidHash(), new byte[65536]); Destroy = new(Pool, unpop, Types); Factory = new(Pool, Types, false, unpop);
            SquidDestroyHooks assets = new() { Selected = () => 0, Emit = effect => { if (effect.Effect == SquidDestroyEffect.PreDestroy) Destroy.CompletePreDestroy(); else if (effect.Effect == SquidDestroyEffect.PostDestroy) Destroy.CompletePostDestroy(); } };
            Dispatch = new(Pool, Destroy, assets, (sid, flags, hooks) => { Destroy.Destroy(sid, flags, hooks); return true; });
            Host = new(Pool, Types, Kernel, Destroy, new SquidProcessState(), asset: Dispatch.Hooks());
            Depends = new(Pool, Destroy, (watcher, watched, form) => { if (Pool.Slot(watcher)[10] == 110) Dispatch.Destroy(watcher, 0); else Host.NotifyDependency(watcher, watched, form); }, Dispatch.Hooks());
            Dispatch.Bind(Host); Dispatch.Bind(Depends);
            Rendering = new(Pool, Host, Visual, new() { Reserve = _ => true, ReleaseArray = () => { } });
            Source = Factory.Create(133, SidAllocation.Client); Put(Source, 14, 2.125f); Put(Source, 18, 3.25f);
            System = new(Rendering, Game, Crt, Frame, new()
            {
                Lifetime = new()
                {
                    Reserve = bytes => { Assert.Equal(104u, bytes); return !FailObject; },
                    Create = (type, flags) => { Assert.Equal(110u, type); Assert.Equal(2u, flags); return Dummy = Factory.Create(type, (SidAllocation)flags); },
                    Pop = (sid, x, y, flags) => { Assert.Equal(Dummy, sid); Assert.Equal(16u, flags); if (FailPop) throw new InvalidOperationException("명시 Pop 실패"); Put(sid, 14, x); Put(sid, 18, y); },
                    Depend = (form, dummy) => { _ = Depends.Create(form, dummy); },
                    Capture = sid =>
                    {
                        Assert.Equal(Source, sid); Assert.Equal(1, Kernel.Size()); Assert.Equal(1, Rendering.Size);
                        if (FailCapture) throw new InvalidOperationException("명시 캡처 실패"); Capturing?.Invoke();
                        return new() { Width = 3, Height = 4, HotspotX = 1, HotspotY = 3, Pixels = Enumerable.Repeat(Transparent ? (byte)255 : (byte)71, 12).ToArray() };
                    },
                },
                Frame = new() { Reserve = bytes => bytes != FailBytes, Release = _ => { }, Invalidate = Dirty.Add, Pixel = Pixels.Add },
                Release = Releases.Add, DestroyUnpaired = sid => { Dispatch.Destroy(sid, 16); },
            });
        }
        /// <summary>원본좌표입력의float비트를기록한다.</summary>
        private void Put(int sid, int offset, float value) => BinaryPrimitives.WriteSingleLittleEndian(Pool.AllocatedBytes(sid).Slice(offset, 4), value);
        /// <summary>Host가살아있을때효과를먼저정리한다.</summary>
        public void Dispose() { System.Dispose(); Kernel.Clear(); }
    }
    /// <summary>세가상생성기모두원천삭제후독립bitmap/더미로실행하고자연소진으로상호종속을반납한다.</summary>
    [Theory]
    [InlineData(ShrapnelKind.Explosion)]
    [InlineData(ShrapnelKind.Collapse)]
    [InlineData(ShrapnelKind.Variant2)]
    public void ActualKernelSurvivesSourceAndExpiresWithPairedDummy(ShrapnelKind kind)
    {
        using Scene s = new(); ShrapnelVisualProcess process = Assert.IsType<ShrapnelVisualProcess>(s.System.Add(s.Source, kind));
        int form = process.Form(), dummy = s.Dummy; uint token = process.Fields.Registration.Token;
        Assert.Equal(16, s.Pool.Slot(form)[10]); Assert.Equal(dummy, process.Parent()); Assert.True(s.Depends.Has(form, dummy)); Assert.Same(process, s.Host.FromForm(form));
        Assert.True(s.Dispatch.Destroy(s.Source, 16)); s.Kernel.RunFrame(); s.Rendering.Draw(); Assert.Equal(6, s.Pixels.Count); Assert.Equal(2, s.Dirty.Count);
        s.Frame.Now = 100; s.Kernel.RunFrame(); Assert.Equal(1, s.System.Size); s.Frame.Now = 101; s.Kernel.RunFrame();
        Assert.Equal(0, s.System.Size); Assert.Equal(0, s.Kernel.Size()); Assert.Null(s.Rendering.Lookup(token)); Assert.Equal(0u, s.Rendering.Slots[0]);
        Assert.Equal(3, s.Pool.Slot(form)[11] & 3); Assert.Equal(3, s.Pool.Slot(dummy)[11] & 3); Assert.Equal(new[] { ShrapnelRelease.Source, ShrapnelRelease.Particles, ShrapnelRelease.Object }, s.Releases);
        Assert.Equal(0u, s.Destroy.PreDepth); Assert.Equal(0u, s.Destroy.PostDepth);
    }
    /// <summary>전체 투명 입력도 최소 용량1을 소유하고 첫 프레임에 모든 수명을 정리한다.</summary>
    [Fact]
    public void TransparentImageExpiresOnFirstFrame()
    {
        using Scene s = new(); s.Transparent = true; ShrapnelVisualProcess process = s.System.Add(s.Source, ShrapnelKind.Variant2)!;
        Assert.Single(process.Fields.Particles); Assert.Equal(0, process.Fields.Total); s.Kernel.RunFrame(); Assert.Equal(0, s.System.Size); Assert.Equal(0, s.Kernel.Size()); Assert.Empty(s.Pixels); Assert.Equal(3, s.Releases.Count);
    }
    /// <summary>104바이트 확보 실패는 등록/더미/난수 소비를 시작하지 않는다.</summary>
    [Fact]
    public void WrapperAllocationFailureLeavesNoLifetime()
    {
        using Scene s = new(); s.FailObject = true; uint game = s.Game.State(), crt = s.Crt.State(); Assert.Null(s.System.Add(s.Source, ShrapnelKind.Explosion));
        Assert.Equal(0, s.Dummy); Assert.Equal(0, s.Kernel.Size()); Assert.Equal(0, s.Rendering.Size); Assert.Empty(s.Releases); Assert.Equal(game, s.Game.State()); Assert.Equal(crt, s.Crt.State());
    }
    /// <summary>쌍구성전Pop실패는더미까지정리하고같은목록빈칸에서재생성한다.</summary>
    [Fact]
    public void PopFailureDestroysUnpairedDummyAndAllowsRetry()
    {
        using Scene s = new(); s.FailPop = true; Assert.Throws<InvalidOperationException>(() => s.System.Add(s.Source, ShrapnelKind.Variant2));
        Assert.Equal(0, s.Kernel.Size()); Assert.Equal(0, s.Rendering.Size); Assert.Equal(0, s.System.Size); Assert.Equal(3, s.Pool.Slot(s.Dummy)[11] & 3); Assert.Equal(new[] { ShrapnelRelease.Object }, s.Releases);
        s.FailPop = false; Assert.Equal(0, s.System.Add(s.Source, ShrapnelKind.Variant2)!.Fields.Registration.Slot);
    }
    /// <summary>쌍구성뒤캡처실패도실제Form삭제사슬이더미/감시Form을정리한다.</summary>
    [Fact]
    public void CaptureFailureDestroysAttachedLifetimeAndAllowsRetry()
    {
        using Scene s = new(); s.FailCapture = true; Assert.Throws<InvalidOperationException>(() => s.System.Add(s.Source, ShrapnelKind.Collapse));
        Assert.Equal(0, s.Kernel.Size()); Assert.Equal(0, s.Rendering.Size); Assert.Equal(0, s.System.Size); Assert.Equal(3, s.Pool.Slot(s.Dummy)[11] & 3);
        s.FailCapture = false; Assert.Equal(0, s.System.Add(s.Source, ShrapnelKind.Collapse)!.Fields.Registration.Slot);
    }
    /// <summary>입자/작업메모리실패때이미확보한bitmap과용량만순서대로반납한다.</summary>
    [Theory]
    [InlineData(288u, 2)]
    [InlineData(12u, 3)]
    public void PrepareAllocationFailureReleasesOnlyAcquiredStorage(uint bytes, int releases)
    {
        using Scene s = new(); s.FailBytes = bytes; Assert.Throws<OutOfMemoryException>(() => s.System.Add(s.Source, ShrapnelKind.Explosion));
        Assert.Equal(0, s.Kernel.Size()); Assert.Equal(0, s.Rendering.Size); Assert.Equal(0, s.System.Size); Assert.Equal(3, s.Pool.Slot(s.Dummy)[11] & 3); Assert.Equal(releases, s.Releases.Count);
    }
    /// <summary>준비중실제Kernel/수집/Draw는미완성버퍼를공개하지않고중첩생성을거부한다.</summary>
    [Fact]
    public void CreationReentryDoesNotExposeIncompleteBuffers()
    {
        using Scene s = new(); s.Capturing = () =>
        {
            s.Kernel.RunFrame(); s.Rendering.Draw(); Assert.Empty(s.Pixels); Assert.Empty(s.Dirty);
            Assert.Equal(0, s.Rendering.Collect(new(-1000, -1000, 1000, 1000), new RenderCollectionEntry[1], 1));
            Assert.Throws<ArgumentException>(() => s.System.Add(s.Source, ShrapnelKind.Variant2));
        };
        Assert.NotNull(s.System.Add(s.Source, ShrapnelKind.Variant2)); s.Kernel.RunFrame(); Assert.Equal(2, s.Dirty.Count);
    }
    /// <summary>공통현재배율과다른값은최초등록과후속출력전에거부한다.</summary>
    [Fact]
    public void CurrentScaleMismatchIsRejectedBeforeCreationAndDraw()
    {
        using Scene s = new(); s.Frame.PixelScaleX = 8; Assert.Throws<InvalidOperationException>(() => s.System.Add(s.Source, ShrapnelKind.Collapse)); Assert.Equal(0, s.Rendering.Size);
        s.Frame.PixelScaleX = 16; _ = s.System.Add(s.Source, ShrapnelKind.Collapse); s.Frame.PixelScaleY = 3; Assert.Throws<InvalidOperationException>(s.Rendering.Draw); s.Frame.PixelScaleY = 11;
    }
}
