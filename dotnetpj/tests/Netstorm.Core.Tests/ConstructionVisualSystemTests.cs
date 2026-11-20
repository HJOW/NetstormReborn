using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 Kernel·Form·등록 목록으로 건설 표시의 생성·실행·삭제·실패 복구를 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 C++ 통합 계약을 실제10.78 수명 모듈에서 실행한다. 장치와캡처는 명시 입력이다.</summary>
public sealed class ConstructionVisualSystemTests
{
    /// <summary>같은 실제 풀·Host·Kernel·표시 목록을 사용하는 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제 서버/클라이언트 영역을 갖는 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>프로세스의 실제 소유자다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>Form 삭제·부모 연결을 처리하는 실제Host다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>공통 부모/종속 삭제의 실제 본체다.</summary>
        public readonly RawSquidDestroy Destroy;
        /// <summary>원본 생성자 주소와같은TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>현재 표시 역할·공유 팔레트·배율이다.</summary>
        public readonly ConstructionVisualState VisualState = new() { Palette = 71 };
        /// <summary>현재 게임 시각·카메라다.</summary>
        public readonly ConstructionFrameState FrameState = new() { Now = 2 };
        /// <summary>색수 경계의인접DWORD까지 포함한 입력이다.</summary>
        public readonly ConstructionFramePalette Palette = new() { Token = 71, Count = 11, Entries = Enumerable.Range(0, 12).Select(value => (uint)value).ToArray() };
        /// <summary>생성과프레임이 공유하는 실제 난수다.</summary>
        public readonly GameRandom Random = new();
        /// <summary>원본 버퍼 반납 순서의 실제 기록이다.</summary>
        public readonly List<ConstructionRelease> Released = [];
        /// <summary>원본40칸 목록 확장의 바이트 수다.</summary>
        public readonly List<uint> Allocations = [];
        /// <summary>현재 프레임의dirty 요청이다.</summary>
        public readonly List<ConstructionFrameRect> Dirty = [];
        /// <summary>현재 카메라를 적용한 출력 위치다.</summary>
        public readonly List<(int X, int Y)> Draws = [];
        /// <summary>최대HP 장치의실제 호출 횟수다.</summary>
        public int MaximumCalls;
        /// <summary>실제 캡처 횟수다.</summary>
        public int Captures;
        /// <summary>서버 영역의실제 부모 자산이다.</summary>
        public readonly int Parent;
        /// <summary>장면보다먼저 반납할현재 표시 관리자다.</summary>
        public ConstructionVisualSystem? OwnedSystem;
        /// <summary>생성을마친 현재 관리자를 읽는다.</summary>
        public ConstructionVisualSystem System => OwnedSystem ?? throw new InvalidOperationException("검사 표시 시스템 미생성");
        /// <summary>현재부모의 실제Form 체인 머리다.</summary>
        public int Head => BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(Parent).Slice(6, 2));

        /// <summary>원본 생성자 표로실제Host를구성하고서버부모를확보한다.</summary>
        public Scene()
        {
            Types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, index) }).ToArray();
            Types[100].Flags1 = 0x10; Types[100].MaxHitPoints = 100;
            Destroy = new(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Host = new(Pool, Types, Kernel, Destroy, new SquidProcessState(), asset: new SquidDestroyHooks
            {
                Selected = () => 0,
                Emit = effect => { if (effect.Effect == SquidDestroyEffect.PreDestroy) Destroy.CompletePreDestroy(); else if (effect.Effect == SquidDestroyEffect.PostDestroy) Destroy.CompletePostDestroy(); },
            });
            Parent = Pool.Allocate(SidAllocation.Server); Span<byte> raw = Pool.AllocatedBytes(Parent); raw.Clear();
            Put(0, 0x501dd0); raw[10] = 100; raw[34] = 1; raw[35] = 255; raw[40] = 1; Put(14, Bits(20.5f)); Put(18, Bits(21.25f));
        }
        /// <summary>검사부모의DWORD 입력만바꾼다.</summary>
        private void Put(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.AllocatedBytes(Parent).Slice(offset, 4), value);
        /// <summary>부모의실제PATCH HP 입력이다.</summary>
        public void Hp(uint value) => Put(26, value);
        /// <summary>실제목록의예약만관찰하며부착은시스템이Host에연결한다.</summary>
        public RenderRegistryHooks RegistryHooks() => new() { Reserve = bytes => { Allocations.Add(bytes); return true; }, ReleaseArray = () => { } };
        /// <summary>실제Form·목록·Kernel부착이끝난뒤명시5×7캡처를공급한다.</summary>
        public ConstructionVisualHooks VisualHooks() => new()
        {
            Reserve = _ => true, CreatePalette = () => 71, Gradient = (_, _, _, _) => { }, BuildPalette = _ => { }, ReleaseParticles = () => { },
            Capture = sid =>
            {
                Assert.Equal(Parent, sid); Assert.True(System.Size > 0); Assert.True(Kernel.Size() > 0);
                var process = Assert.IsType<ConstructionVisualProcess>(Host.FromForm(Head)); Assert.Equal(17, Pool.Slot(Head)[10]);
                Assert.Equal(process.Registration.Token, System.Slots[process.Registration.Slot]); Captures++;
                return new() { Width = 5, Height = 7, HotspotX = 2, HotspotY = 3, Pixels = Enumerable.Repeat((byte)17, 35).ToArray() };
            },
        };
        /// <summary>실제프레임의HP·dirty·출력요청만관찰한다.</summary>
        public ConstructionFrameHooks FrameHooks() => new()
        {
            MaxHitPoints = sid => { Assert.Equal(Parent, sid); MaximumCalls++; return 100; }, Invalidate = Dirty.Add,
            Draw = request => { Assert.Equal(5, request.Pitch); Assert.Equal(35, request.Pixels.Length); Assert.Equal(5, request.Clip.Right); Assert.Equal(7, request.Clip.Bottom); Draws.Add((request.X, request.Y)); },
        };
        /// <summary>지정장치또는기본장치로같은관리자를마지막에소유한다.</summary>
        public void Create(ConstructionVisualHooks? visual = null, ConstructionFrameHooks? frame = null) => OwnedSystem = new(Pool, Host, VisualState, FrameState, Palette, Random, RegistryHooks(), visual ?? VisualHooks(), frame ?? FrameHooks(), Released.Add);
        /// <summary>표시선행·실제HP진행·현재contained삭제를같은건설시작에연결한다.</summary>
        public ConstructionProcessHooks BuildHooks()
        {
            var contained = new RawContainedLookup(Pool, (form, flags) => Host.Kill(Host.FromForm(form) ?? throw new InvalidOperationException("검사 contained 프로세스 없음"), flags));
            return System.MakeProcessHooks(Pool, new()
            {
                Reserve = () => true, LoadSound = _ => 0, ReserveSound = () => false, AttachSound = (_, _, _, _) => { }, Refresh = () => { },
                MaxHitPoints = _ => 100, UpdateDisplay = _ => { }, RemoveContained = (sid, type) => { _ = contained.Find(sid, type, 0, true); },
                PlayAt = (_, _, _, _) => { }, ReadySound = _ => 0, Play = _ => { },
                Unpop = (sid, _) => Pool.AllocatedBytes(sid)[11] |= 4, Repop = (sid, flags) => { Assert.Equal(9u, flags); Pool.AllocatedBytes(sid)[11] &= 0xfb; }, Push = (_, _, _, _) => { },
            });
        }
        /// <summary>Host보다먼저관리자를종료하고실제Kernel의남은소유권을정리한다.</summary>
        public void Dispose() { OwnedSystem?.Dispose(); Kernel.Clear(); }
    }
    /// <summary>float좌표의입력비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>모든완성객체의버퍼3개·등록뒤객체반납순서를확인한다.</summary>
    private static void CheckReleased(Scene scene, int count)
    {
        ConstructionRelease[] one = [ConstructionRelease.Source, ConstructionRelease.Output, ConstructionRelease.Particles, ConstructionRelease.Object];
        Assert.Equal(Enumerable.Range(0, count).SelectMany(_ => one).ToArray(), scene.Released);
    }

    /// <summary>실제client Form·Kernel·현재카메라Draw·자기삭제와빈칸재사용이다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void AttachRunDrawKillAndReuse(bool instant)
    {
        using var scene = new Scene(); scene.Create(); ConstructionVisualProcess process = scene.System.Add(scene.Parent, instant); int form = process.Form();
        Assert.InRange(form, 5, 14999); Assert.Same(process, scene.Host.FromForm(form)); Assert.Equal(0, process.Registration.Slot); Assert.Equal(1, scene.Kernel.Size());
        scene.Hp(50); scene.Kernel.RunFrame(); Assert.Equal(2, scene.Dirty.Count); Assert.Equal(-1, process.Registration.ZOrder);
        scene.FrameState.CameraX = 4; scene.FrameState.CameraY = 5; scene.System.Draw(); Assert.Equal((322, 225), Assert.Single(scene.Draws));
        scene.Pool.AllocatedBytes(scene.Parent)[40] = 0; scene.Kernel.RunFrame();
        Assert.Equal(0, scene.System.Size); Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(0u, scene.System.Slots[0]); Assert.Equal(1, scene.Pool.Slot(form)[11] & 1); CheckReleased(scene, 1);
        scene.Pool.AllocatedBytes(scene.Parent)[40] = 1; ConstructionVisualProcess next = scene.System.Add(scene.Parent, !instant);
        Assert.Equal(0, next.Registration.Slot); Assert.Equal(new uint[] { 160 }, scene.Allocations);
    }

    /// <summary>Start는표시17뒤HP진행30을붙이며완료다음Kernel순회에서표시도종료한다.</summary>
    [Theory]
    [InlineData(0f)]
    [InlineData(8f)]
    public void StartRunsBothProcessesToCompletion(float duration)
    {
        using var scene = new Scene(); scene.Create(); var state = new ConstructionProcessState { Now = 2 };
        var build = new RawConstructionProcess(scene.Pool, scene.Types, state, scene.BuildHooks(), scene.Host); build.Start(scene.Parent, duration);
        var visual = Assert.IsType<ConstructionVisualProcess>(scene.Kernel.Get(1)); var progress = Assert.IsType<ConstructionProcess>(scene.Kernel.Get(2));
        Assert.Equal(visual.Form(), BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(progress.Form()).Slice(4, 2)));
        scene.FrameState.Now = state.Now = 6; scene.Kernel.RunFrame(); Assert.Equal(duration == 0 ? 40 : 50, BinaryPrimitives.ReadInt32LittleEndian(scene.Pool.Slot(scene.Parent).Slice(26, 4)));
        scene.FrameState.Now = state.Now = duration == 0 ? 12 : 10; scene.Kernel.RunFrame(); Assert.Equal(1, scene.Kernel.Size()); Assert.Equal(0, scene.Pool.Slot(scene.Parent)[40] & 1);
        scene.Kernel.RunFrame(); Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(0, scene.System.Size); Assert.Equal(0, scene.Head); CheckReleased(scene, 1);
    }

    /// <summary>현재contained삭제는가장최근표시만지우고다른표시/Regular링크를보존한다.</summary>
    [Fact]
    public void ContainedRemovalPreservesOtherProcesses()
    {
        using var scene = new Scene(); scene.Create(); ConstructionVisualProcess first = scene.System.Add(scene.Parent, false), second = scene.System.Add(scene.Parent, true);
        RegularProcess regular = scene.Host.AddRegular(scene.Parent, 1, 1) ?? throw new InvalidOperationException("검사 Regular 부착 실패");
        var lookup = new RawContainedLookup(scene.Pool, (form, flags) => scene.Host.Kill(scene.Host.FromForm(form)!, flags));
        Assert.Equal(second.Form(), lookup.Find(scene.Parent, 17)); _ = lookup.Find(scene.Parent, 17, 0, true);
        Assert.Equal(2, scene.Kernel.Size()); Assert.Equal(1, scene.System.Size); Assert.Equal(first.Form(), BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(regular.Form()).Slice(4, 2)));
        _ = lookup.Find(scene.Parent, 17, 0, true); Assert.Equal(1, scene.Kernel.Size()); CheckReleased(scene, 2); scene.Host.Kill(regular, 0); Assert.Equal(0, scene.Head);
    }

    /// <summary>실제부모의공통삭제가종속Form·Kernel·등록·버퍼를모두정리한다.</summary>
    [Fact]
    public void ParentDestructionReleasesVisuals()
    {
        using var scene = new Scene(); scene.Create(); int first = scene.System.Add(scene.Parent, false).Form(), second = scene.System.Add(scene.Parent, true).Form();
        scene.Pool.AllocatedBytes(scene.Parent)[11] |= 4; scene.Destroy.Destroy(scene.Parent, 0, scene.Host.Hooks());
        Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(0, scene.System.Size); Assert.Equal(1, scene.Pool.Slot(first)[11] & 1); Assert.Equal(1, scene.Pool.Slot(second)[11] & 1); CheckReleased(scene, 2);
    }

    /// <summary>시스템을먼저종료해도실제Host에Form·Kernel참조를남기지않는다.</summary>
    [Fact]
    public void ShutdownDeletesRemainingForms()
    {
        using var scene = new Scene(); scene.Create(); _ = scene.System.Add(scene.Parent, false); _ = scene.System.Add(scene.Parent, true); scene.System.Dispose();
        Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(0, scene.Head); Assert.Equal(71u, scene.VisualState.Palette); CheckReleased(scene, 2);
    }

    /// <summary>부착전후실패는기존객체를보존하고새Form·Kernel·목록만정리한다.</summary>
    [Fact]
    public void FailedCreationRollsBackNewLifetime()
    {
        using var scene = new Scene(); bool failObject = false, failCapture = false; ConstructionVisualHooks hooks = scene.VisualHooks(); var capture = hooks.Capture!;
        hooks.Reserve = bytes => !(failObject && bytes == 136); hooks.Capture = sid => failCapture ? throw new InvalidOperationException("검사 캡처 실패") : capture(sid);
        scene.Create(hooks); ConstructionVisualProcess alive = scene.System.Add(scene.Parent, false); failObject = true;
        Assert.Throws<OutOfMemoryException>(() => scene.System.Add(scene.Parent, false)); failObject = false; failCapture = true;
        Assert.Throws<InvalidOperationException>(() => scene.System.Add(scene.Parent, false)); failCapture = false;
        scene.VisualState.ProcessType = 100; Assert.ThrowsAny<Exception>(() => scene.System.Add(scene.Parent, false)); scene.VisualState.ProcessType = 17;
        Assert.Equal(1, scene.System.Size); Assert.Equal(1, scene.Kernel.Size()); Assert.Same(alive, scene.Host.FromForm(alive.Form())); Assert.Equal(alive.Form(), scene.Head);
        Assert.Equal(1, scene.System.Add(scene.Parent, true).Registration.Slot);
    }

    /// <summary>캡처중Kernel/Draw/수집은미완성버퍼를소비하지않고중첩Add를거부한다.</summary>
    [Fact]
    public void CreationReentrySkipsIncompleteBuffers()
    {
        using var scene = new Scene(); ConstructionVisualHooks hooks = scene.VisualHooks(); var capture = hooks.Capture!;
        hooks.Capture = sid =>
        {
            scene.Kernel.RunFrame(); scene.System.Draw(); Assert.Equal(0, scene.MaximumCalls); Assert.Empty(scene.Draws);
            var output = new RenderCollectionEntry[1]; Assert.Equal(0, scene.System.Collect(new(-1000, -1000, 1000, 1000), output, 1));
            Assert.Throws<InvalidOperationException>(() => scene.System.Add(scene.Parent, false)); return capture(sid);
        };
        scene.Create(hooks); _ = scene.System.Add(scene.Parent, false); scene.Kernel.RunFrame(); Assert.Equal(1, scene.MaximumCalls); Assert.Equal(1, scene.System.Size);
    }

    /// <summary>출력중자기삭제와40→80칸확장에도해제된배열/객체를다시읽지않는다.</summary>
    [Fact]
    public void DrawDeletionAndListGrowth()
    {
        using var scene = new Scene(); ConstructionFrameHooks hooks = scene.FrameHooks(); bool changed = false; int draws = 0; ConstructionVisualProcess? first = null;
        hooks.Draw = request =>
        {
            draws++; if (changed) return; changed = true; scene.Host.Kill(first!, 0);
            // 지나간0번슬롯은다음Draw에,나머지현재슬롯은이번Draw에제출된다.
            for (int index = 0; index < 41; index++) _ = scene.System.Add(scene.Parent, false);
        };
        scene.Create(frame: hooks); first = scene.System.Add(scene.Parent, false); scene.System.Draw();
        Assert.Equal(41, draws); Assert.Equal(41, scene.System.Size); Assert.Equal(80, scene.System.Slots.Length); Assert.Equal(new uint[] { 160, 320 }, scene.Allocations);
        scene.System.Draw(); Assert.Equal(82, draws); CheckReleased(scene, 1); scene.System.Dispose(); Assert.Equal(0, scene.Kernel.Size()); CheckReleased(scene, 42);
    }

    /// <summary>수집뒤삭제한토큰은건너뛰고재사용슬롯의새객체를이전목록에섞지않는다.</summary>
    [Fact]
    public void CollectedSnapshotDeletionAndReuse()
    {
        using var scene = new Scene(); scene.Create(); ConstructionVisualProcess first = scene.System.Add(scene.Parent, false), second = scene.System.Add(scene.Parent, false);
        var output = new RenderCollectionEntry[3]; Assert.Equal(2, scene.System.Collect(new(20, 20, 22, 23), output, 3)); uint oldToken = output[0].Token;
        scene.Host.Kill(first, 0); ConstructionVisualProcess replacement = scene.System.Add(scene.Parent, false); Assert.NotEqual(oldToken, replacement.Registration.Token);
        scene.System.DrawCollected(output.AsSpan(0, 2)); Assert.Single(scene.Draws); Assert.Equal(second.Registration.Token, output[1].Token);
    }

    /// <summary>외부공통목록을공유한두시스템은자기수명만반납하고다른표시를보존한다.</summary>
    [Fact]
    public void SharedRenderingPreservesOtherSystem()
    {
        using var scene = new Scene(); scene.Create(); RenderProcessSystem rendering = scene.System.Rendering;
        ConstructionVisualHooks hooks = scene.VisualHooks(); hooks.Capture = _ => new() { Width = 5, Height = 7, Pixels = Enumerable.Repeat((byte)17, 35).ToArray() };
        using var second = new ConstructionVisualSystem(rendering, scene.FrameState, scene.Palette, scene.Random, hooks, scene.FrameHooks(), scene.Released.Add);
        _ = scene.System.Add(scene.Parent, false); _ = second.Add(scene.Parent, false); Assert.Equal(2, rendering.Size);
        scene.System.Dispose(); Assert.Equal(1, second.Size); Assert.Equal(1, rendering.Size); rendering.Draw(); Assert.Single(scene.Draws);
    }
}
