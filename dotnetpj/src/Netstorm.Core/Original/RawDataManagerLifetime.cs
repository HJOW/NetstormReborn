using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// DataManager의 연결 섹션과 MemStream 버퍼를 원본 소멸 순서로 반납하는 데 쓰는 외부 장치 연결이다(원본 <c>DataManagerLifetimeHooks</c>).
/// 실제 호스트 포인터 대신 DWORD 핸들로 현재 섹션과 두 종류의 할당을 조회한다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerLifetimeHooks
{
    /// <summary>현재 노드의 이름·next를 포함한 60바이트 이상을 빌린다.</summary>
    public RawBytesLookup? Stream { get; set; }

    /// <summary>MemStream 첫 DWORD가 가리키는 실제 버퍼 할당만 반납한다.</summary>
    public Action<uint>? ReleaseBuffer { get; set; }

    /// <summary>버퍼 소멸 뒤 같은 60바이트 노드의 할당을 반납한다.</summary>
    public Action<uint>? ReleaseStream { get; set; }
}

/// <summary>
/// 10.78 DataManager 소멸(004bd330→0044da60)과 MemStream 소멸(0044d040)의 원본 순서다.
/// 효과 뒤 head 재조회와 소멸·free 사이의 현재 바이트를 보존한다. cpppj <c>o/RawDataManagerLifetime</c>을 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerLifetime
{
    /// <summary>0044da60이 처음 쓰는 10.78 DataManager base 가상 표 식별자다.</summary>
    private const uint BaseVtable = 0x00507dd8;

    /// <summary>MemStream 한 개의 실제 물리 폭이다.</summary>
    private const int StreamBytes = 16;

    /// <summary>DataManager 한 개의 실제 최소 물리 폭(vptr, head, current)이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭(MemStream 16 + 이름 40 + next 4)이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>섹션 노드의 next DWORD 위치다.</summary>
    private const int NodeNext = 0x38;

    /// <summary>
    /// 0044d040: 비영 버퍼를 먼저 반납한 뒤 data·alloc·end·pos만 0으로 만든다. 반납 효과가 pos 등을 바꾸어도 반환 뒤 같은 16바이트를 원본 순서로 비운다.
    /// </summary>
    /// <param name="stream">MemStream 16바이트.</param>
    /// <param name="releaseBuffer">실제 버퍼 반납 장치. 버퍼가 비어 있으면 호출하지 않는다.</param>
    public static void DestroyMemStream(Span<byte> stream, Action<uint>? releaseBuffer)
    {
        if (stream.Length < StreamBytes)
        {
            throw new ArgumentException("MemStream 소멸의 실제16BYTE 누락");
        }
        uint buffer = Read(stream, 0);
        if (buffer != 0)
        {
            Need(releaseBuffer, "MemStream 실제 버퍼 반납 미연결")(buffer);
        }
        Put(stream, 0, 0);
        Put(stream, 12, 0);
        Put(stream, 8, 0);
        Put(stream, 4, 0);
    }

    /// <summary>
    /// 004bd330→0044da60: base 가상 표를 쓰고 현재 head를 먼저 unlink한 뒤 각 노드를 소멸한다.
    /// manager는 최소 12바이트, 노드는 최소 60바이트다. manager.current와 섹션 이름·next는 보존한다. 빈 목록도 vptr를 쓴다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트(vptr, head, current).</param>
    /// <param name="hooks">노드 조회와 반납 장치.</param>
    public static void DestroyDataManager(Span<byte> manager, DataManagerLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("DataManager 소멸의 실제12BYTE 누락");
        }
        uint head = Read(manager, 4);
        Put(manager, 0, BaseVtable);
        // 앞선 버퍼·free 효과가 바꾼 현재 head를 다음 순회에서 다시 읽는다
        while (head != 0)
        {
            if (hooks.Stream is null || hooks.ReleaseStream is null)
            {
                throw new InvalidOperationException("DataManager 실제 섹션 조회/반납 미연결");
            }
            uint node = Read(manager, 4);
            Span<byte> stream = hooks.Stream(node);
            if (stream.Length < NodeBytes)
            {
                throw new ArgumentException("DataManager 실제60BYTE 섹션 노드 누락");
            }
            Put(manager, 4, Read(stream, NodeNext));
            DestroyMemStream(stream, hooks.ReleaseBuffer);
            hooks.ReleaseStream(node);
            head = Read(manager, 4);
        }
    }
}
