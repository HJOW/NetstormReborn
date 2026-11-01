using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Template 저장소의 빈 manager·로그인 이미지 범위(TS4)를 검사한다: 실제 빈 Image 할당·원문 생성자·35개 Create 초기화(<c>AllocateEmptyManager</c> 계열),
/// 지역 Template를 읽어 다섯 섹션만 남기는 송신용 이미지(<c>ReadLoginImage</c>: <c>CloneSectionFrom</c>·<c>Dump</c>·<c>SaveImage</c>)와 그 임시 수명.
/// cpppj의 CampaignTemplateLoginImageTests(2), TemplateDiagnosticClientTests(1 중 저장소가 하는 덤프 호출 부분), DataManagerInitializeTests(1 중 빈 Image 부분)를 옮겼다.
/// <c>FortTemplate::Parse</c>는 <see cref="FortTemplate.ReadSections"/>로 대체했다. 실제 Client의 level 29 필터·FILE·공유 버퍼·Chat 연결은 Client 진단 계층이 없어 F3으로 넘긴다.
/// 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다 (0.134 TS4, 2026-11-02).
/// </summary>
public sealed class CampaignTemplateLoginTests
{
    /// <summary>strip한 로그인 이미지에서 서명(2바이트) 뒤 35개 길이 레코드가 차지하는 고정 앞부분(2+35×2)이다.</summary>
    private const int LoginImageHeader = 72;

    /// <summary>
    /// 파일의 35개 길이 WORD와 payload를 독립 해석한다. 결과 생성에는 제품 직렬화기를 쓰지 않는다.
    /// </summary>
    /// <param name="bytes">서명 2바이트로 시작하는 파일 이미지.</param>
    /// <exception cref="ArgumentOutOfRangeException">서명, 길이 레코드, payload 범위가 파일을 벗어난다.</exception>
    private static List<byte[]> LoginImageSections(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < 2)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "로그인이미지검사의서명누락");
        }
        List<byte[]> result = [];
        int position = 2;
        // 원본 고정 35개 이름에 대응하는 길이와 payload를 파일 순서대로 읽는다
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            if (position > bytes.Length || bytes.Length - position < 2)
            {
                throw new ArgumentOutOfRangeException(nameof(bytes), "로그인이미지검사의길이누락");
            }
            int length = bytes[position] + (bytes[position + 1] << 8);
            if (length < 2 || length > bytes.Length - position)
            {
                throw new ArgumentOutOfRangeException(nameof(bytes), "로그인이미지검사의payload범위");
            }
            result.Add(bytes.Slice(position + 2, length - 2).ToArray());
            position += length;
        }
        return result;
    }

    /// <summary>
    /// 선택한 파일 이미지만 공급한다. 지역 Template는 권한·현재 전역 슬롯·제목을 조회하면 실패하도록 지정한다.
    /// </summary>
    /// <param name="bytes">읽기 장치가 돌려줄 파일 바이트.</param>
    private static TemplateFileLoadHooks LoginImageFile(byte[] bytes) => new()
    {
        Read = _ => bytes,
        TypeCount = () => 188,
        ReadOnly = _ => throw new InvalidOperationException("지역readonlyTemplate의파일권한조회금지"),
        CurrentSlot = () => throw new InvalidOperationException("지역Template의전역현재슬롯조회금지"),
        RefreshTitle = () => throw new InvalidOperationException("지역Template의전역제목갱신금지"),
        TooManyTypes = () => throw new InvalidOperationException("검사입력에없는TYPE진단"),
        RejectFile = _ => throw new InvalidOperationException("검사입력에없는파일제거요청"),
    };

    /// <summary>제품의 다른 진단을 성공으로 바꾸지 않고 그대로 문자열로 관찰한다. 예외가 없으면 빈 문자열이다.</summary>
    /// <param name="action">실행할 동작.</param>
    private static string Failure(Action action)
    {
        try
        {
            action();
        }
        catch (Exception error)
        {
            return error.Message;
        }
        return "";
    }

    /// <summary>동작이 예외를 던지면 참이다. 예외 종류는 가리지 않고 던졌는지만 본다.</summary>
    /// <param name="action">실행할 동작.</param>
    private static bool Throws(Action action)
    {
        try
        {
            action();
        }
        catch (Exception)
        {
            return true;
        }
        return false;
    }

    /// <summary>알려진 바이트(0xa5)로 채운 이름표 할당으로 lazy 이름표 owner를 만든다. 할당 횟수를 세어 준다.</summary>
    /// <param name="allocations">최초 할당 횟수를 받는 카운터.</param>
    private static ClientTemplateSectionNames CountingNames(int[] allocations) => new(size =>
    {
        allocations[0]++;
        byte[] allocation = new byte[size];
        Array.Fill(allocation, (byte)0xa5);
        return allocation;
    });

    /// <summary>원문 004bf3e0의 고정 네 섹션과 현재 Terr 하나 외의 payload는 모두 비어야 한다. 전역 owner·이름표·현재 선택은 보존된다.</summary>
    [Fact]
    public void LoginImageStripsToFiveSectionsAndKeepsGlobalOwner()
    {
        byte[] bytes = RealFortBytes("thewarbegins");
        List<byte[]> original = LoginImageSections(bytes);
        // source의 FE 서명이 NONAME의 기본 0 서명으로 복제되지 않는 별도 입력이다
        bytes[1] = 0xfe;
        FortSerializationGlobals globals = new();
        int[] nameAllocations = [0];
        ClientTemplateSectionNames names = CountingNames(nameAllocations);
        CampaignTemplateStorage storage = new(globals, names);
        storage.BindDiagnosticLog((_, _) => { });
        storage.InstallFile(1, new FortTemplate(), bytes, "global-kept.fort");
        storage.SelectState(1).Position = 5;
        Memory<byte> allocation = storage.State(1).Bytes;
        byte[] headers = storage.Headers.ToArray();
        Span<byte> firstNames = names.Names();
        Assert.Equal(1, nameAllocations[0]);
        // 원본 Chaff의 비영 payload도 strip 뒤 사라지는지 확인한다
        Assert.NotEmpty(original[4]);
        // 서로 다른 현재 영역은 각각 자기 Terr만 보유하며 이전 호출의 임시 manager를 재사용하지 않는다
        foreach (uint territory in new uint[] { 0, 7, 19 })
        {
            int reads = 0;
            int typeReads = 0;
            globals.Version = 123;
            TemplateFileLoadHooks file = LoginImageFile(bytes) with
            {
                Read = name =>
                {
                    reads++;
                    Assert.Equal("source.fort", name);
                    Assert.Equal(0u, globals.Version);
                    Assert.Equal(1, storage.OwnedImages);
                    Assert.Equal("State", storage.CurrentSection(1));
                    globals.Version = 19;
                    return bytes;
                },
                // source 생성 뒤 읽기와 NONAME 생성 뒤 저장의 같은 전역 0 쓰기를 구별한다
                TypeCount = () =>
                {
                    typeReads++;
                    if (typeReads == 1)
                    {
                        Assert.Equal(19u, globals.Version);
                        globals.Version = 37;
                    }
                    else
                    {
                        Assert.Equal(2, typeReads);
                        Assert.Equal(0u, globals.Version);
                        globals.Version = 53;
                    }
                    return 188;
                },
            };
            FortImageData image = storage.ReadLoginImage("source.fort", file, territory);
            List<byte[]> sections = LoginImageSections(image.Bytes());
            IReadOnlyList<string> sectionNames = FortTemplate.SectionNames;
            string selected = "Terr" + territory.ToString("00");
            int expectedEnd = LoginImageHeader;
            Assert.Equal(32768, image.Storage.Length);
            Assert.Equal((byte)'F', image.Bytes()[0]);
            Assert.Equal(0, image.Bytes()[1]);
            Assert.Equal(1, reads);
            Assert.Equal(2, typeReads);
            Assert.Equal(53u, globals.Version);
            // 원문 5개 이름의 독립 payload와 빈 나머지를 모두 비교한다
            for (int index = 0; index < sectionNames.Count; index++)
            {
                string name = sectionNames[index];
                bool keep = name == "Subscriber" || name == "Territory" || name == "TypeNames" || name == "CoreData" || name == selected;
                if (keep)
                {
                    Assert.Equal(original[index], sections[index]);
                    expectedEnd += original[index].Length;
                }
                else
                {
                    Assert.Empty(sections[index]);
                }
            }
            Assert.Equal(expectedEnd, image.End);
            Assert.Equal(1, storage.OwnedImages);
            Assert.Equal("global-kept.fort", storage.Name(1));
            Assert.Equal("State", storage.CurrentSection(1));
            Assert.True(headers.AsSpan().SequenceEqual(storage.Headers.Span));
            Assert.Equal(5u, storage.State(1).Position);
            Assert.True(SameStorage(storage.State(1).Bytes.Span, allocation.Span));
            Assert.True(SameStorage(names.Names(), firstNames));
            Assert.Equal(1, nameAllocations[0]);
        }
    }

    /// <summary>
    /// 임시 실패(정상 false, 장치 예외, 일부 섹션 읽기 예외, 현재 Terr 누락) 뒤에도 전역 owner·current를 그대로 두고 다음 정상 읽기도 가능하다.
    /// </summary>
    [Fact]
    public void LoginImageFailuresPreserveGlobalOwner()
    {
        byte[] bytes = RealFortBytes("thewarbegins");
        CampaignTemplateStorage storage = new();
        storage.BindDiagnosticLog((_, _) => { });
        storage.InstallFile(1, new FortTemplate(), bytes, "kept.fort");
        storage.SelectState(1).Position = 3;
        byte[] headers = storage.Headers.ToArray();
        // 부재 정상 false / 장치 예외 / 일부 섹션 읽기 예외 / 현재 Terr 누락을 각각 독립 임시 수명으로 실행한다
        for (int variant = 0; variant < 4; variant++)
        {
            TemplateFileLoadHooks file = LoginImageFile(bytes);
            uint territory = variant == 3 ? 20u : 0u;
            storage.SerializationVersion = 91;
            if (variant == 0)
            {
                file = file with { Read = _ => [] };
            }
            if (variant == 1)
            {
                file = file with { Read = _ => throw new InvalidOperationException("선택파일장치중단") };
            }
            if (variant == 2)
            {
                // 첫 Subscriber 노드는 붙고 둘째 길이 읽기가 물리 범위를 벗어난다
                file = file with { Read = _ => new byte[] { (byte)'F', 0xfe, 5, 0, 11, 13, 17, 8, 0 } };
            }
            string failure = Failure(() => storage.ReadLoginImage("temporary.fort", file, territory));
            Assert.NotEmpty(failure);
            if (variant == 0)
            {
                Assert.Equal("로그인송신의지역Template읽기실패진단", failure);
            }
            if (variant == 1)
            {
                Assert.Equal("선택파일장치중단", failure);
            }
            if (variant == 3)
            {
                Assert.Equal("로그인이미지복제의원문섹션누락진단", failure);
            }
            Assert.Equal(0u, storage.SerializationVersion);
            Assert.Equal(1, storage.OwnedImages);
            Assert.Equal("State", storage.CurrentSection(1));
            Assert.Equal(3u, storage.State(1).Position);
            Assert.True(headers.AsSpan().SequenceEqual(storage.Headers.Span));
            FortImageData recovered = storage.ReadLoginImage("temporary.fort", LoginImageFile(bytes), 0);
            Assert.True(recovered.End > LoginImageHeader);
            Assert.Equal(1, storage.OwnedImages);
            Assert.Equal("kept.fort", storage.Name(1));
        }
    }

    /// <summary>
    /// 덤프는 복제가 끝난 NONAME를 SaveImage 전에 level 29로 두 번 출력한다(파일명+접두, 끝 합계). 출력 장치가 아무것도 하지 않아도(필터) 이미지는 같다.
    /// 장치를 연결하지 않으면 덤프 경계 누락으로 거부하고 임시 manager를 남기지 않으며, 이후 연결하면 정상으로 읽는다.
    /// cpppj TemplateDiagnosticClientTests의 저장소 쪽 부분이며 실제 Client의 필터/FILE/Chat 연결은 F3이다.
    /// </summary>
    [Fact]
    public void LoginImageDumpUsesBoundTraceDevice()
    {
        byte[] bytes = RealFortBytes("thewarbegins");
        CampaignTemplateStorage storage = new();
        TemplateFileLoadHooks file = LoginImageFile(bytes);
        // 장치 미연결은 덤프 위치에서 선택된 경계 누락으로 거부하고 지역 manager와 전역을 남기지 않는다
        Assert.NotEmpty(Failure(() => storage.ReadLoginImage("source.fort", file, 0)));
        Assert.Equal(0, storage.OwnedImages);
        List<(int Level, string Text)> logged = [];
        storage.BindDiagnosticLog((level, text) => logged.Add((level, text)));
        FortImageData image = storage.ReadLoginImage("source.fort", file, 0);
        Assert.Equal(
            [(29, "File NONAME: Stripped Image:\n"), (29, $"{image.End - LoginImageHeader:0000} bytes total.\n")],
            logged);
        // 필터가 닫힌 출력 장치(아무것도 하지 않음)에서도 같은 이미지를 만든다
        storage.BindDiagnosticLog((_, _) => { });
        FortImageData filtered = storage.ReadLoginImage("source.fort", file, 0);
        Assert.Equal(image.End, filtered.End);
        Assert.True(image.Storage.AsSpan().SequenceEqual(filtered.Storage));
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>
    /// 음수로 보이는 영역 번호(DWORD 비트를 signed 서식으로 씀)는 "Terr-5" 같은 이름을 찾다 원문 섹션 누락 진단으로 거부한다.
    /// 한 자리 번호는 0을 채운 두 자리("Terr09")로 찾아 그 섹션이 남는다.
    /// </summary>
    [Fact]
    public void LoginImageTerritoryNameFollowsSignedFormat()
    {
        byte[] bytes = RealFortBytes("thewarbegins");
        List<byte[]> original = LoginImageSections(bytes);
        CampaignTemplateStorage storage = new();
        storage.BindDiagnosticLog((_, _) => { });
        Assert.Equal("로그인이미지복제의원문섹션누락진단", Failure(() => storage.ReadLoginImage("source.fort", LoginImageFile(bytes), 0xfffffffbu)));
        FortImageData image = storage.ReadLoginImage("source.fort", LoginImageFile(bytes), 9);
        List<byte[]> sections = LoginImageSections(image.Bytes());
        int terr09 = FortTemplate.SectionNames.ToList().IndexOf("Terr09");
        int terr10 = FortTemplate.SectionNames.ToList().IndexOf("Terr10");
        Assert.Equal(original[terr09], sections[terr09]);
        Assert.Empty(sections[terr10]);
    }

    /// <summary>
    /// 실제 빈 Image 생성과 보호 TEST01 reader를 같은 Client 이름표에 연결해 목록·stream·cursor·수명을 검사한다.
    /// 원문 0044ce10은 다음 노드를 붙일 때 이전 tail을 end까지 압축한다. 그래서 앞 34개 빈 섹션은 버퍼 반납(포인터 0, 용량 0)이고 마지막 Terr19만 실제 버퍼와 초기 16384 용량이다.
    /// </summary>
    [Fact]
    public void EmptyImageAndExistingFileShareLazyNames()
    {
        int[] calls = [0];
        ClientTemplateSectionNames names = CountingNames(calls);
        CampaignTemplateStorage fresh = new(null, names);
        fresh.SerializationVersion = 0xdeadbeef;
        uint token = fresh.AllocateEmptyManager(276);
        Assert.NotEqual(0u, token);
        Assert.Equal(1, fresh.OwnedImages);
        Assert.Equal(0u, Header(fresh, 20));
        Assert.Equal(0, calls[0]);
        Assert.Equal(token, fresh.ConstructEmptyManager(token, "d/New.fort"));
        Assert.Equal(0u, fresh.SerializationVersion);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(fresh.ManagerBytes(token)[4..]));
        Assert.Equal(0, calls[0]);
        SetHeader(fresh, 20, token);
        fresh.InitializeEmptyManager(token);
        Span<byte> pointer = names.Names();
        Assert.Equal(1, calls[0]);
        Assert.Equal(FortTemplate.SectionNames.ToList(), fresh.SectionOrder(1).ToList());
        Assert.Equal("Terr19", fresh.CurrentSection(1));
        // 모든 표준 섹션의 원본 16바이트 MemStream 필드를 같은 노드에서 관찰한다
        foreach (string name in FortTemplate.SectionNames)
        {
            uint capacity = name == "Terr19" ? 16384u : 0u;
            TemplateWriteStream stream = fresh.FindSection(1, name);
            fresh.AccessSection(stream, (raw, _) =>
            {
                Assert.Equal(capacity != 0, BinaryPrimitives.ReadUInt32LittleEndian(raw) != 0);
                Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(raw[4..]));
                Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(raw[8..]));
                Assert.Equal(capacity, BinaryPrimitives.ReadUInt32LittleEndian(raw[12..]));
            });
        }
        byte[] file = RealFortBytes("TEST01");
        CampaignTemplateStorage existing = new(null, names);
        existing.InstallFile(1, FortTemplate.ReadSections(file), file, "d/TEST01.fort");
        Assert.True(SameStorage(names.Names(), pointer));
        Assert.Equal(1, calls[0]);
        Assert.Equal(0, names.Names()[0]);
        fresh.ClearAll();
        existing.ClearAll();
        Assert.True(SameStorage(names.Names(), pointer));
        Assert.Equal(1, calls[0]);
        Assert.Equal(0, fresh.OwnedImages);
        Assert.Equal(0, existing.OwnedImages);
    }

    /// <summary>
    /// 빈 Image 계약: 크기는 정확히 276이어야 하고 핸들은 서로 다르다. 알 수 없는 핸들, 이중 생성, 255바이트를 넘는 이름과 NUL 포함 이름, 생성자 이전 초기화를 거부한다.
    /// 거부한 호출은 공유 직렬화 버전을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void EmptyImageContractIsStrict()
    {
        CampaignTemplateStorage storage = new();
        Assert.True(Throws(() => storage.AllocateEmptyManager(275)));
        Assert.True(Throws(() => storage.AllocateEmptyManager(277)));
        Assert.Equal(0, storage.OwnedImages);
        uint first = storage.AllocateEmptyManager(276);
        uint second = storage.AllocateEmptyManager(276);
        Assert.NotEqual(first, second);
        Assert.NotEqual(0u, first);
        Assert.NotEqual(0u, second);
        Assert.Equal(2, storage.OwnedImages);
        // 알 수 없는 핸들
        Assert.True(Throws(() => storage.ConstructEmptyManager(99, "x")));
        Assert.True(Throws(() => storage.ManagerBytes(99)));
        Assert.True(Throws(() => storage.InitializeEmptyManager(99)));
        // 생성자 이전의 초기화는 거부한다
        Assert.True(Throws(() => storage.InitializeEmptyManager(first)));
        storage.SerializationVersion = 77;
        Assert.True(Throws(() => storage.ConstructEmptyManager(first, new string('a', 256))));
        Assert.True(Throws(() => storage.ConstructEmptyManager(first, "a\0b")));
        Assert.Equal(77u, storage.SerializationVersion);
        Assert.Equal(first, storage.ConstructEmptyManager(first, new string('a', 255)));
        Assert.Equal(0u, storage.SerializationVersion);
        // 이미 생성한 manager의 이중 생성은 거부한다
        storage.SerializationVersion = 55;
        Assert.True(Throws(() => storage.ConstructEmptyManager(first, "again")));
        Assert.Equal(55u, storage.SerializationVersion);
        // 이름은 manager+c에 NUL로 끝나 기록된다
        Assert.Equal(0x61, storage.ManagerBytes(first)[12]);
        Assert.Equal(0, storage.ManagerBytes(first)[12 + 255]);
        Assert.Equal(276, storage.ManagerBytes(first).Length);
    }

    /// <summary>
    /// 빈 Image는 모든 표준 위치를 이미 연결된 것으로 표시하므로, 늦은 표준 섹션(Terr19)만 먼저 만들어 앞쪽 위치가 빈 섹션으로 채워져도
    /// 앞 34개 이름의 가짜 선행 노드를 목록에 투영하지 않는다. 한 번 더 생성하면 같은 이름을 교체해도 목록은 그 하나뿐이다.
    /// </summary>
    [Fact]
    public void EmptyImageDoesNotProjectPhantomPredecessors()
    {
        CampaignTemplateStorage storage = new();
        uint token = storage.AllocateEmptyManager(276);
        storage.ConstructEmptyManager(token, "d/New.fort");
        SetHeader(storage, 20, token);
        storage.CreateSection(1, "Terr19");
        // 검색은 현재 파싱 자료와 목록을 동기화하는 경로다
        TemplateWriteStream stream = storage.FindSection(1, "Terr19");
        Assert.NotEqual(0u, stream.Section);
        Assert.Equal(["Terr19"], storage.SectionOrder(1).ToList());
        storage.CreateSection(1, "Terr19");
        storage.FindSection(1, "Terr19");
        Assert.Equal(["Terr19"], storage.SectionOrder(1).ToList());
    }

    /// <summary>
    /// 이미 초기화한 manager를 다시 초기화하면 root 진단(0xc9)을 선택된 장치에 먼저 알린다. 진단 장치가 없으면 거부해 아무것도 바꾸지 않는다.
    /// 진단이 던지면 목록은 그대로다. 소멸·반납 장치는 같은 소유 이미지를 비운다.
    /// </summary>
    [Fact]
    public void EmptyImageReinitializeReportsRootAndLifetimeReleasesOwner()
    {
        CampaignTemplateStorage storage = new();
        uint token = storage.AllocateEmptyManager(276);
        storage.ConstructEmptyManager(token, "d/New.fort");
        SetHeader(storage, 20, token);
        storage.InitializeEmptyManager(token);
        List<string> before = storage.SectionOrder(1).ToList();
        Assert.True(Throws(() => storage.InitializeEmptyManager(token)));
        List<DataManagerInitializeAssertion> reported = [];
        Assert.True(Throws(() => storage.InitializeEmptyManager(token, kind =>
        {
            reported.Add(kind);
            throw new InvalidOperationException("진단에서 중단");
        })));
        Assert.Equal([DataManagerInitializeAssertion.RootNotEmpty], reported);
        Assert.Equal(before, storage.SectionOrder(1).ToList());
        TemplateLifetimeHooks lifetime = storage.ManagerLifetime();
        lifetime.Destroy!(token);
        lifetime.Release!(token);
        Assert.Equal(0, storage.OwnedImages);
    }
}
