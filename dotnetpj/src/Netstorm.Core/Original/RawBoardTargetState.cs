// 운반 명령의 실패점을 같은 장기 수명에 두고 Board·AI가 직접 빌린다.
namespace Netstorm.Core.Original;

// 보드 목표 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.

// 월드 초기화 004af170이 기록한 실패점이며 진단 뒤 현재 값을 다시 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class BoardTargetState
{
    /// <summary>545370/545374의 실제 초기화 뒤 두 float다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public RawPathPoint Invalid { get; set; } = new() { X = -10000.0f, Y = -10000.0f };
}
