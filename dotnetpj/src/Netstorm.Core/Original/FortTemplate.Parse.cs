namespace Netstorm.Core.Original;

/// <summary>
/// <see cref="FortTemplate"/>의 파일 해석 부분이다(cpppj <c>o/Template.cpp</c>). 원본 Datamanager.cpp 0044d380 ↔ CD 0044c750(섹션 읽기), Template.cpp 004bd800 ↔ CD 00427e00(타입 번호 변환),
/// 004bdc60 ↔ CD 00428320(오브젝트), 004bd130 ↔ CD 00426dd0(내용물), 004be160 ↔ CD 004288a0(청크), 004bf190 ↔ CD 00429670(덱), 004bd450(Subscriber), 00483300(Money)을 구조로 읽는다.
/// 범위는 파일을 구조로 읽는 데까지다. 원본은 읽으면서 곧바로 오브젝트를 만들어 월드에 놓는데 그 부분과 <c>State</c>·<c>Mission</c>·<c>CoreData</c>·<c>Badges</c>·<c>CompressedData</c> 섹션의 해석은
/// 옮기지 않았다(원시 바이트로 보존, F2 Fort 적재가 맡는다).
/// </summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed partial class FortTemplate
{
    /// <summary>청크 레코드의 시작 표지('c')다.</summary>
    private const byte ChunkMark = 99;

    /// <summary><c>Territory</c> 섹션의 고정 길이(6바이트 × 20, 원본 004bd660)다.</summary>
    private const int TerritoryBytes = 0x78;

    /// <summary>원본 memstream처럼 리틀 엔디언으로 읽고 끝을 넘으면 예외를 던진다(원본은 assert "pos&lt;=end"). 섹션 이름은 오류 메시지에만 쓴다.</summary>
    private ref struct SectionStream
    {
        /// <summary>읽을 섹션 본문이다.</summary>
        private readonly ReadOnlySpan<byte> _bytes;

        /// <summary>오류 메시지에 쓰는 섹션 이름이다.</summary>
        private readonly string _name;

        /// <summary>현재 읽기 위치다.</summary>
        private int _position;

        /// <summary>섹션 본문과 이름으로 읽기 흐름을 만든다.</summary>
        /// <param name="bytes">섹션 본문.</param>
        /// <param name="name">섹션 이름.</param>
        public SectionStream(ReadOnlySpan<byte> bytes, string name)
        {
            _bytes = bytes;
            _name = name;
            _position = 0;
        }

        /// <summary>남은 바이트 수다.</summary>
        public readonly int Remaining => _bytes.Length - _position;

        /// <summary>1바이트를 읽는다.</summary>
        public byte U8()
        {
            Require(1);
            return _bytes[_position++];
        }

        /// <summary>부호 없는 16비트를 읽는다.</summary>
        public ushort U16()
        {
            Require(2);
            ushort value = (ushort)(_bytes[_position] | (_bytes[_position + 1] << 8));
            _position += 2;
            return value;
        }

        /// <summary>부호 없는 32비트를 읽는다.</summary>
        public uint U32()
        {
            uint low = U16();
            return low | ((uint)U16() << 16);
        }

        /// <summary>형식 오류를 섹션 이름·위치와 함께 알린다.</summary>
        /// <param name="message">오류 내용.</param>
        /// <exception cref="InvalidDataException">항상 던진다.</exception>
        [System.Diagnostics.CodeAnalysis.DoesNotReturn]
        public readonly void Fail(string message) =>
            throw new InvalidDataException($"fort {_name} @{_position}: {message}");

        /// <summary>읽을 만큼 남았는지 확인한다.</summary>
        /// <param name="count">읽으려는 바이트 수.</param>
        private readonly void Require(int count)
        {
            if (Remaining < count)
            {
                Fail("pos<=end");
            }
        }
    }

    /// <summary>변환한 타입 번호의 타입 정보를 찾는다. 표 밖의 번호는 형식 오류다.</summary>
    /// <param name="types">타입 표.</param>
    /// <param name="type">변환한 타입 번호.</param>
    /// <param name="stream">오류 위치를 알릴 흐름.</param>
    private static OriginalTypeRecord TypeOf(RiftTypeTable types, int type, in SectionStream stream)
    {
        if (type < 0 || type >= types.Types.Count)
        {
            stream.Fail("type number outside the type table");
        }
        return types.Types[type];
    }

    /// <summary>내용물 목록을 읽는다(원본 004bd130). 개수 바이트만큼 항목을 읽으며 container 타입은 중첩 목록을 읽는다.</summary>
    /// <param name="stream">섹션 흐름.</param>
    /// <param name="fort">타입 번호 변환표를 가진 파일.</param>
    /// <param name="types">타입 표.</param>
    /// <param name="containerListFlags">그릇 타입의 +0xfc 값.</param>
    private static FortContentList ReadContents(ref SectionStream stream, FortTemplate fort, RiftTypeTable types, uint containerListFlags)
    {
        List<FortContent> items = [];
        bool complete = true;
        // 개수 바이트만큼 항목을 읽는다
        for (byte count = stream.U8(); count != 0; count--)
        {
            byte storedType = stream.U8();
            int type = fort.ConvertType(storedType);
            if (type == 0)
            {
                complete = false; // 원본은 여기서 읽기를 그만두고 0을 돌려준다
                break;
            }
            OriginalTypeRecord record = TypeOf(types, type, in stream);
            int quantity = 0;
            if ((record.Flags1 & TypeFlag1.SaveQA) != 0)
            {
                quantity = stream.U8();
            }
            if ((record.Flags1 & TypeFlag1.SaveQB) != 0)
            {
                quantity = unchecked((short)stream.U16());
            }
            uint listFlags = record.ContentListFlags;
            if (listFlags == 0)
            {
                listFlags = containerListFlags;
            }
            bool listFlagsStored = false;
            if (listFlags == 0)
            {
                listFlags = stream.U8();
                listFlagsStored = true;
            }
            if ((record.Flags1 & TypeFlag1.Container) != 0)
            {
                FortContentList nested = ReadContents(ref stream, fort, types, record.ContainerListFlags);
                items.Add(new FortContent
                {
                    StoredType = storedType,
                    Type = type,
                    Quantity = quantity,
                    ListFlags = listFlags,
                    ListFlagsStored = listFlagsStored,
                    Contents = nested.Items,
                    ContentsComplete = nested.Complete,
                });
                if (!nested.Complete)
                {
                    complete = false; // 원본: 중첩이 실패하면 0
                    break;
                }
                continue;
            }
            items.Add(new FortContent
            {
                StoredType = storedType,
                Type = type,
                Quantity = quantity,
                ListFlags = listFlags,
                ListFlagsStored = listFlagsStored,
            });
        }
        return new FortContentList { Items = items, Complete = complete };
    }

    /// <summary>오브젝트 레코드 하나를 읽는다(원본 004bdc60의 읽기 부분).</summary>
    /// <param name="stream">섹션 흐름.</param>
    /// <param name="fort">타입 번호 변환표를 가진 파일.</param>
    /// <param name="types">타입 표.</param>
    /// <param name="version">청크 섹션 버전.</param>
    private static FortObject ReadObject(ref SectionStream stream, FortTemplate fort, RiftTypeTable types, byte version)
    {
        byte position = stream.U8();
        byte storedType = stream.U8();
        int type;
        int? shorthandOwner = null;
        if (storedType < BridgeShorthandMin)
        {
            type = fort.ConvertType(storedType);
        }
        else
        {
            shorthandOwner = 0xff - storedType;
            type = types.BridgeType();
        }
        if (type == 0)
        {
            if (stream.U8() != 0)
            {
                stream.Fail("num == 0"); // 원본 assert
            }
            return new FortObject { Position = position, StoredType = storedType, Type = type, ShorthandOwner = shorthandOwner };
        }
        if (type == types.IslandType())
        {
            return new FortObject
            {
                Position = position,
                StoredType = storedType,
                Type = type,
                ShorthandOwner = shorthandOwner,
                IslandByte = stream.U8(),
            };
        }
        OriginalTypeRecord record = TypeOf(types, type, in stream);
        byte? frame = null;
        byte? quantityByte = null;
        short? quantityWord = null;
        byte? bridgeFrame = null;
        byte? legacyState = null;
        byte? factoryState = null;
        byte? owner = null;
        FortContentList? contents = null;
        if ((record.Flags1 & TypeFlag1.SaveFrame) != 0)
        {
            frame = stream.U8();
        }
        if ((record.Flags1 & TypeFlag1.SaveQA) != 0)
        {
            quantityByte = stream.U8();
        }
        if ((record.Flags1 & TypeFlag1.SaveQB) != 0)
        {
            quantityWord = unchecked((short)stream.U16());
        }
        if ((record.Flags2 & TypeFlag2.Bridge) != 0)
        {
            bridgeFrame = stream.U8();
        }
        // 버전 0: 원본 FUN_004ad0a0은 `or edx, 0x4000000` 뒤의 분기 때문에 항상 참이라 모든 타입에서 1바이트를 읽는다
        if (version == 0)
        {
            legacyState = stream.U8();
        }
        if (version > 1 && (record.Flags2 & TypeFlag2.Factory) != 0)
        {
            factoryState = stream.U8();
        }
        if (version > 0 && (record.Flags2 & TypeFlag2.OwnerSavedMask) != 0)
        {
            owner = stream.U8();
        }
        if ((record.Flags1 & TypeFlag1.Container) != 0)
        {
            contents = ReadContents(ref stream, fort, types, record.ContainerListFlags);
        }
        return new FortObject
        {
            Position = position,
            StoredType = storedType,
            Type = type,
            ShorthandOwner = shorthandOwner,
            Frame = frame,
            QuantityByte = quantityByte,
            QuantityWord = quantityWord,
            BridgeFrame = bridgeFrame,
            LegacyState = legacyState,
            FactoryState = factoryState,
            Owner = owner,
            Contents = contents,
        };
    }

    /// <summary>
    /// <c>Chaff</c>·<c>TerrNN</c>: 버전 바이트 뒤로 섹션이 끝날 때까지 청크 레코드를 읽는다(원본 004be160).
    /// 원본은 월드 청크를 y 바깥·x 안쪽으로 돌며 해당 청크마다 레코드 하나를 읽는다. 어느 청크인지는 영역 배치가 정한다.
    /// </summary>
    /// <param name="bytes">섹션 본문.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="fort">타입 번호 변환표를 가진 파일.</param>
    /// <param name="types">타입 표.</param>
    private static FortChunkSection ReadChunkSection(ReadOnlySpan<byte> bytes, string name, FortTemplate fort, RiftTypeTable types)
    {
        if (bytes.IsEmpty)
        {
            return new FortChunkSection();
        }
        SectionStream stream = new(bytes, name);
        byte version = stream.U8();
        bool present = stream.Remaining != 0;
        List<FortChunk> chunks = [];
        // 청크 레코드마다 표지와 오브젝트 수가 온다
        while (stream.Remaining != 0)
        {
            if (stream.U8() != ChunkMark)
            {
                stream.Fail("data.readByte() == 99"); // 원본 assert
            }
            List<FortObject> objects = [];
            // 빈 레코드(타입 0)도 개수에 들어 있으므로 그대로 보존한다
            for (ushort count = stream.U16(); count != 0; count--)
            {
                objects.Add(ReadObject(ref stream, fort, types, version));
            }
            chunks.Add(new FortChunk { Objects = objects });
        }
        return new FortChunkSection { Present = present, Version = version, Chunks = chunks };
    }

    /// <summary>현재 원시 <c>TypeNames</c>에서 편의 변환표만 다시 만들며 실제 manager/전역 변환표는 건드리지 않는다.</summary>
    /// <param name="fort">변환표를 채울 파일.</param>
    /// <param name="types">타입 표.</param>
    private static void ReadTypeConversion(FortTemplate fort, RiftTypeTable types)
    {
        // 이전 파일의 해시/변환이 빈 새 섹션으로 넘어가지 않게 한다
        fort.TypeHashes.Clear();
        fort.HasTypeConversion = false;
        Array.Clear(fort.TypeConversion);
        ReadOnlySpan<byte> section = fort.Section("TypeNames");
        if (section.IsEmpty)
        {
            return;
        }
        SectionStream stream = new(section, "TypeNames");
        // 개수 바이트 뒤로 해시가 이어진다
        for (byte count = stream.U8(); count != 0; count--)
        {
            fort.TypeHashes.Add(stream.U32());
        }
        fort.HasTypeConversion = true;
        IReadOnlyList<OriginalTypeRecord> current = types.Types;
        // 같은 해시의 현재 타입이 여럿이면 번호가 큰 것이 남는다(이름 없는 타입의 해시 0이 그렇다)
        for (int type = 0; type < current.Count; type++)
        {
            uint hash = RiftTypeTable.NameHash(current[type].Name);
            // 파일의 번호 가운데 해시가 같은 첫 번호만 고친다
            for (int stored = 0; stored < fort.TypeHashes.Count; stored++)
            {
                if (fort.TypeHashes[stored] != hash)
                {
                    continue;
                }
                if (stored < fort.TypeConversion.Length)
                {
                    fort.TypeConversion[stored] = (byte)type;
                }
                break;
            }
        }
    }

    /// <summary>객체 로더가 요구하는 원시 <c>Chaff</c>/<c>TerrNN</c> 섹션만 해석하며 다른 저장 섹션의 cursor와 할당을 보존한다.</summary>
    /// <param name="fort">해석 결과를 채울 파일.</param>
    /// <param name="types">타입 표.</param>
    private static void ReadObjectRecords(FortTemplate fort, RiftTypeTable types)
    {
        fort.Chaff = ReadChunkSection(fort.Section("Chaff"), "Chaff", fort, types);
        // 영역 섹션 20개
        for (int index = 0; index < TerritoryCount; index++)
        {
            string name = Names[SectionCount - TerritoryCount + index];
            fort.Territories[index] = ReadChunkSection(fort.Section(name), name, fort, types);
        }
    }

    /// <summary>실제 Editor의 파일/Subscriber/지형 준비 뒤 같은 현재 원시 섹션에서 객체 읽기 자료만 갱신한다. 파일 읽기/manager cursor는 바꾸지 않는다.</summary>
    /// <param name="types">타입 표.</param>
    public void RefreshObjectRecords(RiftTypeTable types)
    {
        ArgumentNullException.ThrowIfNull(types);
        ReadTypeConversion(this, types);
        ReadObjectRecords(this, types);
    }

    /// <summary>
    /// 파일 전체를 구조로 읽는다(cpppj <c>FortTemplate::Parse</c>). 타입 표는 그 파일을 읽을 판본의 것이어야 한다.
    /// 원본은 섹션을 이름 목록 순서로 35개 읽으며 파일이 먼저 끝나면 거기서 멈춘다(새 구현의 보호 처리).
    /// </summary>
    /// <param name="bytes">.fort 파일 전체 바이트.</param>
    /// <param name="types">타입 표.</param>
    /// <exception cref="InvalidDataException">형식 오류(첫 바이트, 섹션 경계, 청크 표지, 타입 표 밖 번호 등).</exception>
    public static FortTemplate Parse(ReadOnlySpan<byte> bytes, RiftTypeTable types)
    {
        ArgumentNullException.ThrowIfNull(types);
        FortTemplate fort = ReadSections(bytes);
        // Subscriber: ID 4바이트 + 문자열(길이 16비트 + 글자)
        ReadOnlySpan<byte> section = fort.Section("Subscriber");
        if (!section.IsEmpty)
        {
            SectionStream stream = new(section, "Subscriber");
            uint id = stream.U32();
            List<char> letters = [];
            // 이름은 길이만큼의 바이트다(문자 하나 = 바이트 하나)
            for (ushort length = stream.U16(); length != 0; length--)
            {
                letters.Add((char)stream.U8());
            }
            fort.Subscriber = new FortSubscriber { Id = id, Name = new string([.. letters]) };
        }
        // Money: 32비트 실수
        section = fort.Section("Money");
        if (section.Length >= 4)
        {
            SectionStream stream = new(section, "Money");
            fort.Money = BitConverter.UInt32BitsToSingle(stream.U32());
        }
        // Territory: 120바이트를 그대로 둔다. 해석은 영역 배치 단계에서 한다
        section = fort.Section("Territory");
        if (section.Length >= TerritoryBytes)
        {
            fort.Territory = section[..TerritoryBytes].ToArray();
        }
        ReadTypeConversion(fort, types);
        // Technology: 플레이어를 그릇으로 한 내용물 목록이다. 그 타입(PlayerSquid)의 +0xfc는 0이라 목록 플래그를 파일에서 읽는다
        section = fort.Section("Technology");
        if (!section.IsEmpty)
        {
            SectionStream stream = new(section, "Technology");
            fort.Technology = ReadContents(ref stream, fort, types, 0);
            fort.TechnologyTrailingBytes = stream.Remaining;
        }
        // Deck: 개수 + (타입, chance, power, 남은 횟수) 4바이트 항목
        section = fort.Section("Deck");
        if (!section.IsEmpty)
        {
            SectionStream stream = new(section, "Deck");
            // 변환한 타입이 0인 항목도 보존한다(원본은 덱에 넣지 않는다)
            for (byte count = stream.U8(); count != 0; count--)
            {
                byte storedType = stream.U8();
                fort.Deck.Add(new FortDeckEntry
                {
                    StoredType = storedType,
                    Type = fort.ConvertType(storedType),
                    Chance = stream.U8(),
                    Power = unchecked((sbyte)stream.U8()),
                    Remaining = stream.U8(),
                });
            }
        }
        ReadObjectRecords(fort, types);
        return fort;
    }
}
