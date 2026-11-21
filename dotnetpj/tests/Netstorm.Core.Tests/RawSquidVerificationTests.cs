using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 장기 두DWORD와 callback 뒤 현재 간격 순회를 검증한다.
namespace Netstorm.Tests;

/// <summary>검증 시작/자유 객체/간격의 서로 다른 저장 시점을 확인한다.</summary>
public sealed class RawSquidVerificationTests
{
    /// <summary>원문WORD SID와 선택TYPE를 가진 짧은 입력이다.</summary>
    private static byte[] Packet(ushort sid, byte type = 0) { byte[] packet = new byte[7]; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), sid); packet[6] = type; return packet; }

    /// <summary>진단 정상 복귀 뒤 현재 입력SID를 저장하며 최초previous0은 본문/로그 없이 같은SID로 간다.</summary>
    [Fact]
    public void BeginRereadsPacketAfterAssertionAndSharesActualAuxWords()
    {
        var pool = new RawSidPool(24000, false); var aux = new RawGameAuxState(); var state = new SquidVerificationState(aux.VerifySidBox, aux.PreviousVerifySidBox); aux.VerifySidBox.Value = 7; byte[] packet = Packet(15000);
        RawSquidVerification.Begin(pool, packet, state, new() { Assertion = (expression, file, line) => { Assert.Equal("!sidToVerify", expression); Assert.Equal(@".\Clientboard.cpp", file); Assert.Equal(0x4cu, line); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), 15005); } });
        Assert.Equal(15005u, aux.VerifySidBox.Value); Assert.Equal(15005u, aux.PreviousVerifySidBox.Value);
    }

    /// <summary>현재SID 자체를 건너뛰며 로그 뒤 currentTYPE/previous를 재조회한다.</summary>
    [Fact]
    public void AdvanceRereadsSlotTypeAndCurrentPreviousAfterLog()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15001)[10] = 70; pool.RawSlot(15001)[11] = 0;
        var state = new SquidVerificationState(new() { Value = 15003 }, new() { Value = 15000 }); var logs = new List<string>();
        RawSquidVerification.Advance(pool, state, new()
        {
            TypeName = sid => { Assert.Equal(15001, sid); return "kind"; },
            Log = (_, text) => { logs.Add(text); pool.RawSlot(15001)[10] = 0; state.Previous.Value = 15002; },
        });
        Assert.Equal(new[] { "VERIFY 15001 (client type=kind): on server is free||form, but not on client 1 != 0\n" }, logs); Assert.Equal(15003u, state.Previous.Value);
    }

    /// <summary>signed65535 경계의15000 감김과DWORD 전체 증가 감김을 물리 선조회 없이 유지한다.</summary>
    [Fact]
    public void AdvanceUsesSignedBoundaryAndDwordWrap()
    {
        var pool = new RawSidPool(24000, false); var state = new SquidVerificationState(new() { Value = 15000 }, new() { Value = 65534 }); RawSquidVerification.Advance(pool, state, new()); Assert.Equal(15000u, state.Previous.Value);
        state.Sid.Value = 0; state.Previous.Value = uint.MaxValue; RawSquidVerification.Advance(pool, state, new()); Assert.Equal(0u, state.Previous.Value);
    }

    /// <summary>첫 로그 뒤 바뀐 검증SID/입력TYPE/저장 슬롯으로 둘째 비교를 하고 정상 끝에서SID만 지운다.</summary>
    [Fact]
    public void FreeVerificationRereadsAfterLogAndClearsOnlySidAtEnd()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15001)[10] = 70; pool.RawSlot(15001)[11] = 0; byte[] packet = Packet(15001, 70); var state = new SquidVerificationState(new(), new()); var logs = new List<string>();
        RawSquidVerification.ReceiveFree(pool, packet, state, new()
        {
            TypeName = sid => "n" + sid,
            Log = (_, text) => { logs.Add(text); if (logs.Count == 1) { state.Sid.Value = 15002; packet[6] = 71; pool.RawSlot(15001)[10] = 72; } },
        });
        Assert.Equal(new[] { "VERIFY 15001 (client type=n15001): isFree 1 != 0\n", "VERIFY 15002 (client type=n15002): freeType 71 != 72\n" }, logs);
        Assert.Equal(0u, state.Sid.Value); Assert.Equal(15002u, state.Previous.Value);
    }

    /// <summary>선택된 이름/로그 누락은이미 저장한SID와이전 순회를롤백하지않는다.</summary>
    [Fact]
    public void MissingLogKeepsStoredSidAndCurrentInterval()
    {
        var pool = new RawSidPool(24000, false); pool.RawSlot(15001)[10] = 70; pool.RawSlot(15001)[11] = 0; var state = new SquidVerificationState(new(), new());
        Assert.Throws<InvalidOperationException>(() => RawSquidVerification.ReceiveFree(pool, Packet(15001), state, new())); Assert.Equal(15001u, state.Sid.Value); Assert.Equal(0u, state.Previous.Value);
    }
}
