// 건설 취소의 실제 환불·SP 조회·가산을 복원한다. 배치/비용 부족 부모는 후속 이식이다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawConstructionPlace.h의 환불에서 소비하는 현재 전역이다.</summary>
public sealed class ConstructionPlaceState
{
    /// <summary>독립 입력의 로컬 Player 번호다.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>독립 입력의 전투 표시 상태다.</summary>
    public bool BattleShown { get; set; }
    /// <summary>환불에서 제외하는 독립 너겟 TYPE다.</summary>
    public uint NuggetType { get; set; } = 154;
    /// <summary>실제 장기 Player 번호를 각 소비 위치에서 읽는다.</summary>
    public Func<uint>? SharedLocalPlayer { get; set; }
    /// <summary>실제 PlayerKnown DWORD의 비영 전체를 소비한다.</summary>
    public Func<uint>? SharedPlayerKnown { get; set; }
    /// <summary>같은 장기 너겟 TYPE owner를 빌린다.</summary>
    public UIntBox? SharedNuggetType { get; set; }
    /// <summary>현재 로컬 번호이며 초기 사본을 만들지 않는다.</summary>
    public uint CurrentLocalPlayer() => SharedLocalPlayer?.Invoke() ?? LocalPlayer;
    /// <summary>현재 PlayerKnown의 비영 또는 독립 bool이다.</summary>
    public bool CurrentBattleShown() => SharedPlayerKnown is { } read ? read() != 0 : BattleShown;
    /// <summary>현재 너겟 TYPE를 선택한다.</summary>
    public uint CurrentNuggetType() => SharedNuggetType?.Value ?? NuggetType;
}

/// <summary>6ch SP 통지의 원문 7BYTE 본문이다. 송신 대상 DWORD는 별도로 유지한다.</summary>
public readonly record struct ConstructionMoneyNotice
{
    /// <summary>본문의 받는 Player 하위 BYTE다.</summary>
    public byte Player { get; init; }
    /// <summary>본문의 원천 Player 하위 BYTE다.</summary>
    public byte Source { get; init; }
    /// <summary>원문 환불의 통지 종류1이다.</summary>
    public byte Kind { get; init; }
    /// <summary>원문 signed64 절삭의 낮은 signed DWORD 금액이다.</summary>
    public int Amount { get; init; }
}

/// <summary>건설 환불과 SP 조회가 호출하는 실제 장부/통지 함수 값이다.</summary>
public sealed record ConstructionPlaceHooks
{
    /// <summary>현재 Player의 SP 저장소를 읽는다.</summary>
    public Func<uint, float>? StoredSp { get; init; }
    /// <summary>현재 로컬 Player의 SP를 가산한다.</summary>
    public Action<float>? AddLocalSp { get; init; }
    /// <summary>지정 Player의 SP를 가산한다.</summary>
    public Action<uint, float>? AddPlayerSp { get; init; }
    /// <summary>원문 대상 DWORD와 6ch 본문을 같은 실제 전송 장치로 보낸다.</summary>
    public Action<uint, ConstructionMoneyNotice>? SendMoney { get; init; }
}

/// <summary>cpppj/src/o/RawConstructionPlace.cpp의 Refund/Money/AddMoney 본체다. Place/Reject/Charge는 아직 제공하지 않는다.</summary>
public sealed class RawConstructionPlace
{
    /// <summary>원문 x87 signed64 정수 변환의 배타적 절댓값 상한이다.</summary>
    private const double FtolLimit = 9223372036854775808.0;
    /// <summary>하위 어댑터의 실제 동일 풀 확인에 사용한다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>환불 진입에서 선택하는 같은 현재 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>효과 사이에서도 현재 값을 다시 읽는 상태다.</summary>
    private readonly ConstructionPlaceState _state;
    /// <summary>생성 때 함수 값을 복사한 실제 장부/통지 장치다.</summary>
    private readonly ConstructionPlaceHooks _hooks;
    /// <summary>환불 전용 최소 모듈을 만든다. CD는 통지만 필요하며 PATCH는 장부 조회/가산도 요구한다.</summary>
    public RawConstructionPlace(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ConstructionPlaceState state, ConstructionPlaceHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.SendMoney is null ||
            (pool.Edition == OriginalEdition.Patch1078 && (hooks.StoredSp is null || hooks.AddLocalSp is null || hooks.AddPlayerSp is null))) throw new ArgumentException("건설 환불 TYPE/실제 장부·통지 연결 누락");
        Pool = pool; _types = types; _state = state; _hooks = hooks with { };
    }
    /// <summary>현재 TYPE 비용을 signed64로 절삭한 낮은 DWORD를 환불한다. CD는 양수 통지만 보낸다.</summary>
    public void Refund(uint type, uint player)
    {
        if (type >= _types.Count || type > byte.MaxValue) throw new ArgumentOutOfRangeException(nameof(type), "건설 환불 TYPE 범위");
        OriginalTypeRecord record = _types[(int)type]; bool patch = Pool.Edition == OriginalEdition.Patch1078;
        if (patch && type == _state.CurrentNuggetType()) return;
        double cost = record.Cost;
        int amount = !double.IsFinite(cost) || Math.Abs(cost) >= FtolLimit ? 0 : unchecked((int)(long)cost);
        if (patch && !(_state.CurrentBattleShown() && player == _state.CurrentLocalPlayer())) AddMoney(player, amount);
        if (amount > 0) _hooks.SendMoney!(player, new() { Player = unchecked((byte)player), Source = unchecked((byte)player), Kind = 1, Amount = amount });
    }
    /// <summary>전투 표시 중의 현재 로컬 Player는 범위 검사 없이 읽고 나머지는1~8만 읽는다.</summary>
    public float Money(uint player)
    {
        RequirePatch();
        if (_state.CurrentBattleShown() && player == _state.CurrentLocalPlayer()) return _hooks.StoredSp!(_state.CurrentLocalPlayer());
        int number = unchecked((int)player); return number > 0 && number < 9 ? _hooks.StoredSp!(player) : 0f;
    }
    /// <summary>현재 로컬 여부에 따라 원문 로컬 또는 지정 Player 가산 함수를 선택한다.</summary>
    public void AddMoney(uint player, float delta)
    {
        RequirePatch(); if (player == _state.CurrentLocalPlayer()) _hooks.AddLocalSp!(delta); else _hooks.AddPlayerSp!(player, delta);
    }
    /// <summary>CD에 존재하지 않는 SP 조회/가산의 직접 호출을 거부한다.</summary>
    private void RequirePatch() { if (Pool.Edition != OriginalEdition.Patch1078) throw new InvalidOperationException("건설 SP 조회/가산은 PATCH 전용"); }
    /// <summary>실제 동일 풀의 SP 저장소·보상 가산만 연결한다. 통지 함수는 호출자가 제공한다.</summary>
    public static ConstructionPlaceHooks MakeHooks(RawSidPool pool, RawSquidReward reward, RawSpStore store, ConstructionPlaceState state, ConstructionPlaceHooks hooks)
    {
        if (!ReferenceEquals(pool, reward.Pool)) throw new ArgumentException("건설 환불의 실제 보상 풀 불일치");
        if (pool.Edition != OriginalEdition.Patch1078) return hooks with { };
        ArgumentNullException.ThrowIfNull(store);
        return hooks with { StoredSp = player => BitConverter.UInt32BitsToSingle(store.Get(player)), AddLocalSp = delta => reward.AddSp(state.CurrentLocalPlayer(), delta), AddPlayerSp = reward.AddSp };
    }
}
