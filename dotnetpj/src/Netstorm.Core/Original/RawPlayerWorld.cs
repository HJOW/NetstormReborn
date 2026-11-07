// 실제 Player152의 첫 생성·PostPop·Technology SID 조회를 같은 월드 기록에 연결한다.
namespace Netstorm.Core.Original;

// Player 월드 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 원본 Player 생성과 명시 owner Technology 조회의 진단 종류다. 2026-11-07 F2d Player 이식에서 추가됐다.
public enum PlayerWorldAssertion
{
    /// <summary>현재 PlayerKnown이 0일 때 원본 assert를 전달한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    PlayerIdKnown,

    /// <summary>같은 좌표의 현재 playerType이 이미 존재할 때 원본 assert를 전달한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    PlayerAlreadyExists,
}

// 실제 Factory·Owner·Pop·예약을 도달 위치에서 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerWorldHooks
{
    /// <summary>원본 assert 후 계속 수행하는 진단 경계다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<PlayerWorldAssertion>? Assertion { get; set; }

    /// <summary>같은 Pool·Hash 현재 타입 검색이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<float, float, uint, int>? Find { get; set; }

    /// <summary>같은 실제 Factory의 생성이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>가상의 실제 소유자 호출이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>가상의 실제 공간·Player 후처리·표시·장부다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>원본 할당의 성공·실패를 보존한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>실제 Regular·같은 Host 생성이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }
}

// 현재 Player 9기록에 Technology 부모 SID를 직접 기록하며 별도 Player 복사본을 만들지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class RawPlayerWorld
{
    /// <summary>실제 Player 한 기록 크기다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Stride = 0xb4;

    /// <summary>실제 Player 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Count = 9;

    /// <summary>실제 Technology DWORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int TechnologyField = 0x9c;

    /// <summary>슬롯 안 소유자 바이트 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 직접 프레임 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int FrameOffset = 36;

    /// <summary>실제 Player152 생성자가 쓰는 PE 가상표다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const uint PatchPlayerVtable = 0x50ee08u;

    /// <summary>실제 Factory·Pop·Host와 같은 살아 있는 원시 풀이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>가상 재분배 없이 실제 마지막 공통 장부 몸체를 실행한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly RawPostPop _base;

    /// <summary>돈·메타·AI와 공유하는 실제 현재 9개 Player 바이트다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 PlayerKnown·두 소유자·fort·special을 호출마다 다시 조회한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerWorldState _state;

    /// <summary>원본 필수 하위 생성·공간·예약·진단의 실제 연결이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerWorldHooks _hooks;

    /// <summary>
    /// 같은 풀·실제 9기록을 확인한다. 이 첫 구현은 PATCH 전용이며 CD 판본·전수 장면 재생은 후속 단위에서 잇는다.
    /// </summary>
    /// <param name="pool">실제 Factory·Pop·Host와 같은 살아 있는 원시 풀이다.</param>
    /// <param name="inner">실제 마지막 공통 장부 몸체다.</param>
    /// <param name="playerRecords">현재 9개 Player 바이트다.</param>
    /// <param name="state">현재 전역이다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <exception cref="ArgumentException">Player 실제 같은 풀·9기록·판본 누락이다.</exception>
    public RawPlayerWorld(RawSidPool pool, RawPostPop inner, byte[] playerRecords, PlayerWorldState state, PlayerWorldHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(inner);
        ArgumentNullException.ThrowIfNull(playerRecords);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(inner.Pool, pool) || playerRecords.Length != Count * Stride || pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("Player 실제 같은 풀·9기록·판본 누락");
        }

        _pool = pool;
        _base = inner;
        _players = playerRecords;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>
    /// 찾기→Factory→소유자→직접 프레임 0→가상 Pop 0을 순서대로 실행한다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>만든 슬롯 번호다.</returns>
    public int Create(int owner)
    {
        if (_state.PlayerKnown == 0u)
        {
            Need(_hooks.Assertion)(PlayerWorldAssertion.PlayerIdKnown);
        }

        int sum = unchecked(owner + 10);
        float x = sum;
        float y = _state.Special != 0u ? 10.0f : 1.0f;
        if (Need(_hooks.Find)(x, y, _state.PlayerType) != 0)
        {
            Need(_hooks.Assertion)(PlayerWorldAssertion.PlayerAlreadyExists);
        }

        int sid = Need(_hooks.Create)(_state.PlayerType, 2u);
        Need(_hooks.SetOwner)(sid, unchecked((uint)owner));
        Write(_pool.AllocatedBytes(sid), FrameOffset, 0u, 4);
        Need(_hooks.Pop)(sid, x, y, 0u);
        return sid;
    }

    /// <summary>
    /// 현재 원본 소유자·SID를 기록한 뒤 실제 할당 성공 조건과 마지막 공통 후처리를 유지한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    public void PostPop(int sid, uint flags)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        Write(_players.AsSpan(), TechnologyOffset(raw[OwnerOffset]), unchecked((uint)sid), 4);
        if ((flags & 1u) != 0u && raw[OwnerOffset] == _state.BattleOwner && !_state.FortMode)
        {
            if (Need(_hooks.ReserveRegular)())
            {
                Need(_hooks.AddRegular)(sid, 1u, 5.0f);
            }
        }

        if ((flags & 1u) != 0u && _state.Special != 0u)
        {
            Write(_pool.AllocatedBytes(sid), FrameOffset, 1u, 4);
        }

        _base.PostPopBase(sid, flags);
    }

    /// <summary>실제 Player 가상표를 확인한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호다.</param>
    /// <returns>Player면 참이다.</returns>
    public bool Handles(int sid)
    {
        return Read(_pool.Slot(sid), 0) == PatchPlayerVtable;
    }

    /// <summary>
    /// Player 기록·공통 후처리와 선택된 실제 예약을 공간 변경 전에 검사한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    /// <exception cref="InvalidOperationException">미복원 Player 가상 후처리 표다.</exception>
    public void Validate(int sid, uint flags)
    {
        if (!Handles(sid))
        {
            throw new InvalidOperationException("미복원 Player 가상 후처리 표");
        }

        Span<byte> raw = _pool.AllocatedBytes(sid);
        TechnologyOffset(raw[OwnerOffset]);
        _base.ValidateBase(sid, flags);
    }

    /// <summary>
    /// 아직 Player 번호가 알려지지 않았으면 원본처럼 실제 기록을 조회하지 않고 0을 반환한다.
    /// </summary>
    /// <returns>현재 전투 소유자의 Technology 값이다.</returns>
    public uint CurrentTechnology()
    {
        return _state.PlayerKnown != 0u ? Read(_players.AsSpan(), TechnologyOffset(_state.BattleOwner)) : 0u;
    }

    /// <summary>명시 소유자 조회는 원본 PlayerKnown 진단 뒤 같은 9기록 DWORD를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>명시 소유자의 Technology 값이다.</returns>
    public uint Technology(int owner)
    {
        if (_state.PlayerKnown == 0u)
        {
            Need(_hooks.Assertion)(PlayerWorldAssertion.PlayerIdKnown);
        }

        return Read(_players.AsSpan(), TechnologyOffset(unchecked((uint)owner)));
    }

    /// <summary>같은 풀 분배·Host 연결을 검증할 때 참조를 제공한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool() => _pool;

    /// <summary>실제 Player 저장소의 같은 바이트 뷰이며 새 기록 사본을 만들지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>9기록 바이트다.</returns>
    public byte[] Records() => _players;

    /// <summary>최초 예약·getter가 읽는 현재 전역을 상위 부모에도 같은 참조로 제공한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>현재 전역이다.</returns>
    public PlayerWorldState State() => _state;

    /// <summary>실제 9기록 안의 Technology DWORD 위치를 계산한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>기록 안 위치다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Player Technology 소유자의 실제 9기록 범위 오류다.</exception>
    private static int TechnologyOffset(uint owner)
    {
        if (owner >= (uint)Count)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "Player Technology 소유자의 실제 9기록 범위 오류");
        }

        return unchecked((int)(owner * Stride + TechnologyField));
    }

    /// <summary>정렬되지 않은 원본 DWORD를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="field">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int field)
    {
        uint value = 0u;
        // 실제 네 바이트를 낮은 비트부터 조립하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[field + index] << (index * 8);
        }

        return value;
    }

    /// <summary>원본이 지정한 낮은 바이트부터 필드 폭만큼 저장한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="field">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 폭이다.</param>
    private static void Write(Span<byte> raw, int field, uint value, int width)
    {
        // 주변 바이트를 보존하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            raw[field + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">Player 월드의 선택된 실제 하위 미연결이다.</exception>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Player 월드의 선택된 실제 하위 미연결");
        }

        return callback;
    }
}
