// 소유 버퍼 반납과 원문 표시 등록/base 소멸을 같은 순서로 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문 free가 구별하는 소유 버퍼와 scalar deleting destructor 객체다.</summary>
public enum ConstructionRelease
{
    /// <summary>현재 SHP 캡처 픽셀이다.</summary>
    Source,
    /// <summary>표시 출력 버퍼다.</summary>
    Output,
    /// <summary>건설 입자 배열이다.</summary>
    Particles,
    /// <summary>flags bit0이 요청한 객체 자체다.</summary>
    Object,
}

/// <summary>RawConstructionLifetime의 파생 소멸/생성 등록 어댑터다.</summary>
public static class RawConstructionLifetime
{
    /// <summary>캡처→출력→입자→등록/base→선택 객체 순서로 소유권을 반납한다. release는 예외를 던지지 않아야 한다.</summary>
    public static void Destroy(ConstructionVisualFields visual, RenderProcessFields registration, RawRenderRegistry registry, uint flags, Action<ConstructionRelease> release)
    {
        ArgumentNullException.ThrowIfNull(visual); ArgumentNullException.ThrowIfNull(registration); ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(release);
        if (visual.Image.Pixels.Length != 0) { release(ConstructionRelease.Source); visual.Image.Pixels = []; }
        if (visual.Output.Length != 0) { release(ConstructionRelease.Output); visual.Output = []; }
        if (visual.Particles.Length != 0) { release(ConstructionRelease.Particles); visual.Particles = []; }
        registry.Destroy(registration);
        if ((flags & 1) != 0) release(ConstructionRelease.Object);
    }

    /// <summary>입력 함수 사본의 부착만 원문 목록으로 바꾸고 실제 부착 뒤 부모/영역/깊이를 돌려준다.</summary>
    public static ConstructionVisualHooks MakeRegistryHooks(RawRenderRegistry registry, RenderProcessFields registration, uint token, ConstructionVisualHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(registration); ArgumentNullException.ThrowIfNull(hooks);
        if (token == 0) throw new ArgumentException("건설 표시 등록 토큰0");
        ConstructionVisualHooks result = hooks.Copy();
        result.Attach = (visual, type) =>
        {
            registry.Construct(registration, token, visual.Parent, type);
            if (registration.Parent < 0 || registration.Parent > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(registration), "건설 표시 부착 부모 범위 오류");
            visual.Parent = registration.Parent; visual.Left = registration.Left; visual.Top = registration.Top;
            visual.Right = registration.Right; visual.Bottom = registration.Bottom; visual.ZOrder = registration.ZOrder;
        };
        return result;
    }
}
