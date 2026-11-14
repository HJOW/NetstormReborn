using Netstorm.Assets;

// 현재 Player의 실제 기술 내용물과40개 목록을 다루는 원본 코어다.
namespace Netstorm.Core.Original;

/// <summary>타입 범위 진단의 서로 다른 두 원문 위치다.</summary>
public enum PlayerTechnologyAssertion
{
    /// <summary>00490330의Find타입 범위 진단이다. 진단 뒤 조회는 계속한다.</summary>
    FindTypeRange,
    /// <summary>00490420의Ensure타입 범위 진단이다. 뒤의Find진단도 별도로 실행한다.</summary>
    EnsureTypeRange,
}

/// <summary>원본 DWORD목록의40개 용량·현재 개수·세미콜론 항목 번호를 보관한다.</summary>
public sealed class PlayerTechnologyList
{
    /// <summary>operator new(0xa0)의 실제40개 원소다. 개수 뒤 원소는 목록으로 소비하지 않는다.</summary>
    public int[] Types { get; } = new int[40];
    /// <summary>목록+8의 현재 개수이며 최대40이다.</summary>
    public uint Count { get; set; }
    /// <summary>004914e0의현재 항목 번호다.</summary>
    public int Index { get; set; }
}

/// <summary>기술 목록 parser의 실제 종료·진행·같은 항목 반복 결과다.</summary>
public enum PlayerTechnologyParseStep
{
    /// <summary>빈 항목을 만나 목록 생성이 끝났다.</summary>
    Complete,
    /// <summary>현재 항목 처리가 끝나 다음 번호로 진행했다.</summary>
    Progress,
    /// <summary>미등록 비빈 이름이라 원문이 같은 항목을 무한 반복한다.</summary>
    Stalled,
}

/// <summary>실제 TYPE조회·가상 내용물 삭제·진단을 공급한다.</summary>
public sealed class PlayerTechnologyHooks
{
    /// <summary>실제 이름표0049a860 또는RiftTypeTable.Find를 호출한다.</summary>
    public Func<string, int>? FindType { get; init; }
    /// <summary>진단 뒤 원문 진행을 계속하는 실제assert경계다.</summary>
    public Action<PlayerTechnologyAssertion>? Assertion { get; init; }
    /// <summary>같은raw내용물 체인을 변경하는 실제 가상삭제+10이다.</summary>
    public Action<int, uint>? Destroy { get; init; }
}

/// <summary>PATCH Player의 기술 조회·삭제·Ensure와40개 문자열 목록 사슬이다.
/// 근거: cpppj/src/o/RawPlayerTechnology.h/.cpp. 같은Player·contained finder/create를 재사용한다.</summary>
public sealed class RawPlayerTechnology
{
    /// <summary>현재9Player와PlayerKnown진단을 공유하는 부모다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>004ac9f0의실제 내용물 체인 조회다.</summary>
    private readonly RawCarrierPreDestroy _finder;
    /// <summary>기존Factory·Form·부모부착의 실제 생성 사슬이다.</summary>
    private readonly RawContainedCreate _create;
    /// <summary>all분류에서 직접 읽는 현재 번호순group표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>실제 이름 조회·가상 삭제·타입 진단이다.</summary>
    private readonly PlayerTechnologyHooks _hooks;

    /// <summary>같은PATCH풀을 사용하는 세 몸체와 실제 타입 조회를 빌린다.</summary>
    public RawPlayerTechnology(RawPlayerWorld player, RawCarrierPreDestroy finder, RawContainedCreate create,
        IReadOnlyList<OriginalTypeRecord> types, PlayerTechnologyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(create);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        if (player.Pool().Edition != OriginalEdition.Patch1078 || !ReferenceEquals(player.Pool(), finder.Pool)
            || !ReferenceEquals(player.Pool(), create.Pool) || types.Count == 0 || hooks.FindType is null)
            throw new ArgumentException("Player기술은 같은PATCH실제풀/타입조회가 필요합니다");
        _player = player; _finder = finder; _create = create; _types = types; _hooks = hooks;
    }

    /// <summary>도달한 실제 효과가 없으면 가짜 성공으로 대체하지 않고 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("Player기술 실제 효과 미연결");
    /// <summary>원본 타입 범위assert후 계속 진행한다. 하위raw몸체가 물리 범위를 별도로 검사한다.</summary>
    private void CheckType(int type, PlayerTechnologyAssertion where)
    {
        if (type < 1 || type >= _types.Count) Need(_hooks.Assertion)(where);
    }
    /// <summary>PlayerKnown진단 뒤 현재 기술부모DWORD를 같은풀의SID폭으로 확인한다.</summary>
    private int Parent(int owner)
    {
        uint parent = _player.Technology(owner);
        if (parent > ushort.MaxValue || parent >= _player.Pool().Capacity)
            throw new ArgumentOutOfRangeException(nameof(owner), "Player기술 실제 부모SID범위 오류");
        return (int)parent;
    }

    /// <summary>00490330의타입 진단→현재부모→flags4/kind일치 첫SID조회다.</summary>
    public int Find(int owner, int type)
    {
        CheckType(type, PlayerTechnologyAssertion.FindTypeRange);
        return _finder.FindContained(Parent(owner), 4, unchecked((uint)type), 0);
    }

    /// <summary>004903b0처럼 부모를 한 번 캡처하고 삭제 후 같은 체인에서 다음flags4내용물을 다시 찾는다.</summary>
    public void Clear(int owner)
    {
        int parent = Parent(owner);
        // 실제 가상삭제는 해당 내용물을 같은 체인에서 제거해야 한다. 하위 실패는 그대로 보고한다.
        while (true)
        {
            int found = _finder.FindContained(parent, 4, 0, 0);
            if (found == 0) return;
            Need(_hooks.Destroy)(found, 0);
        }
    }

    /// <summary>00490420의중첩 타입진단/Find후 없으면 현재부모를 다시 읽어(0,4,10,0)으로생성한다.</summary>
    public void Ensure(int owner, int type)
    {
        CheckType(type, PlayerTechnologyAssertion.EnsureTypeRange);
        if (Find(owner, type) == 0) _create.Create(Parent(owner), unchecked((uint)type), 0, 4, 0x10, 0);
    }

    /// <summary>용량 확인→기존항목 중복검사→끝추가 순서다. 가득 차면 중복검사를 생략한다.</summary>
    private static void Append(PlayerTechnologyList list, int type)
    {
        if (list.Count >= list.Types.Length) return;
        // 활성 개수 안에서만 같은 항목을 찾는다.
        for (int index = 0; index < list.Count; index++) if (list.Types[index] == type) return;
        list.Types[list.Count++] = type;
    }

    /// <summary>004914e0의한반복이다. 미등록 비빈 이름에서 항목 번호를 증가시키지 않는다.</summary>
    public PlayerTechnologyParseStep ParseStep(string text, PlayerTechnologyList list)
    {
        ArgumentNullException.ThrowIfNull(text);
        ArgumentNullException.ThrowIfNull(list);
        if (list.Count > list.Types.Length) throw new ArgumentOutOfRangeException(nameof(list), "Player기술목록 실제40개 범위 오류");
        int previous = list.Index;
        string value = ConfigNumbers.ListItem(text, list.Index);
        int type = _hooks.FindType!(value);
        if (value.Length == 3 && value[0] is 'a' or 'A' && value[1] is 'l' or 'L' && value[2] is 'l' or 'L')
        {
            // 원문처럼0번부터 전체TYPE을 순서대로 보고group10만 제외한다.
            for (int index = 0; index < _types.Count; index++) if (_types[index].Group != 10) Append(list, index);
            list.Index = unchecked(list.Index + 1);
        }
        if (type > 0) { Append(list, type); list.Index = unchecked(list.Index + 1); }
        if (value.Length == 0) return PlayerTechnologyParseStep.Complete;
        return list.Index == previous ? PlayerTechnologyParseStep.Stalled : PlayerTechnologyParseStep.Progress;
    }

    /// <summary>004915c0의목록을 완성한 뒤 순서대로Ensure한다. 원본 무한반복 입력은 생성 전에 명시 거부한다.</summary>
    public void AddList(int owner, string text)
    {
        var list = new PlayerTechnologyList();
        // 미등록 항목을 건너뛰어 성공으로 처리하지 않고 원문의 무진행을 보고한다.
        while (true)
        {
            PlayerTechnologyParseStep step = ParseStep(text, list);
            if (step == PlayerTechnologyParseStep.Complete) break;
            if (step == PlayerTechnologyParseStep.Stalled) throw new InvalidOperationException("원본004914e0이 미등록 기술 이름에서 같은 항목을 무한 반복합니다");
        }
        // 먼저 완성한 최대40개 목록을 바꾸지 않고 실제Ensure에 전달한다.
        for (int index = 0; index < list.Count; index++) Ensure(owner, list.Types[index]);
    }

    /// <summary>연결 어댑터에서 같은raw세계인지 확인한다.</summary>
    public RawSidPool Pool() => _player.Pool();

    /// <summary>실제RiftTypeTable.Find를 직접 연결하고 기존 진단·삭제 경계를 보존한다.</summary>
    public static PlayerTechnologyHooks BindPlayerTechnologyTypes(RiftTypeTable types, PlayerTechnologyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        return new PlayerTechnologyHooks { FindType = types.Find, Assertion = hooks.Assertion, Destroy = hooks.Destroy };
    }
}
