using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 편집 작업 파일 장치(<see cref="EditorFiles"/>)와 현재 Fort 파일 장치(<see cref="EditorFortFileDevices"/>)를 유니코드 임시 작업본에서 검사한다.
/// cpppj <c>EditorFileWriterTests</c>·<c>EditorFilesTests</c>·<c>EditorFortFileDevicesTests</c>·<c>EditorAtomicWriteTests</c> 중
/// Template 저장소가 필요 없는 항목을 옮겼다(2026-11-01 0.127). 원본 x86 fixture가 없는 직역 검사이므로 L1이 아니다.
/// </summary>
public sealed class EditorFilesDeviceTests
{
    /// <summary>한글과 이모지가 든 보호 원본 폴더 경로다. 만들지는 않는다.</summary>
    private static string Protected(EditorTempRoot temp) => Path.Combine(temp.Root, "보호 자산");

    /// <summary>한글과 이모지가 든 작업본 폴더 경로다. 만들지는 않는다.</summary>
    private static string WorkFolder(EditorTempRoot temp) => Path.Combine(temp.Root, "편집 작업본 😀");

    /// <summary>보호 원본을 하나 둔 작업 파일 장치를 만든다. 생성만으로는 파일을 만들지 않는다.</summary>
    private static EditorFiles Open(EditorTempRoot temp) => new(WorkFolder(temp), [Protected(temp)]);

    /// <summary>표식으로 구별되는 64바이트 가짜 Fort 이미지를 만든다.</summary>
    private static byte[] Image(byte marker = 7) => Enumerable.Range(0, 64).Select(index => (byte)(index + marker)).ToArray();

    /// <summary>현재 Fort 이름·키·선택 횟수·진단 번호를 한곳에서 바꾸며 효과 묶음으로 내보낸다. 제품의 현재 Fort 소유자라고 주장하지 않는다.</summary>
    private sealed class CurrentFort
    {
        /// <summary>현재 NUL 제외 Fort 이름이다.</summary>
        public string Name = "";
        /// <summary>선택한 FileSpec의 키다.</summary>
        public string Key = "fortSpec";
        /// <summary>FileSpec 선택 효과가 실행된 횟수다.</summary>
        public int Selects;
        /// <summary>받은 진단 번호들이다.</summary>
        public readonly List<uint> Assertions = [];
        /// <summary>이 상태를 읽는 효과 묶음을 만든다.</summary>
        public EditorFortPathEffects Effects() => new()
        {
            CurrentName = () => Name, SelectFileSpec = () => Selects++, FileSpecKey = () => Key, Assertion = line => Assertions.Add(line)
        };
    }

    /// <summary>fortSpec·SOLOSpec 경로 지정을 가진 Config를 만든다.</summary>
    private static ConfigInterface Config()
    {
        var config = new ConfigInterface(null); config.Startup(new());
        config.Set("fortSpec", "d/{local.1}.fort"); config.Set("SOLOSpec", "maps/{local.1}.fort"); return config;
    }

    /// <summary>작업 파일이 쓰기 금지가 아닌 일반 속성이 되게 하고 읽기 전용이면 푼다.</summary>
    private static void Writable(string path) => File.SetAttributes(path, FileAttributes.Normal);

    // ---- Template 저장 쓰기 (cpppj EditorFileWriterTests) ----

    /// <summary>Template 저장은 기존 파일 끝을 보존하고 없는 파일만 만들며, 일반 Write는 계속 잘라쓴다.</summary>
    [Fact]
    public void TemplateWriteKeepsExistingTailCreatesMissingAndGeneralWriteTruncates()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp);
        byte[] initial = Enumerable.Repeat((byte)0x5a, 37).ToArray(), prefix = [1, 0, 255];
        string path = files.Prepare("기존 맵 😀", initial); var writer = files.Writer();
        writer.Permissions(path, 0x180); writer.Write(path, prefix);
        byte[] expected = (byte[])initial.Clone(); prefix.CopyTo(expected, 0); Assert.Equal(expected, File.ReadAllBytes(path));
        // 빈 쓰기도 기존 파일의 끝을 자르지 않는다.
        writer.Write(path, []); Assert.Equal(expected, File.ReadAllBytes(path));
        byte[] longer = Enumerable.Repeat((byte)0x73, 53).ToArray(); writer.Write(path, longer); Assert.Equal(longer, File.ReadAllBytes(path));
        string created = files.ResolvePath("d/새 맵 😀.fort"); Assert.False(File.Exists(created));
        writer.Write(created, prefix); Assert.Equal(prefix, File.ReadAllBytes(created));
        string empty = files.ResolvePath("d/empty.fort"); writer.Write(empty, []); Assert.True(File.Exists(empty)); Assert.Empty(File.ReadAllBytes(empty));
        // 일반 복사·준비 입력의 기존 잘라쓰기 계약은 그대로다.
        files.Write(path, new byte[] { 9 }); Assert.Equal(new byte[] { 9 }, File.ReadAllBytes(path));
    }

    /// <summary>보호 경로·작업본 밖·없는 부모의 쓰기는 파일을 바꾸거나 만들기 전에 거부된다.</summary>
    [Fact]
    public void TemplateWriteRejectsProtectedOutsideAndAbsentParentBeforeMutation()
    {
        using var temp = new EditorTempRoot(); byte[] original = [3, 1, 4, 1];
        string kept = Path.Combine(Protected(temp), "d", "kept.fort"); Directory.CreateDirectory(Path.GetDirectoryName(kept)!); File.WriteAllBytes(kept, original);
        var files = Open(temp); files.EnsureDirectory("d"); var writer = files.Writer();
        Assert.ThrowsAny<Exception>(() => writer.Permissions(kept, 0x180)); Assert.ThrowsAny<Exception>(() => writer.Write(kept, []));
        Assert.Equal(original, File.ReadAllBytes(kept));
        string outside = Path.Combine(temp.Root, "outside.fort"); Assert.ThrowsAny<Exception>(() => writer.Write(outside, original)); Assert.False(File.Exists(outside));
        string missing = files.ResolvePath("absent/child.fort"); Assert.ThrowsAny<Exception>(() => writer.Write(missing, original));
        Assert.False(Directory.Exists(Path.GetDirectoryName(missing)));
    }

    /// <summary>쓰기를 공유하지 않는 열린 핸들 때문에 두 열기가 모두 실패하면 기존 바이트를 자르지 않고 오류를 낸다.</summary>
    [Fact]
    public void TemplateWriteSharingViolationPreservesExistingBytes()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var temp = new EditorTempRoot(); var files = Open(temp); byte[] original = [2, 7, 1, 8, 2];
        string path = files.Prepare("잠긴 맵 😀", original);
        using (new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            Assert.ThrowsAny<Exception>(() => files.Writer().Write(path, []));
        Assert.Equal(original, File.ReadAllBytes(path));
    }

    // ---- 존재·readonly·권한 (cpppj EditorFilesTests) ----

    /// <summary>존재는 열 수 있는 파일만 5이고, readonly는 실제 속성을 읽으며, 권한 변경은 속성만 풀고 모드 0x180만 받는다.</summary>
    [Fact]
    public void ExistsReadOnlyAndMakeWritableUseActualAttributes()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); string path = files.Prepare("Map", Image());
        Assert.Equal(5, files.Exists(path)); Assert.Equal(0, files.Exists("d/missing.fort")); Assert.Equal(0, files.Exists("d"));
        Assert.False(files.ReadOnly(path));
        File.SetAttributes(path, FileAttributes.ReadOnly); Assert.True(files.ReadOnly(path)); Assert.Equal(5, files.Exists(path));
        Assert.ThrowsAny<Exception>(() => files.Write(path, [1]));
        files.MakeWritable(path, 0x180); Assert.False(files.ReadOnly(path)); files.Write(path, [1]); Assert.Equal(new byte[] { 1 }, File.ReadAllBytes(path));
        // chmod 실패는 쓰기를 중단시키지 않는다. 없는 파일도 오류가 아니다.
        files.MakeWritable("d/missing.fort", 0x180);
        Assert.Throws<ArgumentException>(() => files.MakeWritable(path, 0));
    }

    /// <summary>readonly 조회는 빈 이름(0x480)과 stat 실패(0x48e)의 원문 진단 뒤 멈추고, 진단 장치가 없으면 연결 누락으로 멈춘다.</summary>
    [Fact]
    public void ReadOnlyEmitsOriginalDiagnosticsThenStopsExplicitly()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); var lines = new List<uint>();
        Assert.Throws<ArgumentException>(() => files.ReadOnly("", lines.Add));
        Assert.Throws<FileNotFoundException>(() => files.ReadOnly("d/none.fort", lines.Add));
        Assert.Equal([0x480u, 0x48eu], lines);
        Assert.Throws<InvalidOperationException>(() => files.ReadOnly("")); Assert.Throws<InvalidOperationException>(() => files.ReadOnly("d/none.fort"));
    }

    /// <summary>생성자 보호·Prepare 이름 계약·보호/작업본 밖 쓰기와 권한 변경 거부를 확인하고 기존 사용자 작업본을 원본으로 되돌리지 않는다.</summary>
    [Fact]
    public void RejectsProtectedPathsAndKeepsExistingWork()
    {
        using var temp = new EditorTempRoot(); byte[] original = Image(); string source = Protected(temp);
        string protectedFile = Path.Combine(source, "d", "TEST01.fort"); Directory.CreateDirectory(Path.GetDirectoryName(protectedFile)!); File.WriteAllBytes(protectedFile, original);
        var files = Open(temp);
        Assert.ThrowsAny<Exception>(() => new EditorFiles(Path.Combine(source, "forbidden"), [source])); Assert.ThrowsAny<Exception>(() => new EditorFiles(WorkFolder(temp), []));
        // 원본 pending 이름과 물리 파일 경로 계약을 확인한다.
        foreach (string name in new[] { "", "../outside", "with.dot", "x/y", "x\\y", "x:y" }) Assert.ThrowsAny<Exception>(() => files.Prepare(name, original));
        Assert.ThrowsAny<Exception>(() => files.Write(protectedFile, [])); Assert.ThrowsAny<Exception>(() => files.MakeWritable(protectedFile, 0x180));
        Assert.ThrowsAny<Exception>(() => files.Write("../outside.fort", [])); Assert.ThrowsAny<Exception>(() => files.Exists(protectedFile));
        Assert.ThrowsAny<Exception>(() => files.ReadOnly(protectedFile)); Assert.ThrowsAny<Exception>(() => files.Delete(protectedFile));
        string path = files.Prepare("valid", original); files.Write(path, new byte[] { 1, 2, 3 });
        Assert.Equal(path, files.Prepare("valid", original)); Assert.Equal(new byte[] { 1, 2, 3 }, File.ReadAllBytes(path));
        Assert.Equal(original, File.ReadAllBytes(protectedFile));
    }

    /// <summary>
    /// 끝 구분자·<c>.</c>·<c>..</c> 표기가 다른 작업본을 만들지 않고 모든 표기에서 수정한 같은 파일을 다시 선택하며,
    /// 정규화가 원본 보호를 우회하지 않고 이름 없는 루트는 형제 작업본으로 바꾸지 않는다. cpppj <c>EditorWorkingRoot</c> 검사 2개를 <see cref="EditorWorkspacePaths"/>로 옮겼다.
    /// </summary>
    [Fact]
    public void WorkingRootNormalizesDirectorySpellingsAndStillRejectsProtectedAndNamelessRoots()
    {
        using var temp = new EditorTempRoot(); string source = Path.Combine(temp.Root, "assets"); Directory.CreateDirectory(Path.Combine(source, "d"));
        string expected = Path.Combine(temp.Root, "assets-editor"); byte[] original = [1, 2, 3], edited = [4, 5];
        // 같은 실제 디렉터리를 가리키는 일반·끝 구분자·상대 요소 표기다.
        var spellings = new List<string> { source, source + "/", Path.Combine(source, "."), Path.Combine(source, "d", ".."), Path.Combine(source, "d", "..") + Path.DirectorySeparatorChar };
        if (OperatingSystem.IsWindows()) spellings.Add(source + "\\");
        var first = new EditorFiles(new EditorWorkspacePaths(source).WorkRoot, [source]); string saved = first.Prepare("Working", original); first.Write(saved, edited);
        // 모든 재진입 표기가 같은 작업 namespace와 기존 수정본을 사용해야 한다.
        foreach (string spelling in spellings)
        {
            var paths = new EditorWorkspacePaths(spelling); Assert.Equal(expected, paths.WorkRoot, ignoreCase: true);
            var again = new EditorFiles(paths.WorkRoot, [spelling]);
            Assert.Equal(saved, again.Prepare("Working", original), ignoreCase: true); Assert.Equal(edited, File.ReadAllBytes(saved));
            Assert.ThrowsAny<Exception>(() => again.Write(Path.Combine(source, "d", "Working.fort"), original));
        }
        Assert.False(File.Exists(Path.Combine(source, "d", "Working.fort"))); Assert.False(Directory.Exists(Path.Combine(source, "-editor")));
        // 작업본이 보호 루트 안에 놓이는 배치와 이름 없는 파일 시스템 루트는 거부한다.
        Assert.ThrowsAny<Exception>(() => new EditorFiles(expected, [temp.Root])); Assert.ThrowsAny<Exception>(() => new EditorFiles(Path.Combine(source, "..", "assets", "-editor"), [source]));
        Assert.ThrowsAny<Exception>(() => new EditorWorkspacePaths(Path.GetPathRoot(source)!));
    }

    // ---- 원자 쓰기·형제 metadata 장치 (cpppj EditorAtomicWriteTests) ----

    /// <summary>같은 파일을 읽는 외부 reader가 잠깐 교체를 막아도 같은 원자 교체가 성공하고 임시 파일이 남지 않는다.</summary>
    [Fact]
    public void AtomicWriteRecoversFromTransientExternalReader()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var temp = new EditorTempRoot(); var files = new EditorFiles(Path.Combine(temp.Root, "state"), [Path.Combine(temp.Root, "protected")]);
        byte[] old = [1, 2, 3], next = [4, 5, 6, 7];
        files.WriteAtomic("catalog-v1.dat", old); string target = files.ResolvePath("catalog-v1.dat");
        // 읽기·쓰기는 공유하되 삭제·교체는 공유하지 않는 외부 reader를 잠깐 둔다.
        var blocked = new FileStream(target, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        var release = new Thread(() => { Thread.Sleep(120); blocked.Dispose(); }); release.Start();
        files.WriteAtomic("catalog-v1.dat", next); release.Join();
        Assert.Equal(next, File.ReadAllBytes(target)); Assert.False(File.Exists(files.ResolvePath("catalog-v1.dat.pending")));
    }

    /// <summary>계속 잠긴 대상은 제한 시간 뒤 실패하며 이전 파일과 새 임시 바이트를 모두 남기고, 잠금이 풀리면 새 요청이 교체한다.</summary>
    [Fact]
    public void AtomicWritePersistentExternalReaderPreservesOldAndPendingBytes()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var temp = new EditorTempRoot(); var files = new EditorFiles(Path.Combine(temp.Root, "state"), [Path.Combine(temp.Root, "protected")]);
        byte[] old = [11, 12, 13], next = [14, 15, 16, 17];
        files.WriteAtomic("catalog-v1.dat", old); string target = files.ResolvePath("catalog-v1.dat"); string pending = files.ResolvePath("catalog-v1.dat.pending");
        var blocked = new FileStream(target, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        try { Assert.Throws<IOException>(() => files.WriteAtomic("catalog-v1.dat", next)); }
        finally { blocked.Dispose(); }
        Assert.Equal(old, File.ReadAllBytes(target)); Assert.Equal(next, File.ReadAllBytes(pending));
        files.WriteAtomic("catalog-v1.dat", next); Assert.Equal(next, File.ReadAllBytes(target)); Assert.False(File.Exists(pending));
    }

    /// <summary>형제 metadata 장치는 작업본과 원본을 모두 보호하고, 이름 범위와 외부 링크 경계를 거부하며, 만들기만으로 폴더를 만들지 않는다.</summary>
    [Fact]
    public void SiblingNamespaceProtectsWorkAndSourceAndRejectsBadNamesAndLinks()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); string work = WorkFolder(temp);
        var sibling = files.SiblingNamespace("-state");
        Assert.Equal(work + "-state", sibling.RootPath); Assert.False(Directory.Exists(sibling.RootPath));
        sibling.WriteAtomic("catalog-v1.dat", [1]); Assert.Equal(new byte[] { 1 }, File.ReadAllBytes(Path.Combine(sibling.RootPath, "catalog-v1.dat")));
        Assert.ThrowsAny<Exception>(() => sibling.Write(Path.Combine(work, "d", "a.fort"), [])); Assert.ThrowsAny<Exception>(() => sibling.Write(Path.Combine(Protected(temp), "a.fort"), []));
        foreach (string suffix in new[] { "", ".x", "a/b", "a\\b", "a:b", "a*b", "a\0b" }) Assert.ThrowsAny<Exception>(() => files.SiblingNamespace(suffix));
        // 형제 이름이 링크로 다른 위치에 매핑되면 거부한다.
        string outside = Path.Combine(temp.Root, "outside"); Directory.CreateDirectory(outside);
        var linked = new EditorFiles(Path.Combine(temp.Root, "linkwork"), [Protected(temp)]);
        EditorTempRoot.Junction(Path.Combine(temp.Root, "linkwork-metadata"), outside);
        Assert.ThrowsAny<Exception>(() => linked.SiblingNamespace("-metadata"));
    }

    /// <summary>장부 갱신이 일시적으로 막혀도 원자 교체의 재시도를 거쳐 성공하고 두 항목이 모두 영속화된다.</summary>
    [Fact]
    public void LedgerUpdateWaitsForTransientExternalReader()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var temp = new EditorTempRoot(); Directory.CreateDirectory(temp.Source); var paths = new EditorWorkspacePaths(temp.Source, temp.Work);
        string ledgerFile = Path.Combine(temp.Work + "-state", "catalog-v1.dat");
        using (var ledger = new EditorCatalogLedger(paths))
        {
            ledger.SetState("d/A.fort", 2);
            var blocked = new FileStream(ledgerFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            var release = new Thread(() => { Thread.Sleep(120); blocked.Dispose(); }); release.Start();
            ledger.SetState("d/B.fort", 2); release.Join();
        }
        using var restored = new EditorCatalogLedger(paths); Assert.Equal(2u, restored.State("d/A.fort")); Assert.Equal(2u, restored.State("d/B.fort"));
        Assert.False(File.Exists(ledgerFile + ".pending"));
    }

    // ---- 현재 Fort 파일 장치 (cpppj EditorFortFileDevicesTests) ----

    /// <summary>현재 경로는 설정과 현재 이름·키를 호출마다 조회하고 작업본 정규 경로를 돌려주되 파일이나 폴더를 만들지 않는다.</summary>
    [Fact]
    public void CurrentPathReusesConfigAndCurrentFileSpecWithoutCreatingFiles()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); var fort = new CurrentFort { Name = "One" };
        var devices = new EditorFortFileDevices(files, new GameFileSystem(temp.Source, []), Config(), fort.Effects());
        string first = devices.CurrentPath(); Assert.Equal(files.ResolvePath("d/one.fort"), first); Assert.Equal(0, devices.Exists(first)); Assert.False(Directory.Exists(WorkFolder(temp)));
        fort.Name = "Two"; fort.Key = "SOLOSpec"; string second = devices.CurrentPath();
        Assert.Equal(files.ResolvePath("maps/two.fort"), second); Assert.NotEqual(first, second); Assert.Equal(2, fort.Selects); Assert.Empty(fort.Assertions);
        // 원본이나 현재 프로세스의 같은 이름으로 fallback하지 않는다.
        Assert.Empty(devices.Read(second));
    }

    /// <summary>준비는 없는 작업본만 원본에서 복사하고, 그 뒤 수정본·수정 시각은 원본을 다시 읽지 않고 그대로 쓴다. Load 경계에는 읽기·권한·이름 비교가 연결된다.</summary>
    [Fact]
    public void PreparedEditedBytesWinOverOriginalAndBindLoadFillsDeviceHooks()
    {
        using var temp = new EditorTempRoot(); byte[] original = Image(7), edited = Image(29); temp.Seed("d/Map.fort", original);
        var files = Open(temp); var source = new GameFileSystem(temp.Source, []); var fort = new CurrentFort { Name = "Map" };
        var devices = new EditorFortFileDevices(files, source, Config(), fort.Effects());
        string path = devices.PrepareCurrent(); Assert.Equal(5, devices.Exists(path)); Assert.Equal(original, devices.Read(path));
        files.Write(path, edited); DateTime time = File.GetLastWriteTimeUtc(path);
        Assert.Equal(path, devices.PrepareCurrent()); Assert.Equal(edited, devices.Read(path)); Assert.Equal(time, File.GetLastWriteTimeUtc(path)); Assert.Equal(original, source.TryReadAllBytes("d/Map.fort"));
        // 호출자가 공급한 나머지 항목은 그대로 두고 장치가 채우는 세 항목만 연결한다.
        var bound = devices.BindLoad(new TemplateFileLoadHooks { TypeCount = () => 188, CurrentSlot = () => 1 });
        Assert.Equal(188, bound.TypeCount!()); Assert.Equal(1u, bound.CurrentSlot!()); Assert.Equal(edited, bound.Read!(path)); Assert.False(bound.ReadOnly!(path));
        Assert.True(bound.SameName!("Map.FORT", "map.fort")); Assert.False(bound.SameName!("map.fort", "other.fort"));
        // CRT stricmp처럼 ASCII만 접고 첫 NUL 뒤는 보지 않는다.
        Assert.False(bound.SameName!("É", "é")); Assert.True(bound.SameName!("ab\0x", "AB\0y"));
    }

    /// <summary>실제 readonly 속성의 판정은 Config의 readOnlyForts 값과 독립이다.</summary>
    [Fact]
    public void ActualReadOnlyAttributeIsSeparateFromConfigGate()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); string path = files.Prepare("Map", Image()); var config = Config();
        var devices = new EditorFortFileDevices(files, new GameFileSystem(temp.Source, []), config, new EditorFortPathEffects());
        File.SetAttributes(path, FileAttributes.ReadOnly);
        config.SetInt("readOnlyForts", 0); Assert.True(devices.ReadOnly(path));
        config.SetInt("readOnlyForts", 1); Assert.True(devices.ReadOnly(path)); Assert.Equal(5, devices.Exists(path)); Assert.NotEmpty(devices.Read(path));
        Writable(path); Assert.False(devices.ReadOnly(path)); Assert.Equal(1, config.GetInt("readOnlyForts"));
    }

    /// <summary>작업 파일이 없으면 읽기는 원본으로 fallback하지 않고, stat 실패는 진단 뒤 명시 오류이며, 준비를 명시했을 때만 복사한다.</summary>
    [Fact]
    public void MissingReadAndStatFailureRemainExplicit()
    {
        using var temp = new EditorTempRoot(); byte[] image = Image(); temp.Seed("d/Map.fort", image); var files = Open(temp); var fort = new CurrentFort { Name = "Map" };
        var devices = new EditorFortFileDevices(files, new GameFileSystem(temp.Source, []), Config(), fort.Effects()); string path = devices.CurrentPath();
        Assert.Equal(0, devices.Exists(path)); Assert.Empty(devices.Read(path)); Assert.ThrowsAny<Exception>(() => devices.ReadOnly(path)); Assert.Equal([0x48eu], fort.Assertions);
        Assert.Equal(path, devices.PrepareCurrent()); Assert.Equal(5, devices.Exists(path)); Assert.Equal(image, devices.Read(path));
        // 원본에도 없는 이름은 준비해도 파일이 생기지 않는다.
        fort.Name = "Absent"; string absent = devices.PrepareCurrent(); Assert.Equal(0, devices.Exists(absent)); Assert.Empty(devices.Read(absent));
    }

    /// <summary>빈 현재 이름 진단이 이름을 고치면 고친 이름을 쓰고, FileSpec 선택 효과 뒤 바뀐 키를 반영하며, 효과를 읽기 전에는 요구하지 않는다.</summary>
    [Fact]
    public void CurrentNameDiagnosticAndSelectorAreReachedLazily()
    {
        using var temp = new EditorTempRoot(); var files = Open(temp); var source = new GameFileSystem(temp.Source, []); var config = Config();
        var fort = new CurrentFort(); var lines = new List<uint>();
        var effects = new EditorFortPathEffects
        {
            CurrentName = () => fort.Name, Assertion = line => { lines.Add(line); fort.Name = "Repaired"; },
            SelectFileSpec = () => fort.Key = "SOLOSpec", FileSpecKey = () => fort.Key
        };
        var devices = new EditorFortFileDevices(files, source, config, effects);
        Assert.Equal(files.ResolvePath("maps/repaired.fort"), devices.CurrentPath()); Assert.Equal([39u], lines);
        // 현재 이름 효과가 없어도 읽기는 가능하지만 경로 계산은 연결 누락으로 멈춘다.
        var missing = new EditorFortFileDevices(files, source, config, new EditorFortPathEffects());
        Assert.Empty(missing.Read(files.ResolvePath("unprepared.fort"))); Assert.Throws<InvalidOperationException>(() => missing.CurrentPath());
    }

    /// <summary>준비·복사 효과를 공급하면 장치가 기본 복사를 하지 않고 그 효과에 맡기며, 해석 효과는 작업 경로 선택을 대신한다.</summary>
    [Fact]
    public void PreparationAndResolutionEffectsReplaceDefaultCopyAndMapping()
    {
        using var temp = new EditorTempRoot(); byte[] image = Image(); temp.Seed("d/Map.fort", image); var files = Open(temp); var fort = new CurrentFort { Name = "Map" };
        var prepared = new List<string>(); string mapped = files.ResolvePath("mapped/chosen.fort");
        var effects = fort.Effects() with { Prepare = prepared.Add, Resolve = _ => mapped };
        var devices = new EditorFortFileDevices(files, new GameFileSystem(temp.Source, []), Config(), effects);
        Assert.Equal(mapped, devices.PrepareCurrent()); Assert.Equal(["d/Map.fort"], prepared);
        // 부모 폴더만 준비되고 파일 복사는 효과의 책임이다.
        Assert.True(Directory.Exists(Path.GetDirectoryName(mapped))); Assert.False(File.Exists(mapped));
    }

    /// <summary>Config 경로와 실제 장치 접근 모두 같은 작업 루트·보호 경로 검사를 거쳐 어떤 쓰기도 하기 전에 거부된다.</summary>
    [Fact]
    public void RejectsProtectedAndEscapedPathsBeforeAnyWrite()
    {
        using var temp = new EditorTempRoot(); byte[] bytes = Image(); string sourceFile = temp.Seed("d/Map.fort", bytes); var files = Open(temp);
        var config = Config(); var fort = new CurrentFort { Name = "Map" };
        var devices = new EditorFortFileDevices(files, new GameFileSystem(temp.Source, []), config, fort.Effects());
        config.Set("fortSpec", "../escape/{local.1}.fort"); Assert.ThrowsAny<Exception>(() => devices.PrepareCurrent());
        config.Set("fortSpec", sourceFile); Assert.ThrowsAny<Exception>(() => devices.PrepareCurrent());
        Assert.ThrowsAny<Exception>(() => files.PreparePath("../escape.fort", bytes)); Assert.ThrowsAny<Exception>(() => devices.Read(sourceFile));
        Assert.Equal(bytes, File.ReadAllBytes(sourceFile)); Assert.False(Directory.Exists(WorkFolder(temp)));
    }

    /// <summary>기존 파일 시스템이 게임 폴더 기준으로 읽는 선행 역슬래시와 대소문자 표기를 같은 정규화로 해석한다.</summary>
    [Fact]
    public void RootedGamePathReusesBaseFileNormalization()
    {
        using var temp = new EditorTempRoot(); byte[] bytes = Image(19); temp.Seed("D/Mixed.fort", bytes); var files = Open(temp); var config = Config(); config.Set("fortSpec", "\\D\\{local.1}.fort");
        var source = new GameFileSystem(temp.Source, []); var fort = new CurrentFort { Name = "Mixed" };
        var devices = new EditorFortFileDevices(files, source, config, fort.Effects());
        string path = devices.PrepareCurrent(); Assert.Equal(files.ResolvePath("d/mixed.fort"), path); Assert.Equal(5, devices.Exists(path));
        Assert.Equal(bytes, devices.Read(path)); Assert.Equal(bytes, source.TryReadAllBytes("\\D\\Mixed.fort"));
    }
}
