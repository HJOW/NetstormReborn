using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 VT의 실제 원문 주소와 장기 Host/자원 정기의 결선을 독립 PE 자료로 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>PE 자료의 기본13/피해14 VT와 실제 예약/현재 사건/미설치 경계를 검사한다. Source 기본3종은 이 자료 밖이다.</summary>
public sealed class NativeSurfaceDefaultRegularTests
{
    /// <summary>보호 PE의 실제83VT 행을 정수로 읽는다. C# 분배 표를 기대값으로 사용하지 않는다.</summary>
    private static IEnumerable<uint[]> VtableRows() => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "damageable-regular-tables-x86.tsv"))
        .Where(line => line.Length > 0 && line[0] != '#').Select(line => line.Split('\t').Select(uint.Parse).ToArray());
    /// <summary>실제 +5c가00496e70인 PE 행만 검사 입력으로 제공한다.</summary>
    public static IEnumerable<object[]> BaseTables() => VtableRows().Where(row => row[1] == 0x496e70).Select(row => new object[] { row[0] });
    /// <summary>가상98과 별개로 실제 +5c=0044b900인14개 PE 행을 입력으로 제공한다.</summary>
    public static IEnumerable<object[]> DamageableTables() => VtableRows().Where(row => row[1] == 0x44b900).Select(row => new object[] { row[0] });

    /// <summary>현재 Native 장치/표시를 갖지만 World를 대여하지 않는 실제 저장소를 만든다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i), FootX = 1, FootY = 1, Cost = 100 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        storage.BindTransmissionDevices(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
        storage.BindProcessDevices(new() { Selected = () => 0, ClearSelection = () => { } });
        storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        storage.BindGraphTrace((_, _) => { }); storage.BindDisplayDiagnostics(new StringBox(), (_, _, _) => { });
        return storage;
    }
    /// <summary>같은 Pool에 부모를 실제 확보하고 요청한 VT를 현재 슬롯에 둔다.</summary>
    private static int Parent(RawSurfaceStorage storage, uint table)
    {
        int sid = storage.Pool.Allocate(SidAllocation.Server); Span<byte> raw = storage.Pool.RawSlot(sid); raw.Clear();
        raw[10] = 74; BinaryPrimitives.WriteUInt32LittleEndian(raw, table); return sid;
    }

    /// <summary>이 독립 PE 자료에 포함된 기본13VT는 사건/count/TYPE와 무관하게 float 비트를 반환한다.</summary>
    [Theory, MemberData(nameof(BaseTables))]
    public void PeBaseVtablesReturnPayloadBitsWithoutWorld(uint table)
    {
        var storage = Storage(); int sid = Parent(storage, table); storage.Pool.RawSlot(sid)[10] = 255;
        float payload = BitConverter.UInt32BitsToSingle(0xffc12345);
        float result = storage.Processes().Host().Regular()!(sid, uint.MaxValue, uint.MaxValue, payload);
        Assert.Equal(0xffc12345u, BitConverter.SingleToUInt32Bits(result)); Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }

    /// <summary>독립 PE의 피해14VT는 일반 사건에서 큰 자원 장치를 미리 요구하지 않는다.</summary>
    [Theory, MemberData(nameof(DamageableTables))]
    public void PeDamageableVtablesUseNaturalGeneralEvent(uint table)
    {
        var storage = Storage(); int sid = Parent(storage, table);
        float result = storage.Processes().Host().Regular()!(sid, 123456, 9, BitConverter.UInt32BitsToSingle(0x80000000));
        Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(result));
    }

    /// <summary>아직 없는 전용은 타입 번호가 같아도 일반 반환으로 통과하지 않는다.</summary>
    [Theory]
    [InlineData(0x50dc30u)]
    [InlineData(0x502cf0u)]
    [InlineData(0x12345678u)]
    public void MissingDedicatedBodyDoesNotBecomeBaseReturn(uint table)
    {
        var storage = Storage(); int sid = Parent(storage, table);
        Assert.Throws<InvalidOperationException>(() => storage.Processes().Host().Regular()!(sid, 99, 0, 1));
    }

    /// <summary>같은 외부 Kernel의 실제 Regular가 World 없이 기본 VT를 호출하고 현재 시각으로 재예약한다.</summary>
    [Fact]
    public void ActualKernelReschedulesNativeBaseWithoutInventedRuntime()
    {
        var storage = Storage(); int sid = Parent(storage, 0x501dd0); var kernel = new Kernel(16);
        var clock = new FrameTimeBox { Value = new FrameTime { Game = 7.5 } }; storage.Processes().BindDevices(kernel, clock);
        SquidProcessHost host = storage.Processes().Host(); Assert.Same(storage.BaseDestroy(), host.Destroy());
        RegularProcess process = Assert.IsType<RegularProcess>(host.AddRegular(sid, 99, 0.125f));
        kernel.RunFrame(); Assert.Equal(1u, process.Count()); Assert.Equal(7.625, process.Time());
        Assert.Throws<InvalidOperationException>(() => host.Runtime()); Assert.False(storage.Borrowed());
    }

    /// <summary>실제 피해 사건은 현재 공유 DWORD로 비교하며 선택하지 않은 이전 값은 일반 반환으로 돌아간다.</summary>
    [Fact]
    public void DamageEventReadsCurrentSharedValueAndRequiresSelectedDevice()
    {
        var storage = Storage(); int sid = Parent(storage, 0x501ea0); RegularHandler handler = storage.Processes().Host().Regular()!;
        Assert.Throws<InvalidOperationException>(() => handler(sid, 6699, 4, 0.5f));
        storage.GameAux().DamageRegularEvent = 777;
        Assert.Equal(0.5f, handler(sid, 6699, 4, 0.5f));
        uint seen = 0; storage.BindResourceDevices(Devices((_, code, _, _) => { seen = code; return 0.75f; }));
        Assert.Equal(0.75f, handler(sid, 777, 4, 0.5f)); Assert.Equal(777u, seen);
    }
    /// <summary>필수 자원 장치의 실제 선택 효과만 호출자가 지정한다.</summary>
    private static RawSurfaceResourceDevices Devices(RegularHandler? damage = null, Action<int>? start = null) => new()
    {
        Random = () => new GameRandom(1), CarrierCheck = _ => { }, GeyserRegistration = (_, _) => { }, Destroy = (_, _) => true,
        DamageableRegular = (sid, code, count, payload) => (damage ?? throw new InvalidOperationException("검사에서 피해를 선택하지 않음"))(sid, code, count, payload),
        StartConstruction = sid => (start ?? throw new InvalidOperationException("검사에서 건설을 선택하지 않음"))(sid),
    };

    /// <summary>추상 상태와 같은 현재 건설 타입의 포함 Form 유무에 따라 재시도하며 실제 현재 사건을 다시 읽는다.</summary>
    [Fact]
    public void ConstructionEventUsesCurrentAbstractBitAndContainedType()
    {
        var storage = Storage(); int sid = Parent(storage, 0x501ea0); RegularHandler handler = storage.Processes().Host().Regular()!;
        Assert.Equal(6700u, storage.GameAux().ConstructionRegularEvent); Assert.Equal(0f, handler(sid, 6700, 0, 1));
        storage.Pool.RawSlot(sid)[40] = 1; Assert.Throws<InvalidOperationException>(() => handler(sid, 6700, 0, 1));
        int starts = 0; storage.BindResourceDevices(Devices(start: parent => { Assert.Equal(sid, parent); starts++; }));
        Assert.Equal(0f, handler(sid, 6700, 0, 1)); Assert.Equal(1, starts);
        int form = Parent(storage, 0x501dd0); storage.Pool.RawSlot(form)[10] = 30;
        BinaryPrimitives.WriteUInt16LittleEndian(storage.Pool.RawSlot(sid)[6..], (ushort)form);
        Assert.Equal(0f, handler(sid, 6700, 0, 1)); Assert.Equal(1, starts);
        storage.GameAux().ConstructionProcessTypeBox.Value = 31;
        Assert.Equal(0f, handler(sid, 6700, 0, 1)); Assert.Equal(2, starts);
        storage.GameAux().ConstructionRegularEvent = 888;
        Assert.Equal(1f, handler(sid, 6700, 0, 1)); Assert.Equal(0f, handler(sid, 888, 0, 1)); Assert.Equal(3, starts);
    }

    /// <summary>간헐천 가상88의 현재 갱신과 직접 프레임 쓰기 뒤 같은 기본 Damageable을 사용한다.</summary>
    [Fact]
    public void NativeGeyserUsesActualDisplayAndNaturalDamageable()
    {
        var storage = Storage(); int sid = Parent(storage, 0x50da88); BinaryPrimitives.WriteUInt16LittleEndian(storage.Pool.RawSlot(sid)[12..], 80);
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(sid)[36..], 5);
        float result = storage.Processes().Host().Regular()!(sid, 0, 1, 1);
        Assert.Equal(0x3cf5c28fu, BitConverter.SingleToUInt32Bits(result)); Assert.Equal(6u, BinaryPrimitives.ReadUInt32LittleEndian(storage.Pool.Slot(sid)[36..]));
    }

    /// <summary>Player 선택은 같은 현재 상태를 요구하고 count0의 자연 반환은 Template Runtime을 미리 요구하지 않는다.</summary>
    [Fact]
    public void NativePlayerEntryUsesSameCurrentStateAndNaturalGate()
    {
        var storage = Storage(); int sid = Parent(storage, 0x50ee08); RegularHandler handler = storage.Processes().Host().Regular()!;
        Assert.Throws<InvalidOperationException>(() => handler(sid, 2, 0, 0.25f));
        var state = new PlayerWorldState(); storage.BindPlayerRegularState(state); state.BattleOwner = 0;
        Assert.Equal(0.25f, handler(sid, 2, 0, 0.25f));
        state.BattleOwner = 1; Assert.Equal(0.25f, handler(sid, 2, 5, 0.25f));
        state.BattleOwner = 0; storage.OwnerMode.Battle = true;
        Assert.Throws<InvalidOperationException>(() => handler(sid, 2, 5, 0.25f));
    }

    /// <summary>현재 아이스 VT의 일반 사건은 기존 실제 장기 본체를 사용하고 성장 하위는 미리 선택하지 않는다.</summary>
    [Fact]
    public void NativeIceEntryUsesActualBodyNaturalEvent()
    {
        var storage = Storage(); int sid = Parent(storage, 0x50e888);
        float value = storage.Processes().Host().Regular()!(sid, 123456, uint.MaxValue, 0.125f);
        Assert.Equal(0.125f, value); Assert.Same(storage.IceTower(), storage.IceTower());
    }

    /// <summary>프레임 수0은 실제 스프라이트 IDIV 실패로 남고 같은 TYPE의 기본 VT로 바꾸면 반환한다.</summary>
    [Fact]
    public void NativeSpriteEntrySelectsCurrentVtableAndRealFrameBody()
    {
        var storage = Storage(); int sid = Parent(storage, 0x50cc08); RegularHandler handler = storage.Processes().Host().Regular()!;
        Assert.Throws<DivideByZeroException>(() => handler(sid, 123456, 0, 0.5f));
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.RawSlot(sid), 0x501dd0);
        Assert.Equal(0.5f, handler(sid, 123456, 0, 0.5f));
    }

    /// <summary>펜스 장기 본체의 일반 사건은 같은 현재 실제 Damageable 부모에 연결한다.</summary>
    [Fact]
    public void NativeFenceEntryUsesExistingBodyAndSameDamageableParent()
    {
        var storage = Storage(); int sid = Parent(storage, 0x509470);
        storage.BindFenceRegularDevices(new() { Damageable = storage.ResourceDamageableRegular });
        Assert.Equal(0.25f, storage.Processes().Host().Regular()!(sid, 123456, 9, 0.25f));
        Assert.Same(storage.FenceRegular(), storage.FenceRegular());
    }
}
