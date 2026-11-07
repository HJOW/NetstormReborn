// 출력 요청의 화면 영역·진단 장치와 프레임 오류 상세를 공유한다.
using Netstorm.Core.Display;

namespace Netstorm.Core.Original;

// 표시 진단 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.

// Squid 출력 좌표를 투영할 화면 영역과 카메라다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.
public sealed class SquidDisplayView
{
    /// <summary>원본 카메라 픽셀이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    public int CameraX { get; set; }

    /// <summary>원본 카메라 픽셀이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    public int CameraY { get; set; }

    /// <summary>Q16 배율이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    public int Zoom { get; set; } = 65536;

    /// <summary>월드 그림을 투영하고 자를 화면 사각형이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    public DisplayRect Viewport { get; set; } = new(0, 0, 0, 0);
}

// Squid 출력 요청을 실제 렌더러에 연결하는 인터페이스다. Renderer 또는 독립 변경 표의 수명을 호출자가 관리한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.
public abstract class SquidDisplaySink
{
    /// <summary>원본 전체 갱신·표시 억제 전역에 대응한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <returns>억제 중이면 참이다.</returns>
    public abstract bool Suppressed();

    /// <summary>원본 순서대로 main·shadow 영역과 플래그를 전달한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <param name="rect">변경 영역이다.</param>
    /// <param name="flags">전달 플래그다.</param>
    public abstract void Invalidate(DisplayRect rect, uint flags);

    /// <summary>독립 기존 장치는 검사 0이며 실제 월드는 같은 저장소의 현재 뷰를 반환한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <returns>현재 검사 뷰다.</returns>
    public virtual RawCheckingFlag Checking() => new();

    /// <summary>선택된 프레임 오류만 실제 저장소를 요구한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <param name="abstract">추상 플래그다.</param>
    public virtual void FrameError(uint type, int frame, uint abstractValue)
    {
        throw new InvalidOperationException("표시 오류 상세 writer 미연결");
    }

    /// <summary>진단 장치가 복귀하면 표시 부모가 현재 TYPE을 다시 조회한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <param name="expression">원문 식이다.</param>
    /// <param name="file">원문 파일이다.</param>
    /// <param name="line">원문 행이다.</param>
    public virtual void Assertion(string expression, string file, uint line)
    {
        throw new InvalidOperationException("표시 원문 진단 장치 미연결");
    }
}

// 같은 TYPE 이름·프레임 개수와 세 인자로 오류 상세를 만든다. 임의 포인터·힙은 공유하지 않는다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.
public static class SquidDisplayDetail
{
    /// <summary>같은 TYPE 이름·프레임 개수와 세 인자로 오류 상세를 만든다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    /// <param name="types">타입 표다.</param>
    /// <param name="shapes">셰이프 표다.</param>
    /// <param name="type">타입 번호다.</param>
    /// <param name="frame">프레임 번호다.</param>
    /// <param name="abstractValue">추상 플래그다.</param>
    /// <returns>호스트 소유 문자열이다.</returns>
    public static string BuildFrameAssertionDetail(IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SquidDisplayShape> shapes, uint type, int frame, uint abstractValue)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        if (types.Count != shapes.Count)
        {
            throw new ArgumentException("표시 오류 상세 TYPE·셰이프 개수 불일치");
        }

        // 원문은 부호 있는 TYPE 범위를 먼저 판정한다
        int signedType = unchecked((int)type);
        bool valid = signedType >= 10 && type < (uint)types.Count;
        string name = valid ? types[(int)type].Name : "invalidType";
        int frames = valid ? shapes[(int)type].FrameCount : 0;
        return "Frame assertion failed. Asking for type=" + signedType.ToString(System.Globalization.CultureInfo.InvariantCulture)
            + " (" + name + ") frame=" + frame.ToString(System.Globalization.CultureInfo.InvariantCulture)
            + " abs=" + unchecked((int)abstractValue).ToString(System.Globalization.CultureInfo.InvariantCulture)
            + "\n\nType has " + frames.ToString(System.Globalization.CultureInfo.InvariantCulture) + " frames\n";
    }
}
