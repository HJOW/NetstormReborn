using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 서버 목록 파일과 파일명 작성기의 실제 바이트/수명/디스크 부작용을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 formatter fixture와 사용자 파일의 우선순위·실패 위치를 확인한다.</summary>
public sealed class RootServerListDiskTests
{
    /// <summary>현재 raw BYTE 입력을 만든다.</summary>
    private static byte[] Bytes(string text) => Encoding.Latin1.GetBytes(text);

    /// <summary>원본352입력의 성공/실패·선증가칸·NUL뒤까지256BYTE전체를 대조한다.</summary>
    [Fact]
    public void FormatterMatchesEveryOriginalPhysicalOutput()
    {
        string[][] rows = X86Fixture.Read("file-name-formatter"); Assert.Equal(352, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var owner = new RawFileName();
            // 원본 입력의 현재칸까지 같은 빈 작성으로 고리를 전진시킨다.
            for (int index = 0; index < X86Fixture.Int(row[1]); index++) owner.Format([]);
            FileNamePart[] parts = row[2] == "-" ? [] : row[2].Split(';').Select(cell =>
            {
                int colon = cell.IndexOf(':'); return new FileNamePart(int.Parse(cell[..colon]), X86Fixture.Bytes(cell[(colon + 1)..]));
            }).ToArray();
            bool failed = false;
            try { owner.Format(parts, (byte)X86Fixture.Int(row[0])); } catch (ArgumentException) { failed = true; }
            return failed == (row[3] == "1") && owner.Index == X86Fixture.Int(row[4]) && owner.CurrentBytes.Span.SequenceEqual(X86Fixture.Bytes(row[5]));
        });
    }

    /// <summary>앞선반환뷰는9번뒤보존·10번뒤재사용되며현재칸입력별칭은선행clear를관찰한다.</summary>
    [Fact]
    public void FormatterBorrowsSameRingAndClearsBeforeReadingAliasedInput()
    {
        var owner = new RawFileName(); ReadOnlyMemory<byte> borrowed = owner.Format([new(-3, Bytes("first"))]);
        // 다음9칸호출은기존반환물리를덮지않는다.
        for (int index = 0; index < 9; index++) owner.Format([new(-3, Bytes("other"))]);
        Assert.Equal(".\\first", Encoding.Latin1.GetString(borrowed.Span));
        ReadOnlyMemory<byte> selected = owner.Format([new(-3, borrowed)]);
        Assert.Equal(".\\", Encoding.Latin1.GetString(selected.Span)); Assert.Equal((byte)'.', borrowed.Span[0]); Assert.Equal((byte)0, borrowed.Span[2]);
    }

    /// <summary>별도루트만만들어검사하며원본및설치보호자료는사용하지않는다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>이검사만소유하는임시루트다.</summary>
        public readonly string Root = Path.Combine(Path.GetTempPath(), "dn-root-" + Guid.NewGuid().ToString("N"));
        /// <summary>동일서버전역이다.</summary>
        public readonly ClientNsInfoGlobals Info = new();
        /// <summary>동일설치파일장치다.</summary>
        public readonly GameFileSystem Files;
        /// <summary>원문DataDir설정이다.</summary>
        public readonly ConfigInterface Configuration = new(_ => null);
        /// <summary>실제파일소비자다.</summary>
        public readonly RootServerListDisk Disk;
        /// <summary>설치와사용자루트를분리하고두원문파일이름을지원한다.</summary>
        public Scene()
        {
            Files = new GameFileSystem(Path.Combine(Root, "install"), []); Directory.CreateDirectory(Path.Combine(Files.BaseDirectory, "d"));
            Configuration.Set("DataDir", "d"); Disk = new RootServerListDisk(Info, Files, Configuration, new UserDataRoot(Path.Combine(Root, "user")));
        }
        /// <summary>자신의절대임시루트만반납한다.</summary>
        public void Dispose() => Directory.Delete(Root, true);
        /// <summary>설치자료의행을쓴다.</summary>
        public void Install(string name, string text) => File.WriteAllBytes(Path.Combine(Files.BaseDirectory, "d", name), Bytes(text));
        /// <summary>사용자파일행을쓴다.</summary>
        public void User(string text) { Directory.CreateDirectory(Path.GetDirectoryName(Disk.UserFile)!); File.WriteAllBytes(Disk.UserFile, Bytes(text)); }
    }

    /// <summary>세파일부재는같은목록/합계를보존하고폴더를만들지않는다.</summary>
    [Fact]
    public void MissingFilesPreserveCurrentOwnerWithoutCreatingUserDirectory()
    {
        using var scene = new Scene(); scene.Info.LoadServerNames(Bytes("existing;Name;1")); uint sum = scene.Info.ServerListChecksum;
        scene.Disk.Load(); Assert.Equal(new[] { "existing;Name;1" }, scene.Info.Servers); Assert.Equal(sum, scene.Info.ServerListChecksum);
        Assert.False(Directory.Exists(Path.GetDirectoryName(scene.Disk.UserFile)));
    }

    /// <summary>일반설치파일→!설치파일→사용자파일의우선순위를순서대로확인한다.</summary>
    [Fact]
    public void ExistingFilesFollowExactUserBangAndPlainPrecedence()
    {
        using var scene = new Scene(); scene.Install("rootservers.dat", "plain;Name;1\n"); scene.Disk.Load(); Assert.Equal("plain;Name;1", Assert.Single(scene.Info.Servers));
        scene.Install("!rootservers.dat", "bang;Name;2\n"); scene.Disk.Load(); Assert.Equal("bang;Name;2", Assert.Single(scene.Info.Servers));
        scene.User("user;Name;3\n"); scene.Disk.Load(); Assert.Equal("user;Name;3", Assert.Single(scene.Info.Servers));
    }

    /// <summary>빈사용자파일및빈!설치파일도성공이므로뒤파일을읽지않는다.</summary>
    [Fact]
    public void EmptySelectedFileClearsListInsteadOfFallingBack()
    {
        using var scene = new Scene(); scene.Install("rootservers.dat", "plain;Name;1\n"); scene.Install("!rootservers.dat", "");
        scene.Info.Servers.Add("old"); scene.Disk.Load(); Assert.Empty(scene.Info.Servers); Assert.Equal(0u, scene.Info.ServerListChecksum);
        scene.Install("!rootservers.dat", "bang;Name;2\n"); scene.User(""); scene.Disk.Load(); Assert.Empty(scene.Info.Servers);
    }

    /// <summary>선택된긴파일의실패는뒤정상파일로숨기지않고기존목록을보존한다.</summary>
    [Fact]
    public void MalformedSelectedFileDoesNotRetryInstallationFallback()
    {
        using var scene = new Scene(); scene.Install("rootservers.dat", "valid;Name;1"); scene.User(new string('x', 257)); scene.Info.Servers.Add("old");
        Assert.Throws<ArgumentException>(scene.Disk.Load); Assert.Equal("old", Assert.Single(scene.Info.Servers));
    }

    /// <summary>같은실제고리작성뒤사용자파일만갱신하고행LF/NUL/비ASCII및stale합계를유지한다.</summary>
    [Fact]
    public void SaveKeepsRawRowsAndSharedNameSideEffectWithoutWritingInstallation()
    {
        using var scene = new Scene(); scene.Install("!rootservers.dat", "protected"); scene.Info.ServerListChecksum = 777;
        scene.Info.Servers.AddRange(["a;\xff;1", "b\nembedded", "c\0ignored\n", "", "already\n"]); scene.Disk.Save();
        Assert.Equal(Bytes("a;\xff;1\nb\nembeddedc\n\nalready\n"), File.ReadAllBytes(scene.Disk.UserFile));
        Assert.Equal("protected", File.ReadAllText(Path.Combine(scene.Files.BaseDirectory, "d", "!rootservers.dat"))); Assert.Equal(777u, scene.Info.ServerListChecksum);
        Assert.Equal(1, scene.Files.FileNameOwner.Index); string path = Encoding.UTF8.GetString(scene.Files.FileNameOwner.CurrentBytes.Span).Split('\0')[0];
        Assert.EndsWith("\\d\\!rootservers.dat", path);
        scene.Info.Servers.Clear(); scene.Disk.Save(); Assert.Empty(File.ReadAllBytes(scene.Disk.UserFile)); Assert.Equal(2, scene.Files.FileNameOwner.Index);
    }

    /// <summary>원문fopen실패는조용히복귀하되앞선이름한칸작성효과를보존한다.</summary>
    [Fact]
    public void OpenFailureReturnsAfterAdvancingNameOwner()
    {
        using var scene = new Scene(); Directory.CreateDirectory(scene.Disk.UserFile); scene.Info.Servers.Add("current");
        scene.Disk.Save(); Assert.Equal(1, scene.Files.FileNameOwner.Index); Assert.True(Directory.Exists(scene.Disk.UserFile));
    }

    /// <summary>사용자경로를rb로열수없으면설치목록으로진행하며디렉터리를빈파일로취급하지않는다.</summary>
    [Fact]
    public void UserOpenFailureContinuesToInstallationList()
    {
        using var scene = new Scene(); Directory.CreateDirectory(scene.Disk.UserFile); scene.Install("!rootservers.dat", "fallback;Name;1");
        scene.Disk.Load(); Assert.Equal("fallback;Name;1", Assert.Single(scene.Info.Servers));
    }

    /// <summary>폴더생성실패는fopen실패와구분하고이미완료된작성칸을보존한다.</summary>
    [Fact]
    public void DirectoryCreationFailureKeepsEarlierNameEffectAndPropagates()
    {
        using var scene = new Scene(); string parent = Path.GetDirectoryName(scene.Disk.UserFile)!; Directory.CreateDirectory(Path.GetDirectoryName(parent)!); File.WriteAllText(parent, "blocked");
        Assert.Throws<IOException>(scene.Disk.Save); Assert.Equal(1, scene.Files.FileNameOwner.Index); Assert.False(File.Exists(scene.Disk.UserFile));
    }
}
