using static Netstorm.Core.Original.RawMissionPacketFields;

// 로그인과 클라이언트 응답이 같은 현재 Ready mask writer를 사용한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissionLogin.cpp의004a6f70 전체 Ready 작성/대상 송신 본체다.</summary>
public static class RawMissionReady
{
    /// <summary>현재 낮은BYTE40만1..8에서 읽으며 대상0은 실제 전체분배, 나머지는 지정 송신이다.</summary>
    public static void Send(uint target, ReadOnlyMemory<byte> players, bool boss, Action<string>? assertion, Action<ReadOnlyMemory<byte>>? broadcast, Action<int, Memory<byte>>? send)
    {
        if (!boss) Need(assertion)("boss"); byte[] packet = [1, 0x6e, 8, 0, 0, 0, 0, 0]; uint mask = 0;
        // 참여 플래그가 없어도 각 owner의 현재 Ready 비트는 원문처럼 포함한다.
        for (int owner = 1; owner < 9; owner++) if ((Get(players.Span, owner * 180 + 0x50, 1) & 0x40) != 0) mask |= 1u << owner;
        Put(packet, 4, mask); if (target == 0) Need(broadcast)(packet); else Need(send)(unchecked((int)target), packet);
    }
}
