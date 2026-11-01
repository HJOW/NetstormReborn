using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Template 저장소의 파일 범위(TS3)를 검사한다: 실제 파일 이미지 읽기(<c>InstallFile</c>), 원문 Load(<c>LoadFile</c>)의 헤더 공개·정상 false·예외·재조회 순서,
/// 현재 섹션의 파일 저장(<c>SaveFile</c>), 이름 부모 연결(<c>BindCurrentName</c>), 그리고 원본 데이터의 .fort 433개를 읽고 다시 저장해 섹션과 flag를 대조하는 왕복.
/// cpppj의 DataManagerReadTests(2), CampaignTemplateLoadFileTests(9), TemplateSerializationGlobalsTests(1 중 InstallFile 부분),
/// CampaignTemplateDynamicTests(2 중 파일 저장 부분)를 옮겼고 <c>FortTemplate::Parse</c>는 <see cref="FortTemplate.ReadSections"/>로 대체했다.
/// 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다 (0.133 TS3, 2026-11-01).
/// </summary>
public sealed class CampaignTemplateFileTests
{
    /// <summary>파일의 35개 길이 레코드만 구성한다. 두 실제 payload 외에는 길이 2의 빈 섹션이며 원본 출력 기대를 이 생성기에서 계산하지 않는다.</summary>
    /// <param name="subscriber">Subscriber 섹션 payload.</param>
    /// <param name="state">State 섹션 payload.</param>
    private static byte[] FileImage(byte[]? subscriber = null, byte[]? state = null)
    {
        subscriber ??= [7, 0, 0, 0, 1, 0, (byte)'A'];
        state ??= [3, 5, 7];
        List<byte> result = [(byte)'F', 0xfe];
        // 두 실제 payload 외에는 길이 2의 빈 섹션이다
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            byte[] payload = index == 0 ? subscriber : index == 1 ? state : [];
            int length = payload.Length + 2;
            result.Add((byte)length);
            result.Add((byte)(length >> 8));
            result.AddRange(payload);
        }
        return [.. result];
    }

    /// <summary>명시 입력의 실제 하위 경계를 기본으로 연결하며 읽기 결과만 검사마다 바꾼다.</summary>
    /// <param name="image">읽기 장치가 돌려줄 파일 바이트.</param>
    private static TemplateFileLoadHooks Effects(byte[] image) => new()
    {
        Read = _ => image,
        // 합성 공식 ASCII 파일명의 실제 비교 모듈을 재사용한다
        SameName = (first, second) => RawDataManagerSelection.CompareSectionNames(first, second) == 0,
        WrongName = () => throw new InvalidOperationException("검사에서 선택하지 않은 이름 불일치 진단"),
        ReadOnly = _ => false,
        TypeCount = () => 188,
        TooManyTypes = () => throw new InvalidOperationException("검사에서 선택하지 않은 TYPE 진단"),
        RejectFile = _ => throw new InvalidOperationException("검사에서 선택하지 않은 파일 제거 요청"),
        CurrentSlot = () => 1u,
        RefreshTitle = () => { },
    };

    /// <summary>저장 억제·숨김 전역을 모두 0으로 둔 저장 상태(9칸 숨김 표)다.</summary>
    private static DataManagerFileSaveState OpenSaveState() => new()
    {
        ReadOnlyForts = () => 0,
        LocalOwner = () => 0,
        HiddenPlayers = () => new byte[9],
    };

    /// <summary>타입 개수만 연결한 이미지 장치다. 진단은 모두 실패로 본다.</summary>
    private static FortImageHooks ImageHooks() => new()
    {
        TypeCount = () => 188,
        Assertion = _ => throw new InvalidOperationException("예상 밖 이미지 진단"),
    };

    /// <summary>저장된 바이트를 모으고 권한 요청(0x180)을 기록하는 파일 쓰기 장치다.</summary>
    /// <param name="name">기대 파일 이름.</param>
    /// <param name="saved">저장된 바이트를 받는 목록.</param>
    /// <param name="permissions">권한 요청 횟수를 받는 카운터.</param>
    private static TemplateFileWriteHooks WriteDevice(string name, List<byte> saved, int[] permissions) => new(
        (target, mode) =>
        {
            Assert.Equal(name, target);
            Assert.Equal(0x180u, mode);
            permissions[0]++;
        },
        (target, bytes) =>
        {
            Assert.Equal(name, target);
            saved.Clear();
            saved.AddRange(bytes.ToArray());
        });

    /// <summary>목표 11개 파일(Early 6, 캠페인 1-1~1-2, 데모 3)을 읽으면 같은 섹션·목록·커서·서명 판정이 공개된다.</summary>
    [Fact]
    public void ReadTargetElevenFiles()
    {
        string[] forts = ["bridgethegap", "secretworkshop", "capturethepriest", "tacticalcombat", "subtledefense", "rawpower", "thewarbegins", "masterofwhirligigs", "demostormrages", "guardmyback", "dissolvedalliance"];
        // 매 파일은 새 owner와 같은 빌림만 만들며 다른 파일의 현재 자료를 재사용하지 않는다
        foreach (string name in forts)
        {
            byte[] bytes = RealFortBytes(name);
            FortTemplate expected = FortTemplate.ReadSections(bytes);
            CampaignTemplateStorage storage = new();
            storage.InstallFile(1, new FortTemplate(), bytes, name);
            storage.Borrow(8, 1);
            // 빌림도 같은 실제 가상 서명 판정 결과를 읽는다
            Assert.Equal((0x46u, bytes[1] == 0xfe ? 1u : 0u), storage.FileHeader(1));
            Assert.Equal(storage.FileHeader(1), storage.FileHeader(8));
            Assert.Equal(expected.Sections.Select(section => section.ToArray()), storage.Template(1).Sections.Select(section => section.ToArray()));
            Assert.Same(storage.Template(1), storage.Template(8));
            Assert.Equal("Terr19", storage.CurrentSection(1));
            Assert.Equal(FortTemplate.SectionNames, storage.SectionOrder(8));
            PlayerTemplateStateStream state = storage.State(8);
            FortTerritoryStream territory = storage.Territory(8);
            Assert.Equal(0u, state.Position);
            Assert.Equal(0u, territory.Position);
            Assert.Equal((uint)storage.Template(1).Section("State").Length, state.End);
            Assert.Equal((uint)storage.Template(1).Section("Territory").Length, territory.End);
            Assert.True(SameStorage(territory.Bytes.Span, storage.Template(1).Section("Territory")));
            storage.Clear(8);
            storage.ClearAll();
            Assert.Equal(0, storage.OwnedImages);
        }
    }

    /// <summary>값형 설치의 서명 판정은 읽기 전(0, 0xffffffff)이고 파일 설치는 (0x46, flag)다. 잘못된 서명과 없는 파일은 manager를 남기지 않는다.</summary>
    [Fact]
    public void FileHeaderAndRejectedImageOwnership()
    {
        byte[] input = RealFortBytes("thewarbegins");
        FortTemplate fort = FortTemplate.ReadSections(input);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort.Clone(), "thewarbegins");
        Assert.Equal((0u, 0xffffffffu), storage.FileHeader(1));
        byte[] modified = [.. input];
        modified[1] = 0xfe;
        storage.InstallFile(1, fort.Clone(), modified, "thewarbegins");
        storage.Borrow(8, 1);
        Assert.Equal((0x46u, 1u), storage.FileHeader(8));
        storage.Clear(8);
        modified[0] = (byte)'X';
        // 실제 파일 삭제 대신 보호 원문 거부 예외를 확인한다
        Assert.Throws<InvalidDataException>(() => storage.InstallFile(1, fort.Clone(), modified, "thewarbegins"));
        Assert.Equal(0, storage.OwnedImages);
        Assert.Throws<InvalidOperationException>(() => storage.InstallFile(1, fort.Clone(), [], "missing"));
        Assert.Equal(0, storage.OwnedImages);
        // 입력 원문은 바뀌지 않았다
        Assert.Equal(input, RealFortBytes("thewarbegins"));
    }

    /// <summary>기존 파일의 이름 비교는 실제 State/current/노드 수명/헤더를 모두 그대로 둔다.</summary>
    [Fact]
    public void LoadExistingNamePreservesActualOwnerAndCursor()
    {
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, new FortTemplate(), FileImage(), "Keep.fort");
        TemplateWriteStream state = storage.SelectSection(1, "State", 1);
        storage.State(1).Position = 2;
        byte[] allocation = ArrayOf(storage.SectionBuffer(state));
        SetHeader(storage, 20 + 8, 7);
        SetHeader(storage, 20 + 12, 0xaabbccdd);
        SetHeader(storage, 20 + 16, 42);
        byte[] before = storage.Headers.Span.Slice(20, 20).ToArray();
        int compares = 0, diagnostics = 0;
        TemplateFileLoadHooks hooks = new()
        {
            SameName = (first, second) =>
            {
                compares++;
                Assert.Equal("Keep.fort", first);
                return RawDataManagerSelection.CompareSectionNames(first, second) == 0;
            },
            WrongName = () => diagnostics++,
        };
        storage.LoadFile(1, new FortTemplate(), "KEEP.FORT", hooks);
        storage.LoadFile(1, new FortTemplate(), "Other.fort", hooks);
        Assert.Equal(2, compares);
        Assert.Equal(1, diagnostics);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal("State", storage.CurrentSection(1));
        Assert.Equal(before, storage.Headers.Span.Slice(20, 20).ToArray());
        Assert.Equal(2u, Observe(storage, state).Position);
        Assert.Same(allocation, ArrayOf(storage.SectionBuffer(state)));
        Assert.True(SameStorage(storage.Template(1).Section("State"), storage.SectionBuffer(state).Span));
    }

    /// <summary>생성/버전 0/현재 header 공개 뒤 readonly, +16 초기화 뒤 실제 read, 서명 검사, 현재 슬롯 재조회 뒤 제목 순서이고 읽은 바이트가 그대로 저장된다.</summary>
    [Fact]
    public void LoadPublishesManagerBeforeReadAndKeepsActualBytes()
    {
        CampaignTemplateStorage storage = new();
        storage.SerializationVersion = 0x12345678;
        SetHeader(storage, 20 + 4, 7);
        SetHeader(storage, 20 + 12, 0x55667788);
        SetHeader(storage, 20 + 16, 99);
        string trace = "";
        uint manager = 0;
        int types = 256;
        TemplateFileLoadHooks hooks = Effects(FileImage()) with
        {
            ReadOnly = name =>
            {
                trace += 'R';
                manager = Header(storage, 20);
                Assert.Equal("fresh.fort", name);
                Assert.NotEqual(0u, manager);
                Assert.Equal(0u, Header(storage, 20 + 4));
                Assert.Equal(0x55667788u, Header(storage, 20 + 12));
                Assert.Equal(99u, Header(storage, 20 + 16));
                Assert.Equal(1, storage.OwnedImages);
                Assert.Equal(0u, storage.SerializationVersion);
                return true;
            },
            Read = name =>
            {
                trace += 'F';
                Assert.Equal("fresh.fort", name);
                Assert.Equal(manager, Header(storage, 20));
                Assert.Equal(1u, Header(storage, 20 + 8));
                Assert.Equal(0u, Header(storage, 20 + 16));
                types = 188;
                return FileImage();
            },
            TypeCount = () =>
            {
                trace += 'Y';
                return types;
            },
            CurrentSlot = () =>
            {
                trace += 'I';
                Assert.Equal(35, storage.SectionOrder(1).Count);
                return 1u;
            },
            RefreshTitle = () =>
            {
                trace += 'T';
                Assert.Equal(manager, Header(storage, 20));
                Assert.Equal(5, storage.Template(1).Section("State")[1]);
            },
        };
        storage.LoadFile(1, new FortTemplate(), "fresh.fort", hooks);
        Assert.Equal("RFYIT", trace);
        Assert.Equal(((uint)'F', 1u), storage.FileHeader(1));
        TemplateWriteStream state = storage.FindSection(1, "State");
        Assert.True(SameStorage(storage.Template(1).Section("State"), storage.SectionBuffer(state).Span));
        Assert.Equal(0u, storage.State(1).Position);
        Assert.Equal(3u, storage.State(1).End);

        List<byte> saved = [];
        int[] permissions = [0];
        storage.SaveFile(1, OpenSaveState(), ImageHooks(), WriteDevice("fresh.fort", saved, permissions));
        Assert.Equal(1, permissions[0]);
        Assert.Equal(FileImage(), saved.ToArray());
    }

    /// <summary>stale readonly 비영은 판정을 생략하고 1로 쓰며 다른 현재 슬롯에는 제목 장치를 요구하지 않는다.</summary>
    [Fact]
    public void LoadPresetReadOnlyAndNonCurrentSelectedEffects()
    {
        CampaignTemplateStorage storage = new();
        SetHeader(storage, 20 + 8, 0x87654321);
        TemplateFileLoadHooks hooks = Effects(FileImage()) with { ReadOnly = null, CurrentSlot = () => 2u, RefreshTitle = null };
        storage.LoadFile(1, new FortTemplate(), "preset.fort", hooks);
        Assert.NotEqual(0u, Header(storage, 20));
        Assert.Equal(1u, Header(storage, 20 + 8));
        Assert.Equal(1, storage.OwnedImages);
    }

    /// <summary>원문 정상 false는 실제 owner를 반납하지만 부가 필드와 장치가 수행한 쓰기를 보존한다.</summary>
    [Fact]
    public void LoadNormalFalseDestroysOnlyCurrentManager()
    {
        CampaignTemplateStorage storage = new();
        SetHeader(storage, 20 + 12, 0x10203040);
        TemplateFileLoadHooks hooks = Effects([]) with
        {
            TypeCount = null,
            CurrentSlot = null,
            RefreshTitle = null,
            Read = _ =>
            {
                Assert.NotEqual(0u, Header(storage, 20));
                Assert.Equal(1, storage.OwnedImages);
                Assert.Equal(0u, Header(storage, 20 + 16));
                SetHeader(storage, 20 + 4, 17);
                SetHeader(storage, 20 + 8, 3);
                SetHeader(storage, 20 + 16, 19);
                storage.SerializationVersion = 27;
                return [];
            },
        };
        storage.LoadFile(1, new FortTemplate(), "absent.fort", hooks);
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(17u, Header(storage, 20 + 4));
        Assert.Equal(3u, Header(storage, 20 + 8));
        Assert.Equal(0x10203040u, Header(storage, 20 + 12));
        Assert.Equal(19u, Header(storage, 20 + 16));
        Assert.Equal(0, storage.OwnedImages);
        Assert.Equal(27u, storage.SerializationVersion);
        Assert.Throws<InvalidOperationException>(() => storage.Template(1));
    }

    /// <summary>잘못된 서명 요청이 정상 복귀하면 같은 실제 서명 필드를 본 뒤 부모의 정상 false 정리를 따른다.</summary>
    [Fact]
    public void LoadRejectedSignatureHasNormalFalseCleanup()
    {
        CampaignTemplateStorage storage = new();
        int rejects = 0;
        TemplateFileLoadHooks hooks = Effects([(byte)'X', 0xfe]) with
        {
            RejectFile = name =>
            {
                rejects++;
                Assert.Equal("reject.fort", name);
                Assert.NotEqual(0u, Header(storage, 20));
                Assert.Equal(((uint)'X', 1u), storage.FileHeader(1));
                SetHeader(storage, 20 + 12, 57);
                SetHeader(storage, 20 + 16, 59);
            },
        };
        storage.LoadFile(1, new FortTemplate(), "reject.fort", hooks);
        Assert.Equal(1, rejects);
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(57u, Header(storage, 20 + 12));
        Assert.Equal(59u, Header(storage, 20 + 16));
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>파일 장치/선택된 진단의 예외는 정상 false가 아니며 이미 공개한 manager를 롤백하지 않는다.</summary>
    [Fact]
    public void LoadExceptionKeepsPublishedManagerAndMissingEffects()
    {
        CampaignTemplateStorage storage = new();
        TemplateFileLoadHooks hooks = Effects(FileImage()) with { Read = _ => throw new IOException("명시 파일 장치 중단") };
        Assert.Throws<IOException>(() => storage.LoadFile(1, new FortTemplate(), "stopped.fort", hooks));
        Assert.NotEqual(0u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(0u, Header(storage, 20 + 4));
        Assert.Equal(0u, Header(storage, 20 + 16));
        uint manager = Header(storage, 20);
        TemplateFileLoadHooks existing = new() { SameName = (first, second) => first == second };
        storage.LoadFile(1, new FortTemplate(), "stopped.fort", existing);
        Assert.Equal(manager, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
        storage.Clear(1);
        Assert.Equal(0, storage.OwnedImages);
        // 실제 readonly의 미연결 위치에서만 중단한다
        Assert.Throws<InvalidOperationException>(() => storage.LoadFile(1, new FortTemplate(), "unbound.fort", new TemplateFileLoadHooks()));
        Assert.NotEqual(0u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
    }

    /// <summary>첫 섹션 append 뒤의 물리 잘림은 이미 붙인 같은 노드와 pos를 남긴다.</summary>
    [Fact]
    public void LoadPartialReadKeepsAttachedActualNode()
    {
        CampaignTemplateStorage storage = new();
        TemplateFileLoadHooks hooks = Effects([(byte)'F', 0xfe, 5, 0, 11, 13, 17, 8, 0]);
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.LoadFile(1, new FortTemplate(), "partial.fort", hooks));
        Assert.NotEqual(0u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(new[] { "Subscriber" }, storage.SectionOrder(1));
        // 요새는 35칸 빈 섹션으로 시작했고, 다음 동기화(검색)도 읽지 않은 표준 위치를 목록에 되살리지 않는다
        Assert.Equal(35, storage.Template(1).Sections.Count);
        TemplateWriteStream stream = storage.FindSection(1, "Subscriber");
        Assert.Equal(new[] { "Subscriber" }, storage.SectionOrder(1));
        ReadOnlySpan<byte> bytes = storage.Template(1).Section("Subscriber");
        Assert.NotEqual(0u, stream.Section);
        Assert.Equal(new byte[] { 11, 13, 17 }, bytes.ToArray());
        Assert.Equal(3u, Observe(storage, stream).End);
        Assert.Equal(0u, Observe(storage, stream).Position);
        storage.Clear(1);
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>readonly 장치가 header를 다른 실제 owner로 돌리면 그 manager에 원문 병합을 수행하고, 빈 새 섹션은 기존 섹션을 덮지 않는다.</summary>
    [Fact]
    public void LoadRereadsRedirectedManagerAndPreservesEmptyExistingSection()
    {
        CampaignTemplateStorage storage = new();
        storage.InstallFile(2, new FortTemplate(), FileImage([21, 22, 23], [31, 32, 33, 34]), "target.fort");
        uint target = Header(storage, 40);
        TemplateWriteStream oldSubscriber = storage.FindSection(2, "Subscriber");
        TemplateWriteStream oldState = storage.SelectSection(2, "State", 1);
        storage.State(2).Position = 2;
        byte[] allocation = ArrayOf(storage.SectionBuffer(oldState));
        uint abandoned = 0;
        TemplateFileLoadHooks hooks = Effects(FileImage([41, 42], [])) with
        {
            ReadOnly = name =>
            {
                Assert.Equal("requested.fort", name);
                abandoned = Header(storage, 20);
                SetHeader(storage, 20, target);
                return false;
            },
            Read = name =>
            {
                Assert.Equal("target.fort", name);
                Assert.Equal(target, Header(storage, 20));
                return FileImage([41, 42], []);
            },
        };
        storage.LoadFile(1, new FortTemplate(), "requested.fort", hooks);
        Assert.NotEqual(0u, abandoned);
        Assert.NotEqual(target, abandoned);
        Assert.Equal(target, Header(storage, 20));
        Assert.Equal(2, storage.OwnedImages);
        Assert.Same(storage.Template(2), storage.Template(1));
        TemplateWriteStream retained = storage.FindSection(1, "State");
        Assert.Equal(oldState.Section, retained.Section);
        Assert.Equal(oldState.Generation, retained.Generation);
        Assert.Same(allocation, ArrayOf(storage.SectionBuffer(retained)));
        Assert.Equal(2u, Observe(storage, retained).Position);
        Assert.Equal(4, storage.Template(2).Section("State").Length);
        Assert.Equal(31, storage.Template(2).Section("State")[0]);
        Assert.Equal(2, storage.Template(1).Section("Subscriber").Length);
        Assert.Equal(41, storage.Template(1).Section("Subscriber")[0]);
        // 새로 읽은 Subscriber가 기존 것을 교체했으므로 이전 참조는 거부된다
        Assert.Throws<InvalidOperationException>(() => storage.AccessSection(oldSubscriber, (_, _) => { }));
        Assert.Equal(35, storage.SectionOrder(1).Count);
        // 명시로 보관한 원래 owner도 검사 끝에 반납한다
        SetHeader(storage, 20, abandoned);
        storage.Clear(1);
        storage.Clear(2);
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>읽기 장치 뒤 header가 0이면 성공한 manager를 유지해도 제목/현재 슬롯을 선행 조회하지 않는다.</summary>
    [Fact]
    public void LoadPostReadNullHeaderSkipsTitleAndKeepsOwner()
    {
        CampaignTemplateStorage storage = new();
        uint saved = 0;
        TemplateFileLoadHooks hooks = Effects(FileImage()) with
        {
            Read = _ =>
            {
                saved = Header(storage, 20);
                SetHeader(storage, 20, 0);
                return FileImage();
            },
            CurrentSlot = null,
            RefreshTitle = null,
        };
        storage.LoadFile(1, new FortTemplate(), "detached.fort", hooks);
        Assert.NotEqual(0u, saved);
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(1, storage.OwnedImages);
        SetHeader(storage, 20, saved);
        Assert.Equal(35, storage.SectionOrder(1).Count);
        storage.Clear(1);
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>읽기 거부 뒤에도 공유 직렬화 전역에 이미 쓴 생성자의 0은 되돌리지 않으며 파일 설치도 같은 전역에 0을 쓴다.</summary>
    [Fact]
    public void InstallFileWritesSharedVersionAndKeepsItOnRejection()
    {
        FortSerializationGlobals globals = new() { Version = 0x89abcdef };
        CampaignTemplateStorage first = new(globals);
        CampaignTemplateStorage second = new(globals);
        first.Install(0, new FortTemplate(), "first");
        Assert.Equal(0u, globals.Version);
        Assert.Equal(0u, second.SerializationVersion);
        second.SerializationVersion = 2;
        second.Install(1, new FortTemplate(), "second");
        Assert.Equal(0u, first.SerializationVersion);
        Assert.Equal(0u, second.SerializationVersion);
        globals.Version = 0xffffffff;
        first.InstallFile(2, new FortTemplate(), FileImage(), "shared.fort");
        Assert.Equal(0u, first.SerializationVersion);
        second.SerializationVersion = 0x12345678;
        Assert.Throws<InvalidOperationException>(() => second.InstallFile(2, new FortTemplate(), [], "bad.fort"));
        // 파일 거부 뒤에도 이미 수행한 생성자 쓰기를 되돌리지 않는다
        Assert.Equal(0u, first.SerializationVersion);
    }

    /// <summary>Temp에서 바뀐 20개 영역 섹션을 포함한 현재 섹션 전체가 35개 이름 순서의 길이 레코드로 저장된다.</summary>
    [Fact]
    public void SaveFileWritesCurrentDynamicSections()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "all-territories.fort");
        TemplateWriteStream[] references = new TemplateWriteStream[20];
        for (uint index = 0; index < 20; index++)
        {
            TemplateWriteStream temporary = storage.CreateSection(1, "Temp");
            storage.WriteSection(temporary, new byte[] { 2, (byte)index, (byte)(index + 40) });
            references[index] = storage.RenameSection(1, $"Terr{index:00}", "Temp");
        }
        List<byte> saved = [];
        int[] permissions = [0];
        storage.SaveFile(1, OpenSaveState(), ImageHooks(), WriteDevice("all-territories.fort", saved, permissions));
        Assert.Equal(1, permissions[0]);
        Assert.Equal(132, saved.Count);
        Assert.Equal((byte)'F', saved[0]);
        Assert.Equal(0, saved[1]);
        Assert.Equal("Terr19", storage.CurrentSection(1));
        int at = 2;
        // 각 WORD 길이와 현재 buffer payload를 35개 이름 순서대로 대조한다
        foreach (string name in FortTemplate.SectionNames)
        {
            int length = saved[at] | (saved[at + 1] << 8);
            Assert.True(length >= 2);
            at += 2;
            byte[] expected = storage.Template(1).Section(name).ToArray();
            Assert.Equal(expected.Length + 2, length);
            Assert.Equal(expected, saved.GetRange(at, expected.Length));
            at += expected.Length;
        }
        Assert.Equal(saved.Count, at);
        Assert.Equal(35, storage.SectionOrder(1).Count);
        // 저장은 current를 바꿀 뿐 쓰기 참조의 수명을 끊지 않는다
        storage.WriteSection(references[^1], new byte[] { 99 });
        Assert.Equal(99, storage.Template(1).Section("Terr19")[^1]);
    }

    /// <summary>
    /// 저장 부모는 저장 억제 전역이 비영이면 이미지를 만들기만 하고 쓰지 않으며, 로컬 숨김이면 건너뛰고, 숨김 표 범위를 넘으면 거부한다.
    /// 호출자의 이미지 장치 묶음은 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void SaveFileGatesAndLeavesCallerHooksUntouched()
    {
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, new FortTemplate(), FileImage(), "gate.fort");
        FortImageHooks image = ImageHooks();
        List<byte> saved = [];
        int[] permissions = [0];
        TemplateFileWriteHooks device = WriteDevice("gate.fort", saved, permissions);

        DataManagerFileSaveState blocked = OpenSaveState();
        blocked.ReadOnlyForts = () => 1;
        storage.SaveFile(1, blocked, image, device);
        Assert.Equal(0, permissions[0]);
        Assert.Empty(saved);

        DataManagerFileSaveState hidden = OpenSaveState();
        hidden.HiddenPlayers = () => new byte[] { 0, 1, 0, 0, 0, 0, 0, 0, 0 };
        hidden.LocalOwner = () => 1;
        storage.SaveFile(1, hidden, image, device);
        Assert.Equal(0, permissions[0]);
        Assert.Empty(saved);

        DataManagerFileSaveState outside = OpenSaveState();
        outside.LocalOwner = () => 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.SaveFile(1, outside, image, device));

        // 장치 묶음의 서명/섹션 조회는 이 호출이 채운 사본에만 있다
        Assert.Null(image.SignatureFlag);
        Assert.Null(image.Section);
        storage.SaveFile(1, OpenSaveState(), image, device);
        Assert.Equal(1, permissions[0]);
        Assert.Equal(FileImage(), saved.ToArray());
        Assert.Null(image.SignatureFlag);
        // 쓰기·권한 장치가 없으면 해당 지점에서 명시 오류다
        TemplateFileWriteHooks missing = new(null!, null!);
        Assert.Throws<InvalidOperationException>(() => storage.SaveFile(1, OpenSaveState(), image, missing));

        // 빌린 칸에서 저장하면 그 칸이 가리키는 소유자(칸 2)의 manager를 저장한다
        storage.InstallFile(2, new FortTemplate(), FileImage([5], [6]), "borrowed.fort");
        storage.Borrow(8, 2);
        List<byte> viaBorrow = [];
        storage.SaveFile(8, OpenSaveState(), image, WriteDevice("borrowed.fort", viaBorrow, [0]));
        Assert.Equal(FileImage([5], [6]), viaBorrow.ToArray());
    }

    /// <summary>저장은 35개 이름을 모두 조회하므로 목록에 없는 섹션이 있으면 가짜 빈 섹션 없이 누락 진단으로 멈추고 파일을 쓰지 않는다.</summary>
    [Fact]
    public void SaveFileRejectsMissingStandardSection()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, new FortTemplate(), "short.fort");
        List<byte> saved = [];
        int[] permissions = [0];
        Assert.Throws<InvalidOperationException>(() => storage.SaveFile(1, OpenSaveState(), ImageHooks(), WriteDevice("short.fort", saved, permissions)));
        Assert.Empty(saved);
        Assert.Equal(0, permissions[0]);
    }

    /// <summary>서명 flag는 파일을 읽은 flag를 그대로 따르고, 값형 설치(미읽기)의 저장은 flag 0이며 FE 판정을 0으로 정규화한다.</summary>
    [Fact]
    public void SaveFileSignatureFlagFollowsFileHeader()
    {
        CampaignTemplateStorage storage = new();
        storage.InstallFile(1, new FortTemplate(), FileImage(), "flagged.fort");
        List<byte> saved = [];
        storage.SaveFile(1, OpenSaveState(), ImageHooks(), WriteDevice("flagged.fort", saved, [0]));
        Assert.Equal(new byte[] { (byte)'F', 0xfe }, saved.GetRange(0, 2));

        // 값형 설치: 서명 DWORD가 0/0xffffffff로 시작하고 저장 시 'F'와 flag 0으로 정규화된다
        storage.Install(2, EmptyFort(), "plain.fort");
        Assert.Equal((0u, 0xffffffffu), storage.FileHeader(2));
        storage.SaveFile(2, OpenSaveState(), ImageHooks(), WriteDevice("plain.fort", saved, [0]));
        Assert.Equal(new byte[] { (byte)'F', 0 }, saved.GetRange(0, 2));
        Assert.Equal((0x46u, 0u), storage.FileHeader(2));
    }

    /// <summary>파일의 타입 수가 256 이상이면 진단 장치로 가고, 장치가 반환하면 읽기를 계속한다.</summary>
    [Fact]
    public void InstallFileWithManyTypesReportsAndContinuesOnlyWhenDeviceReturns()
    {
        CampaignTemplateStorage storage = new();
        // 기본 InstallFile은 진단에서 예외를 던진다
        Assert.Throws<InvalidOperationException>(() => storage.InstallFile(1, new FortTemplate(), FileImage(), "types.fort", typeCount: 256));
        Assert.Equal(0, storage.OwnedImages);
        // LoadFile은 진단 장치가 반환하면 서명 읽기를 계속한다
        int diagnostics = 0;
        TemplateFileLoadHooks hooks = Effects(FileImage()) with { TypeCount = () => 300, TooManyTypes = () => diagnostics++ };
        storage.LoadFile(1, new FortTemplate(), "types.fort", hooks);
        Assert.Equal(1, diagnostics);
        Assert.Equal(1, storage.OwnedImages);
    }

    /// <summary>이름 부모 연결은 같은 헤더 칸과 manager+c의 256바이트 이름 배열을 그대로 빌려 주고, 없는 칸·manager는 거부한다.</summary>
    [Fact]
    public void BindCurrentNameBorrowsSameHeaderAndManagerName()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, new FortTemplate(), "current.fort");
        FortCurrentNameHooks hooks = new();
        Assert.Same(hooks, storage.BindCurrentName(hooks));
        // 헤더 조회는 저장소 헤더의 같은 20바이트다: 한쪽에 쓰면 다른 쪽에서 보인다
        Span<byte> header = hooks.Header!(1);
        Assert.Equal(20, header.Length);
        Assert.Equal(storage.Headers.Span.Slice(20, 20).ToArray(), header.ToArray());
        header[16] = 0x5a;
        Assert.Equal(0x5au, Header(storage, 20 + 16));
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Header!(9));
        // 이름 조회는 manager+c의 256바이트 배열이며 쓰면 현재 이름이 바뀐다
        uint manager = Header(storage, 20);
        Span<byte> name = hooks.ManagerName!(manager);
        Assert.Equal(256, name.Length);
        Assert.Equal("current.fort", System.Text.Encoding.ASCII.GetString(name[..name.IndexOf((byte)0)]));
        name[0] = (byte)'X';
        Assert.Equal("Xurrent.fort", storage.Name(1));
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.ManagerName!(manager + 100));
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.ManagerName!(0));
    }

    /// <summary>원본 데이터의 .fort 파일을 확장자를 뺀 이름으로 읽는다.</summary>
    /// <param name="name">확장자를 뺀 파일 이름.</param>
    private static byte[] RealFortBytes(string name)
    {
        string? directory = GameDataLocator.FindDataDirectory();
        Assert.True(directory is not null, "클론 데이터 폴더가 없습니다");
        return GameFileSystem.Open(directory!).ReadAllBytes("d/" + name + ".fort");
    }

    /// <summary>
    /// 데이터 폴더의 .fort 전체(원본 기준 433개)를 <c>InstallFile</c>로 읽고 <c>SaveFile</c>로 다시 쓴 뒤 섹션과 flag를 입력 파일 자체와 대조한다.
    /// 기대값이 입력 파일에서 나오므로 cpppj 없이 모든 파일을 덮는다. 어긋나는 파일은 원인을 분류해 모두 모아 보고한다.
    /// </summary>
    [Fact]
    public void AllOriginalFortFilesRoundTripThroughInstallAndSave()
    {
        string? directory = GameDataLocator.FindDataDirectory();
        Assert.True(directory is not null, "클론 데이터 폴더가 없습니다");
        GameFileSystem files = GameFileSystem.Open(directory!);
        IReadOnlyList<string> names = files.Find("d/*.fort");
        Assert.True(names.Count >= 433, $"원본 .fort 수가 433보다 적다: {names.Count}");
        List<string> problems = [];
        // 파일마다 새 저장소에서 읽고 저장한다
        foreach (string path in names)
        {
            byte[] bytes = files.ReadAllBytes(path);
            FortTemplate original = FortTemplate.ReadSections(bytes);
            try
            {
                CampaignTemplateStorage storage = new();
                storage.InstallFile(1, new FortTemplate(), bytes, path);
                List<byte> saved = [];
                storage.SaveFile(1, OpenSaveState(), ImageHooks(), WriteDevice(path, saved, [0]));
                FortTemplate rewritten = FortTemplate.ReadSections(saved.ToArray());
                byte expectedFlag = original.Flag == 0xfe ? (byte)0xfe : (byte)0;
                if (rewritten.Flag != expectedFlag)
                {
                    problems.Add($"{path}: flag {original.Flag:x2} -> {rewritten.Flag:x2}");
                }
                else if (rewritten.Sections.Count != FortTemplate.SectionCount || rewritten.TrailingBytes != 0)
                {
                    problems.Add($"{path}: 저장 결과 섹션 {rewritten.Sections.Count}개, 뒤꼬리 {rewritten.TrailingBytes}바이트");
                }
                else
                {
                    // 원본에 있던 섹션은 같아야 하고 원본에 없던 꼬리 섹션은 빈 섹션이어야 한다
                    for (int index = 0; index < FortTemplate.SectionCount; index++)
                    {
                        byte[] want = index < original.Sections.Count ? original.Sections[index].ToArray() : [];
                        if (!want.AsSpan().SequenceEqual(rewritten.Sections[index].Span))
                        {
                            problems.Add($"{path}: 섹션 {FortTemplate.SectionNames[index]} 불일치");
                            break;
                        }
                    }
                }
            }
            catch (Exception exception)
            {
                problems.Add($"{path}: {exception.GetType().Name}: {exception.Message}");
            }
        }
        Assert.True(problems.Count == 0, $"왕복 불일치 {problems.Count}개/{names.Count}개:\n" + string.Join("\n", problems.Take(30)));
    }
}
