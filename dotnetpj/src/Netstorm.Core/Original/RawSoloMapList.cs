using System.Text;

namespace Netstorm.Core.Original;

/// <summary>Client 수명의 맵 이름·플래그 50칸과 현재 캐시다.</summary>
public sealed class SoloMapListState
{
    public string?[] Names { get; } = new string?[50];
    public uint[] Flags { get; } = new uint[50];
    public int Count { get; set; }
    public uint Dirty { get; set; }
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
    private int Index(int value) => value >= 0 && value < 50 ? value : throw new ArgumentOutOfRangeException(nameof(value));
    private string Name(int value) => _state.Names[Index(value)] ?? throw new InvalidOperationException("단독 맵 목록의 현재 이름 포인터 누락");
    private static string CString(string value) { int nul = value.IndexOf('\0'); return nul < 0 ? value : value[..nul]; }
    private static bool Same(string left, string right)
    {
        left = CString(left); right = CString(right); if (left.Length != right.Length) return false;
        for (int i = 0; i < left.Length; i++) { char a = left[i], b = right[i]; if (a is >= 'A' and <= 'Z') a = (char)(a + 32); if (b is >= 'A' and <= 'Z') b = (char)(b + 32); if (a != b) return false; }
        return true;
    }

    /// <summary>현재 dirty·설정과 실제 파일 열거를 사용해 원본 순서로 캐시한다.</summary>
    public int Refresh(int mode)
    {
        if (_state.Dirty != 0)
        {
            for (int i = 0; i < 50; i++) if (_state.Names[i] != null) { _state.Names[i] = null; _state.Flags[i] = 0; }
            _state.Count = 0; _state.Dirty = 0;
        }
        int mayEdit = Need(_hooks.MayEditMissions)(); if (_state.Names[0] != null) return _state.Count;
        foreach (string file in Need(_hooks.Enumerate)())
        {
            if (_state.Count >= 50) break; byte kind = Need(_hooks.Kind)(file); int selected = _state.Count;
            if (kind != mode && !(mayEdit != 0 && mode == 2 && kind == 1)) continue;
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
