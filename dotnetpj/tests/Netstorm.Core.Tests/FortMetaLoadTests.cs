using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Badges·Technology 로더(<see cref="RawFortMetaLoad"/>)를 원본 x86 실행 기대값으로 대조한다.
/// cpppj <c>FortMetaLoadTests</c>의 전수 재생 1개와 선택·진단·실패 직접 3개를 옮겼다.
/// 실제 Factory·Contained 사슬로 내용물을 만드는 검사는 <c>RawFortObjectLoad</c>가 필요한 F2b로 넘긴다. (F2a, 2026-11-02)
/// </summary>
public sealed class FortMetaLoadTests
{
    /// <summary>원본 입력 열의 고정 순서이며 기대값은 fixture에만 있다.</summary>
    private enum MetaField
    {
        /// <summary>진입점 종류(0 Badges·1 SetBadge·2 Technology·3 FindBadge).</summary>
        Kind,

        /// <summary>유형 DWORD.</summary>
        Type,

        /// <summary>수량·개수.</summary>
        Count,

        /// <summary>장부 입력 profile(중복·unsigned 마지막 타입).</summary>
        Profile,

        /// <summary>Template 있음 여부.</summary>
        Template,

        /// <summary>Template 칸 번호.</summary>
        Slot,

        /// <summary>섹션 있음 여부.</summary>
        Present,

        /// <summary>섹션 끝.</summary>
        End,

        /// <summary>시작 깊이.</summary>
        Depth,

        /// <summary>이전 conversion 있음 여부.</summary>
        OldConversion,

        /// <summary>준비 뒤 conversion 있음 여부.</summary>
        NewConversion,

        /// <summary>돌연변이 동작 번호.</summary>
        Mutation,
    }

    /// <summary>
    /// 패딩까지 명시한 28개 220BYTE 입력 표다.
    /// </summary>
    /// <param name="profile">중복·마지막 unsigned 타입을 쓰는 입력 profile.</param>
    private static byte[] BadgeInput(uint profile)
    {
        byte[] bytes = new byte[6160];
        Array.Fill(bytes, (byte)0xa5);
        // 첫 중복·마지막 unsigned 타입을 원본 입력과 같게 준비한다
        for (int index = 0; index < 28; index++)
        {
            uint type = (uint)(70 + index);
            if (profile != 0 && (index == 1 || index == 8))
            {
                type = 80;
            }
            if (profile != 0 && index == 27)
            {
                type = 0xffffffffu;
            }
            BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(index * 220), type);
        }
        return bytes;
    }

    /// <summary>
    /// 전체 raw 표를 원본 무손실 BYTE run 형식으로 직렬화한다.
    /// </summary>
    /// <param name="bytes">직렬화할 바이트다. 패딩이나 상위 수량 BYTE도 하나도 생략하지 않는다.</param>
    private static string BadgeRle(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        int start = 0;
        // 같은 BYTE의 최대 연속 구간을 찾는다
        while (start < bytes.Length)
        {
            int end = start + 1;
            while (end < bytes.Length && bytes[end] == bytes[start])
            {
                end++;
            }
            if (result.Length != 0)
            {
                result.Append(',');
            }
            result.Append(end - start);
            result.Append(':');
            result.Append(digits[bytes[start] >> 4]);
            result.Append(digits[bytes[start] & 15]);
            start = end;
        }
        return result.ToString();
    }

    /// <summary>한 입력의 명시 하위 경계와 현재 장부·깊이를 갖는 장면이다.</summary>
    private sealed class FortMetaReplayScene
    {
        /// <summary>판본과 실제 하위 소비자 풀을 공유한다.</summary>
        public RawSidPool Pool { get; }

        /// <summary>원본 입력의 12개 DWORD다.</summary>
        public List<uint> Input { get; } = [];

        /// <summary>전체 raw 장부와 실제 stream 메모리다.</summary>
        public byte[] Badges;

        /// <summary>실제 stream 메모리다.</summary>
        public List<byte> Data { get; } = [];

        /// <summary>원본 현재 로딩 깊이며 callback 뒤 재조회한다.</summary>
        public uint Depth;

        /// <summary>Template·isMine·현재 conversion의 명시 입력이다.</summary>
        public FortMetaLoadState State { get; } = new();

        /// <summary>pointer 존재·pos·end를 bytes와 구별한다.</summary>
        public FortMetaStream Stream { get; } = new();

        /// <summary>명시 하위 경계의 원본 순서 기록이다.</summary>
        public List<string> Events { get; } = [];

        /// <summary>보고 후 정상 반환한 진단 횟수다.</summary>
        public int Assertions;

        /// <summary>
        /// 기대 장부·분기를 계산하지 않고 원본 입력만 초기화한다.
        /// </summary>
        /// <param name="pool">공유할 SID 풀이다.</param>
        /// <param name="fields">원본 입력 12개 DWORD 문자열이다.</param>
        public FortMetaReplayScene(RawSidPool pool, string fields)
        {
            Pool = pool;
            // 각 원본 DWORD를 순서대로 전달한다
            foreach (string field in fields.Split(','))
            {
                Input.Add(uint.Parse(field, CultureInfo.InvariantCulture));
            }
            Assert.Equal(12, Input.Count);
            Badges = BadgeInput(Input[(int)MetaField.Profile]);
            Depth = Input[(int)MetaField.Depth];
            State.TemplatePresent = Input[(int)MetaField.Template] != 0;
            State.TemplateSlot = unchecked((int)Input[(int)MetaField.Slot]);
            State.LocalOwner = 1;
            State.HasTypeConversion = Input[(int)MetaField.OldConversion] != 0;
            uint count = Input[(int)MetaField.Kind] == 0 ? Input[(int)MetaField.Count] : 3u;
            Data.Add((byte)count);
            // 메모리의 타입·수량 BYTE만 공급하며 setter 결과는 계산하지 않는다
            for (uint index = 0; index < count; index++)
            {
                Data.Add((byte)(index % 7 == 6 ? 255 : 70 + (int)(index % 28)));
                Data.Add((byte)(index * 13 + 1));
            }
            Stream.Set(Data.ToArray(), 19, Input[(int)MetaField.End], Input[(int)MetaField.Present] != 0);
        }

        /// <summary>
        /// TypeNames·하위 Contents 실행 완료를 주장하지 않는 명시 관찰 경계다.
        /// </summary>
        /// <param name="dropPrepareTypeNames">참이면 변환 준비 효과를 빼고 필수 효과 누락을 검사한다.</param>
        public FortMetaLoadHooks Hooks(bool dropPrepareTypeNames = false)
        {
            FortMetaLoadHooks hooks = new()
            {
                Assertion = reason =>
                {
                    // 원본 진단 이름의 fixture 표기다
                    char[] names = ['B', 'C', 'T', 'M'];
                    Events.Add("A:" + names[(int)reason]);
                    Assertions++;
                    if (Input[(int)MetaField.Mutation] == 3)
                    {
                        BinaryPrimitives.WriteUInt32LittleEndian(Badges.AsSpan(0, 4), 999);
                    }
                },
                LoadingDepthChanged = value =>
                {
                    Assert.Equal(Depth, value);
                    Events.Add("L:" + value.ToString(CultureInfo.InvariantCulture));
                },
                TechnologyParent = () =>
                {
                    Events.Add("P:50");
                    return new Sid(50);
                },
                PrepareTypeNames = dropPrepareTypeNames
                    ? null
                    : () =>
                    {
                        Events.Add("N:" + (State.HasTypeConversion ? "1" : "0"));
                        State.HasTypeConversion = Input[(int)MetaField.NewConversion] != 0;
                    },
                Contents = (parent, current, flags, relaxedConversion) =>
                {
                    Assert.Equal(new Sid(50), parent);
                    Assert.Equal(17u, flags);
                    Assert.True(relaxedConversion);
                    Events.Add("C:50:17:1:" + Depth.ToString(CultureInfo.InvariantCulture) + ":" + current.Position.ToString(CultureInfo.InvariantCulture));
                    current.Position = 5;
                    if (Input[(int)MetaField.Mutation] == 2)
                    {
                        State.HasTypeConversion = true;
                    }
                },
                ReleaseTypeConversion = () =>
                {
                    Assert.True(State.HasTypeConversion);
                    Events.Add("F");
                },
                RefreshTechnologyUi = () =>
                {
                    Events.Add("U");
                    if (Input[(int)MetaField.Mutation] == 1)
                    {
                        Depth += 5u;
                    }
                },
            };
            return hooks;
        }

        /// <summary>
        /// 이벤트 기록만 연결하며 발생할 이벤트를 미리 계산하지 않는다.
        /// </summary>
        public string EventsText()
        {
            // 모든 관찰을 순서대로 저장한다
            return Events.Count == 0 ? "-" : string.Join(";", Events);
        }
    }

    /// <summary>
    /// 두 실제 PE의 고유 정상·assert 입력과 전체 장부·pos·end·depth·변환·효과·진단을 전수 대조한다.
    /// </summary>
    [Fact]
    public void ReplaysBothNativeFullBadgeAndWrapper()
    {
        string[][] rows = X86Fixture.Read("fortmeta");
        int count = 0;
        int normal = 0;
        int diagnosed = 0;
        // 판본별 풀은 장기 객체이며 각 입력의 raw 표와 stream만 새로 구성한다
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RawSidPool pool = new(24000, false, null, edition);
            string label = edition == OriginalEdition.Patch1078 ? "originals" : "originalCD";
            // 고유 행의 전체 장부·pos·end·depth·변환·효과·진단을 출력과 대조한다
            foreach (string[] row in rows)
            {
                if (row[0] != label)
                {
                    continue;
                }
                Assert.Equal(10, row.Length);
                FortMetaReplayScene scene = new(pool, row[1]);
                RawFortMetaLoad loader = new(pool, scene.Badges, () => scene.Depth, value => scene.Depth = value, scene.State, scene.Hooks());
                string result = "-";
                if (scene.Input[(int)MetaField.Kind] == 0)
                {
                    loader.LoadBadges(scene.Stream);
                }
                else if (scene.Input[(int)MetaField.Kind] == 1)
                {
                    loader.SetBadge(scene.Input[(int)MetaField.Type], unchecked((int)scene.Input[(int)MetaField.Count]));
                }
                else if (scene.Input[(int)MetaField.Kind] == 2)
                {
                    loader.LoadTechnology(scene.Stream);
                }
                else
                {
                    result = unchecked((uint)loader.FindBadge(scene.Input[(int)MetaField.Type])).ToString(CultureInfo.InvariantCulture);
                }
                bool same = result == row[2]
                    && scene.Stream.Position == uint.Parse(row[3], CultureInfo.InvariantCulture)
                    && scene.Stream.End == uint.Parse(row[4], CultureInfo.InvariantCulture)
                    && scene.Depth == uint.Parse(row[5], CultureInfo.InvariantCulture)
                    && scene.State.HasTypeConversion == (row[6] != "0")
                    && scene.EventsText() == row[7]
                    && BadgeRle(scene.Badges) == row[8]
                    && scene.Assertions == int.Parse(row[9], CultureInfo.InvariantCulture);
                Assert.True(same);
                Assert.Same(pool, loader.Pool());
                count++;
                if (row[9] == "0")
                {
                    normal++;
                }
                else
                {
                    diagnosed++;
                }
            }
        }
        Assert.Equal(rows.Length, count);
        Assert.True(normal > 0 && diagnosed > 0);
    }

    /// <summary>
    /// 부족한 장부·필수 효과·손상 물리 메모리를 기본 성공값으로 대체하지 않는다.
    /// cpppj <c>FortMetaLoad_RejectsMissingRecordsEffectsAndTruncatedMemory</c>를 옮겼다.
    /// </summary>
    [Fact]
    public void RejectsMissingRecordsEffectsAndTruncatedMemory()
    {
        RawSidPool pool = new(24000, false, null, OriginalEdition.Patch1078);
        FortMetaReplayScene scene = new(pool, "0,80,1,0,1,1,1,6,0,0,1,0");
        Assert.ThrowsAny<Exception>(() => new RawFortMetaLoad(pool, scene.Badges, () => scene.Depth, value => scene.Depth = value, scene.State, scene.Hooks(dropPrepareTypeNames: true)));
        Assert.ThrowsAny<Exception>(() => new RawFortMetaLoad(pool, scene.Badges.AsSpan(0, 6159).ToArray(), () => scene.Depth, value => scene.Depth = value, scene.State, scene.Hooks()));
        RawFortMetaLoad loader = new(pool, scene.Badges, () => scene.Depth, value => scene.Depth = value, scene.State, scene.Hooks());
        scene.Stream.Bytes = Array.Empty<byte>();
        Assert.ThrowsAny<Exception>(() => loader.LoadBadges(scene.Stream));
    }

    /// <summary>
    /// 두 판본의 깊이·Template 진단과 기술 부모가 현재 섹션을 바꾼 뒤에만 실제 getter를 실행한다.
    /// cpppj <c>FortMetaLoad_SelectsAfterDiagnosticsAndTechnologyParent</c>를 옮겼다.
    /// </summary>
    [Fact]
    public void SelectsAfterDiagnosticsAndTechnologyParent()
    {
        // 두 판본은 Badges의 즉시 깊이 증감만 다르며 Technology 선택 시점은 같다
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RawSidPool pool = new(24000, false, null, edition);
            byte[] badges = BadgeInput(0);
            uint depth = 3;
            FortMetaLoadState state = new() { TemplatePresent = false, TemplateSlot = 8, LocalOwner = 1 };
            byte[] firstBytes = [0];
            FortMetaStream first = new();
            first.Set(firstBytes, 11, 1, true);
            FortMetaStream second = new();
            second.Set(firstBytes, 19, 1, true);
            FortMetaStream? current = first;
            List<string> effects = [];
            FortMetaLoadHooks hooks = new()
            {
                Assertion = reason =>
                {
                    effects.Add("A" + ((int)reason).ToString(CultureInfo.InvariantCulture));
                    if (reason == FortMetaAssertion.MissingBadgesTemplate)
                    {
                        current = second;
                    }
                },
                LoadingDepthChanged = value =>
                {
                    Assert.Equal(depth, value);
                    effects.Add("L" + value.ToString(CultureInfo.InvariantCulture));
                },
                TechnologyParent = () =>
                {
                    Assert.Equal(4u, depth);
                    effects.Add("P");
                    current = second;
                    return new Sid(50);
                },
                PrepareTypeNames = () =>
                {
                    effects.Add("N");
                    state.HasTypeConversion = true;
                },
                Contents = (parent, stream, flags, relaxed) =>
                {
                    Assert.Equal(new Sid(50), parent);
                    Assert.Same(second, stream);
                    Assert.Equal(17u, flags);
                    Assert.True(relaxed && stream.Position == 0 && depth == 4);
                    effects.Add("C");
                    stream.Position = 1;
                },
                ReleaseTypeConversion = () => effects.Add("F"),
                RefreshTechnologyUi = () => effects.Add("U"),
            };
            RawFortMetaLoad meta = new(pool, badges, () => depth, value => depth = value, state, hooks);
            meta.LoadBadgesSelected(() =>
            {
                Assert.Equal(3u, depth);
                Assert.Same(second, current);
                effects.Add("S3");
                current!.Position = 0;
                return current;
            });
            List<string> badgeEffects = edition == OriginalEdition.Patch1078
                ? ["A0", "S3"]
                : ["L4", "L3", "A0", "S3"];
            Assert.Equal(badgeEffects, effects);
            Assert.Equal(11u, first.Position);
            Assert.Equal(1u, second.Position);
            Assert.Equal(3u, depth);
            effects.Clear();
            current = first;
            meta.LoadTechnologySelected(() =>
            {
                Assert.Equal(4u, depth);
                Assert.Same(second, current);
                Assert.Equal("P", effects[^1]);
                effects.Add("S4");
                current!.Position = 0;
                return current;
            });
            Assert.Equal(["L4", "A2", "A3", "P", "S4", "N", "C", "F", "U", "L3"], effects);
            Assert.Equal(11u, first.Position);
            Assert.Equal(1u, second.Position);
            Assert.False(state.HasTypeConversion);
            Assert.Equal(3u, depth);
        }
    }

    /// <summary>
    /// 기술 selector 실패도 이미 수행한 부모 조회를 유지하면서 원래 깊이를 복구하고 빠진 callback은 쓰기 전에 거부한다.
    /// cpppj <c>FortMetaLoad_SelectionFailureRestoresDepth</c>를 옮겼다.
    /// </summary>
    [Fact]
    public void SelectionFailureRestoresDepth()
    {
        RawSidPool pool = new(24000, false, null, OriginalEdition.Patch1078);
        byte[] badges = BadgeInput(0);
        uint depth = 3;
        int parents = 0;
        FortMetaLoadState state = new();
        FortMetaLoadHooks hooks = new()
        {
            Assertion = _ => throw new InvalidOperationException("정상 Template의 예상 밖 진단"),
            LoadingDepthChanged = value => Assert.Equal(depth, value),
            TechnologyParent = () =>
            {
                Assert.Equal(4u, depth);
                parents++;
                return default;
            },
            PrepareTypeNames = () => throw new InvalidOperationException("선택 실패 뒤 추가 처리 없음"),
            ReleaseTypeConversion = () => throw new InvalidOperationException("선택 실패 뒤 추가 처리 없음"),
            RefreshTechnologyUi = () => throw new InvalidOperationException("선택 실패 뒤 추가 처리 없음"),
            Contents = (_, _, _, _) => throw new InvalidOperationException("선택 실패 뒤 내용물 없음"),
        };
        RawFortMetaLoad meta = new(pool, badges, () => depth, value => depth = value, state, hooks);
        Assert.ThrowsAny<Exception>(() => meta.LoadTechnologySelected(() =>
        {
            Assert.Equal(4u, depth);
            Assert.Equal(1, parents);
            throw new InvalidOperationException("기술 파일 선택 실패 입력");
        }));
        Assert.Equal(3u, depth);
        Assert.Equal(1, parents);
        Assert.ThrowsAny<Exception>(() => meta.LoadTechnologySelected(null!));
        Assert.ThrowsAny<Exception>(() => meta.LoadBadgesSelected(null!));
        Assert.Equal(3u, depth);
        Assert.Equal(1, parents);
    }

    /// <summary>선택 Technology Contents만 실제 Factory·ContainedCreate·ContentForm으로 같은 풀에 재생한다.</summary>
    [Fact]
    public void ActualSamePoolContentsAndMandatoryBoundaries()
    {
        // 부모 getter·변환표 준비·실제 UI 설명 경계이며 HP·프레임·Pop·Owner는 이 분기에서 요청하지 않는다
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RawSidPool pool = new(24000, false, null, edition);
            RawSquidHash hash = new();
            List<OriginalTypeRecord> types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToList();
            // C# 생성·내용물 모듈은 10.78 표 188개 가드를 두므로 CD 장면에서는 뒤에 빈 기록을 채운 복사본을 쓴다
            List<OriginalTypeRecord> moduleTypes = edition == OriginalEdition.Patch1078 ? types : [.. types, .. Enumerable.Range(0, 17).Select(_ => new OriginalTypeRecord())];
            List<RiftTypeFrames> frames = [];
            for (int index = 0; index < types.Count; index++)
            {
                frames.Add(new RiftTypeFrames([]));
            }
            types[89].ConstructorAddress = 0;
            // C# 팩토리는 재구성된 생성자만 지원하므로(원본 89번 생성자는 미복원) 기본 Squid로 만든다.
            // 부모 슬롯의 가상 표는 이 검사에서 대조하지 않으며 연결·소유자·내용물은 동일하게 대조된다.
            types[89].FootX = types[89].FootY = 1;
            types[110].Flags1 = 0x1000;
            types[110].ContentListFlags = 0x10;
            types[111].Flags1 = 0x2000;
            types[111].ContentListFlags = 0x20;
            RawSquidFactory factory = new(pool, moduleTypes);
            int parent = factory.Create(89, (SidAllocation)2);
            List<int> children = [];
            int transmits = 0;
            RawContentFormHost forms = new(pool, moduleTypes,
                (owner, form, reason) =>
                {
                    Assert.Equal(parent, owner);
                    Assert.Equal(1u, reason);
                    children.Add(form);
                },
                (sid, flags) => transmits++);
            ContainedCreateState containedState = new();
            RawContainedCreate contained = new(pool, containedState, forms.BindCreate(factory, new ContainedCreateHooks
            {
                NotifySurface = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            }));
            GameRandom random = new(12345);
            FortObjectLoadState objectState = new();
            RawFortObjectLoad objects = new(pool, hash, types, frames, random, objectState, ContentsOnly(contained));
            FortTemplate fort = new();
            for (int index = 0; index < FortTemplate.SectionCount; index++)
            {
                fort.Sections.Add(new RawSectionBuffer());
            }
            int technologyIndex = FortTemplate.SectionNames.ToList().IndexOf("Technology");
            byte[] techBytes = [2, 110, 9, 111, 254, 255, 0xee];
            fort.Sections[technologyIndex] = RawSectionBuffer.FromBytes(techBytes);
            fort.TechnologyTrailingBytes = 1;
            FortContent first = new() { StoredType = 110, Type = 110, Quantity = 9 };
            FortContent second = new() { StoredType = 111, Type = 111, Quantity = -2 };
            fort.Technology = new FortContentList { Items = [first, second] };
            uint depth = 3;
            FortMetaLoadState state = new() { HasTypeConversion = true };
            List<string> effects = [];
            FortMetaLoadHooks hooks = new()
            {
                Assertion = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
                LoadingDepthChanged = value =>
                {
                    Assert.Equal(depth, value);
                    objectState.LoadingDepth = value;
                },
                TechnologyParent = () =>
                {
                    effects.Add("parent");
                    return new Sid((ushort)parent);
                },
                PrepareTypeNames = () =>
                {
                    effects.Add("prepare-boundary");
                    Assert.True(state.HasTypeConversion);
                },
                ReleaseTypeConversion = () =>
                {
                    effects.Add("release-boundary");
                    Assert.True(state.HasTypeConversion);
                },
                RefreshTechnologyUi = () => effects.Add("ui-boundary"),
            };
            hooks = RawFortMetaLoad.MakeFortMetaContentsHooks(pool, objects, fort, hooks);
            byte[] badges = BadgeInput(0);
            RawFortMetaLoad meta = new(pool, badges, () => depth, value => depth = value, state, hooks);
            FortMetaStream stream = new() { Bytes = fort.Sections[technologyIndex].Memory, Position = 19, End = 7, Present = true };
            meta.LoadTechnology(stream);
            Assert.Equal(2, children.Count);
            Assert.Equal(0, transmits);
            Assert.Equal(6u, stream.Position);
            Assert.Equal(7u, stream.End);
            Assert.Equal(3u, depth);
            Assert.Equal(3u, objectState.LoadingDepth);
            Assert.False(state.HasTypeConversion);
            Assert.Equal(new[] { "parent", "prepare-boundary", "release-boundary", "ui-boundary" }, effects);
            RawSquidForms formsView = new(pool);
            Assert.Equal(parent, formsView.Parent(children[0]));
            Assert.Equal(parent, formsView.Parent(children[1]));
            Assert.Equal(110u, ReadU16(pool.Slot(children[0]), 18));
            Assert.Equal(111u, ReadU16(pool.Slot(children[1]), 18));
            Assert.Equal(9u, ReadU16(pool.Slot(children[0]), 12));
            Assert.Equal(65534u, ReadU16(pool.Slot(children[1]), 12));
            Assert.Equal(12345u, random.State());
            Assert.Equal(children[0], (int)ReadU16(pool.Slot(parent), 6));
            Assert.Equal(children[1], (int)ReadU16(pool.Slot(children[0]), 4));
            RawSidPool foreign = new(24000, false, null, edition);
            Assert.Throws<ArgumentException>(() => RawFortMetaLoad.MakeFortMetaContentsHooks(foreign, objects, fort, hooks));
            FortMetaStream wrong = new() { Bytes = ReadOnlyMemory<byte>.Empty, Position = 19, End = 7, Present = true };
            Assert.Throws<ArgumentException>(() => hooks.Contents!(new Sid((ushort)parent), wrong, 17, true));
            FortMetaStream alien = new() { Bytes = new ReadOnlyMemory<byte>((byte[])techBytes.Clone()), Position = 19, End = 7, Present = true };
            Assert.Throws<ArgumentException>(() => hooks.Contents!(new Sid((ushort)parent), alien, 17, true));
        }
    }

    /// <summary>실제 Contents만 실행하는 경로의 명시 효과만 둔다.</summary>
    /// <param name="contained">같은 풀의 실제 내용물 생성 몸체다.</param>
    private static FortObjectLoadHooks ContentsOnly(RawContainedCreate contained)
    {
        return new FortObjectLoadHooks
        {
            Create = (type, flags) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            NotifySurface = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            SetState = (_, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            SetOwner = (_, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            HasHitPoints = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            MaxHitPoints = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            VirtualCount = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            Pop = (_, _, _, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            Destroy = (_, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            CreateContent = (parent, type, quantity, listFlags, flags, quality) => contained.Create(parent, type, quantity, listFlags, flags, quality),
            ChunkRegion = (_, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            TerritoryAllowed = (_, _, _) => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            LoadingDepthChanged = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
            FinishOwner = _ => throw new InvalidOperationException("선택 Technology 대비 의미복원 경계 호출"),
        };
    }

    /// <summary>비정렬 WORD를 little endian으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static uint ReadU16(ReadOnlySpan<byte> bytes, int offset)
    {
        return (uint)bytes[offset] | ((uint)bytes[offset + 1] << 8);
    }
}
