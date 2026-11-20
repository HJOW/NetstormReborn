using System.Buffers.Binary;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 장기 Player 할당의 실제 물리·현재 주소·대여와 부분 실패를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>cpppj ClientPlayerRecordsTests의 물리 계약을 실제 초기화·동맹 자식으로 검사한다.</summary>
public sealed class ClientPlayerRecordsTests
{
    /// <summary>같은 실제 기록의 DWORD를 읽는다.</summary>
    private static uint Read(Memory<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Span[offset..]);
    /// <summary>검사 입력만 실제 기록에 쓴다.</summary>
    private static void Write(Memory<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Span[offset..], value);
    /// <summary>별도 행 합성 대신 실제 방향별 동맹 부모를 실행한다.</summary>
    private static void Alliance(ClientPlayerRecords records, uint owner, uint mask) =>
        new RawPlayerParticipation(OriginalEdition.Patch1078, records.Records, records.Alliances, new byte[9], new(), new()).SetAlliance(owner, mask);

    /// <summary>생성·대여 자체는 미할당이며9행과120바이트 공장 입력을 한 실제 부모가 만든다.</summary>
    [Fact]
    public void InitializationOwnsExactRecordsAndActualFactoryBytes()
    {
        int allocations = 0; var records = new ClientPlayerRecords(bytes => { Assert.Equal(120, bytes.Length); allocations++; bytes.Fill(0x79); });
        Assert.Equal(0u, records.Base); Assert.True(records.Records.IsEmpty); Assert.False(records.Initialized);
        using var use = records.Acquire(); Assert.Equal(0, allocations); Assert.False(use.Initialized);
        use.Initialize((owner, mask) => Alliance(records, owner, mask));
        Assert.True(use.Initialized); Assert.True(records.Initialized); Assert.Equal(9, allocations); Assert.Equal(1620, records.Records.Length);
        byte[] expected = new byte[1620];
        // 독립 기대9행에서 원문이 쓰는 필드만 지정하고 나머지 전체 바이트도 비교한다.
        for (int owner = 0; owner < 9; owner++)
        {
            int offset = owner * 180;
            Write(expected, offset, unchecked(0x30001000u + (uint)owner * 0x1000)); Write(expected, offset + 4, 30);
            Write(expected, offset + 0x54, 1u << owner); Write(expected, offset + 0x78, 127);
            Assert.Equal(Enumerable.Repeat(0x79797979u, 30), records.Factory((uint)owner).ToArray());
            // 같은 실제 동맹 자식이 만든81DWORD 전체의 방향을 확인한다.
            for (int target = 0; target < 9; target++) Assert.Equal(owner == target ? 1u : 0u, records.Alliances[owner * 9 + target]);
        }
        Assert.Equal(expected, records.Records.ToArray());
    }

    /// <summary>현재 +0/+4가 다른 공장과 그 중간 주소를 가리키면 복사 없이 같은 정확한 부분을 본다.</summary>
    [Fact]
    public void FactoryReadsCurrentPointerCapacityAndAlignedInterior()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x43)); using var use = records.Acquire();
        use.Initialize((owner, mask) => Alliance(records, owner, mask));
        uint original = Read(records.Records, 180), other = Read(records.Records, 360);
        Write(records.Records, 180, other + 4); Write(records.Records, 184, 2);
        Assert.Equal(2, records.Factory(1).Length); records.Factory(1)[0] = 0xabcdef;
        Assert.Equal(0xabcdefu, records.Factory(2)[1]); Assert.Equal(0x43434343u, records.Factory(2)[0]);
        Write(records.Records, 184, 0); Write(records.Records, 180, 0); Assert.Empty(records.Factory(1).ToArray());
        Write(records.Records, 184, uint.MaxValue); Assert.Throws<ArgumentOutOfRangeException>(() => records.Factory(1).ToArray());
        Write(records.Records, 184, 30); Write(records.Records, 180, other + 4); Assert.Throws<ArgumentOutOfRangeException>(() => records.Factory(1).ToArray());
        Write(records.Records, 184, 1); Write(records.Records, 180, other + 1); Assert.Throws<InvalidOperationException>(() => records.Factory(1).ToArray());
        Write(records.Records, 180, original); Assert.Throws<ArgumentOutOfRangeException>(() => records.Factory(9).ToArray());
    }

    /// <summary>반납은 필드를 유지하며 다음 실제 부모만 이전 공장을 반납하고 새 주소를 발급한다.</summary>
    [Fact]
    public void ExclusiveUseAndAdoptionKeepValuesUntilExplicitInitialization()
    {
        var deck = new RawProductionDeckStorage(); var records = new ClientPlayerRecords(bytes => bytes.Fill(0x67), productionDeckStorage: deck);
        var first = records.Acquire(); Assert.Throws<InvalidOperationException>(() => first.AdoptCurrent());
        first.Initialize((owner, mask) => Alliance(records, owner, mask)); uint previous = records.Base;
        records.FortCountBox.Value = 7; records.LastLoginPlayerBox.Value = 8; records.Factory(3)[0] = 88;
        Assert.Throws<InvalidOperationException>(() => records.Acquire());
        Assert.Throws<InvalidOperationException>(() => records.Initialize((owner, mask) => Alliance(records, owner, mask)));
        Assert.Throws<InvalidOperationException>(() => first.Initialize((owner, mask) => Alliance(records, owner, mask)));
        first.Dispose(); first.Dispose();
        using (var adopt = records.Acquire()) { adopt.AdoptCurrent(); Assert.True(adopt.Initialized); Assert.Equal(previous, records.Base); Assert.Equal(88u, records.Factory(3)[0]); Assert.Equal(7u, records.FortCountBox.Value); }
        records.Initialize((owner, mask) => Alliance(records, owner, mask));
        Assert.NotEqual(previous, records.Base); Assert.Equal(0u, records.FortCountBox.Value); Assert.Equal(0u, records.LastLoginPlayerBox.Value);
        Assert.Equal(0x67676767u, records.Factory(3)[0]); Assert.Same(deck, records.ProductionDeckStorage);
        Assert.Throws<ObjectDisposedException>(() => first.AdoptCurrent());
    }

    /// <summary>두 번째new 실패는 현재 base·첫 행·동맹 및 이미 쓰인 카운터를 되돌리지 않는다.</summary>
    [Fact]
    public void InitializationFailurePreservesReachedBytesAndCounterAliases()
    {
        int allocations = 0; ClientPlayerRecords? records = null;
        records = new(bytes => { if (++allocations == 2) throw new IOException("두 번째 실제 공장 입력 실패"); bytes.Fill(0x29); });
        using var use = records.Acquire(); records.FortCountBox.Value = 91;
        Assert.Throws<IOException>(() => use.Initialize((owner, mask) => { Assert.Equal(0u, records.FortCountBox.Value); Alliance(records, owner, mask); records.FortCountBox.Value = 73; }));
        Assert.False(records.Initialized); Assert.False(use.Initialized); Assert.Equal(73u, records.FortCountBox.Value);
        Assert.Equal(1620, records.Records.Length); Assert.NotEqual(0u, Read(records.Records, 0)); Assert.Equal(30u, Read(records.Records, 4)); Assert.Equal(127u, Read(records.Records, 0x78));
        Assert.All(records.Records.Span[180..].ToArray(), value => Assert.Equal(0, value)); Assert.Equal(1u, records.Alliances[0]); Assert.Equal(0u, records.Alliances[10]);
        Assert.Throws<InvalidOperationException>(() => use.Initialize((owner, mask) => Alliance(records, owner, mask)));
        Assert.Throws<InvalidOperationException>(() => use.AdoptCurrent());
    }

    /// <summary>현재 Core가 같은 Player를 빌리면 실제 Core0 선택 전 재할당을 막는다.</summary>
    [Fact]
    public void CoreBindingTracksSameBaseAndRequiresActualClearBeforeFree()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x38)); var core = new ClientCoreDataGlobals();
        records.BindCoreData(core); Assert.Same(core, records.BoundCoreData); Assert.True(core.Players.IsEmpty);
        records.Initialize((owner, mask) => Alliance(records, owner, mask)); Assert.True(core.Players.Equals(records.Records));
        var player = new PlayerWorldState(); core.Select(player, 0, 2, 2); core.Current.Span[0] = 0xa7;
        Assert.Equal(0xa7, records.Records.Span[2 * 180 + 0x6c]); uint before = records.Base;
        Assert.Throws<InvalidOperationException>(() => records.Initialize((owner, mask) => Alliance(records, owner, mask)));
        Assert.True(records.Initialized); Assert.Equal(before, records.Base); Assert.Equal(0xa7, core.Current.Span[0]);
        core.Select(player, 0, 0, 0); Assert.Equal(0xa7, core.Cached()[0]);
        records.Initialize((owner, mask) => Alliance(records, owner, mask)); Assert.NotEqual(before, records.Base); Assert.True(core.Players.Equals(records.Records));
        Assert.True(core.Current.IsEmpty); Assert.Equal(0xa7, core.Cached()[0]);
        Assert.Throws<InvalidOperationException>(() => records.BindCoreData(new()));
        using var use = records.Acquire(); Assert.Throws<InvalidOperationException>(() => records.BindCoreData(core));
    }

    /// <summary>길이0인 내부 포인터도 해제 관문에 걸리고 할당 끝의 포인터는 현재 영역 밖이다.</summary>
    [Fact]
    public void EmptyInteriorCorePointerStillRequiresClear()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(1)); var core = new ClientCoreDataGlobals(); records.BindCoreData(core);
        records.Initialize((owner, mask) => Alliance(records, owner, mask)); core.Current = records.Records.Slice(50, 0);
        Assert.Throws<InvalidOperationException>(() => records.Initialize((owner, mask) => Alliance(records, owner, mask)));
        core.Current = records.Records.Slice(records.Records.Length, 0); records.Initialize((owner, mask) => Alliance(records, owner, mask)); Assert.True(records.Initialized);
    }

    /// <summary>잘못된 free 종류는 선행 카운터0과 기존 물리를 보존하고 완료 플래그만 내린다.</summary>
    [Fact]
    public void WrongFactoryTokenFailsAtActualFreeBoundary()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(5)); records.Initialize((owner, mask) => Alliance(records, owner, mask));
        uint factory = Read(records.Records, 0), previous = records.Base; Write(records.Records, 0, records.Base); records.FortCountBox.Value = 5;
        Assert.Throws<InvalidOperationException>(() => records.Initialize((owner, mask) => Alliance(records, owner, mask)));
        Assert.False(records.Initialized); Assert.Equal(0u, records.FortCountBox.Value); Assert.Equal(previous, records.Base); Assert.Equal(previous, Read(records.Records, 0));
        Write(records.Records, 0, factory); records.Initialize((owner, mask) => Alliance(records, owner, mask)); Assert.True(records.Initialized); Assert.NotEqual(previous, records.Base);
    }

    /// <summary>CD의 실제1548바이트와172간격·+70 및 같은 공장/현재Core슬롯을 검증한다.</summary>
    [Fact]
    public void CdUsesActualRecordWidthAndReallocation()
    {
        var records = new ClientPlayerRecords(bytes => bytes.Fill(0x59), OriginalEdition.Cd1072); var core = new ClientCoreDataGlobals(); records.BindCoreData(core);
        int alliances = 0;
        // CD 동맹 인자는 원문 자기 마스크이며 PATCH 전용 자식은 실행하지 않는다.
        void CheckAlliance(uint owner, uint mask) { Assert.Equal(1u << (int)owner, mask); Assert.Equal(127u, Read(records.Records, (int)owner * 172 + 0x70)); alliances++; }
        using (var use = records.Acquire()) { use.Initialize(CheckAlliance); Assert.True(use.Initialized); }
        Assert.Equal(9, alliances); Assert.Equal(172u, records.Stride); Assert.Equal(1548, records.Records.Length); Assert.True(core.Players.Equals(records.Records));
        byte[] expected = new byte[1548];
        // CD 독립 기대 물리의 모든 바이트를 비교한다.
        for (int owner = 0; owner < 9; owner++) { int offset = owner * 172; Write(expected, offset, 0x30001000u + (uint)owner * 0x1000); Write(expected, offset + 4, 30); Write(expected, offset + 0x54, 1u << owner); Write(expected, offset + 0x70, 127); }
        Assert.Equal(expected, records.Records.ToArray()); records.Factory(8)[29] = 41; uint previous = records.Base;
        records.Initialize(CheckAlliance); Assert.Equal(18, alliances); Assert.NotEqual(previous, records.Base); Assert.Equal(0x59595959u, records.Factory(8)[29]); Assert.Equal(1548, core.Players.Length);
    }
}
