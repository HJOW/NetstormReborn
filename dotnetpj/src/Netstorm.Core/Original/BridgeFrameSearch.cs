using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 다리 프레임의 금간·단단한 코드를 찾는 원본 검색(원본 Bridge::CrackedFrame·NormalFrame).
/// 금간 번호는 10 이상, 단단한 번호는 20 이상이며, 금간 코드는 0x20 플래그를 갖는다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/Bridge.cpp (W03 삭제·수명 일곱째 조각, 2026-10-21).
/// </summary>
public static class BridgeFrameSearch
{
    /// <summary>다리 클러스터의 둘째 글자(원본 kBridgeVariant).</summary>
    private const char BridgeVariant = 'P';

    /// <summary>금간 번호의 시작 오프셋(원본 kCrackedNumberOffset).</summary>
    private const int CrackedNumberOffset = 10;

    /// <summary>단단한 번호의 경계(원본 kHardNumber).</summary>
    private const int HardNumber = 20;

    /// <summary>금간 프레임의 플래그 비트(원본 kBridgeCrackedFrame).</summary>
    private const uint CrackedFrameFlag = 0x20;

    /// <summary>단단한 프레임의 플래그 비트(원본 kBridgeHardFrame).</summary>
    private const uint HardFrameFlag = 0x40;

    /// <summary>프레임 번호의 부호 있는 8비트 값(원본 FrameNumber).</summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">프레임 인덱스.</param>
    /// <returns>프레임 코드에 적힌 번호.</returns>
    public static int FrameNumber(IReadOnlyList<FrameCode> frames, int frame)
    {
        if (frame < 0 || frame >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), frame, "다리 프레임 번호 범위 오류");
        }
        // 원본은 번호를 signed 8비트로 읽는다.
        return (sbyte)frames[frame].Number;
    }

    /// <summary>프레임 코드의 방향 문자(원본 FrameSide).</summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">프레임 인덱스.</param>
    /// <returns>방향 문자.</returns>
    public static char FrameSide(IReadOnlyList<FrameCode> frames, int frame)
    {
        if (frame < 0 || frame >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), frame, "다리 프레임 번호 범위 오류");
        }
        return frames[frame].Side;
    }

    /// <summary>
    /// 금간·단단한 다리 프레임을 찾는다. 강제이면 단단한 번호를 1 로 낮추고,
    /// 금간 번호 이상이면 찾지 못한다(-1).
    /// </summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">현재 프레임 인덱스.</param>
    /// <param name="force">단단한 번호를 낮추는지.</param>
    /// <returns>찾은 프레임 인덱스. 없으면 -1.</returns>
    public static int CrackedFrame(IReadOnlyList<FrameCode> frames, int frame, bool force)
    {
        int number = FrameNumber(frames, frame);
        if (force && number >= HardNumber)
        {
            number = 1;
        }
        else if (number >= CrackedNumberOffset)
        {
            return -1;
        }
        return FindMasked(frames, FrameSide(frames, frame), BridgeVariant, (byte)(number + CrackedNumberOffset), CrackedFrameFlag);
    }

    /// <summary>
    /// 정상 다리 프레임을 찾는다. 단단한 번호는 1 로 낮추고 금간 번호는 10 을 뺀다.
    /// </summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">현재 프레임 인덱스.</param>
    /// <returns>찾은 프레임 인덱스. 없으면 -1.</returns>
    public static int NormalFrame(IReadOnlyList<FrameCode> frames, int frame)
    {
        int number = FrameNumber(frames, frame);
        if (number >= HardNumber)
        {
            number = 1;
        }
        else if (number >= CrackedNumberOffset)
        {
            number -= CrackedNumberOffset;
        }
        return FindNumber(frames, FrameSide(frames, frame), BridgeVariant, (byte)number);
    }

    /// <summary>방향·변형·번호가 일치하고 플래그 마스크가 겹치는 첫 프레임을 찾는다(원본 FindMasked).</summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="side">방향 문자.</param>
    /// <param name="variant">변형 문자.</param>
    /// <param name="number">프레임 번호.</param>
    /// <param name="mask">플래그 마스크.</param>
    /// <returns>찾은 프레임 인덱스. 없으면 -1.</returns>
    public static int FindMasked(IReadOnlyList<FrameCode> frames, char side, char variant, byte number, uint mask)
    {
        // 앞쪽부터 첫 일치를 돌려준다.
        for (int i = 0; i < frames.Count; i++)
        {
            FrameCode code = frames[i];
            if (code.Side == side && code.Variant == variant && (byte)code.Number == number
                && ((uint)(sbyte)(byte)code.Flags & mask) != 0)
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>방향·변형·번호가 일치하는 첫 프레임을 찾는다(원본 FindNumber).</summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="side">방향 문자.</param>
    /// <param name="variant">변형 문자.</param>
    /// <param name="number">프레임 번호.</param>
    /// <returns>찾은 프레임 인덱스. 없으면 -1.</returns>
    public static int FindNumber(IReadOnlyList<FrameCode> frames, char side, char variant, byte number)
    {
        // 앞쪽부터 첫 일치를 돌려준다.
        for (int i = 0; i < frames.Count; i++)
        {
            FrameCode code = frames[i];
            if (code.Side == side && code.Variant == variant && (byte)code.Number == number)
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>단단한 프레임 플래그를 확인한다.</summary>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="frame">프레임 인덱스.</param>
    /// <returns>단단한 비트(0x40)가 켜져 있는지.</returns>
    public static bool IsHard(IReadOnlyList<FrameCode> frames, int frame) =>
        (frames[frame].Flags & (FrameCodeFlags)HardFrameFlag) != 0;
}
