using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Template 저장소의 동적 섹션 범위(TS2)를 검사한다: Temp 같은 비표준 이름의 생성·쓰기·이름 교체(같은 노드·할당·생성 수명 보존), 마지막 섹션 압축,
/// State 재생성과 최소 확장, 쓰기 참조의 수명, 인라인 쓰기 빌림의 규칙.
/// cpppj의 CampaignTemplateDynamicTests(2 중 SaveFile 없는 부분), DataManagerCreateTests(1), DataManagerResizeTests(3), DataManagerStreamsTests(1),
/// PlayerStateInitializationTests(2), TemplateSerializationGlobalsTests(1)를 옮겼고 <c>FortTemplate::Parse</c>는 <see cref="FortTemplate.ReadSections"/>로 대체했다.
/// 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다 (0.132 TS2, 2026-11-01).
/// </summary>
public sealed class CampaignTemplateSectionsTests
{
    /// <summary>실제 Rename은 원래 Temp 노드와 할당을 남기고 대상 노드만 반납한다.</summary>
    [Fact]
    public void TempRenamePreservesNodeAndAllocation()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "dynamic.fort");
        storage.Borrow(8, 1);
        TemplateWriteStream previous = storage.FindSection(1, "Terr00");
        TemplateWriteStream source = storage.CreateSection(8, "Temp");
        // 비표준 이름의 할당은 요새의 35칸 밖에 있다
        Assert.Equal(35, storage.Template(1).Sections.Count);
        // 생성 직후의 섹션은 크기 0에 기본 용량 16384다
        Assert.Equal(0, storage.SectionBuffer(source).Count);
        Assert.Equal(16384, storage.SectionBuffer(source).Capacity);
        byte[] bytes = [2, 7, 9];
        storage.WriteSection(source, bytes);
        StreamObservation before = Observe(storage, source);
        byte[] allocation = ArrayOf(storage.SectionBuffer(source));

        TemplateWriteStream renamed = storage.RenameSection(1, "Terr00", "Temp");
        StreamObservation after = Observe(storage, source);
        Assert.Equal(source, renamed);
        Assert.NotEqual(previous.Section, renamed.Section);
        Assert.Equal(before, after);
        // 같은 배열 객체가 표준 Terr00 위치로 옮겨졌다(복사하지 않았다)
        Assert.Same(allocation, ArrayOf(storage.Template(8).Sections[IndexOf("Terr00")]));
        Assert.Same(allocation, ArrayOf(storage.SectionBuffer(source)));
        Assert.Equal(3, storage.Template(1).Section("Terr00").Length);
        Assert.Equal("Terr00", storage.CurrentSection(8));
        IReadOnlyList<string> order = storage.SectionOrder(1);
        Assert.Equal(35, order.Count);
        Assert.Equal("Terr00", order[^1]);
        Assert.Single(order, name => name == "Terr00");
        Assert.DoesNotContain("Temp", order);

        // 반납된 이전 Terr00 참조로는 쓸 수 없고, 사라진 Temp 이름은 빈 결과다
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(previous, bytes));
        TemplateWriteStream absent = storage.FindSection(8, "Temp");
        Assert.Equal(default, absent);
        Assert.Equal("", storage.CurrentSection(1));
        Assert.Throws<InvalidOperationException>(() => storage.CurrentWriteStream(1));
        storage.WriteSection(source, bytes.AsSpan(0, 1));
        Assert.Equal(4, storage.Template(1).Section("Terr00").Length);
        TemplateWriteStream selected = storage.SelectSection(8, "tErR00", 0);
        Assert.Equal(source.Section, selected.Section);
        Assert.Equal(0u, Observe(storage, source).Position);
        storage.Clear(8);
        Assert.Equal(1, storage.OwnedImages);
        storage.Clear(1);
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(source, bytes));
    }

    /// <summary>
    /// Temp의 반복 생성/이름 교체는 20개 영역 섹션을 같은 할당으로 연결하고, 다음 노드 생성 때 이전 마지막 섹션을 end 크기로 압축한다.
    /// cpppj의 파일 저장(SaveFile) 확인은 저장 부모가 TS3이라 옮기지 않았다.
    /// </summary>
    [Fact]
    public void TwentyTerritoriesReuseTemporaryAllocations()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "all-territories.fort");
        TemplateWriteStream[] references = new TemplateWriteStream[20];
        // 실제 원본 부모의 Temp→TerrNN 흐름을 이름별로 반복한다
        for (uint index = 0; index < 20; index++)
        {
            string name = $"Terr{index:00}";
            TemplateWriteStream temporary = storage.CreateSection(1, "Temp");
            byte[] bytes = [2, (byte)index, (byte)(index + 40)];
            storage.WriteSection(temporary, bytes);
            byte[] allocation = ArrayOf(storage.SectionBuffer(temporary));
            references[index] = storage.RenameSection(1, name, "Temp");
            Assert.Equal(temporary, references[index]);
            Assert.Same(allocation, ArrayOf(storage.Template(1).Sections[IndexOf(name)]));
            Assert.Equal(bytes, storage.Template(1).Section(name).ToArray());
        }
        // 새 노드를 붙일 때마다 이전 마지막 섹션은 end(3)로 압축됐고 마지막 하나만 처음 할당(16384) 그대로다
        for (int index = 0; index < 19; index++)
        {
            Assert.Equal(3, storage.Template(1).Sections[IndexOf($"Terr{index:00}")].Capacity);
        }
        Assert.Equal(16384, storage.Template(1).Sections[IndexOf("Terr19")].Capacity);
        Assert.Equal(35, storage.SectionOrder(1).Count);
        Assert.Equal("Terr19", storage.SectionOrder(1)[^1]);
        storage.WriteSection(references[^1], new byte[] { 99 });
        Assert.Equal(99, storage.Template(1).Section("Terr19")[^1]);
    }

    /// <summary>
    /// 파싱된 섹션 수보다 뒤의 표준 이름(Terr05=20번)으로 Temp를 바꾸면 그 위치는 이미 연결된 것으로 표시되어 같은 이름의 노드가 중복 생성되지 않는다.
    /// 요새 버퍼 목록이 그 위치까지 늘어나므로 다음 동기화는 그 사이의 아직 연결되지 않은 표준 위치(3~19번)를 원본 순서대로 뒤에 붙인다.
    /// </summary>
    [Fact]
    public void RenameBeyondParsedSectionsDoesNotDuplicateStandardNode()
    {
        FortTemplate fort = new();
        for (int index = 0; index < 3; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "beyond");
        TemplateWriteStream temp = storage.CreateSection(1, "Temp");
        storage.WriteSection(temp, new byte[] { 1, 2 });
        TemplateWriteStream renamed = storage.RenameSection(1, "Terr05", "Temp");
        Assert.Equal(temp, renamed);
        Assert.Equal(21, storage.Template(1).Sections.Count);
        Assert.Equal(new byte[] { 1, 2 }, storage.Template(1).Section("Terr05").ToArray());
        // 다음 동기화(검색)가 사이의 표준 위치 17개를 뒤에 붙인다
        _ = storage.FindSection(1, "Nothing");
        List<string> expected = ["Subscriber", "State", "Mission", "Terr05"];
        expected.AddRange(FortTemplate.SectionNames.Skip(3).Take(17));
        Assert.Equal(expected, storage.SectionOrder(1));
    }

    /// <summary>표준 이름에서 동적 이름으로 옮긴 동일 노드도 typed 커서를 잘못 재사용하거나 원래 표준 노드를 되살리지 않는다.</summary>
    [Fact]
    public void TypedCursorFollowsCurrentName()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "typed.fort");
        TemplateWriteStream temp = storage.CreateSection(1, "Temp");
        storage.WriteSection(temp, new byte[] { 17, 29 });
        TemplateWriteStream state = storage.RenameSection(1, "State", "Temp");
        Assert.Equal(temp.Section, state.Section);
        Assert.Equal(2u, storage.State(1).Position);
        Assert.Equal(2u, storage.State(1).End);
        Assert.Equal(new byte[] { 17, 29 }, storage.State(1).Bytes.ToArray());
        Assert.Same(ArrayOf(storage.SectionBuffer(temp)), ArrayOf(storage.Template(1).Sections[IndexOf("State")]));

        storage.State(1).Position = 1;
        StreamObservation before = Observe(storage, temp);
        TemplateWriteStream renamed = storage.RenameSection(1, "Scratch", "State");
        Assert.Equal(temp.Section, renamed.Section);
        Assert.Equal(before.Position, Observe(storage, temp).Position);
        // 이름이 바뀌면 State 커서는 비워지고 표준 State 노드는 되살아나지 않는다
        Assert.True(storage.State(1).Bytes.IsEmpty);
        Assert.Equal(0u, storage.FindSection(1, "State").Section);
        Assert.DoesNotContain("State", storage.SectionOrder(1));

        TemplateWriteStream restored = storage.RenameSection(1, "State", "Scratch");
        Assert.Equal(temp, restored);
        Assert.Equal(1u, storage.State(1).Position);
        Assert.Equal(2u, storage.State(1).End);
        // 같은 이름을 다시 만들면 새 노드이고 이전 참조는 거부된다
        TemplateWriteStream replacement = storage.CreateSection(1, "State");
        Assert.NotEqual(temp.Section, replacement.Section);
        Assert.Throws<InvalidOperationException>(() => storage.AccessSection(temp, (_, _) => { }));
    }

    /// <summary>State 생성 부모는 같은 소유 manager의 목록 순서와 마지막 위치를 보존하고, 이미 마지막인 State도 반복해서 교체한다.</summary>
    [Fact]
    public void StateCreationParentPreservesSharedOrderAndTail()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, RealFort("thewarbegins"), "createState");
        storage.Borrow(8, 1);
        byte[] input = new byte[28];
        // 첫 교체와 이미 마지막인 State의 반복 교체를 같은 소유 manager에 적용한다
        for (byte repeat = 0; repeat < 3; repeat++)
        {
            Array.Fill(input, (byte)(0x31 + repeat));
            PlayerTemplateStateStream state = storage.InitializeState(8, input);
            IReadOnlyList<string> order = storage.SectionOrder(1);
            Assert.True(state.Present);
            Assert.Equal(28u, state.Position);
            Assert.Equal(28u, state.End);
            Assert.Equal(input, state.Bytes.ToArray());
            Assert.Same(storage.State(1), state);
            Assert.Equal(35, order.Count);
            Assert.Equal("State", order[^1]);
            Assert.Single(order, name => name == "State");
            Assert.Equal("State", storage.CurrentSection(1));
            Assert.True(SameStorage(state.Bytes.Span, storage.Template(1).Section("State")));
        }
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>State가 충분하지 않으면 임시 섹션으로 확장하고 같은 데이터를 현재 State로 인계한다.</summary>
    [Fact]
    public void MinimumStateUsesTemporarySectionAndSharedData()
    {
        FortTemplate fort = RealFort("thewarbegins");
        fort.Sections[IndexOf("State")] = RawSectionBuffer.FromBytes([0x12, 0x34, 0x56]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "minimumState");
        storage.Borrow(8, 1);
        uint depth = 0;
        int ensured = 0;
        RawPlayerTemplateState state = new(
            OriginalEdition.Patch1078,
            storage.Headers,
            slot => storage.SelectState(slot),
            slot => storage.Name(slot),
            () => depth,
            value => depth = value,
            () => true,
            _ => throw new InvalidOperationException("최소 State 예상 밖 진단"),
            _ => { },
            (_, _) => throw new InvalidOperationException("실제 최소 확장에서 분리 할당 호출 금지"),
            _ => throw new InvalidOperationException("실제 최소 확장에서 분리 복사 완료 호출 금지"));
        state.BindMinimumSize((slot, minimum) =>
        {
            ensured++;
            Assert.Equal(28, minimum);
            return storage.EnsureStateSize(slot, minimum);
        });

        byte[] first = state.Get(8).ToArray();
        IReadOnlyList<string> order = storage.SectionOrder(1);
        Assert.Equal(1, ensured);
        Assert.Equal(28, first.Length);
        Assert.Equal(new byte[] { 0x12, 0x34, 0x56 }, first[..3]);
        Assert.All(first[3..], value => Assert.Equal(0, value));
        Assert.True(SameStorage(storage.State(1).Bytes.Span, storage.Template(1).Section("State")));
        Assert.Equal(35, order.Count);
        Assert.Equal("State", order[^1]);
        Assert.Single(order, name => name == "State");
        Assert.Equal(28u, storage.State(1).Position);
        Assert.Equal(28u, storage.State(1).End);

        // 두 번째 조회는 이미 충분해 확장하지 않고 같은 할당을 돌려준다
        Span<byte> second = state.Get(1);
        Assert.True(SameStorage(second, storage.Template(1).Section("State")));
        Assert.Equal(1, ensured);
        Assert.Equal(0u, storage.State(8).Position);
        Assert.Equal(order, storage.SectionOrder(8));
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>State가 마지막이고 원래 큰 할당을 가진 경우에도 원문 복사를 마친 뒤 tail 압축/원래 State 반납을 수행한다.</summary>
    [Fact]
    public void MinimumStateTailCopiesBeforeCompaction()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, RealFort("thewarbegins"), "tailMinimum");
        byte[] initial = new byte[28];
        Array.Fill(initial, (byte)0xa7);
        RawPlayerTemplateState.InitializeStateBytes(initial);
        storage.InitializeState(1, initial);
        RawSectionBuffer bytes = storage.Template(1).Sections[IndexOf("State")];
        bytes.Resize(3);
        bytes[0] = 0x12;
        bytes[1] = 0x34;
        bytes[2] = 0x56;
        PlayerTemplateStateStream stateStream = storage.State(1);
        stateStream.Bytes = bytes.Memory;
        stateStream.Position = 0;
        stateStream.End = 3;
        stateStream.Present = true;
        Assert.Equal("State", storage.SectionOrder(1)[^1]);
        Assert.True(bytes.Capacity >= 0x4000);

        PlayerTemplateStateStream current = storage.EnsureStateSize(1, 28);
        IReadOnlyList<string> order = storage.SectionOrder(1);
        Assert.Equal(28, current.Bytes.Length);
        Assert.Equal(28u, current.Position);
        Assert.Equal(28u, current.End);
        Assert.Equal(new byte[] { 0x12, 0x34, 0x56 }, current.Bytes.Span[..3].ToArray());
        Assert.All(current.Bytes.Span[3..].ToArray(), value => Assert.Equal(0, value));
        Assert.Equal(35, order.Count);
        Assert.Equal("State", order[^1]);
        Assert.Single(order, name => name == "State");
        Assert.True(SameStorage(current.Bytes.Span, storage.Template(1).Section("State")));
        // 확장 직후 현재 노드의 원본 필드: 임시 토큰이 아니라 노드 토큰이고 pos/end/alloc은 모두 28이다
        StreamObservation raw = Observe(storage, storage.CurrentWriteStream(1));
        Assert.Equal(0x10000000u, raw.Pointer & 0xf0000000u);
        Assert.Equal(28u, raw.Position);
        Assert.Equal(28u, raw.End);
        Assert.Equal(28u, raw.Capacity);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>signed 최소 크기와 충분한 원문은 할당/순서/커서를 보존하며 CD 또는 빈 부모 연결은 거부한다.</summary>
    [Fact]
    public void SufficientStateKeepsAllocationAndBindingIsRequired()
    {
        FortTemplate fort = new();
        SetSection(fort, "State", new byte[32]);
        fort.Sections[IndexOf("State")].Span.Fill(0xa7);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "sufficient");
        storage.State(1).Position = 17;
        byte[] pointer = ArrayOf(storage.Template(1).Sections[IndexOf("State")]);
        IReadOnlyList<string> order = storage.SectionOrder(1);
        // 실제 signed 부모의 음수/0/이미 충분한 최소 경계를 같은 저장소에 적용한다
        foreach (int minimum in new[] { -1, 0, 1, 28, 32 })
        {
            PlayerTemplateStateStream state = storage.EnsureStateSize(1, minimum);
            Assert.Same(pointer, ArrayOf(storage.Template(1).Sections[IndexOf("State")]));
            Assert.Equal(17u, state.Position);
            Assert.Equal(32u, state.End);
            Assert.Equal(order, storage.SectionOrder(1));
        }

        RawPlayerTemplateState Build(OriginalEdition edition)
        {
            uint depth = 0;
            return new RawPlayerTemplateState(edition, storage.Headers, slot => storage.State(slot), slot => storage.Name(slot), () => depth, value => depth = value, () => true, _ => { }, _ => { });
        }
        // CD는 PATCH 실제 부모를 추정으로 쓰지 않고, 빈 부모 연결도 거부한다
        Assert.Throws<ArgumentException>(() => Build(OriginalEdition.Cd1072).BindMinimumSize((_, _) => storage.State(1)));
        Assert.Throws<ArgumentException>(() => Build(OriginalEdition.Patch1078).BindMinimumSize(null!));
        storage.ClearAll();
    }

    /// <summary>State를 새로 만들 때 앞의 빈 마지막 섹션(용량 37)은 반납되고, 같은 State가 목록 끝에 한 번만 남는다.</summary>
    [Fact]
    public void CommonStreamsRecreateAndReleaseEmptyTail()
    {
        FortTemplate fort = RealFort("thewarbegins");
        fort.Sections[IndexOf("State")].Clear();
        fort.Sections[^1].Clear();
        fort.Sections[^1].Reserve(37);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "commonStreams");
        storage.Borrow(8, 1);
        byte[] record = new byte[28];
        Array.Fill(record, (byte)0xa7);
        RawPlayerTemplateState.InitializeStateBytes(record);

        byte[] first = storage.InitializeState(8, record).Bytes.ToArray();
        Assert.Equal(record, first);
        Assert.Equal("State", storage.CurrentSection(1));
        Assert.Equal("State", storage.SectionOrder(1)[^1]);
        // 마지막이던 빈 Terr19의 예약 용량은 새 노드를 붙일 때 압축으로 반납됐다
        Assert.Equal(0, storage.Template(1).Sections[^1].Capacity);

        record[3] ^= 0x40;
        PlayerTemplateStateStream second = storage.InitializeState(1, record);
        IReadOnlyList<string> order = storage.SectionOrder(8);
        Assert.True(SameStorage(second.Bytes.Span, storage.Template(8).Section("State")));
        Assert.Equal(record, second.Bytes.ToArray());
        Assert.Equal(35, order.Count);
        Assert.Equal("State", order[^1]);
        Assert.Single(order, name => name == "State");
        Assert.Equal(28u, storage.State(8).Position);
        storage.SelectTerritory(1);
        Assert.Equal(order, storage.SectionOrder(8));
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>빈 State는 미정 입력 28바이트와 원본 초기화 규칙으로 다시 만들어지고, 목록 순서와 현재 바이트가 일관된다.</summary>
    [Fact]
    public void EmptyStateRecreatedWithCurrentBytesAndOrder()
    {
        FortTemplate fort = RealFort("thewarbegins");
        int at = IndexOf("State");
        fort.Sections[at].Clear();
        fort.Sections[at].Reserve(79);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "emptyState");
        storage.Borrow(8, 1);
        IReadOnlyList<string> originalOrder = storage.SectionOrder(1);
        Assert.Equal(35, originalOrder.Count);
        Assert.Equal("State", originalOrder[at]);
        byte[] seed = new byte[28];
        Array.Fill(seed, (byte)0xa7);
        uint depth = 0;
        int initializations = 0;
        RawPlayerTemplateState state = new(
            OriginalEdition.Patch1078,
            storage.Headers,
            slot => storage.SelectState(slot),
            slot => storage.Name(slot),
            () => depth,
            value => depth = value,
            () => true,
            _ => throw new InvalidOperationException("실제 State 생성 예상 밖 진단"),
            _ => { });
        state.BindInitialization(new PlayerTemplateStateInitialization
        {
            Bytes = (_, bytes) =>
            {
                initializations++;
                seed.AsSpan().CopyTo(bytes);
            },
            Replace = (slot, bytes) => storage.InitializeState(slot, bytes),
        });

        byte[] first = state.Get(8).ToArray();
        IReadOnlyList<string> order = storage.SectionOrder(1);
        Assert.Equal(1, initializations);
        Assert.Equal(35, order.Count);
        Assert.Equal("State", order[^1]);
        Assert.Single(order, name => name == "State");
        Assert.Equal(0xa1, first[0]);
        Assert.Equal(0xa7, first[1]);
        Assert.Equal(0xa7, first[10]);
        Assert.Equal(0xa7, first[18]);
        Assert.Equal(0xa7, first[26]);
        Assert.Equal(0, first[4]);
        Assert.Equal(0, first[17]);
        Assert.Equal(0, first[24]);
        Assert.Equal(28, first.Length);
        Assert.True(SameStorage(storage.State(1).Bytes.Span, storage.Template(1).Section("State")));
        Assert.Equal(28u, storage.State(1).Position);
        Assert.Equal(28u, storage.State(1).End);
        Assert.True(storage.Template(1).Sections[at].Capacity >= 0x4000);
        Assert.Equal("State", storage.CurrentSection(8));
        storage.SelectTerritory(1);
        Assert.Equal("Territory", storage.CurrentSection(8));
        Assert.Equal(order, storage.SectionOrder(8));

        // 두 번째 조회는 이미 있는 State를 재사용하고 미정 입력을 다시 요구하지 않는다
        Span<byte> second = state.Get(1);
        Assert.Equal(1, initializations);
        Assert.True(SameStorage(second, storage.Template(1).Section("State")));
        Assert.Equal(0u, storage.State(8).Position);
        Assert.Equal(order, storage.SectionOrder(1));
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>새 State 앞의 마지막 Territory 압축이 실제 버퍼를 교체하면 같은 지형 커서도 새 버퍼/끝을 가리킨다.</summary>
    [Fact]
    public void StateRecreationCompactsPreviousTerritoryTail()
    {
        FortTemplate fort = RealFort("thewarbegins");
        int territoryAt = IndexOf("Territory");
        // 요새를 Territory까지만 남기고 State를 비우며 Territory에 예약 용량을 둔다
        fort.Sections.RemoveRange(territoryAt + 1, fort.Sections.Count - (territoryAt + 1));
        fort.Sections[IndexOf("State")].Clear();
        fort.Sections[territoryAt].Reserve(512);
        byte[] before = fort.Sections[territoryAt].ToArray();
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "compactTail");
        storage.Borrow(8, 1);
        byte[] record = new byte[28];
        Array.Fill(record, (byte)0xa7);
        RawPlayerTemplateState.InitializeStateBytes(record);

        PlayerTemplateStateStream state = storage.InitializeState(8, record);
        FortTerritoryStream territory = storage.Territory(1);
        IReadOnlyList<string> order = storage.SectionOrder(1);
        Assert.Equal(28u, state.Position);
        Assert.Equal(120u, territory.Position);
        Assert.Equal(120u, territory.End);
        Assert.Equal("Territory", order[^2]);
        Assert.Equal("State", order[^1]);
        Assert.True(SameStorage(territory.Bytes.Span, storage.Template(8).Section("Territory")));
        Assert.Equal(120, storage.Template(1).Sections[territoryAt].Capacity);
        Assert.Equal(before, territory.Bytes.ToArray());
        storage.SelectTerritory(8);
        Assert.Equal(0u, territory.Position);
        Assert.Equal(order, storage.SectionOrder(1));
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>섹션 생성/이름 교체/선택/쓰기는 공유 직렬화 버전을 바꾸지 않고, 이름이 바뀐 뒤 이전 참조는 거부된다.</summary>
    [Fact]
    public void SectionsAndClearPreserveVersion()
    {
        FortSerializationGlobals globals = new();
        CampaignTemplateStorage storage = new(globals);
        storage.Install(0, new FortTemplate(), "source");
        globals.Version = 2;
        byte[] bytes = [7, 9];
        TemplateWriteStream temp = storage.CreateSection(0, "Temp");
        storage.WriteSection(temp, bytes);
        Assert.Equal(2u, globals.Version);
        TemplateWriteStream old = storage.CreateSection(0, "Terr00");
        storage.WriteSection(old, bytes);
        Assert.Equal(2u, globals.Version);
        TemplateWriteStream renamed = storage.RenameSection(0, "Terr00", "Temp");
        Assert.Equal(temp, renamed);
        Assert.Equal(2u, globals.Version);
        // 실제 선택의 pos 0 뒤 같은 옛 참조로 덮어써 추가 할당을 요구하지 않는다
        TemplateWriteStream selected = storage.SelectSection(0, "Terr00", 1);
        Assert.Equal(temp, selected);
        storage.WriteSection(temp, bytes);
        Assert.Equal(2u, globals.Version);
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(old, bytes));
        storage.Borrow(8, 0);
        Assert.Equal(2u, globals.Version);
        storage.Clear(8);
        Assert.Equal(2u, globals.Version);
        Assert.Equal(1, storage.OwnedImages);
        storage.ClearAll();
        Assert.Equal(2u, globals.Version);
        Assert.Equal(0, storage.OwnedImages);
        storage.Clear(0);
        storage.ClearAll();
        Assert.Equal(2u, globals.Version);
        {
            // 실제 소멸/Storage 반납도 현재 버전을 임의로 0으로 만들지 않는다
            CampaignTemplateStorage nested = new(globals);
            Assert.Equal(2u, globals.Version);
            nested.Install(0, new FortTemplate(), "nested");
            globals.Version = 3;
        }
        Assert.Equal(3u, globals.Version);
    }

    /// <summary>인라인 쓰기 동안에는 같은 노드의 중첩 빌림과 섹션 구조 변경(생성·이름 교체·확장)이 거부되고, 끝나면 다시 허용된다.</summary>
    [Fact]
    public void NestedAccessAndStructureChangesAreRejected()
    {
        FortTemplate fort = EmptyFort();
        SetSection(fort, "State", new byte[28]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "nested");
        TemplateWriteStream temp = storage.CreateSection(1, "Temp");
        int entered = 0;
        storage.AccessSection(temp, (_, _) =>
        {
            entered++;
            Assert.Throws<InvalidOperationException>(() => storage.AccessSection(temp, (_, _) => { }));
            Assert.Throws<InvalidOperationException>(() => storage.CreateSection(1, "Other"));
            Assert.Throws<InvalidOperationException>(() => storage.RenameSection(1, "Terr00", "Temp"));
            Assert.Throws<InvalidOperationException>(() => storage.EnsureStateSize(1, 40));
        });
        Assert.Equal(1, entered);
        // 연산이 끝나면 구조 변경이 다시 허용된다
        TemplateWriteStream other = storage.CreateSection(1, "Other");
        Assert.NotEqual(temp.Section, other.Section);
        Assert.Throws<ArgumentException>(() => storage.AccessSection(other, null!));
    }

    /// <summary>인라인 쓰기가 예외를 던져도 그 전까지의 pos/end/부분 쓰기는 보존되고 빌림은 해제된다.</summary>
    [Fact]
    public void AccessSectionExceptionKeepsPartialWriteAndReleasesBorrow()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "partial");
        TemplateWriteStream temp = storage.CreateSection(1, "State");
        Assert.Throws<FormatException>(() => storage.AccessSection(temp, (fields, hooks) =>
        {
            RawMemStream.Write(fields, new byte[] { 1, 2 }, 2, hooks);
            throw new FormatException("연산 중 예외");
        }));
        // 예외 전의 부분 쓰기와 논리 끝은 요새와 State 커서에 그대로 공개된다
        Assert.Equal(new byte[] { 1, 2 }, storage.Template(1).Section("State").ToArray());
        Assert.Equal(2u, storage.State(1).End);
        Assert.Equal(2u, storage.State(1).Position);
        Assert.Equal(new byte[] { 1, 2 }, storage.State(1).Bytes.ToArray());
        // 빌림이 해제됐으므로 다음 쓰기와 구조 변경이 가능하다
        storage.WriteSection(temp, new byte[] { 3 });
        Assert.Equal(new byte[] { 1, 2, 3 }, storage.Template(1).Section("State").ToArray());
        _ = storage.CreateSection(1, "Temp");
    }

    /// <summary>쓰기 참조는 비어 있거나, 다른 manager를 가리키거나, 같은 이름이 재생성되면 거부된다.</summary>
    [Fact]
    public void WriteReferencesRejectStaleDefaultAndUnknown()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "refs");
        byte[] bytes = [1];
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(default, bytes));
        TemplateWriteStream first = storage.CreateSection(1, "Temp");
        storage.WriteSection(first, bytes);
        // 같은 이름의 재생성은 이전 노드를 제거하므로 이전 참조는 거부된다
        TemplateWriteStream second = storage.CreateSection(1, "Temp");
        Assert.NotEqual(first.Section, second.Section);
        Assert.NotEqual(first.Generation, second.Generation);
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(first, bytes));
        storage.WriteSection(second, bytes);
        // 섹션 번호만 같은 다른 생성 수명의 참조도 거부한다
        TemplateWriteStream forged = second with { Generation = second.Generation + 1 };
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(forged, bytes));
        TemplateWriteStream wrongManager = second with { Manager = second.Manager + 100 };
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(wrongManager, bytes));
        TemplateWriteStream zeroGeneration = second with { Generation = 0 };
        Assert.Throws<InvalidOperationException>(() => storage.WriteSection(zeroGeneration, bytes));
    }

    /// <summary>manager를 직접 지정한 섹션 생성은 그 manager를 고정하고, 없거나 반납된 manager는 거부한다.</summary>
    [Fact]
    public void CreateManagerSectionPinsTheChosenManager()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, EmptyFort(), "first");
        storage.Install(2, EmptyFort(), "second");
        uint firstManager = storage.CreateSection(1, "Temp").Manager;
        uint secondManager = storage.CreateSection(2, "Temp").Manager;
        Assert.NotEqual(firstManager, secondManager);
        // 칸 번호가 아니라 지정한 manager에만 만들고 다른 칸의 manager는 건드리지 않는다
        TemplateWriteStream created = storage.CreateManagerSection(firstManager, "Pinned");
        Assert.Equal(firstManager, created.Manager);
        Assert.Contains("Pinned", storage.SectionOrder(1));
        Assert.DoesNotContain("Pinned", storage.SectionOrder(2));
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.CreateManagerSection(0, "x"));
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.CreateManagerSection(secondManager + 1000, "x"));
        storage.Clear(2);
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.CreateManagerSection(secondManager, "x"));
    }

    /// <summary>State 확장 복사 중에는 새 섹션 생성과 최소 확장이 거부되고, 확정 뒤에는 허용된다.</summary>
    [Fact]
    public void StructureChangesAreRejectedWhileStateGrowthIsPending()
    {
        FortTemplate fort = EmptyFort();
        SetSection(fort, "State", [5, 6]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "pending");
        _ = storage.GrowState(1, 28);
        Assert.Throws<InvalidOperationException>(() => storage.CreateSection(1, "Temp"));
        Assert.Throws<InvalidOperationException>(() => storage.EnsureStateSize(1, 40));
        storage.CommitStateGrowth(1);
        _ = storage.CreateSection(1, "Temp");
        Assert.Equal(40u, storage.EnsureStateSize(1, 40).End);
    }

    /// <summary>State 입력은 정확히 28바이트여야 하고, State가 없거나 이름이 없는 대상은 누락 진단으로 멈춘다.</summary>
    [Fact]
    public void StateAndRenameContractsAreRejected()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, new FortTemplate(), "empty");
        Assert.Throws<ArgumentException>(() => storage.InitializeState(1, new byte[27]));
        Assert.Throws<ArgumentException>(() => storage.InitializeState(1, new byte[29]));
        // State 섹션이 아예 없으면 최소 확장의 원문 누락 진단이다
        Assert.Throws<InvalidOperationException>(() => storage.EnsureStateSize(1, 28));
        // 없는 이름을 바꾸려 하면 노드 조회가 실패한다
        Assert.Throws<InvalidOperationException>(() => storage.RenameSection(1, "Anything", "Missing"));
        Assert.Empty(storage.SectionOrder(1));
    }
}
