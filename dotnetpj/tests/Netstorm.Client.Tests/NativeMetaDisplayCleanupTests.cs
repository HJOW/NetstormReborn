using System.Buffers.Binary;
using Netstorm.Core.Original;

// 메타 이탈이 현재 CS를 다시 읽고 정상 삭제 뒤에만 해당 필드를 반납하는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>구역/참가자/전투 순서와 실패 접두를 실제 CS·가변 배열 소유자로 검증한다.</summary>
public sealed class NativeMetaDisplayCleanupTests
{
    /// <summary>현재 기록의 한 DWORD만 명시 입력으로 쓴다.</summary>
    private static void Put(Memory<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Span.Slice(offset, 4), value);
    /// <summary>현재 실제 DWORD를 관측한다.</summary>
    private static uint Word(Memory<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Span.Slice(offset, 4));
    /// <summary>실제 CS 생성자를 실행한 장기 소유자를 만든다.</summary>
    private static ClientChallengeGlobals Challenge()
    {
        var challenge = new ClientChallengeGlobals(); Assert.True(challenge.Prepare()); return challenge;
    }
    /// <summary>실제 참가자의16BYTE 배열 헤더에 두 등록된 할당과 길이/상한을 설치한다.</summary>
    private static (uint First, uint Second) Arrays(ClientChallengeGlobals challenge, uint participant)
    {
        uint first = challenge.AllocateArray(8), second = challenge.AllocateArray(8);
        Memory<byte> header = challenge.Participant(participant).Slice(0xbc, 16);
        Put(header, 0, first); Put(header, 4, second); Put(header, 8, 1); Put(header, 12, 2);
        return (first, second);
    }
    /// <summary>SID가 있는 경우의 배열만 반납하고 참가자0/39·전투0/9를 원문 순서대로 방문한다.</summary>
    [Fact]
    public void LeaveOrdersActualZonesParticipantsAndBattlesWithoutResettingOtherFields()
    {
        var challenge = Challenge(); byte[] zones = new byte[RawMetaZoneCleanup.Bytes]; var calls = new List<uint>();
        Put(zones, 0, 1); Put(zones, 76, 50); Put(zones, 88, 0x12345678);
        Put(zones, RawMetaZoneCleanup.Stride, 0xffffffff); Put(zones, RawMetaZoneCleanup.Stride + 76, 51);
        Put(zones, 50 * RawMetaZoneCleanup.Stride, 2); Put(zones, 50 * RawMetaZoneCleanup.Stride + 76, 100);
        Put(challenge.Participant(0), 0xb8, 101); Put(challenge.Participant(39), 0xb8, 139);
        var first = Arrays(challenge, 0); var last = Arrays(challenge, 39); var retained = Arrays(challenge, 1);
        byte[] retainedRecord = challenge.Participant(1).ToArray(); uint current = challenge.Current;
        Put(challenge.Battle(0), 0x20, 201); Put(challenge.Battle(9), 0x20, 209); Put(challenge.Battle(9), 0x24, 0xdeadbeef);
        NativeMetaDisplayCleanup.Leave(challenge, zones, (sid, flags) =>
        {
            Assert.Equal(0u, flags); calls.Add(sid);
            if (sid == 101) { Assert.Equal(0u, Word(zones, 50 * RawMetaZoneCleanup.Stride + 76)); Assert.Equal(101u, Word(challenge.Participant(0), 0xb8)); }
            if (sid == 139) { Assert.Equal(0u, Word(challenge.Participant(0), 0xb8)); Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(first.First)); }
            if (sid == 201) Assert.Equal(0u, Word(challenge.Participant(39), 0xb8));
        });
        Assert.Equal(new uint[] { 50, 100, 101, 139, 201, 209 }, calls);
        Assert.Equal(current, challenge.Current); Assert.Equal(retainedRecord, challenge.Participant(1).ToArray());
        Assert.Equal(0x12345678u, Word(zones, 88)); Assert.Equal(1u, Word(zones, 0)); Assert.Equal(51u, Word(zones, RawMetaZoneCleanup.Stride + 76));
        Assert.Equal(0xdeadbeefu, Word(challenge.Battle(9), 0x24));
        Assert.All(challenge.Participant(0).Slice(0xbc, 16).ToArray(), value => Assert.Equal(0, value));
        Assert.All(challenge.Participant(39).Slice(0xbc, 16).ToArray(), value => Assert.Equal(0, value));
        // 두 반납 포인터는 소유자에서 사라지고 SID0 참가자의 두 할당은 유지된다.
        foreach (uint token in new[] { first.First, first.Second, last.First, last.Second }) Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(token));
        Assert.Equal(8, challenge.ArrayBytes(retained.First).Length); Assert.Equal(8, challenge.ArrayBytes(retained.Second).Length);
        Assert.Equal(0u, Word(challenge.Battle(0), 0x20)); Assert.Equal(0u, Word(challenge.Battle(9), 0x20));
    }

    /// <summary>실제 삭제 실패 전에 끝난 구역만0으로 쓰며 참가자·배열·뒤 전투는 건드리지 않는다.</summary>
    [Fact]
    public void DestroyFailurePreservesEarlierZoneEffectsAndUnvisitedChallenge()
    {
        var challenge = Challenge(); byte[] zones = new byte[RawMetaZoneCleanup.Bytes];
        Put(zones, 0, 1); Put(zones, 76, 5); Put(zones, 50 * RawMetaZoneCleanup.Stride, 1); Put(zones, 50 * RawMetaZoneCleanup.Stride + 76, 6);
        Put(challenge.Participant(0), 0xb8, 8); var array = Arrays(challenge, 0); var calls = new List<uint>();
        Assert.Throws<IOException>(() => NativeMetaDisplayCleanup.Leave(challenge, zones, (sid, _) => { calls.Add(sid); if (sid == 6) throw new IOException("실제 삭제 실패 입력"); }));
        Assert.Equal(new uint[] { 5, 6 }, calls); Assert.Equal(0u, Word(zones, 76)); Assert.Equal(6u, Word(zones, 50 * RawMetaZoneCleanup.Stride + 76));
        Assert.Equal(8u, Word(challenge.Participant(0), 0xb8)); Assert.Equal(8, challenge.ArrayBytes(array.First).Length);
    }

    /// <summary>콜백이 현재 CS를 바꾸면 반환 뒤 SID와 배열은 새 현재 기록에 쓰고 이전 기록을 보존한다.</summary>
    [Fact]
    public void DestroyCallbackReselectsCurrentChallengeBeforeSidAndArrayStores()
    {
        var challenge = Challenge(); uint previous = challenge.Current; Memory<byte> old = challenge.Participant(0);
        Put(old, 0xb8, 10); var oldArray = Arrays(challenge, 0);
        challenge.Current = 0; Assert.True(challenge.Prepare()); uint next = challenge.Current; Memory<byte> current = challenge.Participant(0);
        Put(current, 0xb8, 20); var nextArray = Arrays(challenge, 0); challenge.Current = previous;
        var calls = new List<uint>();
        NativeMetaDisplayCleanup.Leave(challenge, new byte[RawMetaZoneCleanup.Bytes], (sid, _) => { calls.Add(sid); challenge.Current = next; });
        Assert.Equal(new uint[] { 10 }, calls); Assert.Equal(10u, Word(old, 0xb8)); Assert.Equal(0u, Word(current, 0xb8));
        Assert.Equal(8, challenge.ArrayBytes(oldArray.First).Length); Assert.Equal(oldArray.First, Word(old, 0xbc));
        Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(nextArray.First)); Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(nextArray.Second));
    }

    /// <summary>배열 free 실패는 앞선 실제 삭제·SID0과 첫 free를 되돌리지 않고 다음 참가자로 진행하지 않는다.</summary>
    [Fact]
    public void ArrayFailureKeepsSidStoreAndSuccessfulFirstFree()
    {
        var challenge = Challenge(); Put(challenge.Participant(0), 0xb8, 10); Put(challenge.Participant(1), 0xb8, 11);
        uint first = challenge.AllocateArray(8); Put(challenge.Participant(0), 0xbc, first); Put(challenge.Participant(0), 0xc0, 0xf0000000);
        var calls = new List<uint>();
        Assert.Throws<InvalidOperationException>(() => NativeMetaDisplayCleanup.Leave(challenge, new byte[RawMetaZoneCleanup.Bytes], (sid, _) => calls.Add(sid)));
        Assert.Equal(new uint[] { 10 }, calls); Assert.Equal(0u, Word(challenge.Participant(0), 0xb8)); Assert.Equal(11u, Word(challenge.Participant(1), 0xb8));
        Assert.Equal(first, Word(challenge.Participant(0), 0xbc)); Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(first));
    }

    /// <summary>비영 마커가 없을 때는 삭제 장치를 요구하지 않고, 선택된 첫 비영 참가자는 해당 위치에서 거부한다.</summary>
    [Fact]
    public void MissingDestroyIsOnlyRequiredAtSelectedNonzeroSid()
    {
        var challenge = Challenge(); byte[] zones = new byte[RawMetaZoneCleanup.Bytes];
        NativeMetaDisplayCleanup.Leave(challenge, zones, null);
        Put(challenge.Participant(0), 0xb8, 123);
        Assert.Throws<InvalidOperationException>(() => NativeMetaDisplayCleanup.Leave(challenge, zones, null));
        Assert.Equal(123u, Word(challenge.Participant(0), 0xb8));
    }

    /// <summary>CS가 없더라도 앞선 실제 구역 삭제를 취소하거나 CS를 자동 생성하지 않는다.</summary>
    [Fact]
    public void MissingChallengeFailsAfterSelectedZoneDeletion()
    {
        var challenge = new ClientChallengeGlobals(); byte[] zones = new byte[RawMetaZoneCleanup.Bytes]; Put(zones, 0, 1); Put(zones, 76, 7);
        var calls = new List<uint>();
        Assert.Throws<InvalidOperationException>(() => NativeMetaDisplayCleanup.Leave(challenge, zones, (sid, _) => calls.Add(sid)));
        Assert.Equal(new uint[] { 7 }, calls); Assert.Equal(0u, Word(zones, 76)); Assert.Equal(0u, challenge.Current);
    }

    /// <summary>같은 Client에 없는 로컬 상태는 첫 구역 쓰기 전에 거부한다.</summary>
    [Fact]
    public void ForeignLocalOwnerFailsBeforeAnyZoneWrite()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 0);
        var foreign = new ClientLocalStateGlobals(game, new()); Put(foreign.MetaZoneRecords, 0, 1); Put(foreign.MetaZoneRecords, 76, 7);
        Assert.Throws<ArgumentException>(() => NativeMetaDisplayCleanup.LeaveCurrent(Challenge(), surface, foreign));
        Assert.Equal(7u, Word(foreign.MetaZoneRecords, 76));
    }
}
