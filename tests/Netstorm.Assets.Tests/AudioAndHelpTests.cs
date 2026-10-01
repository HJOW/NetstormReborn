namespace Netstorm.Assets.Tests;

/// <summary>원본 소리(WAV·.mus)와 도움말 앵커 절 테스트</summary>
public sealed class AudioAndHelpTests
{
    /// <summary>원본 효과음·음악을 모두 해석하고, 음악 길이가 ffprobe 값과 같다</summary>
    [Fact]
    public void OriginalSoundsAndMusic_Parse()
    {
        string root = OriginalData.RequireDirectory();
        string[] files = [.. Directory.GetFiles(Path.Combine(root, "sound")), .. Directory.GetFiles(Path.Combine(root, "music"))];
        string[] waves = [.. files.Where(f => f.EndsWith(".wav", StringComparison.OrdinalIgnoreCase) || f.EndsWith(".mus", StringComparison.OrdinalIgnoreCase))];
        Assert.True(waves.Length > 200);
        // 파일마다 머리를 읽어 PCM 형식을 확인한다
        foreach (string file in waves)
        {
            WaveFile wave = WaveFile.ReadHeader(file);
            Assert.InRange(wave.SampleRate, 6000, 44100);
            Assert.True(wave.DataLength > 0, file);
        }
        WaveFile rain = WaveFile.ReadHeader(OriginalData.RequireFile("music/rain22.mus"));
        Assert.Equal((22050, 2, 16), (rain.SampleRate, rain.Channels, rain.BitsPerSample));
        Assert.Equal(214.08, rain.DurationSeconds, 2);
    }

    /// <summary>8bit 표본은 가운데 128 을 0 으로 하는 16bit 값이 된다</summary>
    [Fact]
    public void Pcm8_ConvertsToSigned16()
    {
        string path = OriginalData.RequireFile("sound/templeComplete.wav");
        using FileStream stream = File.OpenRead(path);
        byte[] pcm = WaveFile.ReadPcm16(stream, out WaveFile wave);
        Assert.Equal(8, wave.BitsPerSample);
        Assert.Equal(wave.DataLength * 2, pcm.Length);
        Assert.Equal([0, 0], wave.ToPcm16([128]));
        Assert.Equal([0, 0x7F], wave.ToPcm16([255]));
    }

    /// <summary>6,000Hz 원본(thunderCrack.wav)을 두 배 표본율로 올리면 길이는 같고 표본 수는 두 배다</summary>
    [Fact]
    public void Resample_KeepsDuration()
    {
        using FileStream stream = File.OpenRead(OriginalData.RequireFile("sound/thunderCrack.wav"));
        byte[] pcm = WaveFile.ReadPcm16(stream, out WaveFile wave);
        Assert.Equal(6000, wave.SampleRate);
        byte[] up = WaveFile.ResamplePcm16(pcm, wave.Channels, 6000, 12000);
        Assert.Equal(pcm.Length * 2, up.Length);
        Assert.Equal(pcm[..2], up[..2]);
        Assert.Equal([0, 0, 50, 0, 100, 0], WaveFile.ResamplePcm16([0, 0, 100, 0], 1, 1, 2)[..6]);
    }

    /// <summary>합성 도움말: 연달아 붙은 앵커는 본문을 공유하고 &lt;info&gt; 는 지운다</summary>
    [Fact]
    public void HelpTopics_SplitsAnchors()
    {
        var help = new HelpTopics("<a name=\"one\">\n<h2>A</h2>\n</a>\n\n<a name=\"runeType\">\n<a name=\"altarType\">\n<info>\n<q>Q</q>\n<p>\nSee <a href=\"#one\">one</a>.\n</a>\n");
        Assert.Equal(3, help.Count);
        Assert.Equal(help.Find("runeType"), help.ForType("altar"));
        IReadOnlyList<TutorialTextRun> runs = HelpTopics.ToRuns(help.ForType("altar")!);
        Assert.Equal(["Q", " See ", "one", "."], runs.Select(r => r.Text));
        Assert.Equal(TutorialTextStyle.Emphasis, runs[0].Style);
        Assert.Equal(TutorialTextStyle.Highlight, runs[2].Style);
    }

    /// <summary>원본 help.english 의 Rain Generator 절이 녹화 상세창 본문과 같은 인용문으로 시작한다</summary>
    [Fact]
    public void OriginalHelp_HasUnitTopics()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), GameLanguage.English);
        HelpTopics help = resources.TryLoadHelp() ?? throw new InvalidDataException("help.english 없음");
        IReadOnlyList<TutorialTextRun> runs = HelpTopics.ToRuns(help.ForType("rainBattery")!);
        Assert.StartsWith("\"In the Beyond, there is one sound that is always present", runs[0].Text.TrimStart());
        Assert.NotNull(help.Find("sacrificeOutline"));
    }
}
