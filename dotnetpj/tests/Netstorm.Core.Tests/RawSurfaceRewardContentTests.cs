// Native 장기 HP/보상 장치와 Content 수명 본체의 장기 수명 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>보상 장치 설치·공유 주소·보상 본체·수명 결합·분배기 Content 등록의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceRewardContentTests
{
    /// <summary>실제 최초 자산 표를 빌리는 독립 원천이다.</summary>
    private sealed class StubSource(System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> types) : Netstorm.Core.Original.IRawSurfaceAssetSource
    {
        /// <summary>실제 최초 로딩 표다.</summary>
        public System.Collections.Generic.IReadOnlyList<Netstorm.Core.Original.OriginalTypeRecord> LoadedTypes { get; } = types;
    }

    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static Netstorm.Core.Original.RawSurfaceStorage Storage(bool withSource = false)
    {
        var types = Enumerable.Range(0, 188).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = Netstorm.Core.Original.OriginalConstructors.Address(index),
            Flags1 = Netstorm.Core.Original.OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        Netstorm.Core.Original.IRawSurfaceAssetSource? source = withSource ? new StubSource(types) : null;
        var storage = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Patch1078, types, frames, shapes, assetSource: source);
        storage.GameAux().SquidTransmission.Compress = 0;
        storage.GameAux().SquidTransmission.ClientSidAllowed = 1;
        // 기본 삭제기의 Unpop/표시 경로가 같은 장기 뷰를 읽으므로 독립 표시 장치를 먼저 연결한다.
        storage.BindDisplayDevices(new Netstorm.Core.Original.RawSurfaceDisplayBindings
        {
            View = () => new Netstorm.Core.Original.SquidDisplayView { Zoom = 65536, Viewport = new Netstorm.Core.Display.DisplayRect(0, 0, 1024, 768) },
            Suppressed = () => false,
            Invalidate = (_, _) => { },
        });
        return storage;
    }

    /// <summary>실제 방송 장치만 있는 Native 전송 묶음이다.</summary>
    private static void BindTransmission(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindTransmissionDevices(new Netstorm.Core.Original.RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
    }

    /// <summary>실제 별칭 주소와 네 효과를 묶은 Content 장치다.</summary>
    private static void BindContent(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindContentDevices(new Netstorm.Core.Original.RawSurfaceContentDevices
        {
            Cursor = new Netstorm.Core.Original.UIntBox() { Value = 0 },
            LocalOwner = new Netstorm.Core.Original.UIntBox() { Value = 1 },
            ContentType = new Netstorm.Core.Original.UIntBox() { Value = 6 },
            Effects = new Netstorm.Core.Original.ContentLifetimeHooks
            {
                CancelCursor = () => { },
                LocalNotice = _ => { },
                RemoteNotice = _ => { },
                Sound = (_, _) => { },
            },
        });
    }

    /// <summary>실제 현재 선택/해제 부모만 있는 기본 선택 묶음이다.</summary>
    private static void BindProcess(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindProcessDevices(new Netstorm.Core.Original.RawSurfaceProcessDevices
        {
            Selected = () => 0u,
            ClearSelection = () => { },
        });
    }

    /// <summary>실제 SP 저장소와 editor 상자를 묶은 보상 장치다.</summary>
    private static Netstorm.Core.Original.RawSurfaceRewardDevices RewardDevices(
        Netstorm.Core.Original.RawSpStore? store = null,
        Netstorm.Core.Original.UIntBox? editor = null) => new()
        {
            Sp = store ?? new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u),
            Editor = editor ?? new Netstorm.Core.Original.UIntBox(),
            RefreshSp = () => { },
            CreditWallet = (_, _) => { },
        };

    /// <summary>실제 방송 장치 결합과 최초 소유자 상태에서만 보상 설치를 허용한다.</summary>
    [Fact]
    public void BindRewardRequiresNativeAndFourDevices()
    {
        var storage = Storage();
        // 전송 장치 없이 보상 장치를 설치하면 거부한다.
        Assert.False(storage.UsesNativeRewardDevices());
        Assert.Throws<ArgumentException>(() => storage.BindRewardDevices(RewardDevices()));
        BindTransmission(storage);
        storage.BindRewardDevices(RewardDevices());
        Assert.True(storage.UsesNativeRewardDevices());
        // 두 번째 설치는 같은 장기 수명을 바꾸지 않는다.
        Assert.Throws<ArgumentException>(() => storage.BindRewardDevices(RewardDevices()));
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        var missing = Storage();
        BindTransmission(missing);
        Assert.Throws<ArgumentException>(() => missing.BindRewardDevices(new Netstorm.Core.Original.RawSurfaceRewardDevices()));
        // CD 판본은 보상 설치를 거부한다.
        var types = Enumerable.Range(0, 171).Select(index => new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = 0,
            Flags1 = 0,
            MaxHitPoints = 1,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 171).Select(_ => new Netstorm.Core.Original.SquidDisplayShape()).ToArray();
        var cd = new Netstorm.Core.Original.RawSurfaceStorage(Netstorm.Core.Original.OriginalEdition.Cd1072, types, frames, shapes);
        Assert.Throws<ArgumentException>(() => cd.BindRewardDevices(RewardDevices()));
        // 이미 빌린 물리에서는 설치하지 않는다.
        var borrowed = Storage();
        BindTransmission(borrowed);
        using var lease = borrowed.AcquirePrepared();
        Assert.Throws<ArgumentException>(() => borrowed.BindRewardDevices(RewardDevices()));
    }

    /// <summary>보상 상태는 실제 SP owner 없이 조회하면 거부한다.</summary>
    [Fact]
    public void RewardStateRequiresSpOwner()
    {
        var storage = Storage();
        BindTransmission(storage);
        // 보상 장치 없이 상태를 요구하면 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.RewardState());
        storage.BindRewardDevices(RewardDevices());
        Netstorm.Core.Original.SquidRewardState first = storage.RewardState();
        Assert.Same(first, storage.RewardState());
    }

    /// <summary>보상 상태는 같은 장기 Percent·자원·모드·TYPE 주소를 별칭으로 읽는다.</summary>
    [Fact]
    public void RewardStateSharesLongTermAddresses()
    {
        var storage = Storage();
        BindTransmission(storage);
        var editor = new Netstorm.Core.Original.UIntBox() { Value = 0 };
        var store = new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(7), () => 7u);
        storage.BindRewardDevices(RewardDevices(store, editor));
        Netstorm.Core.Original.SquidRewardState state = storage.RewardState();
        // 같은 GameAux 보상 비율/자원 주소를 빌린다.
        Assert.Same(storage.GameAux().RefundPercentBox, state.SharedPercent);
        Assert.Same(storage.GameAux().ResourcePoolBox, state.SharedGeyserPool);
        storage.GameAux().RefundPercent = 30;
        Assert.Equal(30u, state.ReadPercent());
        storage.GameAux().ResourcePool = -5;
        Assert.Equal(-5, state.ReadGeyserPool());
        // 같은 OwnerMode fort/battle을 현재값으로 읽는다.
        storage.OwnerMode.Fort = true;
        storage.OwnerMode.Battle = true;
        Assert.True(state.ReadFixedSpA());
        Assert.True(state.ReadHalfHealth());
        // editor 상자는 0이 아니면 고정으로 읽는다.
        Assert.False(state.ReadFixedSpB());
        editor.Value = 1;
        Assert.True(state.ReadFixedSpB());
        // 같은 GameAux TYPE 주소를 빌린다.
        Assert.Same(storage.GameAux().PriestTypeBox, state.SharedPriestType);
        Assert.Same(storage.GameAux().AltarTypeBox, state.SharedAltarType);
        Assert.Same(storage.GameAux().RainBlockerTypeBox, state.SharedSilentType);
        Assert.Same(storage.GameAux().GrowingRainBlockerTypeBox, state.SharedAliasType);
        Assert.Same(storage.GameAux().NuggetTypeBox, state.SharedNuggetType);
        Assert.Equal(158u, state.ReadPriestType());
        // 진단 장치는 같은 장기 checking을 따른다.
        Assert.True(state.Checking.Read() == storage.Checking().Read());
    }

    /// <summary>보상 본체는 실제 최초 자산 owner 뒤에만 만들고 재사용한다.</summary>
    [Fact]
    public void RewardRequiresSourceAndReuses()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindRewardDevices(RewardDevices());
        // 최초 자산 owner 없이 보상 본체를 요구하면 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.Reward());
        var sourced = Storage(withSource: true);
        BindTransmission(sourced);
        sourced.BindRewardDevices(RewardDevices());
        Netstorm.Core.Original.RawSquidReward first = sourced.Reward();
        Assert.Same(first, sourced.Reward());
        Assert.Same(sourced.Pool, first.Pool);
    }

    /// <summary>수명 본체는 같은 정렬·삭제·상태·보상을 한 번 연결하고 재사용한다.</summary>
    [Fact]
    public void ContentLifetimeBindsRewardAndReuses()
    {
        var sourced = Storage(withSource: true);
        BindTransmission(sourced);
        BindContent(sourced);
        sourced.BindRewardDevices(RewardDevices());
        Netstorm.Core.Original.RawContentLifetime first = sourced.ContentLifetime();
        Assert.Same(first, sourced.ContentLifetime());
        Assert.Same(sourced.Pool, first.Pool);
        // 보상 장치 없이 수명을 요구하면 보상 본체에서 거부한다.
        var noReward = Storage(withSource: true);
        BindTransmission(noReward);
        BindContent(noReward);
        Assert.Throws<InvalidOperationException>(() => noReward.ContentLifetime());
    }

    /// <summary>분배기는 World 없이 같은 Host·의존·Gump·Content 수명을 결합하고 재사용한다.</summary>
    [Fact]
    public void ProcessDispatchBindsContentLifetimeAndReuses()
    {
        var sourced = Storage(withSource: true);
        BindTransmission(sourced);
        BindProcess(sourced);
        BindContent(sourced);
        sourced.BindRewardDevices(RewardDevices());
        Netstorm.Core.Original.RawSquidFormDispatch first = sourced.ProcessFormDispatch();
        Assert.Same(first, sourced.ProcessFormDispatch());
        Assert.Same(sourced.Pool, first.Pool);
        // 보상 없이 분배기를 요구하면 수명 경유에서 거부한다.
        var noReward = Storage(withSource: true);
        BindTransmission(noReward);
        BindProcess(noReward);
        BindContent(noReward);
        Assert.Throws<InvalidOperationException>(() => noReward.ProcessFormDispatch());
    }
}
