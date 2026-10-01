using Netstorm.Core.Audio;

namespace Netstorm.Core.Tests;

/// <summary>배경음악 선택 규칙 (docs/exe/music.md) 테스트</summary>
public sealed class MusicDirectorTests
{
    /// <summary>원본 곡 길이(초, 2026-10-01 ffprobe). 결과 음악은 30초 이하 규칙 검사용</summary>
    private static readonly Dictionary<string, double> Durations = new(StringComparer.OrdinalIgnoreCase)
    {
        ["ser22.mus"] = 203.9,
        ["wind22.mus"] = 206.8,
        ["rain22.mus"] = 214.1,
        ["thu22.mus"] = 225.0,
        ["sun22.mus"] = 232.8,
        ["sacrifice.mus"] = 139.9,
        ["fanfare.mus"] = 21.6,
        ["defeat.mus"] = 81.1,
        ["anticipation.mus"] = 106.5,
    };

    /// <summary>표의 길이를 쓰는 감독을 만든다</summary>
    private static MusicDirector Create() => new(name => Durations.GetValueOrDefault(name));

    /// <summary>요청 중 곡 이름만 뽑는다</summary>
    private static string[] Tracks(IEnumerable<AudioCue> cues) =>
        [.. cues.Where(c => c.Kind == AudioCueKind.Music).Select(c => c.Name)];

    /// <summary>메뉴는 ser22</summary>
    [Fact]
    public void Menu_PlaysSer22()
    {
        MusicDirector music = Create();
        music.StartMenu(0);
        Assert.Equal(["ser22.mus"], Tracks(music.TakeCues()));
        Assert.Equal(203.9, music.NextChangeAt, 3);
    }

    /// <summary>
    /// 2026-09-30 녹화 재현: 첫 곡 rain22(난수 색인 0 의 다음) → 곡 길이마다 thu22(천둥 효과음 동반) → sun22 → wind22.
    /// </summary>
    [Fact]
    public void Battle_CyclesElementTracksBackToBack()
    {
        MusicDirector music = Create();
        music.StartBattle(9.1, random: 500);
        Assert.Equal(["rain22.mus"], Tracks(music.TakeCues()));
        music.Update(9.1 + 214.0, mySacrificeInProgress: false);
        Assert.Empty(music.TakeCues());
        music.Update(9.1 + 214.1, false);
        IReadOnlyList<AudioCue> cues = music.TakeCues();
        Assert.Equal(["thu22.mus"], Tracks(cues));
        Assert.Contains(new AudioCue(AudioCueKind.Sound, "thunderCrack.wav"), cues);
        music.Update(music.NextChangeAt, false);
        Assert.Equal(["sun22.mus"], Tracks(music.TakeCues()));
        music.Update(music.NextChangeAt, false);
        Assert.Equal(["wind22.mus"], Tracks(music.TakeCues()));
        music.Update(music.NextChangeAt, false);
        Assert.Equal(["rain22.mus"], Tracks(music.TakeCues()));
    }

    /// <summary>난수 3999 → 색인 3 → 첫 곡은 다음 색인 0(wind22)</summary>
    [Fact]
    public void Battle_FirstTrackIsAfterRandomIndex()
    {
        MusicDirector music = Create();
        music.StartBattle(0, random: 3999);
        Assert.Equal(["wind22.mus"], Tracks(music.TakeCues()));
    }

    /// <summary>
    /// 의식 시작 → 즉시 희생 음악. 의식이 끝나도 곡 끝까지 이어지고, 끝나면 중단된 곡이 아니라 다음 원소 곡으로 간다
    /// (녹화: wind22 중 12:06.4 에 sacrifice 시작).
    /// </summary>
    [Fact]
    public void Sacrifice_InterruptsAndReturnsToNextTrackAfterItEnds()
    {
        MusicDirector music = Create();
        music.StartBattle(0, random: 3999);
        music.TakeCues();
        music.OnMySacrificeStarted(45.6);
        Assert.Equal(["sacrifice.mus"], Tracks(music.TakeCues()));
        // 의식은 약 82초 뒤 끝났지만 음악은 그대로다
        music.Update(45.6 + 82, mySacrificeInProgress: false);
        Assert.Empty(music.TakeCues());
        music.Update(45.6 + 139.9, false);
        Assert.Equal(["rain22.mus"], Tracks(music.TakeCues()));
    }

    /// <summary>곡이 끝나는 순간 의식이 진행 중이면 원소 곡 대신 희생 음악을 다시 튼다</summary>
    [Fact]
    public void Sacrifice_ContinuesWhenTrackEndsDuringRitual()
    {
        MusicDirector music = Create();
        music.StartBattle(0, random: 0);
        music.TakeCues();
        music.Update(music.NextChangeAt, mySacrificeInProgress: true);
        Assert.Equal(["sacrifice.mus"], Tracks(music.TakeCues()));
        double end = music.NextChangeAt;
        music.Update(end, true);
        Assert.Equal(["sacrifice.mus"], Tracks(music.TakeCues()));
        Assert.Equal(end + 139.9, music.NextChangeAt, 3);
    }

    /// <summary>결과 음악(30초 이하)은 재생 중 다른 요청을 막고, 다음 곡은 180초 뒤에 정한다</summary>
    [Fact]
    public void Result_LocksAndUsesShortTrackRecheck()
    {
        MusicDirector music = Create();
        music.StartBattle(0, random: 0);
        music.TakeCues();
        music.PlayResult(100, victory: true);
        Assert.Equal(["fanfare.mus"], Tracks(music.TakeCues()));
        Assert.Equal(100 + MusicDirector.ShortTrackRecheckSeconds, music.NextChangeAt, 3);
        music.OnMySacrificeStarted(110);
        Assert.Empty(music.TakeCues());
        music.OnMySacrificeStarted(125);
        Assert.Equal(["sacrifice.mus"], Tracks(music.TakeCues()));
    }
}
