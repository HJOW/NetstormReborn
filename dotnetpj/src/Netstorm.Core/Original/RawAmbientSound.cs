// Client의 공유 주변음 기한·종류·강제값과 같은 Client 수명이 쓰는 원본 전역을 공유한다.
namespace Netstorm.Core.Original;

// 주변음 모듈을 담는 네임스페이스다. 2026-11-19 0.633에서 R4-4 정기 처리용으로 추가했다.

/// <summary>원본 00544d58/50/54 또는 CD 대응 전역이다. 최초 BSS 0이며 Session 반납·설정으로 임의 변경하지 않는다.</summary>
/// <remarks>2026-11-19 0.633에서 추가했다. 근거: cpppj <c>o/RawAmbientSound.h</c>의 <c>AmbientSoundGlobals</c>.</remarks>
public sealed class AmbientSoundState
{
    /// <summary>State·Edit가 쓰는 같은 기한이다. 프레임 사본으로 바꾸지 않는다. 2026-11-19 0.633에서 추가했다.</summary>
    public double Deadline { get; set; }

    /// <summary>0천둥·1독수리·2고래이며 범위 밖 값도 그대로 보존한다. 2026-11-19 0.633에서 추가했다.</summary>
    public int Kind { get; set; }

    /// <summary>signed 양수일 때만 기존 kind를 유지한다. 원문 최초값은 0이다. 2026-11-19 0.633에서 추가했다.</summary>
    public int Forced { get; set; }
}

/// <summary>도달한 실제 장치만 요구하며 난수·시간 조건은 부모가 처리한다.</summary>
/// <remarks>2026-11-19 0.633에서 추가했다. 근거: cpppj <c>o/RawAmbientSound.h</c>의 <c>AmbientSoundHooks</c>.</remarks>
public sealed class AmbientSoundHooks
{
    /// <summary>비교와 재예약에 각각 현재 벽시각을 읽는다. 2026-11-19 0.633에서 추가했다.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>이름·반복·음량·좌우·우선순위·한도의 실제 재생이다. 2026-11-19 0.633에서 추가했다.</summary>
    public Action<string, uint, int, int, uint, int>? Sound { get; set; }

    /// <summary>소리 요청이 복귀한 뒤 현재 모달 토큰을 다시 읽는다. 2026-11-19 0.633에서 추가했다.</summary>
    public Func<uint>? CurrentDialog { get; set; }

    /// <summary>모달 0 천둥일 때만 화면 번쩍임을 요청한다. 2026-11-19 0.633에서 추가했다.</summary>
    public Action<int>? Flash { get; set; }
}

/// <summary>주변음 004180f0/CD 004eca20과 State의 같은 벽시각 기한을 직접 연결한다.</summary>
/// <remarks>2026-11-19 0.633에서 추가했다. 근거: cpppj <c>o/RawAmbientSound.h/.cpp</c>의 <c>StepAmbientSound</c>·<c>ResetAmbientSoundDeadline</c>.</remarks>
public static class RawAmbientSound
{
    /// <summary>두 판본의 실제 PE double 상수인 재예약 간격이다. 2026-11-19 0.633에서 추가했다.</summary>
    private const double AmbientInterval = 120.0;

    /// <summary>두 판본의 실제 PE double 상수인 재시작 지연이다. 2026-11-19 0.633에서 추가했다.</summary>
    private const double StateDelay = 10.0;

    /// <summary>004180d0/CD 004ec9e0의 현재 벽시각+10 재예약이다. 게임 시계·이전 프레임 캐시를 쓰지 않는다.</summary>
    /// <param name="state">기한을 쓸 주변음 상태다.</param>
    /// <param name="wallSeconds">현재 벽시각을 읽는다.</param>
    /// <exception cref="ArgumentNullException">상태·시계 장치가 없다.</exception>
    /// <remarks>2026-11-19 0.633에서 추가했다.</remarks>
    public static void ResetDeadline(AmbientSoundState state, Func<double> wallSeconds)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(wallSeconds);
        state.Deadline = wallSeconds() + StateDelay;
    }

    /// <summary>미래·비순서 기한이면 첫 시계 조회만 수행한다. 재예약은 두 번째 조회 뒤 난수보다 먼저 저장한다.</summary>
    /// <param name="state">직접 갱신하는 같은 Client 수명 주변음이다.</param>
    /// <param name="random">매 프레임 같은 게임 난수를 직접 소비한다.</param>
    /// <param name="hooks">도달한 실제 장치 묶음이다.</param>
    /// <exception cref="ArgumentNullException">상태·난수·장치가 없다.</exception>
    /// <exception cref="InvalidOperationException">도달한 실제 장치가 미연결이다.</exception>
    /// <remarks>2026-11-19 0.633에서 추가했다. Movie 갱신 잠금 없는 가지에서 바람 앞에 호출한다.</remarks>
    public static void Step(AmbientSoundState state, GameRandom random, AmbientSoundHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(hooks);
        double now = Need(hooks.WallSeconds)();
        if (!(state.Deadline <= now))
        {
            return;
        }

        state.Deadline = Need(hooks.WallSeconds)() + AmbientInterval;
        if (state.Forced <= 0)
        {
            state.Kind = unchecked((int)(random.Next(3000u) / 1000u));
        }

        if (state.Kind == 0)
        {
            Need(hooks.Sound)("thunderQuietDistant.wav", 0u, 0, 0, 0u, 0);
            if (Need(hooks.CurrentDialog)() == 0u)
            {
                Need(hooks.Flash)(0);
            }
        }
        else if (state.Kind == 1 || state.Kind == 2)
        {
            int pan = unchecked((int)random.Next(20000u)) - 10000;
            Need(hooks.Sound)(state.Kind == 1 ? "eagleScream.wav" : "whale.wav", 0u, 0, pan, 0u, 0);
        }
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식)이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("주변음 실제 장치 미연결");
        }

        return function;
    }
}
