namespace Netstorm.Client.Display;

/// <summary>
/// 원본 색 표의 한 항목. 같은 RGB를 서로 다른 전역에 적는 호출도 원본 순서대로 별도 항목으로 남긴다.
/// 근거: cpppj/src/client/PaletteColors.h 의 NamedPaletteColor. (R2a, 2026-10-28)
/// </summary>
/// <param name="Red">빨강 성분(0~255).</param>
/// <param name="Green">초록 성분(0~255).</param>
/// <param name="Blue">파랑 성분(0~255).</param>
/// <param name="Index">목록이나 표에서 현재 항목을 찾는 인덱스.</param>
public readonly record struct NamedPaletteColor(byte Red, byte Green, byte Blue, uint Index);

/// <summary>
/// 기본 9색과 날씨 4색의 별칭, 전체 호출 순서의 색 표. 파일 팔레트를 읽을 때만 다시 계산하며 일시 팔레트 효과에는 갱신하지 않는다.
/// 근거: cpppj/src/client/PaletteColors.h 의 PaletteColorTable. (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteColorTable
{
    /// <summary>기본 9색(검정/빨강/초록/파랑/자홍/노랑/주황/흰색/회색)의 팔레트 번호.</summary>
    public uint[] Basic { get; } = new uint[9];

    /// <summary>날씨 4색(바람·비·천둥·해)의 팔레트 번호. 기본 노랑·파랑·빨강과 해 갈색 항목을 그대로 복사한다.</summary>
    public uint[] Weather { get; } = new uint[4];

    /// <summary>원본 호출 순서의 이름있는 색 목록.</summary>
    public List<NamedPaletteColor> Named { get; } = [];
}

/// <summary>
/// 원본 Screen.cpp 팔레트 로더가 채우는 기본·표시·날씨 색 번호 표. 실제 논리 팔레트의 검색 함수로 계산한다.
/// 근거: cpppj/src/client/PaletteColors.cpp 의 BuildPaletteColorTable (원본 004a4850). (R2a, 2026-10-28)
/// </summary>
public static class PaletteColors
{
    /// <summary>10.78의 색 검색 호출 수(중복 RGB 포함). CD판은 12~19번 여덟 색을 제외한 50개다.</summary>
    public const int PatchColorCount = 58;

    /// <summary>CD판의 색 검색 호출 수.</summary>
    public const int CdColorCount = 50;

    /// <summary>원본 004a4850의 58개 검색 인자 순서(R,G,B). CD판에 없는 12~19번 추가 색을 포함한다.</summary>
    private static readonly byte[][] NamedRgb =
    [
        [0, 0, 0], [255, 22, 22], [22, 255, 22], [22, 22, 255], [255, 22, 255], [255, 255, 22],
        [240, 61, 0], [255, 255, 255], [128, 128, 128], [181, 140, 111], [95, 188, 92], [151, 215, 210],
        [0, 255, 102], [255, 102, 204], [255, 153, 0], [51, 255, 102], [255, 0, 153], [99, 66, 255],
        [0, 128, 0], [184, 147, 20], [233, 214, 186], [200, 175, 144], [249, 233, 145], [219, 213, 203],
        [215, 207, 119], [45, 62, 32], [17, 66, 60], [17, 66, 60], [108, 57, 54], [74, 58, 56],
        [114, 106, 103], [250, 248, 238], [84, 80, 81], [206, 193, 178], [255, 255, 255], [128, 128, 128],
        [47, 24, 17], [55, 51, 54], [88, 121, 67], [111, 146, 48], [153, 179, 55], [216, 205, 57],
        [54, 100, 130], [191, 75, 71], [204, 193, 180], [91, 129, 50], [127, 113, 182], [226, 185, 82],
        [53, 122, 112], [238, 126, 40], [146, 168, 186], [223, 133, 116], [249, 247, 239], [118, 173, 105],
        [178, 172, 215], [223, 206, 108], [129, 192, 185], [238, 144, 41],
    ];

    /// <summary>
    /// 실제 논리 팔레트의 검색 함수로 원본 전체 색 표를 계산한다. 기본색·날씨 별칭은 새 검색 없이 계산된 번호를 복사한다.
    /// 일시 팔레트 효과(SetPalette)에는 호출하지 않으며 파일 로더에서만 호출한다.
    /// </summary>
    /// <param name="logical">R/G/B/플래그 순서의 논리 팔레트 DWORD 256개.</param>
    /// <param name="patch1078">10.78이면 참이다. 거짓이면 CD판처럼 12~19번 추가 색을 건너뛴다.</param>
    public static PaletteColorTable BuildPaletteColorTable(ReadOnlySpan<uint> logical, bool patch1078 = true)
    {
        if (logical.Length != 256)
        {
            throw new ArgumentException("논리 팔레트는 256색이어야 합니다", nameof(logical));
        }
        var result = new PaletteColorTable();
        // CD판에 없는 12~19번 추가 색만 건너뛰고 원본 호출 순서를 유지한다
        for (int i = 0; i < NamedRgb.Length; i++)
        {
            if (!patch1078 && i >= 12 && i < 20)
            {
                continue;
            }
            byte[] rgb = NamedRgb[i];
            uint index = PaletteSearch.FindPaletteColor(logical, rgb[0], rgb[1], rgb[2]);
            result.Named.Add(new NamedPaletteColor(rgb[0], rgb[1], rgb[2], index));
            // 앞 9개 호출이 기본 9색이다(CD판도 건너뛰기 전이므로 같은 순서다)
            if (i < result.Basic.Length)
            {
                result.Basic[i] = index;
            }
        }
        result.Weather[0] = result.Basic[5];
        result.Weather[1] = result.Basic[3];
        result.Weather[2] = result.Basic[1];
        result.Weather[3] = result.Named[9].Index;
        return result;
    }
}
