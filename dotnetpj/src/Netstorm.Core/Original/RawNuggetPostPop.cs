// 너겟 최초 두 Regular의 조회·독립 확보·난수 소비·생성·Damageable 순서다.
namespace Netstorm.Core.Original;

/// <summary>원문484f90이 도달하는 실제 조회/확보/게임 난수/생성/부모 장치다.</summary>
public sealed class NuggetPostPopHooks
{
    /// <summary>현재 parent의 사건1 예약을 실제 Regular 목록에서 조회한다.</summary>
    public Func<int, uint, bool>? HasRegular { get; set; }
    /// <summary>각각의 new0x28을 독립 수행하여 같은 확보 메모리를 생성자에 넘긴다.</summary>
    public Func<bool>? Allocate { get; set; }
    /// <summary>게임 난수0~0.3/0.5의 실제 float 저장 결과다.</summary>
    public Func<float, float, float>? Random { get; set; }
    /// <summary>성공한 같은 확보에 사건/부모/지연의 Regular를 생성·부착한다.</summary>
    public Action<int, uint, float>? Construct { get; set; }
    /// <summary>마지막에 항상 직접 호출하는 실제 전체 Damageable 부모다.</summary>
    public Action<int, uint>? Damageable { get; set; }
    /// <summary>CPP 함수 묶음의 값 복사이며 실제 캡처 owner나 장치는 실행하지 않는다.</summary>
    internal NuggetPostPopHooks Copy() => (NuggetPostPopHooks)MemberwiseClone();
}

/// <summary>미리보기와 최초 실체에서 같은 실제 너겟 부모 순서를 유지한다.</summary>
public sealed class RawNuggetPostPop
{
    /// <summary>너겟과 실제 Regular Form이 공유하는 실제 PATCH 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>외부 묶음과 독립인 원문 함수 값 사본이다.</summary>
    private readonly NuggetPostPopHooks _hooks;
    /// <summary>같은10.78 풀/장치를 빌리며 참조 대상은 처리기보다 오래 살아야 한다.</summary>
    public RawNuggetPostPop(RawSidPool pool, NuggetPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("너겟 후처리는10.78 몸체입니다", nameof(pool));
        _pool = pool; _hooks = hooks.Copy();
    }
    /// <summary>다른 월드의 예약 메모리를 섞지 않도록 같은 실제 풀을 반환한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>선택한 실제 하위만 요구하며 빈 장치를 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("너겟 실제 후처리 하위 몸체 미연결");
    /// <summary>flags1에서만 두 독립 예약을 하고 항상 실제 부모를 호출한다.</summary>
    public void PostPop(int sid, uint flags)
    {
        if ((flags & 1) != 0)
        {
            if (!Need(_hooks.HasRegular)(sid, 1) && Need(_hooks.Allocate)())
            {
                float payload = Need(_hooks.Random)(0, 0.3f); Need(_hooks.Construct)(sid, 1, payload);
            }
            if (Need(_hooks.Allocate)())
            {
                float payload = Need(_hooks.Random)(0, 0.5f); Need(_hooks.Construct)(sid, 3, payload);
            }
        }
        Need(_hooks.Damageable)(sid, flags);
    }
}
