// Client의 공유 바람과 같은 Client 수명이 쓰는 원문 전역을 공유한다.
namespace Netstorm.Core.Original;

// 주변 바람 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.

// PATCH 세 float와 CD 세 float의 같은 Client 수명 바람이다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.
public sealed class AmbientWindState
{
    /// <summary>X 바람은 실제 PE BSS 0이며 연소 입자의 가속에 사용한다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    public float X { get; set; }

    /// <summary>Y 바람의 실제 PE 최초값 1이며 각도 계산 전에 매번 감쇠한다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    public float Y { get; set; } = 1.0f;

    /// <summary>마지막 잡음 분기에서 갱신한 도 각도이며 나머지 프레임에는 보존한다. 2026-11-07 F2d 서피스 월드 바람 이식에서 추가됐다.</summary>
    public float Angle { get; set; }
}

// 감쇠 두 저장·100난수·잡음 난수·X 저장·현재 각도 저장을 유지하는 바람 진행기다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.
public static class RawAmbientWind
{
    /// <summary>실제 배정밀도 감쇠 상수다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const double Damping = 0.9;

    /// <summary>실제 배정밀도 각도 환산 상수다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const double DegreesPerRadian = 57.295827908797776;

    /// <summary>두 판본의 잡음 하한 입력이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const float NoiseMinimum = -0.06f;

    /// <summary>두 판본의 잡음 상한 입력이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const float NoiseMaximum = 0.06f;

    /// <summary>잡음 분기에 들어가는 100난수 상한이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const uint NoiseEnterLimit = 100u;

    /// <summary>잡음 분기에 들어가는 역치다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const uint NoiseEnterThreshold = 10u;

    /// <summary>잡음 값을 뽑는 65536난수 상한이다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    private const uint NoiseValueLimit = 65536u;

    /// <summary>게임 시계 정지와 별개인 매 프레임 호출이다. 시간 배율·재시드·독립 난수를 추가하지 않는다. 2026-11-07 F2d 바람 진행 이식에서 추가됐다.</summary>
    /// <param name="state">직접 갱신하는 같은 Client 수명 바람이다.</param>
    /// <param name="random">매 프레임 같은 게임 난수를 직접 소비한다.</param>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="diagnostics">각도 부모가 빌리는 현재 검사·진단 묶음이다. null이면 독립 기본값을 쓴다.</param>
    public static void Step(AmbientWindState state, GameRandom random, OriginalEdition edition, BoardAngleDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(random);

        // 두 판본 모두 같은 감쇠 두 저장을 먼저 한다
        state.X = (float)((double)state.X * Damping);
        state.Y = (float)((double)state.Y * Damping);

        // 100난수가 역치 이상이면 잡음 없이 기존 각도와 두 번째 난수를 보존한다
        if (random.Next(NoiseEnterLimit) >= NoiseEnterThreshold)
        {
            return;
        }

        uint value = random.Next(NoiseValueLimit);
        double width = Math.Abs((double)NoiseMaximum - (double)NoiseMinimum);

        // 두 판본의 잡음 곱셈 순서를 구별한다
        double offset = edition == OriginalEdition.Patch1078
            ? (double)value * (1.0 / 65536.0) * width + (double)NoiseMinimum
            : width * value * (1.0 / 65536.0) + (double)NoiseMinimum;
        state.X = (float)(offset + state.X);
        float degrees = (float)(Math.Atan2(-(double)state.Y, (double)state.X) * DegreesPerRadian);
        state.Angle = (float)RawBoardAngle.Normalize(degrees, edition, diagnostics);
    }
}
