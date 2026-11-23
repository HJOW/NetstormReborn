// 같은 Player 원본 바이트를 직접 읽는 간헐천 등록 순회를 검사한다.
// cpppj/src/o/RawGeyserAiRegistration.cpp 의 Register/BindPatch 를 이식한다.
// 0.901에서 RU-1 실제 owner 조립용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>실제 Player9 기록과 PATCH 하위의 계약이다.</summary>
public sealed class RawGeyserAiRegistrationTests
{
    /// <summary>PATCH 9기록 1620바이트를 만든다.</summary>
    private static byte[] Records()
    {
        var records = new byte[9 * 0xb4];
        return records;
    }

    /// <summary>지정한 owner의 AI 포인터를 쓴다.</summary>
    private static void Put(byte[] records, uint owner, uint pointer)
    {
        int at = unchecked((int)(owner * 0xb4 + 0xb0));
        records[at] = unchecked((byte)pointer);
        records[at + 1] = unchecked((byte)(pointer >> 8));
        records[at + 2] = unchecked((byte)(pointer >> 16));
        records[at + 3] = unchecked((byte)(pointer >> 24));
    }

    /// <summary>null AI만 있으면 하위 없이 자연 반환한다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void NullOwnersReturnNaturallyWithoutPatch()
    {
        var registration = new RawGeyserAiRegistration(OriginalEdition.Patch1078, Records());
        registration.Register(7, 1);
    }

    /// <summary>미연결 nonnull은 첫 요청의 네 DWORD를 그대로 예외에 담는다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void UnboundNonNullThrowsFirstRequestBoundary()
    {
        byte[] records = Records();
        Put(records, 2, 0x11223344);
        Put(records, 5, 0x55667788);
        var registration = new RawGeyserAiRegistration(OriginalEdition.Patch1078, records);
        var boundary = Assert.Throws<GeyserAiRegistrationBoundary>(() => registration.Register(9, 0));
        Assert.Equal(2u, boundary.Owner);
        Assert.Equal(0x11223344u, boundary.AiPointer);
        Assert.Equal(9u, boundary.Sid);
        Assert.Equal(0u, boundary.State);
    }

    /// <summary>PATCH 하위는 번호순으로 현재 포인터를 다시 읽어 호출한다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void PatchUpdateReceivesOwnersInOrderWithCurrentPointers()
    {
        byte[] records = Records();
        Put(records, 1, 10);
        Put(records, 8, 20);
        var registration = new RawGeyserAiRegistration(OriginalEdition.Patch1078, records);
        var calls = new List<(uint Token, uint Sid, uint State)>();
        registration.BindPatch((token, sid, state) => calls.Add((token, sid, state)));
        registration.Register(3, 1);
        Assert.Equal([(10u, 3u, 1u), (20u, 3u, 1u)], calls);
    }

    /// <summary>짧은 기록·CD 판본·빈 하위는 명시 거부한다. 0.901에서 추가됐다.</summary>
    [Fact]
    public void ShortRecordsAndWrongEditionAndNullUpdateAreRejected()
    {
        Assert.Throws<ArgumentException>(() => new RawGeyserAiRegistration(OriginalEdition.Patch1078, new byte[10]));
        var cd = new RawGeyserAiRegistration(OriginalEdition.Cd1072, new byte[9 * 0xac]);
        Assert.Throws<ArgumentException>(() => cd.BindPatch((_, _, _) => { }));
        var patch = new RawGeyserAiRegistration(OriginalEdition.Patch1078, Records());
        Assert.Throws<ArgumentNullException>(() => patch.BindPatch(null!));
    }
}
