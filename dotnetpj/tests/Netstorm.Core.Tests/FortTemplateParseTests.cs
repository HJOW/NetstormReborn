using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// <see cref="FortTemplate.Parse"/>의 합성 입력 검사다: 섹션 분리, 타입 번호 변환, 오브젝트 선택 필드, 내용물, 덱, 청크 버전별 차이, 형식 오류.
/// cpppj <c>FortTests</c>의 FortTemplate 2개를 직역했고 복제·갱신 경계(직접)를 더했다. 실제 .fort 파일 대조는 <see cref="RiftTypeOriginalFilesTests"/>가 맡는다. (F1, 2026-11-02)
/// </summary>
public sealed class FortTemplateParseTests
{
    /// <summary>섹션 번호다(실행 파일의 이름 목록 순서).</summary>
    private const int Subscriber = 0, Chaff = 4, Territory = 6, TypeNames = 7, Technology = 9, Money = 10, Deck = 11, Terr00 = 15, Terr01 = 16, Terr02 = 17;

    /// <summary>검사용 파일의 타입 번호다. 실제 파일처럼 0번은 이름 없는 타입이다.</summary>
    private const byte None = 0, Cannon = 1, Priest = 2, Bomb = 3, IslandType = 4, Factory = 5, Nugget = 6, Unknown = 7;

    /// <summary>리틀 엔디언 정수를 바이트 목록 끝에 붙인다.</summary>
    /// <param name="bytes">붙일 목록.</param>
    /// <param name="value">정수 값.</param>
    /// <param name="size">바이트 수.</param>
    private static void Put(List<byte> bytes, uint value, int size)
    {
        // 낮은 바이트부터 쓴다
        for (int index = 0; index < size; index++)
        {
            bytes.Add((byte)(value >> (index * 8)));
        }
    }

    /// <summary>섹션 35개(비어 있는 것은 길이 2)로 .fort 바이트를 만든다. 번호는 저장 순서다.</summary>
    /// <param name="bodies">섹션 번호별 본문.</param>
    /// <param name="sectionCount">만들 섹션 수.</param>
    private static byte[] MakeFort(Dictionary<int, byte[]> bodies, int sectionCount = FortTemplate.SectionCount)
    {
        List<byte> file = [0x46, 0x00];
        // 길이 필드에는 자기 2바이트가 들어 있다
        for (int index = 0; index < sectionCount; index++)
        {
            byte[]? body = bodies.GetValueOrDefault(index);
            Put(file, (uint)((body?.Length ?? 0) + 2), 2);
            if (body is not null)
            {
                file.AddRange(body);
            }
        }
        return [.. file];
    }

    /// <summary>섹션·타입 번호 변환·오브젝트 선택 필드·내용물·덱을 한 파일로 검사한다.</summary>
    [Fact]
    public void ReadsSectionsObjectsAndContents()
    {
        RiftTypeTableTests.TestTypes source = new(OriginalEdition.Patch1078);
        RiftTypeTable table = source.Table(OriginalEdition.Patch1078);
        Dictionary<int, byte[]> bodies = [];
        // Subscriber: ID + 길이 붙은 이름
        List<byte> subscriber = [];
        Put(subscriber, 0x12345678, 4);
        Put(subscriber, 3, 2);
        subscriber.AddRange([(byte)'F', (byte)'o', 0xe9]);
        bodies[Subscriber] = [.. subscriber];
        // Money: 539.0f
        List<byte> money = [];
        Put(money, 0x4406c000, 4);
        bodies[Money] = [.. money];
        bodies[Territory] = Enumerable.Repeat((byte)7, 0x78).ToArray();
        // TypeNames: 파일 번호 0~7의 이름 해시. 7번은 지금 없는 타입이다
        List<byte> names = [8];
        // 이름마다 해시 4바이트
        foreach (string name in new[] { "", "sunCannon", "priest", "bombHeal", "island", "sunFactory", "nugget", "goneType" })
        {
            Put(names, RiftTypeTable.NameHash(name), 4);
        }
        bodies[TypeNames] = [.. names];
        // Chaff(버전 2): 청크 둘. 첫 청크에 포대·다리 약식 표기·섬·사제(내용물)·작업장
        bodies[Chaff] =
        [
            2,
            99, 5, 0,
            0x12, Cannon, 9, 3,                                  // 프레임 9, 소유자 3
            0xf0, 0xfe, 4,                                       // 다리 약식 표기: 소유자 1, 클러스터 4
            0x00, IslandType, 0,                                 // 섬: 1바이트 건너뜀
            0x34, Priest, 0, 2, Bomb, 5, Nugget, 0xfe, 0xff,     // 소유자 0, 내용물 2개(QA 5, QB -2)
            0x56, Factory, 1, 9, 0,                              // 작업장 상태 1, 소유자 9, 빈 내용물
            99, 0, 0,
        ];
        // Terr00(버전 1): 작업장 상태 바이트가 없다
        bodies[Terr00] = [1, 99, 1, 0, 0x01, Factory, 2, 0];
        // Terr01(버전 0): 모든 타입 뒤에 상태 1바이트가 있고 소유자는 없다
        bodies[Terr01] = [0, 99, 2, 0, 0x02, Cannon, 7, 0xaa, 0x03, Factory, 0xbb, 0];
        // Terr02: 버전 바이트만 있으면 내용이 없는 섹션이다
        bodies[Terr02] = [2];
        // Technology: 포대(목록 플래그 4), 폭탄(QA 6, 플래그는 타입 것), 사제(플래그 4, 중첩 1개), 뒤에 0 하나
        bodies[Technology] = [3, Cannon, 4, Bomb, 6, Priest, 4, 1, Nugget, 1, 0, 0];
        // Deck: 항목 둘. 둘째는 변환할 수 없는 타입이다
        bodies[Deck] = [2, Cannon, 4, 1, 255, Unknown, 3, 0xff, 2];
        byte[] bytes = [.. MakeFort(bodies), 1, 2, 3]; // 35개 뒤의 남는 바이트
        FortTemplate fort = FortTemplate.Parse(bytes, table);

        Assert.True(fort.Sections.Count == 35 && fort.TrailingBytes == 3 && fort.Flag == 0);
        Assert.NotNull(fort.Subscriber);
        Assert.Equal(0x12345678u, fort.Subscriber.Id);
        Assert.Equal("Foé", fort.Subscriber.Name);
        Assert.Equal(539.0f, fort.Money!.Value);
        Assert.True(fort.Territory.Length == 0x78 && fort.Territory[0] == 7);
        Assert.True(fort.Section("typenames").Length == 33 && fort.Section("Reserved2").IsEmpty && fort.Section("nope").IsEmpty);
        // 변환: 이름 없는 0번은 이름 없는 현재 타입 가운데 가장 큰 번호가 된다(원본 004bd800 그대로)
        Assert.True(fort.HasTypeConversion && fort.TypeHashes.Count == 8);
        Assert.Equal(187, fort.ConvertType(None));
        Assert.True(fort.ConvertType(Cannon) == table.Find("sunCannon") && fort.ConvertType(Unknown) == 0);
        Assert.Equal(0xfe, fort.ConvertType(0xfe)); // 0xf6 이상은 바꾸지 않는다

        Assert.True(fort.Chaff.Present && fort.Chaff.Version == 2 && fort.Chaff.Chunks.Count == 2);
        IReadOnlyList<FortObject> objects = fort.Chaff.Chunks[0].Objects;
        Assert.True(objects.Count == 5 && fort.Chaff.Chunks[1].Objects.Count == 0);
        Assert.True(objects[0].CellX == 1 && objects[0].CellY == 2 && objects[0].Type == table.Find("sunCannon"));
        Assert.True(objects[0].Frame == 9 && objects[0].Owner == 3 && objects[0].FactoryState is null && objects[0].Contents is null);
        Assert.True(objects[1].Type == 82 && objects[1].ShorthandOwner == 1 && objects[1].BridgeFrame == 4 && objects[1].Owner is null);
        Assert.True(objects[2].Type == 94 && objects[2].IslandByte == 0);
        Assert.True(objects[3].Owner == 0 && objects[3].NormalizedOwner() == 1);
        Assert.NotNull(objects[3].Contents);
        Assert.True(objects[3].Contents!.Complete && objects[3].Contents!.Items.Count == 2);
        IReadOnlyList<FortContent> carried = objects[3].Contents!.Items;
        Assert.True(carried[0].Type == table.Find("bombHeal") && carried[0].Quantity == 5 && carried[0].ListFlags == 1 && !carried[0].ListFlagsStored);
        Assert.True(carried[1].Type == table.Find("nugget") && carried[1].Quantity == -2 && carried[1].ListFlags == 0x20 && !carried[1].ListFlagsStored);
        Assert.True(objects[4].FactoryState == 1 && objects[4].Owner == 9 && objects[4].NormalizedOwner() == 1);
        Assert.True(objects[4].Contents is not null && objects[4].Contents!.Items.Count == 0);

        Assert.True(fort.Territories[0].Version == 1 && fort.Territories[0].Chunks.Count == 1);
        Assert.True(fort.Territories[0].Chunks[0].Objects[0].FactoryState is null && fort.Territories[0].Chunks[0].Objects[0].Owner == 2);
        IReadOnlyList<FortObject> legacy = fort.Territories[1].Chunks[0].Objects;
        Assert.True(legacy[0].Frame == 7 && legacy[0].LegacyState == 0xaa && legacy[0].Owner is null);
        Assert.True(legacy[1].LegacyState == 0xbb && legacy[1].FactoryState is null && legacy[1].Contents is not null);
        Assert.True(!fort.Territories[2].Present && fort.Territories[2].Chunks.Count == 0 && !fort.Territories[3].Present);

        Assert.True(fort.Technology.Complete && fort.Technology.Items.Count == 3 && fort.TechnologyTrailingBytes == 1);
        Assert.True(fort.Technology.Items[0].ListFlags == 4 && fort.Technology.Items[0].ListFlagsStored);
        Assert.True(fort.Technology.Items[1].Quantity == 6 && fort.Technology.Items[1].ListFlags == 1 && !fort.Technology.Items[1].ListFlagsStored);
        Assert.True(fort.Technology.Items[2].ListFlagsStored && fort.Technology.Items[2].Contents.Count == 1);
        Assert.True(fort.Technology.Items[2].Contents[0].Quantity == 1 && fort.Technology.Items[2].Contents[0].ListFlags == 0x20);

        Assert.True(fort.Deck.Count == 2 && fort.Deck[0].Type == table.Find("sunCannon") && fort.Deck[0].Chance == 4);
        Assert.True(fort.Deck[0].Power == 1 && fort.Deck[0].Remaining == 255);
        Assert.True(fort.Deck[1].Type == 0 && fort.Deck[1].Power == -1 && fort.Deck[1].Remaining == 2);
    }

    /// <summary><c>TypeNames</c>가 비어 있으면 파일의 번호를 그대로 쓴다. 잘못된 입력은 예외다.</summary>
    [Fact]
    public void HandlesMissingConversionAndRejectsBrokenInput()
    {
        RiftTypeTableTests.TestTypes source = new(OriginalEdition.Patch1078);
        RiftTypeTable table = source.Table(OriginalEdition.Patch1078);
        byte cannon = (byte)table.Find("sunCannon");
        Dictionary<int, byte[]> bodies = [];
        bodies[Chaff] = [2, 99, 2, 0, 0x11, cannon, 1, 2, 0x22, 0, 0];
        FortTemplate fort = FortTemplate.Parse(MakeFort(bodies), table);
        Assert.True(!fort.HasTypeConversion && fort.Chaff.Chunks[0].Objects.Count == 2);
        Assert.True(fort.Chaff.Chunks[0].Objects[0].Type == cannon && fort.Chaff.Chunks[0].Objects[0].Owner == 2);
        Assert.Equal(0, fort.Chaff.Chunks[0].Objects[1].Type); // 타입 0: 뒤의 0 한 바이트만 읽는 빈 레코드
        Assert.True(fort.Subscriber is null && fort.Money is null && fort.Deck.Count == 0 && fort.Technology.Items.Count == 0);
        // 섹션이 35개보다 적은 파일은 있는 만큼만 읽는다
        Assert.Equal(5, FortTemplate.Parse(MakeFort(bodies, 5), table).Sections.Count);

        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse([0x47, 0], table));
        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse([0x46], table));
        bodies[Chaff] = [2, 98, 0, 0]; // 청크 표지가 'c'가 아니다
        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse(MakeFort(bodies), table));
        bodies[Chaff] = [2, 99, 1, 0, 0x11, cannon, 1]; // 소유자 바이트 앞에서 끝난다
        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse(MakeFort(bodies), table));
        bodies[Chaff] = [2, 99, 1, 0, 0x11, 0, 5]; // 타입 0 뒤의 바이트가 0이 아니다(원본 assert "num == 0")
        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse(MakeFort(bodies), table));
        bodies[Chaff] = [2, 99, 1, 0, 0x11, 200, 0]; // 타입 표 밖의 번호
        Assert.Throws<InvalidDataException>(() => FortTemplate.Parse(MakeFort(bodies), table));
        // 내용물이 변환할 수 없는 타입에서 끊기면 그 뒤는 읽지 않고 표시만 남긴다
        bodies.Clear();
        List<byte> typeNames = [1];
        Put(typeNames, RiftTypeTable.NameHash("sunCannon"), 4);
        bodies[TypeNames] = [.. typeNames];
        bodies[Technology] = [2, 0, 4, 9, 9];
        FortTemplate partial = FortTemplate.Parse(MakeFort(bodies), table);
        Assert.True(partial.Technology.Items.Count == 1 && !partial.Technology.Complete);
        Assert.Equal(1, partial.TechnologyTrailingBytes);
    }

    /// <summary>
    /// 직접 경계: (1) 파일 안의 모든 섹션 읽기 오류는 섹션 이름·위치를 메시지에 담는다. (2) <see cref="FortTemplate.RefreshObjectRecords"/>는 원시 섹션만 다시 해석하고 이전 변환표를 지운다.
    /// (3) <see cref="FortTemplate.Clone"/>은 해석 필드를 함께 복사하되 원본 배열을 공유하지 않는다.
    /// </summary>
    [Fact]
    public void RefreshAndCloneKeepParsedFieldsIndependent()
    {
        RiftTypeTableTests.TestTypes source = new(OriginalEdition.Patch1078);
        RiftTypeTable table = source.Table(OriginalEdition.Patch1078);
        byte cannon = (byte)table.Find("sunCannon");
        Dictionary<int, byte[]> bodies = [];
        bodies[Chaff] = [2, 99, 1, 0, 0x11, cannon, 1, 2];
        bodies[Territory] = Enumerable.Repeat((byte)3, 0x78).ToArray();
        bodies[Deck] = [1, cannon, 4, 1, 9];
        FortTemplate fort = FortTemplate.Parse(MakeFort(bodies), table);
        FortTemplate copy = fort.Clone();
        Assert.Equal(fort.Chaff.Chunks[0].Objects[0].Type, copy.Chaff.Chunks[0].Objects[0].Type);
        Assert.Equal(fort.Deck.Count, copy.Deck.Count);
        Assert.NotSame(fort.Territory, copy.Territory);
        Assert.Equal(fort.Territory, copy.Territory);
        // 복사본의 목록·배열을 바꿔도 원본은 그대로다
        copy.Deck.Clear();
        copy.Territory[0] = 9;
        Assert.Single(fort.Deck);
        Assert.Equal(3, fort.Territory[0]);

        // TypeNames가 있으면 저장 번호 0이 sunCannon이 된다. 섹션을 직접 바꾼 뒤 갱신하면 객체 해석이 새 섹션을 따르고, 사라진 TypeNames의 변환표는 비워진다
        List<byte> typeNames = [1];
        Put(typeNames, RiftTypeTable.NameHash("sunCannon"), 4);
        bodies[TypeNames] = [.. typeNames];
        bodies[Chaff] = [2, 99, 1, 0, 0x11, 0, 1, 2];
        FortTemplate withNames = FortTemplate.Parse(MakeFort(bodies), table);
        Assert.True(withNames.HasTypeConversion);
        Assert.Equal(cannon, withNames.TypeConversion[0]);
        Assert.Equal(cannon, withNames.Chaff.Chunks[0].Objects[0].Type);
        Assert.True(withNames.Chaff.Chunks[0].Objects[0].Frame == 1 && withNames.Chaff.Chunks[0].Objects[0].Owner == 2);
        withNames.Sections[TypeNames].Release();
        withNames.Sections[Chaff] = RawSectionBuffer.FromBytes([2, 99, 1, 0, 0x11, 0, 0]);
        withNames.RefreshObjectRecords(table);
        Assert.False(withNames.HasTypeConversion);
        Assert.Empty(withNames.TypeHashes);
        Assert.Equal(0, withNames.TypeConversion[0]);
        // 변환표가 없으면 저장 번호 0이 그대로 빈 타입이라 뒤의 0 한 바이트만 읽는 빈 레코드가 된다
        Assert.Equal(0, withNames.Chaff.Chunks[0].Objects[0].Type);
        Assert.Throws<ArgumentNullException>(() => withNames.RefreshObjectRecords(null!));
        Assert.Throws<ArgumentNullException>(() => FortTemplate.Parse(MakeFort(bodies), null!));
    }
}
