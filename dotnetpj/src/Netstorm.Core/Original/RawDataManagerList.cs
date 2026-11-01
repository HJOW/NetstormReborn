using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 같은 manager head/current와 섹션 next를 쓰는 추가·제거 본문이 쓰는 외부 장치 연결이다(원본 <c>DataManagerListHooks</c>).
/// 조회는 같은 실제 노드이며 압축·소멸·반납은 현재 소유자의 실제 소비자다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerListHooks
{
    /// <summary>같은 60바이트 조회·버퍼 free·노드 free를 공유한다.</summary>
    public DataManagerLifetimeHooks Lifetime { get; set; } = new();

    /// <summary>마지막 노드의 실제 0044cd40이다. 그 뒤 현재 필드를 다시 읽는다.</summary>
    public Action<uint>? Compact { get; set; }

    /// <summary>0044d0f0의 temp-&gt;next 진단이다. 반환 뒤 같은 선행 노드를 다시 읽는다.</summary>
    public Action? MissingPredecessor { get; set; }
}

/// <summary>
/// 10.78 DataManager 섹션 목록의 추가(0044ce10)와 제거(0044d0f0)다.
/// 선행 링크·압축·소멸 뒤 재조회와 stale current를 보존한다. cpppj <c>o/RawDataManagerList</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerList
{
    /// <summary>DataManager 한 개의 실제 최소 물리 폭이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>섹션 노드의 next DWORD 위치다.</summary>
    private const int NodeNext = 0x38;

    /// <summary>원본 포인터를 호스트 주소로 자르지 않고 같은 실제 60바이트에서 조회한다.</summary>
    /// <param name="hooks">노드 조회 장치.</param>
    /// <param name="handle">노드 핸들. 0이면 거부한다.</param>
    private static Span<byte> Node(DataManagerListHooks hooks, uint handle)
    {
        if (handle == 0 || hooks.Lifetime.Stream is null)
        {
            throw new InvalidOperationException("DataManager 목록의 실제 노드 조회 미연결");
        }
        Span<byte> bytes = hooks.Lifetime.Stream(handle);
        if (bytes.Length < NodeBytes)
        {
            throw OutOfRange("DataManager 목록의 실제60BYTE 누락");
        }
        return bytes;
    }

    /// <summary>원본이 보고한 뒤 계속하는 선행 링크 진단이다. 소비자가 실제 수리나 중단을 선택한다.</summary>
    /// <param name="hooks">진단 장치.</param>
    private static void Missing(DataManagerListHooks hooks)
    {
        Need(hooks.MissingPredecessor, "DataManager 목록의 선행 링크 진단 미연결")();
    }

    /// <summary>
    /// 0044ce10: 비어 있지 않은 목록은 마지막 stream 압축 뒤 마지막 노드의 next와 current에 새 핸들을 쓴다. 빈 목록은 압축이나 노드 조회 없이 head·current 두 필드만 쓴다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="stream">추가할 노드 핸들.</param>
    /// <param name="hooks">노드 조회·압축 장치.</param>
    public static void Append(Span<byte> manager, uint stream, DataManagerListHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("DataManager 추가의 실제12BYTE 누락");
        }
        uint tail = Read(manager, 4);
        if (tail != 0)
        {
            // 원본처럼 현재 next를 따라 마지막 노드를 찾는다
            uint next;
            while ((next = Read(Node(hooks, tail), NodeNext)) != 0)
            {
                tail = next;
            }
            Need(hooks.Compact, "DataManager 마지막 stream 압축 미연결")(tail);
            Put(Node(hooks, tail), NodeNext, stream);
        }
        else
        {
            Put(manager, 4, stream);
        }
        Put(manager, 8, stream);
    }

    /// <summary>
    /// 0044d0f0: 먼저 unlink하고 같은 노드의 MemStream 소멸→노드 반납을 수행한다. 해제 도중 외부 효과가 바꾼 current는 별도로 0으로 만들지 않는다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="stream">제거할 노드 핸들.</param>
    /// <param name="hooks">노드 조회·소멸·반납·진단 장치.</param>
    public static void Remove(Span<byte> manager, uint stream, DataManagerListHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("DataManager 제거의 실제12BYTE 누락");
        }
        uint previous = Read(manager, 4);
        if (stream == previous)
        {
            Put(manager, 4, Read(Node(hooks, stream), NodeNext));
        }
        else
        {
            if (Read(Node(hooks, previous), NodeNext) == 0)
            {
                Missing(hooks);
            }
            else
            {
                // 목표의 직전 노드를 유지하고 다음 링크가 0이면 같은 선행 노드로 진단한다
                for (; ; )
                {
                    uint next = Read(Node(hooks, previous), NodeNext);
                    if (next == stream)
                    {
                        break;
                    }
                    previous = next;
                    if (Read(Node(hooks, previous), NodeNext) == 0)
                    {
                        break;
                    }
                }
                if (Read(Node(hooks, previous), NodeNext) == 0)
                {
                    Missing(hooks);
                }
            }
            uint current = Read(Node(hooks, previous), NodeNext);
            Put(Node(hooks, previous), NodeNext, Read(Node(hooks, current), NodeNext));
        }
        if (stream != 0)
        {
            RawDataManagerLifetime.DestroyMemStream(Node(hooks, stream), hooks.Lifetime.ReleaseBuffer);
            Need(hooks.Lifetime.ReleaseStream, "DataManager 실제 노드 반납 미연결")(stream);
        }
    }
}
