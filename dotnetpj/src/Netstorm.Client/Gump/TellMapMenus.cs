using Netstorm.Client.Mission;
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
    /// <summary>스크롤 표시 창의 첫 목록 번호를 읽는다. 없으면 원본처럼 전체 목록을 한 번에 만든다.</summary>
    public Func<int>? WindowFirst { get; init; }
    /// <summary>표시 창의 첫 번호를 쓴다(범위 보정 뒤와 새로 열 때의 0).</summary>
    public Action<int>? SetWindowFirst { get; init; }
    /// <summary>스크롤 이동 뒤 재표시이면 참이며 일회성 표시를 소비한다. 참이면 목록을 다시 열거하지 않는다.</summary>
    public Func<bool>? TakeKeepCache { get; init; }
    /// <summary>캐시 재사용 때 직전 목록 구성이 끝에 건 dirty를 지워 같은 목록을 다시 열거하지 않게 한다.</summary>
    public Action? ReuseCache { get; init; }
    /// <summary>이전·다음 이동 행(문구, 이동할 행 수)을 현재 열에 추가한다.</summary>
    public Action<string, int>? AppendScroll { get; init; }
    /// <summary>이동 행의 고정 단어(Previous·Next·of)만 현재 언어로 바꾼다. 비어 있으면 영어 그대로다.</summary>
    public Func<string, string>? Translate { get; init; }
}

/// <summary>원본 Tell 맵 목록의 열 생성·현재 캐시 재조회와 키 복구다.</summary>
public static class TellMapMenus
{
    /// <summary>5차: 목록 메뉴가 한 번에 보여 주는 최대 행 수다. 원본 한 화면 분량(두 열 25행)이며 이보다 길면 이전·다음 이동 행으로 스크롤한다.</summary>
    public const int MapListPageRows = 50;

    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Tell 맵 목록의 현재 장치 미연결");

    /// <summary>현재 열 수에 맵 행을 추가한다. 비맵 레이블은 false로 반환한다. 50행 초과 목록은 스크롤 장치와 함께 한 창씩 만든다.</summary>
    /// <remarks>2026-11-11 0.363에서 cpppj HEAD의 5차 스크롤 표시를 반영했다. 스크롤 장치가 비어 있으면 원본처럼 전체 목록을 한 번에 두 열로 만든다.</remarks>
    public static bool Append(string label, ref uint columns, TellMapMenuHooks hooks)
    {
        if (columns == 0) { Need(hooks.BeginVertical)(0x40000); columns = unchecked(columns + 1); }
        string lower = string.Concat(label.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
        bool solo = lower == "*sololist*"; if (!solo && lower != "*fortlist*") return false;
        // 5차: 스크롤 재표시이면 목록을 다시 열거하지 않고 같은 캐시를 쓴다. 일회성 표시는 여기서 소비한다.
        bool keep = hooks.TakeKeepCache?.Invoke() ?? false;
        // 새로 연 목록은 첫 창에서 시작한다
        if (!keep) { Need(hooks.Invalidate)(); hooks.SetWindowFirst?.Invoke(0); }
        else hooks.ReuseCache?.Invoke();
        Need(hooks.SelectFileSpec)(solo); Need(hooks.Refresh)(solo ? 2 : 0);
        if (hooks.WindowFirst is not null && hooks.SetWindowFirst is not null && hooks.AppendScroll is not null && Need(hooks.Count)() > MapListPageRows)
        {
            // 5차: 긴 목록은 한 창(50행)씩 스크롤한다. 50행 이하는 아래 원본 흐름을 그대로 쓴다.
            int total = Need(hooks.Count)();
            // 마지막 창은 항상 꽉 채운다
            int first = Math.Clamp(hooks.WindowFirst(), 0, total - MapListPageRows);
            hooks.SetWindowFirst(first);
            int last = first + MapListPageRows;
            // 번역 장치가 없거나 키가 없으면 영어 원문 그대로다
            string Word(string key) => hooks.Translate is { } translate ? translate(key) : key;
            if (first > 0) hooks.AppendScroll("~o< " + Word("Previous") + " " + MapListPageRows + "  (" + Math.Max(1, first - MapListPageRows + 1) + "-" + first + " " + Word("of") + " " + total + ")", -MapListPageRows);
            // 이번 창에서 추가한 행 수다
            int shown = 0;
            // 창의 모든 행을 순서대로 추가하고 절반에서 두 번째 열을 만든다
            for (int item = first; item < last; item++)
            {
                if (!Need(hooks.AppendRow)(item)) break;
                if (++shown == MapListPageRows / 2) { Need(hooks.BeginVertical)(0x40000); columns = unchecked(columns + 1); }
            }
            if (last < total) hooks.AppendScroll("~o" + Word("Next") + " " + MapListPageRows + " >  (" + (last + 1) + "-" + Math.Min(total, last + MapListPageRows) + " " + Word("of") + " " + total + ")", MapListPageRows);
        }
        else
        {
            int index = 0;
            // Refresh의 과거 개수 대신 각 성공 행 추가 뒤 현재 개수로 분할 지점을 검사한다.
            while (Need(hooks.AppendRow)(index))
            {
                index = unchecked(index + 1); int count = Need(hooks.Count)();
                if (count > 15 && index == count / 2) { Need(hooks.BeginVertical)(0x40000); columns = unchecked(columns + 1); }
            }
        }
        if (solo) { Need(hooks.Invalidate)(); Need(hooks.SelectFileSpec)(false); }
        return true;
    }

    /// <summary>같은 캐시·파일 키·현재 선택을 Tell 메타의 맵 행 장치에 연결한다. 스크롤 창 상태도 같은 목록 상태에 둔다.</summary>
    /// <remarks>2026-11-11 0.363에서 5차 스크롤 장치 배선을 추가했다. 이동 행 선택 소비는 메뉴 소유자(R7)에서 잇는다.</remarks>
    public static TellMapMenuHooks Bind(RawSoloMapList list, SoloMapListState state, Action<bool> selectFileSpec,
        Func<string> current, Func<string> fallback, Action<uint> begin, Action<SoloMapRow, uint> add, Func<string, string>? translate = null)
    {
        return new TellMapMenuHooks
        {
            BeginVertical = begin, Invalidate = () => state.Dirty = 1, SelectFileSpec = selectFileSpec,
            Refresh = mode => list.Refresh(mode), Count = () => state.Count,
            AppendRow = index => { var row = list.Row(index, current(), fallback()); if (row == null) return false; add(row, 0x020000ca); return true; },
            WindowFirst = () => state.WindowFirst,
            SetWindowFirst = first => state.WindowFirst = first,
            TakeKeepCache = () => { bool value = state.KeepCache; state.KeepCache = false; return value; },
            ReuseCache = () => state.Dirty = 0,
            AppendScroll = (label, delta) => add(new SoloMapRow(label, delta.ToString(System.Globalization.CultureInfo.InvariantCulture), false, true), FortSelection.ScrollMapList),
            Translate = translate,
        };
    }
}
