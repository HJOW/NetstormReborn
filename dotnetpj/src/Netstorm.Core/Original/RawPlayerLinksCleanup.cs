using System.Buffers.Binary;

// 현재 Player 기록의 세 링크 DWORD와 실제 Deck 반납 순서를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>현재 물리 Player base와 선택된 실제 메모리 반납 장치다.</summary>
public sealed class PlayerLinksCleanupHooks
{
    /// <summary>매 원문 load/store 위치에서 다시 읽을 현재 Player 기록이다.</summary>
    public Func<Memory<byte>>? Players { get; set; }
    /// <summary>선택된 비영 Deck 토큰의 실제 할당 소유자가 수행할 반납이다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>0048fcd0의9개 Player 기술/개수0쓰기와 Deck free/현재 base 재조회를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawPlayerLinksCleanup.cpp/.h. 실패 이전의 쓰기를 되돌리지 않는다.</remarks>
public static class RawPlayerLinksCleanup
{
    /// <summary>현재 owner와 무관하게9개 기록을 원문 순서로 정리한다.</summary>
    public static void Run(PlayerLinksCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        // free의 재진입도 자기 순회 위치를 가지며 매 load/store의 현재 base를 읽는다.
        for (int owner = 0; owner < 9; owner++)
        {
            int offset = owner * 180;
            BinaryPrimitives.WriteUInt32LittleEndian(Need(hooks.Players)().Span[(offset + 0x9c)..], 0);
            BinaryPrimitives.WriteUInt32LittleEndian(Need(hooks.Players)().Span[(offset + 8)..], 0);
            uint deck = BinaryPrimitives.ReadUInt32LittleEndian(Need(hooks.Players)().Span[(offset + 0x68)..]);
            if (deck != 0)
            {
                Need(hooks.Release)(deck);
                BinaryPrimitives.WriteUInt32LittleEndian(Need(hooks.Players)().Span[(offset + 0x68)..], 0);
            }
        }
    }
    /// <summary>실제로 선택한 경계가 없으면 그 위치에서 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("Player 링크 정리의 선택된 실제 장치 미연결");
}
