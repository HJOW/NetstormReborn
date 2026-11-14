// 같은 물리 TYPE 기록과 현재 전역을 원문 순서로 읽는다.
namespace Netstorm.Core.Original;

/// <summary>HP getter가 소비 위치마다 읽는 checking·약화·현재 TYPE 주소다.</summary>
public sealed class RiftHitPointGlobals
{
    /// <summary>PATCH에서 HP 플래그 진단을 고르는 현재 DWORD다.</summary>
    public RawCheckingFlag Checking { get; init; } = new();
    /// <summary>사제 HP를 1/4로 읽는 현재 비영 관문이다.</summary>
    public RawCheckingFlag Quarter { get; init; } = new();
    /// <summary>번호 계산 뒤 읽는 같은 현재 사제 TYPE다.</summary>
    public required Func<uint> Priest { get; init; }
    /// <summary>PATCH의 번호 계산 뒤 읽는 같은 현재 다이스 TYPE다.</summary>
    public required Func<uint> Dais { get; init; }
}

/// <summary>선택된 시점에만 요구하는 실제 번호 계산·복귀 가능한 진단이다.</summary>
public sealed class RiftHitPointHooks
{
    /// <summary>저장된 물리 this와 호출 당시의 현재 TYPE base로 번호를 계산한다.</summary>
    public Func<uint>? Number { get; set; }
    /// <summary>원문 식·파일·줄을 전달하며 정상 복귀하면 이후 전역을 다시 읽는다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>PATCH0049fce0/CD00444720의 TYPE HP getter와 현재 물리 표 번호 계산이다.</summary>
public static class RawRiftHitPoints
{
    /// <summary>원문1e3의 HP 플래그 예외 진단 식이다.</summary>
    public const string Expression = "(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType";
    /// <summary>원문 진단의 대소문자까지 보존한 소스 경로다.</summary>
    public const string Source = "\\Ns\\O\\Rifttype.cpp";

    /// <summary>진단 뒤 현재 약화·번호를 다시 읽고 처음 선택한 물리 기록의 현재 HP를 반환한다.</summary>
    public static int Read(OriginalEdition edition, OriginalTypeRecord type, RiftHitPointGlobals globals, RiftHitPointHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(type); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        return ReadSelected(edition, () => type, globals, hooks);
    }

    /// <summary>같은 물리 칸을 조회하며 관리 레코드 교체도 같은 주소의 필드 갱신으로 해석한다.</summary>
    internal static int ReadSelected(OriginalEdition edition, Func<OriginalTypeRecord> type, RiftHitPointGlobals globals, RiftHitPointHooks hooks)
    {
        if (edition == OriginalEdition.Patch1078 && globals.Checking && (type().Flags1 & 0x10) == 0)
        {
            uint number = (hooks.Number ?? throw new InvalidOperationException("TYPE HP 실제 번호 장치 미연결"))();
            if (number != globals.Dais())
                (hooks.Assertion ?? throw new InvalidOperationException("TYPE HP 실제 진단 장치 미연결"))(Expression, Source, 0x1e3);
        }
        bool quarter = false;
        if (globals.Quarter)
        {
            uint number = (hooks.Number ?? throw new InvalidOperationException("TYPE HP 실제 번호 장치 미연결"))();
            quarter = number == globals.Priest();
        }
        return quarter ? type().MaxHitPoints / 4 : type().MaxHitPoints;
    }

    /// <summary>전체 표와 명시 배열 뷰를 같은 번호 계산에 연결한다. 다른 할당을 레코드 값으로 추측하지 않는다.</summary>
    public static uint CurrentRiftTypeNumber(int recordIndex, IReadOnlyList<OriginalTypeRecord> selectedBase,
        IReadOnlyList<OriginalTypeRecord> currentBase)
    {
        if (ReferenceEquals(selectedBase, currentBase))
        {
            if (recordIndex < 0 || recordIndex >= selectedBase.Count)
                throw new ArgumentOutOfRangeException(nameof(recordIndex));
            return unchecked((uint)recordIndex);
        }
        return CurrentRiftTypeNumber(recordIndex, PhysicalView(selectedBase), PhysicalView(currentBase));
    }

    /// <summary>관리 배열과 부분 뷰의 실제 소유자·시작 위치를 구한다.</summary>
    private static ArraySegment<OriginalTypeRecord> PhysicalView(IReadOnlyList<OriginalTypeRecord> types) => types switch
    {
        OriginalTypeRecord[] array => new(array),
        ArraySegment<OriginalTypeRecord> segment => segment,
        _ => throw new InvalidOperationException("TYPE HP의 서로 다른 물리 표 주소 연결"),
    };

    /// <summary>같은 물리 배열의 겹치는 두 뷰로 저장된 this 번호를 계산한다. 음수도 DWORD로 보존한다.</summary>
    /// <remarks>C# 클래스 참조의 중복 여부로 주소를 추정하지 않고 선택 당시의 배열 인덱스를 보관한다.</remarks>
    public static uint CurrentRiftTypeNumber(int recordIndex, ArraySegment<OriginalTypeRecord> selectedBase,
        ArraySegment<OriginalTypeRecord> currentBase)
    {
        if (selectedBase.Count == 0 || currentBase.Count == 0 || recordIndex < 0 || recordIndex >= selectedBase.Count)
            throw new ArgumentOutOfRangeException(nameof(recordIndex), "TYPE HP 표의 물리 주소 범위");
        if (!ReferenceEquals(selectedBase.Array, currentBase.Array)
            || selectedBase.Offset >= currentBase.Offset + currentBase.Count
            || currentBase.Offset >= selectedBase.Offset + selectedBase.Count)
            throw new InvalidOperationException("TYPE HP의 서로 다른 물리 표 주소 연결");
        return unchecked((uint)(selectedBase.Offset + recordIndex - currentBase.Offset));
    }
}
