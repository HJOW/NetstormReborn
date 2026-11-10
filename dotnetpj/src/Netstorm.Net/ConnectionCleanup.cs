// 로컬 연결의 닫힘·해제·전체 정리를 공유한다.
// 근거: cpppj `src/o/RawConnectionCleanup.h`·`RawConnectionCleanup.cpp`(004df440·004df520·004e2940·004e29c0·004e2a10·004e2a00).
// 등록 표(`ConnectionRegistration`)와 같은 전역·훅을 쓰며 분배기 호출은 3차 로컬 구현 때 잇는다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.

namespace Netstorm.Net;

// 연결 정리 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 닫힘·해제·전체 정리 본체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public static class ConnectionCleanup
{
    /// <summary>송수신 칸 수 22개다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private const int TransceiverSlots = 22;

    /// <summary>대기 칸 수 32개다. 0번은 비워 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private const int PendingSlots = 32;

    /// <summary>닫힘 표시 0x40 비트다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private const uint ClosedFlag = 0x40u;

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("연결 정리의 선택된 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>표 칸을 빌린다. 범위 밖은 거부한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="entries">읽고 쓸 표다.</param>
    /// <param name="index">빌릴 번호다.</param>
    private static ref uint Slot(uint[] entries, int index)
    {
        if (index < 0 || index >= entries.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index), index, "연결 정리 표 범위 오류");
        }

        return ref entries[index];
    }

    /// <summary>토큰의 연결을 닫힘으로 표시하고 깊이가 0이면 바로 파괴·반납한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="token">닫을 연결 토큰이다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    private static void CloseToken(uint token, ConnectionCleanupHooks hooks) =>
        DeferConnectionClose(Need(hooks.Node)(token), () => Need(hooks.Destroy)(token), () => Need(hooks.Release)(token));

    /// <summary>송수신 칸을 닫고 동기화 값이면 지운 뒤 칸을 비운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="index">닫을 송수신 번호다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    private static void CloseMainSlot(ConnectionCleanupGlobals globals, int index, ConnectionCleanupHooks hooks)
    {
        uint token = Slot(globals.Transceivers, index);
        if (token == 0u)
        {
            return;
        }

        if (token == globals.Sync.Synchronization)
        {
            globals.Sync.Synchronization = 0u;
        }

        CloseToken(token, hooks);
        Slot(globals.Transceivers, index) = 0u;
    }

    /// <summary>004df440의 닫힘 표시와 깊이 0일 때 바로 파괴·반납을 잇는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="connection">닫을 연결이다. 호출자가 소유한다.</param>
    /// <param name="destroy">깊이 0일 때 부를 파괴 효과다.</param>
    /// <param name="release">깊이 0일 때 부를 반납 효과다.</param>
    public static void DeferConnectionClose(PacketConnection connection, Action destroy, Action release)
    {
        ArgumentNullException.ThrowIfNull(connection);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(release);
        uint depth = connection.Depth;
        connection.Flags |= ClosedFlag;
        if (depth == 0u)
        {
            Need(destroy)();
            Need(release)();
        }
    }

    /// <summary>004df520의 -31..-1 대기 해제를 잇는다. 범위 밖·빈 칸은 조용히 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="id">고를 대기 번호다. 음수만 받는다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    public static void ClosePendingConnection(int id, ConnectionCleanupGlobals globals, ConnectionCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (id >= 0 || id <= -PendingSlots)
        {
            return;
        }

        int index = -id;
        uint token = Slot(globals.Pending, index);
        if (token == 0u)
        {
            return;
        }

        CloseToken(token, hooks);
        uint count = globals.Sync.PendingCount - 1u;
        Slot(globals.Pending, index) = 0u;
        globals.Sync.PendingCount = count;
    }

    /// <summary>004e2940의 송수신·대기 해제를 잇는다. 음수는 대기로 넘기고 22 이상은 진단한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="id">고를 번호다. 음수는 대기, 0..21은 송수신이다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    public static void DisconnectTransceiver(int id, ConnectionCleanupGlobals globals, ConnectionCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (id < 0)
        {
            ClosePendingConnection(id, globals, hooks);
            return;
        }

        if (id >= TransceiverSlots)
        {
            Need(hooks.Assertion)(250u);
        }

        CloseMainSlot(globals, id, hooks);
        uint buffer = Slot(globals.Buffers, id);
        if (buffer != 0u)
        {
            Need(hooks.Release)(buffer);
            Slot(globals.Buffers, id) = 0u;
        }
    }

    /// <summary>004e29c0의 0..21 전원 해제를 잇는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    public static void CloseAllTransceivers(ConnectionCleanupGlobals globals, ConnectionCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        // 콜백 중 칸이 바뀌어도 같은 호출에서는 되돌아가지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 0; index < TransceiverSlots; ++index)
        {
            CloseMainSlot(globals, index, hooks);
        }
    }

    /// <summary>004e2a10의 서버 1..8 전원 해제를 잇는다. 서버가 아니면 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="server">서버 여부다. 0이면 둔다.</param>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    public static void EndPlayerConnections(uint server, ConnectionCleanupGlobals globals, ConnectionCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (server == 0u)
        {
            return;
        }

        // 서버 1..8만 닫고 나머지는 같은 저장 값을 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 1; index < 9; ++index)
        {
            CloseMainSlot(globals, index, hooks);
        }
    }

    /// <summary>004df4a0·004e2a00의 0..31 대기 전체 해제를 잇는다. 개수는 손대지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="globals">적을 정리 표다.</param>
    /// <param name="hooks">부를 노드·파괴·반납·진단 효과다.</param>
    public static void CloseAllPendingConnections(ConnectionCleanupGlobals globals, ConnectionCleanupHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        // 빈 칸은 건너뛰고 찬 칸만 닫고 비운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 0; index < PendingSlots; ++index)
        {
            uint token = Slot(globals.Pending, index);
            if (token != 0u)
            {
                CloseToken(token, hooks);
                Slot(globals.Pending, index) = 0u;
            }
        }
    }
}
