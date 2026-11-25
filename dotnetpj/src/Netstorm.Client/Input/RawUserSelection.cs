using Netstorm.Core.Original;

// 원문 세 인자 선택과 다중 선택이 같은 현재 Pool/TYPE/장기 전역을 읽는다.
namespace Netstorm.Client.Input;

/// <summary>선택·마우스 TYPE·기억 좌표와 현재 다중 배열의 공유 필드다.</summary>
public sealed class UserSelectionState
{
    /// <summary>현재 전체 선택 SID다.</summary>
    public required UIntBox Selected { get; init; }
    /// <summary>기억한 마우스 대상 TYPE다.</summary>
    public required UIntBox MouseType { get; init; }
    /// <summary>기억한 float X/Y의 원문 두 DWORD다.</summary>
    public required uint[] RememberedPoint { get; init; }
    /// <summary>독립 호출의 현재 다중 배열 뷰다.</summary>
    public ReadOnlyMemory<uint> Multiple { get; init; }
    /// <summary>통지 뒤 다시 읽는 signed 개수다.</summary>
    public required IntBox MultipleCount { get; init; }
    /// <summary>장기 owner의 재할당된 현재 배열을 다음 반복에서 다시 조회한다.</summary>
    public Func<ReadOnlyMemory<uint>>? CurrentMultiple { get; init; }
}

/// <summary>선택된 실제 가상 통지·표시·이동·음향을 해당 호출 위치에서 제공한다.</summary>
public sealed class UserSelectionHooks
{
    /// <summary>460de0의 현재 정지 관문이다.</summary>
    public Func<uint>? Paused { get; init; }
    /// <summary>5434e4의 현재 활성 관문이다.</summary>
    public Func<uint>? Enabled { get; init; }
    /// <summary>463b50의 현재 콘솔 등 차단 관문이다.</summary>
    public Func<uint>? Blocked { get; init; }
    /// <summary>540c70의 현재 로컬 소유자다.</summary>
    public Func<uint>? LocalOwner { get; init; }
    /// <summary>5c85a4의 현재 편집 모드다.</summary>
    public Func<uint>? Editor { get; init; }
    /// <summary>5412ec의 현재 다이스 TYPE다.</summary>
    public Func<uint>? Dais { get; init; }
    /// <summary>541138의 현재 마나 프로세스 TYPE다.</summary>
    public Func<uint>? ManaProcess { get; init; }
    /// <summary>54113c의 현재 사정거리 프로세스 TYPE다.</summary>
    public Func<uint>? RangeProcess { get; init; }
    /// <summary>현재 대상의 가상+7c 전체 DWORD 선택 통지다.</summary>
    public Action<int, uint>? SelectNotice { get; init; }
    /// <summary>앞선 효과 뒤 현재 가상+88 표시 갱신이다.</summary>
    public Action<int>? Refresh { get; init; }
    /// <summary>종속 프로세스의 가상+1c 사건/정보/값 통지다.</summary>
    public Action<int, uint, uint, uint>? ProcessNotice { get; init; }
    /// <summary>44dab0의 실제 재귀 진단이다.</summary>
    public Action<int>? Dump { get; init; }
    /// <summary>4741d0의 마나 표시 생성이며 둘째0은 Form 미지정이다.</summary>
    public Action<int, uint>? ShowMana { get; init; }
    /// <summary>474900의 사정거리 표시 생성이다.</summary>
    public Action<int, uint>? ShowRange { get; init; }
    /// <summary>474240의 사정거리 종류 판정이다.</summary>
    public Func<uint, uint>? RangeType { get; init; }
    /// <summary>4ac9f0(1,0,0)의 현재 내용물 SID다.</summary>
    public Func<int, int>? Content { get; init; }
    /// <summary>447930의 같은 위치 제단 SID다.</summary>
    public Func<int, int>? Altar { get; init; }
    /// <summary>425ed0의 현재 이동/운반 상태다.</summary>
    public Func<int, uint>? Moving { get; init; }
    /// <summary>426080의 실제491 Regular 생성 경계다.</summary>
    public Action<int>? MovementPreview { get; init; }
    /// <summary>같은 장기 전역의 실제 강조 모드 변경이다.</summary>
    public Action<uint>? Highlight { get; init; }
    /// <summary>두 float 비트·이름·flags의 실제 음향 호출이다.</summary>
    public Action<uint, uint, string, uint>? Sound { get; init; }
    /// <summary>selectedSid 진단 후 현재 선택을 다시 읽는다.</summary>
    public Action? Assertion { get; init; }
    /// <summary>내용물 미리보기의 Form 진단을 실제 소비자에게 제공한다.</summary>
    public Action? PreviewAssertion { get; init; }
    /// <summary>고정 용량/중복 제외로 같은 다중 목록에 추가한다.</summary>
    public Action<uint>? AppendMultiple { get; init; }
    /// <summary>같은 목록의 전체 일치를 안정 압축한다.</summary>
    public Action<uint>? RemoveMultiple { get; init; }
    /// <summary>다중 이동의 선행0/정상복귀1을 같은 활성 DWORD에 쓴다.</summary>
    public Action<uint>? WriteEnabled { get; init; }
}

/// <summary>RawUserSelection.cpp의 현재 재조회·짧은 메뉴·실패 prefix를 보존하는10.78 본체다.</summary>
public sealed class RawUserSelection
{
    /// <summary>원문 WorldDestSelected 명령의 전체 DWORD다.</summary>
    public const uint WorldDestinationSelectedCommand = 0x02000044;
    /// <summary>같은 실제50BYTE 슬롯 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>callback 뒤 다시 읽는 현재188TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>같은 장기 선택/마우스/다중 필드다.</summary>
    private readonly UserSelectionState _state;
    /// <summary>선택된 하위만 해당 위치에서 요구하는 실제 장치다.</summary>
    private readonly UserSelectionHooks _hooks;

    /// <summary>같은 판본과 물리 폭을 검증하고 참조만 보관한다.</summary>
    public RawUserSelection(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, UserSelectionState state, UserSelectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || state.RememberedPoint.Length != 2) throw new ArgumentException("사용자선택10.78풀/TYPE/기억점폭오류");
        _pool = pool; _types = types; _state = state; _hooks = hooks;
    }
    /// <summary>살아 있는 선택 객체를 Session의 약한 연결에 묶어 두는 수명 고리다. 선택이 살아 있으면 Session의 약한 연결도 만료되지 않는다.</summary>
    /// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawUserSelection.h 71행의 sessionLifetime_.</remarks>
    public object? SessionLifetime { get; set; }
    /// <summary>미선택 효과는 요구하지 않고 도달한 하위 누락만 실패한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("실제사용자선택하위미연결");
    /// <summary>전체 DWORD를 물리/WORD 범위 확인 뒤에만 슬롯 번호로 변환한다.</summary>
    private int SlotId(uint sid) => sid != 0 && sid < _pool.Capacity && sid <= ushort.MaxValue ? (int)sid : throw new ArgumentOutOfRangeException(nameof(sid), "사용자선택SID범위");
    /// <summary>반납/죽음 비트를 추가 제외하지 않고 실제 슬롯만 읽는다.</summary>
    private ReadOnlySpan<byte> Raw(uint sid) => _pool.Slot(SlotId(sid));
    /// <summary>현재 TYPE를 호출 시점에서 조회한다.</summary>
    private OriginalTypeRecord Type(uint type) => type < _types.Count ? _types[(int)type] : throw new ArgumentOutOfRangeException(nameof(type));
    /// <summary>정렬되지 않은 원문 BYTE를 little endian DWORD/WORD로 조립한다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 주소부터 순서대로 읽는다.
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (index * 8);
        return value;
    }
    /// <summary>callback이 재할당한 장기 배열을 각 반복에서 다시 조회한다.</summary>
    private ReadOnlyMemory<uint> CurrentMultiple() => _state.CurrentMultiple is { } current ? current() : _state.Multiple;
    /// <summary>현재 signed 개수/배열로 +7c(0)→+88을 호출하고 정상 꼬리에서만 개수를0으로 쓴다.</summary>
    public void ClearMultiple()
    {
        // 통지 뒤 개수와 다음 원소를 현재 owner에서 다시 읽는다.
        for (int index = 0; index < _state.MultipleCount.Value; index++)
        {
            ReadOnlyMemory<uint> multiple = CurrentMultiple(); if (index >= multiple.Length) throw new ArgumentOutOfRangeException(nameof(index), "다중선택물리범위");
            uint number = multiple.Span[index]; if (number == 0 || number >= _pool.Capacity || (Raw(number)[11] & 3) != 0) continue;
            int sid = SlotId(number); Need(_hooks.SelectNotice)(sid, 0); Need(_hooks.Refresh)(sid);
        }
        _state.MultipleCount.Value = 0;
    }
    /// <summary>효과 전 next를 보관해 종속 프로세스50/51만 통지한 뒤 기존 단일 선택을 해제한다.</summary>
    private void ClearPrevious(int sid)
    {
        uint next = Read(Raw((uint)sid), 6, 2); var visited = new HashSet<uint>();
        // 원문이 방문하는 모든 물리 후보를 같은 순서로 검사하며 손상 순환만 거부한다.
        while (next != 0)
        {
            uint number = next; if (!visited.Add(number)) throw new InvalidOperationException("사용자선택프로세스체인순환");
            ReadOnlySpan<byte> raw = Raw(number); next = Read(raw, 4, 2); byte type = raw[10];
            if (type >= 10 && type < 70 && (type == Need(_hooks.ManaProcess)() || type == Need(_hooks.RangeProcess)())) Need(_hooks.ProcessNotice)(SlotId(number), 3, 0, 0);
        }
        Need(_hooks.SelectNotice)(sid, 0); Need(_hooks.Refresh)(sid); _state.Selected.Value = 0;
    }
    /// <summary>flags2의 이동 역할 비트가 있을 때만 원시 좌표와 TYPE를 기억한다.</summary>
    public void Remember(uint x, uint y, uint type)
    {
        if ((Type(type).Flags2 & 0x30000) != 0) { _state.RememberedPoint[0] = x; _state.RememberedPoint[1] = y; _state.MouseType.Value = type; }
    }
    /// <summary>검색 뒤 현재 선택 관문과 Select(0,1)를 실행하고 원래 found의 현재 extra를 재검사한다.</summary>
    public void RecheckMouse(Func<uint, uint, uint, uint>? find)
    {
        if (_state.MouseType.Value == 0) return;
        uint found = Need(find)(_state.RememberedPoint[0], _state.RememberedPoint[1], _state.MouseType.Value);
        if (found == 0 || (_state.Selected.Value != 0 && _state.Selected.Value != found)) return;
        _ = Select(found, 0, 1); if ((Raw(found)[40] & 9) == 0) _state.MouseType.Value = 0;
    }
    /// <summary>forceSelect와activateMove를 독립 DWORD로 처리하고 최종 재지정 대상 번호를 반환한다.</summary>
    public uint Select(uint number, uint forceSelect, uint activateMove)
    {
        // 다이스가 유효 제단으로 위임하면 최초 관문/다중 정리부터 다시 수행한다.
        for (;;)
        {
            if ((number != 0 && Need(_hooks.Paused)() != 0) || (Need(_hooks.Enabled)() == 0 && number == 0) || Need(_hooks.Blocked)() != 0) return 0;
            ClearMultiple(); if (_state.Selected.Value != 0) { _ = Raw(_state.Selected.Value); ClearPrevious(SlotId(_state.Selected.Value)); }
            if (number == 0 || (Raw(number)[11] & 2) != 0) { Need(_hooks.Highlight)(0); return 0; }
            int sid = SlotId(number); Need(_hooks.Dump)(sid);
            if ((Type(Raw(number)[10]).Flags1 & 0x10000000) != 0) Need(_hooks.ShowMana)(sid, 0);
            if (Need(_hooks.RangeType)(Raw(number)[10]) != 0) Need(_hooks.ShowRange)(sid, 0);
            if ((Type(Raw(number)[10]).Flags2 & 0x30000) != 0 && Need(_hooks.Content)(sid) != 0) Need(_hooks.ShowRange)(sid, 0);
            if (Raw(number)[10] == Need(_hooks.Dais)())
            {
                int altar = Need(_hooks.Altar)(sid); if (altar != 0 && unchecked((uint)altar) < _pool.Capacity) { number = (uint)altar; continue; }
            }
            if ((Type(Raw(number)[10]).Flags1 & 0x08000000) == 0 || forceSelect != 0)
            {
                _state.Selected.Value = number; Need(_hooks.SelectNotice)(sid, 1); Need(_hooks.Refresh)(sid);
                ReadOnlySpan<byte> raw = Raw(number); uint x = Read(raw, 14), y = Read(raw, 18);
                if (!(BitConverter.UInt32BitsToSingle(x) == BitConverter.UInt32BitsToSingle(_state.RememberedPoint[0]) && BitConverter.UInt32BitsToSingle(y) == BitConverter.UInt32BitsToSingle(_state.RememberedPoint[1]))) _state.MouseType.Value = 0;
                if ((Raw(number)[40] & 9) != 0) Remember(Read(Raw(number), 14), Read(Raw(number), 18), Raw(number)[10]);
                if ((Type(Raw(number)[10]).Flags2 & 0x30000) != 0 && (Raw(number)[40] & 1) == 0)
                {
                    if (_state.Selected.Value == 0) Need(_hooks.Assertion)();
                    if (_state.Selected.Value != 0)
                    {
                        if (Need(_hooks.Moving)(sid) == 0 && activateMove != 0 && (Need(_hooks.Editor)() != 0 || Raw(number)[34] == Need(_hooks.LocalOwner)())) Need(_hooks.Highlight)(5);
                        Need(_hooks.MovementPreview)(sid);
                    }
                }
                Need(_hooks.Sound)(Read(Raw(number), 14), Read(Raw(number), 18), "select.wav", 0);
            }
            return number;
        }
    }
    /// <summary>enabled만 검사해 다중 정리→선택0→강조0을 실행하며 기존 단일 선택 가상통지를 추가하지 않는다.</summary>
    public void Clear() { if (Need(_hooks.Enabled)() != 0) { ClearMultiple(); _state.Selected.Value = 0; Need(_hooks.Highlight)(0); } }
    /// <summary>전체 DWORD 토글/주선택 승격/목록 및 원래 대상의 표시 호출 순서를 보존한다.</summary>
    public void ToggleMultiple(uint number, uint on)
    {
        if (number == 0 || number >= _pool.Capacity || number > ushort.MaxValue || (Raw(number)[11] & 3) != 0) return;
        int sid = SlotId(number); uint marked = (uint)((Raw(number)[40] >> 2) & 1); if (on == uint.MaxValue) on = (~marked) & 1; if (marked == on) return;
        Need(_hooks.SelectNotice)(sid, on);
        if (_state.Selected.Value == number)
        {
            if (_state.MultipleCount.Value == 0)
            {
                if (Need(_hooks.Enabled)() == 0 || Need(_hooks.Blocked)() != 0) return;
                ClearMultiple(); if (_state.Selected.Value != 0) ClearPrevious(SlotId(_state.Selected.Value)); Need(_hooks.Highlight)(0); return;
            }
            ReadOnlyMemory<uint> current = CurrentMultiple(); if (current.IsEmpty) throw new ArgumentOutOfRangeException(nameof(current), "다중선택승격첫원소없음");
            _state.Selected.Value = current.Span[0]; number = _state.Selected.Value;
        }
        else if (on != 0) { Need(_hooks.AppendMultiple)(number); Need(_hooks.Sound)(Read(Raw(number), 14), Read(Raw(number), 18), "select.wav", 0); Need(_hooks.Refresh)(sid); return; }
        Need(_hooks.RemoveMultiple)(number); Need(_hooks.Refresh)(sid);
    }
    /// <summary>Shift 후보의 이동 역할 뒤에 현재 기본 선택을 조회하고 단일 선택/추가 토글을 구분한다.</summary>
    public void ExtendSelection(uint number, uint forceSelect)
    {
        if (Need(_hooks.Paused)() != 0 || number == 0 || (Type(Raw(number)[10]).Flags2 & 0x30000) == 0) return;
        if (_state.Selected.Value == 0 || (Raw(_state.Selected.Value)[11] & 1) != 0 || (Type(Raw(_state.Selected.Value)[10]).Flags2 & 0x30000) == 0) _ = Select(number, forceSelect, 1);
        else ToggleMultiple(number, uint.MaxValue);
    }
    /// <summary>enabled0→현재 다중 전송→enabled1→정리다. 전송 실패 때 선행0을 복구하지 않는다.</summary>
    public void MoveMultiple(uint x, uint y, Action<uint, uint, uint>? move)
    {
        Need(_hooks.WriteEnabled)(0);
        // 각 전송 뒤 현재 개수와 다음 원소를 다시 읽는다.
        for (int index = 0; index < _state.MultipleCount.Value; index++)
        {
            ReadOnlyMemory<uint> current = CurrentMultiple(); if (index >= current.Length) throw new ArgumentOutOfRangeException(nameof(index), "다중이동현재배열범위"); uint number = current.Span[index];
            if (number != 0 && number < _pool.Capacity && number <= ushort.MaxValue && (Raw(number)[11] & 3) == 0) Need(move)(number, x, y);
        }
        Need(_hooks.WriteEnabled)(1); ClearMultiple();
    }
    /// <summary>Carrier는 Select(sid,0,0) 뒤 실제 공통+ac의 전체 반환을 유지한다.</summary>
    public uint CarrierMenu(int sid, InputEvent input, Func<int, InputEvent, uint>? common)
    {
        if ((Raw(unchecked((uint)sid))[40] & 1) == 0) _ = Select((uint)sid, 0, 0); return Need(common)(sid, input);
    }
    /// <summary>Archer는 셋째 인자만1인 선택 뒤 같은 공통+ac를 호출한다.</summary>
    public uint ArcherMenu(int sid, InputEvent input, Func<int, InputEvent, uint>? common)
    {
        if ((Raw(unchecked((uint)sid))[40] & 1) == 0) _ = Select((uint)sid, 0, 1); return Need(common)(sid, input);
    }
}
