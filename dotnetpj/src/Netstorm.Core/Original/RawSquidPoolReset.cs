namespace Netstorm.Core.Original;

/// <summary>
/// 경로에 저장되는 두 float 좌표다. 런타임의 double 좌표와 섞지 않는다.
/// 원본 <c>o/RawPathRuntime.h</c>의 <c>RawPathPoint</c>이며, 경로 런타임 본체를 옮길 때 그 파일로 옮긴다.
/// 근거: cpppj/src/o/RawPathRuntime.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public struct RawPathPoint
{
    /// <summary>X 좌표.</summary>
    public float X;

    /// <summary>Y 좌표.</summary>
    public float Y;
}

/// <summary>
/// 월드 SID 풀 정리 부모(원본 004af170/CD 004aaa70)를 기존 실제 저장소와 실패점에 연결한다.
/// 하위 SID Reset과 부모의 추가 기록·좌표 효과를 구별한다. 실패점은 손실 위치·미사일 이전점 등 다른 -10000 필드와 별개다.
/// 근거: cpppj/src/o/RawSquidPoolReset.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public static class RawSquidPoolReset
{
    /// <summary>실패 좌표 값(-10000)의 비트. Native는 Y 다음 X의 두 DWORD만 쓴다.</summary>
    private const uint InvalidCoordinateBits = 0xc61c4000u;

    /// <summary>
    /// 이미 하위 Reset이 끝난 시작 생성자가 재실행 없이 같은 부모 꼬리만 잇는다. 기록 80DWORD→실패 Y→X다.
    /// </summary>
    /// <param name="pool">정리할 SID 풀. 호출 전체 동안 같은 실제 저장소여야 한다.</param>
    /// <param name="invalid">실패점으로 쓸 경로 점. 호출 전체 동안 같은 실제 저장소여야 한다.</param>
    public static void Finish(RawSidPool pool, ref RawPathPoint invalid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        pool.ClearDeletionHistory();
        invalid.Y = BitConverter.UInt32BitsToSingle(InvalidCoordinateBits);
        invalid.X = BitConverter.UInt32BitsToSingle(InvalidCoordinateBits);
    }

    /// <summary>
    /// 실제 하위 Reset(capacity) 정상 반환 뒤에만 같은 기록·실패점을 정리한다. 실패하면 기존 기록·실패점의 뒤 효과를 실행하지 않는다.
    /// 선행 UI/Kernel/Runtime 반납은 호출자 책임이다.
    /// </summary>
    /// <param name="pool">정리할 SID 풀. 호출 전체 동안 같은 실제 저장소여야 한다.</param>
    /// <param name="invalid">실패점으로 쓸 경로 점. 호출 전체 동안 같은 실제 저장소여야 한다.</param>
    /// <param name="capacity">하위 Reset 인자. 0 이하면 기존 풀만 초기화한다.</param>
    public static void Reset(RawSidPool pool, ref RawPathPoint invalid, int capacity = 0)
    {
        ArgumentNullException.ThrowIfNull(pool);
        pool.Reset(capacity);
        Finish(pool, ref invalid);
    }
}
