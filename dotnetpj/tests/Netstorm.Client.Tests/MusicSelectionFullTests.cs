using System.Globalization;
using System.Runtime.Versioning;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 같은 실제 선택/제어/채널/Winmm 열린 파일로 곡 선택의 모든 독립 원문행을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>앞선 미준비372행을 포함한 전체2064행과 실제 연속 곡 교체·옵션 재선택을 검증한다.</summary>
[SupportedOSPlatform("windows")]
public sealed class MusicSelectionFullTests
{
    /// <summary>688행씩의 같은256BYTE 이름·96BYTE 채널·파일 위치·생존 파일 수·중첩 잠금을 원문 그대로 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void EveryOriginalSelectionMatchesActualFileAndChannel(string name, OriginalEdition edition)
    {
        if (!OperatingSystem.IsWindows()) return;
        string[][] all = X86Fixture.Read("musicselection");
        Assert.Equal(2064, all.Length);
        string[][] rows = all.Where(row => row[0] == name).ToArray();
        Assert.Equal(688, rows.Length);
        Assert.Equal(124, rows.Count(row => row[1].Split(',')[4] == "0"));
        Assert.Equal(564, rows.Count(row => row[1].Split(',')[4] != "0"));
        using var inputs = new MusicOracleFiles();
        // 기대 출력은 입력에 쓰지 않고 같은 실제 파일 소유자와 음악 본체들을 매 행 새로 조립한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            string[] value = row[1].Split(',');
            Assert.Equal(15, value.Length);
            using var scene = new MusicOracleScene(inputs, edition)
            {
                SelectionLookup = true,
                FindMode = MusicOracleScene.Whole(value[8]),
                FileMode = MusicOracleScene.Whole(value[9]),
                CreateResult = MusicOracleScene.Whole(value[10]),
                SeekFailure = MusicOracleScene.Whole(value[11]),
            };
            var oldHeader = new MusicFileHeader();
            Assert.True(scene.Files.Open(inputs.Valid, oldHeader));
            Assert.Equal(1u, oldHeader.File);
            scene.LiveFiles.Add(oldHeader.File);
            Assert.Equal(5, scene.Files.Seek(oldHeader.File, 5));
            scene.Seed(MusicOracleScene.Number(value[7]) != 0, MusicOracleScene.Number(value[6]));
            scene.State.SetField(MusicField.File, oldHeader.File);
            Span<byte> current = scene.Selected.Raw().Span;
            current.Fill(0xae);
            byte[] old = Convert.FromHexString(value[0]);
            old.CopyTo(current);
            current[old.Length] = 0;
            current[255] = 0x7d;
            scene.Selected.Enabled = MusicOracleScene.Whole(value[2]);
            scene.Sound.MusicInitialized = MusicOracleScene.Number(value[4]) != 0;
            scene.Sound.Device = MusicOracleScene.Number(value[5]) != 0;
            MusicOpenHooks open = scene.OpenHooks();
            MusicPlaybackDevices playback = scene.Channel.BindPlaybackDevices(open, scene.BufferHooks());
            var selection = new MusicSelection(scene.Selected, scene.Music,
                new(MusicOracleScene.Text(value[12]), MusicOracleScene.Text(value[13])), open.Find!, playback);
            if (MusicOracleScene.Number(value[14]) != 0) selection.Select(MusicOracleScene.Name(value[1]));
            else selection.Select(MusicOracleScene.Name(value[1]), MusicOracleScene.Whole(value[3]));
            int position = scene.Files.Position(scene.State.Field(MusicField.File));
            Assert.Equal(0, scene.LockDepth);
            Assert.Equal(row[2], scene.Events());
            Assert.Equal(Convert.FromHexString(row[3]), scene.State.Raw().ToArray());
            Assert.Equal(Convert.FromHexString(row[4]), scene.Selected.Raw().ToArray());
            Assert.Equal(int.Parse(row[5], CultureInfo.InvariantCulture), position);
            Assert.Equal(MusicOracleScene.Number(row[6]), (uint)scene.LiveFiles.Count);
        }
    }

    /// <summary>같은 이름·특수곡·없는 곡·음악 끄기의 실제 연속 파일 토큰 수명과 loop 규칙을 검사한다.</summary>
    [Fact]
    public void ConsecutiveSelectionKeepsOriginalNameAndOwnsFilesUntilStop()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var inputs = new MusicOracleFiles();
        using var scene = new MusicOracleScene(inputs, OriginalEdition.Patch1078) { SelectionLookup = true };
        scene.Sound.MusicInitialized = scene.Sound.Device = true;
        MusicOpenHooks open = scene.OpenHooks();
        var playback = scene.Channel.BindPlaybackDevices(open, scene.BufferHooks());
        var selection = new MusicSelection(scene.Selected, scene.Music, new("music", "backup"), open.Find!, playback);
        selection.Select("old.mus");
        uint old = scene.State.Field(MusicField.File);
        Assert.True(scene.Channel.Active());
        Assert.Equal(3u, scene.State.Field(MusicField.Flags));
        string before = scene.Events();
        selection.Select("OLD.MUS");
        Assert.Equal("old.mus", scene.Selected.Current());
        Assert.Equal(old, scene.State.Field(MusicField.File));
        Assert.Equal(before, scene.Events());
        selection.Select("FANFARE.MUS");
        Assert.Equal("FANFARE.MUS", scene.Selected.Current());
        Assert.Equal(1u, scene.State.Field(MusicField.Flags));
        Assert.Equal(-1, scene.Files.Position(old));
        uint fanfare = scene.State.Field(MusicField.File);
        selection.Select("missing.mus");
        Assert.Equal("missing.mus", scene.Selected.Current());
        Assert.Equal(fanfare, scene.State.Field(MusicField.File));
        Assert.True(scene.Channel.Active());
        selection.Select("defeat.mus");
        Assert.Equal(1u, scene.State.Field(MusicField.Flags));
        Assert.Equal(-1, scene.Files.Position(fanfare));
        scene.Selected.Enabled = 0;
        selection.Select("track.mus");
        Assert.Equal("track.mus", scene.Selected.Current());
        Assert.False(scene.Channel.Active());
        Assert.Empty(scene.LiveFiles);
        Assert.Equal(0, scene.LockDepth);
    }

    /// <summary>실제 파일 열기 실패의 demo 재선택이 명시 옵션보다 현재 전역 옵션을 따르고 재귀를 종료하는지 검사한다.</summary>
    [Fact]
    public void ActualFileOpenFailureReentersGlobalOptionAndStopsDemoRecursion()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var inputs = new MusicOracleFiles();
        using var scene = new MusicOracleScene(inputs, OriginalEdition.Patch1078) { SelectionLookup = true, FileMode = 3 };
        scene.Sound.MusicInitialized = scene.Sound.Device = true;
        MusicOpenHooks open = scene.OpenHooks();
        var playback = scene.Channel.BindPlaybackDevices(open, scene.BufferHooks());
        var selection = new MusicSelection(scene.Selected, scene.Music, new(), open.Find!, playback);
        selection.Select("track.mus");
        Assert.Equal("demo.mus", scene.Selected.Current());
        Assert.True(scene.Channel.Active());
        Assert.Equal(3u, scene.State.Field(MusicField.Flags));
        Assert.Single(scene.LiveFiles);
        scene.Music.Stop(playback);
        scene.Selected.Enabled = 0;
        selection.Select("track.mus", 1);
        Assert.Equal("demo.mus", scene.Selected.Current());
        Assert.False(scene.Channel.Active());
        Assert.Empty(scene.LiveFiles);
        scene.Selected.Enabled = 1;
        scene.FileMode = 1;
        selection.Select("track.mus");
        Assert.Equal("demo.mus", scene.Selected.Current());
        Assert.False(scene.Channel.Active());
        Assert.Empty(scene.LiveFiles);
        Assert.Equal(0, scene.LockDepth);
    }

    /// <summary>상위 존재 검사와 채널 Open이 같은 실제 디스크 resolver를 공유하고 보조 경로의 곡으로 교체한다.</summary>
    [Fact]
    public void ActualDiskLookupAndOpenShareSecondaryDirectoryAndFileLifetime()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var inputs = new MusicOracleFiles();
        byte[] wave = File.ReadAllBytes(inputs.Valid);
        inputs.Write("music/old.mus", wave);
        inputs.Write("backup/track.mus", wave);
        using var scene = new MusicOracleScene(inputs, OriginalEdition.Patch1078);
        scene.Sound.MusicInitialized = scene.Sound.Device = true;
        MusicOpenHooks open = scene.Files.OpenHooks(WindowsSoundFiles.MakeDiskResolver(inputs.Root));
        var playback = scene.Channel.BindPlaybackDevices(open, scene.BufferHooks());
        var selection = new MusicSelection(scene.Selected, scene.Music, new("music", "backup"), open.Find!, playback);
        selection.Select("old.mus");
        uint old = scene.State.Field(MusicField.File);
        Assert.True(scene.Channel.Active());
        selection.Select("track.mus");
        Assert.Equal(-1, scene.Files.Position(old));
        Assert.True(scene.Channel.Active());
        Assert.Equal("track.mus", scene.Selected.Current());
        Assert.Equal(46, scene.Files.Position(scene.State.Field(MusicField.File)));
        scene.Selected.Enabled = 0;
        selection.Select("track.mus");
        Assert.Equal(0u, scene.State.Field(MusicField.File));
        Assert.Equal(0, scene.LockDepth);
    }
}
