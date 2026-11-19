using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 직접 AI 지갑·파라미터의 세션 결선을 검사하는 클래스다. 2026-11-19 0.638에서 R4-5 AI 직접 명령 세션 결선용으로 추가했다.
// 근거: cpppj src/client/RawCampaignSession.cpp 4866·4871행의 CrankAiMoney·SetAiParameter다.
public sealed class HostSessionAiCommandTests
{
    /// <summary>같은 Player 기록과 세션 소유 본체를 쓰는 세션을 만든다. 2026-11-19 0.638에서 추가했다.</summary>
    /// <returns>새 세션이다.</returns>
    private static HostSession NewSession() => new(new Kernel(), new GameRandom(), new CrtRandom());

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.638에서 추가했다.</summary>
    /// <param name="bytes">대상 바이트 배열이다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 낮은 바이트부터 폭 4바이트로 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>바이트 배열에서 리틀 엔디언 DWORD를 읽는다. 2026-11-19 0.638에서 추가했다.</summary>
    /// <param name="bytes">원본 바이트 배열이다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>읽은 DWORD다.</returns>
    private static uint Get(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4));

    /// <summary>세션 소유 본체를 확보하고 해당 소유자의 Player 기록에 토큰을 둔다. 2026-11-19 0.638에서 추가했다.</summary>
    /// <param name="session">대상 세션이다.</param>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="fill">본체 채움 바이트다.</param>
    /// <returns>확보한 본체 배열이다.</returns>
    private static byte[] AttachBrain(HostSession session, int owner, byte fill)
    {
        uint token = session.AiBrainMemory.Allocate(RawAiCommandState.BodyBytes);
        byte[] raw = session.AiBrainMemory.Bytes(token);
        Array.Fill(raw, fill);
        Put(session.PlayerRecords, owner * RawAiCommandState.PlayerBytes + RawAiCommandState.BrainOffset, token);
        return raw;
    }

    /// <summary>세션 첫 본체 토큰이 원문과 같은 0x20000000에서 시작하는지 확인한다.</summary>
    [Fact]
    public void FirstBrainTokenMatchesOriginalBase()
    {
        var session = NewSession();
        Assert.Equal(0x20000000u, session.AiBrainMemory.Allocate(1));
    }

    /// <summary>세션 지갑 쓰기는 부착된 brain에만 고정 30000을 쓴다.</summary>
    [Fact]
    public void CrankAiMoneyWritesAttachedBrainOnly()
    {
        var session = NewSession();
        byte[] second = AttachBrain(session, 2, 0xa2);
        byte[] fourth = AttachBrain(session, 4, 0xa4);
        session.CrankAiMoney();
        Assert.Equal(30000u, Get(second, RawAiCommandState.MoneyOffset));
        Assert.Equal(30000u, Get(fourth, RawAiCommandState.MoneyOffset));
        Assert.Equal(0xa4a4a4a4u, Get(fourth, 0x220));
    }

    /// <summary>세션 파라미터 쓰기는 선택기별 위치에 float 절삭·DWORD를 쓴다.</summary>
    [Fact]
    public void SetAiParameterWritesSelectorOffsets()
    {
        var session = NewSession();
        byte[] raw = AttachBrain(session, 2, 0xa2);
        session.SetAiParameter(2u | (1u << 4) | (6u << 16));
        Assert.Equal(BitConverter.SingleToUInt32Bits(6f), Get(raw, 0x228));
        session.SetAiParameter(2u | (0u << 4) | (6u << 16));
        Assert.Equal(BitConverter.SingleToUInt32Bits(6f), Get(raw, 0x220));
        Assert.Equal(BitConverter.SingleToUInt32Bits(6f), Get(raw, 0x600));
        session.SetAiParameter(2u | (2u << 4) | (unchecked((uint)-3) << 16));
        Assert.Equal(unchecked((uint)-3), Get(raw, 0x230));
    }

    /// <summary>소유자 0과 brain 없는 소유자는 조용히 반환한다.</summary>
    [Fact]
    public void SetAiParameterIgnoresOwnerZeroAndMissingBrain()
    {
        var session = NewSession();
        byte[] raw = AttachBrain(session, 2, 0xa2);
        session.SetAiParameter(0u);
        session.SetAiParameter(3u | (1u << 4) | (6u << 16));
        Assert.Equal(0xa2a2a2a2u, Get(raw, 0x228));
    }
}
