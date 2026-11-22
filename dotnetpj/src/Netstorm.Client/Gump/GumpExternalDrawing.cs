namespace Netstorm.Client.Gump; // 같은 공통 수집/정렬의 외부 파생 출력 수명을 소유한다.

/// <summary>Source RawMenuExternalGumps의 실제 텍스트/소유 판정/현재 표면 출력 장치다.</summary>
public sealed class GumpExternalGumps
{
    /// <summary>같은 TextGump/StyleText가 읽는 외부 실제 노드다.</summary>
    public Func<uint, TextGumpNode?>? Text { get; set; }
    /// <summary>현재 파생 토큰을 소유하는지 매번 확인한다.</summary>
    public Func<uint, bool>? Owns { get; set; }
    /// <summary>현재 같은 픽셀/색/배경 장치로 가상 Draw를 수행한다.</summary>
    public Action<uint, GumpPixelTarget, GumpGlyphColorState, GumpBackgroundDraw>? Draw { get; set; }
    /// <summary>Source의 값 전달처럼 함수 슬롯을 복사하며 callback의 현재 owner는 그대로 공유한다.</summary>
    internal GumpExternalGumps Copy() => new() { Text = Text, Owns = Owns, Draw = Draw };
}

/// <summary>cpppj/src/client/RawMenuControls.cpp의 공통 버튼/고정 슬롯/독립 소유 등록·조회 본체다.</summary>
public sealed class GumpExternalDrawing
{
    /// <summary>독립 소유 등록보다 먼저 조회하는 실제 공통 버튼 장치다.</summary>
    private GumpExternalGumps? _common;
    /// <summary>기존 고정 슬롯의 번호순 장치다.</summary>
    private readonly SortedDictionary<uint, GumpExternalGumps> _fixed = [];
    /// <summary>반납한 번호를 다시 쓰지 않는 독립 UI 출력 장치다.</summary>
    private readonly SortedDictionary<uint, GumpExternalGumps> _owned = [];
    /// <summary>마지막으로 발급한 출력 수명 번호이며 Gump 등록ID와 다르다.</summary>
    private uint _next;
    /// <summary>현재 독립 UI 출력 등록 수의 읽기 전용 관찰이다.</summary>
    public int RegistrationCount => _owned.Count;
    /// <summary>공통 버튼의 실제 소유/출력 장치만 받으며 독립 등록 번호를 만들지 않는다.</summary>
    public void BindCommonButtons(GumpExternalGumps hooks)
    {
        Validate(hooks); _common = hooks.Copy();
    }
    /// <summary>선택한 실제 외부 장치 누락은 등록 전에 거부한다.</summary>
    private static void Validate(GumpExternalGumps hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); if (hooks.Owns is null || hooks.Draw is null) throw new ArgumentException("외부 Gump의 실제 소유/출력 장치 필요", nameof(hooks));
    }
    /// <summary>기본 고정 슬롯0의 기존 장치를 교체하거나 반납한다.</summary>
    public void SetExternalGumps(GumpExternalGumps hooks) => SetExternalGumps(0, hooks);
    /// <summary>빈 소유자 훅은 해당 고정 슬롯만 반납하고 다른 UI는 보존한다.</summary>
    public void SetExternalGumps(uint slot, GumpExternalGumps hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); if (hooks.Owns is null) _fixed.Remove(slot); else _fixed[slot] = hooks.Copy();
    }
    /// <summary>원문 가상 Draw의 독립 소유 토큰을 단조 증가 발급한다.</summary>
    public uint RegisterExternalGumps(GumpExternalGumps hooks)
    {
        Validate(hooks); if (_next == uint.MaxValue) throw new OverflowException("외부 Gump 출력 소유 토큰 소진");
        uint registration = _next + 1; _owned.Add(registration, hooks.Copy()); _next = registration; return registration;
    }
    /// <summary>자기 등록만 반납하며 다른 고정/독립 UI와 다음 번호는 유지한다.</summary>
    public void UnregisterExternalGumps(uint registration) => _owned.Remove(registration);
    /// <summary>공통→고정 번호순→독립 등록순으로 현재 실제 소유자를 조회한다.</summary>
    private GumpExternalGumps? External(uint token)
    {
        if (_common?.Owns?.Invoke(token) == true) return _common;
        // 현재 고정 슬롯 중 첫 소유자를 반환한다.
        foreach (var hooks in _fixed.Values) if (hooks.Owns?.Invoke(token) == true) return hooks;
        // 살아 있는 각 UI의 같은 파생만 조회한다.
        foreach (var hooks in _owned.Values) if (hooks.Owns!(token)) return hooks;
        return null;
    }
    /// <summary>일반/공통 버튼과 구별해 고정→독립 순서의 실제 외부 텍스트만 읽는다.</summary>
    public TextGumpNode? Text(uint token)
    {
        // Owns와 별도인 원문 Text 조회 순서를 유지한다.
        foreach (var hooks in _fixed.Values) if (hooks.Text?.Invoke(token) is { } node) return node;
        // 후속 UI의 같은 살아 있는 텍스트 주소를 반환한다.
        foreach (var hooks in _owned.Values) if (hooks.Text?.Invoke(token) is { } node) return node;
        return null;
    }
    /// <summary>현재 수집/정렬/클립 안의 같은 장치로 선택된 실제 외부 Draw를 한 번 수행한다.</summary>
    public bool TryDraw(uint token, GumpPixelTarget target, GumpGlyphColorState colors, GumpBackgroundDraw background)
    {
        var hooks = External(token); if (hooks is null) return false;
        (hooks.Draw ?? throw new InvalidOperationException("선택된 외부 Gump 실제 Draw 미연결"))(token, target, colors, background); return true;
    }
}
