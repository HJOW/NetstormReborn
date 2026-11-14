// 생성 부모의 같은 현재 fakeSurface DWORD를 월드 교체와 일반 준비 사이에도 보존한다.
namespace Netstorm.Core.Original;

/// <summary>현재 공장에 장기 소유자의 생성 진단 TYPE 주소를 빌린다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>PATCH005412e0의 최초162이며 일반 월드 준비로 덮어쓰지 않는 현재 DWORD다.</summary>
    public UIntBox FakeSurfaceTypeBox { get; } = new() { Value = 162u };
}
