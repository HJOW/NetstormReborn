// Factory4549c0의 현재 레벨/프레임/난수와 선택된 큰 업그레이드 효과를 공유한다.
// cpppj/src/o/RawFactoryRegular.h·cpp 의 RunFactoryRegular 본체를 이식한다.
// 2026-11-19 0.628에서 R4-2 공장 프레임·정기 집계 후속용으로 추가했다.
namespace Netstorm.Core.Original;

// 공장 정기 처리 모듈을 담는 네임스페이스다. 2026-11-19 0.628에서 추가했다.

/// <summary>
/// 조회와 실제 가상 효과만 경계로 두고 사건/호출 순서는 본문이 결정한다.
/// 근거: cpppj <c>o/RawFactoryRegular.h</c>의 <c>FactoryRegularHooks</c>.
/// </summary>
/// <remarks>2026-11-19 0.628에서 R4-2 공장 프레임·정기 집계 후속용으로 추가했다.</remarks>
public sealed class FactoryRegularHooks
{
    /// <summary>현재 가상80이며 두 조회를 합치지 않는다. 2026-11-19 0.628에서 추가했다.</summary>
    public Func<int, int>? Level { get; set; }

    /// <summary>레벨 조회 뒤의 현재 TYPE+130이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Func<int, int, int>? FirstFrame { get; set; }

    /// <summary>레벨 조회 뒤의 현재 TYPE+170이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Func<int, int, int>? FrameCount { get; set; }

    /// <summary>같은 현재00532710이며 0범위도 원문 선행 seed 쓰기를 보존한다. 2026-11-19 0.628에서 추가했다.</summary>
    public Func<uint, uint>? Random { get; set; }

    /// <summary>실제4acee0의 Frame.Set이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Action<int, int, uint>? Frame { get; set; }

    /// <summary>실제4afc90의 한 번 감싸기 프레임 진행이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Action<int, int, uint>? Advance { get; set; }

    /// <summary>count0의 실제 전체 예측 그림/건설/감시쌍 생성이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Action<int, float>? Begin { get; set; }

    /// <summary>비0 count의 실제 완료·현재AI 통지·-1 반환이다. 2026-11-19 0.628에서 추가했다.</summary>
    public Func<int, float>? Finish { get; set; }

    /// <summary>현재 레벨이 2보다 큰 사건5의 실제2/1 플레어다. 2026-11-19 0.628에서 추가했다.</summary>
    public Action<int, int, uint>? Flare { get; set; }

    /// <summary>선택된 조기 반환 이외의 실제44b900이다. 2026-11-19 0.628에서 추가했다.</summary>
    public RegularHandler? Damageable { get; set; }
}

/// <summary>
/// 두 직접 VT와 원문의 비가상 기본 호출이 같은 본문을 사용한다.
/// 근거: cpppj <c>o/RawFactoryRegular.h/.cpp</c>의 <c>RunFactoryRegular</c>, 원본 004549c0.
/// </summary>
/// <remarks>2026-11-19 0.628에서 R4-2 공장 프레임·정기 집계 후속용으로 추가했다.</remarks>
public static class RawFactoryRegular
{
    /// <summary>현재 조회 순서와 DWORD 프레임 덧셈 감김을 보존한다. 2026-11-19 0.628에서 추가했다.</summary>
    /// <param name="pool">판본만 읽는 슬롯 풀이다. 슬롯 바이트는 건드리지 않는다.</param>
    /// <param name="parent">처리할 부모 슬롯 번호다.</param>
    /// <param name="event">사건 번호다.</param>
    /// <param name="count">개수다.</param>
    /// <param name="payload">전달 값이다.</param>
    /// <param name="hooks">선택된 실제 효과 묶음이다.</param>
    /// <returns>원문과 같은 반환 값이다.</returns>
    /// <exception cref="ArgumentException">10.78이 아닌 판본이다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 장치가 미연결이다.</exception>
    public static float Run(RawSidPool pool, int parent, uint @event, uint count, float payload, FactoryRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("Factory4549c0은 PATCH10.78 본문이다", nameof(pool));
        }

        if (@event == 2)
        {
            int firstLevel = FactoryNeed(hooks.Level)(parent);
            int first = FactoryNeed(hooks.FirstFrame)(parent, firstLevel);
            int currentLevel = FactoryNeed(hooks.Level)(parent);
            int size = FactoryNeed(hooks.FrameCount)(parent, currentLevel);
            uint offset = FactoryNeed(hooks.Random)(unchecked((uint)size));
            // 원본 signed DWORD 덧셈 감김을 유지한다.
            FactoryNeed(hooks.Frame)(parent, unchecked((int)unchecked((uint)first + offset)), 0);
        }
        else if (@event == 1)
        {
            FactoryNeed(hooks.Advance)(parent, 1, 0);
        }
        else if (@event == 3)
        {
            if (count != 0)
            {
                return FactoryNeed(hooks.Finish)(parent);
            }

            FactoryNeed(hooks.Begin)(parent, payload);
            FactoryNeed(hooks.Advance)(parent, 1, 0);
        }
        else if (@event == 4 && count != 0)
        {
            return 0.0f;
        }
        else if (@event == 5 && FactoryNeed(hooks.Level)(parent) > 2)
        {
            FactoryNeed(hooks.Flare)(parent, 2, 1);
        }

        return FactoryNeed(hooks.Damageable)(parent, @event, count, payload);
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다. 2026-11-19 0.628에서 추가했다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식)이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T FactoryNeed<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Factory Regular의 선택된 실제 하위 미연결");
        }

        return function;
    }
}
