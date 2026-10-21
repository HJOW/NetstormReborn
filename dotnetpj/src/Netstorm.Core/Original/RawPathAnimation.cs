using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 경로 애니메이션의 방향 판정(원본 RawPathAnimation::Direction).
/// 프레임 코드의 방향 문자를 'A' 기준 방향 번호로 바꾼다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPathAnimation.cpp (W03 삭제·수명 아홉째 조각, 2026-10-21).
/// </summary>
public static class RawPathAnimation
{
    /// <summary>
    /// 프레임 코드의 방향 문자를 읽는다(원본 RawPathAnimation::Side).
    /// </summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">프레임 인덱스.</param>
    /// <returns>방향 문자 코드.</returns>
    public static int Side(IReadOnlyList<FrameCode> frames, int frame)
    {
        if (frame < 0 || frame >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), frame, "경로 방향 프레임 범위 오류");
        }
        // 원본은 방향 문자를 signed 8비트로 읽는다.
        return (sbyte)frames[frame].Side;
    }

    /// <summary>
    /// 현재 프레임의 방향 번호를 'A' 기준으로 돌려준다(원본 RawPathAnimation::Direction).
    /// </summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">프레임 인덱스.</param>
    /// <returns>'A' 가 0 인 방향 번호.</returns>
    public static int Direction(IReadOnlyList<FrameCode> frames, int frame) => Side(frames, frame) - 'A';
}
