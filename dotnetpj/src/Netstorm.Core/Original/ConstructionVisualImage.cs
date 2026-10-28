namespace Netstorm.Core.Original;

/// <summary>
/// 현재 SHP 프레임을 캡처한 팔레트 픽셀. 0xff는 투명이며 hotspot은 화면 픽셀 기준이다.
/// 근거: cpppj/src/o/RawConstructionVisual.h 의 ConstructionVisualImage. (R2d-2, 2026-10-29)
/// 건설 표시 생성 본체(RawConstructionVisual)는 R4 세션 조립에서 잇는다.
/// </summary>
public sealed class ConstructionVisualImage
{
    /// <summary>캡처 경계가 공급한 가로 크기.</summary>
    public int Width { get; set; }

    /// <summary>캡처 경계가 공급한 세로 크기.</summary>
    public int Height { get; set; }

    /// <summary>캡처 경계가 공급한 기준점 가로 좌표.</summary>
    public int HotspotX { get; set; }

    /// <summary>캡처 경계가 공급한 기준점 세로 좌표.</summary>
    public int HotspotY { get; set; }

    /// <summary>가로×세로 픽셀. 입자 선택이 원본 색을 0xff로 바꾼다.</summary>
    public byte[] Pixels { get; set; } = [];
}
