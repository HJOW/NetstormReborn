using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using Netstorm.Client.Player;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 ClientPlayerRecords의 현재 base/약한 수명을 장기 고유 Pre 연결에서 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>초기화 없는 최초 결합과 실제 기록 재할당·owner 만료·공통 장치 경계를 검사한다.</summary>
public sealed class ClientPlayerDeletionTests
{
    /// <summary>Native 결합 여부만 선택하며 선택되지 않은 전송을 성공 no-op으로 대신하지 않는다.</summary>
    private static ClientSurfaceGlobals Surface(bool native = true)
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        RawSurfaceTransmissionDevices? transmission = native ? new()
        {
            Broadcast = _ => throw new InvalidOperationException("기록 결합에서 방송 선택 금지"),
            Connected = _ => throw new InvalidOperationException("기록 결합에서 접속 조회 금지"),
            Send = (_, _) => throw new InvalidOperationException("기록 결합에서 원격 송신 금지"),
        } : null;
        return new(game, new(local), new(), static () => 1000, transmission);
    }
    /// <summary>원문 초기화가 요구하는 실제9행 동맹 자식을 같은 owner에 연결한다.</summary>
    private static void Initialize(ClientPlayerRecords records) => records.Initialize((owner, mask) =>
        new RawPlayerParticipation(OriginalEdition.Patch1078, records.Records, records.Alliances, new byte[9], new(), new()).SetAlliance(owner, mask));

    /// <summary>Native·대여 전·최초 결합 계약을 검사하고 Player 미할당 상태/난수/배열을 보존한다.</summary>
    [Fact]
    public void BindingRequiresNativeAndUnborrowedStorageWithoutReadingRecords()
    {
        int allocations = 0; var records = new ClientPlayerRecords(bytes => { allocations++; bytes.Clear(); });
        Assert.Throws<ArgumentException>(() => Surface(false).BindPlayerRecords(records));
        ClientSurfaceGlobals surface = Surface();
        using (surface.Storage.AcquirePrepared()) Assert.Throws<ArgumentException>(() => surface.BindPlayerRecords(records));
        surface.BindPlayerRecords(records); Assert.Equal(0, allocations); Assert.Equal(0u, records.Base); Assert.True(records.Records.IsEmpty); Assert.False(records.Initialized);
        Assert.Throws<ArgumentException>(() => surface.BindPlayerRecords(records));
        Assert.Throws<ArgumentException>(() => surface.BindPlayerRecords(new(bytes => bytes.Clear())));
    }

    /// <summary>실제 새base를 매번 선택하고 CommonDeletion 미설치도 앞선자신SID0을보존해드러낸다.</summary>
    [Fact]
    public void RealReinitializedRecordsReachActualCommonSelectionWithPrefixPreserved()
    {
        ClientSurfaceGlobals surface = Surface(); var records = new ClientPlayerRecords(bytes => bytes.Clear()); surface.BindPlayerRecords(records); Initialize(records);
        int sid = surface.Storage.Pool.Allocate(SidAllocation.Client); surface.Storage.Pool.AllocatedBytes(sid)[34] = 1;
        Memory<byte> first = records.Records; uint firstBase = records.Base;
        BinaryPrimitives.WriteUInt32LittleEndian(first.Span[(180 + 0x9c)..], (uint)sid); BinaryPrimitives.WriteUInt32LittleEndian(first.Span[(180 + 0x50)..], 0xabcdef01);
        Assert.Contains("공통 삭제", Assert.Throws<InvalidOperationException>(() => surface.Storage.PlayerPreDestroy(sid, 0x1234)).Message);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(first.Span[(180 + 0x9c)..])); Assert.Equal(0xabcdef01u, BinaryPrimitives.ReadUInt32LittleEndian(first.Span[(180 + 0x50)..]));
        byte[] firstAfter = first.ToArray(); Initialize(records); Assert.NotEqual(firstBase, records.Base); Memory<byte> second = records.Records;
        BinaryPrimitives.WriteUInt32LittleEndian(second.Span[(180 + 0x9c)..], (uint)sid); BinaryPrimitives.WriteUInt32LittleEndian(second.Span[(180 + 0x50)..], 0xfedcba01);
        Assert.Contains("공통 삭제", Assert.Throws<InvalidOperationException>(() => surface.Storage.PlayerPreDestroy(sid, 9)).Message);
        Assert.Equal(firstAfter, first.ToArray()); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(second.Span[(180 + 0x9c)..])); Assert.Equal(0xfedcba01u, BinaryPrimitives.ReadUInt32LittleEndian(second.Span[(180 + 0x50)..]));
    }

    /// <summary>약한기록owner를JIT 호출자스택에서분리하고장기장치가강하게보유하지않는지관측한다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference<ClientPlayerRecords> BindEphemeral(ClientSurfaceGlobals surface)
    {
        var records = new ClientPlayerRecords(static bytes => bytes.Clear()); surface.BindPlayerRecords(records); return new(records);
    }
    /// <summary>owner 만료를 새기록/재결합으로 숨기지 않고 현재 조회 위치에서 명시 실패한다.</summary>
    [Fact]
    public void BindingKeepsOwnerWeakAndDoesNotAllowRebindingAfterExpiry()
    {
        ClientSurfaceGlobals surface = Surface(); WeakReference<ClientPlayerRecords> weak = BindEphemeral(surface);
        // 다른 강한 owner가 없는 명시 입력을 실제 GC로 회수하며 장기 closure의 보유 여부를 확인한다.
        for (int i = 0; i < 3; i++) { GC.Collect(); GC.WaitForPendingFinalizers(); }
        Assert.False(weak.TryGetTarget(out _));
        int sid = surface.Storage.Pool.Allocate(SidAllocation.Client);
        Assert.Contains("기록 owner 미연결", Assert.Throws<InvalidOperationException>(() => surface.Storage.PlayerPreDestroy(sid, 0)).Message);
        Assert.Throws<ArgumentException>(() => surface.BindPlayerRecords(new(static bytes => bytes.Clear())));
    }
}
