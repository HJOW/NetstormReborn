namespace Netstorm.Core.Simulation;

/// <summary>
/// 이동형 유닛이 바라보는 8방향. 원본 .type 클러스터 이름의 측면 글자(A~H)와 같은 순서이며
/// 2026-10-03 원본 자동 분석 녹화(사제 이동 5구간·골렘 이동)의 걷는 프레임을 스프라이트와 맞춰 확정했다:
/// A=북, B=북동, C=동, D=남동, E=남, F=남서, G=서, H=북서 (docs/videos/auto-war-begins-20261003.md).
/// 화면 연출 전용 상태이며 게임 규칙·검사합에는 들어가지 않는다.
/// </summary>
public static class UnitHeading
{
    /// <summary>아직 한 번도 움직이지 않아 저장 프레임·기본 프레임을 그대로 쓰는 상태.</summary>
    public const int None = -1;

    /// <summary>방향 수.</summary>
    public const int Count = 8;

    /// <summary>한 걸음의 부호(dx, dy: -1·0·1)를 방향 번호로 바꾼다. (0, 0)이면 <see cref="None"/>.</summary>
    /// <param name="dx">가로 걸음 부호 (양수 = 오른쪽·동)</param>
    /// <param name="dy">세로 걸음 부호 (양수 = 아래·남)</param>
    public static int FromStep(int dx, int dy)
    {
        dx = Math.Sign(dx);
        dy = Math.Sign(dy);
        // 위쪽(북)부터 시계 방향으로 나열한 3×3 표 (행 = dy + 1, 열 = dx + 1)
        return (dx, dy) switch
        {
            (0, -1) => 0, (1, -1) => 1, (1, 0) => 2, (1, 1) => 3,
            (0, 1) => 4, (-1, 1) => 5, (-1, 0) => 6, (-1, -1) => 7,
            _ => None,
        };
    }

    /// <summary>방향 번호에 해당하는 클러스터 측면 글자 (A~H). 방향이 없으면 null.</summary>
    /// <param name="heading">방향 번호</param>
    public static char? Side(int heading) => heading is >= 0 and < Count ? (char)('A' + heading) : null;
}
