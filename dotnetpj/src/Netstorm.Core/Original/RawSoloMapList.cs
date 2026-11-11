using System.Text;

namespace Netstorm.Core.Original;

/// <summary>Client 수명의 맵 이름·플래그 캐시와 현재 개수다. 원문은 고정 50칸이지만 5차의 무제한 목록을 위해 늘어난다.</summary>
public sealed class SoloMapListState
{
    public List<string?> Names { get; } = Enumerable.Repeat<string?>(null, 50).ToList();
    public List<uint> Flags { get; } = Enumerable.Repeat(0u, 50).ToList();
    public int Count { get; set; }
    public uint Dirty { get; set; }
    /// <summary>5차: 열거해 담을 최대 개수다. 원문 50이며 Edit 목록의 무제한 요구는 메뉴 소유자가 int32 최대로 올린다.</summary>
    /// <remarks>2026-11-11 0.363에서 추가했다.</remarks>
    public int Limit { get; set; } = 50;
    /// <summary>스크롤 표시 창의 첫 목록 번호다. 새로 연 목록은 첫 창에서 시작한다.</summary>
    /// <remarks>2026-11-11 0.363에서 5차 스크롤 표시를 위해 추가했다.</remarks>
    public int WindowFirst { get; set; }
    /// <summary>스크롤 이동 뒤 재표시 표시다. 참이면 목록을 다시 열거하지 않고, 읽을 때 소비한다.</summary>
    /// <remarks>2026-11-11 0.363에서 5차 스크롤 표시를 위해 추가했다.</remarks>
    public bool KeepCache { get; set; }
}

/// <summary>맵 파일·임시 Template과 현재 사용자 및 번역 장치다.</summary>
public sealed class SoloMapListHooks
{
    public Func<int>? MayEditMissions { get; init; }
    public Func<IReadOnlyList<string>>? Enumerate { get; init; }
    public Func<string, byte>? Kind { get; init; }
    public Action? Construct { get; init; }
    public Action<string>? Load { get; init; }
    public Func<bool>? Present { get; init; }
    public Func<string>? Subscriber { get; init; }
    public Func<byte>? StateByte { get; init; }
    public Func<string>? UserName { get; init; }
    public Func<uint>? Reserved { get; init; }
    public Func<string, string>? Translate { get; init; }
}

/// <summary>원본 메뉴에 표시할 맵의 현재 레이블과 선택 결과다.</summary>
public sealed record SoloMapRow(string Label, string Name, bool Checked, bool Enabled);

/// <summary>원본 맵 목록의 파일 분류·50칸 캐시·행 표시다. 근거: cpppj RawSoloMapList.</summary>
public sealed class RawSoloMapList
{
    private readonly SoloMapListState _state;
    private readonly SoloMapListHooks _hooks;
    public RawSoloMapList(SoloMapListState state, SoloMapListHooks hooks) { _state = state; _hooks = hooks; }
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("단독 맵 목록의 실제 하위 미연결");
    private int Index(int value) => value >= 0 && value < _state.Names.Count ? value : throw new ArgumentOutOfRangeException(nameof(value));
    private string Name(int value) => _state.Names[Index(value)] ?? throw new InvalidOperationException("단독 맵 목록의 현재 이름 포인터 누락");
    private static string CString(string value) { int nul = value.IndexOf('\0'); return nul < 0 ? value : value[..nul]; }
    private static bool Same(string left, string right)
    {
        left = CString(left); right = CString(right); if (left.Length != right.Length) return false;
        for (int i = 0; i < left.Length; i++) { char a = left[i], b = right[i]; if (a is >= 'A' and <= 'Z') a = (char)(a + 32); if (b is >= 'A' and <= 'Z') b = (char)(b + 32); if (a != b) return false; }
        return true;
    }

    /// <summary>현재 dirty·설정과 실제 파일 열거를 사용해 원본 순서로 캐시한다. 한도가 50을 넘으면 필요한 칸을 늘린다.</summary>
    public int Refresh(int mode)
    {
        if (_state.Dirty != 0)
        {
            // 원본 50개 포인터 순서로 소유 문자열과 같은 flags를 정리한다. 5차 무제한 목록은 늘어난 칸도 같은 규칙으로 정리한다.
            for (int i = 0; i < _state.Names.Count; i++) if (_state.Names[i] != null) { _state.Names[i] = null; _state.Flags[i] = 0; }
            // 늘어난 칸은 모두 비웠으므로 원래 크기로 되돌린다
            if (_state.Names.Count > 50) { _state.Names.RemoveRange(50, _state.Names.Count - 50); _state.Flags.RemoveRange(50, _state.Flags.Count - 50); }
            _state.Count = 0; _state.Dirty = 0;
        }
        int mayEdit = Need(_hooks.MayEditMissions)(); if (_state.Names[0] != null) return _state.Count;
        // 원본 열거 순서대로 검사하며 수락한 이름만 limit(원문 50, 5차 무제한)개까지 담는다
        foreach (string file in Need(_hooks.Enumerate)())
        {
            if (_state.Count >= _state.Limit) break; byte kind = Need(_hooks.Kind)(file); int selected = _state.Count;
            if (kind != mode && !(mayEdit != 0 && mode == 2 && kind == 1)) continue;
            // 5차: limit이 50을 넘을 때만 새 칸을 만든다
            while (_state.Names.Count <= selected) { _state.Names.Add(null); _state.Flags.Add(0); }
            int slot = Index(selected); _state.Names[slot] = CString(file); Need(_hooks.Construct)(); Need(_hooks.Load)(Name(_state.Count));
            if (!Need(_hooks.Present)()) _state.Flags[slot] |= 1;
            else
            {
                string subscriber = Need(_hooks.Subscriber)(); if ((Need(_hooks.StateByte)() & 1) != 0) _state.Flags[slot] |= 2;
                if (!Same(subscriber, Need(_hooks.UserName)())) _state.Flags[slot] |= 4;
                if (Need(_hooks.Reserved)() != 0) _state.Flags[slot] |= 8;
            }
            _state.Count = unchecked(_state.Count + 1);
        }
        return _state.Count;
    }

    /// <summary>현재 캐시와 현재 선택 이름에서 메뉴 한 행을 만든다.</summary>
    public SoloMapRow? Row(int index, string current, string fallback)
    {
        if (index >= _state.Count) return null; string name = Name(index); current = CString(current);
        bool check = (current.Length == 0 && Same(name, fallback)) || Same(name, current); uint flags = _state.Flags[Index(index)];
        string pattern = (flags & 1) != 0 ? "~r%s" : (flags & 2) != 0 ? Need(_hooks.Translate)("%s (~rCORRUPT~.)") : "%s";
        name = Name(index); var output = new StringBuilder(); bool used = false;
        for (int i = 0; i < pattern.Length; i++)
        {
            if (pattern[i] != '%') { output.Append(pattern[i]); continue; }
            if (++i >= pattern.Length) throw new InvalidOperationException("단독 맵 행 원본 서식 오류");
            if (pattern[i] == '%') output.Append('%');
            else if (pattern[i] == 's' && !used) { output.Append(CString(name)); used = true; }
            else throw new InvalidOperationException("단독 맵 행 원본 서식 오류");
        }
        string label = output.ToString();
        if (Encoding.Latin1.GetByteCount(label) >= 256) throw new InvalidOperationException("단독 맵 목록 행 버퍼 범위");
        return new(label, name, check, (flags & 8) == 0);
    }
}
