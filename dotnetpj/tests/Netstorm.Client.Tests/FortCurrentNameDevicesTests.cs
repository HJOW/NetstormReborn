using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// 실제 Client 이름 버퍼·목록 dirty, Template manager, Notify 파일 저장, 작업본 삭제를 결합한 현재 Fort 이름 장치
/// (<see cref="FortCurrentNameDevices"/>)를 검사한다. cpppj <c>FortCurrentNameDevicesTests</c> 7개를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. cpppj의 장면은 <c>Client</c> 전체를 만들지만 C#에는 아직 없어(F3) 장면이 <see cref="ClientFortGlobals"/>·설정·저장소·파일 장치를
/// 직접 조립한다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다(이름 부모 <see cref="RawFortCurrentName"/>의 L1은 Core 검사 876관측).
/// 보호 원본은 임시 폴더의 복사본이며, 실제 <c>originals</c>에는 쓰지 않는다. cpppj의 <c>FortTemplate::Parse</c>는 F1이라 <see cref="FortTemplate.ReadSections"/>를 쓴다.
/// </remarks>
public sealed class FortCurrentNameDevicesTests
{
    /// <summary>NUL 포함 안정된 UTF-8 바이트를 실제 원문 부모에 전달한다.</summary>
    /// <param name="value">이름.</param>
    private static byte[] NameInput(string value) => [.. Encoding.UTF8.GetBytes(value), 0];

    /// <summary>
    /// 이름 부모의 외부 요청(전체 snapshot·제목·진단)은 관측 장치로 구별하고 manager·Notify·파일은 실제 본체를 쓰는 검사 장면이다.
    /// 임시 루트(보호 원본 폴더와 작업본 폴더)를 소유하고 끝나면 지운다.
    /// </summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>검사 전용 임시 루트다.</summary>
        public readonly EditorTempRoot Temp = new();

        /// <summary>실제 Client와 같은 이름·목록·파일 명세 소유자다.</summary>
        public readonly ClientFortGlobals Names = new();

        /// <summary>현재 manager 저장소다.</summary>
        public readonly CampaignTemplateStorage Storage = new();

        /// <summary>fortSpec·SOLOSpec·mayEditMissions를 가진 설정이다.</summary>
        public readonly ConfigInterface Config = new(null);

        /// <summary>쓰기·삭제가 모두 쓰는 작업 이름공간이다.</summary>
        public readonly EditorFiles Files;

        /// <summary>같은 manager 이름을 실제 Notify가 다시 읽는 Player State getter다.</summary>
        public readonly RawPlayerTemplateState State;

        /// <summary>현재 Template 소유자다.</summary>
        public uint Owner = 1;

        /// <summary>현재 Fort 모드다.</summary>
        public uint Fort = 1;

        /// <summary>현재 Editor 모드다.</summary>
        public uint Editor = 1;

        /// <summary>현재 Mission 모드다.</summary>
        public uint Mission = 0;

        /// <summary>저장 억제 DWORD(readOnlyForts)다.</summary>
        public uint ReadOnly = 0;

        /// <summary>실제 저장 부모가 호출된 횟수다.</summary>
        public int SaveCount;

        /// <summary>Notify가 호출된 횟수다.</summary>
        public int NotifyCount;

        /// <summary>제목 갱신 횟수다.</summary>
        public int Titles;

        /// <summary>미션 중 여부(State getter 입력)다.</summary>
        private readonly bool _inMission = false;

        /// <summary>로딩 깊이(State getter 입력)다.</summary>
        private uint _depth;

        /// <summary>9칸 숨김 표다.</summary>
        private readonly byte[] _hidden = new byte[9];

        /// <summary>전체 snapshot 요청 당시의 현재 이름들이다.</summary>
        public readonly List<string> Snapshots = [];

        /// <summary>원문 진단 번호들이다.</summary>
        public readonly List<uint> Assertions = [];

        /// <summary>보호 원본 폴더다(복사본이며 만들 때마다 한글·이모지가 든다).</summary>
        public string ProtectedFolder => Path.Combine(Temp.Root, "보호 원본 😀");

        /// <summary>작업본 폴더다.</summary>
        public string WorkFolder => Path.Combine(Temp.Root, "이름 작업본 😀");

        /// <summary>실제 Config/State/저장 장치만 연결하며 아직 파일/manager는 만들지 않는다.</summary>
        public Scene()
        {
            Config.Startup(new());
            Config.Set("fortSpec", "d/{local.1}.fort");
            Config.Set("SOLOSpec", "d/{local.1}.solo");
            Config.SetInt("mayEditMissions", 0);
            Files = new EditorFiles(WorkFolder, [ProtectedFolder]);
            State = new RawPlayerTemplateState(
                OriginalEdition.Patch1078,
                Storage.Headers,
                slot => Storage.SelectState(slot),
                slot => Storage.Name(slot),
                () => _depth,
                value => _depth = value,
                () => _inMission,
                _ => throw new InvalidOperationException("이름 결합의 예상 밖 State 진단"),
                _ => throw new InvalidOperationException("PATCH 이름 결합의 CD 경계"));
            State.BindMinimumSize((slot, size) => Storage.EnsureStateSize(slot, size));
            State.BindFileSave(slot =>
            {
                SaveCount++;
                Storage.SaveFile(slot,
                    new DataManagerFileSaveState { ReadOnlyForts = () => ReadOnly, LocalOwner = () => Owner, HiddenPlayers = () => _hidden },
                    new FortImageHooks { TypeCount = () => 188, Assertion = _ => throw new InvalidOperationException("이름 저장의 이미지 진단") },
                    Files.Writer());
            });
        }

        /// <summary>이름 부모의 현재 getter는 호출마다 같은 명시 모드/소유자를 읽고 dirty는 목록과 같은 DWORD에 쓴다.</summary>
        public FortCurrentNameGlobals Globals() => new()
        {
            Owner = () => Owner,
            Fort = () => Fort,
            Editor = () => Editor,
            Mission = () => Mission,
            Dirty = () => Names.Catalog.Dirty,
            SetDirty = value => Names.Catalog.Dirty = value,
        };

        /// <summary>snapshot 관측은 게임 전체 저장의 증거가 아니며 Notify는 실제 부모/파일 writer를 호출한다.</summary>
        public FortCurrentNameHooks Hooks() => new()
        {
            Snapshot = () => Snapshots.Add(Names.Current()),
            Notify = slot =>
            {
                NotifyCount++;
                State.Changed(slot);
            },
            RefreshTitle = () => Titles++,
            Assertion = line => Assertions.Add(line),
            State = slot => State.Get(slot),
        };

        /// <summary>현재 이름 버퍼의 명시 입력만 준비한다. Snapshot/dirty/설정을 선행 실행하지 않는다.</summary>
        /// <param name="value">현재 이름.</param>
        public void SetCurrent(string value) => FortSelection.SetPendingFortName(Names.CurrentBytes(), NameInput(value));

        /// <summary>실제 TEST01 작업본과 같은 manager/State를 설치하고 빌린 슬롯의 같은 이름도 관측한다.</summary>
        /// <param name="name">작업본 이름.</param>
        public string Install(string name)
        {
            byte[] bytes = RealFortBytes("TEST01");
            string path = Files.Prepare(name, bytes);
            Storage.InstallFile(Owner, FortTemplate.ReadSections(bytes), bytes, path);
            Storage.Borrow(8, Owner);
            SetCurrent(name);
            return path;
        }

        /// <summary>임시 루트를 지운다.</summary>
        public void Dispose() => Temp.Dispose();
    }

    /// <summary>두 버퍼는 같은 Client에서 고정되고 진단 뒤 실제 source와 전방 겹침·부분 쓰기를 보존한다.</summary>
    [Fact]
    public void ClientBuffersAndPendingPhysicalCopy()
    {
        using Scene scene = new();
        Assert.Equal("", scene.Names.Current());
        Assert.Equal("", scene.Names.Pending());
        Assert.Equal(1u, scene.Names.Catalog.Dirty);
        Assert.False(SameStorage(scene.Names.CurrentBytes(), scene.Names.PendingBytes()));
        Assert.True(SameStorage(scene.Names.CurrentBytes(), scene.Names.CurrentBytes()));
        scene.SetCurrent("Current");
        FortSelection.SetPendingFortName(scene.Names.PendingBytes(), NameInput("Pending"));
        Assert.Equal("Current", scene.Names.Current());
        Assert.Equal("Pending", scene.Names.Pending());
        // 진단이 같은 원본 메모리를 바꾸면 진단 뒤의 현재 원본을 다시 읽는다
        byte[] source = [(byte)'B', (byte)'a', (byte)'d', (byte)'.', 0, 0x71, 0x72, 0x73];
        int diagnostics = 0;
        FortSelection.SetPendingFortName(scene.Names.PendingBytes(), source, () =>
        {
            diagnostics++;
            source[0] = (byte)'O';
            source[1] = (byte)'K';
            source[2] = 0;
        });
        Assert.Equal(1, diagnostics);
        Assert.Equal("OK", scene.Names.Pending());
        // 목적과 원본이 같은 물리 버퍼여도 전방 복사가 그대로 유지된다
        FortSelection.SetPendingFortName(scene.Names.PendingBytes(), scene.Names.PendingBytes());
        Assert.Equal("OK", scene.Names.Pending());
        Assert.True(SameStorage(scene.Names.CurrentBytes(), scene.Names.CurrentBytes()));
        // 목적이 짧으면 먼저 쓴 부분만 남기고 멈춘다
        byte[] shortTarget = [0x71, 0x72];
        Assert.ThrowsAny<Exception>(() => FortSelection.SetPendingFortName(shortTarget, NameInput("Pending")));
        Assert.Equal((byte)'P', shortTarget[0]);
        Assert.Equal((byte)'e', shortTarget[1]);
        // 같은 9바이트 키의 주소는 키를 바꿔도 그대로이고 선택 포인터와 내용 변경은 구별된다
        ReadOnlyMemory<byte> key = scene.Names.FileSpec.Buffer;
        scene.Names.FileSpec.SelectSolo();
        scene.Names.SelectFileSpec();
        Assert.Equal("SOLOSpec", scene.Names.SelectedFileSpec());
        Assert.Equal("SOLOSpec", Encoding.ASCII.GetString(key.Span[..8]));
        Assert.Equal(0, key.Span[8]);
        Assert.True(SameStorage(key.Span, scene.Names.FileSpec.Buffer.Span));
    }

    /// <summary>실제 현재 이름 변경의 dirty는 다음 목록에서 오래된 캐시를 반납하고 작업 디스크를 다시 읽게 한다.</summary>
    [Fact]
    public void CurrentSelectionInvalidatesSameCatalog()
    {
        using Scene scene = new();
        byte[] original = RealFortBytes("TEST01");
        scene.Files.Prepare("Visible", original);
        GameFileSystem source = new(scene.WorkFolder, []);
        SoloMapCatalog catalog = new(source, scene.Config, new SoloMapCatalogEffects
        {
            TypeCount = () => 188,
            AllocationBytes = _ => throw new InvalidOperationException("정상 작업 맵의 빈 State"),
            UserName = () => "",
            Translate = text => text,
            FileSpecKey = scene.Names.FileSpec.Current,
            ListState = scene.Names.Catalog,
        });
        SoloMapListState cache = scene.Names.Catalog;
        cache.Names[0] = "Cached";
        cache.Count = 1;
        cache.Dirty = 0;
        // 이미 정상인 캐시를 명시 검사 입력으로 만든다
        Assert.Same(cache, catalog.State);
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal("Cached", cache.Names[0]);
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("Selected"));
        Assert.Equal("Selected", scene.Names.Current());
        Assert.Equal(1u, cache.Dirty);
        Assert.Equal(1, scene.Titles);
        Assert.Empty(scene.Snapshots);
        Assert.Equal(0, scene.NotifyCount);
        Assert.Equal(0, scene.Storage.OwnedImages);
        Assert.Equal("Selected", scene.Config.Configuration.Get("lastEditFort"));
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal("Visible", cache.Names[0]);
        Assert.Equal(0u, cache.Dirty);
        Assert.Equal("Selected", scene.Names.Current());
    }

    /// <summary>같은 manager·빌린 슬롯·기존 stream은 새 이름에도 유지되고 실제 Notify가 새 파일에 저장한 뒤 이전 파일만 삭제한다.</summary>
    [Fact]
    public void WritableManagerNotifyRenameAndDelete()
    {
        using Scene scene = new();
        string old = scene.Install("Old");
        byte manager = scene.Storage.Headers.Span[(int)scene.Owner * 20];
        TemplateWriteStream stream = scene.Storage.CreateSection(scene.Owner, "Temp");
        scene.Storage.WriteSection(stream, [0x31, 0x32]);
        byte[] before = File.ReadAllBytes(old);
        Span<byte> bytes = scene.State.Get(scene.Owner);
        bytes[12] ^= 0x53;
        byte marker = bytes[12];
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("New"));
        // 가상 게임 경로는 소문자로 정규화되어 새 파일 이름이 정해진다
        string destination = scene.Files.ResolvePath("d/new.fort");
        Assert.False(File.Exists(old));
        Assert.True(File.Exists(destination));
        Assert.Equal(1, scene.Storage.OwnedImages);
        Assert.Equal(manager, scene.Storage.Headers.Span[(int)scene.Owner * 20]);
        Assert.Equal(destination, scene.Storage.Name(scene.Owner));
        Assert.Equal(destination, scene.Storage.Name(8));
        Assert.Equal(["Old", "New"], scene.Snapshots);
        Assert.Equal(1, scene.NotifyCount);
        Assert.Equal(1, scene.SaveCount);
        Assert.Equal(1, scene.Titles);
        Assert.Empty(scene.Assertions);
        Assert.Equal("New", scene.Config.Configuration.Get("lastEditFort"));
        Assert.Equal(2u, scene.Names.Catalog.Dirty);
        byte[] saved = File.ReadAllBytes(destination);
        Assert.NotEqual(before, saved);
        Assert.Equal(marker, FortTemplate.ReadSections(saved).Section("State")[12]);
        // 이름을 바꿔도 보관한 쓰기 참조와 같은 stream이 유지된다
        scene.Storage.WriteSection(stream, [0x33]);
        Assert.Equal(stream.Generation, scene.Storage.FindSection(8, "Temp").Generation);
    }

    /// <summary>저장된 isMine은 첫 snapshot과 이전 파일 삭제만 생략하고 같은 manager의 새 이름·Notify·후속 snapshot을 유지한다.</summary>
    [Fact]
    public void ReadonlyOldFileIsRetained()
    {
        using Scene scene = new();
        string old = scene.Install("Kept");
        byte[] original = File.ReadAllBytes(old);
        scene.Storage.Headers.Span[(int)scene.Owner * 20 + 8] = 1;
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("Copy"));
        Assert.Equal(original, File.ReadAllBytes(old));
        Assert.True(File.Exists(scene.Files.ResolvePath("d/copy.fort")));
        Assert.Equal(["Copy"], scene.Snapshots);
        Assert.Equal(1, scene.NotifyCount);
        Assert.Equal(1, scene.SaveCount);
        Assert.Equal(0, scene.Storage.Headers.Span[(int)scene.Owner * 20 + 8]);
    }

    /// <summary>실제 Notify가 실패하면 이미 쓴 current·manager 이름을 유지하고 뒤의 삭제·제목·dirty·설정은 실행하지 않는다.</summary>
    [Fact]
    public void FailedNotifyKeepsPartialNameWithoutDeletingOld()
    {
        using Scene scene = new();
        string old = scene.Install("Before");
        FortCurrentNameHooks hooks = scene.Hooks();
        hooks.Notify = _ => throw new InvalidOperationException("명시 Notify 실패");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), hooks, scene.Files);
        InvalidOperationException failure = Assert.Throws<InvalidOperationException>(() => devices.Set(NameInput("After")));
        Assert.Equal("명시 Notify 실패", failure.Message);
        // 가상 게임 경로는 소문자로 정규화되지만 실제 이름 버퍼는 After를 보존한다
        Assert.Equal("After", scene.Names.Current());
        Assert.Equal(scene.Files.ResolvePath("d/after.fort"), scene.Storage.Name(1));
        Assert.True(File.Exists(old));
        Assert.False(File.Exists(scene.Files.ResolvePath("d/after.fort")));
        Assert.Equal(["Before"], scene.Snapshots);
        Assert.Equal(0, scene.Titles);
        Assert.Equal(1u, scene.Names.Catalog.Dirty);
    }

    /// <summary>고정 256바이트 목적 범위를 넘으면 먼저 쓴 부분만 남고 뒤의 외부 효과를 실행하지 않는다.</summary>
    [Fact]
    public void PhysicalCurrentNameOverflowPreservesPrefix()
    {
        using Scene scene = new();
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks());
        Assert.ThrowsAny<Exception>(() => devices.Set(NameInput(new string('N', 260))));
        Span<byte> bytes = scene.Names.CurrentBytes();
        Assert.True(bytes.ToArray().All(value => value == (byte)'N'));
        Assert.ThrowsAny<Exception>(() => scene.Names.Current());
        Assert.Equal(0, scene.Titles);
        Assert.Equal(1u, scene.Names.Catalog.Dirty);
        Assert.Empty(scene.Snapshots);
    }

    /// <summary>삭제 요청은 자기 단일 파일에만 도달하며 보호 경로와 루트 밖을 거부하고 디렉터리는 유지한다.</summary>
    [Fact]
    public void DeleteUsesOnlyCheckedSingleFile()
    {
        using Scene scene = new();
        byte[] original = RealFortBytes("TEST01");
        string path = scene.Files.Prepare("Delete", original);
        string protectedFile = Path.Combine(scene.ProtectedFolder, "d", "TEST01.fort");
        Directory.CreateDirectory(Path.GetDirectoryName(protectedFile)!);
        File.WriteAllBytes(protectedFile, original);
        Assert.ThrowsAny<Exception>(() => scene.Files.Delete(protectedFile));
        Assert.ThrowsAny<Exception>(() => scene.Files.Delete(Path.Combine(scene.Temp.Root, "outside.fort")));
        scene.Files.Delete(path);
        Assert.False(File.Exists(path));
        // 이미 없는 파일의 삭제는 실패 결과를 쓰지 않는다
        scene.Files.Delete(path);
        string directory = Path.Combine(scene.WorkFolder, "empty");
        Directory.CreateDirectory(directory);
        scene.Files.Delete(directory);
        Assert.True(Directory.Exists(directory));
        Assert.Equal(original, File.ReadAllBytes(protectedFile));
    }

    /// <summary>한글·이모지 이름은 UTF-8 바이트로 이름 버퍼와 manager에 오가고, 설정에는 올바른 문자열로 기록되며, 작업 파일 경로도 같은 이름을 쓴다.</summary>
    [Fact]
    public void UnicodeNameRoundTripsThroughBufferManagerAndConfig()
    {
        using Scene scene = new();
        string old = scene.Install("Old");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("한글 맵 😀"));
        Assert.Equal("한글 맵 😀", scene.Names.Current());
        string destination = scene.Files.ResolvePath("d/한글 맵 😀.fort");
        Assert.Equal(destination, scene.Storage.Name(scene.Owner));
        Assert.True(File.Exists(destination));
        Assert.False(File.Exists(old));
        Assert.Equal("한글 맵 😀", scene.Config.Configuration.Get("lastEditFort"));
        Assert.Equal(["Old", "한글 맵 😀"], scene.Snapshots);
    }

    /// <summary>
    /// 원문 CRT 비교는 대소문자를 구별하지 않는다: 작업 namespace 없이 같은 이름을 다른 대소문자로 바꾸면 이전 경로와 같다고 보아 삭제를 요청하지 않는다.
    /// 삭제 장치가 없어 삭제 요청이 나가면 명시 오류가 되므로 오류 없이 끝나면 비교가 대소문자를 무시한 것이다.
    /// </summary>
    [Fact]
    public void SameNameWithDifferentCaseDoesNotRequestDelete()
    {
        using Scene scene = new();
        string old = scene.Install("Same");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks());
        devices.Set(NameInput("SAME"));
        Assert.Equal("SAME", scene.Names.Current());
        Assert.True(File.Exists(old));
        Assert.Equal(1, scene.NotifyCount);
    }

    /// <summary>이전 이름이 새 이름의 접두일 때도 두 경로는 다르다고 보아 이전 작업 파일을 삭제한다(길이가 다른 경로를 같다고 보지 않는다).</summary>
    [Fact]
    public void RenameToLongerNameWithSamePrefixDeletesOld()
    {
        using Scene scene = new();
        string old = scene.Install("Old");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("OldX"));
        Assert.False(File.Exists(old));
        Assert.True(File.Exists(scene.Files.ResolvePath("d/oldx.fort")));
        Assert.Equal(scene.Files.ResolvePath("d/oldx.fort"), scene.Storage.Name(scene.Owner));
    }

    /// <summary>
    /// 원문 CRT 경로 비교는 NUL 앞 전체 문자열을 ASCII 대소문자만 무시하고 비교한다: 한쪽이 다른 쪽의 접두인 경로나 ASCII 밖 바이트의 대소문자 차이는 같다고 보지 않는다.
    /// 실제 경로는 둘 다 같은 확장자로 끝나 이런 입력이 나오지 않으므로 장치가 연결한 비교 훅을 직접 호출한다.
    /// </summary>
    [Fact]
    public void SamePathComparesWholeStringsIgnoringAsciiCaseOnly()
    {
        using Scene scene = new();
        FortCurrentNameHooks hooks = scene.Hooks();
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), hooks, scene.Files);
        Assert.NotNull(devices);
        FortSamePath same = hooks.SamePath!;
        Assert.True(same(NameInput("d/Map.FORT"), NameInput("D/map.fort")));
        Assert.False(same(NameInput("d/map"), NameInput("d/map.fort")));
        Assert.False(same(NameInput("d/map.fort"), NameInput("d/map")));
        Assert.False(same(NameInput("d/map.fort"), NameInput("d/mab.fort")));
        // NUL 뒤의 바이트는 비교하지 않는다
        Assert.True(same([(byte)'a', 0, (byte)'x'], [(byte)'A', 0, (byte)'y']));
        // ASCII 밖의 대소문자 쌍(É/é)은 C 로캘 비교에서 다르다
        Assert.False(same(NameInput("É"), NameInput("é")));
        // 대문자 범위의 양 끝 바로 밖(@ 위와 [ 아래)은 32 차이가 나도 대소문자 쌍이 아니다
        Assert.False(same(NameInput("[x"), NameInput("{x")));
        Assert.False(same(NameInput("@x"), NameInput("`x")));
    }

    /// <summary>
    /// 절대 경로로 설정된 파일 명세는 게임 경로처럼 소문자로 정규화하지 않고 작업 namespace 안의 같은 대소문자 경로로 해석한다.
    /// </summary>
    [Fact]
    public void AbsoluteFileSpecKeepsCaseInsideWorkNamespace()
    {
        using Scene scene = new();
        string folder = Path.Combine(scene.WorkFolder, "Maps");
        Directory.CreateDirectory(folder);
        scene.Config.Set("fortSpec", folder.Replace('\\', '/') + "/{local.1}.fort");
        scene.Install("Old");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks(), scene.Files);
        devices.Set(NameInput("Abs"));
        string expected = scene.Files.ResolvePath(Path.Combine(folder, "Abs.fort"));
        Assert.Equal(expected, scene.Storage.Name(scene.Owner));
        Assert.True(File.Exists(expected));
        Assert.Contains("Maps", scene.Storage.Name(scene.Owner));
        Assert.Contains("Abs.fort", scene.Storage.Name(scene.Owner));
    }

    /// <summary>삭제 장치가 없는 이름 장치는 작업 namespace 삭제를 빈 성공으로 만들지 않고 명시 오류로 멈춘다.</summary>
    [Fact]
    public void DeleteWithoutWorkspaceIsExplicit()
    {
        using Scene scene = new();
        scene.Install("Plain");
        FortCurrentNameDevices devices = new(scene.Names, scene.Storage, scene.Config, scene.Globals(), scene.Hooks());
        Assert.ThrowsAny<Exception>(() => devices.Set(NameInput("Other")));
        Assert.Equal("Other", scene.Names.Current());
    }

    /// <summary>기본 Fort 선택은 newFort의 PathSpec 결과를 기존 전체 이름 부모에 전달하고 장치가 없으면 명시 오류다.</summary>
    [Fact]
    public void ConfiguredNewFortSelectsPathSpecResult()
    {
        using Scene scene = new();
        scene.Config.Set("newFort", "d/기본 {local.1}.fort");
        string? selected = null;
        FortCurrentNameDevices.SelectConfiguredNewFort(scene.Config, name => selected = name);
        Assert.Equal(scene.Config.PathSpec("newFort"), selected);
        Assert.NotEqual(scene.Config.PathSpec("fortSpec"), selected);
        Assert.Throws<InvalidOperationException>(() => FortCurrentNameDevices.SelectConfiguredNewFort(scene.Config, null));
    }
}
