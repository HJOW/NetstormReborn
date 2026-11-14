using System.Buffers.Binary;

// 사제 소유자의 원래 소유자WORD·현재 모드·로컬비트 후단을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>사제 소유자 지정이 소비하는 실제 현재 로딩/로컬 Player 값이다.</summary>
public sealed class PriestOwnerState
{
    /// <summary>현재 요새 적재 중첩 깊이다.</summary>
    public uint LoadingDepth { get; set; }
    /// <summary>편집 모드에서 로컬 비트 대상인 Player다.</summary>
    public uint FortPlayer { get; set; }
    /// <summary>전투 모드에서 로컬 비트 대상인 Player다.</summary>
    public uint BattlePlayer { get; set; }
}

/// <summary>공통 지정·무상태 알림·선택된 원문 진단 경계다.</summary>
public sealed class PriestOwnerHooks
{
    /// <summary>같은 실제 공통 Owner 본문이다.</summary>
    public Action<int, uint>? Base { get; set; }
    /// <summary>원래 소유자/로컬비트 쓰기 뒤 원문 무상태 표면 알림이다.</summary>
    public Action<int>? Notify { get; set; }
    /// <summary>요청0의 식/파일/행을 보고하고 정상 복귀할 수 있다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>실제 사제 가상 표의 소유자 지정 재정의다. TYPE/free를 선행 강제하지 않는다.</summary>
public sealed class RawPriestOwner
{
    /// <summary>실제 원문 슬롯을 빌린다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>알림/부모 효과 뒤 다시 읽을 실제 현재 모드다.</summary>
    private readonly SquidOwnerMode _mode;
    /// <summary>현재 로딩/로컬번호를 빌린다.</summary>
    private readonly PriestOwnerState _state;
    /// <summary>구성 때 값 복사한 필수 공통 지정 함수다.</summary>
    private readonly Action<int, uint> _base;
    /// <summary>구성 때 값 복사한 필수 알림 함수다.</summary>
    private readonly Action<int> _notify;
    /// <summary>선택 때 요구하는 원문 진단 함수 사본이다.</summary>
    private readonly Action<string, string, uint>? _assertion;
    /// <summary>장치를 실행하지 않고 두 필수 함수를 확인해 복사한다.</summary>
    public RawPriestOwner(RawSidPool pool, SquidOwnerMode mode, PriestOwnerState state, PriestOwnerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(mode); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Base is null || hooks.Notify is null) throw new ArgumentException("사제 소유자 효과 누락", nameof(hooks));
        _pool = pool; _mode = mode; _state = state; _base = hooks.Base; _notify = hooks.Notify; _assertion = hooks.Assertion;
    }
    /// <summary>현재 가상 표가 판본의 실제 사제 표인지 확인한다.</summary>
    public bool Handles(int sid) => BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid)) == (_pool.Edition == OriginalEdition.Patch1078 ? 0x0050f210u : 0x005003e0u);
    /// <summary>최초 분기의 effective를 유지하고 각 후단의 모드/로딩/WORD만 다시 읽는다.</summary>
    public void Set(int sid, uint player)
    {
        if (!Handles(sid)) throw new ArgumentException("사제 소유자 가상 표 불일치", nameof(sid));
        Span<byte> raw = _pool.RawSlot(sid); bool patch = _pool.Edition == OriginalEdition.Patch1078, initialBattle = _mode.Battle;
        bool changesOriginal = !initialBattle || _state.LoadingDepth != 0; uint effective = changesOriginal ? player : (uint)(raw[12] & 0x7f);
        bool callsBase = (!changesOriginal || effective != 0) && (!patch || effective <= 8);
        if (patch && changesOriginal && player == 0)
            (_assertion ?? throw new InvalidOperationException("사제 원래 소유자0의 실제 진단 장치 미연결"))("newPlayerId != INVALID_PLAYER_ID", @".\Priest.cpp", initialBattle ? 0x5bu : 0x68u);
        if (changesOriginal && player != 0) { raw[12] = unchecked((byte)(((uint)raw[12] & 0x80u) | (player & 0x7fu))); _notify(sid); }
        if (callsBase) _base(sid, effective);
        if (_state.LoadingDepth == 0 || _mode.Battle)
        {
            bool local = (_mode.Fort && effective == _state.FortPlayer) || (_mode.Battle && effective == _state.BattlePlayer);
            raw[12] = (byte)((raw[12] & 0x7f) | (local ? 0x80 : 0)); _notify(sid);
        }
    }
    /// <summary>현재 실제 가상 표로 사제/다른 자산을 분배하고 외부 base 함수를 값 복사한다.</summary>
    public static Action<int, uint> MakePriestOwnerDispatch(RawPriestOwner priest, Action<int, uint> fallback)
    {
        ArgumentNullException.ThrowIfNull(priest); ArgumentNullException.ThrowIfNull(fallback);
        return (sid, player) => { if (priest.Handles(sid)) priest.Set(sid, player); else fallback(sid, player); };
    }
}
