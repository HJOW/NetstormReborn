using System.Buffers.Binary;
using Netstorm.Core.Original;

// 수신의 전체 슬롯/현재 callback/검증 분기를 실제 물리에서 비교한다.
namespace Netstorm.Tests;

/// <summary>35BYTE 객체 수신의 저장 순서·오류 전 문맥·경고표를 확인한다.</summary>
public sealed class RawSquidReceiveTests
{
    /// <summary>현재 입력의 지정 폭만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 인접 바이트를 유지한다.
        for (int index = 0; index < width; index++) bytes[offset + index] = (byte)(value >> (index * 8));
    }
    /// <summary>서로 다른 각 필드를 가진 고정35BYTE 입력이다.</summary>
    private static byte[] Packet()
    {
        byte[] packet = new byte[35]; Put(packet, 4, 15000, 2); Put(packet, 6, 0x3fa00000); Put(packet, 10, 0xc0200000); Put(packet, 14, 0x1234, 2);
        packet[16] = 70; packet[18] = 2; Put(packet, 19, 0xaabbccdd); Put(packet, 23, 0x11223344); Put(packet, 27, 0xfff4, 2); packet[29] = 0x81; Put(packet, 30, 0x89abcdef); packet[34] = 0xac; return packet;
    }
    /// <summary>원문 선택 위치의 단순 실제 슬롯 효과만 제공한다. 공간 효과는 각 검사가 관측한다.</summary>
    private static SquidReceiveHooks Hooks(RawSidPool pool) => new()
    {
        Take = (type, sid) => { pool.RawSlot(sid)[10] = (byte)type; pool.RawSlot(sid)[11] = 0; return sid; },
        Word = (sid, word) => RawSquidWord.Set(pool, sid, word, () => 82, null), Owner = (sid, owner) => pool.RawSlot(sid)[34] = (byte)owner,
        Flags1 = _ => 0, FirstPop = _ => 1, HasTechnology = (_, _) => true,
    };

    /// <summary>실제 생성자/writer→실제Take/WORD/Owner→수신을 이어도별도할당/풀카운터변경을하지않는다.</summary>
    [Fact]
    public void ActualFactoryAndWriterRoundTripUseSameDestinationPoolAndOwners()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index) }).ToArray();
        types[74].Flags1 = 0x10; types[74].MaxHitPoints = 200; types[74].ZOrder = 3;
        var source = new RawSidPool(32768); var sourceFactory = new RawSquidFactory(source, types); int sid = sourceFactory.Create(74); Assert.Equal(15000, sid);
        Span<byte> sourceRaw = source.AllocatedBytes(sid); Put(sourceRaw, 14, 0x3fa00000); Put(sourceRaw, 18, 0xc0200000); Put(sourceRaw, 12, 0x22, 2); sourceRaw[11] = 0x10; sourceRaw[34] = 2; Put(sourceRaw, 26, 123); sourceRaw[35] = 3;
        byte[]? wire = null; var writer = new RawSquidTransmission(source, new() { Compress = 0 }, new() { Broadcast = packet => wire = packet.ToArray() }); writer.Pop(sid, 0); Assert.NotNull(wire); Assert.Equal(35, wire.Length);
        var destination = new RawSidPool(32768, false); var factory = new RawSquidFactory(destination, types); var owner = new SquidOwner(destination, types, new(), new() { Battle = true });
        uint free = destination.FreeCount; var state = new SquidReceiveState(new(), new(), new uint[255]); int pops = 0;
        RawSquidReceive.Receive(destination, wire, state, new()
        {
            Take = factory.Take, Word = (current, word) => RawSquidWord.Set(destination, current, word, () => 82, null), Owner = owner.Set,
            Flags1 = type => types[type].Flags1, FirstPop = factory.FirstPopFlags, HasTechnology = (player, type) => player == 0 || types[type].TechBitIndex == -1,
            Pop = (current, x, y, flags) => { pops++; Assert.Equal(sid, current); Assert.Equal(1.25f, x); Assert.Equal(-2.5f, y); Assert.Equal(9u, flags); },
        });
        Assert.Equal(1, pops); Assert.Equal(free, destination.FreeCount); Assert.Equal(74, destination.Slot(sid)[10]); Assert.Equal(2, destination.Slot(sid)[34]); Assert.Equal(123, BinaryPrimitives.ReadInt32LittleEndian(destination.Slot(sid).Slice(26, 4)));
        Assert.Equal(0x22, BinaryPrimitives.ReadUInt16LittleEndian(destination.Slot(sid).Slice(12, 2))); Assert.Equal(0u, state.TypeWas.Value);
    }

    /// <summary>복원된 두 좌표/캐시/수량/추가 비트/HP·반환SID와 전체 stale 바이트를 고정 값에 비교한다.</summary>
    [Fact]
    public void NormalReceivePreservesWholeSlotAndUsesReturnedObject()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15000).Fill(0x81); pool.RawSlot(15001).Fill(0xd3); byte[] incomingBefore = pool.Slot(15000).ToArray(), expected = pool.Slot(15001).ToArray();
        var aux = new RawGameAuxState(); var state = new SquidReceiveState(aux.VerifySidBox, aux.SupportTypeWasBox, new uint[255]); var trace = new List<string>();
        SquidReceiveHooks hooks = Hooks(pool);
        hooks.Take = (type, sid) => { Assert.Equal(70u, aux.SupportTypeWasBox.Value); Assert.Equal(15000, sid); trace.Add("take"); pool.RawSlot(15001)[10] = (byte)type; return 15001; };
        hooks.Word = (sid, word) => { trace.Add("word"); RawSquidWord.Set(pool, sid, word, () => 82, null); };
        hooks.Owner = (sid, owner) => { trace.Add("owner"); pool.RawSlot(sid)[34] = (byte)owner; };
        hooks.Flags1 = type => { trace.Add("flags"); Assert.Equal(70u, type); return 0x10; }; hooks.FirstPop = _ => { trace.Add("first"); return 2; };
        hooks.HasTechnology = (owner, type) => { trace.Add("tech"); Assert.Equal(2u, owner); Assert.Equal(70u, type); return true; };
        hooks.Pop = (sid, x, y, flags) => { trace.Add("pop"); Assert.Equal(15001, sid); Assert.Equal(1.25f, x); Assert.Equal(-2.5f, y); Assert.Equal(10u, flags); };
        RawSquidReceive.Receive(pool, Packet(), state, hooks);
        expected[10] = 70; expected[11] = 4; Put(expected, 12, 0x1234, 2); Put(expected, 14, 0x3fa00000); Put(expected, 18, 0xc0200000); Put(expected, 22, 20, 2); Put(expected, 24, 0xffe5, 2);
        Put(expected, 26, 0xfffffff4); expected[34] = 2; expected[35] = 0x81; Put(expected, 36, 0x89abcdef); expected[40] = 0xaa; Put(expected, 42, 0xaabbccdd); Put(expected, 46, 0x11223344);
        Assert.Equal(expected, pool.Slot(15001).ToArray()); Assert.Equal(incomingBefore, pool.Slot(15000).ToArray()); Assert.Equal(0u, aux.SupportTypeWasBox.Value);
        Assert.Equal(new[] { "take", "word", "owner", "flags", "first", "tech", "pop" }, trace);
    }

    /// <summary>기술 검사 전 ownerMask를 보존하고Tell 뒤 현재TYPE/owner에 경고와 표식을 쓴다.</summary>
    [Fact]
    public void NoMakeUsesStoredOwnerMaskAndCurrentTypeAfterTell()
    {
        var pool = new RawSidPool(24000, false); var state = new SquidReceiveState(new(), new(), new uint[255]); SquidReceiveHooks hooks = Hooks(pool); int marked = 0;
        hooks.HasTechnology = (_, _) => { pool.RawSlot(15000)[34] = 3; return false; };
        hooks.NoMake = sid => { pool.RawSlot(sid)[10] = 71; pool.RawSlot(sid)[34] = 5; };
        hooks.MarkNoMake = sid => { marked++; Assert.Equal(5, pool.Slot(sid)[34]); }; hooks.Pop = (_, _, _, _) => { };
        RawSquidReceive.Receive(pool, Packet(), state, hooks); Assert.Equal(0u, state.NoMakeWarnings.Span[70]); Assert.Equal(4u, state.NoMakeWarnings.Span[71]); Assert.Equal(1, marked);
    }

    /// <summary>contained 진단이 복귀하며 상태/입력을 바꾸면 같은 현재 분기와 좌표를 선택한다.</summary>
    [Fact]
    public void ReturningContainedAssertionRereadsStateAndPacketCoordinates()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15000)[11] = 0; var state = new SquidReceiveState(new(), new(), new uint[255]); byte[] packet = Packet(); packet[17] = 8; SquidReceiveHooks hooks = Hooks(pool); int called = 0;
        hooks.Assertion = (expression, file, line) => { Assert.Equal("!(s->isContained())", expression); Assert.Equal(@".\Clientboard.cpp", file); Assert.Equal(0xc5u, line); pool.RawSlot(15000)[11] &= 0xf7; Put(packet, 6, 0x41180000); };
        hooks.Pop = (_, x, y, _) => { called++; Assert.Equal(9.5f, x); Assert.Equal(-2.5f, y); };
        hooks.ContainedPop = (_, _, _) => throw new IOException("진단 전 상태로 부착하면 안 됨"); RawSquidReceive.Receive(pool, packet, state, hooks);
        Assert.Equal(1, called); Assert.Equal(0x3fa00000u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(15000).Slice(14, 4)));
    }

    /// <summary>선택된 마지막Pop 누락에서는 이미 쓴 슬롯과Take 전TYPE 문맥을 되돌리지 않는다.</summary>
    [Fact]
    public void MissingSelectedPopPreservesPriorWritesAndTypeContext()
    {
        var pool = new RawSidPool(24000, false); var state = new SquidReceiveState(new(), new(), new uint[255]);
        Assert.Throws<InvalidOperationException>(() => RawSquidReceive.Receive(pool, Packet(), state, Hooks(pool)));
        Assert.Equal(70u, state.TypeWas.Value); Assert.Equal(70, pool.Slot(15000)[10]); Assert.Equal(0xaabbccddu, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(15000).Slice(42, 4)));
    }

    /// <summary>검증은packet 마스크2/8을bool로 줄이지 않고 항목마다 현재SID와 저장 슬롯을 사용한다.</summary>
    [Fact]
    public void VerificationKeepsNativeMaskedValuesAndCurrentLogSid()
    {
        var pool = new RawSidPool(24000, false); Span<byte> raw = pool.RawSlot(15000); raw[10] = 70; raw[34] = 2; raw[11] = 10;
        var state = new SquidReceiveState(new() { Value = 15000 }, new() { Value = 99 }, Memory<uint>.Empty); byte[] packet = Packet(); packet[17] = 10; var logs = new List<string>();
        RawSquidReceive.Receive(pool, packet, state, new()
        {
            TypeName = sid => { Assert.Equal(logs.Count == 0 ? 15000 : 15001, sid); return "kind"; },
            Log = (level, text) => { Assert.Equal(0x1d, level); logs.Add(text); if (logs.Count == 1) { state.VerifySid.Value = 15001; pool.RawSlot(15000)[11] = 2; } },
        });
        Assert.Equal(new[] { "VERIFY 15000 (client type=kind): deadFlag 2 != 1\n", "VERIFY 15001 (client type=kind): containedFlag 8 != 0\n" }, logs);
        Assert.Equal(0u, state.VerifySid.Value); Assert.Equal(99u, state.TypeWas.Value);
    }

    /// <summary>SID 진단 뒤에도 최초 입력SID 슬롯을 유지하며 로그 중단은 현재 검증SID를 보존한다.</summary>
    [Fact]
    public void VerificationAssertionKeepsSelectedSlotAndExceptionKeepsCurrentSid()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15000)[10] = 70; pool.RawSlot(15000)[34] = 2; pool.RawSlot(15000)[11] = 0;
        var state = new SquidReceiveState(new() { Value = 15001 }, new(), Memory<uint>.Empty); byte[] packet = Packet();
        Assert.Throws<IOException>(() => RawSquidReceive.Receive(pool, packet, state, new()
        {
            Assertion = (expression, _, line) => { Assert.Equal("zus->sid == sidToVerify", expression); Assert.Equal(0x8du, line); Put(packet, 4, 15002, 2); state.VerifySid.Value = 15003; pool.RawSlot(15000)[10] = 71; },
            TypeName = sid => { Assert.Equal(15003, sid); return "changed"; },
            Log = (_, text) => { Assert.Equal("VERIFY 15003 (client type=changed): type 70 != 71\n", text); throw new IOException("로그 중단"); },
        }));
        Assert.Equal(15003u, state.VerifySid.Value);
    }
}
