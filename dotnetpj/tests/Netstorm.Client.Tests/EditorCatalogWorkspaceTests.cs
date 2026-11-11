using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 편집 맵 목록·직접 Edit 준비가 이관 장부를 거쳐 실제 작업 디렉터리만 사용하는지 임시 원본·작업본으로 검사한다.
/// cpppj <c>EditorMapCatalogFilesTests</c>의 장부·삭제 보존·디렉터리·링크·잠금 항목을 옮겼다(2026-11-01 0.126).
/// 원본 x86 fixture가 없는 직역 검사이므로 L1이 아니다.
/// </summary>
public sealed class EditorCatalogWorkspaceTests
{
    /// <summary>목록이 읽는 Subscriber와 State만 채운 최소 Fort 이미지를 만든다. kind는 State의 17번째 바이트다.</summary>
    private static byte[] Image(byte kind, byte first = 0)
    {
        var bytes = new List<byte> { 0x46, 0 };
        // 고정 35섹션 중 목록이 읽는 처음 두 섹션만 채우고 나머지는 빈 섹션으로 둔다.
        for (int index = 0; index < 35; index++)
        {
            byte[] section = index == 0 ? [1, 0, 0, 0, 2, 0, (byte)'M', (byte)'e'] : index == 1 ? new byte[28] : [];
            if (index == 1) { section[0] = first; section[17] = kind; }
            int length = section.Length + 2; bytes.Add((byte)length); bytes.Add((byte)(length >> 8)); bytes.AddRange(section);
        }
        return [.. bytes];
    }

    /// <summary>한 항목을 담은 최소 TAFF 아카이브를 만든다. 데이터는 XOR로 인코딩한다.</summary>
    private static TaffArchive Archive(string name, byte[] data)
    {
        byte[] nameBytes = Encoding.Latin1.GetBytes(name), payload = (byte[])data.Clone(); XorCipher.Apply(payload);
        int directory = 0x30, dataStart = directory + 8 + nameBytes.Length + 1; var raw = new byte[dataStart + payload.Length];
        Encoding.ASCII.GetBytes("TAFF v0.2\x1a").CopyTo(raw, 0);
        BitConverter.GetBytes(1).CopyTo(raw, 0x14); BitConverter.GetBytes(directory).CopyTo(raw, 0x20); BitConverter.GetBytes(dataStart).CopyTo(raw, 0x28);
        BitConverter.GetBytes(0).CopyTo(raw, directory); BitConverter.GetBytes(data.Length).CopyTo(raw, directory + 4);
        nameBytes.CopyTo(raw, directory + 8); payload.CopyTo(raw, dataStart);
        return new TaffArchive(raw);
    }

    /// <summary>임시 원본(파일 시스템)과 작업본으로 편집 작업본 장치를 연다.</summary>
    private static EditorCatalogWorkspace Open(EditorTempRoot temp, params TaffArchive[] archives)
    {
        Directory.CreateDirectory(temp.Source);
        return new EditorCatalogWorkspace(new GameFileSystem(temp.Source, archives), temp.Work);
    }

    /// <summary>실제 목록 본체에 작업본 열거·읽기 장치를 연결한 맵 목록을 만든다.</summary>
    private static SoloMapCatalog Catalog(EditorTempRoot temp, EditorCatalogWorkspace workspace)
    {
        var config = new ConfigInterface(null); config.Startup(new()); config.Set("fortSpec", "d/{local.1}.fort"); config.SetInt("mayEditMissions", 1);
        return new SoloMapCatalog(new GameFileSystem(temp.Source, []), config, new SoloMapCatalogEffects
        {
            TypeCount = () => 188, AllocationBytes = _ => throw new InvalidOperationException("정상 목록의 예상 밖 빈 State"),
            UserName = () => "Me", Translate = text => text, EnumeratePaths = workspace.Enumerate, ReadFile = workspace.Read
        });
    }

    /// <summary>구분자만 통일한다. "."·".." 후보나 물리 파일명은 바꾸지 않는다.</summary>
    private static string[] Slash(IEnumerable<string> paths) => paths.Select(path => path.Replace('\\', '/')).ToArray();

    /// <summary>원본 FileSpec 이름 변환처럼 첫 점 앞까지를 이름으로 쓴다.</summary>
    private static string Name(string path)
    {
        string name = Path.GetFileName(path); int dot = name.IndexOf('.'); return dot < 0 ? name : name[..dot];
    }

    /// <summary>기존 사용자 내용과 원본 읽기 전용 속성을 보존하고 실제 작업 디렉터리의 OS 결과를 목록에 그대로 전달한다.</summary>
    [Fact]
    public void WorkOsOrderPreservesExistingCopyAndReadonly()
    {
        using var temp = new EditorTempRoot(); byte[] original = Image(2), edited = Image(1, 1);
        temp.Seed("d/First.fort", original); string second = temp.Seed("d/Second.fort", original); File.SetAttributes(second, FileAttributes.ReadOnly);
        using var workspace = Open(temp);
        workspace.Files.Prepare("First", edited); workspace.Files.Prepare("NewSaved", original);
        var listed = workspace.Enumerate("d/*.fort"); var actual = workspace.Files.MatchDisk("d/*.fort");
        Assert.Equal(Slash(actual), Slash(listed)); Assert.Equal(3, actual.Count);
        Assert.Equal(edited, workspace.Read("d/First.fort")); Assert.Equal(original, File.ReadAllBytes(Path.Combine(temp.Source, "d", "First.fort")));
        Assert.True((File.GetAttributes(workspace.Files.ResolvePath("d/Second.fort")) & FileAttributes.ReadOnly) != 0);
        var catalog = Catalog(temp, workspace);
        Assert.Equal(3, catalog.List.Refresh(2));
        // 실제 work 위치의 이름과 종류가 같은 칸에 있어야 한다. 편집한 First만 편집 가능 표시다.
        for (int index = 0; index < actual.Count; index++)
        {
            Assert.Equal(Name(actual[index]), catalog.State.Names[index]);
            Assert.Equal(Name(actual[index]).Equals("first", StringComparison.OrdinalIgnoreCase) ? 2u : 0u, catalog.State.Flags[index]);
        }
        Assert.True(File.GetAttributes(second).HasFlag(FileAttributes.ReadOnly)); File.SetAttributes(second, FileAttributes.Normal);
    }

    /// <summary>새 저장은 dirty 뒤 실제 작업 위치에서 조회되고 새 소유자도 기존 사용자 내용을 읽는다.</summary>
    [Fact]
    public void NewNameRefreshAndOwnerRestartUseActualWork()
    {
        using var temp = new EditorTempRoot(); byte[] original = Image(2), changed = Image(1, 1); temp.Seed("d/Original.fort", original);
        using (var workspace = Open(temp))
        {
            var catalog = Catalog(temp, workspace); Assert.Equal(1, catalog.List.Refresh(2));
            string saved = workspace.Files.Prepare("SavedAgain", original); workspace.Files.Write(saved, original);
            Assert.Equal(1, catalog.List.Refresh(2)); catalog.Invalidate(); Assert.Equal(2, catalog.List.Refresh(2));
            var actual = workspace.Files.MatchDisk("d/*.fort");
            for (int index = 0; index < actual.Count; index++) Assert.Equal(Name(actual[index]), catalog.State.Names[index]);
        }
        using var reopened = Open(temp);
        Assert.Equal(original, reopened.Read("d/SavedAgain.fort"));
        reopened.Files.Write(reopened.Files.ResolvePath("d/SavedAgain.fort"), changed);
        Assert.Equal(changed, reopened.Read("d/SavedAgain.fort")); Assert.False(File.Exists(Path.Combine(temp.Source, "d", "SavedAgain.fort")));
    }

    /// <summary>첫 점 이름이 같아도 두 물리 파일을 두 행으로 유지한다.</summary>
    [Fact]
    public void DuplicateLogicalNamesRemainTwoNativeRows()
    {
        using var temp = new EditorTempRoot(); byte[] bytes = Image(2);
        temp.Seed("d/Same.fort", bytes); temp.Seed("d/Same.extra.fort", bytes); temp.Seed("d/Other.fort", bytes);
        using var workspace = Open(temp);
        var listed = workspace.Enumerate("d/*.fort"); var actual = workspace.Files.MatchDisk("d/*.fort");
        Assert.Equal(3, listed.Count); Assert.Equal(Slash(actual), Slash(listed));
        var catalog = Catalog(temp, workspace); Assert.Equal(3, catalog.List.Refresh(2));
        int duplicates = 0;
        for (int index = 0; index < actual.Count; index++) { Assert.Equal(Name(actual[index]), catalog.State.Names[index]); if (Name(actual[index]).Equals("same", StringComparison.OrdinalIgnoreCase)) duplicates++; }
        Assert.Equal(2, duplicates); Assert.True(workspace.Files.PathExists("d/Same.extra.fort"));
    }

    /// <summary>새 이름이 목록의 처음 50칸에 들어가는지는 합성 추가가 아니라 실제 작업 디렉터리 순서에서 정해진다.</summary>
    [Fact]
    public void NativeFiftyMatchesActualWorkOsSnapshot()
    {
        using var temp = new EditorTempRoot(); byte[] bytes = Image(2);
        for (int index = 0; index < 53; index++) temp.Seed($"d/Original{index}.fort", bytes);
        using var workspace = Open(temp); workspace.ImportDisk("d/*.fort"); workspace.Files.Prepare("NewOnly", bytes);
        var actual = workspace.Files.MatchDisk("d/*.fort"); Assert.Equal(54, actual.Count); Assert.Equal(Slash(actual), Slash(workspace.Enumerate("d/*.fort")));
        var catalog = Catalog(temp, workspace); Assert.Equal(50, catalog.List.Refresh(2));
        bool expectedNew = false, listedNew = false;
        for (int index = 0; index < 50; index++)
        {
            Assert.Equal(Name(actual[index]), catalog.State.Names[index]);
            expectedNew |= Name(actual[index]).Equals("newonly", StringComparison.OrdinalIgnoreCase); listedNew |= catalog.State.Names[index] == "NewOnly";
        }
        Assert.Equal(expectedNew, listedNew);
    }

    /// <summary>삭제 완료 키는 모든 표기와 직접 준비에서 원본 loose 파일이나 아카이브로 되살아나지 않는다.</summary>
    [Fact]
    public void CompletedDeletionAliasesAndDirectPrepareNeverRevive()
    {
        using var temp = new EditorTempRoot(); byte[] bytes = Image(2), archived = Image(1);
        temp.Seed("d/Removed.fort", bytes); string source = Path.Combine(temp.Source, "d", "Removed.fort");
        string target;
        using (var workspace = Open(temp, Archive("\\d\\Removed.fort", archived)))
        {
            target = workspace.Prepare(source);
            Assert.Equal(workspace.Resolve("d/./REMOVED.fort"), target, ignoreCase: true); Assert.Equal(target, workspace.Resolve(target), ignoreCase: true);
            Assert.Equal(bytes, workspace.Read("d/Removed.fort"));
            workspace.Files.Delete(target); Assert.Null(workspace.Read(source));
        }
        using (var reopened = Open(temp, Archive("\\d\\Removed.fort", archived)))
        {
            Assert.Null(reopened.Read(target)); Assert.Null(reopened.Read("d/../d/removed.fort"));
            Assert.Equal(target, reopened.Prepare("d/Removed.fort"), ignoreCase: true); Assert.False(File.Exists(target));
            Assert.Empty(reopened.Enumerate("d/*.fort"));
            reopened.Files.Write(target, archived);
            // 사용자가 명시 저장한 새 내용은 자동 원본 재복사와 구별되고 원본은 그대로다.
            Assert.Equal(archived, reopened.Read("d/Removed.fort")); Assert.Equal(bytes, File.ReadAllBytes(source));
        }
    }

    /// <summary>이관 전의 아카이브 전용 파일은 읽을 수만 있고 목록·장부에 더해지지 않으며 직접 준비에서만 설치된다.</summary>
    [Fact]
    public void ArchiveOnlyContentIsReadWithoutListingUntilPrepared()
    {
        using var temp = new EditorTempRoot(); byte[] archived = Image(1);
        using var workspace = Open(temp, Archive("\\d\\OnlyArchive.fort", archived));
        Assert.Equal(archived, workspace.Read("d/OnlyArchive.fort")); Assert.Empty(workspace.Enumerate("d/*.fort"));
        Assert.False(File.Exists(workspace.Resolve("d/OnlyArchive.fort")));
        string prepared = workspace.Prepare("d/OnlyArchive.fort"); Assert.Equal(archived, File.ReadAllBytes(prepared));
        Assert.Single(workspace.Enumerate("d/*.fort"));
    }

    /// <summary>원본 디렉터리는 내용 복사 없이 빈 작업 디렉터리가 되고 명시 부재는 기록하지 않으며 "."·".." 후보와 형제 장부 파일은 실제 열거에 그대로 반영된다.</summary>
    [Fact]
    public void DirectoriesMissingPatternsDotsAndSiblingMetadata()
    {
        using var temp = new EditorTempRoot(); temp.Seed("d/Folder.fort/nested/unrelated", [1]);
        using var workspace = Open(temp);
        var listed = workspace.Enumerate("d/*.fort"); Assert.Single(listed);
        string folder = workspace.Files.ResolvePath("d/Folder.fort"); Assert.True(Directory.Exists(folder)); Assert.Empty(Directory.EnumerateFileSystemEntries(folder));
        Assert.Null(workspace.Read("d/Folder.fort"));
        var missing = workspace.Enumerate("d/Missing.fort"); Assert.Single(missing); Assert.Equal("Missing", Name(missing[0])); Assert.Null(workspace.Read("d/Missing.fort"));
        // 첫 명시 부재를 완료로 기록하면 나중에 생긴 실제 원본이 이관되지 못한다.
        byte[] image = Image(2); temp.Seed("d/Missing.fort", image); Assert.Equal(image, workspace.Read("d/Missing.fort"));
        var all = workspace.Enumerate("*"); bool dot = false, parent = false;
        foreach (string value in all)
        {
            string leaf = Path.GetFileName(value); dot |= leaf == "."; parent |= leaf == "..";
            Assert.DoesNotContain(leaf, new[] { "catalog-v1.dat", "catalog.lock", "work-state" });
        }
        Assert.True(dot && parent); Assert.True(File.Exists(Path.Combine(temp.Work + "-state", "catalog-v1.dat")));
        // 작업본 안의 링크가 밖을 가리키거나 형제 폴더를 절대 경로로 주면 열거·읽기·해석 모두 거부한다.
        Directory.CreateDirectory(Path.Combine(temp.Root, "outside")); EditorTempRoot.Junction(Path.Combine(temp.Work, "escape"), Path.Combine(temp.Root, "outside"));
        Assert.ThrowsAny<Exception>(() => workspace.Enumerate("escape/*.fort")); Assert.ThrowsAny<Exception>(() => workspace.Read("escape/any.fort"));
        Assert.ThrowsAny<Exception>(() => workspace.Resolve(Path.Combine(temp.Root, "work-extra", "any.fort")));
    }

    /// <summary>중단된 Pending, 손상 장부, 동시 소유자, 장부 임시 파일 열기 실패를 새 장부 성공이나 자동 재복사로 바꾸지 않는다.</summary>
    [Fact]
    public void PendingCorruptLockedAndFailedJournalAreExplicit()
    {
        byte[] bytes = Image(2);
        // 작업본이 없는 Pending은 원본을 다시 복사하지 않고 거부한다.
        using (var temp = new EditorTempRoot())
        {
            temp.Seed("d/Pending.fort", bytes); Directory.CreateDirectory(temp.Source);
            using (var ledger = new EditorCatalogLedger(new EditorWorkspacePaths(temp.Source, temp.Work))) ledger.SetState("d/Pending.fort", 1);
            using var workspace = Open(temp);
            Assert.ThrowsAny<Exception>(() => workspace.Prepare("d/Pending.fort")); Assert.False(File.Exists(Path.Combine(temp.Work, "d", "Pending.fort")));
        }
        // 작업본이 있는 Pending은 완료로 승격하고 두 번째 소유자는 잠금으로 거부한다. 지운 뒤에는 되살리지 않는다.
        using (var temp = new EditorTempRoot())
        {
            temp.Seed("d/Pending.fort", bytes); Directory.CreateDirectory(temp.Source);
            using (var seed = Open(temp)) seed.Files.Prepare("Pending", bytes);
            using (var ledger = new EditorCatalogLedger(new EditorWorkspacePaths(temp.Source, temp.Work))) ledger.SetState("d/Pending.fort", 1);
            using (var workspace = Open(temp))
            {
                Assert.Equal(bytes, workspace.Read("d/Pending.fort"));
                using var other = Open(temp); Assert.Throws<IOException>(() => other.Read("d/Pending.fort"));
                workspace.Files.Delete("d/Pending.fort");
            }
            using var again = Open(temp); Assert.Null(again.Read("d/Pending.fort"));
        }
        // 손상한 장부는 그대로 보존하고 읽기를 거부한다.
        using (var temp = new EditorTempRoot())
        {
            temp.Seed("d/Pending.fort", bytes); string state = temp.Work + "-state"; Directory.CreateDirectory(state);
            File.WriteAllBytes(Path.Combine(state, "catalog-v1.dat"), [1, 2, 3]);
            using (var workspace = Open(temp)) Assert.ThrowsAny<Exception>(() => workspace.Read("d/Pending.fort"));
            Assert.Equal(new byte[] { 1, 2, 3 }, File.ReadAllBytes(Path.Combine(state, "catalog-v1.dat")));
        }
        // 장부 임시 파일 위치가 디렉터리라 쓸 수 없으면 준비와 이후 읽기 모두 실패하고 작업본이 생기지 않는다.
        using (var temp = new EditorTempRoot())
        {
            temp.Seed("d/Pending.fort", bytes); string state = temp.Work + "-state"; Directory.CreateDirectory(Path.Combine(state, "catalog-v1.dat.pending"));
            using var workspace = Open(temp);
            Assert.ThrowsAny<Exception>(() => workspace.Prepare("d/Pending.fort")); Assert.ThrowsAny<Exception>(() => workspace.Read("d/Pending.fort"));
            Assert.False(File.Exists(Path.Combine(temp.Work, "d", "Pending.fort")));
        }
    }

    /// <summary>작업본이 원본 폴더 안이거나 원본이 작업본 안인 중첩 배치는 파일을 만들기 전에 거부한다.</summary>
    [Fact]
    public void NestedSourceAndWorkRootsAreRejected()
    {
        using var temp = new EditorTempRoot(); Directory.CreateDirectory(temp.Source);
        var files = new GameFileSystem(temp.Source, []);
        Assert.ThrowsAny<Exception>(() => new EditorCatalogWorkspace(files, Path.Combine(temp.Source, "work")));
        Assert.ThrowsAny<Exception>(() => new EditorCatalogWorkspace(files, temp.Root));
        Assert.False(Directory.Exists(Path.Combine(temp.Source, "work")));
    }

    /// <summary>
    /// 분류 파일을 읽은 뒤 교체한 현재 작업본을 별도 메타 Template이 다시 읽어야 한다(cpppj <c>ClassificationAndMetadataStillRereadCurrentWork</c>).
    /// 분류 읽기의 TYPE 개수 조회 시점에 작업본을 다른 종류로 바꿔, 목록 행이 두 번째 읽기의 값을 쓰고 원본 파일은 변하지 않음을 확인한다. (2026-11-02 TS6)
    /// </summary>
    [Fact]
    public void ClassificationAndMetadataStillRereadCurrentWork()
    {
        using var temp = new EditorTempRoot();
        byte[] before = Image(2), after = Image(1, 1);
        string sourceFile = temp.Seed("d/Current.fort", before);
        using var workspace = Open(temp);
        var config = new ConfigInterface(null); config.Startup(new()); config.Set("fortSpec", "d/{local.1}.fort");
        int loads = 0;
        var catalog = new SoloMapCatalog(new GameFileSystem(temp.Source, []), config, new SoloMapCatalogEffects
        {
            TypeCount = () => { if (++loads == 1) workspace.Files.Write(workspace.Resolve("d/Current.fort"), after); return 188; },
            AllocationBytes = _ => throw new InvalidOperationException("정상 목록의 예상 밖 빈 State"),
            UserName = () => "Me", Translate = text => text, EnumeratePaths = workspace.Enumerate, ReadFile = workspace.Read
        });
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(2, loads);
        Assert.Equal(2u, catalog.State.Flags[0]);
        Assert.Equal("Current", catalog.State.Names[0]);
        Assert.Equal(before, File.ReadAllBytes(sourceFile));
    }

    /// <summary>완료 이관 뒤 사용자가 지운 항목만 삭제 표식으로 구별한다.</summary>
    /// <remarks>2026-11-11 0.355에서 `Deleted` 재대조와 함께 추가했다.</remarks>
    [Fact]
    public void DeletedDistinguishesUserRemovedCompletedEntry()
    {
        using var temp = new EditorTempRoot();
        temp.Seed("d/Gone.fort", Image(2));
        using (var workspace = Open(temp))
        {
            // 이관 전과 작업본이 살아 있을 때는 삭제 표식이 아니다
            Assert.False(workspace.Deleted("d/Gone.fort"));
            Assert.NotNull(workspace.Read("d/Gone.fort"));
            Assert.False(workspace.Deleted("d/Gone.fort"));
            // 완료 이관 뒤 지우면 삭제 표식이다
            workspace.Files.Delete("d/Gone.fort");
            Assert.True(workspace.Deleted("d/Gone.fort"));
        }
        // Pending(미완료)은 지워도 삭제 표식이 아니다
        using (var ledger = new EditorCatalogLedger(new EditorWorkspacePaths(temp.Source, temp.Work))) ledger.SetState("d/Gone.fort", 1);
        using var again = Open(temp);
        Assert.False(again.Deleted("d/Gone.fort"));
    }
}
