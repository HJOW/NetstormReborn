// 원본 게임 전역 난수와 CRT 난수 및 시각 기반 재시드 몸체를 공유한다.
namespace Netstorm.Core.Original;

// 게임 난수 모듈을 담는 네임스페이스다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.

// 원본 게임 전역 정수 난수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.
public sealed class GameRandom
{
    /// <summary>상태가 0일 때 대신 쓰는 고정 시드다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    public const uint SeedReplacement = 0x0bad0badu;

    /// <summary>상태 전이 곱셈 계수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint Multiplier = 0x10003u;

    /// <summary>상태 전이 덧셈 계수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint Increment = 3u;

    /// <summary>다음 계산에 소비·갱신하는 32비트 생성기 상태다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private uint _state;

    /// <summary>원본 전역의 현재 상태를 그대로 받아 만든다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">시작 상태다. 0은 첫 호출에서 고정 시드로 바뀐다.</param>
    public GameRandom(uint state = 0u)
    {
        _state = state;
    }

    /// <summary>한 번 전진한 뒤 [0, 상한) 값을 반환한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="limit">상한이다. 0은 원본의 0 나눗셈이므로 거부한다.</param>
    /// <returns>난수 결과다.</returns>
    public uint Next(uint limit)
    {
        // 상한 검사를 먼저 한다
        if (limit == 0u)
        {
            throw new ArgumentOutOfRangeException(nameof(limit), "게임 난수의 상한은 0일 수 없습니다.");
        }

        // 0 상태 대체를 먼저 한다
        if (_state == 0u)
        {
            _state = SeedReplacement;
        }

        // 32비트 감김 곱셈·덧셈으로 전진한다
        _state = unchecked(_state * Multiplier + Increment);

        // 상위 16비트만 나머지에 쓴다
        return (_state >> 16) % limit;
    }

    /// <summary>진단·기계어 대조용 현재 상태를 읽는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>현재 32비트 상태다.</returns>
    public uint State() => _state;

    /// <summary>시각 기반 재시드가 만든 새 상태를 주입한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">주입할 32비트 상태다.</param>
    public void SetState(uint state)
    {
        _state = state;
    }
}

// 원본 MSVC CRT 난수와 별개인 스레드 스트림이다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.
public sealed class CrtRandom
{
    /// <summary>상태 전이 곱셈 계수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint Multiplier = 0x343fdu;

    /// <summary>상태 전이 덧셈 계수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint Increment = 0x269ec3u;

    /// <summary>결과 15비트 마스크다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    public const uint MaxValue = 0x7fffu;

    /// <summary>같은 스레드의 다음 CRT 난수가 소비할 자체 32비트 상태다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private uint _state;

    /// <summary>비어 있으면 자체 상태를 쓰고 비어 있지 않으면 기존 원본 CRT 상태를 직접 갱신하는 공유 상자다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private readonly UIntBox? _shared;

    /// <summary>원본 새 스레드 저장소의 초기값 1로 만든다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">시작 상태다.</param>
    public CrtRandom(uint state = 1u)
    {
        _state = state;
    }

    /// <summary>별도 저장소를 초기화하지 않고 기존 상태 상자의 주소를 유지하는 빌림을 만든다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">빌릴 외부 상태 상자다. 이 객체보다 오래 살아야 한다.</param>
    /// <returns>외부 상자를 직접 읽고 쓰는 난수다.</returns>
    public static CrtRandom Borrow(UIntBox state)
    {
        ArgumentNullException.ThrowIfNull(state);
        return new CrtRandom(state);
    }

    /// <summary>외부 상자를 빌린 비공개 생성자다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="shared">빌릴 외부 상자다.</param>
    private CrtRandom(UIntBox shared)
    {
        _shared = shared;
    }

    /// <summary>상태×214013+2531011의 하위 32비트를 저장하고 상위 15비트를 반환한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>15비트 난수 결과다.</returns>
    public uint Next()
    {
        // 공유 상자가 있으면 그 값을 직접 전진시킨다
        if (_shared is not null)
        {
            _shared.Value = unchecked(_shared.Value * Multiplier + Increment);
            return (_shared.Value >> 16) & MaxValue;
        }

        // 자체 상태를 전진시킨다
        _state = unchecked(_state * Multiplier + Increment);
        return (_state >> 16) & MaxValue;
    }

    /// <summary>독립 원문 대조·재현에서 실제 현재 값을 읽는다. 스트림을 전진시키지 않는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>현재 32비트 상태다.</returns>
    public uint State() => _shared is not null ? _shared.Value : _state;

    /// <summary>원본 srand의 스레드 상태 쓰기와 같은 명시 재시드다. 0도 유효하다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">쓸 32비트 상태다.</param>
    public void SetState(uint state)
    {
        // 공유 상자가 있으면 그곳에 쓴다
        if (_shared is not null)
        {
            _shared.Value = state;
            return;
        }

        _state = state;
    }

    /// <summary>각 스레드에 하나만 두는 원본 스레드 수명 난수다. 세션·미션 재생성으로 초기화하지 않는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private static readonly ThreadLocal<CrtRandom> ThreadState = new(() => new CrtRandom());

    /// <summary>같은 스레드의 모든 CRT 소비자가 공유하는 스레드 난수를 빌린다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>스레드 공유 난수다.</returns>
    public static CrtRandom Thread() => ThreadState.Value!;
}

// 원본 게임 난수 초기화와 SP용 임시 시각 재시드를 같은 CRT·게임 상태에 연결한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.
public sealed class RawGameRandomSeed
{
    /// <summary>모드 0이 선택하는 원본 고정 시드다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    public const uint FixedSeed = 0x38d535u;

    /// <summary>실제 MSVC CRT의 상태 곱셈 상수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint CrtMultiplier = 0x343fdu;

    /// <summary>실제 MSVC CRT의 상태 덧셈 상수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint CrtIncrement = 0x269ec3u;

    /// <summary>원본 초기화가 버리는 워밍업 횟수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private const uint Warmup = 100u;

    /// <summary>SP·공격·생성이 함께 쓰는 같은 원본 게임 난수다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private readonly GameRandom _game;

    /// <summary>원본 같은 스레드의 CRT 난수 상태 상자다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private readonly UIntBox _crt;

    /// <summary>실제 전역 DWORD이며 0만 고정 시드를 선택하는 모드 상자다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private readonly UIntBox _mode;

    /// <summary>실제 시간 함수의 부호 있는 유닉스 초 입력 경계다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    private readonly Func<int> _unixSeconds;

    /// <summary>모든 참조와 유닉스 초 공급자는 이 객체보다 오래 살아야 한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="game">같은 원본 게임 난수다.</param>
    /// <param name="crt">같은 스레드의 CRT 상태 상자다.</param>
    /// <param name="mode">실제 모드 DWORD 상자다.</param>
    /// <param name="unixSeconds">유닉스 초 공급자다. null이면 실제 세션 초기화 전에 거부한다.</param>
    public RawGameRandomSeed(GameRandom game, UIntBox crt, UIntBox mode, Func<int> unixSeconds)
    {
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(unixSeconds);
        _game = game;
        _crt = crt;
        _mode = mode;
        _unixSeconds = unixSeconds;
    }

    /// <summary>실제 MSVC CRT 한 번의 상태 갱신과 15비트 반환이다. 같은 CRT를 지형 소비자와 공유한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <param name="state">갱신할 32비트 상태 저장소다.</param>
    /// <returns>상위 15비트 결과다.</returns>
    public static uint NextCrt(ref uint state)
    {
        // 부호 없는 연산으로 실제 32비트 감김을 유지한다
        state = unchecked(state * CrtMultiplier + CrtIncrement);
        return (state >> 16) & 0x7fffu;
    }

    /// <summary>현재 모드에 따라 시드를 고르고 100회를 버린 뒤 세 난수 곱을 게임 난수에 기록한다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>게임 난수에 기록한 시드다.</returns>
    public uint Initialize()
    {
        // 시각의 -1 및 부호 있는 32비트도 srand의 부호 없는 상태로 그대로 전달한다
        uint crt = _mode.Value == 0u ? FixedSeed : unchecked((uint)_unixSeconds());

        // 원본과 같은 100회 소비를 수행하며 반환값은 사용하지 않는다
        for (uint i = 0u; i < Warmup; ++i)
        {
            NextCrt(ref crt);
        }

        uint first = NextCrt(ref crt);
        uint second = NextCrt(ref crt);
        uint third = NextCrt(ref crt);
        uint seed = unchecked(first * second * third);
        _crt.Value = crt;
        _game.SetState(seed);
        return seed;
    }

    /// <summary>모드를 1로 임시 변경한 뒤 초기화하고 원래 DWORD를 복원한다. 공급자 예외 때도 임시 모드를 외부에 남기지 않는다. 2026-11-07 F2d 게임 난수 이식에서 추가됐다.</summary>
    /// <returns>게임 난수에 기록한 시드다.</returns>
    public uint Reseed()
    {
        uint previous = _mode.Value;
        _mode.Value = 1u;
        try
        {
            uint seed = Initialize();
            _mode.Value = previous;
            return seed;
        }
        catch
        {
            _mode.Value = previous;
            throw;
        }
    }
}
