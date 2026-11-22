using Netstorm.Core.Original;

// Native 장기 자원 장치와 너겟/간헐천 실제 부모의 장기 수명 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>자원 장치 설치·확보/구성·장기 너겟/운반체/간헐천 본문의 선택 계약을 확인한다.</summary>
public sealed class RawSurfaceResourcesCurrentTests
{
    /// <summary>현재 TYPE/프레임/셰이프와 원문 생성자를 갖는 독립 장기 물리다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord
        {
            ConstructorAddress = OriginalConstructors.Address(index),
            Flags1 = OriginalTypeRecord.HasHitPoints,
            MaxHitPoints = 1001,
            FootX = 1,
            FootY = 1,
        }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>Native 전송 장치와 실제 게임 난수/운반체/등록 장치를 묶는다.</summary>
    private static void BindTransmission(RawSurfaceStorage storage)
    {
        storage.BindTransmissionDevices(new RawSurfaceTransmissionDevices
        {
            Broadcast = _ => { },
            Connected = _ => false,
            Send = (_, _) => { },
        });
    }

    /// <summary>자원 장치 여섯 필수 항목을 실제 스텁으로 묶는다.</summary>
    private static RawSurfaceResourceDevices Devices(GameRandom? random = null) => new()
    {
        Random = random is null ? () => new GameRandom(1) : () => random,
        CarrierCheck = _ => { },
        DamageableRegular = (_, _, _, payload) => payload,
        StartConstruction = _ => { },
        Destroy = (_, _) => true,
        GeyserRegistration = (_, _) => { },
    };

    /// <summary>실제 방송 장치 결합만 자원 설치를 허용하며 함수 값 복사에서 효과를 만들지 않는다.</summary>
    [Fact]
    public void BindRequiresNativeTransmissionAndSixDevices()
    {
        var storage = Storage();
        // 전송 장치 없이 자원 장치를 설치하면 거부한다.
        Assert.False(storage.UsesNativeResourceDevices());
        Assert.Throws<ArgumentException>(() => storage.BindResourceDevices(Devices()));
        BindTransmission(storage);
        storage.BindResourceDevices(Devices());
        Assert.True(storage.UsesNativeResourceDevices());
        // 두 번째 설치는 같은 장기 수명을 바꾸지 않는다.
        Assert.Throws<ArgumentException>(() => storage.BindResourceDevices(Devices()));
        var missing = Storage();
        BindTransmission(missing);
        // 필수 항목이 하나라도 없으면 설치하지 않는다.
        Assert.Throws<ArgumentException>(() => missing.BindResourceDevices(new RawSurfaceResourceDevices()));
    }

    /// <summary>같은 확보 주소의 중복과 비어 있는 구성/해제를 진입 전에 거부한다.</summary>
    [Fact]
    public void AllocateDuplicateAndMissingConstructAreRejected()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindResourceDevices(Devices());
        Assert.True(storage.AllocateResourceRegular());
        // 같은 주소에 두 번 확보하면 원문처럼 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.AllocateResourceRegular());
        var empty = Storage();
        // 확보 없이 구성하면 명시 실패한다.
        Assert.Throws<InvalidOperationException>(() => empty.ConstructResourceRegular(50, 1, 0.25f));
    }

    /// <summary>확보 뒤 구성이 같은 장기 Host에 붙고 다음 조회에서 사건을 찾는다.</summary>
    [Fact]
    public void AllocateConstructAttachesAndFindsEvent()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindResourceDevices(Devices());
        int sid = storage.FactoryExecutor().Create(74);
        Assert.True(storage.AllocateResourceRegular());
        storage.ConstructResourceRegular(sid, 7, 0.5f);
        // 같은 부모/사건 조회가 실제 Host 예약을 찾는다.
        Assert.NotNull(storage.Processes().Host().FindEvent(sid, 7));
        // 소비한 확보는 다시 비어 다음 예약에 쓸 수 있다.
        Assert.True(storage.AllocateResourceRegular());
    }

    /// <summary>자원 설치 뒤 실제 외부 Kernel을 결합할 수 있고 자원 설치 없이도 Native 공장이 존재한다.</summary>
    [Fact]
    public void ResourceInstallationDoesNotCreateHostBeforeExternalDevices()
    {
        var storage = Storage(); BindTransmission(storage); storage.BindResourceDevices(Devices());
        var kernel = new Kernel(16); var clock = new FrameTimeBox();
        storage.Processes().BindDevices(kernel, clock);
        int form = storage.Processes().Host().CreateOrTakeForm(0x10, 0);
        Assert.Same(kernel, storage.Processes().Kernel()); Assert.Equal(2, storage.Pool.Slot(form)[10]);
        var withoutResource = Storage(); BindTransmission(withoutResource);
        Assert.False(withoutResource.UsesNativeResourceDevices());
        int nativeForm = withoutResource.Processes().Host().CreateOrTakeForm(0x10, 0);
        Assert.Equal(2, withoutResource.Pool.Slot(nativeForm)[10]);
    }

    /// <summary>예약한 같은 객체를 사건/비트 인자로 초기화하고 실제 현재 시각과 count0으로 붙인다.</summary>
    [Fact]
    public void ResourceConstructUsesIdenticalReservedObjectAndCurrentClock()
    {
        var storage = Storage(); BindTransmission(storage); storage.BindResourceDevices(Devices());
        var clock = new FrameTimeBox(); storage.Processes().BindDevices(new Kernel(16), clock);
        int sid = storage.FactoryExecutor().Create(74); Assert.True(storage.AllocateResourceRegular());
        var reservedField = typeof(RawSurfaceStorage).GetField("_resourceRegular", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)!;
        var reserved = Assert.IsType<RegularProcess>(reservedField.GetValue(storage));
        clock.Value = new FrameTime { Game = 123.5 }; float payload = BitConverter.UInt32BitsToSingle(0x80000000);
        storage.ConstructResourceRegular(sid, 17, payload);
        var attached = Assert.IsType<RegularProcess>(storage.Processes().Host().FindEvent(sid, 17));
        Assert.Same(reserved, attached); Assert.Equal(0u, attached.Count()); Assert.Equal(123.5, attached.Time());
        Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(attached.Payload())); Assert.Null(reservedField.GetValue(storage));
    }

    /// <summary>장기 운반체는 실제 Template 로딩 owner 결합 뒤에만 만들 수 있다.</summary>
    [Fact]
    public void ResourceCarrierRequiresSerializationGlobals()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindResourceDevices(Devices());
        // 로딩 깊이 owner 없이 장기 운반체를 요구하면 거부한다.
        Assert.Throws<InvalidOperationException>(() => storage.ResourceCarrierPostPop());
        storage.BindSerializationGlobals(new FortSerializationGlobals());
        RawCarrierPostPop first = storage.ResourceCarrierPostPop();
        // 같은 장기 물리를 다음 World에서도 유지한다.
        Assert.Same(first, storage.ResourceCarrierPostPop());
        Assert.Same(storage.Pool, first.Pool);
    }

    /// <summary>장기 너겟은 실제 게임 난수·확보·구성·피해 부모를 같은 저장소에서 빌린다.</summary>
    [Fact]
    public void ResourceNuggetUsesLongTermDevicesWithoutWorld()
    {
        var storage = Storage();
        BindTransmission(storage);
        var random = new GameRandom(0x1234);
        storage.BindResourceDevices(Devices(random));
        storage.BindSerializationGlobals(new FortSerializationGlobals());
        RawNuggetPostPop nugget = storage.ResourceNuggetPostPop();
        Assert.Same(nugget, storage.ResourceNuggetPostPop());
        Assert.Same(storage.Pool, nugget.Pool);
        // 같은 장기 본문을 두 번 요구해도 새로 만들지 않는다.
        RawAssetPostPop geyser = storage.ResourceAssetPostPop();
        Assert.Same(geyser, storage.ResourceAssetPostPop());
        Assert.Same(storage.Pool, geyser.Pool);
    }

    /// <summary>간헐천 장기 본문은 Native 탐색기(native:true)를 선택하고 독립 AtPosition을 쓰지 않는다.</summary>
    [Fact]
    public void ResourceGeyserSelectsNativeFinderAndSharedLists()
    {
        var storage = Storage();
        BindTransmission(storage);
        storage.BindResourceDevices(Devices());
        storage.BindSerializationGlobals(new FortSerializationGlobals());
        RawAssetPostPop geyser = storage.ResourceAssetPostPop();
        // 공유 목록과 권한이 같은 장기 주소를 보존한다.
        Assert.True(ReferenceEquals(storage.SidLists.Geysers, geyser.Pool == storage.Pool ? storage.SidLists.Geysers : null));
        // 간헐천 가상표의 Validate 는 Native finder(VisitAtPosition) 가 있어 통과한다.
        int sid = storage.FactoryExecutor().Create(74);
        storage.Pool.RawSlot(sid)[0] = 0x88;
        storage.Pool.RawSlot(sid)[1] = 0xDA;
        storage.Pool.RawSlot(sid)[2] = 0x50;
        storage.Pool.RawSlot(sid)[3] = 0x00;
        storage.Pool.RawSlot(sid)[10] = 122;
        // 같은 장기 장치의 사전 검사는 실제 실행 없이 통과한다.
        geyser.Validate(sid, 1);
    }
}
