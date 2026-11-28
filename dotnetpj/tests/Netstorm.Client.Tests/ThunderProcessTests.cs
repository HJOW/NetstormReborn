using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 화면/시각은 명시 장치이고 객체·Form·종속 체인·프레임·삭제는 실제 공통 제품을 사용한다.
namespace Netstorm.Client.Tests;

/// <summary>ThunderProcess를 실제 Host/Kernel에 부착해 생성·진행·반납·선택된 실패 접두를 검사한다.</summary>
public sealed class ThunderProcessTests
{
    /// <summary>실제 void bridge 부모와 ProcessForm 생성자·공통 삭제를 사용하는 독립 장면이다. Game의 기술 부모 조회를 대신하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 슬롯/등록/삭제가 공유하는 하나의 풀이다.</summary>
        public RawSidPool Pool { get; } = new(32768, false);
        /// <summary>실제 슬롯 순서 실행/가상 소멸을 수행하는 커널이다.</summary>
        public Kernel Kernel { get; } = new(8);
        /// <summary>같은 실제188TYPE 생성자 표다. 외부 자산 파일/성공 응답은 만들지 않는다.</summary>
        public OriginalTypeRecord[] Types { get; }
        /// <summary>실제 공통 생성자로 부모와 ProcessForm을 만든다.</summary>
        public RawSquidFactory Factory { get; }
        /// <summary>실제 종속 목록과 Form 삭제를 실행한다.</summary>
        public RawSquidDestroy Destroy { get; }
        /// <summary>동일 Pool/Kernel/공장을 쓰는 실제 부착·삭제 소유자다.</summary>
        public SquidProcessHost Host { get; }
        /// <summary>Factory.Create(82,Client)로 생성한 실제 void bridge SID다.</summary>
        public int Parent { get; }
        /// <summary>같은 현재/논리/저장 팔레트를 사용하는 창 없는 화면이다.</summary>
        public Screen Screen { get; } = new(8, 4);
        /// <summary>CPP Time().wall처럼 현재 프레임 캐시를 공급하는 실제 상자다.</summary>
        public FrameTimeBox Time { get; } = new();
        /// <summary>현재 장면에 생성/부착한 실제 프로세스이며 수명은 Kernel이 맡는다.</summary>
        public ThunderProcess? Process { get; private set; }
        /// <summary>Client 등록 목록 역할의 PID 관측값이다. 프레임/삭제를 대신 실행하지 않는다.</summary>
        public HashSet<uint> Registered { get; } = [];
        /// <summary>실제 소멸 callback이 받은 PID 순서다. 미등록 조용한 거부는0을 받는다.</summary>
        public List<uint> Destroyed { get; } = [];
        /// <summary>실제 장치 진입 순서만 기록한다.</summary>
        public List<string> Events { get; } = [];
        /// <summary>실제 Init/Frame이 읽은 벽시각 횟수다.</summary>
        public int Clocks { get; private set; }
        /// <summary>현재 화면 모드 조회 횟수다.</summary>
        public int Modes { get; private set; }
        /// <summary>Init의 실제 두 부분 팔레트 읽기 횟수다.</summary>
        public int Reads { get; private set; }
        /// <summary>실제 부분 색 적용 횟수다.</summary>
        public int Applies { get; private set; }
        /// <summary>실제 저장 팔레트 복구 횟수다.</summary>
        public int Restores { get; private set; }
        /// <summary>Init의 clock 위치에서 선택할 명시 실패다.</summary>
        public bool FailClock { get; set; }
        /// <summary>이 번호의 Init 읽기가 작성 뒤 실패한다.0이면 실패하지 않는다.</summary>
        public int FailRead { get; set; }
        /// <summary>부분 팔레트를 실제 쓴 뒤 선택할 명시 실패다.</summary>
        public bool FailApply { get; set; }
        /// <summary>현재 저장 팔레트를 실제 복구한 뒤 선택할 명시 실패다.</summary>
        public bool FailRestore { get; set; }
        /// <summary>소멸 PID를 관측하고 등록 해제하기 직전에 선택할 명시 실패다.</summary>
        public bool FailDestroyed { get; set; }

        /// <summary>프로세스/종속 삭제는 기존 제품으로 구성하고 부모 자산의 공통 pre/post 완료만 제공한다.</summary>
        public Scene(bool fullScreen = false)
        {
            Types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord
            {
                ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, index), FootX = 1, FootY = 1,
            }).ToArray();
            var unpop = new RawUnpop(Pool, new RawSquidHash(), new byte[65536]);
            Destroy = new(Pool, unpop, Types);
            Host = new(Pool, Types, Kernel, Destroy, new SquidProcessState(), asset: new()
            {
                Selected = () => 0,
                Emit = effect =>
                {
                    if (effect.Effect == SquidDestroyEffect.PreDestroy) Destroy.CompletePreDestroy();
                    else if (effect.Effect == SquidDestroyEffect.PostDestroy) Destroy.CompletePostDestroy();
                    else throw new InvalidOperationException("번개 독립 장면의 미선택 자산 효과: " + effect.Effect);
                },
            });
            Factory = new(Pool, Types); Host.BindFormFactory(Factory);
            Parent = Factory.Create(82, SidAllocation.Client);
            Assert.Equal((byte)82, Pool.Slot(Parent)[10]); Assert.NotEqual(0, Pool.Slot(Parent)[11] & RawSidPool.VoidBit);
            Screen.SetMode(ScreenMode.BackBufferDib | (fullScreen ? ScreenMode.FullScreen : 0));
            Palette(140, 120, 100, 99);
        }

        /// <summary>전체256색의 독립 입력을 기존 값 로더로 읽는다. 새 native 팔레트 초안에는 의존하지 않는다.</summary>
        public void Palette(byte blue, byte green, byte red, byte reserved)
        {
            byte[] bytes = new byte[1024];
            // 명시 입력 파일의 BGRX256칸을 채우며 기대 번개 결과를 계산하지 않는다.
            for (int index = 0; index < 256; index++) { bytes[index * 4] = blue; bytes[index * 4 + 1] = green; bytes[index * 4 + 2] = red; bytes[index * 4 + 3] = reserved; }
            Screen.LoadPalette(new GamePalette(bytes));
        }

        /// <summary>OnAttached보다 앞선 실제 Kernel/PID/Form/부모 부착을 장치 진입 시 확인한다.</summary>
        private void AttachedIdentity()
        {
            Assert.NotNull(Process); uint id = Kernel.Find(Process); Assert.NotEqual(0u, id);
            Assert.Same(Process, Kernel.Get(id)); Assert.Same(Process, Host.FromForm(Process.Form()));
            Assert.Equal(Parent, Process.Parent()); Assert.Equal(Parent, RawContainedLink.CommonContainedParent(Pool, Process.Form()));
            Assert.Equal(Process.Form(), Head()); Assert.Equal((byte)ThunderProcess.ProcessType, Pool.Slot(Process.Form())[10]);
            Assert.Equal(id, BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(Process.Form())[18..]));
        }

        /// <summary>현재 화면/캐시 시각만 빌리고 시간 진행이나 프로세스 실행은 대신하지 않는다.</summary>
        private PaletteFlashHooks Hooks() => new()
        {
            WallSeconds = () => { Clocks++; Events.Add("clock"); AttachedIdentity(); if (FailClock) throw new InvalidOperationException("clock"); return Time.Value.Wall; },
            FullScreen = () => { Modes++; Events.Add("mode"); return (Screen.Flags & ScreenMode.FullScreen) != 0; },
            ReadPalette = (start, count, destination) =>
            {
                Reads++; Events.Add("read" + Reads); AttachedIdentity();
                Screen.Palette.Slice((int)start, (int)count).CopyTo(destination);
                if (Reads == FailRead) throw new InvalidOperationException("read" + Reads);
            },
            ApplyPalette = (start, count, colors) =>
            {
                Applies++; Events.Add("apply"); Screen.SetPalette(start, count, colors, true);
                if (FailApply) throw new InvalidOperationException("apply");
            },
            RestorePalette = () =>
            {
                Restores++; Events.Add("restore"); Screen.SetPalette(0, 256, null, true);
                if (FailRestore) throw new InvalidOperationException("restore");
            },
        };

        /// <summary>생성자 순수성과 실제 소멸 중 PID 가시성을 검사할 callback만 보관한다.</summary>
        public ThunderProcess Create(int brightness = 0, bool? creationFullScreen = null)
        {
            Process = new(creationFullScreen ?? ((Screen.Flags & ScreenMode.FullScreen) != 0), Hooks(), id =>
            {
                Events.Add("destroy"); Destroyed.Add(id);
                if (id != 0) { Assert.Same(Process, Kernel.Get(id)); Assert.Equal(id, Kernel.Find(Process!)); }
                if (FailDestroyed) throw new InvalidOperationException("destroy");
                Registered.Remove(id);
            }, brightness);
            return Process;
        }

        /// <summary>실제 TYPE55/flags10 부착이 정상 복귀한 경우에만 현재 Kernel PID를 등록한다.</summary>
        public ThunderProcess? Attach()
        {
            ThunderProcess? attached = (ThunderProcess?)Host.Attach(Process ?? throw new InvalidOperationException("생성 전 부착"), ThunderProcess.ProcessType, Parent, 0x10);
            if (attached is not null) Registered.Add(Kernel.Find(attached));
            return attached;
        }

        /// <summary>별도 목록 없이 같은 실제 부모+6 WORD를 읽는다.</summary>
        public int Head() => BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(Parent)[6..]);
    }

    /// <summary>생성자는 callback을 읽지 않고 실제 부착 후에만 clock→팔레트 두 읽기를 수행한다.</summary>
    [Fact]
    public void ConstructorIsQuietAndActualAttachPublishesIdentityBeforeInit()
    {
        var scene = new Scene(); ThunderProcess process = scene.Create();
        Assert.Empty(scene.Events); Assert.Equal(0, process.Form()); Assert.Equal(0, process.Parent());
        Assert.Same(process, scene.Attach()); Assert.Equal(new[] { "clock", "read1", "read2" }, scene.Events);
        Assert.Equal(0, scene.Modes); Assert.Equal(1, scene.Kernel.Size()); Assert.Single(scene.Registered);
        uint id = scene.Kernel.Find(process); int form = process.Form(); scene.Time.Value.Wall = 1;
        scene.Kernel.RunFrame();
        Assert.Null(scene.Kernel.Get(id)); Assert.Equal(new[] { id }, scene.Destroyed); Assert.Empty(scene.Registered);
        Assert.Equal(0, scene.Applies); Assert.Equal(0, scene.Restores); Assert.NotEqual(0, scene.Pool.Slot(form)[11] & RawSidPool.FreeBit); Assert.Equal(0, scene.Head());
    }

    /// <summary>같은 Kernel 프레임이 실제 부분 색을 적용하고 자연 종료는 복구/등록 해제를 한 번만 수행한다.</summary>
    [Fact]
    public void NaturalFullScreenCompletionRestoresBeforeSinglePidRetirement()
    {
        var scene = new Scene(true); ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process); int form = process.Form();
        scene.Kernel.RunFrame(); Assert.Equal(164, scene.Screen.Palette[228].Red); Assert.Equal(100, scene.Screen.Palette[227].Red);
        scene.Time.Value.Wall = 1; scene.Kernel.RunFrame();
        Assert.Equal(100, scene.Screen.Palette[228].Red); Assert.Equal(1, scene.Restores); Assert.Equal(new[] { id }, scene.Destroyed);
        Assert.True(scene.Events.IndexOf("restore") < scene.Events.IndexOf("destroy")); Assert.Null(scene.Kernel.Get(id));
        Assert.NotEqual(0, scene.Pool.Slot(form)[11] & RawSidPool.FreeBit); Assert.Equal(0u, scene.Destroy.PreDepth); Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>signed 밝기와 최신 저장 팔레트가 실제 부착/명시 Host.Kill을 거쳐 소비된다.</summary>
    [Fact]
    public void ExplicitKillUsesSignedBrightnessAndRestoresLatestSavedPalette()
    {
        var scene = new Scene(true); ThunderProcess process = scene.Create(-17); scene.Attach(); uint id = scene.Kernel.Find(process);
        scene.Kernel.RunFrame(); Assert.Equal(new ScreenColor(123, 103, 83, 99), scene.Screen.Palette[228]);
        scene.Palette(30, 30, 30, 30); scene.Host.Kill(process, 0x10);
        Assert.Equal(new ScreenColor(30, 30, 30, 30), scene.Screen.Palette[228]); Assert.Equal(1, scene.Restores);
        Assert.Equal(new[] { id }, scene.Destroyed); Assert.Null(scene.Kernel.Get(id)); Assert.Empty(scene.Registered); Assert.Equal(0, scene.Head());
    }

    /// <summary>실제 부모 Destroy가 종속 Form을 찾아 같은 Kernel 객체를 반납한다.</summary>
    [Fact]
    public void ActualParentDestroyReleasesThunderFormAndSameKernelObject()
    {
        var scene = new Scene(true); ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process); int form = process.Form();
        scene.Destroy.Destroy(scene.Parent, 0x10, scene.Host.Hooks());
        Assert.Null(scene.Kernel.Get(id)); Assert.Equal(new[] { id }, scene.Destroyed); Assert.Equal(1, scene.Restores); Assert.Empty(scene.Registered);
        Assert.NotEqual(0, scene.Pool.Slot(scene.Parent)[11] & RawSidPool.FreeBit); Assert.NotEqual(0, scene.Pool.Slot(form)[11] & RawSidPool.FreeBit);
        Assert.Equal(0u, scene.Destroy.PreDepth); Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>Kernel.Clear는 화면/PID 수명만 닫고 Form/부모 체인의 뒤 물리 정리를 합성하지 않는다.</summary>
    [Fact]
    public void KernelClearRestoresAndUnregistersButKeepsFormCleanupBoundary()
    {
        var scene = new Scene(true); ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process); int form = process.Form();
        scene.Kernel.Clear();
        Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(new[] { id }, scene.Destroyed); Assert.Equal(1, scene.Restores); Assert.Empty(scene.Registered);
        Assert.Equal(0, scene.Pool.Slot(form)[11] & RawSidPool.FreeBit); Assert.Equal(form, scene.Head());
    }

    /// <summary>창 모드의 짧은 수명은 생성 당시 값이며 현재 모드 조회는 부착 후 각 프레임에서 다시 한다.</summary>
    [Fact]
    public void CapturedWindowDurationAndCurrentFullScreenModeStayIndependent()
    {
        var scene = new Scene(); ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process);
        scene.Screen.SetMode(ScreenMode.FullScreen | ScreenMode.BackBufferDib); scene.Time.Value.Wall = 0.02;
        scene.Kernel.RunFrame();
        Assert.Null(scene.Kernel.Get(id)); Assert.Equal(1, scene.Restores); Assert.Equal(0, scene.Applies); Assert.Equal(new[] { id }, scene.Destroyed);
    }

    /// <summary>원문 비활성 부모 거부는 등록/clock/팔레트 읽기 전에 local 소유를 PID0으로 반납한다.</summary>
    [Theory]
    [InlineData(RawSidPool.FreeBit)]
    [InlineData(RawSidPool.DeadBit)]
    public void InactiveActualParentRejectsWithoutReadingClockOrPalette(byte flag)
    {
        var scene = new Scene(true); scene.Pool.AllocatedBytes(scene.Parent)[11] |= flag; scene.Create(); uint free = scene.Pool.FreeCount;
        Assert.Null(scene.Attach()); Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(free, scene.Pool.FreeCount);
        Assert.Equal(new uint[] { 0 }, scene.Destroyed); Assert.Equal(new[] { "destroy" }, scene.Events); Assert.Empty(scene.Registered);
    }

    /// <summary>Init 실패는 이미 부착한 Form/Kernel을 남기고 초기화 미완료 Cancel에서는 복구를 합성하지 않는다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    public void InitFailureKeepsTransferredOwnershipAndItsExactReadPrefix(int failure)
    {
        var scene = new Scene(true) { FailClock = failure == 0, FailRead = failure }; ThunderProcess process = scene.Create();
        string cause = failure == 0 ? "clock" : "read" + failure;
        Assert.Equal(cause, Assert.Throws<InvalidOperationException>(() => scene.Attach()).Message);
        uint id = scene.Kernel.Find(process); Assert.NotEqual(0u, id); Assert.Same(process, scene.Kernel.Get(id)); Assert.Equal(process.Form(), scene.Head());
        Assert.Empty(scene.Destroyed); Assert.Empty(scene.Registered); Assert.Equal(1, scene.Clocks); Assert.Equal(failure, scene.Reads);
        scene.Kernel.Clear(); Assert.Equal(new[] { id }, scene.Destroyed); Assert.Equal(0, scene.Restores); Assert.Equal(0, scene.Modes);
    }

    /// <summary>Frame 적용이 실제 화면을 쓴 뒤 던져도 같은 Kernel 객체/등록을 보존하고 뒤 명시 Kill이 반납한다.</summary>
    [Fact]
    public void ApplyFailureKeepsVisiblePrefixAndKernelOwnershipUntilActualKill()
    {
        var scene = new Scene(true) { FailApply = true }; ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process);
        Assert.Equal("apply", Assert.Throws<InvalidOperationException>(scene.Kernel.RunFrame).Message);
        Assert.Equal(164, scene.Screen.Palette[228].Red); Assert.Same(process, scene.Kernel.Get(id)); Assert.Contains(id, scene.Registered); Assert.Empty(scene.Destroyed);
        scene.FailApply = false; scene.Host.Kill(process, 0x10); Assert.Equal(1, scene.Restores); Assert.Equal(new[] { id }, scene.Destroyed);
    }

    /// <summary>자연 종료의 복구가 실패하면 initialized0/Host.Kill에 아직 도달하지 않았다는 접두를 보존한다.</summary>
    [Fact]
    public void NaturalRestoreFailureRetainsProcessUntilFollowingRealKernelFrame()
    {
        var scene = new Scene(true) { FailRestore = true }; ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process);
        scene.Time.Value.Wall = 1;
        Assert.Equal("restore", Assert.Throws<InvalidOperationException>(scene.Kernel.RunFrame).Message);
        Assert.Equal(1, scene.Restores); Assert.Same(process, scene.Kernel.Get(id)); Assert.Contains(id, scene.Registered); Assert.Empty(scene.Destroyed);
        scene.FailRestore = false; scene.Kernel.RunFrame();
        Assert.Equal(2, scene.Restores); Assert.Equal(new[] { id }, scene.Destroyed); Assert.Null(scene.Kernel.Get(id));
    }

    /// <summary>명시 소멸 실패는 기존 Kernel.Remove의 부분 소유/삭제깊이를 숨기지 않는다. 이 격리 장면에 재정리 성공을 합성하지 않는다.</summary>
    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void DisposeFailurePreservesCancelThenCallbackPrefixAndKernelRemovalBoundary(bool failRestore)
    {
        var scene = new Scene(true) { FailRestore = failRestore, FailDestroyed = !failRestore }; ThunderProcess process = scene.Create(); scene.Attach(); uint id = scene.Kernel.Find(process);
        Assert.Equal(failRestore ? "restore" : "destroy", Assert.Throws<InvalidOperationException>(() => scene.Host.Kill(process, 0x10)).Message);
        Assert.Equal(1, scene.Restores); Assert.Equal(100, scene.Screen.Palette[228].Red);
        Assert.Equal(failRestore ? 0 : 1, scene.Destroyed.Count); Assert.Contains(id, scene.Registered);
        Assert.Same(process, scene.Kernel.Get(id)); Assert.NotEqual(0, scene.Pool.Slot(process.Form())[11] & RawSidPool.DeadBit); Assert.Equal(1u, scene.Destroy.PreDepth);
        // Core가 이미 반납 중인 소유권을 tests가 다시 채우거나 Form을 수동 해제하지 않는다. 모두 managed 독립 장면이다.
    }
}
