// 실제 포구 SHP 추가 헤더와 TYPE 자료의 현재 값을 보관한다.
namespace Netstorm.Core.Original;

/// <summary>RawProjectileGeometry.h의 실제 물리 프레임·포구·사정거리 자료다.</summary>
public sealed class ProjectileGeometryType
{
    /// <summary>가상78의 실제 기본 프레임이다.</summary>
    public int DefaultFrame { get; set; }
    /// <summary>TYPE+114의 실제 전체 물리 프레임 수다.</summary>
    public int FrameCount { get; set; }
    /// <summary>TYPE+110의 균등 투사체 방향 수다.</summary>
    public int Directions { get; set; }
    /// <summary>SHP 물리 헤더+20/+22의 signed WORD 쌍이다.</summary>
    public (short X, short Y)[] ShotOffsets { get; set; } = [];
    /// <summary>충돌 상자에서만 더하는 실제 높이 확장이다.</summary>
    public int HeightExtension { get; set; }
    /// <summary>방향 불일치 시 정수로 절삭하는 TYPE+44 사정거리다.</summary>
    public float Range { get; set; }
}
