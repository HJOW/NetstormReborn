namespace Netstorm.Core.Original;

/// <summary>
/// Squid 표시 프레임의 크기·핫스폿·부가 좌표. SHP 프레임 앞 36바이트와 추가 헤더에서 읽는다.
/// 근거: cpppj/src/o/SquidDisplay.h 의 SquidDisplayFrame. (R2d-2, 2026-10-29)
/// 표시 수집·카메라(SquidDisplay 본체)는 R4 세션 조립에서 잇는다.
/// </summary>
public sealed class SquidDisplayFrame
{
    /// <summary>SHP 프레임의 가로 크기(픽셀).</summary>
    public short Width { get; set; }

    /// <summary>SHP 프레임의 세로 크기(픽셀).</summary>
    public short Height { get; set; }

    /// <summary>그림 기준점의 가로 오프셋.</summary>
    public short HotspotX { get; set; }

    /// <summary>그림 기준점의 세로 오프셋.</summary>
    public short HotspotY { get; set; }

    /// <summary>같은 프레임의 셀 가로 크기. 픽셀 크기와 별도로 해시 단계에 쓴다.</summary>
    public float CellWidth { get; set; }

    /// <summary>같은 프레임의 셀 세로 크기.</summary>
    public float CellHeight { get; set; }

    /// <summary>월드 수집에서 폭/높이와 같은 float 추가 헤더의 기준점 가로 좌표.</summary>
    public float CellHotspotX { get; set; }

    /// <summary>월드 수집에서 폭/높이와 같은 float 추가 헤더의 기준점 세로 좌표.</summary>
    public float CellHotspotY { get; set; }

    /// <summary>SHP +0x20의 signed short 포구 가로 좌표. 앞쪽 그림 기준점과 구별해 사정선에 쓴다.</summary>
    public short MuzzleX { get; set; }

    /// <summary>SHP +0x22의 signed short 포구 세로 좌표.</summary>
    public short MuzzleY { get; set; }

    /// <summary>SHP 추가 헤더 +0x10/+0x14의 보행 간격 가로. 표시 수집용 기준점과 구별해 48bb40에 공급한다.</summary>
    public float WalkStepX { get; set; }

    /// <summary>SHP 추가 헤더의 보행 간격 세로.</summary>
    public float WalkStepY { get; set; }
}

/// <summary>
/// Squid 표시용 셰이프와 프레임 치수 목록. 타입 번호와 같은 인덱스로 쓴다.
/// 근거: cpppj/src/o/SquidDisplay.h 의 SquidDisplayShape. (R2d-2, 2026-10-29)
/// 목록은 R4에서 실제 SHP와 타입 표로 채운다(SquidRenderer::Shapes 자리). 표시 수집·카메라 본체는 R4 몫이다.
/// </summary>
public sealed class SquidDisplayShape
{
    /// <summary>타입 +0x114의 클러스터 수. 그림자/abstract 레이어를 포함하지 않는다.</summary>
    public int FrameCount { get; set; }

    /// <summary>원본 타입 +0xdc의 null 여부를 빈 배열과 구별한다. 거짓이면 그림이 없다.</summary>
    public bool Loaded { get; set; }

    /// <summary>타입별 프레임 자료.</summary>
    public List<SquidDisplayFrame> Frames { get; } = [];
}
