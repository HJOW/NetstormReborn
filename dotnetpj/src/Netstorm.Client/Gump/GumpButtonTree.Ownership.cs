namespace Netstorm.Client.Gump; // 같은 공통 트리의 호스트 수명과 소유 문자열 영역이다.

// 공통 트리의 호스트 자식 수명과 원문 소유 문자열을 한 저장소에 보관한다.
/// <summary>RawMenuControls.cpp의 공통 소유 표/SetNodeHelp/문자열 반납을 같은 실제 트리에 연결한다.</summary>
public sealed partial class GumpButtonTree
{
    /// <summary>Native 게임 필드가 아닌 동기 생성 범위의 현재 호스트 소유자다.</summary>
    private object? _activeOwner;
    /// <summary>등록 전 부분 생성까지 포함하는 실제 노드별 호스트 소유자다.</summary>
    private readonly Dictionary<uint, object> _nodeOwners = [];
    /// <summary>공통 기본 소멸자가 반납하는 실제 소유 제목/도움말 문자열이다.</summary>
    private readonly Dictionary<uint, string> _helpStrings = [];
    /// <summary>정적61000001..5/기본HTML61010000/전용HTML과 구별하는 CPP의 동일 별칭 시작이다.</summary>
    private uint _nextHelpString = 0x61100000;
    /// <summary>다른 UI의 동기 생성에서 같은 저장소/공통 노드는 공유하되 닫기 소유권은 구별한다.</summary>
    public T WithNodeOwner<T>(object owner, Func<T> action)
    {
        ArgumentNullException.ThrowIfNull(owner); ArgumentNullException.ThrowIfNull(action); object? previous = _activeOwner; _activeOwner = owner;
        try { return action(); } finally { _activeOwner = previous; }
    }
    /// <summary>실제 할당 당시의 호스트 소유자만 조회하며 Native 노드 필드는 바꾸지 않는다.</summary>
    public object? NodeOwner(uint token) => _nodeOwners.GetValueOrDefault(token);
    /// <summary>부분 생성 정리용 같은 소유자의 현재 살아 있는 토큰 사본이다.</summary>
    public uint[] OwnedNodes(object owner)
    {
        ArgumentNullException.ThrowIfNull(owner); return _nodeOwners.Where(pair => ReferenceEquals(pair.Value, owner)).Select(pair => pair.Key).ToArray();
    }
    /// <summary>공통 소멸자로 아직 반납하지 않은 실제 소유 문자열 수다.</summary>
    public int OwnedTextCount => _helpStrings.Count;
    /// <summary>같은 현재 소유 문자열을 읽고 해제/미등록 토큰을 명시 거부한다.</summary>
    public string ReadOwnedText(uint token) => _helpStrings.TryGetValue(token, out string? text) ? text : throw new ArgumentOutOfRangeException(nameof(token), "Gump 현재 소유 문자열 없음");
    /// <summary>기존 반납/현재 포인터0/새 별칭 게시의 원문 순서로 제목 또는 도움말 키를 교체한다.</summary>
    private void SetNodeHelp(uint token, string text, bool word)
    {
        ArgumentNullException.ThrowIfNull(text); var node = Node(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "도움말 실제 노드 없음"); uint pointer = word ? node.AlternateTextPointer : node.TextPointer; _helpStrings.Remove(pointer); if (word) node.AlternateTextPointer = 0; else node.TextPointer = 0;
        if (_nextHelpString == 0x61ffffff) throw new OverflowException("도움말 문자열 주소 영역 소진"); uint value = ++_nextHelpString; _helpStrings.Add(value, text); if (word) node.AlternateTextPointer = value; else node.TextPointer = value;
    }
    /// <summary>같은 기본+78의 실제 소유 제목만 교체한다.빈 값도 원문처럼 새 문자열을 소유한다.</summary>
    public void SetHelpText(uint token, string text) => SetNodeHelp(token, text, false);
    /// <summary>같은 기본+7c의 실제 소유 도움말 키만 교체한다.</summary>
    public void SetHelpWord(uint token, string text) => SetNodeHelp(token, text, true);
}
