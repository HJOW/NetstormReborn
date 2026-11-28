using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

// 원문 저장 가상 분배와 실제 Template stream의 재귀/부분 쓰기를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>저장 결과·NativeLoaded를 주입하지 않고 명시 raw 입력과 실제 장기 Surface/Template를 사용한다.</summary>
public sealed class NativeFortSaveTests
{
    /// <summary>선택되지 않은 시각/RNG/World 생성이 없는 실제 물리 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 현재 Client의 권한·내용물 TYPE·checking owner다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>조회/검색/저장에 공유하는 실제 표면이다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>실제 생성 수명과 부분 쓰기를 보관하는 장기 템플릿이다.</summary>
        public readonly CampaignTemplateStorage Templates = new();
        /// <summary>현재 가상표로 저장 가능 여부를 읽는 검사 대상이다.</summary>
        public readonly NativeFortSaveVirtual Virtuals;
        /// <summary>선택된 진단의 명시 입력을 받고 기본값은 예상 밖 진단을 거부한다.</summary>
        public Action<string, string, uint> Assertion = (text, file, line) => throw new IOException($"{text}:{file}:{line:x}");

        /// <summary>현재188 TYPE/프레임과 실제 할당 슬롯을 준비한다. 저장 본체가 쓰는 값은 되돌리지 않는다.</summary>
        public Scene(bool frames = true)
        {
            var local = new ClientLocalStateGlobals(Game, new());
            Surface = new(Game, new(local), new(), () => throw new IOException("저장 조회 중 시각 접근 금지"));
            Surface.BindDisplayDiagnostics((text, file, line) => Assertion(text, file, line));
            Surface.Storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
            if (frames) Surface.Storage.Frames.AddRange(Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])));
            // 실제 할당 뒤 생성 입력만 작성하며 free/dead 필터를 저장기에 추가하지 않는다.
            for (int sid = 5; sid <= 10; sid++)
            {
                Assert.Equal(sid, Surface.Storage.Pool.Allocate(SidAllocation.Client));
                Raw(sid).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(sid), 0x501dd0);
                Raw(sid)[10] = 6;
            }
            Templates.Install(1, EmptyFort(), "contents.fort");
            Virtuals = new(Surface);
        }

        /// <summary>현재 실제 할당의50BYTE를 직접 빌리는 입력 도우미다.</summary>
        public Span<byte> Raw(int sid) => Surface.Storage.Pool.AllocatedBytes(sid);
        /// <summary>현재 타입/종류/QA/목록/다음 링크의 입력을 같은 슬롯에 쓴다.</summary>
        public void Content(int sid, ushort kind, ushort qa, byte flags, ushort next = 0)
        {
            Raw(sid)[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[18..], kind);
            BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[12..], qa); Raw(sid)[20] = flags;
            BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[4..], next);
        }
        /// <summary>부모의 종속 머리를 현재 입력으로 지정한다.</summary>
        public void Head(int parent, ushort first) => BinaryPrimitives.WriteUInt16LittleEndian(Raw(parent)[6..], first);
        /// <summary>실제 반환 stream으로 전체 내용물 쓰기를 수행한다.</summary>
        public void Save(uint parent, TemplateWriteStream stream) => NativeFortContentSave.Save(Surface, Templates, parent, stream);
    }

    /// <summary>83개 실제 생성 VT를 현재 인스턴스로 분배하며 거부9종은 잘못된 TYPE도 읽지 않는다.</summary>
    [Fact]
    public void ObjectRobustDispatchesAllConstructedTablesBeforeSelectingType()
    {
        var scene = new Scene(); Assert.Same(scene.Surface, scene.Virtuals.Surface);
        uint[] tables = OriginalConstructors.Recipes.Select(recipe => recipe.Vtable)
            .Append(OriginalConstructors.BaseVtableFor(OriginalEdition.Patch1078)).Distinct().ToArray();
        Assert.Equal(83, tables.Length); int rejected = 0, balloons = 0;
        // 현재 VT를 TYPE 생성자와 억지로 일치시키지 않고 각 실제 분기를 선택한다.
        foreach (uint table in tables)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(scene.Raw(5), table); scene.Raw(5)[10] = 255;
            if (table is 0x5070c0 or 0x508cc8 or 0x5092d0 or 0x5096a0 or 0x509758 or 0x509810 or 0x50a8e8 or 0x50db78 or 0x50ee08)
            {
                Assert.Equal(0u, scene.Virtuals.ObjectRobust(5)); rejected++; continue;
            }
            Assert.Throws<ArgumentOutOfRangeException>(() => scene.Virtuals.ObjectRobust(5));
            scene.Raw(5)[10] = 165; scene.Surface.Storage.Types[165].Flags1 = 0x20000000;
            if (table is 0x5040c0 or 0x504198)
            {
                BinaryPrimitives.WriteInt32LittleEndian(scene.Raw(5)[36..], -1);
                Assert.Equal(1u, scene.Virtuals.ObjectRobust(5)); balloons++;
            }
            else
            {
                Assert.Equal(0u, scene.Virtuals.ObjectRobust(5));
                scene.Surface.Storage.Types[165].Flags1 = 0x1fffffff;
                Assert.Equal(1u, scene.Virtuals.ObjectRobust(5));
            }
        }
        Assert.Equal(9, rejected); Assert.Equal(2, balloons);
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Raw(5), 0x12345678); scene.Raw(5)[10] = 255;
        Assert.Throws<InvalidOperationException>(() => scene.Virtuals.ObjectRobust(5));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Virtuals.ObjectRobust(0x10005));
    }

    /// <summary>벌룬의 signed frame 경계는 현재 TYPE의 실제 코드 개수와 비교하고 generic flags를 사용하지 않는다.</summary>
    [Theory]
    [InlineData(0x5040c0u)]
    [InlineData(0x504198u)]
    public void BalloonRobustReadsCurrentSignedFrameAndCurrentCodes(uint table)
    {
        var scene = new Scene(); BinaryPrimitives.WriteUInt32LittleEndian(scene.Raw(5), table);
        scene.Surface.Storage.Types[6].Flags1 = uint.MaxValue;
        scene.Surface.Storage.Frames[6] = new([new((byte)'A', (byte)'A', 0, 0), new((byte)'B', (byte)'A', 0, 0)]);
        // 마지막 유효 프레임·끝·음수의 signed 비교를 그대로 검사한다.
        foreach ((int frame, uint expected) in new[] { (int.MinValue, 1u), (-1, 1u), (0, 1u), (1, 1u), (2, 0u), (int.MaxValue, 0u) })
        {
            BinaryPrimitives.WriteInt32LittleEndian(scene.Raw(5)[36..], frame);
            Assert.Equal(expected, scene.Virtuals.ObjectRobust(5));
        }
        var missing = new Scene(frames: false); BinaryPrimitives.WriteUInt32LittleEndian(missing.Raw(5), table);
        Assert.Throws<ArgumentOutOfRangeException>(() => missing.Virtuals.ObjectRobust(5));
        BinaryPrimitives.WriteUInt32LittleEndian(missing.Raw(5), 0x501dd0);
        Assert.Equal(1u, missing.Virtuals.ObjectRobust(5));
    }

    /// <summary>목록 하위BYTE의256개 입력을 검사하고 world 진단의 정상 복귀 뒤 현재 flags를 재조회한다.</summary>
    [Fact]
    public void ContentRobustUsesCurrentLowByteAndRereadsAfterReturningAssertion()
    {
        var scene = new Scene(); int diagnostics = 0;
        scene.Assertion = (text, file, line) =>
        {
            Assert.Equal("!(getListFlags() & CFF_WORLD)", text); Assert.Equal(@"\Ns\O\Squid.cpp", file);
            Assert.Equal(0xbb1u, line); diagnostics++;
        };
        scene.Raw(5)[21] = 255; scene.Raw(5)[10] = 255;
        // 현재 TYPE·가상표를 읽지 않는 내용물 판정이며 상위 flags BYTE도 무시한다.
        for (int flags = 0; flags <= 255; flags++)
        {
            scene.Raw(5)[20] = (byte)flags;
            Assert.Equal((flags & 0x34) != 0 ? 1u : 0u, scene.Virtuals.ContentRobust(5));
        }
        Assert.Equal(128, diagnostics);
        scene.Assertion = (_, _, _) => scene.Raw(5)[20] = 0x10;
        scene.Raw(5)[20] = 0x80; Assert.Equal(1u, scene.Virtuals.ContentRobust(5));
        scene.Assertion = (_, _, _) => { scene.Raw(5)[20] = 0; throw new IOException("원문 진단 실패"); };
        scene.Raw(5)[20] = 0xb4; Assert.Throws<IOException>(() => scene.Virtuals.ContentRobust(5));
        Assert.Equal(0, scene.Raw(5)[20]);
    }

    /// <summary>hasLevel의 기존 OR 조건과 이미 연결된 base/Dais Level을 사용한다.</summary>
    [Fact]
    public void LevelUsesExistingCurrentVirtualBodyAndOriginalHasLevelCondition()
    {
        var scene = new Scene(); Span<byte> raw = scene.Raw(5); raw[11] = 0xe3;
        Assert.Equal(1u, scene.Virtuals.HasLevel(5)); Assert.Equal(7u, scene.Virtuals.Level(5));
        raw[11] = 0xe2;
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Raw(5), 0x507950); scene.Raw(5)[10] = 165;
        scene.Surface.Storage.Frames[165] = new(Enumerable.Repeat(new RiftFrameCode((byte)'M', (byte)'A', 0, 0), 180));
        Assert.Equal(2u, scene.Virtuals.Level(5));
        scene.Raw(5)[10] = 255; Assert.Throws<ArgumentOutOfRangeException>(() => scene.Virtuals.HasLevel(5));
    }

    /// <summary>실제 stream의 앞/뒤 canary와 별도 current를 보존하며 QA 두 폭·중첩·거부 항목을 깊이 우선 저장한다.</summary>
    [Fact]
    public void RecursiveContentsUseActualReturnedStreamAndIndependentNestedFinders()
    {
        var scene = new Scene(); scene.Head(5, 6); scene.Head(6, 8);
        scene.Content(6, 70, 0x3412, 0x10, 7); scene.Content(7, 71, 0x7856, 0x20);
        scene.Content(8, 72, 0xabcd, 0x04, 9); scene.Content(9, 73, 0xffff, 0);
        scene.Surface.Storage.Types[70].Flags1 = 0x21000; scene.Surface.Storage.Types[71].Flags1 = 0x2000;
        TemplateWriteStream stream = scene.Templates.CreateSection(1, "Technology");
        scene.Templates.WriteSection(stream, Enumerable.Repeat((byte)0xa5, 32).ToArray());
        TemplateWriteStream other = scene.Templates.CreateSection(1, "Temp"); scene.Templates.WriteSection(other, [0x99]);
        // 새 노드 생성의 원문 tail 압축이 pos를 end로 옮긴 뒤 같은 기존 stream만 되감는다.
        scene.Templates.AccessSection(stream, (fields, _) => BinaryPrimitives.WriteUInt32LittleEndian(fields[4..], 3));
        byte[][] before = Enumerable.Range(5, 6).Select(sid => scene.Surface.Storage.Pool.Slot(sid).ToArray()).ToArray();
        scene.Save(5, stream);
        byte[] expected = Enumerable.Repeat((byte)0xa5, 32).ToArray();
        new byte[] { 2, 70, 0x12, 0x10, 1, 72, 0x04, 71, 0x56, 0x78, 0x20 }.CopyTo(expected, 3);
        Assert.Equal(expected, scene.Templates.SectionBuffer(stream).ToArray());
        Assert.Equal(14u, Observe(scene.Templates, stream).Position); Assert.Equal("Temp", scene.Templates.CurrentSection(1));
        Assert.Equal(new byte[] { 0x99 }, scene.Templates.SectionBuffer(other).ToArray());
        // 재귀 저장이 실제 슬롯/부모/종속 링크·QA·상태를 변경하지 않았는지 전수 대조한다.
        for (int index = 0; index < before.Length; index++) Assert.Equal(before[index], scene.Surface.Storage.Pool.Slot(index + 5).ToArray());
    }

    /// <summary>dead 부모 진단 뒤 바뀐 head를 읽고 Content 진단 뒤 저장된 next·현재 TYPE/QA를 사용한다.</summary>
    [Fact]
    public void ContentsRereadCurrentHeadTypeAndKindAfterCallbacksButPreserveSavedNext()
    {
        var scene = new Scene(); scene.Game.AiDebug.Checking.Value = 256; scene.Game.Authority.Boss = false;
        scene.Raw(5)[11] = 2; scene.Head(5, 10);
        scene.Content(6, 70, 1, 0x80, 7); scene.Content(7, 71, 2, 0x10, 8); scene.Content(8, 72, 3, 0x10);
        scene.Surface.Storage.Types[71].Flags1 = 0x1000;
        var calls = new List<uint>(); scene.Assertion = (_, _, line) =>
        {
            calls.Add(line);
            if (line == 0x13c) scene.Head(5, 6);
            else
            {
                Assert.Equal(0xbb1u, line); scene.Content(6, 71, 0x3456, 0x10, 10);
                scene.Surface.Storage.ContainedType.Value = 9; scene.Raw(7)[10] = 9;
            }
        };
        TemplateWriteStream stream = scene.Templates.CreateSection(1, "Technology"); scene.Save(5, stream);
        Assert.Equal(new uint[] { 0x13c, 0xbb1 }, calls);
        Assert.Equal(new byte[] { 2, 71, 0x56, 0x10, 71, 2, 0x10 }, scene.Templates.SectionBuffer(stream).ToArray());
        Assert.Equal(10, BinaryPrimitives.ReadUInt16LittleEndian(scene.Raw(6)[4..]));
    }

    /// <summary>목록 진단 실패는 count/kind/QA까지 남기고 다음 저장은 기존 stream의 현재 cursor에서 계속한다.</summary>
    [Fact]
    public void ContentsKeepWrittenPrefixWhenListAssertionFails()
    {
        var scene = new Scene(); scene.Head(5, 6); scene.Content(6, 70, 0x3412, 0x10);
        scene.Surface.Storage.Types[70].Flags1 = 0x3000; scene.Surface.Storage.Types[6].ContainerListFlags = 0x20;
        scene.Assertion = (text, file, line) =>
        {
            Assert.Equal("(f.s().getListFlags() & lf) == lf", text); Assert.Equal(@"\Ns\O\Template.cpp", file);
            Assert.Equal(0x116u, line); throw new IOException("목록 진단 거부");
        };
        TemplateWriteStream stream = scene.Templates.CreateSection(1, "Technology");
        Assert.Throws<IOException>(() => scene.Save(5, stream));
        Assert.Equal(new byte[] { 1, 70, 0x12, 0x12, 0x34 }, scene.Templates.SectionBuffer(stream).ToArray());
        Assert.Equal(5u, Observe(scene.Templates, stream).Position);
        scene.Surface.Storage.Types[70].ContentListFlags = 0x10; scene.Save(5, stream);
        Assert.Equal(new byte[] { 1, 70, 0x12, 0x12, 0x34, 1, 70, 0x12, 0x12, 0x34 }, scene.Templates.SectionBuffer(stream).ToArray());
    }

    /// <summary>잘못된 부모/종속 순환은 앞선 count를 남기며 반납된 동일 이름 stream을 새 수명으로 대신하지 않는다.</summary>
    [Fact]
    public void InvalidParentCycleAndRetiredStreamRemainExplicitFailures()
    {
        var scene = new Scene(); TemplateWriteStream first = scene.Templates.CreateSection(1, "Technology");
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Save(0x10005, first));
        Assert.Equal(new byte[] { 0 }, scene.Templates.SectionBuffer(first).ToArray());
        scene.Head(5, 6); scene.Content(6, 70, 0, 0x10, 6);
        Assert.Throws<InvalidOperationException>(() => scene.Save(5, first));
        Assert.Equal(new byte[] { 0, 1, 70, 0x10 }, scene.Templates.SectionBuffer(first).ToArray());
        TemplateWriteStream replacement = scene.Templates.CreateSection(1, "Technology");
        Assert.Throws<InvalidOperationException>(() => scene.Save(5, first));
        Assert.Empty(scene.Templates.SectionBuffer(replacement).ToArray());
    }
}
