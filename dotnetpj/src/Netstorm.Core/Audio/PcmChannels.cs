namespace Netstorm.Core.Audio;

/// <summary>원본 Options의 스피커 교환을 위한 16비트 스테레오 PCM 변환.</summary>
public static class PcmChannels
{
    /// <summary>프레임마다 좌·우 16비트 표본을 교환한다. 표본 자체와 재생 길이는 유지한다.</summary>
    public static void SwapStereo16(Span<byte> pcm)
    {
        if (pcm.Length % 4 != 0) throw new ArgumentException("스테레오 PCM은 4바이트 프레임이어야 합니다.", nameof(pcm));
        // 각 프레임의 두 채널을 바이트 순서 변경 없이 교환한다.
        for (int i = 0; i < pcm.Length; i += 4)
        {
            (pcm[i], pcm[i + 2]) = (pcm[i + 2], pcm[i]);
            (pcm[i + 1], pcm[i + 3]) = (pcm[i + 3], pcm[i + 1]);
        }
    }
}
