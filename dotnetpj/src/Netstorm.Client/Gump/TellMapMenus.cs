using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>Tell 맵 목록이 현재 열·파일 키와 캐시를 사용하는 장치다.</summary>
public sealed class TellMapMenuHooks
{
    public Action<uint>? BeginVertical { get; init; }
    public Action? Invalidate { get; init; }
    public Action<bool>? SelectFileSpec { get; init; }
    public Action<int>? Refresh { get; init; }
    public Func<int>? Count { get; init; }
    public Func<int, bool>? AppendRow { get; init; }
}

/// <summary>원본 Tell 맵 목록의 열 생성·현재 캐시 재조회와 키 복구다.</summary>
public static class TellMapMenus
{
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Tell 맵 목록의 현재 장치 미연결");

    /// <summary>현재 열 수에 맵 행을 추가한다. 비맵 레이블은 false로 반환한다.</summary>
    public static bool Append(string label, ref uint columns, TellMapMenuHooks hooks)
    {
        if (columns == 0) { Need(hooks.BeginVertical)(0x40000); columns = unchecked(columns + 1); }
        string lower = string.Concat(label.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
        bool solo = lower == "*sololist*"; if (!solo && lower != "*fortlist*") return false;
        Need(hooks.Invalidate)(); Need(hooks.SelectFileSpec)(solo); Need(hooks.Refresh)(solo ? 2 : 0);
        int index = 0;
        while (Need(hooks.AppendRow)(index))
        {
            index = unchecked(index + 1); int count = Need(hooks.Count)();
            if (count > 15 && index == count / 2) { Need(hooks.BeginVertical)(0x40000); columns = unchecked(columns + 1); }
        }
        if (solo) { Need(hooks.Invalidate)(); Need(hooks.SelectFileSpec)(false); }
        return true;
    }

    /// <summary>같은 캐시·파일 키·현재 선택을 Tell 메타의 맵 행 장치에 연결한다.</summary>
    public static TellMapMenuHooks Bind(RawSoloMapList list, SoloMapListState state, Action<bool> selectFileSpec,
        Func<string> current, Func<string> fallback, Action<uint> begin, Action<SoloMapRow, uint> add)
    {
        return new TellMapMenuHooks
        {
            BeginVertical = begin, Invalidate = () => state.Dirty = 1, SelectFileSpec = selectFileSpec,
            Refresh = mode => list.Refresh(mode), Count = () => state.Count,
            AppendRow = index => { var row = list.Row(index, current(), fallback()); if (row == null) return false; add(row, 0x020000ca); return true; }
        };
    }
}
