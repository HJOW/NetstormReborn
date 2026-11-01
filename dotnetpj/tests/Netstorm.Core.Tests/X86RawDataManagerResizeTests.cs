using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// DataManager 섹션 생성·복제·이름 교체·최소 길이 확장을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DataManagerResizeTests.cpp(286행)와 DataManagerCreateTests.cpp(220행)와 같다.
/// 행마다 원본이 관측한 전체 메모리(796/17380바이트)를 입력으로 받아 같은 외부 효과를 재생하고 전체 바이트·반환 핸들·효과 순서를 비교한다 (0.128, 2026-11-01).
/// </summary>
public sealed class X86RawDataManagerResizeTests
{
    /// <summary>확장 검사에서 노드/버퍼에 쓴 불투명 주소(원문 관측 입력)다.</summary>
    private const uint Nodes = 0x12100100, Buffers = 0x12200100;

    /// <summary>확장 검사의 노드/버퍼 한 칸 폭이다.</summary>
    private const uint NodeStride = 76, BufferStride = 80;

    /// <summary>확장 검사 전체 796바이트 안의 버퍼 영역/이름 영역 시작이다.</summary>
    private const int BufferStart = 332, NamesStart = 732;

    /// <summary>
    /// 부모/하위 열 본문의 전체 796바이트·반환 핸들·각 경계 상태를 독립 286행에 대조한다.
    /// 최소 확장(G), 이름 교체(R), 생성자(그 밖)를 한 파일에서 재생한다.
    /// </summary>
    [Fact]
    public void ResizeMatchesNative286Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-resize");
        Assert.Equal(286, rows.Length);
        List<string> problems = [];
        // 각 행은 다른 행의 결과와 독립적인 같은 원본 입력을 적용한다
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(7, input.Length);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(796, bytes.Length);
            char kind = input[0][0];
            uint position = Number(input[2]);
            uint nameKind = Number(input[5]);
            uint variant = Number(input[6]);
            int minimum = (int)long.Parse(input[4], System.Globalization.CultureInfo.InvariantCulture);
            uint allocations = 0;
            StringBuilder events = new();
            uint original = Nodes + position * NodeStride + 8;
            uint temporary = Nodes + 3 * NodeStride + 8;

            // 같은 manager 12바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 12);
            // 같은 raw 노드만 조회한다
            Span<byte> Node(uint handle)
            {
                uint offset = handle - Nodes - 8;
                if (offset % NodeStride != 0 || offset / NodeStride >= 4)
                {
                    throw new ArgumentOutOfRangeException(null, "확장 원문 노드 범위");
                }
                return bytes.AsSpan(28 + (int)(offset / NodeStride) * (int)NodeStride + 8, 60);
            }
            // 반납한 관측용 바이트도 지우지 않는 실제 버퍼 64바이트다
            Span<byte> Buffer(uint handle)
            {
                uint offset = handle - Buffers - 8;
                if (offset % BufferStride != 0 || offset / BufferStride >= 5)
                {
                    throw new ArgumentOutOfRangeException(null, "확장 원문 버퍼 범위");
                }
                return bytes.AsSpan(BufferStart + (int)(offset / BufferStride) * (int)BufferStride + 8, 64);
            }
            // 외부 효과 전 현재 796바이트를 기록한다
            void Event(char type, string value)
            {
                if (events.Length != 0)
                {
                    events.Append(';');
                }
                events.Append(type).Append(',').Append(value).Append(',').Append(Hex(bytes));
            }

            DataManagerResizeHooks hooks = new();
            hooks.Selection.Stream = Node;
            hooks.Selection.Compare = (left, right) =>
            {
                Event('Q', Hex(Latin1(left)) + '/' + Hex(Latin1(right)));
                return RawDataManagerSelection.CompareSectionNames(left, right);
            };
            hooks.Memory.Buffer = Buffer;
            // 원본 allocator의 명시 입력만 적용해 생성 뒤 data/end/alloc 재조회를 검사한다
            hooks.Memory.Allocate = size =>
            {
                Event('A', size.ToString());
                uint pointer = Buffers + (3 + allocations) * BufferStride + 8;
                if (variant == 1 && kind == 'G' && allocations == 0)
                {
                    Put(Node(original), 0, Buffers + ((position + 1) % 3) * BufferStride + 8);
                    Put(Node(original), 8, 2);
                    Put(Manager(), 8, 0xdead1234);
                }
                if (variant == 3 && allocations == 0)
                {
                    Put(Node(temporary), 12, size + 1);
                }
                allocations++;
                return pointer;
            };
            hooks.Memory.Release = pointer =>
            {
                Event('F', pointer.ToString());
                if (variant == 2)
                {
                    Put(Manager(), 8, 0xdead5678);
                }
            };
            hooks.Memory.Assertion = _ => throw new InvalidOperationException("정상 확장 원문에 없는 MemStream 진단");
            hooks.List.Lifetime.Stream = Node;
            hooks.List.Lifetime.ReleaseBuffer = hooks.Memory.Release;
            hooks.List.Lifetime.ReleaseStream = handle =>
            {
                Event('N', handle.ToString());
                if (variant == 2)
                {
                    Put(Manager(), 8, 0xdead5678);
                }
            };
            hooks.List.Compact = handle => RawMemStream.Compact(Node(handle), hooks.Memory);
            hooks.List.MissingPredecessor = () => throw new InvalidOperationException("정상 확장 원문에 없는 링크 진단");
            hooks.AllocateStream = size =>
            {
                Event('S', size.ToString());
                Assert.Equal(60u, size);
                return temporary;
            };
            hooks.MissingSource = () => throw new InvalidOperationException("정상 확장 원문에 없는 원문 누락");

            string desired = TextAt(bytes.AsSpan(NamesStart));
            string prior = TextAt(bytes.AsSpan(NamesStart + 32));
            uint result;
            if (kind == 'G')
            {
                result = RawDataManagerResize.EnsureStreamSize(Manager(), desired, minimum, hooks);
            }
            else if (kind == 'R')
            {
                result = RawDataManagerResize.RenameStream(Manager(), desired, prior, hooks);
            }
            else
            {
                result = RawDataManagerResize.ConstructStream(temporary, nameKind == 2 ? null : nameKind == 1 ? prior : desired, minimum, hooks);
            }
            if (result != Number(row[3]) || Hex(bytes) != row[2] || (events.Length == 0 ? "-" : events.ToString()) != row[4])
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "DataManager 확장 286행");
    }

    /// <summary>signed 최소 크기와 충분한 원문은 할당/순서/cursor를 보존하며, 필수 장치가 없으면 사용 지점에서 거부한다.</summary>
    [Fact]
    public void SufficientStateKeepsStorageAndRequiredHooksAreEnforced()
    {
        // 충분한 32바이트 원문과 단일 노드 목록을 직접 구성한다
        byte[] manager = new byte[12];
        byte[] node = new byte[60];
        byte[] buffer = new byte[64];
        Array.Fill(buffer, (byte)0xa7);
        Put(manager, 4, 1);
        Put(node, 0, 2);
        Put(node, 4, 17);
        Put(node, 8, 32);
        Put(node, 12, 64);
        Encoding.Latin1.GetBytes("State", node.AsSpan(16));
        DataManagerResizeHooks hooks = new();
        hooks.Selection.Stream = handle => handle == 1 ? node : throw new ArgumentOutOfRangeException(null, "노드 범위");
        // 실제 signed 부모의 음수/0/이미 충분한 최소 경계를 같은 저장소에 적용한다
        foreach (int minimum in new[] { -1, 0, 1, 28, 32 })
        {
            RawDataManagerResize.EnsureStreamSize(manager, "State", minimum, hooks);
            Assert.Equal(17u, Get(node, 4));
            Assert.Equal(32u, Get(node, 8));
            Assert.Equal(2u, Get(node, 0));
            Assert.Equal(1u, Get(manager, 4));
        }
        Assert.Throws<InvalidOperationException>(() => RawDataManagerResize.EnsureStreamSize(manager, "Missing", 1, hooks));
        Assert.Throws<InvalidOperationException>(() => RawDataManagerResize.EnsureStreamSize(manager, "State", 33, hooks));
    }

    /// <summary>생성 검사에서 노드/버퍼/큰 버퍼에 쓴 불투명 주소(원문 관측 입력)다.</summary>
    private const uint CreateNodes = 0x12100100, CreateBuffers = 0x12200100, CreateLarge = 0x12210000;

    /// <summary>생성 검사 전체 17380바이트 안의 노드/버퍼/큰 버퍼/이름 시작과 한 칸 폭이다.</summary>
    private const int CreateNodeStart = 56, CreateNodeStride = 76, CreateBufferStart = 436, CreateBufferStride = 80, CreateLargeStart = 916, CreateNamesStart = 17316;

    /// <summary>
    /// 열한 실제 본문의 전체 17380바이트·반환 핸들·모든 경계 직전 상태를 독립 220행과 대조한다.
    /// 생성(N), 복제(C), 대입(그 밖)을 재생하며 외부 효과마다 직전 전체 메모리를 기대 효과 열과 즉시 비교한다.
    /// </summary>
    [Fact]
    public void CreateMatchesNative220Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-create");
        Assert.Equal(220, rows.Length);
        List<string> problems = [];
        // 각 행의 입력/기대 바이트는 원본 PE의 최초 관측만 사용한다
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            string[] inputs = row[0].Split(',');
            Assert.Equal(6, inputs.Length);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(17380, bytes.Length);
            char kind = inputs[0][0];
            uint variant = Number(inputs[5]);
            uint original = CreateNodes + 3 * CreateNodeStride + 8;
            uint fresh = CreateNodes + 4 * CreateNodeStride + 8;
            uint smallAllocations = 0;
            int eventIndex = 0;
            string[] events = row[4] == "-" ? [] : row[4].Split(';');
            bool failed = false;

            // 같은 manager/원문 manager 12바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 12);
            Span<byte> SourceManager() => bytes.AsSpan(36, 12);
            // 같은 실제 노드 60바이트다
            Span<byte> Node(uint handle)
            {
                uint offset = handle - CreateNodes - 8;
                if (offset % CreateNodeStride != 0 || offset / CreateNodeStride >= 5)
                {
                    throw new ArgumentOutOfRangeException(null, "생성 원문 노드 범위");
                }
                return bytes.AsSpan(CreateNodeStart + (int)(offset / CreateNodeStride) * CreateNodeStride + 8, 60);
            }
            // 원문 data의 +3바이트 변이도 같은 할당 내부에서 읽는다
            Span<byte> Buffer(uint pointer)
            {
                if (pointer >= CreateLarge + 8 && pointer < CreateLarge + 8 + 0x4000)
                {
                    uint largeOffset = pointer - CreateLarge - 8;
                    return bytes.AsSpan(CreateLargeStart + 8 + (int)largeOffset, 0x4000 - (int)largeOffset);
                }
                uint offset = pointer - CreateBuffers - 8;
                uint index = offset / CreateBufferStride;
                uint inside = offset % CreateBufferStride;
                if (index >= 6 || inside >= 64)
                {
                    throw new ArgumentOutOfRangeException(null, "생성 원문 실제 버퍼 범위");
                }
                return bytes.AsSpan(CreateBufferStart + (int)index * CreateBufferStride + 8 + (int)inside, 64 - (int)inside);
            }
            // 제품의 현 메모리를 최초 경계 직전의 전체 원문과 즉시 비교한다
            void Event(char type, string value)
            {
                if (eventIndex >= events.Length)
                {
                    failed = true;
                    throw new InvalidOperationException("생성 원문에 없는 경계 효과");
                }
                string[] expected = events[eventIndex++].Split(',');
                if (expected.Length != 3)
                {
                    failed = true;
                    throw new InvalidOperationException("생성 원문의 경계 열 누락");
                }
                if (expected[0] != type.ToString() || expected[1] != value || !Decode(expected[2]).AsSpan().SequenceEqual(bytes))
                {
                    failed = true;
                }
            }

            DataManagerResizeHooks hooks = new();
            hooks.Selection.Stream = Node;
            hooks.Selection.Compare = (left, right) =>
            {
                Event('Q', Hex(Latin1(left)) + '/' + Hex(Latin1(right)));
                return RawDataManagerSelection.CompareSectionNames(left, right);
            };
            hooks.Memory.Buffer = Buffer;
            // 할당 경계에서 원문을 바꿔 대입 부모의 data/end 재조회를 검사한다
            hooks.Memory.Allocate = size =>
            {
                Event('A', size.ToString());
                uint pointer = size == 0x4000 ? CreateLarge + 8 : CreateBuffers + (4 + smallAllocations++) * CreateBufferStride + 8;
                if ((variant == 1 && size == 0x4000) || (variant == 3 && kind == 'M'))
                {
                    Put(Node(original), 0, CreateBuffers + 2 * CreateBufferStride + 11);
                    Put(Node(original), 8, 2);
                }
                return pointer;
            };
            // 반납 뒤 current/원문 변이는 본문의 후속 쓰기와 구별한다
            void Release(char type, uint pointer)
            {
                Event(type, pointer.ToString());
                if (variant == 2)
                {
                    Put(Manager(), 8, 0xdead5678);
                    Put(Node(original), 0, CreateBuffers + CreateBufferStride + 8);
                    Put(Node(original), 8, 3);
                }
            }
            hooks.Memory.Release = pointer => Release('F', pointer);
            hooks.Memory.Assertion = _ => throw new InvalidOperationException("정상 생성 원문에 없는 MemStream 진단");
            hooks.List.Lifetime.Stream = Node;
            hooks.List.Lifetime.ReleaseBuffer = hooks.Memory.Release;
            hooks.List.Lifetime.ReleaseStream = handle => Release('N', handle);
            hooks.List.Compact = handle => RawMemStream.Compact(Node(handle), hooks.Memory);
            hooks.List.MissingPredecessor = () => throw new InvalidOperationException("정상 생성 원문에 없는 선행 노드 진단");
            hooks.AllocateStream = size =>
            {
                Event('S', size.ToString());
                Assert.Equal(60u, size);
                return fresh;
            };
            hooks.MissingSource = () => throw new InvalidOperationException("정상 복제 원문에 없는 원문 누락");

            uint result = 0;
            string name = TextAt(bytes.AsSpan(CreateNamesStart));
            if (kind == 'N')
            {
                result = RawDataManagerResize.CreateStream(Manager(), name, hooks);
            }
            else if (kind == 'C')
            {
                result = RawDataManagerResize.CloneStream(Manager(), SourceManager(), name, hooks);
            }
            else
            {
                RawMemStream.Copy(Node(fresh), Node(original), hooks.Memory);
            }
            if (failed || (kind != 'M' && result != Number(row[3])) || !bytes.AsSpan().SequenceEqual(Decode(row[2])) || eventIndex != events.Length)
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "DataManager 생성 220행");
    }

    /// <summary>빈 manager의 new60 실패는 원본처럼 NULL을 추가하고 현재 0을 반환하며 임의 노드를 만들지 않는다. 필수 장치가 없으면 거부한다.</summary>
    [Fact]
    public void CreateEmptyNullAllocationAndRequiredEffects()
    {
        byte[] manager = new byte[12];
        uint requests = 0;
        DataManagerResizeHooks hooks = new()
        {
            AllocateStream = size =>
            {
                requests++;
                Assert.Equal(60u, size);
                return 0;
            },
        };
        Assert.Equal(0u, RawDataManagerResize.CreateStream(manager, "State", hooks));
        Assert.Equal(1u, requests);
        Assert.All(manager, value => Assert.Equal(0, value));
        Assert.Throws<InvalidOperationException>(() => RawDataManagerResize.CreateStream(manager, "State", new DataManagerResizeHooks()));
    }
}
