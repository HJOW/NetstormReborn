using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 같은 DataManager의 임시 stream 생성·이름 교체·최소 길이 확장이 쓰는 외부 장치 연결이다(원본 <c>DataManagerResizeHooks</c>).
/// 모든 조회는 같은 노드와 버퍼를 빌려야 하며 각 경계 뒤 현재 원문을 다시 읽는다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. cpppj는 하위 묶음을 값으로 복사하지만 C#은 같은 객체를 공유한다. 장치는 한 번 연결한 뒤 바꾸지 않는 용도라 차이가 없다.</remarks>
public sealed class DataManagerResizeHooks
{
    /// <summary>같은 head/current와 60바이트 노드의 이름 검색이다.</summary>
    public DataManagerSelectionHooks Selection { get; set; } = new();

    /// <summary>같은 노드의 추가·압축·제거·실제 반납이다.</summary>
    public DataManagerListHooks List { get; set; } = new();

    /// <summary>같은 실제 버퍼의 할당·쓰기·조회다.</summary>
    public MemStreamHooks Memory { get; set; } = new();

    /// <summary>원본 new 60의 실제 할당과 반환 핸들이다.</summary>
    public Func<uint, uint>? AllocateStream { get; set; }

    /// <summary>0044d230/0044d290의 원문 섹션 누락 진단이다. 잘못된 0 포인터를 정상값으로 바꾸지 않는다.</summary>
    public Action? MissingSource { get; set; }
}

/// <summary>
/// 10.78 DataManager의 stream 생성(0044d070·0044d1b0), 복제(0044d230), 이름 교체(0044d160), 최소 길이 확장(0044d290)이다.
/// 원본의 임시 이름과 부모·자식 효과 뒤 원문·현재 할당 길이 재조회를 유지한다. cpppj <c>o/RawDataManagerResize</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerResize
{
    /// <summary>0044d290이 생성·이름 교체에 같은 값으로 쓰는 원본 임시 이름이다.</summary>
    private const string TemporaryName = "T$e$m$p";

    /// <summary>DataManager 한 개의 실제 최소 물리 폭이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>노드 안 이름 시작 위치다.</summary>
    private const int NameOffset = 16;

    /// <summary>노드 안 이름의 실제 물리 폭이다. 이름과 NUL이 이 안에 들어가야 한다.</summary>
    private const int NameBytes = 40;

    /// <summary>섹션 노드의 next DWORD 위치다.</summary>
    private const int NodeNext = 0x38;

    /// <summary>새 stream의 기본 할당 크기(16384)다.</summary>
    private const int DefaultSize = 0x4000;

    /// <summary>포인터를 잘라 호스트 주소로 쓰지 않고 같은 실제 노드를 빌린다.</summary>
    /// <param name="hooks">노드 조회 장치.</param>
    /// <param name="handle">노드 핸들. 0이면 거부한다.</param>
    private static Span<byte> Node(DataManagerResizeHooks hooks, uint handle)
    {
        if (handle == 0 || hooks.Selection.Stream is null)
        {
            throw new InvalidOperationException("DataManager 확장의 실제 노드 조회 미연결");
        }
        Span<byte> bytes = hooks.Selection.Stream(handle);
        if (bytes.Length < NodeBytes)
        {
            throw OutOfRange("DataManager 확장의 실제60BYTE 누락");
        }
        return bytes;
    }

    /// <summary>원본 strcpy처럼 이름과 NUL만 쓰며 나머지 이름 40바이트는 초기화하지 않는다.</summary>
    /// <param name="node">60바이트 노드.</param>
    /// <param name="name">쓸 이름. 첫 NUL 이전만 쓴다.</param>
    private static void WriteName(Span<byte> node, string name)
    {
        name = CString(name);
        if (name.Length >= NameBytes)
        {
            throw OutOfRange("DataManager 실제 이름40BYTE 범위");
        }
        Latin1.GetBytes(name, node.Slice(NameOffset, name.Length));
        node[NameOffset + name.Length] = 0;
    }

    /// <summary>실제 접근하는 양의 범위만 조회한다. 길이 0은 잘못된 포인터를 역참조하지 않는다.</summary>
    /// <param name="hooks">버퍼 조회 장치.</param>
    /// <param name="pointer">버퍼 핸들.</param>
    /// <param name="position">버퍼 안 시작 위치.</param>
    /// <param name="length">필요한 길이.</param>
    private static Span<byte> Buffer(DataManagerResizeHooks hooks, uint pointer, uint position, uint length)
    {
        if (length == 0)
        {
            return default;
        }
        if (pointer == 0 || hooks.Memory.Buffer is null)
        {
            throw new InvalidOperationException("DataManager 확장의 실제 버퍼 조회 미연결");
        }
        Span<byte> bytes = hooks.Memory.Buffer(pointer);
        if (position > (uint)bytes.Length || length > (uint)bytes.Length - position)
        {
            throw OutOfRange("DataManager 확장의 실제 버퍼 물리 범위");
        }
        return bytes.Slice((int)position, (int)length);
    }

    /// <summary>
    /// 0044d070: 첫 16바이트 초기화→최소 1 할당→현재 이름과 NUL 복사→next 0 순서이며 같은 this 핸들을 반환한다.
    /// 이름이 없으면(원문 NULL) 첫 이름 바이트만 0으로 쓴다.
    /// </summary>
    /// <param name="stream">생성할 노드 핸들.</param>
    /// <param name="name">섹션 이름. null이면 빈 이름이다.</param>
    /// <param name="size">요청 크기(signed). 1보다 작으면 1이다.</param>
    /// <param name="hooks">노드·버퍼 장치.</param>
    public static uint ConstructStream(uint stream, string? name, int size, DataManagerResizeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Span<byte> node = Node(hooks, stream);
        node[..16].Clear();
        RawMemStream.Allocate(node, (uint)Math.Max(size, 1), hooks.Memory);
        if (name is not null)
        {
            WriteName(node, name);
        }
        else
        {
            node[NameOffset] = 0;
        }
        Put(node, NodeNext, 0);
        return stream;
    }

    /// <summary>
    /// 0044d1b0: 같은 이름을 제거한 뒤 새 60바이트 노드를 할당하고 16384 크기로 생성해 목록에 추가한다.
    /// 원본 생성 부모는 새 this가 아니라 마지막 추가 효과 뒤의 manager.current를 반환한다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="hooks">노드·목록·버퍼 장치.</param>
    public static uint CreateStream(Span<byte> manager, string name, DataManagerResizeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint existing = RawDataManagerSelection.FindSection(manager, name, hooks.Selection);
        if (existing != 0)
        {
            RawDataManagerList.Remove(manager, existing, hooks.List);
        }
        uint stream = Need(hooks.AllocateStream, "DataManager 섹션 생성의 실제 노드 할당 미연결")(NodeBytes);
        if (stream != 0)
        {
            ConstructStream(stream, name, DefaultSize, hooks);
        }
        RawDataManagerList.Append(manager, stream, hooks.List);
        return Read(manager, 8);
    }

    /// <summary>
    /// 0044d230: 다른 manager의 현재 원문을 찾고 대상 생성 뒤 원문 data가 있을 때만 대입한 뒤 pos를 0으로 되돌린다.
    /// 원문 노드는 생성 전의 같은 객체이며 그 data/end만 대상 생성이 끝난 뒤 다시 읽는다.
    /// </summary>
    /// <param name="manager">대상 DataManager 12바이트.</param>
    /// <param name="sourceManager">원문 DataManager 12바이트.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="hooks">노드·목록·버퍼 장치.</param>
    public static uint CloneStream(Span<byte> manager, Span<byte> sourceManager, string name, DataManagerResizeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint original = RawDataManagerSelection.FindSection(sourceManager, name, hooks.Selection);
        if (original == 0)
        {
            Need(hooks.MissingSource, "DataManager 복제의 원문 누락 진단 미연결")();
        }
        Span<byte> source = Node(hooks, original);
        uint stream = CreateStream(manager, name, hooks);
        Span<byte> target = Node(hooks, stream);
        if (Read(source, 0) != 0)
        {
            RawMemStream.Copy(target, source, hooks.Memory);
        }
        Put(target, 4, 0);
        return stream;
    }

    /// <summary>
    /// 0044d160: 대상 이름이 있으면 제거한 뒤 원래 이름을 찾고 대상 문자열과 NUL만 쓴다.
    /// 기존 대상의 반납 효과 뒤 원래 이름을 찾으므로 선행 current를 캐시하지 않는다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="name">바꿀 새 이름.</param>
    /// <param name="previousName">현재 이름.</param>
    /// <param name="hooks">노드·목록 장치.</param>
    public static uint RenameStream(Span<byte> manager, string name, string previousName, DataManagerResizeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint target = RawDataManagerSelection.FindSection(manager, name, hooks.Selection);
        if (target != 0)
        {
            RawDataManagerList.Remove(manager, target, hooks.List);
        }
        uint source = RawDataManagerSelection.FindSection(manager, previousName, hooks.Selection);
        WriteName(Node(hooks, source), name);
        return Read(manager, 8);
    }

    /// <summary>
    /// 0044d290: signed end가 최소보다 작을 때만 임시 생성→현재 원문 복사→남은 용량 0 채움→추가→이름 교체를 수행한다.
    /// 부모는 생성 전 원문 노드를 보관하되 실제 data/end는 생성이 끝난 뒤 다시 읽는다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="minimum">필요한 최소 end(signed).</param>
    /// <param name="hooks">노드·목록·버퍼 장치.</param>
    public static uint EnsureStreamSize(Span<byte> manager, string name, int minimum, DataManagerResizeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint original = RawDataManagerSelection.FindSection(manager, name, hooks.Selection);
        if (original == 0)
        {
            Need(hooks.MissingSource, "DataManager 최소 확장의 누락 진단 미연결")();
        }
        Span<byte> source = Node(hooks, original);
        if (unchecked((int)Read(source, 8)) >= minimum)
        {
            return Read(manager, 8);
        }
        uint allocated = Need(hooks.AllocateStream, "DataManager 임시 stream 실제 할당 미연결")(NodeBytes);
        if (allocated != 0)
        {
            ConstructStream(allocated, TemporaryName, minimum, hooks);
        }
        Span<byte> target = Node(hooks, allocated);
        uint length = Read(source, 8);
        RawMemStream.Write(target, Buffer(hooks, Read(source, 0), 0, length), length, hooks.Memory);
        uint remaining = Read(target, 12) - Read(target, 4);
        Buffer(hooks, Read(target, 0), Read(target, 4), remaining).Clear();
        uint end = Read(target, 12);
        Put(target, 4, end);
        if (unchecked((int)Read(target, 8)) < unchecked((int)end))
        {
            Put(target, 8, end);
        }
        RawDataManagerList.Append(manager, allocated, hooks.List);
        RenameStream(manager, name, TemporaryName, hooks);
        return Read(manager, 8);
    }
}
