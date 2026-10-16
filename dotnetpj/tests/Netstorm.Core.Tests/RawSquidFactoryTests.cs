using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 오브젝트 팩토리(<see cref="RawSquidFactory"/>)의 보호 경로와 필드 보존 규칙을 확인한다.
/// 원본 기대값 재생(X86RawFactoryTests)이 정상 호출의 결과를 대조하고, 이 검사들은 cpppj/tests/CreationTests.cpp 의 보호 검사 6종을
/// 10.78 로 옮겨 "거부한 호출은 풀을 한 바이트도 바꾸지 않는다"를 확인한다 (W02 첫 조각, 2026-10-16).
/// </summary>
public sealed class RawSquidFactoryTests
{
    /// <summary>10.78 슬롯의 소유자 바이트 위치.</summary>
    private const int OwnerOffset = 34;

    /// <summary>10.78 슬롯의 HP(4바이트) 시작 위치.</summary>
    private const int HpOffset = 26;

    /// <summary>생성자 주소 표만 채운 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) };
        }
        return types;
    }

    /// <summary>풀 전체 바이트의 복사본. 거부된 호출 앞뒤를 비교하는 데 쓴다.</summary>
    /// <param name="pool">대상 풀.</param>
    private static byte[] Snapshot(RawSidPool pool) => pool.Bytes.ToArray();

    /// <summary>미복원 생성자·자산 밖 번호·패치판 금지 타입·클라이언트 권한 오류는 슬롯을 할당하기 전에 거부한다.</summary>
    [Fact]
    public void CreateGuardsRejectBeforeAllocating()
    {
        var pool = new RawSidPool(32768, server: false);
        var factory = new RawSquidFactory(pool, Types());
        byte[] before = Snapshot(pool);
        // 자산 밖(1), 표 밖(188), 지원 안 하는 153, 패치판 금지 162, 서버 권한 없는 기본 할당(74, flags 0)
        Assert.ThrowsAny<Exception>(() => factory.Create(1, SidAllocation.Client));
        Assert.ThrowsAny<Exception>(() => factory.Create(188, SidAllocation.Client));
        Assert.ThrowsAny<Exception>(() => factory.Create(153, SidAllocation.Client));
        Assert.ThrowsAny<Exception>(() => factory.Create(162, SidAllocation.Client));
        Assert.ThrowsAny<Exception>(() => factory.Create(74, SidAllocation.Server));
        Assert.Equal(before, Snapshot(pool));
        Assert.Equal((32763u, 1u), (pool.FreeCount, pool.PredictableCursor));
    }

    /// <summary>
    /// non-void·타입 불일치·다른 클래스·contained 슬롯·서버 영역 밖 번호의 Take 는 바이트를 바꾸기 전에 거부하고,
    /// 정상 Take 는 풀 카운터를 건드리지 않고 지정하지 않은 필드를 보존한다.
    /// </summary>
    [Fact]
    public void TakeGuardsRejectBeforeWritingAndKeepPoolAccounting()
    {
        var pool = new RawSidPool(32768);
        var factory = new RawSquidFactory(pool, Types());
        int sid = factory.Create(74);
        uint free = pool.FreeCount;
        // 상태 0: void 가 아니므로 공간 해제 연결이 필요해 거부된다
        pool.AllocatedBytes(sid)[RawSidPool.StateOffset] = 0;
        byte[] before = Snapshot(pool);
        Assert.Throws<InvalidOperationException>(() => factory.Take(74, sid));
        Assert.Equal(before, Snapshot(pool));
        // void 로 되돌린 뒤 다른 타입·서버 영역 밖·풀 밖 번호
        pool.AllocatedBytes(sid)[RawSidPool.StateOffset] = RawSidPool.VoidBit;
        before = Snapshot(pool);
        Assert.ThrowsAny<Exception>(() => factory.Take(80, sid));
        Assert.ThrowsAny<Exception>(() => factory.Take(74, RawSidPool.ServerFirst - 1));
        Assert.ThrowsAny<Exception>(() => factory.Take(74, 32768));
        Assert.Equal(before, Snapshot(pool));
        // 다른 클래스(가상 표 불일치)
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[0] = 123;
        bytes[1] = bytes[2] = bytes[3] = 0;
        before = Snapshot(pool);
        Assert.Throws<InvalidOperationException>(() => factory.Take(74, sid));
        Assert.Equal(before, Snapshot(pool));
        // contained 비트
        bytes[0] = 0xD0;
        bytes[1] = 0x1D;
        bytes[2] = 0x50;
        bytes[3] = 0;
        bytes[RawSidPool.StateOffset] = RawSidPool.VoidBit | 8;
        before = Snapshot(pool);
        Assert.Throws<InvalidOperationException>(() => factory.Take(74, sid));
        Assert.Equal(before, Snapshot(pool));
        // 정상 Take: 상태에서 dead 만 지우고 void 를 켜며, 지정하지 않은 필드(HP 바이트, 임의 필드)와 풀 카운터를 보존한다
        bytes[RawSidPool.StateOffset] = RawSidPool.DeadBit | RawSidPool.VoidBit;
        bytes[HpOffset] = 0xA1;
        bytes[HpOffset + 1] = 0xB2;
        bytes[32] = 9;
        Assert.Equal(sid, factory.Take(74, sid));
        Assert.Equal(RawSidPool.VoidBit, bytes[RawSidPool.StateOffset]);
        Assert.Equal((byte)0xA1, bytes[HpOffset]);
        Assert.Equal((byte)0xB2, bytes[HpOffset + 1]);
        Assert.Equal((byte)9, bytes[32]);
        Assert.Equal((free, RawSidPool.ServerFirst + 1), (pool.FreeCount, pool.FirstFree(false)));
        // 빈 서버 슬롯도 받는다(예: 서버가 정한 번호)
        Assert.Equal(16000, factory.Take(74, 16000));
        Assert.Equal(free, pool.FreeCount);
    }

    /// <summary>
    /// 표에 없는 생성자 주소·null/assert 생성자(153·159·169)는 Create·Construct·Take 어느 경로로도 슬롯을 바꾸지 못한다.
    /// </summary>
    [Fact]
    public void UnsupportedConstructorsAreRejectedOnEveryPath()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[82].ConstructorAddress = 0x12345678;
        var factory = new RawSquidFactory(pool, types);
        int sid = factory.Create(74);
        byte[] before = Snapshot(pool);
        uint free = pool.FreeCount;
        uint cursor = pool.PredictableCursor;
        // 잘못된 생성자로 갈 모든 진입 경로에서 변경 전 거부를 확인한다
        foreach (uint type in new uint[] { 82, 153, 159, 169 })
        {
            Assert.ThrowsAny<Exception>(() => factory.Create(type));
            Assert.ThrowsAny<Exception>(() => factory.Construct(type, sid));
            Assert.ThrowsAny<Exception>(() => factory.Take(type, sid));
        }
        Assert.Equal(before, Snapshot(pool));
        Assert.Equal((free, cursor), (pool.FreeCount, pool.PredictableCursor));
    }

    /// <summary>
    /// anim 의 생성자는 HP 를 0 으로 지우고 공통 postTake 는 HP 를 보존한다. 10.78 은 HP 네 바이트 모두 지우며 소유자는 보존하고,
    /// 지운 슬롯은 void 이므로 풀에 반납할 수 있다.
    /// </summary>
    [Fact]
    public void AnimTakeClearsFourHpBytesAndReturnsToPool()
    {
        var pool = new RawSidPool(32768);
        var factory = new RawSquidFactory(pool, Types());
        int sid = factory.Create(161);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[HpOffset] = 1;
        bytes[HpOffset + 1] = 2;
        bytes[HpOffset + 2] = 3;
        bytes[HpOffset + 3] = 4;
        bytes[OwnerOffset] = 9;
        factory.PostTake(sid);
        Assert.Equal(new byte[] { 1, 2, 3, 4 }, bytes.Slice(HpOffset, 4).ToArray());
        Assert.Equal(sid, factory.Take(161, sid));
        Assert.Equal(new byte[] { 0, 0, 0, 0 }, bytes.Slice(HpOffset, 4).ToArray());
        Assert.Equal((byte)9, bytes[OwnerOffset]);
        uint free = pool.FreeCount;
        pool.Release(sid);
        Assert.Equal(free + 1, pool.FreeCount);
        Assert.NotEqual(0, pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit);
    }

    /// <summary>
    /// 독립 생성자 호출은 타입 바이트·상태·HP·소유자·zOrder 를 초기화하지 않고, 잘못된 슬롯(5 번 미만·풀 밖·빈 슬롯·contained)에는 쓰지 않는다.
    /// </summary>
    [Fact]
    public void ConstructPreservesCallerFieldsAndGuardsSlotState()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[82].Flags1 = OriginalTypeRecord.HasHitPoints;
        types[82].MaxHitPoints = 999;
        types[82].ZOrder = 5;
        var factory = new RawSquidFactory(pool, types);
        int sid = factory.Create(82);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.TypeOffset] = 0;
        bytes[HpOffset] = 7;
        bytes[35] = 77;
        bytes[OwnerOffset] = 9;
        byte[] before = Snapshot(pool);
        Assert.Equal(sid, factory.Construct(82, sid));
        Assert.Equal(before, Snapshot(pool));
        Assert.ThrowsAny<Exception>(() => factory.Construct(82, 0));
        Assert.ThrowsAny<Exception>(() => factory.Construct(82, 32768));
        Assert.ThrowsAny<Exception>(() => factory.Construct(82, 15001));
        Assert.Equal(before, Snapshot(pool));
        bytes[RawSidPool.StateOffset] |= 8;
        byte[] contained = Snapshot(pool);
        Assert.ThrowsAny<Exception>(() => factory.Construct(82, sid));
        Assert.Equal(contained, Snapshot(pool));
    }

    /// <summary>
    /// 공통 postCreate 의 HP·소유자 규칙: 서버 풀은 소유자를 0 으로 하고 HP 가 필요한 타입만 최대 HP 를 쓴다.
    /// mana(158)는 약화 옵션이 켜지면 최대 HP 를 0 쪽으로 절삭한 1/4 로 쓰고, 받침은 abstract 비트가 켜져 있을 때만 HP 를 쓴다.
    /// 클라이언트 풀은 소유자·HP 를 건드리지 않고 zOrder 만 복사한다.
    /// </summary>
    [Fact]
    public void PostCreateAppliesOwnerHpAndWeakenedManaRules()
    {
        OriginalTypeRecord[] types = Types();
        types[158].Flags1 = OriginalTypeRecord.HasHitPoints;
        types[158].MaxHitPoints = 1001;
        types[158].ZOrder = 0x1F3;
        types[82].MaxHitPoints = 500;
        types[83].Flags1 = OriginalTypeRecord.HasHitPoints;
        types[83].MaxHitPoints = 40;
        types[85].Flags2 = OriginalTypeRecord.Dais;
        types[85].Flags1 = OriginalTypeRecord.HasHitPoints;
        types[85].MaxHitPoints = 77;
        var pool = new RawSidPool(32768);
        var normal = new RawSquidFactory(pool, types);
        var weak = new RawSquidFactory(pool, types, weakenedMana: true);
        Assert.Equal(1001, BitConverter.ToInt32(pool.Slot(normal.Create(158)).Slice(HpOffset, 4)));
        // 약화 옵션: 1001 / 4 = 250 (0 쪽 절삭)
        int weakSid = weak.Create(158);
        Assert.Equal(250, BitConverter.ToInt32(pool.Slot(weakSid).Slice(HpOffset, 4)));
        Assert.Equal((byte)0xF3, pool.Slot(weakSid)[35]);
        // HP 비트가 없는 타입은 HP 를 쓰지 않는다
        Assert.Equal(0, BitConverter.ToInt32(pool.Slot(normal.Create(82)).Slice(HpOffset, 4)));
        Assert.Equal(40, BitConverter.ToInt32(pool.Slot(normal.Create(83)).Slice(HpOffset, 4)));
        // 받침: abstract 비트(+40 의 1)가 꺼져 있으면 HP 를 쓰지 않는다. 켠 채 postCreate 를 다시 부르면 쓴다
        int dais = normal.Create(85);
        Assert.Equal(0, BitConverter.ToInt32(pool.Slot(dais).Slice(HpOffset, 4)));
        pool.AllocatedBytes(dais)[40] |= 1;
        normal.PostCreate(dais);
        Assert.Equal(77, BitConverter.ToInt32(pool.Slot(dais).Slice(HpOffset, 4)));
        // 클라이언트 풀: 소유자·HP 는 그대로이고 zOrder 만 복사한다
        var clientPool = new RawSidPool(32768, server: false);
        var client = new RawSquidFactory(clientPool, types);
        int clientSid = client.Create(83, SidAllocation.Client);
        Assert.Equal(0, BitConverter.ToInt32(clientPool.Slot(clientSid).Slice(HpOffset, 4)));
    }

    /// <summary>
    /// 최초 Pop 플래그: 추가 비트(+40)의 abstract(1)·매립(8)이 모두 꺼져 있으면 1, 켜져 있으면 각각 2·4 를 더한다.
    /// 원본 004ad470 의 정적 읽기를 옮긴 것이며, 원본 기계어 기대값(unpop-x86.tsv 의 FirstPop 행)과의 대조는 Unpop 모듈 이식 때 한다.
    /// </summary>
    [Theory]
    [InlineData(0, 1u)]
    [InlineData(2, 1u)]
    [InlineData(1, 2u)]
    [InlineData(8, 4u)]
    [InlineData(9, 6u)]
    [InlineData(0xFF, 6u)]
    public void FirstPopFlagsReadAbstractAndBuriedBits(int extra, uint expected)
    {
        var pool = new RawSidPool(32768);
        var factory = new RawSquidFactory(pool, Types());
        int sid = factory.Create(74);
        pool.AllocatedBytes(sid)[40] = (byte)extra;
        Assert.Equal(expected, factory.FirstPopFlags(sid));
        // 가상 표가 타입과 맞지 않으면 거부한다
        pool.AllocatedBytes(sid)[0] ^= 0xFF;
        Assert.Throws<InvalidOperationException>(() => factory.FirstPopFlags(sid));
    }

    /// <summary>타입 레코드 수와 풀 혼용 같은 연결 오류는 생성 시점에 거부한다.</summary>
    [Fact]
    public void ConstructorRejectsWrongTypeCount()
    {
        var pool = new RawSidPool(32768);
        Assert.Throws<ArgumentException>(() => new RawSquidFactory(pool, new OriginalTypeRecord[10]));
        Assert.Same(pool, new RawSquidFactory(pool, Types()).Pool);
    }
}
