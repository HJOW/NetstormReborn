using System.Buffers.Binary;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 현재 Player 헤더와 공장 부분뷰의 같은 실제 물리를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>Player+0/+4/+8과 실제 후처리 목록의 양방향 공유·재할당 인계를 확인한다.</summary>
public sealed class ClientPlayerFactoryBindingsTests
{
    /// <summary>검사 입력의 실제 현재 DWORD를 읽는다.</summary>
    private static uint Read(ClientPlayerRecords records, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(records.Records.Span[offset..]);
    /// <summary>별칭 입력을 실제 현재 기록에 쓴다.</summary>
    private static void Put(ClientPlayerRecords records, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(records.Records.Span[offset..], value);
    /// <summary>실제 원문 초기화가 전달하는 자기 마스크를 확인한다.</summary>
    private static void SelfAlliance(uint owner, uint mask) => Assert.Equal(1u << (int)owner, mask);

    /// <summary>실제 공통Owner가 같은Player두행의공장목록을제거/추가하고원문꼬리를보존한다.</summary>
    [Fact]
    public void ActualCommonOwnerMovesFactoryBetweenSamePlayerHeaderLists()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x38)); using var use = records.Acquire(); use.Initialize(SelfAlliance);
        var book = new RawPostPopState(); use.BindFactory(book.OwnerFactories[3], 3); use.BindFactory(book.OwnerFactories[4], 4);
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        pool.AllocatedBytes(sid).Clear(); pool.AllocatedBytes(sid)[10] = 85; pool.AllocatedBytes(sid)[34] = 3;
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); types[85].Flags2 = 0x4000;
        var mode = new SquidOwnerMode { Fort = true }; var owner = new SquidOwner(pool, types, book, mode);
        book.OwnerFactories[3].Append(sid); book.OwnerFactories[3].Append(77); book.OwnerFactories[3].Append(sid);
        owner.Set(sid, 4);
        Assert.Equal(1u, Read(records, 3 * 180 + 8)); Assert.Equal(1u, Read(records, 4 * 180 + 8)); Assert.Equal(4, pool.Slot(sid)[34]);
        Assert.Equal(new uint[] { 77, 77, (uint)sid }, records.Factory(3)[..3].ToArray()); Assert.Equal((uint)sid, records.Factory(4)[0]);
        mode.Fort = false; owner.Set(sid, 2); Assert.Equal(1u, Read(records, 4 * 180 + 8)); Assert.Equal(0u, Read(records, 2 * 180 + 8));
    }

    /// <summary>같은 목록의 추가/안정압축은 현재count를 쓰고 inactive꼬리와실제배열을보존한다.</summary>
    [Fact]
    public void SameHeaderAppendRemoveAndInteriorPointerShareActualStorage()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x79)); using var use = records.Acquire(); use.Initialize(SelfAlliance);
        var list = new RawPostPopList(); use.BindFactory(list, 1); Assert.True(list.MatchesCurrentSource(records, 1));
        Assert.Equal(30u, list.Capacity); Assert.Equal(30, list.Storage.Length); Assert.Throws<InvalidOperationException>(() => list.Entries);
        list.Append(51); list.Append(52); list.Append(51); list.Append(53); list.RemoveAll(51);
        Assert.Equal(2u, Read(records, 188)); Assert.Equal(new uint[] { 52, 53 }, list.Items().ToArray());
        Assert.Equal(new uint[] { 52, 53, 51, 53 }, records.Factory(1)[..4].ToArray());
        uint original = Read(records, 180), other = Read(records, 360);
        Put(records, 180, other + 4); Put(records, 184, 2); Put(records, 188, 0);
        list.Append(61); list.Append(62); list.Append(63);
        Assert.Equal(2, list.Storage.Length); Assert.Equal(new uint[] { 61, 62 }, list.Items().ToArray());
        Assert.Equal(new uint[] { 0x79797979, 61, 62, 0x79797979 }, records.Factory(2)[..4].ToArray()); Assert.Equal(0u, Read(records, 368));
        Put(records, 180, original); Put(records, 184, 30); Assert.Equal(new uint[] { 52, 53 }, list.Items().ToArray());
    }

    /// <summary>대여 반납은목록을지우지않고다음부모뒤같은getter가새Player/factory를읽는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ReuseAfterReallocationKeepsSameOwnerSlotAndReadsNewBase(OriginalEdition edition)
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x67), edition); var list = new RawPostPopList();
        using (var first = records.Acquire()) { first.Initialize(SelfAlliance); first.BindFactory(list, 8); list.Append(88); first.ReuseOrBindFactory(list, 8); }
        uint previous = records.Base; Assert.Equal(new uint[] { 88 }, list.Items().ToArray());
        using var second = records.Acquire(); Assert.Throws<InvalidOperationException>(() => second.ReuseOrBindFactory(list, 8));
        second.Initialize(SelfAlliance); second.ReuseOrBindFactory(list, 8); Assert.NotEqual(previous, records.Base);
        Assert.Empty(list.Items().ToArray()); Assert.Equal(0x67676767u, list.Storage[0]); list.Append(99);
        Assert.Equal(1u, Read(records, (int)(8 * records.Stride) + 8)); second.ReuseOrBindFactory(list, 8);
        Assert.Equal(new uint[] { 99 }, list.Items().ToArray()); Assert.Throws<InvalidOperationException>(() => second.BindFactory(list, 8));
    }

    /// <summary>다른 Client/행·기존독립배열을덮지않고초기화미완료/반납대여도거부한다.</summary>
    [Fact]
    public void BindingRejectsForeignOwnersSlotsExistingStorageAndExpiredUse()
    {
        var first = new ClientPlayerRecords(bytes => bytes.Fill(1)); var other = new ClientPlayerRecords(bytes => bytes.Fill(2));
        var a = first.Acquire(); using var b = other.Acquire(); var list = new RawPostPopList();
        Assert.Throws<InvalidOperationException>(() => a.BindFactory(list, 0)); a.Initialize(SelfAlliance); b.Initialize(SelfAlliance);
        a.BindFactory(list, 2); list.Append(77);
        Assert.Throws<InvalidOperationException>(() => a.ReuseOrBindFactory(list, 3)); Assert.Throws<InvalidOperationException>(() => b.ReuseOrBindFactory(list, 2));
        Assert.Throws<InvalidOperationException>(() => a.BindFactory(new() { Entries = [55], Count = 1 }, 2));
        Assert.Throws<InvalidOperationException>(() => a.BindFactory(new(), 9)); Assert.Equal(new uint[] { 77 }, list.Items().ToArray());
        a.Dispose(); Assert.Throws<ObjectDisposedException>(() => a.ReuseOrBindFactory(list, 2)); Assert.Equal(new uint[] { 77 }, list.Items().ToArray());
    }

    /// <summary>full/잘못된count는포인터를읽지않으며currentStorage장치와호출함수사본을함께보존한다.</summary>
    [Fact]
    public void SpanBindingCopiesCallablesAndDefersStorageUntilReached()
    {
        uint[] first = [11, 12, 13, 14, 15]; uint[] second = [21, 22, 23, 24, 25]; bool switched = false;
        uint count = 0, capacity = 2; int reads = 0;
        var binding = new PostPopListCurrentBinding
        {
            SpanStorage = () => { reads++; return (switched ? second : first).AsSpan(1, 2); }, Capacity = () => capacity,
            ReadCount = () => count, WriteCount = value => { switched = true; count = value; }, SourceOwner = first, SourceSlot = 4,
        };
        var list = new RawPostPopList(); list.BindCurrent(binding); binding.SpanStorage = () => throw new IOException("외부함수객체변경");
        list.Append(31); Assert.Equal(new uint[] { 11, 31, 13, 14, 15 }, first); Assert.Equal(1u, count); list.Append(32);
        Assert.Equal(new uint[] { 21, 22, 32, 24, 25 }, second); Assert.Equal(2, reads); Assert.True(list.MatchesCurrentSource(first, 4));
        list.Append(33); Assert.Equal(2, reads); capacity = 1; Assert.Throws<InvalidOperationException>(() => list.Items().ToArray()); Assert.Equal(2, reads);
        Assert.Throws<InvalidOperationException>(() => new RawPostPopList().BindCurrent(new() { Storage = () => first, SpanStorage = () => first, Capacity = () => 5, ReadCount = () => 0, WriteCount = _ => { } }));
    }

    /// <summary>용량0/포인터0은full관문에서접근하지않고잘못된개수는Items의앞선관문에서실패한다.</summary>
    [Fact]
    public void ActualZeroCapacityHeaderAvoidsNullPointerDereference()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(1)); using var use = records.Acquire(); use.Initialize(SelfAlliance);
        var list = new RawPostPopList(); use.BindFactory(list, 3); Put(records, 3 * 180, 0); Put(records, 3 * 180 + 4, 0);
        list.Append(55); Assert.Equal(0u, list.Count); Assert.Empty(list.Storage.ToArray());
        Put(records, 3 * 180 + 8, 1); list.Append(55); Assert.Equal(1u, list.Count); Assert.Throws<InvalidOperationException>(() => list.Items().ToArray());
    }
}
