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
