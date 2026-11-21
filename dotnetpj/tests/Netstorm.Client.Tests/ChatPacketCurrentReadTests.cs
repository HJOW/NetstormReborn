using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;

// 원문 패킷의 현재 count 재조회와 C locale 숫자를 독립 입력으로 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 원문 본체를 실제 현재 읽기 경계에서 검사한다.</summary>
public sealed class ChatPacketCurrentReadTests
{
    /// <summary>같은 비정렬 DWORD 입력을 작성한다.</summary>
    private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);
    /// <summary>구독자 getter가 count를 줄이면 다음 반복은 새 count에서 종료해야 한다.</summary>
    [Fact]
    public void SearchReadsCurrentCountAfterSubscriberGetter()
    {
        byte[] manager = new byte[2712], recipients = new byte[8]; Put(manager, 0, 2); Put(manager, 8, 2); Put(recipients, 0, 200); Put(recipients, 4, 100);
        int reads = 0, added = 0; var hooks = new ChatPacketHooks
        {
            Memory = token => token == 1 ? manager : recipients, CurrentName = () => [78, 0],
            Read = address => { if (address != RawChatPacket.SubscriberAddress) return 0; reads++; Put(manager, 0, 0); return 100; },
            AddRecipient = (_, _) => { added++; throw new OperationCanceledException(); },
        };
        Assert.Throws<OperationCanceledException>(() => ChatPacketBody.SendChatPacket(1, 1, [120, 0], hooks)); Assert.Equal(2, reads); Assert.Equal(1, added);
    }
    /// <summary>사용자 음수 표식이 원문 signed 십진 접두에 들어가지 않아야 한다.</summary>
    [Fact]
    public void BattlePrefixUsesInvariantSignedDecimal()
    {
        byte[] manager = new byte[2712], recipients = new byte[4]; Put(manager, 0, 1); Put(manager, 8, 2); Put(manager, 16, uint.MaxValue); Put(recipients, 0, 100); byte[]? sent = null;
        var hooks = new ChatPacketHooks { Memory = token => token == 1 ? manager : recipients, CurrentName = () => [78, 0], Read = address => address switch { RawChatPacket.SubscriberAddress => 100, RawChatPacket.BattleAddress => 1, _ => 0 }, Send = (_, bytes) => sent = bytes };
        CultureInfo previous = CultureInfo.CurrentCulture; var culture = (CultureInfo)CultureInfo.InvariantCulture.Clone(); culture.NumberFormat.NegativeSign = "negative:";
        try { CultureInfo.CurrentCulture = culture; Assert.Equal(1u, ChatPacketBody.SendChatPacket(1, 1, [120, 0], hooks)); }
        finally { CultureInfo.CurrentCulture = previous; }
        Assert.NotNull(sent); Assert.Equal("0-1x", Encoding.Latin1.GetString(sent!, 17, 4)); Assert.Equal(4, BinaryPrimitives.ReadUInt16LittleEndian(sent!.AsSpan(12, 2)));
    }
}
