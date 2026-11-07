// 초기 지형과 지역 재도장이 사용하는 원본 후보 선택(0049aa90)을 하나의 몸체로 공유한다(cpppj o/RawFrameChoose.h/.cpp).
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 0049aa90의 번호 wildcard(-1) 프레임 후보 선택기다(cpppj <c>o/RawFrameChoose</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public static class RawFrameChoose
{
    /// <summary>
    /// 원본 0049aa90의 번호 wildcard(-1) 요청을 선택한다. theme=-1은 전체 후보, 0..3은 원소 quartile이다.
    /// </summary>
    /// <param name="frames">프레임 코드 표다.</param>
    /// <param name="code">요청 방향·변형 코드다.</param>
    /// <param name="theme">테마 번호(-1은 전체, 0..3은 사분위)다.</param>
    /// <param name="random">게임 난수 생성기다.</param>
    /// <returns>선택된 프레임 인덱스 또는 후보가 없으면 -1이다.</returns>
    public static int ChooseTerrainFrame(
        RiftTypeFrames frames,
        RiftFrameCode code,
        int theme,
        NetstormRandom random)
    {
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(random);
        List<int> candidates = [];

        void Collect(byte side, byte? variant)
        {
            candidates.Clear();
            IReadOnlyList<RiftFrameCode> codes = frames.Codes;
            // 각 검색은 프레임 코드 저장 순서대로 최대 256개를 채운다.
            for (int i = 0; i < codes.Count; i++)
            {
                if (codes[i].Side == side && (!variant.HasValue || codes[i].Variant == variant.Value) && candidates.Count < 256)
                {
                    candidates.Add(i);
                }
            }
        }

        Collect(code.Side, code.Variant);
        if (candidates.Count == 0)
        {
            (char normCard, char normDiag) = TerrainBuilder.Normalize((char)code.Side, (char)code.Variant);
            Collect((byte)normCard, (byte)normDiag);
        }
        if (candidates.Count == 0)
        {
            Collect(code.Side, (byte)'A');
        }
        if (candidates.Count == 0)
        {
            Collect(code.Side, null);
        }
        if (candidates.Count == 0)
        {
            return -1;
        }
        if (theme == -1)
        {
            return candidates[random.Next(candidates.Count)];
        }
        int quarter = candidates.Count / 4;
        int low = quarter * theme + 1;
        int high = quarter * (theme + 1);
        if (high <= low)
        {
            throw new ArgumentOutOfRangeException(nameof(theme), "지역 프레임 quartile 난수 범위 오류");
        }
        int index = low + random.Next(high - low);
        if (index < 0 || index >= candidates.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(theme), "지역 프레임 후보 범위 오류");
        }
        return candidates[index];
    }
}
