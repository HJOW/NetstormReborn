using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// MemStream 16바이트와 DataManager 섹션 목록·소멸·검색/선택을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DataManagerStreamsTests.cpp(404행), DataManagerLifetimeTests.cpp(392행), DataManagerSelectionTests.cpp(864행)와 같다.
/// 행마다 원본이 관측한 전체 메모리를 입력으로 받아 같은 외부 효과(할당·free·진단·비교)를 재생하고 전체 바이트와 효과 순서를 비교한다 (0.128, 2026-11-01).
/// </summary>
public sealed class X86RawDataManagerListTests
{
    /// <summary>스트림 검사에서 노드/버퍼에 쓴 불투명 주소(원문 관측 입력)다.</summary>
    private const uint StreamNodes = 0x12100100, StreamBuffers = 0x12200100;

    /// <summary>스트림 검사의 노드/버퍼 한 칸 폭과 전체 716바이트 안의 실제 시작 위치다.</summary>
    private const uint StreamNodeStride = 76, StreamBufferStride = 80;

    /// <summary>스트림 검사의 전체 716바이트 안 버퍼 영역/원문 영역 시작이다.</summary>
    private const int StreamBufferStart = 332, StreamSourceStart = 652;

    /// <summary>
    /// 실제 중첩 할당·쓰기·압축·소멸과 진단·힙 경계 뒤 재조회를 전체 404행에 대조한다. 매 행은 다른 행과 독립인 같은 전체 메모리를 쓴다.
    /// </summary>
    [Fact]
    public void MemStreamAndListMatchNative404Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-streams");
        Assert.Equal(404, rows.Length);
        List<string> problems = [];
        // 행마다 원본 입력 메모리 전체를 새로 복원해 같은 동작을 재생한다
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(10, input.Length);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(716, bytes.Length);
            char kind = input[0][0];
            uint count = Number(input[2]);
            uint select = Number(input[3]);
            uint length = Number(input[8]);
            uint variant = Number(input[9]);
            StringBuilder events = new();
            uint selected = StreamNodes + (kind == 'P' && count != 0 ? count - 1 : select) * StreamNodeStride + 8;

            // 같은 manager 12바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 12);
            // 같은 실제 노드 60바이트만 빌린다
            Span<byte> Node(uint handle)
            {
                uint offset = handle - StreamNodes - 8;
                if (offset % StreamNodeStride != 0 || offset / StreamNodeStride >= 4)
                {
                    throw new ArgumentOutOfRangeException(null, "원문 실제 노드 범위");
                }
                return bytes.AsSpan(28 + (int)(offset / StreamNodeStride) * (int)StreamNodeStride + 8, 60);
            }
            // free 뒤에도 관측용 바이트는 지우지 않는 실제 버퍼 64바이트다
            Span<byte> Buffer(uint handle)
            {
                uint offset = handle - StreamBuffers - 8;
                if (offset % StreamBufferStride != 0 || offset / StreamBufferStride >= 4)
                {
                    throw new ArgumentOutOfRangeException(null, "원문 실제 버퍼 범위");
                }
                return bytes.AsSpan(StreamBufferStart + (int)(offset / StreamBufferStride) * (int)StreamBufferStride + 8, 64);
            }
            // 외부 효과 직전 전체 716바이트를 기록한다
            void Event(char type, uint value)
            {
                if (events.Length != 0)
                {
                    events.Append(';');
                }
                events.Append(type).Append(',').Append(value).Append(',').Append(Hex(bytes));
            }

            MemStreamHooks memory = new() { Buffer = Buffer };
            // 원문 allocator의 명시 변이 입력이다
            memory.Allocate = size =>
            {
                Event('A', size);
                if (variant == 1)
                {
                    Put(Node(selected), 12, size / 2);
                    Put(Node(selected), 4, 0x87654321);
                }
                return StreamBuffers + 3 * StreamBufferStride + 8;
            };
            // 원본 free 경계 뒤 재조회를 대조한다
            memory.Release = handle =>
            {
                Event('F', handle);
                if (variant == 2)
                {
                    Put(Node(selected), 4, 0x87654321);
                    Put(Node(selected), 8, 0x1234);
                    Put(Manager(), 8, 0x456789ab);
                }
            };
            memory.Assertion = reason =>
            {
                Event('D', reason == MemStreamAssertion.ExistingData ? 0x14u : 0x24u);
                if (kind == 'W' && variant == 1)
                {
                    Put(Node(selected), 4, 5);
                }
            };
            DataManagerListHooks list = new();
            list.Lifetime.Stream = Node;
            list.Lifetime.ReleaseBuffer = memory.Release;
            list.Lifetime.ReleaseStream = handle => Event('N', handle);
            list.Compact = handle => RawMemStream.Compact(Node(handle), memory);
            list.MissingPredecessor = () => throw new InvalidOperationException("정상 원문404행에 없는 링크 진단");

            switch (kind)
            {
                case 'A':
                    RawMemStream.Allocate(Node(selected), length, memory);
                    break;
                case 'W':
                    RawMemStream.Write(Node(selected), variant == 3 ? Buffer(StreamBuffers + 8)[1..] : bytes.AsSpan(StreamSourceStart, 64), length, memory);
                    break;
                case 'C':
                    RawMemStream.Compact(Node(selected), memory);
                    break;
                case 'P':
                    RawDataManagerList.Append(Manager(), select != 0 ? StreamNodes + 3 * StreamNodeStride + 8 : 0, list);
                    break;
                case 'R':
                    RawDataManagerList.Remove(Manager(), selected, list);
                    break;
                default:
                    throw new InvalidOperationException("원문 입력의 동작 종류 오류");
            }
            string trace = events.Length == 0 ? "-" : events.ToString();
            if (Hex(bytes) != row[2] || trace != row[3])
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "MemStream·목록 404행");
    }

    /// <summary>길이 0과 빈 목록은 원본이 실제로 접근하지 않는 버퍼/노드 소비자를 요구하지 않고, 부족한 물리 폭은 쓰기 전에 거부한다.</summary>
    [Fact]
    public void MemStreamPhysicalAndZeroContracts()
    {
        byte[] storage = new byte[24];
        Array.Fill(storage, (byte)0xa5);
        byte[] before = [.. storage];
        Assert.True(Throws(() => RawMemStream.Allocate(storage.AsSpan(0, 15), 0, new MemStreamHooks())));
        Assert.Equal(before, storage);
        Span<byte> stream = storage.AsSpan(0, 16);
        Put(stream, 0, 0);
        MemStreamHooks memory = new() { Allocate = size => { Assert.Equal(0u, size); return 0; } };
        RawMemStream.Allocate(stream, 0, memory);
        Assert.All(stream.ToArray(), value => Assert.Equal(0, value));
        Assert.Equal(0xa5, storage[16]);
        RawMemStream.Write(stream, default, 0, new MemStreamHooks());
        RawMemStream.Compact(stream, new MemStreamHooks());
        Assert.Equal(0u, Get(stream, 4));
        Assert.Equal(0u, Get(stream, 8));
        byte[] manager = new byte[12];
        Put(manager, 0, 0x87654321);
        RawDataManagerList.Append(manager, 0x1234, new DataManagerListHooks());
        Assert.Equal(0x87654321u, Get(manager, 0));
        Assert.Equal(0x1234u, Get(manager, 4));
        Assert.Equal(0x1234u, Get(manager, 8));
        Put(manager, 4, 0);
        Assert.True(Throws(() => RawDataManagerList.Remove(manager, 0, new DataManagerListHooks())));
    }

    /// <summary>
    /// 쓰기 용량 진단은 signed 비교다. pos+길이가 0x80000000 이상으로 넘어가 음수가 되면 양수 용량보다 작으므로 진단하지 않고,
    /// 그 뒤 실제 버퍼 물리 범위에서 거부한다. 원본 404행이 닿지 않는 부호 경계를 직접 확인한다.
    /// </summary>
    [Fact]
    public void MemStreamWriteCapacityUsesSignedComparison()
    {
        byte[] stream = new byte[16];
        Put(stream, 0, 1);
        Put(stream, 4, 0x7fffffff);
        Put(stream, 12, 0x7fffffff);
        int assertions = 0;
        MemStreamHooks hooks = new() { Buffer = _ => new byte[64], Assertion = _ => assertions++ };
        Assert.True(Throws(() => RawMemStream.Write(stream, new byte[1], 1, hooks)));
        Assert.Equal(0, assertions);
        // 용량이 pos+길이보다 작은 일반 경우에는 진단한 뒤 원본처럼 계속 쓴다
        Put(stream, 4, 10);
        Put(stream, 12, 10);
        RawMemStream.Write(stream, new byte[1], 1, hooks);
        Assert.Equal(1, assertions);
        Assert.Equal(11u, Get(stream, 4));
        Assert.Equal(11u, Get(stream, 8));
    }

    /// <summary>
    /// 목록에 없는 대상을 제거하면 탐색이 끝난 뒤 마지막 선행 노드의 next가 0이라 진단한다. 진단이 돌아와도 원본은 next 0의 노드를 읽으므로 명시 오류로 멈춘다.
    /// 원본 404행에는 이 경로가 없어 직접 확인한다.
    /// </summary>
    [Fact]
    public void RemoveMissingTargetDiagnosesAfterSearch()
    {
        byte[] manager = new byte[12];
        byte[][] nodes = [new byte[60], new byte[60]];
        Put(manager, 4, 1);
        Put(nodes[0], 0x38, 2);
        int diagnosed = 0;
        DataManagerListHooks hooks = new() { MissingPredecessor = () => diagnosed++ };
        hooks.Lifetime.Stream = handle => nodes[handle - 1];
        Assert.True(Throws(() => RawDataManagerList.Remove(manager, 3, hooks)));
        Assert.Equal(1, diagnosed);
        Assert.Equal(1u, Get(manager, 4));
        Assert.Equal(2u, Get(nodes[0], 0x38));
    }

    /// <summary>진단 뒤 수리된 같은 선행 노드를 다시 읽는 계약이다. 404행의 통과 수와 구분한다.</summary>
    [Fact]
    public void RemoveRequeriesRepairedPredecessor()
    {
        byte[] manager = new byte[12];
        byte[][] nodes = [new byte[60], new byte[60], new byte[60]];
        Put(manager, 4, 1);
        Put(manager, 8, 9);
        Put(nodes[1], 4, 17);
        Put(nodes[1], 8, 19);
        uint released = 0;
        uint diagnosed = 0;
        DataManagerListHooks hooks = new();
        hooks.Lifetime.Stream = handle => nodes[handle - 1];
        hooks.Lifetime.ReleaseStream = handle => released = handle;
        hooks.MissingPredecessor = () =>
        {
            diagnosed++;
            Put(nodes[0], 0x38, 2);
            Put(nodes[1], 0x38, 3);
        };
        RawDataManagerList.Remove(manager, 2, hooks);
        Assert.Equal(1u, diagnosed);
        Assert.Equal(2u, released);
        Assert.Equal(1u, Get(manager, 4));
        Assert.Equal(9u, Get(manager, 8));
        Assert.Equal(3u, Get(nodes[0], 0x38));
        Assert.Equal(3u, Get(nodes[1], 0x38));
        Assert.Equal(0u, Get(nodes[1], 4));
        Assert.Equal(0u, Get(nodes[1], 8));
    }

    /// <summary>
    /// 자기 대입은 free 뒤 같은 첫 16바이트가 비워져 길이 0으로 재할당되는 원본의 결과다.
    /// </summary>
    [Fact]
    public void MemStreamCopySelfAssignmentResetsBeforeReadingLength()
    {
        byte[] node = new byte[60];
        Array.Fill(node, (byte)0xa7, 16, 44);
        Put(node, 0, 1);
        Put(node, 4, 4);
        Put(node, 8, 7);
        Put(node, 12, 8);
        uint releases = 0;
        uint allocations = 0;
        MemStreamHooks hooks = new()
        {
            Release = pointer =>
            {
                releases++;
                Assert.Equal(1u, pointer);
                Assert.Equal(4u, Get(node, 4));
                Assert.Equal(7u, Get(node, 8));
            },
            Allocate = size =>
            {
                allocations++;
                Assert.True(size == 0 && Get(node, 0) == 0 && Get(node, 4) == 0 && Get(node, 8) == 0 && Get(node, 12) == 0);
                return 2;
            },
            Buffer = _ => throw new InvalidOperationException("0길이 자기 대입에서 버퍼 조회 금지"),
        };
        RawMemStream.Copy(node, node, hooks);
        Assert.Equal(1u, releases);
        Assert.Equal(1u, allocations);
        Assert.Equal(2u, Get(node, 0));
        Assert.Equal(0u, Get(node, 4));
        Assert.Equal(0u, Get(node, 8));
        Assert.Equal(0u, Get(node, 12));
        Assert.All(node.Skip(16), value => Assert.Equal(0xa7, value));
    }

    /// <summary>DataManager 소멸 노드의 불투명 주소와 한 칸 폭이다.</summary>
    private const uint LifetimeNodes = 0x12101000, LifetimeBuffers = 0x12200000;

    /// <summary>수명 검사의 한 칸 폭(양쪽 보호 8바이트 포함)과 노드 수다.</summary>
    private const int LifetimeStride = 76, LifetimeNodeCount = 5;

    /// <summary>전체 412바이트·매 free 직전 상태·외부 효과 뒤 head 재조회와 stale 필드를 392행에 대조한다.</summary>
    [Fact]
    public void LifetimeMatchesNative392Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-lifetime");
        Assert.Equal(392, rows.Length);
        List<string> problems = [];
        // 각 행의 원본 입력은 제품 출력이나 직전 행의 결과와 독립적이다
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(6, input.Length);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(32 + LifetimeStride * LifetimeNodeCount, bytes.Length);
            uint variant = Number(input[5]);
            StringBuilder trace = new();

            // 같은 manager 16바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 16);
            // 현재 불투명 node만 같은 60바이트로 해석한다
            Span<byte> Stream(uint handle)
            {
                uint index = (handle - LifetimeNodes - 8) / LifetimeStride;
                Assert.True(index < LifetimeNodeCount);
                return bytes.AsSpan(32 + (int)index * LifetimeStride + 8, 60);
            }
            // 원본 관측의 명시 free 경계 변이만 적용하며 제품 판단을 대체하지 않는다
            void Release(char kind, uint handle)
            {
                if (trace.Length != 0)
                {
                    trace.Append(';');
                }
                trace.Append(kind).Append(',').Append(handle).Append(',').Append(Hex(bytes));
                uint index = kind == 'B' ? (handle - LifetimeBuffers) / 0x1000u : (handle - LifetimeNodes - 8) / LifetimeStride;
                Span<byte> node = bytes.AsSpan(32 + (int)index * LifetimeStride + 8, 60);
                if (variant == 1 && kind == 'B')
                {
                    Put(node, 0, 0xabcdef01);
                    Put(node, 4, 0x87654321);
                    Put(node, 0x38, 0x98765432);
                }
                if (variant == 2 && kind == 'N')
                {
                    Put(Manager(), 4, 0);
                    Put(Manager(), 8, 0x87654321);
                }
                if (variant == 3 && kind == 'B')
                {
                    uint following = Get(Manager(), 4);
                    if (following != 0)
                    {
                        Put(Stream(following), 0x38, 0);
                    }
                }
            }
            DataManagerLifetimeHooks hooks = new()
            {
                Stream = Stream,
                ReleaseBuffer = handle => Release('B', handle),
                ReleaseStream = handle => Release('N', handle),
            };
            if (input[0] == "S")
            {
                RawDataManagerLifetime.DestroyMemStream(Stream(LifetimeNodes + 8), hooks.ReleaseBuffer);
            }
            else
            {
                RawDataManagerLifetime.DestroyDataManager(Manager(), hooks);
            }
            if (Hex(bytes) != row[2] || (trace.Length == 0 ? "-" : trace.ToString()) != row[3])
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "DataManager 수명 392행");
    }

    /// <summary>실제 폭이 부족하면 쓰기 전에 거부하고 빈 버퍼/목록은 외부 장치 없이 자연 반환한다.</summary>
    [Fact]
    public void LifetimePhysicalContractsAndEmptyStorage()
    {
        byte[] bytes = new byte[16];
        Array.Fill(bytes, (byte)0xa5);
        byte[] before = [.. bytes];
        Assert.Throws<ArgumentException>(() => RawDataManagerLifetime.DestroyMemStream(bytes.AsSpan(0, 15), null));
        Assert.Equal(before, bytes);
        Assert.Throws<ArgumentException>(() => RawDataManagerLifetime.DestroyDataManager(bytes.AsSpan(0, 11), new DataManagerLifetimeHooks()));
        Assert.Equal(before, bytes);
        Put(bytes, 0, 0);
        RawDataManagerLifetime.DestroyMemStream(bytes, null);
        Assert.All(bytes, value => Assert.Equal(0, value));
        Put(bytes, 8, 0x87654321);
        Put(bytes, 12, 0x12345678);
        RawDataManagerLifetime.DestroyDataManager(bytes, new DataManagerLifetimeHooks());
        Assert.Equal(0x507dd8u, Get(bytes, 0));
        Assert.Equal(0u, Get(bytes, 4));
        Assert.Equal(0x87654321u, Get(bytes, 8));
        Assert.Equal(0x12345678u, Get(bytes, 12));
    }

    /// <summary>선택 검사에서 노드에 쓴 불투명 주소와 한 칸 폭이다.</summary>
    private const uint SelectionNodes = 0x12100100, SelectionStride = 76;

    /// <summary>이름/순서/첫 일치/검색 실패/current 변이/두 진단 뒤 재조회와 정확한 쓰기 폭을 864행에 대조한다. 첫 행은 CSV 머리글이다.</summary>
    [Fact]
    public void SelectionMatchesNative864Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-selection");
        Assert.True(rows.Length == 865 && rows[0][0] == "input");
        string[] names = ["sTaTe", "TERRITORY", "missing"];
        List<string> problems = [];
        int compared = 0;
        // 각 행은 독립적으로 관측한 원본 전체 메모리를 같은 저장소에 적용한다
        foreach (string[] row in rows)
        {
            if (row[0] == "input")
            {
                continue;
            }
            Assert.Equal(5, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(7, input.Length);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(256, bytes.Length);
            uint variant = Number(input[6]);
            uint query = Number(input[3]);
            StringBuilder trace = new();
            uint comparisons = 0;

            // 같은 manager 12바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 12);
            DataManagerSelectionHooks hooks = new()
            {
                // 같은 입력 노드의 실제 60바이트만 빌린다
                Stream = handle =>
                {
                    uint index = (handle - SelectionNodes - 8) / SelectionStride;
                    if (index >= 3 || (handle - SelectionNodes - 8) % SelectionStride != 0)
                    {
                        throw new ArgumentOutOfRangeException(null, "검색 원문 노드 범위");
                    }
                    return bytes.AsSpan(28 + (int)index * (int)SelectionStride + 8, 60);
                },
                // 비교 결과는 공식 ASCII CRT 경계이며 원본 판단 본문을 대체하지 않는다
                Compare = (left, right) =>
                {
                    if (trace.Length != 0)
                    {
                        trace.Append(';');
                    }
                    trace.Append("C,").Append(Hex(Latin1(right))).Append(',').Append(Hex(bytes));
                    comparisons++;
                    if ((variant == 1 || variant == 2) && comparisons == 1)
                    {
                        Put(Manager(), 8, SelectionNodes + 2 * SelectionStride + 8);
                    }
                    return variant == 1 && comparisons == 1 ? 0 : RawDataManagerSelection.CompareSectionNames(left, right);
                },
                // 원본 관측에 명시한 진단 수리 입력만 적용한다
                Assertion = kind =>
                {
                    int line = kind == DataManagerSelectionAssertion.MissingCurrent ? 0x68 : 0x6a;
                    if (trace.Length != 0)
                    {
                        trace.Append(';');
                    }
                    trace.Append("A,").Append(line).Append(',').Append(Hex(bytes));
                    if (line == 0x68 || variant == 3)
                    {
                        Put(Manager(), 8, SelectionNodes + 2 * SelectionStride + 8);
                    }
                },
            };
            uint result = input[0] == "F"
                ? RawDataManagerSelection.FindSection(Manager(), names[query], hooks)
                : RawDataManagerSelection.SelectSection(Manager(), names[query], unchecked((int)Number(input[5])), hooks);
            if (result != Number(row[3]) || Hex(bytes) != row[2] || (trace.Length == 0 ? "-" : trace.ToString()) != row[4])
            {
                problems.Add(row[0]);
            }
            compared++;
        }
        Assert.Equal(864, compared);
        NoProblems(problems, "DataManager 선택 864행");
    }

    /// <summary>실제 이름 규칙과 물리 범위를 검증하고 복원되지 않은 CRT 로캘/잘못된 포인터를 성공으로 처리하지 않는다.</summary>
    [Fact]
    public void SelectionPhysicalContractsAndAsciiNames()
    {
        Assert.Equal(0, RawDataManagerSelection.CompareSectionNames("STATE", "state"));
        Assert.Equal(0, RawDataManagerSelection.CompareSectionNames("State\0ignored", "state"));
        Assert.True(RawDataManagerSelection.CompareSectionNames("State", "State0") < 0);
        Assert.True(RawDataManagerSelection.CompareSectionNames("Territory", "State") > 0);
        byte[] manager = new byte[12];
        Array.Fill(manager, (byte)0xa5);
        byte[] before = [.. manager];
        Assert.Throws<ArgumentException>(() => RawDataManagerSelection.FindSection(manager.AsSpan(0, 11), "State", new DataManagerSelectionHooks()));
        Assert.Equal(before, manager);
        Put(manager, 4, 0);
        Assert.Equal(0u, RawDataManagerSelection.FindSection(manager, "State", new DataManagerSelectionHooks()));
        Assert.Equal(0u, Get(manager, 8));
        Assert.Throws<InvalidOperationException>(() => RawDataManagerSelection.SelectSection(manager, "State", 1, new DataManagerSelectionHooks()));
        Assert.Equal(0u, Get(manager, 8));
        Assert.Throws<ArgumentException>(() => RawDataManagerSelection.CompareSectionNames("À", "à"));
    }
}
