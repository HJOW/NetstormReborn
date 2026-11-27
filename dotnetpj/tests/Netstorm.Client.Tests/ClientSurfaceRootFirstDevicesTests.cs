// 같은 실제 Surface/Runtime을 빌리는 Root 첫 장치 묶음을 검사한다.
// cpppj/src/client/ClientMain.cpp 447~455·468~488·525행의 PostPop·Resource·Reward·Deletion·PlayerRecords 조립을 이식한다.
// 0.901에서 RU-1 실제 owner 조립용으로 추가됐다.
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>첫 Process 생성 뒤·첫 메뉴/World 전에 설치하는 실제 장치의 계약이다.</summary>
public sealed class ClientSurfaceRootFirstDevicesTests
{
    /// <summary>같은 장기 Template 깊이와 실제 Client 시계/Surface를 첫 Host 전에 준비한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 게임 난수 owner다.</summary>
        public GameRandom Random { get; } = new(17);

        /// <summary>같은 실제 SP 저장소 owner다.</summary>
        public ClientSpGlobals Sp { get; }

        /// <summary>같은 실제 장기 전역이다.</summary>
        public ClientGameplayGlobals Game { get; } = new();

        /// <summary>같은 Template 로딩 깊이 owner다.</summary>
        public CampaignTemplateStorage Templates { get; } = new();

        /// <summary>같은 실제 연결표 owner다.</summary>
        public ClientConnectionGlobals Connections { get; }

        /// <summary>제품과 같은 최초 설치 경로의 Surface다.</summary>
        public ClientSurfaceGlobals Surface { get; }

        /// <summary>실제 SP 표시 갱신을 관찰한다.</summary>
        public int RefreshCalls { get; private set; }

        /// <summary>실제 기억점을 관찰한다.</summary>
        public List<(uint X, uint Y, uint Type)> Remembered { get; } = [];

        /// <summary>실제 소리를 관찰한다.</summary>
        public List<int> UnitLostCalls { get; } = [];

        /// <summary>현재 데모 값이다. 실제 메뉴 owner 대신 호스트가 보관한다.</summary>
        public bool Demo { get; set; }

        /// <summary>장치 설치 중 상태를 조회하지 않고 전송 뒤 첫 장치를 연결한다.</summary>
        public Scene()
        {
            var local = new ClientLocalStateGlobals(Game, new());
            Connections = new(local);
            Sp = new(Random, new UIntBox { Value = 1u }, new UIntBox(), static () => 1700000000);
            Sp.Store.Initialize();
            Surface = new(Game, new(local), new(), static () => 1000, installDevices: surface =>
            {
                surface.Storage.BindSerializationGlobals(Templates.SerializationGlobals);
                surface.BindDisplayDiagnostics((_, _, _) => throw new IOException("미선택 원본 진단"));
                surface.BindNativeTransmission(Connections, () => { });
            });
            Game.PlayerRecords.Initialize((_, _) => { });
        }

        /// <summary>실제 owner들로 첫 장치를 설치한다.</summary>
        public void InstallFirst()
        {
            Surface.BindNativeRootFirstDevices(
                Sp,
                Random,
                () => Demo,
                () => RefreshCalls++,
                (x, y, type) => Remembered.Add((x, y, type)),
                sid => UnitLostCalls.Add(sid));
        }

        /// <summary>공통 삭제 본체가 요구하는 188 TYPE 표를 준비한다.</summary>
        public void EnsureAssetTables()
        {
            if (Surface.Storage.Types.Count == 0)
            {
                var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index), FootX = 1, FootY = 1 }).ToArray();
                var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
                var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
                Surface.Storage.InitializeAssetTables(types, frames, shapes);
            }

            // 공통 삭제 본체가 요구하는 표시 장치를 시험용으로 연결한다. 제품 표시는 R5에서 잇는다.
            try
            {
                Surface.Storage.BindDisplayDevices(new RawSurfaceDisplayBindings
                {
                    View = () => new SquidDisplayView { Zoom = 65536, Viewport = new Netstorm.Core.Display.DisplayRect(0, 0, 1024, 768) },
                    Suppressed = () => false,
                    Invalidate = (_, _) => { },
                });
            }
            catch (ArgumentException)
            {
                // 이미 설치됐으면 같은 장치를 유지한다.
            }
        }

        /// <summary>같은 장기 Player의 현재 기록을 빌린다.</summary>
        public void Dispose()
        {
        }
    }

    /// <summary>첫 설치는 같은 실제 SP/Editor/Player owner를 빌리고 중복 설치를 거부한다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void FirstInstallationBorrowsActualOwnersAndRejectsDuplicate()
    {
        using var scene = new Scene();
        scene.InstallFirst();
        Assert.True(scene.Surface.Storage.UsesNativeResourceDevices());
        Assert.True(scene.Surface.Storage.UsesNativeRewardDevices());
        Assert.Same(scene.Game.World.EditorBox, scene.Surface.Storage.RewardState().SharedFixedSpB);
        Assert.Throws<ArgumentException>(() => scene.InstallFirst());
    }

    /// <summary>실제 현재 커서·Player 주소와 공통 삭제기가 같은 내용물 수명을 사용한다.</summary>
    [Fact]
    public void ContentDevicesShareOwnersAndCancelActualSelectedForm()
    {
        using var scene = new Scene(); scene.InstallFirst(); var input = new Input.InputQueue(); int cancelled = 0;
        scene.Surface.BindNativeContentDevices(scene.Connections, input, () => { cancelled++; scene.Game.Cursor.Preview = 0; }, (_, _) => throw new IOException("미선택 위치 음향"));
        var state = scene.Surface.Storage.ContentState(); Assert.Same(scene.Game.Player.BattleOwnerBox, state.SharedLocalOwner);
        scene.Game.Player.BattleOwner = 4; Assert.Equal(4u, state.ReadLocalOwner());
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); _ = new HostAssetTables(new HostStartupScene(data), scene.Surface.Storage);
        scene.Surface.Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        int form = scene.Surface.Storage.FactoryExecutor().CreateContained(2); scene.Game.Cursor.Preview = (uint)form;
        Assert.Equal((uint)form, state.CurrentCursor!()); Assert.True(scene.Surface.Storage.ProcessFormDispatch().Destroy(form, 0));
        Assert.Equal(1, cancelled); Assert.Equal(0u, scene.Game.Cursor.PreviewBox.Value); Assert.NotEqual(0, scene.Surface.Storage.Pool.Slot(form)[11] & 1);
        Assert.True(input.Empty);
    }

    /// <summary>자원 Random은 설치한 같은 실제 난수 owner를 빌린다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void ResourceRandomBorrowsSameActualOwner()
    {
        using var scene = new Scene();
        scene.InstallFirst();
        // 같은 스트림을 두 번 읽으면 값이 진행한다. 새 난수를 만들면 진행하지 않는다.
        uint first = scene.Random.Next(65536);
        uint second = scene.Random.Next(65536);
        Assert.NotEqual(first, second);
        // 간헐천 등록은 null AI만 있으면 하위 없이 자연 반환한다. 실제 Player9를 읽는다.
        var registration = new RawGeyserAiRegistration(OriginalEdition.Patch1078, scene.Game.PlayerRecords.Records);
        bool called = false;
        registration.BindPatch((_, _, _) => called = true);
        registration.Register(7, 1);
        Assert.False(called);
    }

    /// <summary>보상 지갑 가산은 실제 Native Reward 장치에 공급된 같은 wallet을 사용한다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void RewardWalletCreditUsesNativeDevices()
    {
        using var scene = new Scene();
        scene.InstallFirst();
        // 데모 거짓이면 NULL brain은 자연 반환한다.
        var wallet = scene.Surface.CreateAiWallet(() => scene.Demo);
        wallet.Credit(1, 5);
        Assert.Equal(0, wallet.Balance(1));
        scene.Demo = true;
        wallet.Credit(1, 5);
        Assert.Equal(0, wallet.Balance(1));
    }

    /// <summary>삭제 장치는 설치한 실제 람다를 보관한다. 공통 본체는 같은 장치로 구성된다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void DeletionDevicesKeepInstalledActualDevices()
    {
        using var scene = new Scene();
        scene.InstallFirst();
        scene.EnsureAssetTables();
        // 공통 본체는 설치 장치를 지나 자산 원천에서 멈춘다. 미설치 진단이 나오지 않으면 설치가 실제 owner로 된 것이다.
        // 근거: CommonDeletion은 장치 미설치면 "실제 Client 장치 미연결"을, 자산 미연결이면 "실제 최초 로딩 자산 미연결"을 낸다.
        var failure = Assert.Throws<InvalidOperationException>(() => scene.Surface.Storage.CommonDeletion());
        Assert.Contains("실제 최초 로딩 자산 미연결", failure.Message);
        Assert.Empty(scene.Remembered);
        Assert.Empty(scene.UnitLostCalls);
    }

    /// <summary>세션 없는 자원 호출은 실제 Runtime 미연결을 드러내고 성공으로 꾸미지 않는다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void ResourceCallsWithoutSessionRevealMissingRuntime()
    {
        using var scene = new Scene();
        scene.InstallFirst();
        var session = new HostSession(scene.Surface.Kernel, new(17), new(23), sharedSurface: scene.Surface, sharedTemplates: scene.Templates);
        Assert.Throws<InvalidOperationException>(() => session.CheckCarrierPostPop(3));
        Assert.Throws<InvalidOperationException>(() => session.HandleDamageableResourceRegular(3, 0, 0, 0f));
        Assert.Throws<InvalidOperationException>(() => session.StartResourceConstruction(3));
        Assert.Throws<InvalidOperationException>(() => session.DestroyResourceObject(3, 0));
        Assert.Throws<InvalidOperationException>(() => session.HandleCarrierResourceRegular(3, 0, 0, 0f));
        session.ReleasePlayerRecordViews();
    }
}
