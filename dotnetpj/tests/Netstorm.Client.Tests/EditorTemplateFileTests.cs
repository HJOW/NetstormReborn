using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 편집 파일 장치(<see cref="EditorFiles"/>·<see cref="EditorFortFileDevices"/>)가 Template 저장소(<see cref="CampaignTemplateStorage"/>)와 만나는 경로를 검사한다:
/// 현재 manager 이름·서명·기존 끝을 소비하는 저장, Notify 저장과 다시 읽기, Fort 선택 State, 읽기 전용 속성과 저장 억제 관문, <see cref="EditorFortFileDevices.ExistingEditorTemplateValid"/>.
/// cpppj <c>EditorFileWriterTests</c> 1개, <c>EditorFilesTests</c> 5개, <c>EditorFortFileDevicesTests</c> 안의 저장소 사용 부분(4개)을 옮겼다.
/// cpppj의 <c>FortTemplate::Parse</c>는 아직 없어(F1) 같은 섹션 읽기만 하는 <see cref="FortTemplate.ReadSections"/>로 대신한다.
/// </summary>
/// <remarks>
/// 2026-11-02 0.135(TS5)에서 추가했다. 원본 x86 fixture가 없는 직역 검사이므로 L1이 아니다. 작업본은 한글·이모지 경로의 임시 폴더라 manager 이름의 UTF-8 보관도 함께 확인한다.
/// 보호 원본 폴더도 임시 복사본이며 실제 <c>originals</c>에는 쓰지 않는다(cpppj는 실제 보호 루트를 썼다).
/// </remarks>
public sealed class EditorTemplateFileTests
{
    /// <summary>한글과 이모지가 든 보호 원본 폴더 경로다. 만들지는 않는다.</summary>
    private static string Protected(EditorTempRoot temp) => Path.Combine(temp.Root, "보호 원본 😀");

    /// <summary>한글과 이모지가 든 작업본 폴더 경로다. 만들지는 않는다.</summary>
    private static string WorkFolder(EditorTempRoot temp) => Path.Combine(temp.Root, "편집 작업본 😀");

    /// <summary>보호 원본 하나를 둔 작업 파일 장치를 만든다. 생성만으로는 파일을 만들지 않는다.</summary>
    private static EditorFiles Open(EditorTempRoot temp) => new(WorkFolder(temp), [Protected(temp)]);

    /// <summary>보호 원본 폴더에 검사용 파일을 둔다(실제 <c>originals</c>가 아니라 임시 폴더다).</summary>
    /// <param name="temp">임시 루트.</param>
    /// <param name="relative">보호 폴더 안 상대 경로.</param>
    /// <param name="bytes">파일 내용.</param>
    private static string SeedProtected(EditorTempRoot temp, string relative, byte[] bytes)
    {
        string path = Path.Combine(Protected(temp), relative.Replace('/', Path.DirectorySeparatorChar));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllBytes(path, bytes);
        return path;
    }

    /// <summary>
    /// 35개 길이 레코드와 28바이트 State 하나로 된 작은 Fort 이미지다. State의 12번째 바이트가 표식이고 17번째 바이트가 2여서 실제 reader가 소비한다.
    /// </summary>
    /// <param name="marker">State 12번째 바이트에 넣을 표식.</param>
    private static byte[] FortImage(byte marker = 7)
    {
        List<byte> result = [(byte)'F', 0];
        // 표준 섹션 35개를 길이 레코드로 만든다. 두 번째 섹션만 State 28바이트를 가진다
        for (int index = 0; index < 35; index++)
        {
            ushort length = (ushort)(index == 1 ? 30 : 2);
            result.Add((byte)length);
            result.Add((byte)(length >> 8));
            if (index == 1)
            {
                byte[] state = new byte[28];
                state[12] = marker;
                state[17] = 2;
                result.AddRange(state);
            }
        }
        return [.. result];
    }

    /// <summary>fortSpec 경로 지정을 가진 Config를 만든다.</summary>
    private static ConfigInterface Config()
    {
        ConfigInterface config = new(null);
        config.Startup(new());
        config.Set("fortSpec", "d/{local.1}.fort");
        return config;
    }

    /// <summary>현재 Fort 이름과 고정 FileSpec 키를 한곳에서 바꾸는 효과 묶음이다. 제품의 현재 Fort 소유자라고 주장하지 않는다.</summary>
    /// <param name="name">현재 Fort 이름.</param>
    private static EditorFortPathEffects PathEffects(string name) => new()
    {
        CurrentName = () => name,
        SelectFileSpec = () => { },
        FileSpecKey = () => "fortSpec",
        Assertion = _ => throw new InvalidOperationException("예상 밖 현재 이름 진단"),
    };

    /// <summary>
    /// 파일 장치 밖의 현재 owner·TYPE·제목·원문 진단만 검사 입력으로 명시한 Load 경계다. 읽기·권한·이름 비교는 장치가 <see cref="EditorFortFileDevices.BindLoad"/>로 채운다.
    /// </summary>
    private static TemplateFileLoadHooks LoadEffects() => new()
    {
        TypeCount = () => 188,
        CurrentSlot = () => 1u,
        RefreshTitle = () => { },
        WrongName = () => throw new InvalidOperationException("예상 밖 기존 이름 진단"),
        TooManyTypes = () => throw new InvalidOperationException("예상 밖 TYPE 진단"),
        RejectFile = _ => throw new InvalidOperationException("예상 밖 서명 제거 요청"),
    };

    /// <summary>타입 개수만 연결한 이미지 장치다. 진단은 모두 실패로 본다.</summary>
    private static FortImageHooks ImageHooks() => new()
    {
        TypeCount = () => 188,
        Assertion = _ => throw new InvalidOperationException("편집 파일 검사에서 예상하지 않은 이미지 진단"),
    };

    /// <summary>저장 억제·로컬 소유자·숨김 표를 현재 값으로 읽는 저장 상태다. 값은 호출 때마다 다시 읽는다.</summary>
    /// <param name="readOnlyForts">저장 억제 DWORD.</param>
    /// <param name="owner">로컬 소유자.</param>
    /// <param name="hidden">9칸 숨김 표.</param>
    private static DataManagerFileSaveState SaveState(Func<uint> readOnlyForts, Func<uint> owner, byte[] hidden) => new()
    {
        ReadOnlyForts = readOnlyForts,
        LocalOwner = owner,
        HiddenPlayers = () => hidden,
    };

    /// <summary>쓰기 장치가 호출되면 실패로 보는 파일 쓰기 경계다. 저장 억제 관문이 장치까지 가지 않음을 확인한다.</summary>
    private static TemplateFileWriteHooks NoWrite() => new(
        (_, _) => throw new InvalidOperationException("억제된 저장이 권한 장치를 호출"),
        (_, _) => throw new InvalidOperationException("억제된 저장이 쓰기 장치를 호출"));

    /// <summary>두 요새의 섹션이 개수와 내용 모두 같은지 확인한다(C++의 <c>sections==sections</c>).</summary>
    /// <param name="expected">기대 요새.</param>
    /// <param name="actual">실제 요새.</param>
    private static void AssertSameSections(FortTemplate expected, FortTemplate actual)
    {
        Assert.Equal(expected.Sections.Count, actual.Sections.Count);
        // 같은 위치의 섹션 내용을 하나씩 비교한다
        for (int index = 0; index < expected.Sections.Count; index++)
        {
            Assert.Equal(expected.Sections[index].Span.ToArray(), actual.Sections[index].Span.ToArray());
        }
    }

    /// <summary>manager+c의 이름을 UTF-8 바이트와 NUL로 바꿔 쓴다. 같은 manager의 나머지 바이트는 그대로 둔다.</summary>
    /// <param name="manager">manager 바이트.</param>
    /// <param name="name">쓸 이름.</param>
    private static void SetManagerName(Span<byte> manager, string name)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(name);
        Assert.True(bytes.Length < 256, "manager 이름 범위");
        bytes.CopyTo(manager[12..]);
        manager[12 + bytes.Length] = 0;
    }

    /// <summary>manager의 지정 DWORD를 원본 little endian으로 쓴다.</summary>
    /// <param name="manager">manager 바이트.</param>
    /// <param name="offset">위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SetManagerField(Span<byte> manager, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(manager[offset..], value);

    // ---- 저장 쓰기 (cpppj EditorFileWriterTests) ----

    /// <summary>
    /// 실제 저장이 같은 manager의 현재 이름·서명·기존 파일 끝을 소비한다: 편의 이름 사본이나 Template의 flag 필드가 아니라 manager+c와 +110을 읽고,
    /// 새 이미지가 기존 파일보다 짧아도 이전 끝을 보존하며, 이름을 바꾸면 다른 파일을 새로 만든다.
    /// </summary>
    [Fact]
    public void SaveUsesCurrentManagerNameSignatureAndExistingTail()
    {
        using var temp = new EditorTempRoot();
        EditorFiles files = Open(temp);
        byte[] sentinel = Enumerable.Repeat((byte)0xa7, 256).ToArray();
        string first = files.Prepare("첫 맵 😀", sentinel);
        string second = files.ResolvePath("d/다음 맵 😀.fort");
        FortTemplate fort = TemplateStorageTestSupport.EmptyFort();
        fort.Sections[1] = RawSectionBuffer.FromBytes([7, 8, 9]);
        fort.Flag = 0;
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "stale-name.fort");
        Span<byte> manager = storage.ManagerBytes(TemplateStorageTestSupport.Header(storage, 20));
        SetManagerName(manager, first);
        SetManagerField(manager, 0x110, 1);
        uint readOnly = 0, owner = 1;
        DataManagerFileSaveState state = SaveState(() => readOnly, () => owner, new byte[9]);
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        byte[] saved = File.ReadAllBytes(first);
        // 72바이트 헤더·길이표와 State 3바이트 뒤의 기존 끝 전체를 보존한다
        Assert.Equal(sentinel.Length, saved.Length);
        Assert.Equal((byte)'F', saved[0]);
        Assert.Equal(0xfe, saved[1]);
        Assert.Equal(new byte[] { 7, 8, 9 }, saved[6..9]);
        Assert.All(saved[75..], value => Assert.Equal(0xa7, value));
        Assert.Equal("Terr19", storage.CurrentSection(1));
        Assert.Equal(first, storage.Name(1));
        // 편의 flag와 이름 사본이 아니라 반대로 바꾼 실제 manager 필드만 쓴다
        SetManagerName(manager, second);
        SetManagerField(manager, 0x110, 0);
        storage.Template(1).Flag = 0xfe;
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        byte[] current = File.ReadAllBytes(second);
        Assert.Equal(75, current.Length);
        Assert.Equal((byte)'F', current[0]);
        Assert.Equal(0, current[1]);
        Assert.Equal(saved, File.ReadAllBytes(first));
        Assert.Equal(second, storage.Name(1));
    }

    // ---- 파일 장치와 저장소 (cpppj EditorFilesTests) ----

    /// <summary>
    /// 실제 TEST01 원문을 작업본에 복사해 같은 State를 Notify로 저장하고 다시 읽는다: 빌린 헤더도 같은 manager를 저장하고, 서명 flag는 파서 사본이 아니라 manager의 값을 쓰며,
    /// 읽기 전용·숨김 소유자·빌림 헤더의 isMine 관문은 저장 부모를 부르되 파일을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void NotifySavesAndReloadsTest01()
    {
        using var temp = new EditorTempRoot();
        byte[] original = TemplateStorageTestSupport.RealFortBytes("TEST01");
        EditorFiles files = Open(temp);
        string path = files.Prepare("TEST01", original);
        Assert.Equal(original, File.ReadAllBytes(path));
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, FortTemplate.ReadSections(original), original, path);
        storage.Borrow(8, 1);
        uint sourceFlag = storage.FileHeader(1).FeFlag;
        // 파서의 과거 flag 사본을 일부러 현재 manager 서명과 다르게 만든다
        storage.Template(1).Flag = sourceFlag != 0 ? (byte)0 : (byte)0xfe;
        uint depth = 0, readOnly = 0, owner = 1;
        byte[] hidden = new byte[9];
        int saves = 0;
        RawPlayerTemplateState state = new(
            OriginalEdition.Patch1078,
            storage.Headers,
            slot => storage.SelectState(slot),
            slot => storage.Name(slot),
            () => depth,
            value => depth = value,
            () => false,
            _ => throw new InvalidOperationException("편집 Notify 예상 밖 진단"),
            _ => throw new InvalidOperationException("PATCH 편집 Notify의 CD 깊이 변경 금지"));
        state.BindMinimumSize((slot, size) => storage.EnsureStateSize(slot, size));
        state.BindFileSave(slot =>
        {
            saves++;
            storage.SaveFile(slot, SaveState(() => readOnly, () => owner, hidden), ImageHooks(), files.Writer());
        });
        Span<byte> bytes = state.Get(1);
        Assert.True(bytes.Length >= 28 && bytes[17] == 2);
        bytes[12] ^= 0x5a;
        uint position = storage.State(1).Position;
        state.Changed(8);
        // 빌린 헤더도 같은 현재 manager를 저장한다
        Assert.Equal(1, saves);
        Assert.Equal("Terr19", storage.CurrentSection(1));
        Assert.Equal(position, storage.State(1).Position);
        byte[] saved = File.ReadAllBytes(path);
        Assert.NotEqual(original, saved);
        Assert.Equal(sourceFlag != 0 ? (byte)0xfe : (byte)0, saved[1]);
        AssertSameSections(FortTemplate.ReadSections(saved), storage.Template(1));
        // 다음 진입 준비가 기존 사용자 작업본을 원본으로 되돌리지 않는다
        Assert.Equal(path, files.Prepare("TEST01", original));
        Assert.Equal(saved, File.ReadAllBytes(path));
        // 저장 억제 DWORD, 숨김 소유자, 빌린 헤더의 isMine, 헤더 +16 전이는 저장 부모를 부르거나 막되 파일은 그대로다
        bytes = state.Get(1);
        bytes[12] ^= 0x19;
        readOnly = 1;
        state.Changed(1);
        Assert.Equal(2, saves);
        Assert.Equal(saved, File.ReadAllBytes(path));
        readOnly = 0;
        hidden[owner] = 1;
        state.Changed(1);
        Assert.Equal(3, saves);
        Assert.Equal(saved, File.ReadAllBytes(path));
        storage.Headers.Span[20 + 8] = 1;
        state.Changed(1);
        Assert.Equal(3, saves);
        storage.Headers.Span[20 + 8] = 0;
        storage.Headers.Span[20 + 16] = 1;
        state.Changed(1);
        Assert.Equal(3, saves);
        // 원본은 복사 입력으로만 읽었다
        Assert.Equal(original, TemplateStorageTestSupport.RealFortBytes("TEST01"));
    }

    /// <summary>
    /// 새 manager의 서명 기본값(-1)은 저장에서 0으로 정규화되고, signed 타입 수 -1이어도 저장 억제 중 진단이 나지 않으며,
    /// 억제를 풀면 권한 요청 0x180 한 번과 72바이트 이미지를 쓰고 현재 섹션 검색이 Terr19로 끝난다.
    /// </summary>
    [Fact]
    public void RealManagerSignatureAndCurrentSearch()
    {
        CampaignTemplateStorage storage = new();
        FortTemplate fort = TemplateStorageTestSupport.EmptyFort();
        fort.Flag = 0xfe;
        storage.Install(0, fort, "blank.fort");
        Assert.Equal((0u, 0xffffffffu), storage.FileHeader(0));
        uint readOnly = 1, owner = 999;
        int assertions = 0;
        FortImageHooks image = new() { TypeCount = () => 0xffffffffu, Assertion = _ => assertions++ };
        DataManagerFileSaveState state = SaveState(() => readOnly, () => owner, new byte[9]);
        storage.SaveFile(0, state, image, NoWrite());
        Assert.Equal(0, assertions);
        Assert.Equal("Terr19", storage.CurrentSection(0));
        Assert.Equal((0x46u, 0u), storage.FileHeader(0));
        readOnly = 0;
        owner = 0;
        byte[] written = [];
        int permissions = 0;
        TemplateFileWriteHooks file = new(
            (name, mode) =>
            {
                Assert.Equal("blank.fort", name);
                Assert.Equal(0x180u, mode);
                permissions++;
            },
            (name, bytes) =>
            {
                Assert.Equal("blank.fort", name);
                written = bytes.ToArray();
            });
        storage.SaveFile(0, state, image, file);
        Assert.Equal(1, permissions);
        Assert.Equal(72, written.Length);
        Assert.Equal((byte)'F', written[0]);
        Assert.Equal(0, written[1]);
        AssertSameSections(FortTemplate.ReadSections(written), storage.Template(0));
    }

    /// <summary>
    /// 원문 저장 부모의 72바이트 빈 이미지가 기존 긴 파일의 끝을 자르지 않고 현재 서명과 섹션 길이만 갱신한다. 저장 억제를 켜면 기존 파일을 그대로 보존한다.
    /// </summary>
    [Fact]
    public void SaveKeepsTailWhenCurrentTemplateGetsShorter()
    {
        using var temp = new EditorTempRoot();
        byte[] original = TemplateStorageTestSupport.RealFortBytes("TEST01");
        Assert.True(original.Length > 72);
        EditorFiles files = Open(temp);
        string path = files.Prepare("ShorterImage", original);
        CampaignTemplateStorage storage = new();
        storage.Install(1, TemplateStorageTestSupport.EmptyFort(), path);
        uint readOnly = 0, owner = 1;
        DataManagerFileSaveState state = SaveState(() => readOnly, () => owner, new byte[9]);
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        byte[] expected = (byte[])original.Clone();
        Array.Clear(expected, 0, 72);
        expected[0] = (byte)'F';
        // 원문 35개 빈 섹션의 길이 WORD는 각각 2이며 기존 72바이트 뒤 자료는 그대로 남는다
        for (int section = 0; section < 35; section++)
        {
            expected[2 + section * 2] = 2;
        }
        byte[] saved = File.ReadAllBytes(path);
        Assert.Equal(expected, saved);
        Assert.Equal(original.Length, saved.Length);
        Assert.Equal("Terr19", storage.CurrentSection(1));
        Assert.Equal((0x46u, 0u), storage.FileHeader(1));
        AssertSameSections(FortTemplate.ReadSections(saved), storage.Template(1));
        Assert.Equal(original, TemplateStorageTestSupport.RealFortBytes("TEST01"));
        // 쓰기 억제 관문은 새 파일 장치 연결 뒤에도 기존 파일을 보존한다
        readOnly = 1;
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        Assert.Equal(saved, File.ReadAllBytes(path));
    }

    /// <summary>보호 폴더 안 원문의 이름으로 저장 부모를 부르면 권한이나 내용을 바꾸기 전에 거부된다.</summary>
    [Fact]
    public void ProtectedNameIsRejectedThroughSaveParent()
    {
        using var temp = new EditorTempRoot();
        byte[] original = TemplateStorageTestSupport.RealFortBytes("TEST01");
        string path = SeedProtected(temp, "d/TEST01.fort", original);
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, FortTemplate.ReadSections(original), original, path);
        EditorFiles files = Open(temp);
        DataManagerFileSaveState state = SaveState(() => 0, () => 1, new byte[9]);
        Assert.ThrowsAny<Exception>(() => storage.SaveFile(1, state, ImageHooks(), files.Writer()));
        Assert.Equal(original, File.ReadAllBytes(path));
    }

    /// <summary>
    /// 맵 읽기 명령이 pending과 다음 State만 예약하면서 기존 전투 Fort의 실제 State·Notify·저장도 같은 장치에 적용한다: 저장 관문은 Template 칸이 아니라 실제 owner의 숨김 표를 읽고,
    /// 저장한 파일을 다시 읽으면 같은 State가 나온다.
    /// </summary>
    [Fact]
    public void FortSelectionUsesCurrentStateAndWriter()
    {
        using var temp = new EditorTempRoot();
        byte[] original = TemplateStorageTestSupport.RealFortBytes("TEST01");
        EditorFiles files = Open(temp);
        string path = files.Prepare("CurrentMap", original);
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, FortTemplate.ReadSections(original), original, path);
        FortSelectionGlobals globals = new() { Mode = 2, InBattle = 1, CurrentState = 14, TemplateOwner = 1, Owner = 7 };
        globals.Next[0] = 7;
        uint depth = 0, readOnly = 0;
        byte[] hidden = new byte[9];
        int saves = 0, loads = 0;
        string pending = "old";
        RawPlayerTemplateState state = new(
            OriginalEdition.Patch1078,
            storage.Headers,
            slot => storage.SelectState(slot),
            slot => storage.Name(slot),
            () => depth,
            value => depth = value,
            () => false,
            _ => throw new InvalidOperationException("Fort 선택의 예상 밖 State 진단"),
            _ => throw new InvalidOperationException("PATCH Fort 선택의 CD 깊이 변경 금지"));
        state.BindMinimumSize((slot, size) => storage.EnsureStateSize(slot, size));
        state.BindFileSave(slot =>
        {
            saves++;
            storage.SaveFile(slot, SaveState(() => readOnly, () => globals.Owner, hidden), ImageHooks(), files.Writer());
        });
        // 저장 관문은 Template 슬롯 1이 아닌 실제 owner 7의 숨김 값을 읽어야 한다
        hidden[globals.TemplateOwner] = 1;
        byte[] before = state.Get(1).ToArray();
        FortSelectionHooks hooks = new()
        {
            ResolveString = token =>
            {
                Assert.Equal(202u, token);
                return "NextMap";
            },
            Pending = name => pending = FortSelection.PendingName(name),
            CurrentFortPath = () => storage.Name(globals.TemplateOwner),
            // 실파일 읽기는 기존 현재 이름을 쓰고 선택한 pending 맵을 설치하지 않는다
            LoadFort = (slot, fileName) =>
            {
                Assert.Equal(1u, slot);
                Assert.Equal(path, fileName);
                byte[] bytes = File.ReadAllBytes(fileName);
                storage.InstallFile(slot, FortTemplate.ReadSections(bytes), bytes, fileName);
                loads++;
            },
            Core = slot =>
            {
                state.Get(slot);
                return storage.State(slot).Bytes;
            },
            GameSeconds = () => 45.75,
            OwnedIslands = selected =>
            {
                Assert.Equal(7u, selected);
                return 0;
            },
            Notify = slot => state.Changed(slot),
        };
        Assert.True(FortSelection.Dispatch(new(FortSelection.ReadFort, 101, 202), globals, hooks));
        Assert.Equal("NextMap", pending);
        Assert.Equal(1u, globals.Next[0]);
        Assert.Equal(2u, globals.Mode);
        Assert.Equal(1, loads);
        Assert.Equal(1, saves);
        Assert.Equal(path, storage.Name(1));
        byte[] expected = (byte[])before.Clone();
        expected[0] &= 0xf9;
        expected[12] = 45;
        expected[13] = 0;
        expected[14] = 0;
        expected[15] = 0;
        Assert.Equal(expected, state.Get(1).ToArray());
        byte[] saved = File.ReadAllBytes(path);
        Assert.NotEqual(original, saved);
        FortTemplate decoded = FortTemplate.ReadSections(saved);
        AssertSameSections(decoded, storage.Template(1));
        CampaignTemplateStorage reload = new();
        reload.InstallFile(1, decoded, saved, path);
        Assert.Equal(expected, reload.State(1).Bytes.Span.ToArray());
        // 원본은 읽기 전용 입력으로만 사용했고 저장은 새 작업본에 수행했다
        Assert.Equal(original, TemplateStorageTestSupport.RealFortBytes("TEST01"));
    }

    // ---- 현재 Fort 파일 장치와 저장소 (cpppj EditorFortFileDevicesTests 중 저장소를 쓰는 부분) ----

    /// <summary>
    /// 준비된 수정본을 장치가 채운 Load 경계로 읽으면 그 수정본의 State를 얻고 이름은 작업본 경로이며, 같은 이름으로 다시 읽어도 기존 manager를 유지한다.
    /// 한글·이모지 작업본 경로가 manager에 UTF-8로 보관되어 읽기 장치가 같은 파일을 찾는다.
    /// </summary>
    [Fact]
    public void PreparedEditedFortLoadsThroughBoundDevices()
    {
        using var temp = new EditorTempRoot();
        byte[] original = FortImage(7), edited = FortImage(29);
        SeedProtected(temp, "d/Map.fort", original);
        EditorFiles files = Open(temp);
        EditorFortFileDevices devices = new(files, new GameFileSystem(Protected(temp), []), Config(), PathEffects("Map"));
        string path = devices.PrepareCurrent();
        files.Write(path, edited);
        CampaignTemplateStorage storage = new();
        TemplateFileLoadHooks load = devices.BindLoad(LoadEffects());
        storage.LoadFile(1, new FortTemplate(), path, load);
        Assert.Equal(1, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 1));
        Assert.Equal(path, storage.Name(1));
        Assert.Equal(29, storage.State(1).Bytes.Span[12]);
        // 이름 비교만 연결한 호출은 같은 이름의 기존 manager를 그대로 둔다
        TemplateFileLoadHooks existing = new() { SameName = load.SameName };
        storage.LoadFile(1, new FortTemplate(), path, existing);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(29, storage.State(1).Bytes.Span[12]);
    }

    /// <summary>실제 읽기 전용 속성이 Config의 저장 억제 값과 독립으로 Load 뒤 헤더 +8(잠김)에 반영된다.</summary>
    [Fact]
    public void ActualReadOnlyAttributeLocksLoadedHeaderIndependentOfConfig()
    {
        using var temp = new EditorTempRoot();
        EditorFiles files = Open(temp);
        string path = files.Prepare("Map", FortImage());
        ConfigInterface config = Config();
        EditorFortFileDevices devices = new(files, new GameFileSystem(Protected(temp), []), config, new EditorFortPathEffects());
        File.SetAttributes(path, FileAttributes.ReadOnly);
        config.SetInt("readOnlyForts", 0);
        Assert.True(devices.ReadOnly(path));
        config.SetInt("readOnlyForts", 1);
        Assert.True(devices.ReadOnly(path));
        Assert.Equal(5, devices.Exists(path));
        Assert.NotEmpty(devices.Read(path));
        CampaignTemplateStorage locked = new();
        locked.LoadFile(1, new FortTemplate(), path, devices.BindLoad(LoadEffects()));
        Assert.Equal(1u, TemplateStorageTestSupport.Header(locked, 20 + 8));
        // 이 검사가 만든 파일의 읽기 전용 속성만 푼다
        File.SetAttributes(path, FileAttributes.Normal);
        Assert.False(devices.ReadOnly(path));
        CampaignTemplateStorage writable = new();
        writable.LoadFile(1, new FortTemplate(), path, devices.BindLoad(LoadEffects()));
        Assert.Equal(0u, TemplateStorageTestSupport.Header(writable, 20 + 8));
        Assert.Equal(1, config.GetInt("readOnlyForts"));
    }

    /// <summary>
    /// Config 저장 억제 DWORD는 이미지가 만들어진 뒤 저장이 그 순간 읽는다: 억제 중에는 쓰기 가능한 파일도 그대로 보존하고, 억제를 풀면 현재 State를 쓴다.
    /// 파일의 읽기 전용 판정과 섞이지 않는다.
    /// </summary>
    [Fact]
    public void SaveGateKeepsWritableFileUntilCurrentDwordAllowsWrite()
    {
        using var temp = new EditorTempRoot();
        EditorFiles files = Open(temp);
        byte[] original = FortImage(7);
        string path = files.Prepare("Map", original);
        EditorFortFileDevices devices = new(files, new GameFileSystem(Protected(temp), []), Config(), new EditorFortPathEffects());
        CampaignTemplateStorage storage = new();
        storage.LoadFile(1, new FortTemplate(), path, devices.BindLoad(LoadEffects()));
        storage.State(1).Bytes.Span[12] = 51;
        uint readOnlyForts = 1, owner = 1;
        DataManagerFileSaveState state = SaveState(() => readOnlyForts, () => owner, new byte[9]);
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        Assert.False(devices.ReadOnly(path));
        Assert.Equal(original, devices.Read(path));
        Assert.Equal("Terr19", storage.CurrentSection(1));
        readOnlyForts = 0;
        storage.SaveFile(1, state, ImageHooks(), files.Writer());
        Assert.Equal(FortImage(51), devices.Read(path));
    }

    /// <summary>유효성은 서명이나 토큰 조회가 아니라 현재 헤더 첫 DWORD의 비영 여부만 본다. 9칸 밖은 거부한다.</summary>
    [Fact]
    public void ValidReadsOnlyCurrentHeaderPointer()
    {
        CampaignTemplateStorage storage = new();
        Assert.Equal(0, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 1));
        // 첫 DWORD의 네 바이트 가운데 어느 하나만 비영이어도 1이고, 다른 칸이나 다음 DWORD는 영향이 없다
        for (int offset = 0; offset < 4; offset++)
        {
            storage.Headers.Span[20 + offset] = 0x80;
            Assert.Equal(1, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 1));
            Assert.Equal(0, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 0));
            Assert.Equal(0, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 2));
            storage.Headers.Span[20 + offset] = 0;
            Assert.Equal(0, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 1));
        }
        storage.Headers.Span[20 + 4] = 1;
        Assert.Equal(0, EditorFortFileDevices.ExistingEditorTemplateValid(storage, 1));
        // 9칸 밖은 헤더 조각이 범위를 벗어나기 전에 슬롯 검사가 먼저 거부해야 한다(예외 매개변수 이름으로 구별한다)
        Assert.Equal("slot", Assert.Throws<ArgumentOutOfRangeException>(() => EditorFortFileDevices.ExistingEditorTemplateValid(storage, 9)).ParamName);
        Assert.Equal("slot", Assert.Throws<ArgumentOutOfRangeException>(() => EditorFortFileDevices.ExistingEditorTemplateValid(storage, uint.MaxValue)).ParamName);
        Assert.Throws<ArgumentNullException>(() => EditorFortFileDevices.ExistingEditorTemplateValid(null!, 1));
    }

    // ---- manager 이름의 UTF-8 보관 (이번 단위에서 직접 더한 검사) ----

    /// <summary>
    /// 한글·이모지 이름은 manager+c에 UTF-8 바이트와 NUL로 보관되고 다시 읽으면 같은 문자열이 된다. 바이트 수가 256 미만일 때만 받는다(문자 수가 아니다).
    /// </summary>
    [Fact]
    public void ManagerNameStoresUtf8AndLimitsByByteCount()
    {
        CampaignTemplateStorage storage = new();
        string name = "한글 맵 😀.fort";
        storage.Install(1, TemplateStorageTestSupport.EmptyFort(), name);
        byte[] encoded = Encoding.UTF8.GetBytes(name);
        Span<byte> manager = storage.ManagerBytes(TemplateStorageTestSupport.Header(storage, 20));
        Assert.Equal(encoded, manager.Slice(12, encoded.Length).ToArray());
        Assert.Equal(0, manager[12 + encoded.Length]);
        Assert.Equal(name, storage.Name(1));
        // 한글 85자는 255바이트라 받고, 86자는 258바이트라 문자 수가 256 미만이어도 거부한다
        string longest = new('가', 85);
        storage.Install(2, TemplateStorageTestSupport.EmptyFort(), longest);
        Assert.Equal(longest, storage.Name(2));
        Assert.Throws<ArgumentException>(() => storage.Install(3, TemplateStorageTestSupport.EmptyFort(), new string('가', 86)));
    }

    /// <summary>
    /// 로그인 이미지가 쓰는 빈 manager 생성(<see cref="CampaignTemplateStorage.ConstructEmptyManager"/>)도 같은 UTF-8 규칙이다:
    /// 이름은 바이트로 manager+c에 기록되고 다시 읽으면 같은 문자열이며, 한글 85자(255바이트)는 받고 86자(258바이트)는 거부한다.
    /// </summary>
    [Fact]
    public void EmptyManagerConstructionStoresUtf8Name()
    {
        CampaignTemplateStorage storage = new();
        uint token = storage.AllocateEmptyManager(276);
        string name = "새 맵 😀.fort";
        Assert.Equal(token, storage.ConstructEmptyManager(token, name));
        byte[] encoded = Encoding.UTF8.GetBytes(name);
        Span<byte> manager = storage.ManagerBytes(token);
        Assert.Equal(encoded, manager.Slice(12, encoded.Length).ToArray());
        Assert.Equal(0, manager[12 + encoded.Length]);
        TemplateStorageTestSupport.SetHeader(storage, 20, token);
        Assert.Equal(name, storage.Name(1));
        // 문자 수는 256 미만이어도 UTF-8 바이트 수가 256 이상이면 생성 계약 위반이다
        uint longest = storage.AllocateEmptyManager(276);
        Assert.Throws<ArgumentException>(() => storage.ConstructEmptyManager(longest, new string('가', 86)));
        Assert.Equal(longest, storage.ConstructEmptyManager(longest, new string('가', 85)));
    }

    /// <summary>잘못된 서명의 제거 요청 장치도 manager 바이트가 아니라 실제 경로를 받는다.</summary>
    [Fact]
    public void RejectedSignatureRequestReceivesRealPath()
    {
        string path = Path.Combine(Path.GetTempPath(), "넷스톰 😀", "d", "거부.fort");
        int rejects = 0;
        TemplateFileLoadHooks hooks = LoadEffects() with
        {
            Read = _ => [(byte)'X', 0xfe],
            RejectFile = name =>
            {
                rejects++;
                Assert.Equal(path, name);
            },
            ReadOnly = _ => false,
            SameName = (first, second) => first == second,
        };
        CampaignTemplateStorage storage = new();
        storage.LoadFile(1, new FortTemplate(), path, hooks);
        Assert.Equal(1, rejects);
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>
    /// 같은 한글 경로를 다시 Load하면 이름 비교 장치가 실제 문자열 두 개를 받아 같은 이름으로 판정하고 기존 manager를 유지하며,
    /// 읽기 전용 판정과 읽기 장치도 manager 바이트가 아니라 실제 경로를 받는다.
    /// </summary>
    [Fact]
    public void LoadPassesRealPathsToDevices()
    {
        string path = Path.Combine(Path.GetTempPath(), "넷스톰 😀", "d", "지도.fort");
        byte[] image = FortImage();
        List<string> reads = [], readOnlyChecks = [], comparisons = [];
        TemplateFileLoadHooks hooks = LoadEffects() with
        {
            Read = name =>
            {
                reads.Add(name);
                return image;
            },
            ReadOnly = name =>
            {
                readOnlyChecks.Add(name);
                return false;
            },
            SameName = (first, second) =>
            {
                comparisons.Add(first + "|" + second);
                return first == second;
            },
        };
        CampaignTemplateStorage storage = new();
        storage.LoadFile(1, new FortTemplate(), path, hooks);
        Assert.Equal([path], reads);
        Assert.Equal([path], readOnlyChecks);
        Assert.Equal(path, storage.Name(1));
        storage.LoadFile(1, new FortTemplate(), path, hooks);
        Assert.Equal([path + "|" + path], comparisons);
        Assert.Equal(1, storage.OwnedImages);
    }
}
