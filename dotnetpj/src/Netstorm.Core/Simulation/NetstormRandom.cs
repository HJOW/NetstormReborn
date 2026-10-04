namespace Netstorm.Core.Simulation;

/// <summary>
/// 원본 게임 전역 정수 난수 (Netstorm.exe <c>FUN_004558c0</c>·<c>FUN_004558f0</c>, 상태 <c>DAT_00532710</c>).
/// 상태 = 상태 × 0x10003 + 3 (32비트 넘침), 결과 = (상태 &gt;&gt; 16) % 상한. 상태가 0이면 먼저 0x0BAD0BAD 로 바꾼다.
/// 다리 조각 추첨(Combatgump.cpp 0043f…·Cursor.cpp 004468b0)과 지식 보석 추첨(Gem.cpp)이 이 난수를 쓴다.
/// 지형 생성의 <c>004bca20</c> 도 같은 식이다 (FortTerrainPreview).
/// </summary>
public sealed class NetstormRandom
{
    /// <summary>상태가 0일 때 대신 쓰는 시드 (원본 상수 0x0BAD0BAD)</summary>
    public const uint DefaultSeed = 0x0BAD0BADu;

    /// <summary>상태 전이 곱셈 계수</summary>
    private const uint Multiplier = 0x10003u;

    /// <summary>상태 전이 덧셈 계수</summary>
    private const uint Increment = 3u;

    /// <summary>현재 내부 상태 (저장·동기화 검사용)</summary>
    public uint State { get; private set; }

    /// <summary>시드로 만든다. 원본 전역 변수의 초기값은 0 이다</summary>
    /// <param name="seed">시작 상태 (0이면 첫 호출 때 기본 시드로 바뀐다)</param>
    public NetstormRandom(uint seed = 0)
    {
        State = seed;
    }

    /// <summary>0 이상 bound 미만의 값 (원본 FUN_004558c0). bound 가 0 이면 원본처럼 나눗셈이 불가능하므로 예외</summary>
    /// <param name="bound">상한 (1 이상)</param>
    public int Next(int bound)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(bound, 1);
        Advance();
        return (int)((State >> 16) % (uint)bound);
    }

    /// <summary>min 이상 max 미만의 값 (원본 FUN_004558f0: (상태 &gt;&gt; 16) % (max − min) + min)</summary>
    /// <param name="min">하한 (포함)</param>
    /// <param name="max">상한 (제외, min 보다 커야 한다)</param>
    public int Next(int min, int max)
    {
        ArgumentOutOfRangeException.ThrowIfLessThanOrEqual(max, min);
        Advance();
        return (int)((State >> 16) % (uint)(max - min)) + min;
    }

    /// <summary>상태를 한 단계 진행한다 (0 이면 기본 시드부터)</summary>
    private void Advance()
    {
        uint current = State == 0 ? DefaultSeed : State;
        State = unchecked(current * Multiplier + Increment);
    }
}
