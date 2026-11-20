// 운반 명령의 실패점을 같은 장기 수명에 두고 Board·AI가 직접 빌린다.
namespace Netstorm.Core.Original;

// 보드 목표 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.

// 월드 초기화 004af170이 기록한 실패점이며 진단 뒤 현재 값을 다시 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class BoardTargetState
{
    /// <summary>545370/545374의 실제 초기화 뒤 두 float다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public RawPathPoint Invalid { get => _invalid; set => _invalid = value; }
    /// <summary>부모가 Y→X 순서로 직접 쓰는 같은 실제 실패점 물리다.</summary>
    private RawPathPoint _invalid = new() { X = -10000.0f, Y = -10000.0f };
    /// <summary>Pool 정리 부모가 사본 없이 빌리는 같은 두 float의 참조다.</summary>
    public ref RawPathPoint InvalidStorage => ref _invalid;
}
