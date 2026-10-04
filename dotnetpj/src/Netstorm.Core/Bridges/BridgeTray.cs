using Netstorm.Core.Simulation;

namespace Netstorm.Core.Bridges;

/// <summary>
/// 플레이어 생산 창의 다리 조각 칸 (원본 Combatgump.cpp 매 프레임 처리 0043f2xx 부근).
/// 템플이 있는 동안 1초마다 조각 칸이 빌 때 하나씩 새 조각을 추첨해 채운다.
/// 템플이 없어지면 칸의 조각이 모두 사라진다 (사용자 확인 규칙 workshop-deck.md 와 exe 가 일치).
/// 칸은 자리가 고정된다: 조각을 집어도 그 칸에 어둡게 남아 칸 수를 차지하고, 놓은 뒤에야 칸이 비며
/// 새 조각은 빈 칸에 들어온다. 다른 조각은 자리를 옮기지 않는다
/// (2026-10-01 네 번째 녹화 01:11~01:20, 생산 창 조각 오브젝트가 집힌 동안 컨테이너에 남는 exe 처리와 일치).
/// </summary>
public sealed class BridgeTray
{
    /// <summary>새 조각을 넣는 간격(초) (원본 _DAT_00531b2c = 1, 시작 시 고정)</summary>
    public const double RefillIntervalSeconds = 1.0;

    /// <summary>한 칸 조각을 강제로 섞는 추첨 주기 (원본 DAT_00557dac % 5)</summary>
    public const int SinglePiecePeriod = 5;

    /// <summary>
    /// 칸에 들어온 조각이 금 간 품질(원본 품질 1)로 있는 시간(0.1초 단위).
    /// 원본 Combatgump.cpp(0043f33x 부근)은 품질 0 인 새 조각을 처음 훑을 때 품질 1 로 올리고 FUN_004af0b0(0x3c)로
    /// 타이머를 건 뒤, 타이머가 지나면(FUN_004af0e0) 품질 2(보통)로 올린다. 타이머는 게임 시각 × 10(0x512a78·0x501450 = ∓10.0)
    /// 의 정수값으로 비교하므로 0x3c = 60 은 **6초**다. 그 뒤로는 더 바뀌지 않는다(생산 창에서는 단단함이 되지 않는다).
    /// </summary>
    public const long CrackedDeciseconds = 60;

    /// <summary>칸 자리별 조각 (빈 칸은 null)</summary>
    private readonly BridgePiece?[] _slots;

    /// <summary>추첨 난수 (원본은 게임 전역 난수 하나를 여러 곳이 함께 쓴다)</summary>
    private readonly NetstormRandom _random;

    /// <summary>다음 조각을 넣을 수 있는 게임 시각(초)</summary>
    private double _nextRefillTime;

    /// <summary>직전 갱신 때 템플이 있었는지 (원본 Combatgump +0xdc)</summary>
    private bool _hadTemple;

    /// <summary>조각 칸 수 (전투 옵션 Bridge Slots: 2·4·6)</summary>
    public int Capacity { get; }

    /// <summary>
    /// 지금까지의 추첨 횟수. 원본은 프로세스 전역 변수(DAT_00557dac)라 미션이 바뀌어도 초기화하지 않는다.
    /// 클론은 칸마다 갖되 필요하면 이어받을 수 있게 설정 가능하게 둔다.
    /// </summary>
    public int DrawCount { get; set; }

    /// <summary>간격 제한 사용 여부 (원본 콘솔 명령 generatortimer/gt, 기본 켜짐)</summary>
    public bool TimerEnabled { get; set; } = true;

    /// <summary>칸 자리별 조각 (빈 칸은 null, 순번 = 생산 창의 칸 위치: 2열 행 우선)</summary>
    public IReadOnlyList<BridgePiece?> Slots => _slots;

    /// <summary>칸에 있는 조각들 (빈 칸을 건너뛴 자리 순서, 집고 있는 조각 포함)</summary>
    public IReadOnlyList<BridgePiece> Pieces => [.. _slots.OfType<BridgePiece>()];

    /// <summary>커서로 집어 어둡게 표시 중인 조각의 칸 번호 (없으면 null)</summary>
    public int? HeldSlot { get; private set; }

    /// <summary>조각 칸을 만든다</summary>
    /// <param name="capacity">칸 수 (BattleOptions.BridgeSlotCount)</param>
    /// <param name="random">추첨 난수</param>
    public BridgeTray(int capacity, NetstormRandom random)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(capacity);
        Capacity = capacity;
        _slots = new BridgePiece?[capacity];
        _random = random;
    }

    /// <summary>
    /// 한 프레임 갱신. 새 조각을 넣었으면 그 조각을 돌려준다.
    /// 원본 순서: 템플이 사라진 순간 칸 비우기 → 템플이 있고 간격이 지났으면 다음 시각 예약 →
    /// 칸이 남았으면 난수 추첨, 5번째 추첨마다(0, 5, 10…) 한 칸 조각이 없을 때 한 칸 조각으로 바꾼다.
    /// </summary>
    /// <param name="now">게임 시각(초)</param>
    /// <param name="hasTemple">플레이어에게 템플이 있는지 (원본 플레이어 +0x7c. 편집기에서는 참으로 준다)</param>
    public BridgePiece? Update(double now, bool hasTemple)
    {
        if (_hadTemple && !hasTemple)
        {
            // 템플을 잃는 순간 칸에 있던 조각이 모두 사라진다.
            Array.Clear(_slots);
            HeldSlot = null;
        }
        _hadTemple = hasTemple;
        if (!hasTemple || (TimerEnabled && now < _nextRefillTime))
        {
            return null;
        }
        // 원본은 칸이 가득 차 있어도 시각을 다시 예약한다 → 칸이 비면 최대 1초 뒤에 채워진다.
        _nextRefillTime = now + RefillIntervalSeconds;
        // 집은 조각도 칸을 차지하므로 빈 자리가 없으면 채우지 않는다
        int empty = Array.IndexOf(_slots, null);
        if (empty < 0)
        {
            return null;
        }
        int pattern = BridgePatternCatalog.Draw(_random.Next(BridgePatternCatalog.DrawRange));
        bool forceSingle = DrawCount % SinglePiecePeriod == 0;
        DrawCount++;
        if (forceSingle && !_slots.Any(p => p?.Pattern.Index == BridgePatternCatalog.SinglePiece))
        {
            pattern = BridgePatternCatalog.SinglePiece;
        }
        var piece = new BridgePiece(pattern) { CuredAtDeciseconds = ToDeciseconds(now) + CrackedDeciseconds };
        // 새 조각은 첫 빈 칸에 들어온다 (녹화: 놓아서 빈 칸이 같은 자리에서 다시 채워짐)
        _slots[empty] = piece;
        return piece;
    }

    /// <summary>
    /// 조각의 지금 품질: 칸에 들어온 뒤 6초 동안은 금 감, 그 뒤는 보통.
    /// 배치할 때 이 품질이 놓인 칸의 상태·수명을 정한다(<see cref="BridgeGrid.Place"/>).
    /// 원본은 칸에 있는 동안(생산 창 플래그 0x40)만 품질을 올린다. 집어 든 조각의 품질 변화는 확인하지 못해
    /// 클론은 들고 있는 동안에도 시각으로 계산한다(근사).
    /// </summary>
    /// <param name="piece">조각</param>
    /// <param name="now">게임 시각(초)</param>
    public static BridgeCondition QualityAt(BridgePiece piece, double now) =>
        ToDeciseconds(now) >= piece.CuredAtDeciseconds ? BridgeCondition.Normal : BridgeCondition.Cracked;

    /// <summary>게임 시각(초)을 원본 타이머 단위(0.1초, 소수점 버림 FUN_004e49c0)로 바꾼다</summary>
    /// <param name="seconds">게임 시각(초)</param>
    private static long ToDeciseconds(double seconds) => (long)Math.Truncate(seconds * 10.0);

    /// <summary>칸 자리에 집을 수 있는 조각이 있는지 (비었거나 이미 집은 칸은 아니다)</summary>
    /// <param name="slot">칸 번호</param>
    public bool CanTake(int slot) => slot >= 0 && slot < Capacity && _slots[slot] != null && HeldSlot != slot;

    /// <summary>칸에서 조각을 집는다 (커서로 옮김). 조각은 놓을 때까지 그 칸에 어둡게 남는다</summary>
    /// <param name="slot">칸 번호 (<see cref="CanTake"/> 가 참이어야 한다)</param>
    public BridgePiece Take(int slot)
    {
        if (!CanTake(slot))
        {
            throw new ArgumentOutOfRangeException(nameof(slot), slot, "집을 수 있는 조각이 없는 칸");
        }
        HeldSlot = slot;
        return _slots[slot]!;
    }

    /// <summary>
    /// 집었던 조각을 칸에 되돌린다. 매뉴얼의 "ESC 로 들고 있는 다리 반환"에 대응한다.
    /// 집은 칸이 남아 있으면 그 칸을 다시 밝게 하고, 템플을 잃어 칸이 비워진 뒤라면 첫 빈 칸에 넣는다. 빈 칸이 없으면 거부
    /// </summary>
    /// <param name="piece">되돌릴 조각</param>
    public bool Return(BridgePiece piece)
    {
        if (HeldSlot is int held && ReferenceEquals(_slots[held], piece))
        {
            HeldSlot = null;
            return true;
        }
        int empty = Array.IndexOf(_slots, null);
        if (empty < 0)
        {
            return false;
        }
        _slots[empty] = piece;
        return true;
    }

    /// <summary>집은 조각을 놓았을 때 그 칸을 비운다 (다음 채우기 때 같은 자리에 새 조각이 들어온다)</summary>
    public void ConsumeHeld()
    {
        if (HeldSlot is int held)
        {
            _slots[held] = null;
        }
        HeldSlot = null;
    }
}
