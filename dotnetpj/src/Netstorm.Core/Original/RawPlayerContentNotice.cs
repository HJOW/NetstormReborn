using System.Buffers.Binary;

// 실제 SP 진단 접두 뒤 현재 Player의 GumpForm 종속에 내용물 통지를 전달한다.
namespace Netstorm.Core.Original;

/// <summary>원문 위치에서 현재 진단/SP/Player/Form 전역을 읽는 뷰다.</summary>
public sealed class PlayerContentNoticeState
{
    /// <summary>00568af8의 현재 SP 분산 배열 커서다.</summary>
    public Func<uint>? CursorA { get; set; }
    /// <summary>0054d4e0의 현재 비교 추적 커서다.</summary>
    public Func<uint>? CursorB { get; set; }
    /// <summary>005318ec의 같은 현재 진단 DWORD다.</summary>
    public UIntBox ProblemCode { get; set; } = new();
    /// <summary>00540cb8을 호출 앞/조건부 SP조회 뒤에 다시 읽는다.</summary>
    public Func<uint>? FallbackCode { get; set; }
    /// <summary>005c89c4의 현재 SP검사 횟수이며 감김으로 감소한다.</summary>
    public UIntBox MoneyChecks { get; set; } = new();
    /// <summary>00595344 PlayerKnown의 현재 전체 DWORD다. inBattle과 구분한다.</summary>
    public Func<uint>? BattleShown { get; set; }
    /// <summary>00540c70의 실제 SP조회 소유자다.</summary>
    public Func<uint>? LocalPlayer { get; set; }
    /// <summary>00594fd0의 실제 첫 SP 배열이 존재하는지 현재 owner에서 읽는다.</summary>
    public Func<bool>? HasFirstSpPool { get; set; }
    /// <summary>00531890의 정확한 signed WORD다.</summary>
    public Func<short>? FirstPermutation { get; set; }
    /// <summary>0054107c의 현재 GumpForm 종류 DWORD다.</summary>
    public Func<uint>? GumpFormType { get; set; }
    /// <summary>005427bc의 현재 Gump 내용물 사건 DWORD다.</summary>
    public Func<uint>? ContentEvent { get; set; }
}

/// <summary>실제 SP getter와 현재 Gump 등록 조회/가상 처리 장치다.</summary>
public sealed class PlayerContentNoticeHooks
{
    /// <summary>현재 소유자의 실제 분산 SP를 float로 읽는다.</summary>
    public Func<uint, float>? Money { get; set; }
    /// <summary>원문 DWORD 등록 ID로 현재 Gump를 찾고 없으면0이다.</summary>
    public Func<uint, uint>? Gump { get; set; }
    /// <summary>현재 Gump 가상+28에 사건/원문 form/reason을 전달한다.</summary>
    public Action<uint, uint, uint, uint>? Handle { get; set; }
}

/// <summary>PATCH00490750→004d55e0의 SP 진단과 현재 Player 종속 UI 통지 본체다.</summary>
/// <remarks>근거: cpppj/src/o/RawPlayerContentNotice.cpp/.h. 상태 필터나 추가 owner 관문은 넣지 않는다.</remarks>
public sealed class RawPlayerContentNotice
{
    /// <summary>원문 ROM00505514의 단정밀도 SP 초과 기준이다.</summary>
    private const float MoneyThreshold = 7000f;
    /// <summary>현재 Player와 종속 체인의 같은 PATCH 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>효과 뒤에도 같은 현재 전역을 읽는 뷰다.</summary>
    private readonly PlayerContentNoticeState _state;
    /// <summary>실제 SP와 Gump 장치다.</summary>
    private readonly PlayerContentNoticeHooks _hooks;
    /// <summary>실제 풀·SP·등록표/UI 경계만 확인하며 슬롯이나 전역은 읽지 않는다.</summary>
    public RawPlayerContentNotice(RawSidPool pool, PlayerContentNoticeState state, PlayerContentNoticeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || hooks.Money is null || hooks.Gump is null || hooks.Handle is null)
            throw new ArgumentException("Player 내용물의 현재 실제 SP/등록표/UI 경계 누락");
        _pool = pool; _state = state; _hooks = hooks;
    }
    /// <summary>상위 부모 분배기가 같은 실제 풀을 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>SP 접두 뒤 현재 head를 읽고 각 next를 효과 전에 보존한 채 같은 사건을 전달한다.</summary>
    public void Notify(int parent, uint form, uint reason)
    {
        uint fallback = Need(_state.FallbackCode)();
        if (Need(_state.CursorA)() != Need(_state.CursorB)()) _state.ProblemCode.Value = fallback;
        if (_state.MoneyChecks.Value != 0)
        {
            if (Need(_state.BattleShown)() != 0)
            {
                float money = Need(_hooks.Money)(Need(_state.LocalPlayer)());
                fallback = Need(_state.FallbackCode)();
                if (money > MoneyThreshold) _state.ProblemCode.Value = fallback;
            }
            _state.MoneyChecks.Value = unchecked(_state.MoneyChecks.Value - 1);
        }
        if (!Need(_state.HasFirstSpPool)()) _state.ProblemCode.Value = fallback;
        if (Need(_state.FirstPermutation)() == -1) _state.ProblemCode.Value = fallback;
        int current = BinaryPrimitives.ReadUInt16LittleEndian(_pool.Slot(parent)[6..]); var visited = new HashSet<int>();
        // 각 next는 등록표 조회/가상 처리보다 먼저 저장하며 현재 종류와 사건은 원문 위치에서 읽는다.
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current)) throw new InvalidOperationException("Player 내용물 UI 종속 체인 물리 범위/순환 오류");
            ReadOnlySpan<byte> raw = _pool.Slot(current); uint type = raw[10], gumpType = Need(_state.GumpFormType)();
            int next = BinaryPrimitives.ReadUInt16LittleEndian(raw[4..]);
            if (type == gumpType)
            {
                uint token = Need(_hooks.Gump)(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]));
                if (token != 0) Need(_hooks.Handle)(token, Need(_state.ContentEvent)(), form, reason);
            }
            current = next;
        }
    }
    /// <summary>선택된 실제 현재 전역/효과가 없으면 호출 위치에서 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("Player 내용물 통지의 실제 전역/효과 미연결");
}
