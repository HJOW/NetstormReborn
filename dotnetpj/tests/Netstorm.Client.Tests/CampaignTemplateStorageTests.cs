using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Template 저장소의 설치·선택 범위(TS1)를 검사한다: 9칸 수명(Install·Borrow·Clear·ClearAll), 같은 manager를 빌린 헤더의 공유 커서,
/// State 확장, 지형·Money·Badges·Technology 선택, 섹션 순서와 Subscriber 읽기, 공유 직렬화 전역.
/// cpppj의 TemplateLifetimeTests(2), DataManagerLifetimeTests(1), DataManagerSelectionTests(3 중 Money·Badges 로더를 뺀 부분), TemplateSerializationGlobalsTests(1 중 앞부분)를
/// 옮겼고 <c>FortTemplate::Parse</c>는 <see cref="FortTemplate.ReadSections"/>로 대체했다(Parse는 F1). 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다 (0.131 TS1, 2026-11-01).
/// 공용 도우미는 0.132(TS2)에서 <see cref="TemplateStorageTestSupport"/>로 옮겼다.
/// </summary>
public sealed class CampaignTemplateStorageTests
{
    /// <summary>실제 파일의 State 변경은 같은 manager의 저장 이미지에도 반영되고 빌린 헤더는 같은 커서를 사용한다.</summary>
    [Fact]
    public void ExplicitOwnershipBorrowAndSerializedState()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, RealFort("thewarbegins"), "thewarbegins");
        Assert.Equal(1, storage.OwnedImages);
        Assert.True(SameStorage(storage.State(1).Bytes.Span, storage.Template(1).Section("State")));

        storage.Borrow(8, 1);
        Assert.Same(storage.Template(1), storage.Template(8));
        Assert.Same(storage.State(1), storage.State(8));
        Assert.Same(storage.Territory(1), storage.Territory(8));
        storage.State(8).Position = 7;
        storage.State(8).Bytes.Span[0] ^= 0x40;
        byte current = storage.State(1).Bytes.Span[0];
        Assert.Equal(7u, storage.State(1).Position);
        Assert.Equal(current, storage.Template(1).Section("State")[0]);

        // 현재 요새를 저장한 이미지를 다시 읽어도 같은 State 첫 바이트다(타입 개수는 10.78 독립 호출의 188을 쓴다)
        FortImageData image = new RawFortImage(OriginalEdition.Patch1078, storage.Template(1), new FortImageHooks { TypeCount = () => 188 }).Save();
        Assert.Equal(current, FortTemplate.ReadSections(image.Bytes()).Section("State")[0]);

        // 빌린 헤더를 반납해도 소유자의 manager와 커서는 유지된다
        storage.Clear(8);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(0u, Header(storage, 8 * 20));
        Assert.Equal(7u, storage.State(1).Position);
        storage.Borrow(0, 1);
        SetHeader(storage, 2 * 20 + 8, 0x7391);
        storage.ClearAll();
        // 빈 포인터의 stale 필드는 전역 정리에서 보존한다
        Assert.Equal(0, storage.OwnedImages);
        Assert.Equal(0u, Header(storage, 0));
        Assert.Equal(0u, Header(storage, 20));
        Assert.Equal(0x7391u, Header(storage, 2 * 20 + 8));
    }

    /// <summary>짧은 State의 확장은 원문 복사 뒤 실제 Fort 섹션을 교체하며 빌린 manager의 커서도 같다.</summary>
    [Fact]
    public void ShortStateGrowthUsesSameSection()
    {
        FortTemplate fort = new();
        SetSection(fort, "State", [0x12, 0x34, 0x56]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "short");
        storage.Borrow(8, 1);
        RawPlayerTemplateState state = PlayerState(storage, select: false);

        Span<byte> bytes = state.Get(8);
        Assert.Equal(28, bytes.Length);
        Assert.Equal(0x12, bytes[0]);
        Assert.Equal(0x34, bytes[1]);
        Assert.Equal(0x56, bytes[2]);
        Assert.True(bytes[3..].ToArray().All(value => value == 0));
        Assert.Equal(28u, storage.State(1).Position);
        Assert.Equal(28u, storage.State(1).End);
        Assert.True(SameStorage(bytes, storage.Template(1).Section("State")));
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>설치 뒤 State가 새로 생기거나 빈 버퍼가 할당돼도 실제 소멸은 현재 버퍼/노드를 반납한다.</summary>
    [Fact]
    public void ReleaseCurrentAndLateSectionAllocations()
    {
        FortTemplate fort = new();
        fort.Sections.Add(new RawSectionBuffer());
        fort.Sections[0].Reserve(37);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "lateState");
        storage.Borrow(8, 1);

        FortTemplate installed = storage.Template(1);
        int at = IndexOf("State");
        // 설치 뒤 외부에서 State 섹션을 새로 만든다
        while (installed.Sections.Count <= at)
        {
            installed.Sections.Add(new RawSectionBuffer());
        }
        installed.Sections[at] = RawSectionBuffer.FromBytes([1, 2, 3, 4]);
        PlayerTemplateStateStream stateStream = storage.State(1);
        stateStream.Bytes = installed.Sections[at].Memory;
        stateStream.Position = 0;
        stateStream.End = 4;
        stateStream.Present = true;

        Memory<byte> replacement = storage.GrowState(8, 28);
        Assert.Equal(28, replacement.Length);
        Assert.True(SameStorage(replacement.Span, storage.Template(1).Section("State")));
        storage.CommitStateGrowth(1);
        storage.Clear(8);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(28, storage.Template(1).Section("State").Length);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
        Assert.Equal(0u, Header(storage, 20));
        // 소멸은 설치 때부터 있던 예약 할당(37)과 뒤늦게 생긴 State 할당을 모두 반납했다
        Assert.All(installed.Sections, section => Assert.Equal(0, section.Capacity));

        // 소멸 직전에 목록을 줄여도 정리는 성공한다
        FortTemplate shrink = new();
        shrink.Sections.AddRange([new RawSectionBuffer(), new RawSectionBuffer(), RawSectionBuffer.FromBytes([7, 8])]);
        storage.Install(1, shrink, "shrink");
        storage.Template(1).Sections.Clear();
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
        storage.Install(1, new FortTemplate(), "empty");
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>파싱 사본이 아닌 현재 원시 Subscriber 섹션과 원본 signed 문자열 경계를 읽는다. 2026-11-10 F3 미뤄 둔 전수 재생(0.302)에서 추가됐다.</summary>
    [Fact]
    public void CurrentSubscriberSectionReadsLiveBytes()
    {
        // cpp `SoloMapCatalogTests.cpp`의 `CurrentSubscriberSection`을 옮겼다.
        FortTemplate fort = EmptyFort();
        fort.Sections[0] = RawSectionBuffer.FromBytes([7, 0, 0, 0, 5, 0, (byte)'A', 0, (byte)'B', (byte)'C', (byte)'D']);
        fort.Subscriber = new FortSubscriber { Id = 7, Name = "stale" };
        CampaignTemplateStorage storage = new();
        storage.Install(0, fort, "sample");
        storage.Borrow(8, 0);
        Assert.Equal("A", storage.ReadSubscriberName(8));
        Assert.Equal("Subscriber", storage.CurrentSection(0));
        // 같은 현재 버퍼를 직접 바꾸면 다음 조회에 그대로 보인다.
        storage.Template(0).Sections[0] = RawSectionBuffer.FromBytes([1, 0, 0, 0, 2, 0, (byte)'N', (byte)'e']);
        Assert.Equal("Ne", storage.ReadSubscriberName(0));
        // signed 길이 0xffff는 거부한다.
        RawSectionBuffer bytes = storage.Template(0).Sections[0];
        bytes.Span[4] = 0xff;
        bytes.Span[5] = 0xff;
        Assert.ThrowsAny<Exception>(() => storage.ReadSubscriberName(8));
        // 길이 0은 빈 이름이다.
        storage.Template(0).Sections[0] = RawSectionBuffer.FromBytes([1, 0, 0, 0, 0, 0]);
        Assert.Equal("", storage.ReadSubscriberName(0));
        // 길이 255는 255바이트를 그대로 읽는다.
        byte[] big = new byte[261];
        big[0] = 1;
        big[4] = 0xff;
        Array.Fill(big, (byte)'x', 6, 255);
        storage.Template(0).Sections[0] = RawSectionBuffer.FromBytes(big);
        Assert.Equal(new string('x', 255), storage.ReadSubscriberName(8));
        // 길이 256은 목록 256바이트 출력을 넘어 거부한다.
        bytes = storage.Template(0).Sections[0];
        bytes.Span[4] = 0;
        bytes.Span[5] = 1;
        bytes.Resize(262);
        bytes.Span[261] = (byte)'x';
        Assert.ThrowsAny<Exception>(() => storage.ReadSubscriberName(0));
        // 비운 버퍼는 거부한다.
        bytes.Clear();
        Assert.ThrowsAny<Exception>(() => storage.ReadSubscriberName(8));
    }

    /// <summary>Player State getter가 같은 목록/current를 선택하고 빌린 헤더와 같은 커서를 공유한다.</summary>
    [Fact]
    public void PlayerSelectsCurrentSectionAndSharedCursor()
    {
        FortTemplate fort = new();
        SetSection(fort, "State", [0x12, 0x34, 0x56]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "selection");
        storage.Borrow(8, 1);
        Assert.Equal("", storage.CurrentSection(1));
        storage.State(1).Position = 2;
        RawPlayerTemplateState state = PlayerState(storage, select: true);

        byte[] first = state.Get(8).ToArray();
        Assert.Equal(28, first.Length);
        Assert.Equal(new byte[] { 0x12, 0x34, 0x56 }, first[..3]);
        Assert.Equal("State", storage.CurrentSection(1));
        Assert.Equal(28u, storage.State(1).Position);
        storage.State(8).Position = 17;
        Span<byte> second = state.Get(1);
        Assert.True(SameStorage(second, storage.Template(8).Section("State")));
        Assert.Equal(0u, storage.State(8).Position);
        Assert.Equal("State", storage.CurrentSection(8));
        storage.Clear(8);
        Assert.Equal("State", storage.CurrentSection(1));
        Assert.Equal(1, storage.OwnedImages);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>실제 미션 파일에서 읽기/쓰기 선택을 바꿔도 같은 요새의 바이트와 커서·선택을 공유한다.</summary>
    [Fact]
    public void TerritoryAndStateSelectSameManager()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, RealFort("thewarbegins"), "thewarbegins");
        storage.Borrow(8, 1);
        storage.Territory(1).Position = 91;
        storage.State(1).Position = 17;

        FortTerritoryStream territory = storage.SelectTerritory(8);
        Assert.Same(storage.Territory(1), territory);
        Assert.Equal(0u, territory.Position);
        Assert.Equal(120u, territory.End);
        Assert.Equal("Territory", storage.CurrentSection(1));
        Assert.Equal(17u, storage.State(1).Position);
        storage.Territory(8).Position = 120;

        PlayerTemplateStateStream state = storage.SelectState(1);
        Assert.Same(storage.State(8), state);
        Assert.Equal(0u, state.Position);
        Assert.Equal("State", storage.CurrentSection(8));
        Assert.Equal(120u, storage.Territory(1).Position);
        ReadOnlySpan<byte> before = storage.Template(1).Section("Territory");
        Assert.Equal(120, before.Length);
        Assert.True(SameStorage(before, territory.Bytes.Span));
        Assert.True(before.SequenceEqual(territory.Bytes.Span));

        // 요새의 원시 섹션을 바꾸면 같은 커서가 그 바이트를 본다
        storage.Template(1).Sections[IndexOf("Territory")][0] ^= 0x40;
        Assert.Equal(storage.Template(8).Section("Territory")[0], territory.Bytes.Span[0]);
        storage.SelectTerritory(1);
        Assert.Equal(0u, territory.Position);
        Assert.Equal("Territory", storage.CurrentSection(8));
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>
    /// Money·Badges·Technology의 선택 모드(읽기/쓰기 허용)와 빈 섹션, 빌린 커서의 공유를 확인한다. cpppj 원본 검사는 실제 Money/Meta 로더(F2)를
    /// 함께 돌리지만 이 단계는 저장소만 옮겼으므로 로더가 진행하는 위치를 직접 써서 같은 커서를 확인한다.
    /// </summary>
    [Fact]
    public void EconomyMetaSelectionModesAndBorrowedCursors()
    {
        FortTemplate fort = new();
        // 35칸을 모두 빈 섹션으로 둔 뒤 Money와 Badges만 채운다
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        fort.Sections[IndexOf("Money")] = RawSectionBuffer.FromBytes([0xef, 0xbe, 0xad, 0xde]);
        fort.Sections[IndexOf("Badges")] = RawSectionBuffer.FromBytes([2, 80, 7, 81, 9]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "economy-meta");
        storage.Borrow(8, 1);

        FortMoneyCursor money = storage.SelectMoney(8);
        Assert.Same(storage.Money(1), money);
        Assert.Equal(0u, money.Position);
        Assert.Equal(4u, money.End);
        Assert.Equal("Money", storage.CurrentSection(1));
        // 로더가 4바이트를 소비한 위치는 다른 헤더에서도 같은 커서로 보인다
        money.Position = 4;
        Assert.Equal(4u, storage.Money(1).Position);

        FortMetaStream badges = storage.SelectBadges(1);
        Assert.Same(storage.Badges(8), badges);
        Assert.True(badges.Present);
        Assert.Equal(5u, badges.End);
        Assert.Equal(new byte[] { 2, 80, 7, 81, 9 }, badges.Bytes.ToArray());
        badges.Position = 5;
        Assert.Equal(5u, storage.Badges(8).Position);
        Assert.Equal(4u, storage.Money(1).Position);

        // 비어 있는 Technology는 쓰기 허용 선택에서 진단 없이 선택되고 present는 거짓이다
        FortMetaStream technology = storage.SelectTechnology(8);
        Assert.True(technology.Bytes.IsEmpty);
        Assert.False(technology.Present);
        Assert.Equal(0u, technology.End);
        Assert.Equal("Technology", storage.CurrentSection(1));

        // 비운 Badges도 쓰기 허용 선택이라 그대로 전달한다
        storage.Template(1).Sections[IndexOf("Badges")].Release();
        FortMetaStream empty = storage.SelectBadges(8);
        Assert.True(empty.Bytes.IsEmpty);
        Assert.Equal(0u, empty.End);
        Assert.False(empty.Present);

        // 비운 Money는 읽기 선택의 빈 읽기 진단으로 멈추고, 그래도 현재 선택은 Money로 남는다
        storage.Template(1).Sections[IndexOf("Money")].Release();
        Assert.Throws<InvalidOperationException>(() => storage.SelectMoney(1));
        Assert.Equal("Money", storage.CurrentSection(8));
        Assert.Equal(0u, storage.Money(8).End);
        storage.Clear(8);
        storage.ClearAll();
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>각 Storage는 자기 직렬화 전역을 따로 소유하고, 설치는 자기 전역에만 0을 쓴다. 전역을 명시 공유한 Storage는 같은 값을 본다.</summary>
    [Fact]
    public void SeparateClientsAndDefaultOwners()
    {
        CampaignTemplateStorage first = new();
        CampaignTemplateStorage second = new();
        Assert.NotSame(first.SerializationGlobals, second.SerializationGlobals);
        first.SerializationVersion = 7;
        second.SerializationVersion = 9;
        first.Install(0, new FortTemplate(), "first");
        Assert.Equal(0u, first.SerializationVersion);
        Assert.Equal(9u, second.SerializationVersion);
        first.SerializationVersion = 8;
        second.Install(0, new FortTemplate(), "second");
        Assert.Equal(8u, first.SerializationVersion);
        Assert.Equal(0u, second.SerializationVersion);

        FortSerializationGlobals shared = new() { Version = 31 };
        FortSerializationGlobals other = new() { Version = 41 };
        CampaignTemplateStorage clientA = new(shared);
        CampaignTemplateStorage clientB = new(other);
        CampaignTemplateStorage clientAgain = new(shared);
        Assert.Same(shared, clientA.SerializationGlobals);
        clientA.Install(0, new FortTemplate(), "shared");
        Assert.Equal(0u, clientAgain.SerializationVersion);
        Assert.Equal(41u, clientB.SerializationVersion);
    }

    /// <summary>같은 Client 이름표 owner를 명시 공유하고, 생략하면 새 owner를 만든다.</summary>
    [Fact]
    public void SectionNamesOwnerIsSharedOrDefaulted()
    {
        ClientTemplateSectionNames names = new();
        CampaignTemplateStorage shared = new(sectionNames: names);
        Assert.Same(names, shared.SectionNamesOwner);
        Assert.NotSame(names, new CampaignTemplateStorage().SectionNamesOwner);
    }

    /// <summary>슬롯 범위와 빈 칸·자기 빌림·원래 소유자를 반납하는 빌림을 명시 오류로 거부한다.</summary>
    [Fact]
    public void HeaderAndBorrowContractsAreRejected()
    {
        CampaignTemplateStorage storage = new();
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.Clear(9));
        Assert.Throws<InvalidOperationException>(() => storage.Template(3));
        Assert.Throws<InvalidOperationException>(() => storage.Borrow(2, 1));
        storage.Install(1, new FortTemplate(), "owner");
        Assert.Throws<ArgumentException>(() => storage.Borrow(1, 1));
        storage.Borrow(8, 1);
        // 빌림 헤더에는 핸들과 빌림 값 1이 쓰인다
        Assert.Equal(Header(storage, 1 * 20), Header(storage, 8 * 20));
        Assert.Equal(1u, Header(storage, 8 * 20 + 4));
        Assert.Equal(0u, Header(storage, 1 * 20 + 4));
        // 소유자 칸에 같은 manager를 빌리면 소유자를 반납하게 되므로 거부한다
        Assert.Throws<ArgumentException>(() => storage.Borrow(1, 8));
        Assert.Equal(1, storage.OwnedImages);
        // 다른 manager를 가진 칸에 빌리면 그 칸의 manager를 먼저 반납한다
        storage.Install(2, new FortTemplate(), "other");
        Assert.Equal(2, storage.OwnedImages);
        storage.Borrow(2, 1);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Same(storage.Template(1), storage.Template(2));
    }

    /// <summary>같은 칸에 다시 설치하면 이전 manager를 반납하고 새 manager가 새 요새·커서를 가진다. 잘못된 이름은 칸을 비운 채 거부한다.</summary>
    [Fact]
    public void InstallReplacesPreviousManagerAndValidatesName()
    {
        CampaignTemplateStorage storage = new();
        FortTemplate firstFort = new();
        SetSection(firstFort, "State", [1, 2, 3]);
        storage.Install(1, firstFort, "first");
        PlayerTemplateStateStream firstState = storage.State(1);
        FortTemplate secondFort = new();
        SetSection(secondFort, "State", [9]);
        storage.Install(1, secondFort, "second");
        Assert.Equal(1, storage.OwnedImages);
        Assert.NotSame(firstState, storage.State(1));
        Assert.Same(secondFort, storage.Template(1));
        Assert.Equal("second", storage.Name(1));
        Assert.Equal(new byte[] { 9 }, storage.State(1).Bytes.ToArray());
        // 새 manager 생성자는 수정 전 파일 서명 DWORD를 0으로, FE 판정을 0xffffffff로 둔다
        Assert.Equal((0u, 0xffffffffu), storage.FileHeader(1));

        Assert.Throws<ArgumentException>(() => storage.Install(1, new FortTemplate(), new string('a', 256)));
        Assert.Equal(0, storage.OwnedImages);
        Assert.Throws<ArgumentException>(() => storage.Install(1, new FortTemplate(), "a\0b"));
        // 255바이트 이름은 허용한다
        storage.Install(1, new FortTemplate(), new string('b', 255));
        Assert.Equal(new string('b', 255), storage.Name(1));
    }

    /// <summary>설치 직후의 섹션 연결 순서는 이름표 순서이고, 선택 전에는 현재 섹션이 없다.</summary>
    [Fact]
    public void SectionOrderFollowsSeedOrderAndCurrentSectionStartsEmpty()
    {
        FortTemplate fort = new();
        for (int index = 0; index < 3; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "three");
        Assert.Equal(new[] { "Subscriber", "State", "Mission" }, storage.SectionOrder(1));
        Assert.Equal("", storage.CurrentSection(1));
        storage.Install(2, RealFort("thewarbegins"), "real");
        Assert.Equal(FortTemplate.SectionNames.Take(storage.Template(2).Sections.Count), storage.SectionOrder(2));
        // 요새가 늘어나면 다음 동기화(선택·조회)가 새 표준 위치만 목록 끝에 붙인다
        storage.Template(1).Sections.Add(new RawSectionBuffer());
        _ = storage.SelectState(1);
        Assert.Equal(new[] { "Subscriber", "State", "Mission", "CoreData" }, storage.SectionOrder(1));
        Assert.Equal("State", storage.CurrentSection(1));
    }

    /// <summary>Subscriber 읽기는 앞의 ID DWORD를 건너뛰고 길이 WORD 뒤의 이름을 첫 NUL까지 돌려주며 같은 노드의 pos를 진행한다.</summary>
    [Fact]
    public void ReadSubscriberNameSkipsIdAndStopsAtNul()
    {
        FortTemplate fort = new();
        SetSection(fort, "Subscriber", [1, 0, 0, 0, 2, 0, (byte)'M', (byte)'e']);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "subscriber");
        Assert.Equal("Me", storage.ReadSubscriberName(1));
        Assert.Equal("Subscriber", storage.CurrentSection(1));

        FortTemplate withNul = new();
        SetSection(withNul, "Subscriber", [1, 0, 0, 0, 3, 0, (byte)'A', 0, (byte)'B']);
        storage.Install(2, withNul, "nul");
        Assert.Equal("A", storage.ReadSubscriberName(2));
    }

    /// <summary>Subscriber가 없거나 짧거나 길이가 범위를 넘으면 가짜 이름 없이 명시 오류로 멈춘다.</summary>
    [Fact]
    public void ReadSubscriberNameRejectsMissingAndOutOfRange()
    {
        CampaignTemplateStorage storage = new();
        // 섹션이 없는 요새: 읽기 선택의 누락 진단
        storage.Install(1, new FortTemplate(), "none");
        Assert.Throws<InvalidOperationException>(() => storage.ReadSubscriberName(1));

        FortTemplate shortFort = new();
        SetSection(shortFort, "Subscriber", [1, 0, 0, 0, 5, 0, (byte)'a']);
        storage.Install(2, shortFort, "short");
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.ReadSubscriberName(2));

        FortTemplate noLength = new();
        SetSection(noLength, "Subscriber", [1, 0, 0, 0, 7]);
        storage.Install(3, noLength, "nolength");
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.ReadSubscriberName(3));

        // 목록 256바이트 출력 한도(길이 256 이상)는 읽기 전에 거부한다
        byte[] longName = new byte[4 + 2 + 256];
        longName[0] = 1;
        longName[4] = 0;
        longName[5] = 1;
        FortTemplate tooLong = new();
        SetSection(tooLong, "Subscriber", longName);
        storage.Install(4, tooLong, "toolong");
        Assert.Throws<ArgumentOutOfRangeException>(() => storage.ReadSubscriberName(4));
    }

    /// <summary>
    /// 읽기 선택(지형·Money)은 빈 섹션을 진단으로 거부하고, 쓰기 허용 선택(State·Badges·Technology)은 빈 섹션도 선택한다.
    /// 거부된 선택도 원본처럼 현재 선택은 그 섹션으로 남는다.
    /// </summary>
    [Fact]
    public void EmptyReadsAreRejectedButWritableSelectionsAreAccepted()
    {
        FortTemplate fort = new();
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "all-empty");
        Assert.Throws<InvalidOperationException>(() => storage.SelectTerritory(1));
        Assert.Equal("Territory", storage.CurrentSection(1));
        Assert.Throws<InvalidOperationException>(() => storage.SelectMoney(1));
        Assert.Equal("Money", storage.CurrentSection(1));
        Assert.False(storage.SelectState(1).Present);
        Assert.Equal("State", storage.CurrentSection(1));
        Assert.False(storage.SelectBadges(1).Present);
        Assert.False(storage.SelectTechnology(1).Present);
        Assert.Equal("Technology", storage.CurrentSection(1));
    }

    /// <summary>
    /// 커서의 끝은 섹션의 크기이고 예약 용량이 아니다. 존재 여부(present)는 용량으로 정해져,
    /// 크기 0이어도 용량이 있으면 존재하고 용량이 없으면(값형 설치의 빈 섹션) 없다.
    /// </summary>
    [Fact]
    public void CursorEndFollowsSizeAndPresenceFollowsCapacity()
    {
        FortTemplate fort = new();
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        fort.Sections[IndexOf("State")] = RawSectionBuffer.FromBytes([1, 2, 3]);
        fort.Sections[IndexOf("State")].Reserve(100);
        fort.Sections[IndexOf("Badges")].Reserve(37);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "reserved");
        PlayerTemplateStateStream state = storage.State(1);
        Assert.Equal(3u, state.End);
        Assert.Equal(3, state.Bytes.Length);
        Assert.True(state.Present);
        // 크기 0에 용량 37인 Badges는 존재하고, 용량 0인 Technology는 없다
        FortMetaStream badges = storage.Badges(1);
        Assert.Equal(0u, badges.End);
        Assert.True(badges.Present);
        Assert.False(storage.Technology(1).Present);
    }

    /// <summary>이미 같은 manager를 빌린 칸에 다시 빌려도 오류가 아니며 manager를 반납하지 않는다.</summary>
    [Fact]
    public void BorrowingTwiceKeepsTheSameManager()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, new FortTemplate(), "owner");
        storage.Borrow(8, 1);
        storage.Borrow(8, 1);
        Assert.Equal(1, storage.OwnedImages);
        Assert.Equal(1u, Header(storage, 8 * 20 + 4));
        Assert.Same(storage.Template(1), storage.Template(8));
    }

    /// <summary>소멸은 저장소가 돌려준 커서의 범위와 위치를 비워서 이후에 그 참조로 반납된 버퍼를 읽지 않게 한다(C++에서는 정의되지 않은 참조).</summary>
    [Fact]
    public void DestroyClearsHeldStateAndTerritoryCursors()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, RealFort("thewarbegins"), "thewarbegins");
        PlayerTemplateStateStream state = storage.State(1);
        FortTerritoryStream territory = storage.Territory(1);
        Assert.False(state.Bytes.IsEmpty);
        Assert.False(territory.Bytes.IsEmpty);
        state.Position = 3;
        territory.Position = 5;
        storage.Clear(1);
        Assert.True(state.Bytes.IsEmpty);
        Assert.Equal(0u, state.Position);
        Assert.Equal(0u, state.End);
        Assert.False(state.Present);
        Assert.True(territory.Bytes.IsEmpty);
        Assert.Equal(0u, territory.Position);
        Assert.Equal(0u, territory.End);
        Assert.Equal(0, storage.OwnedImages);
    }

    /// <summary>선택 없이 커서를 처음 공개할 때 그 이름의 현재 노드가 없으면 명시 오류다.</summary>
    [Fact]
    public void LoadCursorRequiresExistingNode()
    {
        CampaignTemplateStorage storage = new();
        storage.Install(1, new FortTemplate(), "empty");
        Assert.Throws<InvalidOperationException>(() => storage.Money(1));
        Assert.Throws<InvalidOperationException>(() => storage.Badges(1));
        Assert.Throws<InvalidOperationException>(() => storage.Technology(1));
        // 선택은 누락 진단으로 멈춘다
        Assert.Throws<InvalidOperationException>(() => storage.SelectMoney(1));
        Assert.Throws<InvalidOperationException>(() => storage.SelectTerritory(1));
    }

    /// <summary>State 확장은 이전 할당을 복사 완료 전까지 보존하고, 복사 중 다시 확장하거나 확정 전에 두 번 확장하는 것을 거부한다.</summary>
    [Fact]
    public void GrowStateKeepsPreviousUntilCommitted()
    {
        FortTemplate fort = new();
        SetSection(fort, "State", [5, 6]);
        CampaignTemplateStorage storage = new();
        storage.Install(1, fort, "grow");
        Memory<byte> grown = storage.GrowState(1, 28);
        Assert.Equal(28, grown.Length);
        Assert.Equal(28, storage.Template(1).Sections[IndexOf("State")].Count);
        // 이전 State 할당은 아직 반납되지 않았으므로 두 번째 확장은 거부한다
        Assert.Throws<InvalidOperationException>(() => storage.GrowState(1, 40));
        storage.CommitStateGrowth(1);
        Assert.Equal(40, storage.GrowState(1, 40).Length);
        // State 표준 위치가 아직 없는 요새도 확장하면 위치까지 빈 섹션으로 늘린다
        storage.Install(2, new FortTemplate(), "none");
        Assert.Equal(28, storage.GrowState(2, 28).Length);
        Assert.Equal(IndexOf("State") + 1, storage.Template(2).Sections.Count);
    }
}
