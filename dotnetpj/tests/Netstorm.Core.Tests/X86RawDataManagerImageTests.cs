using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// DataManager 파일 이미지 읽기(섹션 병합·mark 진행·Template 서명·파일 읽기 부모)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DataManagerReadTests.cpp(144행)와 같다. 행마다 원본이 관측한 전체 1400바이트를 입력으로 받아 같은 외부 효과를 재생하고
/// 전체 바이트와 경계 직전 상태를 비교한다 (0.128, 2026-11-01).
/// </summary>
public sealed class X86RawDataManagerImageTests
{
    /// <summary>읽기 검사에서 노드/버퍼/입력에 쓴 불투명 주소(원문 관측 입력)다.</summary>
    private const uint Nodes = 0x12100100, Buffers = 0x12200100, Inputs = 0x12201000;

    /// <summary>읽기 검사 전체 1400바이트 안의 노드/버퍼/입력/이름표 시작이다.</summary>
    private const int NodeStart = 64, BufferStart = 520, InputStart = 1080, NamesStart = 1336;

    /// <summary>아홉 본문과 큰 부모의 인라인 처리를 전체 1400바이트·모든 경계 상태의 독립 144행과 대조한다.</summary>
    [Fact]
    public void ReadMatchesNative144Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-read");
        Assert.Equal(144, rows.Length);
        List<string> problems = [];
        // 초기화와 변이는 원본 입력만 사용하며 행 사이에 결과를 넘기지 않는다
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(7, input.Length);
            uint variant = Number(input[6]);
            byte[] bytes = Decode(row[1]);
            Assert.Equal(1400, bytes.Length);
            uint created = 0;
            uint allocating = 0;
            int eventIndex = 0;
            bool failed = false;
            string[] events = row[3].Split(';');

            // 같은 manager 12바이트와 MarkStream 20바이트다
            Span<byte> Manager() => bytes.AsSpan(8, 12);
            Span<byte> Source() => bytes.AsSpan(36, 20);
            // 같은 60바이트 노드다
            Span<byte> Node(uint pointer)
            {
                uint offset = pointer - Nodes - 8;
                if (offset % 76 != 0 || offset / 76 >= 6)
                {
                    throw new ArgumentOutOfRangeException(null, "파일 읽기 원문의 노드 범위");
                }
                return bytes.AsSpan(NodeStart + (int)(offset / 76) * 76 + 8, 60);
            }
            // 실제 입력 원문/대체 입력/할당은 서로 다른 같은 배열이다
            Span<byte> Buffer(uint pointer)
            {
                if (pointer == Inputs || pointer == Inputs + 128)
                {
                    return bytes.AsSpan(InputStart + (int)(pointer - Inputs), 128);
                }
                uint offset = pointer - Buffers - 8;
                if (offset % 80 != 0 || offset / 80 >= 7)
                {
                    throw new ArgumentOutOfRangeException(null, "파일 읽기 원문의 버퍼 범위");
                }
                return bytes.AsSpan(BufferStart + (int)(offset / 80) * 80 + 8, 64);
            }
            // 외부 효과 직전 전체 상태를 즉시 최초 원문과 비교한다
            void Event(char kind, string value)
            {
                if (eventIndex >= events.Length)
                {
                    failed = true;
                    throw new InvalidOperationException("파일 읽기 원문에 없는 경계 효과");
                }
                string[] expected = events[eventIndex++].Split(',');
                if (expected.Length != 3)
                {
                    failed = true;
                    throw new InvalidOperationException("파일 읽기 경계 열 누락");
                }
                if (expected[0] != kind.ToString() || expected[1] != value || !Decode(expected[2]).AsSpan().SequenceEqual(bytes))
                {
                    failed = true;
                }
            }
            // free 뒤 current 변이를 원본 후속 쓰기와 구분한다
            void Release(char kind, uint pointer)
            {
                Event(kind, pointer.ToString());
                if (variant == 2)
                {
                    Put(Manager(), 8, 0xdead5678);
                }
            }

            DataManagerImageHooks hooks = new()
            {
                Names = () =>
                {
                    Event('T', "names");
                    return bytes.AsSpan(NamesStart, 64);
                },
            };
            hooks.Streams.Selection.Stream = Node;
            hooks.Streams.Selection.Compare = (left, right) =>
            {
                Event('Q', Hex(Latin1(left)) + '/' + Hex(Latin1(right)));
                return RawDataManagerSelection.CompareSectionNames(left, right);
            };
            hooks.Streams.AllocateStream = size =>
            {
                Event('S', size.ToString());
                Assert.True(size == 60 && created < 3);
                allocating = Nodes + (3 + created++) * 76 + 8;
                return allocating;
            };
            hooks.Streams.Memory.Buffer = Buffer;
            hooks.Streams.Memory.Allocate = size =>
            {
                Event('A', size.ToString());
                uint index = (allocating - Nodes - 8) / 76;
                Assert.True(size <= 64);
                if (variant == 1)
                {
                    Put(Source(), 0, Inputs + 128);
                }
                return Buffers + (index >= 3 ? index : 6) * 80 + 8;
            };
            hooks.Streams.Memory.Release = pointer => Release('F', pointer);
            hooks.Streams.List.Lifetime.Stream = Node;
            hooks.Streams.List.Lifetime.ReleaseBuffer = hooks.Streams.Memory.Release;
            hooks.Streams.List.Lifetime.ReleaseStream = pointer => Release('N', pointer);
            hooks.Streams.List.Compact = pointer =>
            {
                allocating = pointer;
                RawMemStream.Compact(Node(pointer), hooks.Streams.Memory);
            };
            hooks.Streams.List.MissingPredecessor = () => throw new InvalidOperationException("정상 파일 원문에 없는 선행 링크 진단");
            hooks.Streams.Memory.Assertion = _ => throw new InvalidOperationException("정상 파일 원문에 없는 MemStream 진단");
            hooks.Assertion = _ => throw new InvalidOperationException("정상 파일 원문에 없는 mark/SNART 진단");

            RawDataManagerImage.ReadImage(Manager(), Source(), hooks);
            if (failed || !bytes.AsSpan().SequenceEqual(Decode(row[2])) || eventIndex != events.Length)
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "DataManager 파일 읽기 144행");
    }

    /// <summary>진단이 pos/end를 바꾸면 원본 mark 부모도 그 현재 값을 읽어 진행한다.</summary>
    [Fact]
    public void MarkAssertionUsesCurrentFields()
    {
        byte[] source = new byte[20];
        byte[] image = [5, 0, 0, 0, 0, 0, 0, 0];
        Put(source, 0, 1);
        Put(source, 4, 3);
        Put(source, 8, 3);
        Put(source, 16, 0);
        uint diagnoses = 0;
        DataManagerImageHooks hooks = new();
        hooks.Streams.Memory.Buffer = pointer =>
        {
            Assert.Equal(1u, pointer);
            return image;
        };
        hooks.Assertion = kind =>
        {
            diagnoses++;
            Assert.Equal(DataManagerImageAssertion.MarkLength, kind);
            Put(source, 4, 4);
            Put(source, 8, 4);
        };
        RawDataManagerImage.AdvanceMark(source, hooks);
        Assert.Equal(1u, diagnoses);
        Assert.Equal(4u, Get(source, 16));
        Assert.Equal(6u, Get(source, 4));
        Assert.Equal(4u, Get(source, 8));
    }

    /// <summary>이름표 가상 조회가 먼저 수행되고 잘못된 시작 pos 진단의 수리를 다음 mark가 읽어야 한다. 이름표가 없으면 거부한다.</summary>
    [Fact]
    public void InitialPositionAndRequiredNames()
    {
        byte[] manager = new byte[12];
        byte[] source = new byte[20];
        byte[] names = [(byte)'/', (byte)'/', 0, 0];
        Put(source, 4, 3);
        Put(source, 8, 2);
        Put(source, 16, 0xffffffffu);
        StringBuilder order = new();
        DataManagerImageHooks hooks = new()
        {
            Names = () =>
            {
                order.Append('N');
                return names;
            },
            Assertion = kind =>
            {
                order.Append('D');
                Assert.Equal(DataManagerImageAssertion.SourcePosition, kind);
                Put(source, 4, 2);
            },
        };
        RawDataManagerImage.ReadImage(manager, source, hooks);
        Assert.Equal("ND", order.ToString());
        Assert.Equal(2u, Get(source, 16));
        Assert.Equal(4u, Get(source, 4));
        Assert.Equal(0, names[0]);
        Assert.Equal(0, names[1]);
        Assert.Equal(0xff, names[2]);
        Assert.Throws<ArgumentException>(() => RawDataManagerImage.ReadImage(manager, source, new DataManagerImageHooks()));
    }

    /// <summary>논리 end 1도 실제 두 바이트가 있으면 원본처럼 읽는다. signed 타입 수/FE 정확 비교와 양쪽 보호 영역을 검사한다.</summary>
    [Fact]
    public void SignatureBytesAndSignedTypeLimit()
    {
        // FE 이외의 값과 signed 음수는 원본의 서로 다른 비교를 통과한다
        foreach (byte flag in new byte[] { 0, 0xfe, 0xff })
        {
            foreach (int count in new[] { -1, 255, 256 })
            {
                byte[] bytes = new byte[292];
                Array.Fill(bytes, (byte)0xa7);
                byte[] before = [.. bytes];
                Span<byte> manager = bytes.AsSpan(8, 276);
                byte[] source = new byte[20];
                byte[] input = [(byte)'F', flag];
                int diagnoses = 0;
                Put(source, 0, 1);
                Put(source, 8, 1);
                Put(source, 12, 2);
                Put(source, 16, 0xffffffffu);
                TemplateFileSignatureHooks hooks = new()
                {
                    Buffer = pointer =>
                    {
                        Assert.Equal(1u, pointer);
                        return input;
                    },
                    TypeCount = () => count,
                    TooManyTypes = () => diagnoses++,
                };
                Assert.True(RawDataManagerImage.ReadTemplateFileSignature(manager, source, hooks));
                Assert.Equal(2u, Get(source, 4));
                Assert.Equal(1u, Get(source, 8));
                Assert.Equal(2u, Get(source, 12));
                Assert.Equal(0xffffffffu, Get(source, 16));
                Assert.Equal(0x46u, Get(manager, 0x10c));
                Assert.Equal(flag == 0xfe ? 1u : 0u, Get(manager, 0x110));
                Assert.Equal(count >= 256 ? 1 : 0, diagnoses);
                // 원본 서명이 실제로 쓰는 두 DWORD만 비교에서 제외한다
                for (int index = 0; index < bytes.Length; index++)
                {
                    if (index < 8 + 0x10c || index >= 8 + 0x114)
                    {
                        Assert.Equal(before[index], bytes[index]);
                    }
                }
            }
        }
    }

    /// <summary>진단 뒤 현재 source 재조회·잘못된 서명의 현재 이름/제거 요청·NULL/짧은 물리 입력을 구별한다. 실제 파일은 건드리지 않는다.</summary>
    [Fact]
    public void SignatureFailureAndCurrentRereads()
    {
        byte[] manager = new byte[276];
        byte[] source = new byte[20];
        byte[] input = [(byte)'?', (byte)'F', 0xfe];
        int rejected = 0;
        int diagnoses = 0;
        Put(source, 0, 1);
        Put(source, 8, 3);
        TemplateFileSignatureHooks hooks = new()
        {
            TypeCount = () => 256,
            TooManyTypes = () =>
            {
                diagnoses++;
                Put(source, 0, 2);
                Put(source, 4, 1);
            },
            Buffer = pointer =>
            {
                Assert.Equal(2u, pointer);
                return input;
            },
        };
        Assert.True(RawDataManagerImage.ReadTemplateFileSignature(manager, source, hooks));
        Assert.Equal(1, diagnoses);
        Assert.Equal(3u, Get(source, 4));
        Assert.Equal(0x46u, Get(manager, 0x10c));
        Assert.Equal(1u, Get(manager, 0x110));
        hooks.TypeCount = () => 188;
        hooks.FileName = () => "rejected.fort";
        hooks.RejectFile = name =>
        {
            Assert.Equal("rejected.fort", name);
            Assert.Equal(2u, Get(source, 4));
            Assert.Equal((uint)'?', Get(manager, 0x10c));
            Assert.Equal(0u, Get(manager, 0x110));
            rejected++;
        };
        Put(source, 4, 0);
        // '?','F'는 서명 실패이며 FE 판정도 0이다
        Assert.False(RawDataManagerImage.ReadTemplateFileSignature(manager, source, hooks));
        Assert.Equal(1, rejected);
        Put(source, 0, 0);
        byte[] before = [.. manager];
        // NULL 입력은 이름/타입 수/제거 경계를 호출하지 않는다
        Assert.False(RawDataManagerImage.ReadTemplateFileSignature(manager, source, new TemplateFileSignatureHooks()));
        Assert.Equal(before, manager);
        Put(source, 0, 2);
        Put(source, 4, 2);
        Assert.True(Throws(() => RawDataManagerImage.ReadTemplateFileSignature(manager, source, hooks)));
        // 첫 바이트 이후의 물리 범위 거부도 앞선 쓰기를 되돌리지 않는다
        Assert.Equal(3u, Get(source, 4));
        Assert.Equal(0xfeu, Get(manager, 0x10c));
    }

    /// <summary>파일 없음·서명 거부/예외·빈 이름표의 정상 본문을 같은 부모로 실행한다. 기대값은 원본 제어 순서에서 정한다.</summary>
    [Fact]
    public void ReadParentBranchesAndCleanupCurrent()
    {
        // 서명 경계가 data를 바꿀 때 최초 포인터를 잘못 반납하면 실패해야 한다
        for (int mode = 0; mode < 4; mode++)
        {
            byte[] manager = new byte[12];
            byte[] names = [(byte)'/', (byte)'/', 0];
            int fileNames = 0, reads = 0, signatures = 0, released = 0;
            int currentMode = mode;
            DataManagerFileHooks hooks = new()
            {
                FileName = () =>
                {
                    fileNames++;
                    return "input.fort";
                },
                ReadFile = (name, kind, length, flags) =>
                {
                    Assert.True(name == "input.fort" && kind == 1 && flags == 0 && length.Length == 4 && Get(length, 0) == 0);
                    reads++;
                    Put(length, 0, 2);
                    return currentMode != 0 ? 1u : 0u;
                },
                Signature = source =>
                {
                    signatures++;
                    Assert.True(Get(source, 0) == 1 && Get(source, 4) == 0 && Get(source, 8) == 2 && Get(source, 12) == 2 && Get(source, 16) == 0xffffffffu);
                    Put(source, 4, 2);
                    if (currentMode < 3)
                    {
                        Put(source, 0, 2);
                    }
                    if (currentMode == 2)
                    {
                        throw new InvalidOperationException("명시 서명 경계 실패");
                    }
                    return currentMode == 3;
                },
                ReleaseFile = pointer =>
                {
                    Assert.Equal(currentMode < 3 ? 2u : 1u, pointer);
                    released++;
                },
            };
            hooks.Image.Names = () => names;
            if (mode == 2)
            {
                Assert.True(Throws(() => RawDataManagerImage.ReadFile(manager, hooks)));
            }
            else
            {
                Assert.Equal(mode == 3, RawDataManagerImage.ReadFile(manager, hooks));
            }
            Assert.Equal(1, fileNames);
            Assert.Equal(1, reads);
            Assert.Equal(mode != 0 ? 1 : 0, signatures);
            Assert.Equal(mode != 0 ? 1 : 0, released);
        }
    }

    /// <summary>본문/반납 효과의 예외에서도 같은 파일을 중복 반납하지 않으며 필수 계약은 실제 사용 지점에서 거부한다.</summary>
    [Fact]
    public void BodyAndReleaseFailureReleaseOnlyOnce()
    {
        byte[] manager = new byte[12];
        byte[] names = [(byte)'/', (byte)'/', 0];
        int released = 0;
        // 원본 본문에 도달하는 최소 파일/서명 입력이다
        DataManagerFileHooks hooks = new()
        {
            FileName = () => "input.fort",
            ReadFile = (_, _, length, _) =>
            {
                Put(length, 0, 2);
                return 1;
            },
            Signature = source =>
            {
                Put(source, 4, 2);
                return true;
            },
            ReleaseFile = pointer =>
            {
                Assert.Equal(1u, pointer);
                released++;
            },
        };
        hooks.Image.Names = () => throw new InvalidOperationException("명시 이름 표 실패");
        Assert.True(Throws(() => RawDataManagerImage.ReadFile(manager, hooks)));
        Assert.Equal(1, released);
        hooks.Image.Names = () => names;
        hooks.ReleaseFile = pointer =>
        {
            Assert.Equal(1u, pointer);
            released++;
            throw new InvalidOperationException("명시 반납 경계 실패");
        };
        Assert.True(Throws(() => RawDataManagerImage.ReadFile(manager, hooks)));
        Assert.Equal(2, released);
        Assert.True(Throws(() => RawDataManagerImage.ReadFile(manager, new DataManagerFileHooks())));
    }
}
