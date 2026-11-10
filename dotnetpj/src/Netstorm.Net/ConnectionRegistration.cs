// 로컬 연결의 송수신기·대기 등록과 재배치를 공유한다.
// 근거: cpppj `src/o/RawConnectionRegistration.h`·`RawConnectionRegistration.cpp`(004e2870·004df4f0·004e27b0).
// 정리 본체(`RawConnectionCleanup.h` 함수들) 연결은 3차 로컬 구현 때 잇는다. 정리 전역·훅 모양은 이 단위에 둔다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.

namespace Netstorm.Net;

// 연결 등록 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 정리 표가 함께 쓰는 동기화 수다. 원본 참조 대신 같은 보관자를 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionSyncState
{
    /// <summary>005c8bf4의 서버 동기화 값이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Synchronization;

    /// <summary>00543604의 대기 개수 DWORD다. 줄 때 unsigned 감김을 그대로 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint PendingCount;
}

// 정리 표의 같은 저장소다. 22칸 송수신·버퍼와 32칸 대기를 본다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionCleanupGlobals
{
    /// <summary>005e4a18의 송수신 22칸이다. 0번은 쓰지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] Transceivers { get; }

    /// <summary>005e4a70의 버퍼 22칸이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] Buffers { get; }

    /// <summary>005e46f0의 대기 32칸이다. 0번은 비워 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] Pending { get; }

    /// <summary>같은 동기화 보관자다. 호출자가 소유하며 세션 수명과 같이 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public ConnectionSyncState Sync { get; }

    /// <summary>같은 22·22·32칸과 동기화 보관자를 빌린다.</summary>
    /// <param name="transceivers">송수신 22칸이다.</param>
    /// <param name="buffers">버퍼 22칸이다.</param>
    /// <param name="pending">대기 32칸이다.</param>
    /// <param name="sync">동기화 보관자다.</param>
    /// <exception cref="ArgumentException">표 크기 오류다.</exception>
    public ConnectionCleanupGlobals(uint[] transceivers, uint[] buffers, uint[] pending, ConnectionSyncState sync)
    {
        ArgumentNullException.ThrowIfNull(transceivers);
        ArgumentNullException.ThrowIfNull(buffers);
        ArgumentNullException.ThrowIfNull(pending);
        ArgumentNullException.ThrowIfNull(sync);
        if (transceivers.Length != 22 || buffers.Length != 22 || pending.Length != 32)
        {
            throw new ArgumentException("연결 정리의 같은 22·22·32 표 필요");
        }

        Transceivers = transceivers;
        Buffers = buffers;
        Pending = pending;
        Sync = sync;
    }
}

// 정리 본체가 부르는 외부 효과 묶음이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionCleanupHooks
{
    /// <summary>연결 노드를 토큰으로 조회한다. flags·depth 필드를 읽고 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<uint, PacketConnection>? Node { get; set; }

    /// <summary>연결 파괴 본체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Destroy { get; set; }

    /// <summary>연결 반납 본체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>진단 번호다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }
}

// 등록 본체가 부르는 외부 효과 묶음이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionRegistrationHooks
{
    /// <summary>연결 노드를 토큰으로 조회한다. sender 필드를 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<uint, PacketConnection>? Node { get; set; }

    /// <summary>버퍼 모드 진입 효과다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Buffered { get; set; }

    /// <summary>진단 번호(b0·b3·b6·bb·c2)다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }
}

// 송수신기·대기 등록과 재배치 본체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public static class ConnectionRegistration
{
    /// <summary>signed 인덱스를 범위로 바꿔 표 칸을 빌린다. INT_MIN 오버플로는 거부한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="entries">읽고 쓸 표다.</param>
    /// <param name="index">빌릴 signed 번호다. 음수는 거부한다.</param>
    /// <exception cref="ArgumentOutOfRangeException">연결 등록 표 범위 오류다.</exception>
    private static ref uint Slot(uint[] entries, long index)
    {
        if (index < 0 || index >= entries.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index), index, "연결 등록 표 범위 오류");
        }

        return ref entries[(int)index];
    }

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("연결 등록의 선택된 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>004e2870의 송수신기 칸 등록과 sender 저장을 잇는다. 찬 칸이면 진단을 먼저 부른다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="id">맡길 송수신 번호다.</param>
    /// <param name="token">적을 연결 토큰이다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 외부 효과다. 찬 칸 진단·노드 조회를 쓴다.</param>
    public static void RegisterTransceiver(int id, uint token, ConnectionCleanupGlobals globals, ConnectionRegistrationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Slot(globals.Transceivers, id) != 0u)
        {
            Need(hooks.Assertion)(0xc2u);
        }

        Slot(globals.Transceivers, id) = token;
        Need(hooks.Node)(token).Sender = unchecked((uint)id);
    }

    /// <summary>004df4f0의 대기 [-id] 0 지우기와 개수 감소를 잇는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="id">지울 대기 번호다. 음수만 받는다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    public static void RemovePendingConnection(int id, ConnectionCleanupGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals);
        Slot(globals.Pending, -(long)id) = 0u;
        globals.Sync.PendingCount -= 1u;
    }

    /// <summary>004e27b0의 대기→송수신 재배치와 sender·buffered·대기 해제를 잇는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="pending">옮길 대기 번호다. 음수만 받는다.</param>
    /// <param name="target">옮겨갈 송수신 번호다. 서버면 1..8, 아니면 9만 받는다.</param>
    /// <param name="server">서버 여부다. 0이 아니면 서버 규칙을 쓴다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 외부 효과다. 규칙·찬 칸·노드·buffered 진단을 쓴다.</param>
    /// <returns>옮겨간 송수신 번호다.</returns>
    public static int AssignPendingConnection(int pending, int target, uint server, ConnectionCleanupGlobals globals, ConnectionRegistrationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (server != 0u)
        {
            if (!(target > 0 && target < 9))
            {
                Need(hooks.Assertion)(0xb0u);
            }
        }
        else if (target != 9)
        {
            Need(hooks.Assertion)(0xb3u);
        }

        if (Slot(globals.Transceivers, target) != 0u)
        {
            Need(hooks.Assertion)(0xb6u);
        }

        uint token = Slot(globals.Pending, -(long)pending);
        Slot(globals.Transceivers, target) = token;
        Need(hooks.Node)(token).Sender = unchecked((uint)target);
        Need(hooks.Buffered)(Slot(globals.Transceivers, target));
        if (pending >= 0)
        {
            Need(hooks.Assertion)(0xbbu);
        }

        RemovePendingConnection(pending, globals);
        return target;
    }
}
