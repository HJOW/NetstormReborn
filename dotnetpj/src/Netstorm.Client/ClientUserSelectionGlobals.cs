using Netstorm.Client.Input;
using Netstorm.Core.Original;

// Runtime보다 오래 유지하는 선택/강조 DWORD와 현재 다중 배열을 소유한다.
namespace Netstorm.Client;

/// <summary>원문 ClientUserSelectionGlobals의 동일 필드를 선택·강조·State가 공유한다.</summary>
public sealed class ClientUserSelectionGlobals
{
    /// <summary>원문 전역 주소에 대응하는 개별 DWORD 식별자다.</summary>
    public enum Field
    {
        /// <summary>005434e4의 선택 활성 DWORD다.</summary>
        Enabled,
        /// <summary>005cae90의 강조 모드다.</summary>
        Mode,
        /// <summary>005caea0의 선택 SID다.</summary>
        Selected,
        /// <summary>005caea4의 마우스 TYPE다.</summary>
        MouseType,
        /// <summary>005cae94의 signed X 비트다.</summary>
        MouseX,
        /// <summary>005cae98의 signed Y 비트다.</summary>
        MouseY,
        /// <summary>005caf38의 기억한 float X 비트다.</summary>
        RememberedX,
        /// <summary>005caf3c의 기억한 float Y 비트다.</summary>
        RememberedY,
        /// <summary>005caf34의 signed 개수 비트다.</summary>
        MultipleCount,
        /// <summary>005caf40의 P/R 누적 선택 DWORD다.</summary>
        PriestSelections,
    }

    /// <summary>원문 최초1인 전체 DWORD이며 bool로 좁히지 않는다.</summary>
    public UIntBox Enabled { get; } = new() { Value = 1 };
    /// <summary>같은 현재 강조 모드다.</summary>
    public UIntBox Mode { get; } = new();
    /// <summary>소비 전에 WORD로 자르지 않는 현재 선택 SID다.</summary>
    public UIntBox Selected { get; } = new();
    /// <summary>기억한 마우스 대상 TYPE다.</summary>
    public UIntBox MouseType { get; } = new();
    /// <summary>커서 보정과 별개인 원시 signed 화면 X다.</summary>
    public IntBox MouseX { get; } = new();
    /// <summary>커서 보정과 별개인 원시 signed 화면 Y다.</summary>
    public IntBox MouseY { get; } = new();
    /// <summary>기억한 float 두 개의 원시 DWORD다.</summary>
    public uint[] Remembered { get; } = new uint[2];
    /// <summary>통지 뒤 다음 반복 조건이 다시 읽는 같은 signed 개수다.</summary>
    public IntBox MultipleCount { get; } = new();
    /// <summary>일반 World reset과 선택 정리에서도 보존하는 P/R 누적값이다.</summary>
    public UIntBox PriestSelections { get; } = new();
    /// <summary>원문 고정30칸의 최초 배열이며 callback이 바꾼 현재 배열도 명시 보관한다.</summary>
    private uint[] _multiple = new uint[30];
    /// <summary>다음 반복은 이 현재 배열을 다시 조회한다. 추가 함수가 용량을 자동 확장하지 않는다.</summary>
    public uint[] Multiple { get => _multiple; set => _multiple = value ?? throw new ArgumentNullException(nameof(value)); }

    /// <summary>용량 관문 뒤 현재 개수 앞에서만 중복을 제외하고 새 SID를 추가한다.</summary>
    public void AppendMultiple(uint sid)
    {
        if (MultipleCount.Value >= _multiple.Length) return;
        if (MultipleCount.Value < 0) throw new ArgumentOutOfRangeException(nameof(MultipleCount), "다중선택추가의음수인덱스");
        // 현재 개수 앞부분의 전체 DWORD 일치만 중복으로 취급한다.
        for (int index = 0; index < MultipleCount.Value; index++) if (_multiple[index] == sid) return;
        _multiple[MultipleCount.Value] = sid; MultipleCount.Value++;
    }

    /// <summary>현재 개수 앞의 모든 일치를 안정 압축하며 사용하지 않는 꼬리와 음수 개수는 보존한다.</summary>
    public void RemoveMultiple(uint sid)
    {
        if (MultipleCount.Value > _multiple.Length) throw new ArgumentOutOfRangeException(nameof(MultipleCount), "다중선택제거의물리범위");
        int removed = 0;
        // 남은 값의 원래 순서를 보존하고 꼬리를 따로 지우지 않는다.
        for (int index = 0; index < MultipleCount.Value; index++)
        {
            uint value = _multiple[index]; if (value == sid) removed++; else _multiple[index - removed] = value;
        }
        MultipleCount.Value -= removed;
    }

    /// <summary>기존 일반 World의 fresh 입력만 초기화한다. prepared 대여나 반납에서는 호출하지 않는다.</summary>
    public void ResetForGeneralWorld()
    {
        Enabled.Value = 1; Mode.Value = Selected.Value = MouseType.Value = 0; MouseX.Value = MouseY.Value = 0;
        Array.Clear(Remembered); MultipleCount.Value = 0;
        if (_multiple.Length != 30) _multiple = new uint[30]; else Array.Clear(_multiple);
    }

    /// <summary>강조 본체도 별도 값 사본 없이 같은 필드를 빌린다.</summary>
    public UserHighlightState HighlightState() => new() { Mode = Mode, SelectedSid = Selected, MouseType = MouseType, MouseX = MouseX, MouseY = MouseY };

    /// <summary>선택 본체는 같은 필드와 각 반복 시점의 현재 배열을 빌린다.</summary>
    public UserSelectionState SelectionState() => new() { Selected = Selected, MouseType = MouseType, RememberedPoint = Remembered, MultipleCount = MultipleCount, CurrentMultiple = () => Multiple };

    /// <summary>signed 좌표/개수를 포함해 전체 DWORD 비트를 반환한다.</summary>
    public uint ReadDword(Field field) => field switch
    {
        Field.Enabled => Enabled.Value, Field.Mode => Mode.Value, Field.Selected => Selected.Value, Field.MouseType => MouseType.Value,
        Field.MouseX => unchecked((uint)MouseX.Value), Field.MouseY => unchecked((uint)MouseY.Value),
        Field.RememberedX => Remembered[0], Field.RememberedY => Remembered[1], Field.MultipleCount => unchecked((uint)MultipleCount.Value),
        Field.PriestSelections => PriestSelections.Value, _ => throw new ArgumentOutOfRangeException(nameof(field)),
    };

    /// <summary>선택한 한 필드만 원문 DWORD로 쓰며 bool 정규화나 이웃 reset을 하지 않는다.</summary>
    public void WriteDword(Field field, uint value)
    {
        switch (field)
        {
            case Field.Enabled: Enabled.Value = value; return; case Field.Mode: Mode.Value = value; return;
            case Field.Selected: Selected.Value = value; return; case Field.MouseType: MouseType.Value = value; return;
            case Field.MouseX: MouseX.Value = unchecked((int)value); return; case Field.MouseY: MouseY.Value = unchecked((int)value); return;
            case Field.RememberedX: Remembered[0] = value; return; case Field.RememberedY: Remembered[1] = value; return;
            case Field.MultipleCount: MultipleCount.Value = unchecked((int)value); return; case Field.PriestSelections: PriestSelections.Value = value; return;
            default: throw new ArgumentOutOfRangeException(nameof(field));
        }
    }
}
