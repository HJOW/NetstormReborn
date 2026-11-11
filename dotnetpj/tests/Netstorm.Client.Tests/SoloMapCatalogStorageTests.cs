using Netstorm.Assets;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 편집 맵 목록(<see cref="SoloMapCatalog"/>)이 실제 Template 저장소로 파일을 읽을 때의 분류·메타 두 수명, 파일 명세 키를 읽는 시점, 사라지는 파일,
/// 실제 원본 맵 전체의 필터·행을 검사한다. cpppj <c>SoloMapCatalogTests</c> 중 실제 저장소를 쓰는 6개를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. 임시 섹션 판독기(<c>Probe</c>)를 실제 저장소로 교체한 뒤의 검사다. 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
/// cpppj의 나머지 3개는 옮기지 않았다: 아카이브 읽기(<c>DiskEnumerationAndArchiveRead</c>)는 TAFF 아카이브가 필요해 편집 작업본 검사가 이미 같은 경계를 확인하고,
/// Subscriber 구간 검사(<c>CurrentSubscriberSection</c>)는 저장소 TS1 검사가 같은 경계를 확인하며, Tell 메타(<c>TellMetadataUsesActualFilesAndCurrentNames</c>)는
/// 대화 페이지 파서와 결합해 Tell 맵 메뉴 검사(<c>TellMapMenusTests</c>)가 맡는다.
/// </remarks>
public sealed class SoloMapCatalogStorageTests
{
    /// <summary>전체 지형 파서 없이 파일의 원시 길이 레코드 35개만 분리한다.</summary>
    /// <param name="bytes">Fort 파일 전체.</param>
    private static List<byte[]> FileSections(byte[] bytes)
    {
        Assert.True(bytes.Length >= 2 && bytes[0] == (byte)'F', "목록 검사 원문의 Fort 서명 누락");
        List<byte[]> sections = [];
        int position = 2;
        // 원본 고정 섹션 수만 읽고 월드 내용은 해석하지 않는다
        for (int index = 0; index < 35; index++)
        {
            Assert.True(bytes.Length - position >= 2, "목록 검사 원문 길이 누락");
            int length = bytes[position] + (bytes[position + 1] << 8);
            Assert.True(length >= 2 && length <= bytes.Length - position, "목록 검사 원문 섹션 범위");
            sections.Add(bytes[(position + 2)..(position + length)]);
            position += length;
        }
        return sections;
    }

    /// <summary>
    /// 검사 전용 Fort 이미지다. Subscriber("Me")와 State 외의 불완전한 자료(섹션 4·7의 0xff)는 목록에서 해석하지 않아야 한다. 보호 파일에는 쓰지 않는다.
    /// </summary>
    /// <param name="state">두 번째 섹션(State) 내용.</param>
    private static byte[] FileImage(byte[] state)
    {
        List<byte> bytes = [(byte)'F', 0];
        // 35개 섹션을 길이 레코드로 쌓는다
        for (int index = 0; index < 35; index++)
        {
            byte[] section = index == 0 ? [1, 0, 0, 0, 2, 0, (byte)'M', (byte)'e'] : index == 1 ? state : index is 4 or 7 ? [0xff] : [];
            int length = section.Length + 2;
            bytes.Add((byte)length);
            bytes.Add((byte)(length >> 8));
            bytes.AddRange(section);
        }
        return [.. bytes];
    }

    /// <summary>미정 바이트는 검사에서 명시 입력하고 게임 규칙은 실제 getter와 목록 몸체에 맡긴다.</summary>
    /// <param name="user">현재 사용자 이름(호출 때마다 읽는다).</param>
    /// <param name="allocations">미정 바이트 요청 횟수 누적 장치.</param>
    private static SoloMapCatalogEffects Effects(Func<string> user, Action allocations) => new()
    {
        TypeCount = () => 188,
        AllocationBytes = bytes =>
        {
            allocations();
            bytes.Fill(0xa7);
        },
        UserName = user,
        Translate = text => text,
    };

    /// <summary>검사 전용 설정을 만든다.</summary>
    /// <param name="fortSpec">fortSpec 값.</param>
    private static ConfigInterface Config(string fortSpec)
    {
        ConfigInterface config = new(null);
        config.Startup(new());
        config.Set("fortSpec", fortSpec);
        return config;
    }

    /// <summary>임시 루트에 검사용 파일을 쓴다. 부모 폴더도 만든다.</summary>
    /// <param name="root">임시 루트.</param>
    /// <param name="relative">루트 기준 경로.</param>
    /// <param name="bytes">파일 내용.</param>
    private static string Write(string root, string relative, byte[] bytes)
    {
        string path = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllBytes(path, bytes);
        return path;
    }

    /// <summary>디스크 열거 순서대로 지정한 이름만 남긴 이름 목록이다(특정 드라이브의 알파벳 순서를 가정하지 않는다).</summary>
    /// <param name="files">파일 시스템.</param>
    /// <param name="pattern">열거 패턴.</param>
    /// <param name="wanted">남길 이름들.</param>
    private static List<string> DiskOrder(GameFileSystem files, string pattern, params string[] wanted)
    {
        List<string> order = [];
        // 열거 순서를 그대로 읽으며 기대 이름만 골라 담는다
        foreach (string path in files.MatchDisk(pattern))
        {
            string name = Path.GetFileNameWithoutExtension(path);
            if (wanted.Contains(name))
            {
                order.Add(name);
            }
        }
        return order;
    }

    /// <summary>빈/짧은 State의 실제 생성·확장과 분류·목록 두 수명을 별도 생성 파일로 확인한다.</summary>
    [Fact]
    public void EmptyShortAndMetadataOnly()
    {
        using EditorTempRoot temp = new();
        byte[] editState = new byte[28];
        editState[17] = 2;
        byte[] missionState = (byte[])editState.Clone();
        missionState[17] = 1;
        Write(temp.Root, "case-mission.fort", FileImage(missionState));
        Write(temp.Root, "case-empty.fort", FileImage([]));
        Write(temp.Root, "case-short.fort", FileImage([0]));
        Write(temp.Root, "case-edit.fort", FileImage(editState));
        GameFileSystem files = new(temp.Root, []);
        ConfigInterface config = Config("{local.1}.fort");
        int allocations = 0;
        SoloMapCatalog catalog = new(files, config, Effects(() => "mE", () => allocations++));
        // 빈 State만 미정 바이트를 요청하고 짧은 State는 실제 확장으로 28바이트가 된다
        Assert.Equal(0, catalog.Kind("case-empty"));
        Assert.Equal(1, allocations);
        Assert.Equal(0, catalog.Kind("case-short"));
        Assert.Equal(1, allocations);
        Assert.Equal(2, catalog.Kind("case-edit"));
        Assert.Equal(1, allocations);
        // 빈 State는 분류용과 목록용 실제 Template에서 각각 생성한다
        Assert.Equal(2, catalog.List.Refresh(0));
        Assert.Equal(3, allocations);
        List<string> kindZero = DiskOrder(files, "*.fort", "case-empty", "case-short");
        // 종류 0 두 파일이 디스크 열거 순서대로 놓였는지, 빈 State만 손상 표시인지 행마다 확인한다
        for (int index = 0; index < kindZero.Count; index++)
        {
            Assert.Equal(kindZero[index], catalog.State.Names[index]);
            Assert.Equal(kindZero[index] == "case-empty" ? 2u : 0u, catalog.State.Flags[index]);
        }
        SoloMapRow? row = catalog.List.Row(catalog.State.Names.IndexOf("case-empty"), "", "CASE-EMPTY");
        Assert.NotNull(row);
        Assert.True(row.Checked);
        Assert.Equal("case-empty (~rCORRUPT~.)", row.Label.Replace("%s", "case-empty"));
        // 파일 명세를 한 파일로 좁히면 캐시가 그대로이고 무효화 뒤에야 다시 읽는다
        config.Set("fortSpec", "case-edit.fort");
        Assert.Equal(2, catalog.List.Refresh(2));
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal("case-edit", catalog.State.Names[0]);
        config.Set("fortSpec", "{local.1}.fort");
        config.SetInt("mayEditMissions", 1);
        catalog.Invalidate();
        List<string> editableOrder = DiskOrder(files, "*.fort", "case-edit", "case-mission");
        Assert.Equal(2, editableOrder.Count);
        Assert.Equal(2, catalog.List.Refresh(2));
        // 편집 허용일 때 두 종류가 모두 열거 순서대로 들어왔는지 확인한다
        for (int index = 0; index < editableOrder.Count; index++)
        {
            Assert.Equal(editableOrder[index], catalog.State.Names[index]);
        }
        config.SetInt("mayEditMissions", 0);
        Assert.Equal(2, catalog.List.Refresh(2));
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Throws<InvalidOperationException>(() => catalog.Kind("missing"));
    }

    /// <summary>원본은 분류 뒤 파일을 다시 읽으므로 그 사이 사라진 파일은 실패 flags 1을 가진 행으로 남는다.</summary>
    [Fact]
    public void FileDisappearsBetweenReads()
    {
        using EditorTempRoot temp = new();
        byte[] state = new byte[28];
        state[17] = 2;
        string path = Write(temp.Root, "vanishing.fort", FileImage(state));
        GameFileSystem files = new(temp.Root, []);
        ConfigInterface config = Config("vanishing.fort");
        int reads = 0;
        SoloMapCatalogEffects baseEffects = Effects(() => "Me", () => { });
        SoloMapCatalog catalog = new(files, config, new SoloMapCatalogEffects
        {
            // 이미 읽은 분류 원문은 유지하고 이 검사 전용 파일 한 개만 제거한다
            TypeCount = () =>
            {
                reads++;
                File.Delete(path);
                return 188;
            },
            AllocationBytes = baseEffects.AllocationBytes,
            UserName = () => throw new InvalidOperationException("읽기 실패한 맵의 Subscriber/사용자 비교를 실행하면 안 됩니다"),
            Translate = baseEffects.Translate,
        });
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(1, reads);
        Assert.Equal(1u, catalog.State.Flags[0]);
        SoloMapRow? row = catalog.List.Row(0, "vanishing", "");
        Assert.NotNull(row);
        Assert.Equal("~rvanishing", row.Label);
        Assert.True(row.Checked);
        Assert.True(row.Enabled);
        Assert.False(File.Exists(path));
    }

    /// <summary>원본 004595a0은 마지막 확장자뿐 아니라 파일 이름의 앞선 점도 제거한다.</summary>
    [Fact]
    public void FileSpecStripsAllExtensions()
    {
        using EditorTempRoot temp = new();
        byte[] state = new byte[28];
        state[17] = 2;
        Write(temp.Root, "map.extra.fort", FileImage(state));
        GameFileSystem files = new(temp.Root, []);
        SoloMapCatalog catalog = new(files, Config("map.extra.fort"), Effects(() => "Me", () => { }));
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal("map", catalog.State.Names[0]);
        SoloMapRow? row = catalog.List.Row(0, "map", "");
        Assert.NotNull(row);
        Assert.Equal("map", row.Label);
        Assert.True(row.Checked);
    }

    /// <summary>같은 9바이트 키 변경과 명시 dirty를 분리하고 실제 다른 두 경로의 분류·메타를 현재 시점에 읽는다.</summary>
    [Fact]
    public void CurrentFileSpecAndExplicitInvalidation()
    {
        using EditorTempRoot temp = new();
        byte[] fortState = new byte[28], soloState = new byte[28];
        fortState[17] = 2;
        soloState[17] = 1;
        soloState[0] = 1;
        Write(temp.Root, "fort/map.fort", FileImage(fortState));
        Write(temp.Root, "solo/map.fort", FileImage(soloState));
        GameFileSystem files = new(temp.Root, []);
        ConfigInterface config = Config("fort/{local.1}.fort");
        config.Set("SOLOSpec", "solo/{local.1}.fort");
        config.SetInt("mayEditMissions", 1);
        // 키 버퍼의 내용이 바뀌어도 같은 9바이트 물리 버퍼이고 마지막은 항상 NUL이다
        SoloMapFileSpec spec = new();
        ReadOnlyMemory<byte> original = spec.Buffer;
        Assert.Equal("fortSpec", spec.Current());
        Assert.Equal(0, original.Span[8]);
        spec.SelectSolo();
        Assert.Equal("SOLOSpec", System.Text.Encoding.ASCII.GetString(original.Span[..8]));
        Assert.Equal(0, original.Span[8]);
        Assert.True(TemplateStorageTestSupport.SameStorage(original.Span, spec.Buffer.Span));
        spec.SelectFort();
        int allocations = 0;
        List<string> keys = [];
        SoloMapCatalogEffects baseEffects = Effects(() => "Me", () => allocations++);
        SoloMapCatalog catalog = new(files, config, new SoloMapCatalogEffects
        {
            TypeCount = baseEffects.TypeCount,
            AllocationBytes = baseEffects.AllocationBytes,
            UserName = baseEffects.UserName,
            Translate = baseEffects.Translate,
            // 각 실제 PathSpec 호출 직전의 현재 버퍼 내용을 기록한다
            FileSpecKey = () =>
            {
                keys.Add(spec.Current());
                return spec.Current();
            },
        });
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(0u, catalog.State.Flags[0]);
        Assert.Equal(["fortSpec", "fortSpec", "fortSpec"], keys);
        // 키만 바꾸고 dirty를 올리지 않으면 캐시가 그대로여서 파일을 읽지 않는다
        keys.Clear();
        spec.SelectSolo();
        Assert.Equal(1, catalog.List.Refresh(99));
        Assert.Empty(keys);
        Assert.Equal(0u, catalog.State.Flags[0]);
        Assert.Equal(1, catalog.Kind("map"));
        Assert.Equal(["SOLOSpec"], keys);
        // 명시 무효화 뒤에는 분류·열거·메타가 모두 새 키를 읽는다
        keys.Clear();
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(2u, catalog.State.Flags[0]);
        Assert.Equal(["SOLOSpec", "SOLOSpec", "SOLOSpec"], keys);
        keys.Clear();
        spec.SelectFort();
        Assert.Equal(2, catalog.Kind("map"));
        Assert.Equal(["fortSpec"], keys);
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(2u, catalog.State.Flags[0]);
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(0u, catalog.State.Flags[0]);
        Assert.Equal(0, allocations);
    }

    /// <summary>
    /// 목록의 기본 파일 명세 키를 바꾸면 이후 열거·분류·메타가 새 키의 설정 경로를 읽는다(외부 키를 빌리지 않은 호출).
    /// dirty를 올리지 않으면 캐시가 유지되고 올린 뒤에만 다시 읽는다.
    /// </summary>
    [Fact]
    public void SelectFileSpecSwitchesDefaultKeyForNextRefresh()
    {
        using EditorTempRoot temp = new();
        byte[] state = new byte[28];
        state[17] = 1;
        Write(temp.Root, "fort/map.fort", FileImage(state));
        Write(temp.Root, "solo/other.fort", FileImage(state));
        GameFileSystem files = new(temp.Root, []);
        ConfigInterface config = Config("fort/{local.1}.fort");
        config.Set("SOLOSpec", "solo/{local.1}.fort");
        SoloMapCatalog catalog = new(files, config, Effects(() => "Me", () => { }));
        Assert.Equal("fortSpec", catalog.FileSpec.Current());
        Assert.Equal(1, catalog.List.Refresh(1));
        Assert.Equal("map", catalog.State.Names[0]);
        catalog.SelectFileSpec(true);
        Assert.Equal("SOLOSpec", catalog.FileSpec.Current());
        // 키만 바꾸고 dirty를 올리지 않으면 비어 있지 않은 캐시가 그대로다
        Assert.Equal(1, catalog.List.Refresh(1));
        Assert.Equal("map", catalog.State.Names[0]);
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(1));
        Assert.Equal("other", catalog.State.Names[0]);
        catalog.SelectFileSpec(false);
        Assert.Equal("fortSpec", catalog.FileSpec.Current());
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(1));
        Assert.Equal("map", catalog.State.Names[0]);
    }

    /// <summary>분류 원문을 읽은 뒤 키가 바뀌면 별도 메타 읽기는 새 키를 사용해야 하며 Refresh 시작 때 키를 캡처하면 실패한다.</summary>
    [Fact]
    public void FileSpecChangesBetweenClassificationAndMetadata()
    {
        using EditorTempRoot temp = new();
        byte[] fortState = new byte[28], soloState = new byte[28];
        fortState[17] = 2;
        soloState[17] = 1;
        soloState[0] = 1;
        Write(temp.Root, "fort/map.fort", FileImage(fortState));
        Write(temp.Root, "solo/map.fort", FileImage(soloState));
        GameFileSystem files = new(temp.Root, []);
        ConfigInterface config = Config("fort/{local.1}.fort");
        config.Set("SOLOSpec", "solo/{local.1}.fort");
        SoloMapFileSpec spec = new();
        int allocations = 0, reads = 0;
        List<string> keys = [];
        SoloMapCatalogEffects baseEffects = Effects(() => "Me", () => allocations++);
        SoloMapCatalog catalog = new(files, config, new SoloMapCatalogEffects
        {
            // 분류 파일 읽기 뒤 실제 콜백에서 같은 키 버퍼만 변경한다
            TypeCount = () =>
            {
                reads++;
                if (reads == 1)
                {
                    spec.SelectSolo();
                }
                return 188;
            },
            AllocationBytes = baseEffects.AllocationBytes,
            UserName = baseEffects.UserName,
            Translate = baseEffects.Translate,
            // 열거·분류·별도 메타의 각 실제 호출 시점을 기록한다
            FileSpecKey = () =>
            {
                keys.Add(spec.Current());
                return spec.Current();
            },
        });
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal(2, reads);
        Assert.Equal("map", catalog.State.Names[0]);
        Assert.Equal(2u, catalog.State.Flags[0]);
        Assert.Equal(["fortSpec", "fortSpec", "SOLOSpec"], keys);
        Assert.Equal(0, allocations);
        Assert.Equal(0u, catalog.State.Dirty);
    }

    /// <summary>
    /// 실제 원본 맵 전체를 원본 OS 순서로 분류하고 세 모드와 편집 허용 조합마다 모든 표시 행을 독립 원시 파일 값과 대조한다.
    /// 분류용 읽기와 목록용 초기화가 원본 파일을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void ProtectedFilesAndCache()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        GameFileSystem files = GameFileSystem.Open(data);
        List<(string Name, string Subscriber, byte Kind, byte Flags, string Path, byte[] Bytes)> source = [];
        bool custom = false;
        // 실제 원본 Win32 디스크 열거 순서가 기대 목록 순서다
        foreach (string path in files.MatchDisk("d/*.fort"))
        {
            byte[] bytes = files.ReadAllBytes(path);
            List<byte[]> sections = FileSections(bytes);
            byte[] subscriber = sections[0], state = sections[1];
            Assert.True(subscriber.Length >= 6);
            int length = subscriber[4] + (subscriber[5] << 8);
            Assert.True(length <= subscriber.Length - 6);
            string author = System.Text.Encoding.Latin1.GetString(subscriber, 6, length);
            int nul = author.IndexOf('\0');
            author = nul < 0 ? author : author[..nul];
            string name = Path.GetFileNameWithoutExtension(path);
            custom |= name.Equals("test01", StringComparison.OrdinalIgnoreCase);
            source.Add((name, author, state.Length > 17 ? state[17] : (byte)0, state.Length == 0 ? (byte)0xa1 : state[0], path, bytes));
        }
        Assert.True(custom && source.Count > 15);
        string user = "test author";
        ConfigInterface config = Config("d/{local.1}.fort");
        SoloMapCatalog catalog = new(files, config, Effects(() => user, () => { }));
        // 같은 파일을 실제 세 모드와 편집 허용 조합으로 재분류한다
        foreach (int mode in new[] { 0, 1, 2 })
        {
            // 편집 허용 설정을 끈 경우와 켠 경우를 모두 실행한다
            foreach (int option in new[] { 0, 1 })
            {
                config.SetInt("mayEditMissions", option);
                catalog.Invalidate();
                // 독립 원시 바이트로 필터와 50개 상한의 기대만 만든다
                var expected = source.Where(item => item.Kind == mode || (mode == 2 && option != 0 && item.Kind == 1)).Take(50).ToList();
                int count = catalog.List.Refresh(mode);
                Assert.Equal(expected.Count, count);
                Assert.Equal(0u, catalog.State.Dirty);
                // 이름, 손상, 작성자, 선택, 활성 전체 행을 비교한다
                for (int index = 0; index < expected.Count; index++)
                {
                    var item = expected[index];
                    uint flags = ((item.Flags & 1u) != 0 ? 2u : 0u) | (!item.Subscriber.Equals(user, StringComparison.OrdinalIgnoreCase) ? 4u : 0u);
                    Assert.Equal(item.Name, catalog.State.Names[index]);
                    Assert.Equal(flags, catalog.State.Flags[index]);
                    SoloMapRow? row = catalog.List.Row(index, item.Name, "unused");
                    Assert.NotNull(row);
                    Assert.Equal(item.Name, row.Name);
                    Assert.True(row.Checked && row.Enabled);
                    Assert.Equal(item.Name + ((flags & 2u) != 0 ? " (~rCORRUPT~.)" : ""), row.Label.Replace("%s", item.Name));
                }
                Assert.Null(catalog.List.Row(count, "", ""));
                if (count != 0)
                {
                    // 캐시가 유효하면 파일 명세를 바꿔도 다시 읽지 않는다
                    string?[] saved = [.. catalog.State.Names];
                    config.Set("fortSpec", "missing/{local.1}.fort");
                    Assert.Equal(count, catalog.List.Refresh(99));
                    Assert.Equal(saved, catalog.State.Names);
                    config.Set("fortSpec", "d/{local.1}.fort");
                }
            }
        }
        // 분류용 읽기와 목록용 초기화가 보호 원문을 바꾸지 않는다
        foreach (var item in source)
        {
            Assert.Equal(item.Kind, catalog.Kind(item.Name));
            Assert.Equal(item.Bytes, files.ReadAllBytes(item.Path));
        }
        var customMap = source.First(item => item.Name.Equals("test01", StringComparison.OrdinalIgnoreCase));
        user = customMap.Subscriber;
        config.Set("fortSpec", "d/TEST01.fort");
        catalog.Invalidate();
        Assert.Equal(1, catalog.List.Refresh(2));
        Assert.Equal("TEST01", catalog.State.Names[0]);
        Assert.Equal(0u, catalog.State.Flags[0] & 4u);
        SoloMapRow? customRow = catalog.List.Row(0, "", "test01");
        Assert.NotNull(customRow);
        Assert.True(customRow.Checked && customRow.Enabled);
        Assert.Equal("TEST01", customRow.Name);
    }

    /// <summary>빈 디스크에 아카이브만 있으면 목록은 비고 아카이브 분류는 같은 값을 내는지 검증한다. 2026-11-11 F3 미뤄 둔 전수 재생(아카이브 열거) 이식에서 추가됐다.</summary>
    [Fact]
    public void DiskEnumerationAndArchiveRead()
    {
        // 원본 SoloMapCatalog_DiskEnumerationAndArchiveRead와 같다. 기본 디스크에는 Fort가 없고
        // 보조 아카이브(netstorm.tarc) 내용은 목록에 뜨지 않지만 분류·읽기에는 그대로 쓴다.
        // C# MatchDisk·MatchFinder는 역슬래시를 슬래시로 접으므로 기대값도 슬래시로 둔다(원본과 표기만 다름).
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        using EditorTempRoot temp = new();
        GameFileSystem files = GameFileSystem.Open(temp.Root, data);
        Assert.Empty(files.MatchDisk("d/*.fort"));
        SoloMapCatalog catalog = new(files, Config("d/{local.1}.fort"), Effects(() => "", () => { }));
        Assert.Equal(0, catalog.List.Refresh(2));
        int reads = 0;
        foreach (string path in files.MatchFinder("d/*.fort"))
        {
            List<byte[]> sections = FileSections(files.ReadAllBytes(path));
            byte[] state = sections[1];
            byte kind = state.Length > 17 ? state[17] : (byte)0;
            string stem = Path.GetFileNameWithoutExtension(path.Replace('\\', '/'));
            Assert.Equal(kind, catalog.Kind(stem));
            ++reads;
        }

        Assert.True(reads > 0);
        Assert.Equal(["d/absent.fort"], files.MatchDisk("d\\absent.fort"));
    }
}
