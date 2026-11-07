// 저장/지형 어댑터에서 넘기는 초기 위치와 표면 객체 스냅샷·표시 콜백을 공유한다.
using Netstorm.Core.Display;

namespace Netstorm.Core.Original;

// 서피스 시드 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.

// 저장/지형 어댑터에서 넘기는 초기 위치다. 등록 뒤 현재 값은 raw 슬롯만 읽는다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.
public readonly record struct SurfaceSeed(
    /// <summary>타입 표에서 객체 종류를 찾는 번호다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    uint Type,
    /// <summary>객체나 지역을 소유한 플레이어 번호다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    uint Owner,
    /// <summary>타입에서 사용할 그림 프레임 번호다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    int Frame,
    /// <summary>X 좌표다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    float X,
    /// <summary>Y 좌표다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    float Y);

// 표면 객체의 현재 SID와 직렬화할 배치 정보다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.
public readonly record struct SurfaceSnapshot(
    /// <summary>원본 풀에서 객체를 찾는 SID 번호다. C++ Sid는 int 슬롯 번호로 옮긴다(기존 규칙). 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    int Sid,
    /// <summary>해당 SID나 출력 항목에 대응하는 객체 자료다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    SurfaceSeed Object);

// 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.
public sealed class SurfaceDisplayHooks
{
    /// <summary>현재 표시나 후처리를 억제하는 전역 상태를 조회하는 콜백이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    public Func<bool>? Suppressed { get; set; }

    /// <summary>변경한 영역의 재그리기를 요청하는 콜백이다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    public Action<DisplayRect, uint>? Invalidate { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 서피스 월드 자료형 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public SurfaceDisplayHooks Clone() => new()
    {
        Suppressed = Suppressed,
        Invalidate = Invalidate,
    };
}
