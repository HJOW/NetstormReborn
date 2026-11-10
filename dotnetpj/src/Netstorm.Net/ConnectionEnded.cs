// 로컬 연결 종료 수신의 사전 구현을 공유한다.
// 근거: cpppj `src/o/RawConnectionEnded.h`·`RawConnectionEnded.cpp`(004b6660).
// 건강 검사(`RawConnectionHealth`, 미션 상태 블록 필요) 연결은 3차 로컬 구현 때 잇는다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.

namespace Netstorm.Net;

// 연결 종료 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 종료 수신이 읽고 쓰는 전역 묶음이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionEndedGlobals
{
    /// <summary>서버 여부다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Server { get; set; }

    /// <summary>동기화 값이다. 끊긴 토큰이면 0으로 지운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Synchronization { get; set; }

    /// <summary>현재 국면 번호다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Phase { get; set; }

    /// <summary>다음 국면 번호다. sender 10·국면 23에서 20으로 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint NextPhase { get; set; }

    /// <summary>활성 22칸이다. 끝난 소유자 칸을 0으로 지운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] Frames { get; }

    /// <summary>flags를 읽을 Player base 바이트다. 호출자가 소유하며 같은 저장소를 본다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<byte[]> Players { get; set; }

    /// <summary>같은 22칸과 Player 조회를 빌린다.</summary>
    /// <param name="frames">활성 22칸이다.</param>
    /// <param name="players">읽을 Player base 바이트 효과다.</param>
    /// <exception cref="ArgumentException">표 크기 오류다.</exception>
    public ConnectionEndedGlobals(uint[] frames, Func<byte[]> players)
    {
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(players);
        if (frames.Length != 22)
        {
            throw new ArgumentException("연결 종료의 같은 활성 22칸 필요", nameof(frames));
        }

        Frames = frames;
        Players = players;
    }
}

// 종료 수신이 부르는 외부 효과 묶음이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionEndedHooks
{
    /// <summary>서버 실패 저장 효과다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action? SaveServerFailure { get; set; }

    /// <summary>연결 해제 효과다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<int>? Disconnect { get; set; }

    /// <summary>소유자 방송 효과다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? BroadcastPlayer { get; set; }
}

// 종료 수신 본체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public static class ConnectionEnded
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("연결 종료의 선택된 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>소유자의 flags DWORD를 지우고 세운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="globals">읽고 쓸 전역 묶음이다.</param>
    /// <param name="owner">바꿀 소유자 번호다.</param>
    /// <param name="clear">고를 비트다.</param>
    /// <param name="set">켤 비트다.</param>
    private static void ChangeFlags(ConnectionEndedGlobals globals, uint owner, uint clear, uint set)
    {
        byte[] bytes = Need(globals.Players)();
        long at = (long)owner * 180 + 0x50;
        if (at < 0 || at + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "연결 종료의 실제 Player flags 범위 오류");
        }

        uint value = 0u;
        // 낮은 바이트부터 원본 DWORD를 읽는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)bytes[(int)at + index] << (index * 8);
        }

        value = (value & ~clear) | set;
        // 같은 위치에 낮은 바이트부터 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[(int)at + index] = unchecked((byte)(value >> (index * 8)));
        }
    }

    /// <summary>004b6660의 종료 수신을 잇는다. 동기화 0 지우기·서버/자신 가리기·소유자 flags·방송·다음 국면을 순서대로 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="sender">끊긴 송신자 번호다.</param>
    /// <param name="connection">끊긴 연결 식별값이다.</param>
    /// <param name="globals">읽고 쓸 전역 묶음이다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static void ReceiveConnectionEnded(int sender, uint connection, ConnectionEndedGlobals globals, ConnectionEndedHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (connection == globals.Synchronization)
        {
            globals.Synchronization = 0u;
        }

        if (!globals.Server)
        {
            if (sender == 9)
            {
                Need(hooks.SaveServerFailure)();
                if (globals.Server)
                {
                    return;
                }
            }

            Need(hooks.Disconnect)(sender);
        }
        else if (sender != 9)
        {
            Need(hooks.Disconnect)(sender);
        }

        if (globals.Server && sender > 0 && sender < 9)
        {
            uint owner = unchecked((uint)sender);
            ChangeFlags(globals, owner, 0u, 0x100u);
            ChangeFlags(globals, owner, 1u, 0u);
            ChangeFlags(globals, owner, 0x40u, 0u);
            Need(hooks.BroadcastPlayer)(owner);
            globals.Frames[(int)owner] = 0u;
        }

        if (sender == 10 && globals.Phase == 23u && globals.NextPhase != 24u && globals.NextPhase != 12u)
        {
            globals.NextPhase = 20u;
        }
    }
}
