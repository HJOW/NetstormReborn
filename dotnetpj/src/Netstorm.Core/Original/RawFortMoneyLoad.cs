using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// Fort Money 섹션의 실제 읽기 갈고리가 보는 같은 Client 전역 조회다(cpppj <c>o/RawFortMoneyLoad.h</c>의 <c>PlayerSpSetReads</c>).
/// 로컬·Editor 상태의 사본을 새로 만들지 않고 호출할 때마다 다시 조회한다.
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다.</remarks>
public sealed class PlayerSpSetReads
{
    /// <summary>현재 로컬 플레이어 SP·표시 경로 선택 값이다(원본 00595344).</summary>
    public Func<uint>? ClientActive { get; set; }

    /// <summary>현재 owner 재조회 값이다(원본 00540c70).</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>Editor 여부 재조회 값이다(원본 005c85a4).</summary>
    public Func<uint>? Editor { get; set; }
}

/// <summary>
/// Fort Money wrapper와 현재 플레이어 SP 지정이 읽는 나머지 전역이다(cpppj <c>o/RawFortMoneyLoad.h</c>의 <c>FortMoneyState</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다. 참조로 공유하며 수명은 호출자가 진다.</remarks>
public sealed class FortMoneyState
{
    /// <summary>원본 guard가 0이 아니면 시작 SP 적용을 생략한다(패치 005c85d4·CD 0051894c).</summary>
    public bool EconomyBlocked { get; set; }

    /// <summary>로컬 플레이어 SP·표시 경로를 선택한다(원본 00595344).</summary>
    public bool ClientActive { get; set; }

    /// <summary>실제 Template의 첫 DWORD가 0이 아니면 참이다. 거짓이면 보고 뒤 계속한다.</summary>
    public bool TemplatePresent { get; set; } = true;

    /// <summary>시작 SP 선택 인덱스다(원본 0052f7ee의 0..2로 1000/2500/3000을 고른다).</summary>
    public byte Difficulty { get; set; }

    /// <summary>CD판의 현재 시작 SP다(원본 005120d0의 signed 값을 float로 변환한다).</summary>
    public int CdStartingSp { get; set; }

    /// <summary>SP 검사 잔여 횟수다(원본 005c89c4). 조건이 맞으면 99를 쓴다.</summary>
    public uint SpCheckCount { get; set; }
}

/// <summary>
/// 원본 진단을 실제 소비자에게 전달하는 경계다(cpppj <c>o/RawFortMoneyLoad.h</c>의 <c>FortMoneyAssertion</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다.</remarks>
public enum FortMoneyAssertion
{
    /// <summary>Template의 첫 DWORD가 0인 원본 진단이다.</summary>
    MissingTemplate,

    /// <summary>실제 4바이트 소비 뒤 signed pos가 end를 넘은 원본 진단이다.</summary>
    StreamPastEnd,
}

/// <summary>
/// 원본 효과를 생략하지 않고 같은 로딩 상태·표시에 연결하는 경계다(cpppj <c>o/RawFortMoneyLoad.h</c>의 <c>FortMoneyHooks</c>).
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다.</remarks>
public sealed class FortMoneyHooks
{
    /// <summary>원본 assert 보고 뒤 계속하는 진단 경계다.</summary>
    public Action<FortMoneyAssertion>? Assertion { get; init; }

    /// <summary>동일 월드 모듈에 현재 깊이 DWORD를 전달한다.</summary>
    public Action<uint>? LoadingDepthChanged { get; init; }

    /// <summary>패치 로컬 SetPlayerSp의 실제 표시 갱신이다.</summary>
    public Action? RefreshSp { get; init; }
}

/// <summary>
/// 패치 004bf340→00483300·CD 004297f0→00414720의 현재 Money 실행 사슬이다(cpppj <c>o/RawFortMoneyLoad</c>).
/// 실제 4바이트 소비→모드별 현재 SP·난수·99회 검사 카운터 적용→깊이 복구 순서를 그대로 실행한다.
/// </summary>
/// <remarks>2026-11-02 F2a에서 추가했다. SP 저장소·장부·9개 Player 기록·현재 깊이는 이 객체보다 오래 살아야 한다.</remarks>
public sealed class RawFortMoneyLoad
{
    /// <summary>시작 SP 선택 표다(패치 0041c7d0의 실제 지역 배열이며 0..2로 조회한다).</summary>
    private static readonly int[] InitialSp = [1000, 2500, 3000];

    /// <summary>Money 시작 SP 지정에서 건너뛰는 현재 Player+50 비트다.</summary>
    private const byte ExcludedPlayer = 0x20;

    /// <summary>판본별 Money fort 값이다.</summary>
    private const float PatchFortSp = 194162.0f;

    /// <summary>CD판 Money fort 값이다.</summary>
    private const float CdFortSp = 100000.0f;

    /// <summary>모든 유효 owner의 special 지정값이다(로컬·비로컬 공통).</summary>
    private const float SpecialLocalSp = 194162.0f;

    /// <summary>패치 시작 SP의 가산 DWORD다.</summary>
    private const uint BonusSp = 4000;

    /// <summary>SP 검사 잔여 횟수의 설정값이다.</summary>
    private const uint SpCheckCount = 99;

    /// <summary>현재 판본이다(SP 저장·Player 기록 stride 선택).</summary>
    private readonly OriginalEdition _edition;

    /// <summary>기존 fort·mission·special 모드와 CD 현재 SP를 공유하는 보상 상태다.</summary>
    private readonly SquidRewardState _reward;

    /// <summary>현재 유효 localOwner 0..8을 읽는 동일 월드 장부다.</summary>
    private readonly RawPostPopState _players;

    /// <summary>원본 9개 Player의 실제 바이트 배열이다(판본별 stride).</summary>
    private readonly byte[] _playerRecords;

    /// <summary>기존 Fort·사제·표면과 동일한 현재 로딩 깊이를 읽고 쓴다.</summary>
    private readonly Func<uint> _getDepth;

    /// <summary>현재 로딩 깊이를 쓴다.</summary>
    private readonly Action<uint> _setDepth;

    /// <summary>현재 guard·client·difficulty·시작 SP·99회 검사 전역이다.</summary>
    private readonly FortMoneyState _state;

    /// <summary>실제 게임 난수를 소비하는 패치 분산 SP 저장소다. CD에서는 null이다.</summary>
    private readonly RawSpStore? _patchStore;

    /// <summary>실제 진단·로딩 상태 전달·표시 효과다.</summary>
    private readonly FortMoneyHooks _hooks;

    /// <summary>
    /// SP·장부·실제 9개 Player 기록·현재 깊이를 빌려 Money 로더를 만든다.
    /// </summary>
    /// <param name="edition">현재 판본이다.</param>
    /// <param name="reward">보상 상태다.</param>
    /// <param name="players">현재 localOwner가 있는 원본 장부다.</param>
    /// <param name="playerRecords">원본 9개 Player의 실제 바이트 배열이다.</param>
    /// <param name="getDepth">현재 로딩 깊이 읽기다.</param>
    /// <param name="setDepth">현재 로딩 깊이 쓰기다.</param>
    /// <param name="state">현재 guard·client·시작 SP·99회 검사 전역이다.</param>
    /// <param name="patchStore">패치 분산 SP 저장소다(CD는 null).</param>
    /// <param name="hooks">실제 진단·로딩 상태 전달·표시 효과다.</param>
    /// <exception cref="ArgumentException">판본별 Player 기록 크기·필수 효과·분산 저장소가 맞지 않는다.</exception>
    public RawFortMoneyLoad(OriginalEdition edition, SquidRewardState reward, RawPostPopState players,
        byte[] playerRecords, Func<uint> getDepth, Action<uint> setDepth,
        FortMoneyState state, RawSpStore? patchStore, FortMoneyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(reward);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(playerRecords);
        ArgumentNullException.ThrowIfNull(getDepth);
        ArgumentNullException.ThrowIfNull(setDepth);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        uint stride = edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
        if (playerRecords.Length != 9 * stride || hooks.Assertion is null || hooks.LoadingDepthChanged is null || hooks.RefreshSp is null ||
            (edition == OriginalEdition.Patch1078 && patchStore is null))
        {
            throw new ArgumentException("Fort Money 실제 Player·SP·필수 효과 누락", nameof(playerRecords));
        }
        _edition = edition;
        _reward = reward;
        _players = players;
        _playerRecords = playerRecords;
        _getDepth = getDepth;
        _setDepth = setDepth;
        _state = state;
        _patchStore = patchStore;
        _hooks = hooks;
    }

    /// <summary>
    /// 활성 로컬은 현재 owner를 재조회하고 그 밖에는 signed 번호 1~8일 때만 분산 잔액을 읽는다(원본 0040ef30).
    /// </summary>
    /// <param name="owner">조회할 소유자 번호.</param>
    /// <param name="store">분산 SP 저장소다.</param>
    /// <param name="reads">현재 Client·owner·Editor 조회다.</param>
    public static float ReadPlayerSpAmount(uint owner, RawSpStore store, PlayerSpSetReads reads)
    {
        ArgumentNullException.ThrowIfNull(store);
        ArgumentNullException.ThrowIfNull(reads);
        if (NeedSp(reads.ClientActive)() != 0 && owner == NeedSp(reads.LocalOwner)())
        {
            return BitConverter.UInt32BitsToSingle(store.Get(NeedSp(reads.LocalOwner)()));
        }
        int number = unchecked((int)owner);
        return number > 0 && number < 9 ? BitConverter.UInt32BitsToSingle(store.Get(owner)) : 0.0f;
    }

    /// <summary>
    /// 00447810의 같은 지정 몸체를 Fort Money와 Session 없는 State 정리가 공유한다. 표시 callback은 로컬 활성 분기에서만 필요하다.
    /// </summary>
    /// <param name="owner">지정할 소유자 번호(signed).</param>
    /// <param name="amount">지정할 금액(signed).</param>
    /// <param name="store">분산 SP 저장소다.</param>
    /// <param name="reads">현재 Client·owner·Editor 조회다.</param>
    /// <param name="refresh">로컬 활성의 표시 갱신이다.</param>
    public static void SetPlayerSpAmount(int owner, int amount, RawSpStore store, PlayerSpSetReads reads, Action refresh)
    {
        ArgumentNullException.ThrowIfNull(store);
        ArgumentNullException.ThrowIfNull(reads);
        ArgumentNullException.ThrowIfNull(refresh);
        if (NeedSp(reads.ClientActive)() != 0 && (uint)owner == NeedSp(reads.LocalOwner)())
        {
            float value = NeedSp(reads.Editor)() != 0 ? SpecialLocalSp : (float)amount;
            store.Set(NeedSp(reads.LocalOwner)(), BitConverter.SingleToUInt32Bits(value));
            NeedSp(refresh)();
        }
        else if (owner > 0 && owner < 9)
        {
            float value = NeedSp(reads.Editor)() != 0 ? SpecialLocalSp : (float)amount;
            store.Set((uint)owner, BitConverter.SingleToUInt32Bits(value));
        }
    }

    /// <summary>
    /// 실제 4바이트 소비→현재 모드별 SP 순서를 wrapper의 현재 깊이·Template 진단 뒤에 실행한다.
    /// </summary>
    /// <param name="stream">현재 원본 pos/end DWORD다. 독립 입력도 원본 getter 시점에 위치를 초기화한다.</param>
    public void Load(FortMoneyCursor stream)
    {
        ArgumentNullException.ThrowIfNull(stream);
        stream.Position = 0;
        LoadSelected(() => stream);
    }

    /// <summary>
    /// 실제 getter는 wrapper의 현재 깊이·진단 효과 뒤에 실행하고 실패하면 같은 원래 깊이를 복구한다.
    /// </summary>
    /// <param name="select">현재 노드를 선택해 같은 cursor를 돌려준다.</param>
    /// <exception cref="ArgumentException">실제 섹션 선택이 없다.</exception>
    public void LoadSelected(Func<FortMoneyCursor> select)
    {
        ArgumentNullException.ThrowIfNull(select);
        bool patch = _edition == OriginalEdition.Patch1078;
        if (patch && _reward.HalfHealthMode && !_state.EconomyBlocked)
        {
            _state.SpCheckCount = SpCheckCount;
        }
        ChangeDepth(_getDepth() + 1);
        try
        {
            if (!_state.TemplatePresent)
            {
                _hooks.Assertion!(FortMoneyAssertion.MissingTemplate);
            }
            ReadMoney(select());
        }
        catch
        {
            ChangeDepth(_getDepth() - 1);
            throw;
        }
        ChangeDepth(_getDepth() - 1);
    }

    /// <summary>
    /// 패치 00483300·CD 00414720의 Money 본문만 실행한다. wrapper·조회·깊이·99회 설정은 별도다.
    /// </summary>
    /// <param name="stream">현재 원본 pos/end DWORD다.</param>
    public void ReadMoney(FortMoneyCursor stream)
    {
        ArgumentNullException.ThrowIfNull(stream);
        bool patch = _edition == OriginalEdition.Patch1078;
        stream.Position += 4u;
        if (unchecked((int)stream.End) < unchecked((int)stream.Position))
        {
            _hooks.Assertion!(FortMoneyAssertion.StreamPastEnd);
        }
        if (_reward.FixedSpA)
        {
            if (patch)
            {
                _patchStore!.Set(_players.LocalOwner, BitConverter.SingleToUInt32Bits(PatchFortSp));
            }
            else
            {
                _reward.CdLocalSp = CdFortSp;
            }
        }
        if (_reward.HalfHealthMode && !_state.EconomyBlocked)
        {
            if (patch)
            {
                if (_state.Difficulty >= InitialSp.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(stream), _state.Difficulty, "Fort Money 실제 difficulty 배열 범위 오류");
                }
                int amount = unchecked((int)((uint)InitialSp[_state.Difficulty] + BonusSp));
                // 원본은 현재 Player 1..8의 flags를 각 반복에서 읽고 같은 게임 난수를 소비하는 SP 지정 몸체를 호출한다
                for (int owner = 1; owner < 9; owner++)
                {
                    if ((PlayerFlags((uint)owner) & ExcludedPlayer) == 0)
                    {
                        SetPlayerSp(owner, amount);
                    }
                }
            }
            else if ((PlayerFlags(_players.LocalOwner) & ExcludedPlayer) == 0)
            {
                _reward.CdLocalSp = (float)_state.CdStartingSp;
            }
        }
    }

    /// <summary>
    /// 패치 00447810: 유효 로컬·비로컬 모두 special 고정 SP를 적용한다. 로컬 활성 때만 표시를 갱신한다.
    /// </summary>
    /// <param name="owner">지정할 소유자 번호(signed).</param>
    /// <param name="amount">지정할 금액(signed).</param>
    /// <exception cref="InvalidOperationException">CD Money에 패치 SetPlayerSp를 호출했다.</exception>
    public void SetPlayerSp(int owner, int amount)
    {
        if (_edition != OriginalEdition.Patch1078)
        {
            throw new InvalidOperationException("CD Money에 패치 SetPlayerSp를 호출할 수 없습니다");
        }
        SetPlayerSpAmount(owner, amount, _patchStore!,
            new PlayerSpSetReads
            {
                ClientActive = () => _state.ClientActive ? 1u : 0u,
                LocalOwner = () => _players.LocalOwner,
                Editor = () => _reward.FixedSpB ? 1u : 0u,
            },
            _hooks.RefreshSp!);
    }

    /// <summary>
    /// 실제 physical Player 0..8의 현재 BYTE만 읽는다.
    /// </summary>
    /// <param name="owner">읽을 Player 번호.</param>
    /// <exception cref="ArgumentOutOfRangeException">실제 Player 범위 오류다.</exception>
    private byte PlayerFlags(uint owner)
    {
        if (owner >= 9)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "Fort Money 실제 Player 범위 오류");
        }
        uint stride = _edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
        return _playerRecords[owner * stride + 0x50u];
    }

    /// <summary>
    /// 원본이 선택한 실제 uint32 조회 장치를 그 호출 위치에서 요구한다.
    /// </summary>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">현재 Player SP 지정 장치가 없다.</exception>
    private static Func<uint> NeedSp(Func<uint>? function)
    {
        if (function is null)
        {
            throw new InvalidOperationException("현재 Player SP 지정 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 원본이 선택한 실제 표시 장치를 그 호출 위치에서 요구한다.
    /// </summary>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">현재 Player SP 지정 장치가 없다.</exception>
    private static Action NeedSp(Action? function)
    {
        if (function is null)
        {
            throw new InvalidOperationException("현재 Player SP 지정 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 원본 현재 로딩 깊이 쓰기와 같은 소비자의 관찰을 함께 실행한다.
    /// </summary>
    /// <param name="value">쓸 깊이 값.</param>
    private void ChangeDepth(uint value)
    {
        _setDepth(value);
        _hooks.LoadingDepthChanged!(value);
    }
}
