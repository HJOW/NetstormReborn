using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client;

// 원본 Intro 존재 감지와 PlayIntro 일반 입력을 실제 Native 영상 상태에 연결한다.
public static class RawIntroMenu
{
    /// <summary>00543248의 PlayIntro→00542a54 초기 명령 DWORD다.</summary>
    public const uint PlayIntroCommand = 0x020000d8u;

    /// <summary>다음 Poll 요청 블록이 수용하는 바이트 수다. next[35] 쓰기에 쓴다.</summary>
    private const int NextBlockSize = 0x90;
    /// <summary>0쓰기→현재 introMovie→비어 있지 않으면 존재 조회→비영이면 1쓰기다.</summary>
    /// <param name="available">현재 Intro 표시값이다.</param>
    /// <param name="path">현재 introMovie 경로를 읽는 장치다.</param>
    /// <param name="probe">파일 존재를 조회하는 장치다. 비영이면 존재한다.</param>
    /// <returns>같은 표시값이다.</returns>
    /// <remarks>2026-11-26 1.052에서 SL-2a intro 감지용으로 추가했다. 근거: cpppj IntroMenu.cpp의 DetectIntroMovie(00435de0)다. 영상 파일이나 창은 열지 않고 존재 장치만 빌린다.</remarks>
    public static uint DetectIntroMovie(UIntBox available, Func<string> path, Func<string, uint> probe)
    {
        ArgumentNullException.ThrowIfNull(available); ArgumentNullException.ThrowIfNull(path); ArgumentNullException.ThrowIfNull(probe);
        available.Value = 0;
        string name = path();
        int nul = name.IndexOf('\0');
        if (nul >= 0) name = name.Substring(0, nul);
        if (name.Length == 0) return available.Value;
        if (probe(name) != 0) available.Value = 1;
        return available.Value;
    }

    /// <summary>일반 입력의 현재 두 인자는 원문처럼 사용하지 않는다. release는 소비만 하고 나머지는 next[0]=39·next[35]=1을 쓴다.</summary>
    /// <param name="input">소비할 입력 사건이다.</param>
    /// <param name="state">같은 실제 로컬 상태 저장소다.</param>
    /// <returns>PlayIntro 명령이면 참이다.</returns>
    /// <remarks>2026-11-26 1.053에서 SL-2b PlayIntro 분배용으로 추가했다. 근거: cpppj IntroMenu.cpp의 DispatchPlayIntroInput이다. 버튼 노출·State39·영상 재생은 R6에서 잇는다.</remarks>
    public static bool DispatchPlayIntroInput(InputEvent input, LocalMissionStateStorage state)
    {
        ArgumentNullException.ThrowIfNull(state);
        if ((input.Code & 0x03ffffffu) != PlayIntroCommand) return false;
        if ((input.Code & InputCode.Release) == 0)
        {
            if (state.Next.Length < NextBlockSize) throw new ArgumentOutOfRangeException(nameof(state), "PlayIntro 다음 블록 0x90 누락");
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span, 39);
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(35 * 4), 1);
        }
        return true;
    }
}
