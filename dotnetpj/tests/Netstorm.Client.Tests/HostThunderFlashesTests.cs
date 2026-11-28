using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Player;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 Surface의 공개 장치/할당 API와 현재 Player 기록만 사용한다. 창과 별도 게임 실행은 하지 않는다.
namespace Netstorm.Client.Tests;

/// <summary>Game 번개 부모의 DWORD·현재 owner·캐시 시각·같은 Host/Kernel 반납을 검사한다.</summary>
public sealed class HostThunderFlashesTests
{
    /// <summary>Game의 현재 장기 물리/실제 자산/Native 기본 ProcessForm 장치를 같은 경로로 준비한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제 장기 Player/CoreData를 소유한다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 실제 Kernel이다.</summary>
        public readonly Kernel Kernel = new();
        /// <summary>동일 Frame.Wall 캐시다.</summary>
        public readonly ClientClockGlobals Clock;
        /// <summary>실제 같은 Surface/Pool/Process Host다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>실제 기본 직렬화 소유자다.</summary>
        public readonly CampaignTemplateStorage Templates = new();
        /// <summary>창 없는 현재/논리/저장 팔레트다.</summary>
        public readonly Screen Screen = new(8, 4);
        /// <summary>실제 보호 사본의 TYPE/프레임/셰이프다.</summary>
        public readonly HostAssetTables Assets;
        /// <summary>같은 작은 논리 화면의 실제 dirty 출력기다. Form 삭제의 공통 표시 후단도 이것을 사용한다.</summary>
        public readonly Renderer Renderer;
        /// <summary>현재 카메라와 이 검사 화면의 실제 지도 영역을 공통 삭제기에 제공한다.</summary>
        public readonly ClientRendererDevices RendererDevices;
        /// <summary>부모 raw 입력은 실제 공개 공장으로 생성한다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>같은 Game 부착/취소 helper다.</summary>
        public readonly HostThunderFlashes Flashes;
        /// <summary>현재 기술 기록이 가리키는 실제 void 부모다.</summary>
        public readonly int Parent;

        /// <summary>실제 Native 전송/공장/삭제 기본 장치와 자산을 준비하며 NativeLoaded/Session은 만들지 않는다.</summary>
        public Scene(bool fullScreen = true)
        {
            var local = new ClientLocalStateGlobals(Game, new()); var connections = new ClientConnectionGlobals(local); Clock = new(local);
            Surface = new(Game, Clock, Kernel, () => throw new IOException("번개에서 동적 밀리초 조회 금지"), installDevices: owner =>
            {
                owner.Storage.BindSerializationGlobals(Templates.SerializationGlobals);
                owner.BindNativeTransmission(connections, () => Game.Selection.Selected.Value = 0);
                owner.Storage.BindGraphTrace((level, text) => ErrorLog.Note($"번개 검사 원본 로그 {level}: {text}"));
                owner.BindDisplayDiagnostics(ErrorLog.RecordAssertion);
            });
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
            var startup = new HostStartupScene(data); Assets = new(startup, Surface.Storage);
            // 실제 공통 Form 삭제기가 빌리는 SP/보상/내용물 owner를 Game과 같은 최초 설치 경로로 연결한다.
            var sp = new ClientSpGlobals(startup.Random, new UIntBox { Value = 1 }, new UIntBox(), () => 1760000000);
            sp.Store.Initialize();
            Surface.BindNativeRootFirstDevices(sp, startup.Random, () => false,
                () => throw new IOException("번개에서 미선택 SP 표시"),
                (_, _, _) => throw new IOException("번개에서 미선택 배치 기억"),
                _ => throw new IOException("번개에서 미선택 유닛 소리"));
            Surface.BindNativeContentDevices(connections, new InputQueue(),
                () => throw new IOException("번개에서 미선택 내용물 커서"),
                (_, _) => throw new IOException("번개에서 미선택 내용물 소리"));
            Renderer = new(Screen.Width, Screen.Height);
            RendererDevices = new(Game, Surface, startup.Cloud, () => Renderer,
                () => new ScreenRect(0, 0, Screen.Width, Screen.Height));
            RendererDevices.InitializeCamera(Screen.Width, Screen.Height); RendererDevices.BindSurfaceDisplay();
            Factory = new(Surface.Storage.Pool, Surface.Storage.Types);
            Parent = Factory.Create(82, SidAllocation.Client);
            Screen.SetMode(ScreenMode.BackBufferDib | (fullScreen ? ScreenMode.FullScreen : 0));
            Palette(100); Clock.Frame.Value.Wall = 100;
            ClientPlayerRuntime.Initialize(Game);
            Game.Player.PlayerKnown = 1; Game.Player.BattleOwnerBox.Value = 1; Technology(1, unchecked((uint)Parent));
            Flashes = new(Surface, Screen, Kernel, Clock);
        }

        /// <summary>현재 CoreData가 빌리는 Player 행의 기술DWORD만 공개 Memory에 작성한다.</summary>
        public void Technology(uint owner, uint parent)
            => BinaryPrimitives.WriteUInt32LittleEndian(Game.CoreData.Players.Span.Slice(checked((int)owner * 180 + 0x9c), 4), parent);

        /// <summary>명시 입력의 저장 팔레트를 실제 기존 로더에 전달한다. 기대 번개 결과는 계산하지 않는다.</summary>
        public void Palette(byte value)
        {
            byte[] bytes = Enumerable.Repeat(value, 1024).ToArray(); Screen.LoadPalette(new GamePalette(bytes));
        }

        /// <summary>실제 등록 프로세스가 하나인 경우 same Kernel에서 그 객체를 읽는다.</summary>
        public ThunderProcess OnlyFlash()
        {
            ThunderProcess[] found = Enumerable.Range(1, checked((int)KernelShape.ProcessCapacity))
                .Select(id => Kernel.Get((uint)id)).OfType<ThunderProcess>().ToArray();
            return Assert.Single(found);
        }

        /// <summary>화면이 살아 있는 동안 실제 취소/Kernel 반납을 먼저 완료한다.</summary>
        public void Dispose()
        {
            try { Flashes.Clear(); }
            finally { Kernel.Clear(); Templates.ClearAll(); }
        }
    }

    /// <summary>Known0은 빈 Player 물리·Challenge보다 먼저 반환하고 선택된 미복원/물리 오류는 감추지 않는다.</summary>
    [Fact]
    public void UnknownPlayerSkipsRecordAndChallengeButSelectedMissingBoundaryFails()
    {
        using var scene = new Scene(); scene.Game.CoreData.Players = Memory<byte>.Empty; scene.Game.Player.PlayerKnown = 0;
        scene.Surface.Storage.OwnerMode.Challenge = true;
        uint free = scene.Surface.Storage.Pool.FreeCount; scene.Flashes.Request(); Assert.Equal(0, scene.Flashes.RegisteredCount);
        scene.Game.Player.PlayerKnown = 1;
        Assert.Throws<InvalidOperationException>(() => scene.Flashes.Request());
        scene.Surface.Storage.OwnerMode.Challenge = false;
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Flashes.Request());
        Assert.Equal(free, scene.Surface.Storage.Pool.FreeCount); Assert.Equal(0, scene.Kernel.Size());
    }

    /// <summary>기술부모의 상위16BIT를 버리지 않고 전체DWORD/현재 행·현재Player view를 매 요청 읽는다.</summary>
    [Fact]
    public void CurrentTechnologyUsesFullDwordAndCurrentOwnerRecord()
    {
        using var scene = new Scene(); scene.Technology(1, 0x10000u | unchecked((uint)scene.Parent));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Flashes.Request()); Assert.Equal(0, scene.Flashes.RegisteredCount);
        scene.Technology(1, 0); scene.Technology(2, unchecked((uint)scene.Parent));
        scene.Flashes.Request(); Assert.Equal(0, scene.Flashes.RegisteredCount);
        scene.Game.Player.BattleOwnerBox.Value = 2; scene.Flashes.Request(); Assert.Equal(1, scene.Flashes.RegisteredCount);
        ThunderProcess process = scene.OnlyFlash(); Assert.Equal(scene.Parent, process.Parent());
        Assert.Equal((byte)55, scene.Surface.Storage.Pool.Slot(process.Form())[10]);
        Assert.Same(process, scene.Surface.Processes.Host().FromForm(process.Form()));
    }

    /// <summary>실제 cached Frame.Wall과 동일 Kernel이 진행하고 Clear는 최신 saved로 복구한 뒤 Form/PID를 반납한다.</summary>
    [Fact]
    public void ActualSurfaceHostAndCachedFrameOwnFlashUntilClearRestoresLatestSavedPalette()
    {
        using var scene = new Scene(); scene.Flashes.Request(); ThunderProcess process = scene.OnlyFlash();
        uint pid = scene.Kernel.Find(process); int form = process.Form();
        Assert.Equal(scene.Parent, RawContainedLink.CommonContainedParent(scene.Surface.Storage.Pool, form));
        scene.Clock.Frame.Value.Wall = 100.01; scene.Kernel.RunFrame();
        Assert.NotEqual((byte)100, scene.Screen.Palette[230].Red);
        scene.Palette(29); scene.Clock.Frame.Value.Wall = 100.02; scene.Kernel.RunFrame();
        scene.Flashes.Clear(); Assert.Equal((byte)29, scene.Screen.Palette[230].Red);
        Assert.Equal(0, scene.Flashes.RegisteredCount); Assert.Null(scene.Kernel.Get(pid));
        Assert.Equal(1, scene.Surface.Storage.Pool.Slot(form)[11] & RawSidPool.FreeBit);
        scene.Screen.SetPalette(230, 1, [new ScreenColor(77, 77, 77, 77)], true);
        scene.Flashes.Clear(); Assert.Equal((byte)77, scene.Screen.Palette[230].Red);
    }

    /// <summary>실제 상태반납 Kernel.Clear도 Dispose를 통해 등록을 지운다. 남은 Form 물리 정리는 별도 State 부모다.</summary>
    [Fact]
    public void ActualKernelClearUsesSameDisposeAndRegistrationCallback()
    {
        using var scene = new Scene(); scene.Flashes.Request(); uint pid = scene.Kernel.Find(scene.OnlyFlash());
        scene.Clock.Frame.Value.Wall = 100.01; scene.Kernel.RunFrame(); scene.Kernel.Clear();
        Assert.Null(scene.Kernel.Get(pid)); Assert.Equal(0, scene.Flashes.RegisteredCount); Assert.Equal((byte)100, scene.Screen.Palette[230].Red);
        scene.Flashes.Clear();
    }

    /// <summary>free/dead 기술부모는 실제 Attach의 조용한 거부이며 프로세스를 Kernel 등록 성공으로 만들지 않는다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void ActualAttachRejectsFreeOrDeadParentWithoutRegistration(bool dead)
    {
        using var scene = new Scene();
        if (dead) scene.Surface.Storage.Pool.AllocatedBytes(scene.Parent)[11] |= RawSidPool.DeadBit;
        else scene.Surface.Storage.Pool.Release(scene.Parent);
        uint free = scene.Surface.Storage.Pool.FreeCount; scene.Flashes.Request();
        Assert.Equal(0, scene.Flashes.RegisteredCount); Assert.Equal(0, scene.Kernel.Size()); Assert.Equal(free, scene.Surface.Storage.Pool.FreeCount);
    }

    /// <summary>표시 모드를 바꾸기 전 Clear하면 SetMode가 임시 번개색을 저장하지 않는다. 실제 창 전환을 실행한 증거는 아니다.</summary>
    [Fact]
    public void ClearBeforeSetModeKeepsRestoredColorsInModeSnapshot()
    {
        using var scene = new Scene(); scene.Flashes.Request(); scene.Clock.Frame.Value.Wall = 100.01; scene.Kernel.RunFrame();
        Assert.NotEqual((byte)100, scene.Screen.Palette[230].Red);
        scene.Flashes.Clear(); scene.Screen.SetMode(ScreenMode.BackBufferDib);
        scene.Screen.SetPalette(230, 1, [new ScreenColor(77, 77, 77, 77)], true);
        scene.Screen.SetPalette(0, 256, null, true); Assert.Equal((byte)100, scene.Screen.Palette[230].Red);
    }

    /// <summary>다른 Kernel/Frame 소유자는 실제 부착/시각/팔레트 변화 전에 거부한다.</summary>
    [Fact]
    public void MixedKernelOrClockIsRejectedBeforeAnyFlash()
    {
        using var scene = new Scene();
        Assert.Throws<ArgumentException>(() => new HostThunderFlashes(scene.Surface, scene.Screen, new(), scene.Clock));
        Assert.Throws<ArgumentException>(() => new HostThunderFlashes(scene.Surface, scene.Screen, scene.Kernel,
            new(new ClientLocalStateGlobals(scene.Game, new()))));
        Assert.Equal(0, scene.Flashes.RegisteredCount); Assert.Equal((byte)100, scene.Screen.Palette[230].Red);
    }
}
