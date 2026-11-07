// 다리 프레임의 금간 전환을 실제 타입 표에서 찾는다.
namespace Netstorm.Core.Original;

// 금간 프레임 판정 모듈을 담는 네임스페이스다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.

// 원본 Bridge::CrackedFrame 계열을 실제 프레임 코드 표에서 복원한다. 금 간 프레임은 보통 번호에 10을 더한 번호, 단단한 프레임은 20 이상이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.
public static class RawBridgeFrames
{
    /// <summary>다리 프레임 검색이 쓰는 변형 글자(원문 push 0x50).</summary>
    private const byte BridgeVariant = (byte)'P';

    /// <summary>금 간 프레임의 번호 오프셋이다.</summary>
    private const int CrackedNumberOffset = 10;

    /// <summary>단단한 프레임의 최소 번호다.</summary>
    private const int HardNumber = 20;

    /// <summary>금 간 프레임의 플래그 비트(원문 kBridgeCrackedFrame).</summary>
    private const uint CrackedFrameFlag = 0x20u;

    /// <summary>현재 프레임의 8비트 번호를 부호 확장해 읽는다.</summary>
    /// <param name="frames">타입별 프레임 표다.</param>
    /// <param name="frame">읽을 프레임 번호다.</param>
    /// <returns>부호 확장한 번호다.</returns>
    public static int FrameNumber(RiftTypeFrames frames, int frame)
    {
        ArgumentNullException.ThrowIfNull(frames);
        IReadOnlyList<RiftFrameCode> codes = frames.Codes;
        if (frame < 0 || frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), frame, "다리 프레임 번호 오류");
        }

        return (sbyte)codes[frame].Number;
    }

    /// <summary>현재 프레임의 방향 글자를 읽는다.</summary>
    /// <param name="frames">타입별 프레임 표다.</param>
    /// <param name="frame">읽을 프레임 번호다.</param>
    /// <returns>방향 글자다.</returns>
    public static byte FrameSide(RiftTypeFrames frames, int frame)
    {
        ArgumentNullException.ThrowIfNull(frames);
        return frames.Codes[frame].Side;
    }

    /// <summary>같은 방향·변형 P·번호+10·금 간 플래그의 첫 프레임을 찾는다. 없거나 이미 금 간·단단하면 -1이다.</summary>
    /// <param name="frames">타입별 프레임 표다.</param>
    /// <param name="frame">현재 프레임 번호다.</param>
    /// <param name="force">단단한 프레임을 번호 1로 보고 찾으면 참이다.</param>
    /// <returns>금 간 프레임 번호다. 없으면 -1이다.</returns>
    public static int CrackedFrame(RiftTypeFrames frames, int frame, bool force)
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

        return frames.FindMasked(FrameSide(frames, frame), BridgeVariant, unchecked((byte)(number + CrackedNumberOffset)), CrackedFrameFlag);
    }
}
