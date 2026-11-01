using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>실제 Datamanager.cpp·Snart.cpp 진단이다(진단 번호가 곧 값이다).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public enum DataManagerInitializeAssertion : uint
{
    /// <summary>초기화하려는 manager의 root가 비어 있지 않다(Datamanager.cpp 0xc9).</summary>
    RootNotEmpty = 0xc9,

    /// <summary>SNART 이름표 첫 구분자가 '/'가 아니다(Snart.cpp 0x46).</summary>
    NameSeparator = 0x46,
}

/// <summary>선택한 SNART 이름의 시작부터 같은 물리 끝까지를 받는 소비자다(실제 0044d1b0에 고정 manager를 연결한다).</summary>
/// <param name="name">이름 시작 포인터와 물리 범위.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void SnartNameConsumer(ReadOnlySpan<byte> name);

/// <summary>신규 manager 초기화가 쓰는 외부 장치 연결이다(원본 <c>DataManagerInitializeHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerInitializeHooks
{
    /// <summary>진단에서 반환하면 그 뒤 현재 표를 얻는다.</summary>
    public Action<DataManagerInitializeAssertion>? Assertion { get; set; }

    /// <summary>가상 +4의 현재 실제 표다. 한 번만 호출한다.</summary>
    public SnartNamesLookup? Names { get; set; }

    /// <summary>같은 이름 시작 포인터와 물리 범위로 실제 0044d1b0에 고정 manager를 연결해 큰 stream을 만든다.</summary>
    public SnartNameConsumer? Create { get; set; }
}

/// <summary>
/// 신규 DataManager 초기화(원문 0044d870)다. 진단 뒤 받은 이름표 포인터와 고정 manager를 끝까지 보존하며,
/// 초기화 앞뒤의 외부 효과를 되돌리거나 표를 사전 문자열 목록으로 복사하지 않는다. cpppj <c>o/RawDataManagerInitialize</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerInitialize
{
    /// <summary>현재 고정 this의 +4 DWORD(root)를 읽는다. 물리 범위가 모자라면 거부한다.</summary>
    /// <param name="manager">DataManager 바이트.</param>
    private static uint Root(ReadOnlySpan<byte> manager)
    {
        if (manager.Length < 8)
        {
            throw OutOfRange("DataManager 초기화의 실제 root 물리 범위");
        }
        return Read(manager, 4);
    }

    /// <summary>
    /// root 검사→이름표 1회 조회→현재 이름/실제 Create→마지막 빈 이름에서 반환한다.
    /// 진단이 owner/표를 바꿔도 그 뒤 위치에서 조회하고, 각 Create 뒤 같은 표에서 이후 이름의 내용을 다시 읽는다.
    /// </summary>
    /// <param name="manager">초기화할 DataManager.</param>
    /// <param name="hooks">진단·이름표·생성 장치.</param>
    public static void Initialize(Span<byte> manager, DataManagerInitializeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (Root(manager) != 0)
        {
            Need(hooks.Assertion, "DataManager 초기화의 선택된 실제 하위 미연결")(DataManagerInitializeAssertion.RootNotEmpty);
        }
        Span<byte> names = Need(hooks.Names, "DataManager 초기화의 선택된 실제 하위 미연결")();
        // 반환 표 포인터는 고정하되 미래 이름의 내용은 각 Create 뒤 같은 표에서 다시 읽는다
        for (uint index = 0; ; index++)
        {
            Span<byte> name = RawSnart.SelectName(names, index, () =>
                Need(hooks.Assertion, "DataManager 초기화의 선택된 실제 하위 미연결")(DataManagerInitializeAssertion.NameSeparator));
            if (name[0] == 0)
            {
                return;
            }
            Need(hooks.Create, "DataManager 초기화의 선택된 실제 하위 미연결")(name);
        }
    }
}
