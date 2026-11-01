using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>원본 진단 두 종류다. 진단 뒤 현재 포인터를 다시 읽는 지점도 구분한다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public enum DataManagerSelectionAssertion
{
    /// <summary>선택한 이름의 섹션이 없다(manager.current가 0).</summary>
    MissingCurrent,

    /// <summary>읽기 선택인데 버퍼가 비었거나 end가 0이다.</summary>
    EmptyRead,
}

/// <summary>
/// DataManager 섹션 검색·선택이 쓰는 외부 장치 연결이다(원본 <c>DataManagerSelectionHooks</c>).
/// 노드는 호스트 포인터를 DWORD로 자르지 않고 현재 60바이트의 불투명 핸들로 찾는다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerSelectionHooks
{
    /// <summary>현재 노드의 16바이트 MemStream·이름 40바이트·next DWORD를 빌린다.</summary>
    public RawBytesLookup? Stream { get; set; }

    /// <summary>실제 CRT 비교 경계다. 생략하면 공식 ASCII 이름 비교(<see cref="RawDataManagerSelection.CompareSectionNames"/>)를 쓴다.</summary>
    public Func<string, string, int>? Compare { get; set; }

    /// <summary>선택된 원본 진단만 호출한다. 이후 호출 본문이 current를 다시 읽는다.</summary>
    public Action<DataManagerSelectionAssertion>? Assertion { get; set; }
}

/// <summary>
/// DataManager의 같은 목록·current를 쓰는 섹션 검색(0044ce60)과 선택(0044ceb0)이다.
/// 진단·비교 뒤의 현재 포인터 재조회와 다른 노드·필드 보존을 원본 순서대로 처리한다. cpppj <c>o/RawDataManagerSelection</c>을 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerSelection
{
    /// <summary>DataManager 한 개의 실제 최소 물리 폭이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>노드 안 이름 시작 위치다.</summary>
    private const int NameOffset = 16;

    /// <summary>노드 안 이름의 실제 물리 폭이다.</summary>
    private const int NameBytes = 40;

    /// <summary>노드 안 next DWORD 위치다.</summary>
    private const int NodeNext = 0x38;

    /// <summary>동일 노드의 실제 최소 폭을 확인하고 원본의 잘못된 메모리를 성공으로 바꾸지 않는다.</summary>
    /// <param name="handle">노드 핸들. 0이면 거부한다.</param>
    /// <param name="hooks">노드 조회 장치.</param>
    private static Span<byte> Node(uint handle, DataManagerSelectionHooks hooks)
    {
        if (handle == 0 || hooks.Stream is null)
        {
            throw new InvalidOperationException("DataManager 현재 노드/조회 연결 누락");
        }
        Span<byte> bytes = hooks.Stream(handle);
        if (bytes.Length < NodeBytes)
        {
            throw OutOfRange("DataManager 실제60BYTE 노드 누락");
        }
        return bytes;
    }

    /// <summary>노드 이름을 첫 NUL까지 읽는다. 물리적 이름 40바이트 밖 읽기는 허용하지 않는다.</summary>
    /// <param name="node">60바이트 노드.</param>
    private static string Name(ReadOnlySpan<byte> node)
    {
        return TerminatedText(node.Slice(NameOffset, NameBytes), "DataManager 이름40BYTE 내 NUL 누락");
    }

    /// <summary>필요한 진단을 생략하지 않는다. 반환한 뒤의 실제 저장소는 호출 본문이 다시 조회한다.</summary>
    /// <param name="hooks">진단 장치.</param>
    /// <param name="kind">진단 종류.</param>
    private static void Assert(DataManagerSelectionHooks hooks, DataManagerSelectionAssertion kind)
    {
        Need(hooks.Assertion, "DataManager 선택의 실제 진단 미연결")(kind);
    }

    /// <summary>CRT의 NUL 이전 공식 ASCII 이름만 지원하는 한 글자 소문자 변환이다. 현재 프로세스 로캘에 의존하지 않는다.</summary>
    /// <param name="value">비교할 글자(바이트값).</param>
    private static int Lower(int value)
    {
        if (value >= 128)
        {
            throw new ArgumentException("DataManager 비ASCII 이름 CRT 로캘 미복원");
        }
        return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
    }

    /// <summary>
    /// 공식 섹션 이름의 ASCII 대소문자 무시 비교(CRT stricmp 경계)다. 첫 차이의 부호와 NUL 종료를 보존하며 비ASCII는 명시적으로 거부한다.
    /// </summary>
    /// <param name="first">첫째 이름.</param>
    /// <param name="second">둘째 이름.</param>
    public static int CompareSectionNames(string first, string second)
    {
        first = CString(first);
        second = CString(second);
        int common = Math.Min(first.Length, second.Length);
        // 매 바이트를 읽는 순서대로 비교한다
        for (int index = 0; index < common; index++)
        {
            int left = Lower(first[index]);
            int right = Lower(second[index]);
            if (left != right)
            {
                return left > right ? 1 : -1;
            }
        }
        return first.Length == second.Length ? 0 : first.Length > second.Length ? 1 : -1;
    }

    /// <summary>
    /// 0044ce60: head에서 첫 이름 일치를 찾는다. 실패하면 manager.current는 0이다. 각 비교 뒤 현재 포인터를 다시 조회한다.
    /// 비교에서 current가 바뀌어도 캡처한 노드 대신 원본처럼 현재 값으로 진행한다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="name">찾을 섹션 이름.</param>
    /// <param name="hooks">노드 조회·비교 장치.</param>
    /// <returns>찾은 노드 핸들. 없으면 0.</returns>
    public static uint FindSection(Span<byte> manager, string name, DataManagerSelectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("DataManager 실제12BYTE 누락");
        }
        Put(manager, 8, Read(manager, 4));
        // 원본처럼 현재 포인터가 0이 될 때 실패하고 첫 일치에서 반환한다
        while (Read(manager, 8) != 0)
        {
            Span<byte> node = Node(Read(manager, 8), hooks);
            string currentName = Name(node);
            int comparison = hooks.Compare is { } compare ? compare(name, currentName) : CompareSectionNames(name, currentName);
            if (comparison == 0)
            {
                return Read(manager, 8);
            }
            Put(manager, 8, Read(Node(Read(manager, 8), hooks), NodeNext));
        }
        return 0;
    }

    /// <summary>
    /// 0044ceb0: 검색한 뒤 필요한 진단을 거쳐 현재 노드의 pos만 0으로 쓴다. writable이 0이면 빈 버퍼를 진단한다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="name">선택할 섹션 이름.</param>
    /// <param name="writable">쓰기 허용 여부(비영이면 허용).</param>
    /// <param name="hooks">노드 조회·비교·진단 장치.</param>
    /// <returns>진단 이후의 현재 노드 핸들.</returns>
    public static uint SelectSection(Span<byte> manager, string name, int writable, DataManagerSelectionHooks hooks)
    {
        FindSection(manager, name, hooks);
        if (Read(manager, 8) == 0)
        {
            Assert(hooks, DataManagerSelectionAssertion.MissingCurrent);
        }
        if (writable == 0)
        {
            Span<byte> node = Node(Read(manager, 8), hooks);
            if (Read(node, 0) == 0 || Read(node, 8) == 0)
            {
                Assert(hooks, DataManagerSelectionAssertion.EmptyRead);
            }
        }
        Put(Node(Read(manager, 8), hooks), 4, 0);
        return Read(manager, 8);
    }
}
