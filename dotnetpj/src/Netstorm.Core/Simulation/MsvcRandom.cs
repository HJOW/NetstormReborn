namespace Netstorm.Core.Simulation;

/// <summary>
/// 원본이 쓰는 MSVC CRT <c>rand()</c> 와 같은 선형 합동 난수 (상태 = 상태 × 214013 + 2531011, 결과 = (상태 &gt;&gt; 16) &amp; 0x7FFF).
/// 게임 로직은 System.Random 대신 이것을 써서 플랫폼과 무관하게 같은 순서를 얻는다 (LEFT_JOBS 2절 "결정론").
/// 지형 미리보기(FortTerrainPreview)가 쓰는 계수와 같다.
/// </summary>
public sealed class MsvcRandom
{
    /// <summary>상태 전이 곱셈 계수 (0x343FD)</summary>
    private const uint Multiplier = 0x343FDu;

    /// <summary>상태 전이 덧셈 계수 (0x269EC3)</summary>
    private const uint Increment = 0x269EC3u;

    /// <summary>결과 15비트 마스크 (RAND_MAX = 0x7FFF)</summary>
    public const int MaxValue = 0x7FFF;

    /// <summary>현재 내부 상태 (저장·동기화 검사용)</summary>
    public uint State { get; private set; }

    /// <summary>시드로 만든다 (MSVC srand 와 같다. 기본 시드 1)</summary>
    /// <param name="seed">시드</param>
    public MsvcRandom(uint seed = 1)
    {
        State = seed;
    }

    /// <summary>다음 값 (0 ~ 0x7FFF)</summary>
    public int Next()
    {
        State = unchecked(State * Multiplier + Increment);
        return (int)((State >> 16) & MaxValue);
    }

    /// <summary>0 이상 bound 미만의 값 (rand() % bound 와 같다. 원본 코드의 나머지 연산 관례)</summary>
    /// <param name="bound">상한 (1 이상)</param>
    public int Next(int bound) => bound <= 1 ? 0 : Next() % bound;
}
