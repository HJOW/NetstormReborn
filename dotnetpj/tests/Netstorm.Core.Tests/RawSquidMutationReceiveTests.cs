using System.Buffers.Binary;
using Netstorm.Core.Original;

// 부분 물리 전체와 callback 뒤 현재 입력/슬롯을 비교한다.
namespace Netstorm.Tests;

/// <summary>내용물·감시·삭제 수신과 공통WORD setter의 원문 순서를 검증한다.</summary>
public sealed class RawSquidMutationReceiveTests
{
    /// <summary>선택 필드의 지정 폭만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 2)
    {
        // 주변 물리를 보존한다.
        for (int index = 0; index < width; index++) bytes[offset + index] = (byte)(value >> (index * 8));
    }
    /// <summary>현재 물리만 관측하는 PATCH 풀이다.</summary>
    private static RawSidPool Pool() => new(24000, false);

    /// <summary>Take 전 상태를 보존하고 반환SID/진단 뒤 현재 입력을 사용하며 firstPop 뒤 상태를 다시 읽는다.</summary>
    [Fact]
    public void ContentUsesOldIncomingStateReturnedObjectAndCurrentFields()
    {
        var pool = Pool(); pool.RawSlot(15000).Fill(0xd3); pool.RawSlot(15001).Fill(0x57); byte[] beforeIncoming = pool.Slot(15000).ToArray();
        byte[] packet = new byte[18]; Put(packet, 4, 15000); Put(packet, 6, 0x1234); Put(packet, 8, 11); Put(packet, 10, 22); Put(packet, 12, 33); Put(packet, 14, 17); packet[16] = 9; packet[17] = 0;
        var trace = new List<string>(); byte[] expected = pool.Slot(15001).ToArray();
        RawSquidMutationReceive.ReceiveContent(pool, packet, new()
        {
            ContentType = () => { trace.Add("type"); return 6; },
            Take = (type, sid) => { Assert.Equal(6u, type); Assert.Equal(15000, sid); trace.Add("take"); packet[16] = 71; return 15001; },
            Word = (sid, word) => { Assert.Equal(15001, sid); Assert.Equal(0x1234, word); trace.Add("word"); Put(packet, 8, 0xabcd); },
            FirstPop = sid => { trace.Add("first"); Assert.Equal(15001, sid); Assert.Equal(4, pool.Slot(sid)[11]); pool.RawSlot(sid)[11] = 0x70; Put(packet, 14, 19); return 2; },
            Attach = (sid, parent, flags) => { trace.Add("attach"); Assert.Equal(15001, sid); Assert.Equal(19, parent); Assert.Equal(10u, flags); },
        });
        Put(expected, 18, 0xabcd); Put(expected, 20, 22); Put(expected, 24, 33); expected[23] = 71; expected[11] = 0x60;
        Assert.Equal(expected, pool.Slot(15001).ToArray()); Assert.Equal(beforeIncoming, pool.Slot(15000).ToArray()); Assert.Equal(new[] { "type", "take", "word", "first", "attach" }, trace);
    }

    /// <summary>감시 수신은firstPop/일반Attach 없이 실제 가상+74의flags0만 선택한다.</summary>
    [Fact]
    public void DependencyPreservesUnwrittenBytesAndUsesDedicatedAttach()
    {
        var pool = Pool(); pool.RawSlot(15000).Fill(0xd3); byte[] expected = pool.Slot(15000).ToArray(); byte[] packet = new byte[13]; Put(packet, 4, 15000); Put(packet, 6, 5); packet[8] = 0x80; Put(packet, 9, 0xabcd); Put(packet, 11, 0x1234);
        int calls = 0;
        RawSquidMutationReceive.ReceiveDependency(pool, packet, new()
        {
            DependType = () => 1, Take = (type, sid) => { Assert.Equal(1u, type); return sid; },
            FirstPop = _ => throw new IOException("미선택firstPop"), Attach = (_, _, _) => throw new IOException("미선택Attach"),
            DependencyAttach = (sid, parent, flags) => { calls++; Assert.Equal(15000, sid); Assert.Equal(5, parent); Assert.Equal(0u, flags); },
        });
        expected[11] = 0x84; Put(expected, 18, 0xabcd); Put(expected, 20, 0x1234); Assert.Equal(expected, pool.Slot(15000).ToArray()); Assert.Equal(1, calls);
    }

    /// <summary>로그/환불 효과 뒤 같은 슬롯과 입력을 다시 읽고 저장flags를 그대로 삭제에 전달한다.</summary>
    [Fact]
    public void DeletedUsesCurrentSlotAfterLogAndStoredFlagsAfterRefund()
    {
        var pool = Pool(); Span<byte> raw = pool.RawSlot(15000); raw[10] = 70; raw[11] = 0; raw[34] = 2; byte[] packet = new byte[8]; Put(packet, 4, 15000);
        var trace = new List<string>();
        RawSquidMutationReceive.ReceiveDeleted(pool, packet, new()
        {
            Boss = () => true,
            Assertion = (expression, file, line) => { trace.Add("assert"); Assert.Equal("!boss", expression); Assert.Equal(@".\Clientboard.cpp", file); Assert.Equal(0x260u, line); },
            TypeName = sid => { Assert.Equal(15000, sid); trace.Add("name"); return "kind"; },
            Log = (level, text) => { Assert.Equal(0x1d, level); Assert.Equal("destroying kind\n", text); trace.Add("log"); Put(packet, 6, 0x20); pool.RawSlot(15000)[34] = 4; },
            Refund = (sid, owner) => { trace.Add("refund"); Assert.Equal(4u, owner); Put(packet, 6, 0); pool.RawSlot(sid)[11] = 1; },
            Destroy = (sid, flags) => { trace.Add("destroy"); Assert.Equal(15000, sid); Assert.Equal(0x200008u, flags); },
            AuditSp = () => trace.Add("audit"),
        });
        Assert.Equal(new[] { "assert", "name", "log", "refund", "destroy", "audit" }, trace);
    }

    /// <summary>로그 뒤free로 바뀌면 삭제/환불을 건너뛰고 현재 감시TYPE 판정과SP 꼬리만 실행한다.</summary>
    [Fact]
    public void DeletedRereadsFreeAfterLogAndDoesNotAuditAfterException()
    {
        var pool = Pool(); pool.RawSlot(15000)[10] = 1; pool.RawSlot(15000)[11] = 0; byte[] packet = new byte[8]; Put(packet, 4, 15000); int audited = 0;
        var hooks = new SquidMutationHooks { Boss = () => false, TypeName = _ => "depend", Log = (_, _) => pool.RawSlot(15000)[11] = 1, DependType = () => 1, AuditSp = () => audited++ };
        RawSquidMutationReceive.ReceiveDeleted(pool, packet, hooks); Assert.Equal(1, audited);
        hooks.Log = (_, _) => throw new IOException("실제 로그 중단"); Assert.Throws<IOException>(() => RawSquidMutationReceive.ReceiveDeleted(pool, packet, hooks)); Assert.Equal(1, audited);
    }

    /// <summary>62h는 현재count/다음WORD를 재조회하고 음수는 데이터/삭제 장치 없이 반환한다.</summary>
    [Fact]
    public void RemoteManyUsesCurrentCountAndNextWordWithoutDeduplication()
    {
        var pool = Pool(); pool.RawSlot(15000)[11] = 0; pool.RawSlot(15001)[11] = 0; byte[] packet = new byte[14]; Put(packet, 4, 1, 4); Put(packet, 8, 15000); Put(packet, 10, 15002); Put(packet, 12, 15000); var calls = new List<int>();
        RawSquidMutationReceive.ReceiveRemoteDeletedMany(pool, packet, (sid, flags) => { calls.Add(sid); Assert.Equal(0u, flags); if (calls.Count == 1) { Put(packet, 4, 3, 4); Put(packet, 10, 15001); } });
        Assert.Equal(new[] { 15000, 15001, 15000 }, calls);
        Put(packet, 4, uint.MaxValue, 4); RawSquidMutationReceive.ReceiveRemoteDeletedMany(pool, packet.AsMemory(0, 8), null);
    }

    /// <summary>53h의SID1/물리 경계와62h의 범위 조건 없음/입력 부족 전 부분 삭제를 구별한다.</summary>
    [Fact]
    public void RemoteSingleAndManyHaveDifferentBoundsAndPreservePriorEffects()
    {
        var pool = Pool(); byte[] single = new byte[6]; Put(single, 4, 0); RawSquidMutationReceive.ReceiveRemoteDeleted(pool, single, null); Put(single, 4, 24000); RawSquidMutationReceive.ReceiveRemoteDeleted(pool, single, null);
        Put(single, 4, 1); int count = 0; RawSquidMutationReceive.ReceiveRemoteDeleted(pool, single, (sid, flags) => { Assert.Equal(1, sid); Assert.Equal(0u, flags); count++; });
        byte[] many = new byte[10]; Put(many, 4, 2, 4); Put(many, 8, 1);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidMutationReceive.ReceiveRemoteDeletedMany(pool, many, (_, _) => count++)); Assert.Equal(2, count);
    }

    /// <summary>62h 송신은 진단 뒤lowWORD를 다시 읽으며count 제한 없이 길이WORD 감김만 적용한다.</summary>
    [Fact]
    public void DeletionObserverRereadsLowWordAndWraps()
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 8).ToArray(); int calls = 0;
        RawSquidMutationReceive.ObserveRemoteDeletedMany(packet, 1, (expression, file, line) => { calls++; Assert.Equal("count != (int)-1", expression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0x9acu, line); Put(packet, 4, 0x1234ffff, 4); });
        Assert.Equal(1, calls); Assert.Equal(6, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        RawSquidMutationReceive.ObserveRemoteDeletedMany(Memory<byte>.Empty, 0, null);
    }

    /// <summary>PATCH WORD는free/예약 물리에도 먼저 저장하고 선택 통지 부족/예외에서 저장을 보존한다.</summary>
    [Fact]
    public void WordSetterKeepsStoreBeforeSelectedNotificationFailure()
    {
        var pool = Pool(); pool.RawSlot(2)[10] = 82; byte[] before = pool.Slot(2).ToArray();
        Assert.Throws<InvalidOperationException>(() => RawSquidWord.Set(pool, 2, 0xabcd, () => 82, null)); Put(before, 12, 0xabcd); Assert.Equal(before, pool.Slot(2).ToArray());
        RawSquidWord.Set(pool, 2, 0x1234, () => 83, null); Assert.Equal(0x1234, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(2).Slice(12, 2)));
    }
}
