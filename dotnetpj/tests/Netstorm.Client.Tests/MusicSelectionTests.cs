using System.Globalization;
using System.Text;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 곡 이름과 선택 부모를 원문 자연 미준비372행 및 명시 채널 경계와 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>음악 채널 Open/worker를 구현하지 않고 원문2064행 중 미준비372행의 전체 곡 이름과 파일 조회 순서를 검증한다.</summary>
public sealed class MusicSelectionTests
{
    /// <summary>원문DWORD 십진수를 signed 비트 그대로 읽는다.</summary>
    private static int Whole(string value) => unchecked((int)uint.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원문 좁은 이름의 HEX를 손실 없이 복원한다.</summary>
    private static string Name(string value) => Encoding.Latin1.GetString(Convert.FromHexString(value));
    /// <summary>원문 사건과 같은 소문자HEX로 좁은 경로를 기록한다.</summary>
    private static string Hex(string value) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(value));

    /// <summary>같은 실제 SoundState/효과음 재생기/음악 제어만 조립한다. 음악 채널은 만들지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>제어의 준비·장치·음량·음소거 전역이다.</summary>
        public readonly SoundState Sound = new();
        /// <summary>이름과 실제 signed 음악 옵션이다.</summary>
        public readonly MusicSelectionState Selected = new();
        /// <summary>같은 실제 소리 전역과 명령 잠금을 보유한다.</summary>
        public readonly SoundMusicControl Music;
        /// <summary>미선택 효과음 호출은 오류로 두고 실제 제어를 만든다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            var effects = new SoundPlayer(new SoundList(edition), Sound, new()
            {
                Status = _ => throw new InvalidOperationException("미선택 효과음 상태"),
                Play = (_, _) => throw new InvalidOperationException("미선택 효과음 재생"),
                SetPosition = (_, _) => throw new InvalidOperationException("미선택 효과음 위치"),
                SetVolume = (_, _) => throw new InvalidOperationException("미선택 효과음 음량"),
                SetPan = (_, _) => throw new InvalidOperationException("미선택 효과음 좌우"),
                Stop = _ => throw new InvalidOperationException("미선택 효과음 정지"),
                Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("미선택 효과음 복제"),
                Load = _ => throw new InvalidOperationException("미선택 효과음 파일"),
            });
            Music = new SoundMusicControl(edition, Sound, effects);
        }
    }

    /// <summary>실제 MusicInitialized0 관측124행씩의 전체256BYTE 이름과 상위 존재 조회를 전수 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void OriginalUninitializedSubsetMatchesWholeNameAndLookup(string name, OriginalEdition edition)
    {
        string[][] all = X86Fixture.Read("musicselection");
        Assert.Equal(2064, all.Length);
        string[][] rows = all.Where(row => row[0] == name && row[1].Split(',')[4] == "0").ToArray();
        Assert.Equal(124, rows.Length);
        // 실제 채널을 요구하지 않는 원문 입력만 같은 실제 선택 부모에 공급한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            string[] input = row[1].Split(',');
            Assert.Equal(15, input.Length);
            var scene = new Scene(edition);
            scene.Sound.Device = Whole(input[5]) != 0;
            scene.Selected.Enabled = Whole(input[2]);
            Span<byte> raw = scene.Selected.Raw().Span;
            raw.Fill(0xae);
            byte[] previous = Convert.FromHexString(input[0]);
            previous.CopyTo(raw);
            raw[previous.Length] = 0;
            raw[255] = 0x7d;
            var events = new List<string>();
            string? Find(string query)
            {
                string lower = query.ToLowerInvariant();
                bool found = Whole(input[8]) != 0 && !lower.Contains("missing.mus", StringComparison.Ordinal);
                if (Whole(input[8]) == 2) found &= !lower.Contains("demo.mus", StringComparison.Ordinal);
                if (Whole(input[8]) == 3) found &= lower.Contains("demo.mus", StringComparison.Ordinal);
                events.Add("F:" + Hex(query) + ':' + (found ? Hex(query) : "-"));
                return found ? query : null;
            }
            var directories = new MusicDirectories(input[12] == "-" ? null : Name(input[12]), input[13] == "-" ? null : Name(input[13]));
            var selection = new MusicSelection(scene.Selected, scene.Music, directories, Find);
            if (Whole(input[14]) != 0) selection.Select(Name(input[1]));
            else selection.Select(Name(input[1]), Whole(input[3]));
            Assert.Equal(row[2], events.Count == 0 ? "-" : string.Join(';', events));
            Assert.Equal(Convert.FromHexString(row[4]), scene.Selected.Raw().ToArray());
            Assert.False(scene.Sound.MusicInitialized);
        }
    }

    /// <summary>첫255BYTE 복사·패딩과 마지막BYTE 보존·첫NUL 조회를 실제 이름 저장소에서 확인한다.</summary>
    [Fact]
    public void NameStoragePreservesLastByteAndRejectsMissingTerminator()
    {
        var state = new MusicSelectionState();
        state.Raw().Span.Fill(0xae);
        state.Raw().Span[255] = 0x7d;
        state.Assign("x");
        Assert.Equal("x", state.Current());
        Assert.All(state.Raw().ToArray()[1..255], value => Assert.Equal((byte)0, value));
        Assert.Equal((byte)0x7d, state.Raw().Span[255]);
        state.Assign(new string('x', 260));
        Assert.All(state.Raw().ToArray()[..255], value => Assert.Equal((byte)'x', value));
        Assert.Equal((byte)0x7d, state.Raw().Span[255]);
        Assert.Throws<ArgumentException>(() => state.Current());
    }

    /// <summary>null과 빈 보조 경로의 원문 재붙이기를 구별하며 실제 경로 범위 오류를 거부한다.</summary>
    [Theory]
    [InlineData(null, "music\\track.mustrack.mus")]
    [InlineData("", "\\track.mus")]
    public void FileLookupPreservesNullAndEmptyDirectoryMeaning(string? secondary, string expectedSecond)
    {
        var queries = new List<string>();
        string? Find(string query) { queries.Add(query); return null; }
        Assert.Null(MusicFileLookup.FindMusicFile("track.mus", new("music", secondary), Find));
        Assert.Equal(new[] { "music\\track.mus", expectedSecond }, queries);
        Assert.Throws<ArgumentException>(() => MusicFileLookup.FindMusicFile(new string('x', 256), new(), Find));
        Assert.Throws<ArgumentException>(() => MusicFileLookup.FindMusicFile("x\0y", new(), Find));
        Assert.Equal(2, queries.Count);
    }

    /// <summary>없는 실제 채널은 자연 관문 뒤에만 요구하며 Select의 앞선 이름 쓰기는 실패 뒤에도 보존한다.</summary>
    [Fact]
    public void MissingSelectedChannelFailsAfterNameStoreWithoutFakeSuccess()
    {
        var scene = new Scene();
        var selection = new MusicSelection(scene.Selected, scene.Music, new(), _ => null);
        selection.Select("track.mus");
        Assert.Equal("track.mus", scene.Selected.Current());
        scene.Sound.MusicInitialized = true;
        scene.Sound.Device = true;
        Assert.Throws<InvalidOperationException>(() => selection.Select("next.mus"));
        Assert.Equal("next.mus", scene.Selected.Current());
        Assert.Throws<InvalidOperationException>(() => scene.Music.Stop(null));
        scene.Music.Stopping = true;
        Assert.False(scene.Music.Play("ignored.mus", 1, new(), _ => null, null));
    }

    /// <summary>명시된 채널 경계 입력으로 특수곡 loop0과 일반곡 loop1의 실제 전달 인자만 관측한다.</summary>
    [Theory]
    [InlineData("FANFARE.MUS", 0u)]
    [InlineData("defeat.mus", 0u)]
    [InlineData("track.mus", 1u)]
    public void SelectedPlaybackForwardsOriginalLoopWithoutImplementingChannel(string name, uint loop)
    {
        var scene = new Scene();
        scene.Sound.MusicInitialized = scene.Sound.Device = true;
        var calls = new List<string>();
        var devices = new MusicPlaybackDevices
        {
            Active = () => { calls.Add("active"); return false; },
            Open = (selected, _, _) => { calls.Add("open:" + selected); return true; },
            EnsureBuffer = () => { calls.Add("buffer"); return true; },
            Start = selectedLoop => { calls.Add("start:" + selectedLoop); return false; },
        };
        var selection = new MusicSelection(scene.Selected, scene.Music, new(), _ => null, devices);
        selection.Select(name);
        Assert.Equal(new[] { "active", "open:" + name, "buffer", "start:" + loop }, calls);
        Assert.Equal(name, scene.Selected.Current());
    }

    /// <summary>파일 열기 실패의 demo 재선택은 명시 인자 대신 같은 현재 전역 옵션을 다시 읽고 재귀를 종료한다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    public void OpenFailureReentersCurrentGlobalOptionAndStopsDemoRecursion(int global)
    {
        var scene = new Scene();
        scene.Sound.MusicInitialized = scene.Sound.Device = true;
        scene.Selected.Enabled = global;
        var opened = new List<string>();
        int activeReads = 0, stops = 0;
        var devices = new MusicPlaybackDevices
        {
            Active = () => { activeReads++; return false; },
            Open = (name, _, _) => { opened.Add(name); return false; },
            Stop = () => stops++,
        };
        var selection = new MusicSelection(scene.Selected, scene.Music, new(), _ => null, devices);
        selection.Select("track.mus", 1);
        Assert.Equal("demo.mus", scene.Selected.Current());
        Assert.Equal(global == 0 ? new[] { "track.mus" } : new[] { "track.mus", "demo.mus" }, opened);
        Assert.Equal(global == 0 ? 1 : 0, stops);
        Assert.Equal(global == 0 ? 2 : 3, activeReads);
    }

    /// <summary>공개 Select는 입력 첫NUL을 사용하고 같은 이름의 대소문자 표기와 현재 저장소 나머지를 보존한다.</summary>
    [Fact]
    public void CStringAndUnchangedCasePreserveActualNameBytes()
    {
        var scene = new Scene();
        scene.Selected.Assign("old.mus");
        byte[] before = scene.Selected.Raw().ToArray();
        var selection = new MusicSelection(scene.Selected, scene.Music, new(), _ => throw new InvalidOperationException("같은 이름의 미선택 조회"));
        selection.Select("OLD.MUS\0ignored");
        Assert.Equal(before, scene.Selected.Raw().ToArray());
        Assert.Throws<ArgumentException>(() => selection.Select(null));
        scene.Selected.Raw().Span.Fill(1);
        Assert.Throws<ArgumentException>(() => selection.Select("new.mus"));
    }
}
