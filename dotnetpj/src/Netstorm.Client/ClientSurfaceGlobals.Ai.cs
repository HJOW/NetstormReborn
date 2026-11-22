using System.Buffers.Binary;
using Netstorm.Core.Original;

// 장기 Client의 실제 현재 Player/Surface 모드와 선택된 Host AI 본체를 연결한다.
namespace Netstorm.Client;

/// <summary>메뉴에 Session이 없을 때도 원문 자연 관문을 보존하는 장기 AI 장치다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>현재 기록과 challenge를 빌리고 비영 brain에 도달했을 때만 실제 Runtime 메모리를 선택한다.</summary>
    public RawAiWallet CreateAiWallet(Func<bool> demo)
    {
        ArgumentNullException.ThrowIfNull(demo);
        return new(() =>
        {
            var session = Gameplay.RequireSession();
            if (!ReferenceEquals(session.Pool, Storage.Pool)) throw new InvalidOperationException("AI 지갑의 실제 현재 Session/Surface Pool 불일치");
            return session.BorrowAiWalletMemory();
        }, () => Gameplay.PlayerRecords.Records, new() { Disabled = () => Storage.OwnerMode.Challenge, Demo = demo });
    }

    /// <summary>4166e0의 현재 모드/양수 owner/실제 brain 관문 뒤에만 같은 전체 Runtime으로 객체 통지를 보낸다.</summary>
    public void NotifyAiObject(uint owner, int sid, bool added)
    {
        if (Storage.OwnerMode.Challenge || unchecked((int)owner) < 1) return;
        ReadOnlyMemory<byte> rows = Gameplay.PlayerRecords.Records;
        ulong at = (ulong)owner * 180 + 0xb0;
        if (at > (ulong)rows.Length || (ulong)rows.Length - at < 4) throw new ArgumentOutOfRangeException(nameof(owner), "공통 AI 통지의 현재 Player 물리 범위");
        if (BinaryPrimitives.ReadUInt32LittleEndian(rows.Span.Slice((int)at, 4)) == 0) return;
        var session = Gameplay.RequireSession();
        if (!ReferenceEquals(session.Pool, Storage.Pool)) throw new InvalidOperationException("AI 통지의 실제 현재 Session/Surface Pool 불일치");
        using var pin = Gameplay.PinCurrentRuntime(session);
        session.NotifyAiObject(owner, sid, added);
    }
}
