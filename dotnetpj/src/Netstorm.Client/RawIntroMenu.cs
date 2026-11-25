using Netstorm.Core.Original;

namespace Netstorm.Client;

// 원본 Intro 존재 감지를 실제 Native 영상 상태에 연결한다.
public static class RawIntroMenu
{
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
}
