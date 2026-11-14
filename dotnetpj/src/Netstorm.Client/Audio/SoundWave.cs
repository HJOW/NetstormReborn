// 오디오 장치와 분리한 원본 WAVE 읽기 결과를 보관한다.
namespace Netstorm.Client.Audio;

/// <summary>Winmm이 읽은 원문 형식18BYTE·data 위치·표본의 소유 결과다.</summary>
public sealed class SoundWave
{
    /// <summary>보정 후 WAVEFORMATEX18BYTE이며 장치에 전달할 때 PCM cbSize만 별도로0으로 한다.</summary>
    public byte[] Format { get; init; } = new byte[18];
    /// <summary>실제 파일 안 data 청크가 시작하는DWORD 위치다.</summary>
    public uint DataOffset { get; init; }
    /// <summary>Winmm이 RIFF/파일 끝으로 줄인 그대로의 표본BYTE다.</summary>
    public byte[] Samples { get; init; } = [];
}
