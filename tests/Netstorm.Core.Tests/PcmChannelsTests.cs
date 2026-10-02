using Netstorm.Core.Audio;
using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>실제 스테레오 교환과 옵션 저장 호환성을 검증한다.</summary>
public sealed class PcmChannelsTests
{
    /// <summary>서로 다른 좌·우 표본이 교환되고 다시 교환하면 원래 음성으로 돌아온다.</summary>
    [Fact]
    public void StereoSwap_IsReversibleAndKeepsSampleBytes()
    {
        byte[] original = [1, 2, 3, 4, 0xFE, 0xFF, 10, 0];
        byte[] pcm = original.ToArray();
        PcmChannels.SwapStereo16(pcm);
        Assert.Equal([3, 4, 1, 2, 10, 0, 0xFE, 0xFF], pcm);
        PcmChannels.SwapStereo16(pcm);
        Assert.Equal(original, pcm);
        Assert.Throws<ArgumentException>(() => PcmChannels.SwapStereo16(new byte[3]));
    }

    /// <summary>새 설정은 저장·복구되고 기존 설정 파일은 바람 켜짐·채널 교환 꺼짐을 사용한다.</summary>
    [Fact]
    public void AudioOptions_RoundTripAndOldSettingsRemainCompatible()
    {
        string directory = Path.Combine(Path.GetTempPath(), "netstorm-options-" + Guid.NewGuid().ToString("N"));
        string path = Path.Combine(directory, DisplaySettings.FileName);
        try
        {
            var settings = new DisplaySettings { WindNoise = false, SpeakerSwap = true };
            Assert.True(settings.Save(path));
            DisplaySettings restored = DisplaySettings.Load(path);
            Assert.False(restored.WindNoise); Assert.True(restored.SpeakerSwap);
            File.WriteAllText(path, "{\"soundOn\":false}");
            restored = DisplaySettings.Load(path);
            Assert.True(restored.WindNoise); Assert.False(restored.SpeakerSwap);
        }
        finally
        {
            if (Directory.Exists(directory)) Directory.Delete(directory, true);
        }
    }

    /// <summary>
    /// 원본 Options 의 Auto-Demo·Tell Tips at Startup·Pass Server Diagnostic 과 팁 번호가 저장·복구되고,
    /// 이전 설정 파일은 원본 녹화의 시작 상태(Auto-Demo·팁 켜짐, 진단 꺼짐, 팁 0번)를 쓴다. 음수 팁 번호는 0 으로 맞춘다.
    /// </summary>
    [Fact]
    public void StartupOptions_RoundTripWithRecordedDefaults()
    {
        string directory = Path.Combine(Path.GetTempPath(), "netstorm-options-" + Guid.NewGuid().ToString("N"));
        string path = Path.Combine(directory, DisplaySettings.FileName);
        try
        {
            var settings = new DisplaySettings { AutoDemo = false, TellTips = false, TipNumber = 15, PassServerDiagnostic = true };
            Assert.True(settings.Save(path));
            DisplaySettings restored = DisplaySettings.Load(path);
            Assert.False(restored.AutoDemo); Assert.False(restored.TellTips);
            Assert.Equal(15, restored.TipNumber); Assert.True(restored.PassServerDiagnostic);
            File.WriteAllText(path, "{\"soundOn\":false,\"tipNumber\":-3}");
            restored = DisplaySettings.Load(path);
            Assert.True(restored.AutoDemo); Assert.True(restored.TellTips);
            Assert.Equal(0, restored.TipNumber); Assert.False(restored.PassServerDiagnostic);
        }
        finally
        {
            if (Directory.Exists(directory)) Directory.Delete(directory, true);
        }
    }
}
