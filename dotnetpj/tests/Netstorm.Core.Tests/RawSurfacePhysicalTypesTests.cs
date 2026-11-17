// 같은 장기 TYPE 목록의 실제 물리 배열 미러와 보상 별칭 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>물리 배열 미러의 참조 공유·교체 반영·보상 별칭 계약을 확인한다.</summary>
public sealed class RawSurfacePhysicalTypesTests
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

    /// <summary>실제 SP 저장소와 editor 상자를 묶은 보상 장치다.</summary>
    private static void BindReward(Netstorm.Core.Original.RawSurfaceStorage storage)
    {
        storage.BindRewardDevices(new Netstorm.Core.Original.RawSurfaceRewardDevices
        {
            Sp = new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u),
            Editor = new Netstorm.Core.Original.UIntBox(),
            RefreshSp = () => { },
            CreditWallet = (_, _) => { },
        });
    }

    /// <summary>같은 개수·순서로 실제 원소 참조를 배열 미러에 복사한다.</summary>
    [Fact]
    public void MirrorSharesReferences()
    {
        var storage = Storage();
        Netstorm.Core.Original.OriginalTypeRecord[] mirror = storage.PhysicalTypes();
        Assert.Equal(storage.Types.Count, mirror.Length);
        // 각 원소가 같은 참조인지 확인한다.
        for (int index = 0; index < mirror.Length; index++)
        {
            Assert.Same(storage.Types[index], mirror[index]);
        }
    }

    /// <summary>원소 교체는 다음 동기화 뒤 미러에 반영된다.</summary>
    [Fact]
    public void MirrorReflectsReplacement()
    {
        var storage = Storage();
        Netstorm.Core.Original.OriginalTypeRecord[] first = storage.PhysicalTypes();
        var replaced = new Netstorm.Core.Original.OriginalTypeRecord
        {
            ConstructorAddress = 1,
            Flags1 = 0,
            MaxHitPoints = 5,
            FootX = 2,
            FootY = 2,
        };
        storage.Types[74] = replaced;
        Netstorm.Core.Original.OriginalTypeRecord[] second = storage.PhysicalTypes();
        // 같은 개 것이면 같은 배열 객체를 재사용한다.
        Assert.Same(first, second);
        Assert.Same(replaced, second[74]);
    }

    /// <summary>대여·반납은 미러 주소를 바꾸지 않는다.</summary>
    [Fact]
    public void MirrorSurvivesAcquire()
    {
        var storage = Storage();
        Netstorm.Core.Original.OriginalTypeRecord[] before = storage.PhysicalTypes();
        using var lease = storage.AcquirePrepared();
        Netstorm.Core.Original.OriginalTypeRecord[] during = storage.PhysicalTypes();
        Assert.Same(before, during);
    }

    /// <summary>보상 본체는 물리 배열 미러로 현재 TYPE를 별칭한다.</summary>
    [Fact]
    public void RewardBindsMirror()
    {
        var storage = Storage(withSource: true);
        BindTransmission(storage);
        BindReward(storage);
        Netstorm.Core.Original.RawSquidReward reward = storage.Reward();
        Assert.Same(reward, storage.Reward());
        // 미러 경유의 현재 HP 조회가 같은 물리를 읽는다.
        Assert.Equal(1001, reward.TypeHitPoints(74));
        // 원소 속성 변경은 같은 참조라 보상에도 보인다.
        storage.Types[74].MaxHitPoints = 777;
        Assert.Equal(777, reward.TypeHitPoints(74));
    }

    /// <summary>보상 현재 TYPE 계약은 배열/뷰만 허용하고 List를 거부한다.</summary>
    [Fact]
    public void RewardStillRejectsList()
    {
        var pool = new Netstorm.Core.Original.RawSidPool(24000, true);
        var records = Enumerable.Range(0, 188).Select(_ => new Netstorm.Core.Original.OriginalTypeRecord()).ToArray();
        var reward = new Netstorm.Core.Original.RawSquidReward(
            pool, records, new Netstorm.Core.Original.RawPostPopState(),
            new Netstorm.Core.Original.SquidRewardState(),
            new Netstorm.Core.Original.RawSpStore(new Netstorm.Core.Original.GameRandom(1), () => 1u));
        Assert.Throws<System.ArgumentException>(() => reward.BindCurrentTypes(records.ToList()));
        reward.BindCurrentTypes(records);
    }

    /// <summary>미러가 없으면 보상 본체를 만들 수 없고 있으면 재사용한다.</summary>
    [Fact]
    public void RewardRequiresSourceEvenWithMirror()
    {
        var storage = Storage();
        BindTransmission(storage);
        BindReward(storage);
        // 원천 없이 미러만으로는 보상 본체를 만들지 않는다.
        Assert.Throws<System.InvalidOperationException>(() => storage.Reward());
        Assert.NotEmpty(storage.PhysicalTypes());
    }
}
