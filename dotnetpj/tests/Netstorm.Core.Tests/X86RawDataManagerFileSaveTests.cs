using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// DataManager 저장 부모(144행), 신규 초기화(64행)와 Template 섹션 이름표(12행), 덤프 정적 계약을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DataManagerFileSaveTests.cpp, DataManagerInitializeTests.cpp, DataManagerDumpTests.cpp와 같다 (0.128, 2026-11-01).
/// </summary>
public sealed class X86RawDataManagerFileSaveTests
{
    /// <summary>독립 관측에만 사용한 불투명 주소 토큰이다.</summary>
    private const uint SaveVtable = 0x1100100, SaveImage = 0x1101000, SaveAlternate = 0x1102000;

    /// <summary>저장 검사의 manager +12 파일명 쓰기: 원본 strcpy와 같이 새 이름과 NUL만 쓰고 남은 manager 바이트는 보존한다.</summary>
    /// <param name="manager">manager 64바이트.</param>
    /// <param name="text">새 파일명.</param>
    private static void WriteName(Span<byte> manager, string text)
    {
        Latin1(text).CopyTo(manager[12..]);
        manager[12 + text.Length] = 0;
    }

    /// <summary>실제 가상 getter가 반환하는 manager +12의 현재 원문이다.</summary>
    /// <param name="manager">manager 바이트.</param>
    private static string ReadName(Span<byte> manager) => TextAt(manager[12..]);

    /// <summary>입력만 재생하여 실제 전체 manager/MarkStream/전역/파일 장치 순서를 원본 144행 결과와 비교한다.</summary>
    [Fact]
    public void SaveMatchesNative144Rows()
    {
        string[][] rows = X86Fixture.Read("datamanager-file-save");
        Assert.Equal(144, rows.Length);
        List<string> problems = [];
        // 각 입력은 원본과 같은 초기 전역과 manager에서 독립 실행한다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(4, input.Length);
            uint readOnly = Number(input[0]);
            uint owner = Number(input[1]);
            uint hidden = Number(input[2]);
            uint variant = Number(input[3]);
            byte[] hiddenPlayers = new byte[9];
            hiddenPlayers[owner] = (byte)hidden;
            byte[] manager = new byte[64];
            Array.Fill(manager, (byte)0xa7);
            Put(manager, 0, SaveVtable);
            WriteName(manager, "unit.fort");
            byte[] snapshot = new byte[20];
            Memory<byte> active = default;
            int names = 0;
            StringBuilder trace = new();

            // 외부 경계 요청만 원본과 같은 순서로 기록한다
            void Event(string text)
            {
                if (trace.Length != 0)
                {
                    trace.Append(';');
                }
                trace.Append(text);
            }
            // 로컬 MarkStream이 살아 있는 각 경계에서 현재 원문을 보관한다
            void Capture() => active.Span.CopyTo(snapshot);

            DataManagerFileSaveHooks hooks = new()
            {
                // 이미지 생성은 저장 억제 관문보다 먼저 실제 현재 전역을 바꿀 수 있다
                Image = (source, stream) =>
                {
                    active = stream;
                    Event("S:" + Hex(stream.Span));
                    Put(stream.Span, 0, variant == 1 ? 0u : SaveImage);
                    Put(stream.Span, 4, 0);
                    Put(stream.Span, 8, variant == 7 ? 0xffffffffu : 17u);
                    Put(stream.Span, 12, 0x8000);
                    Put(stream.Span, 16, 0x12345678);
                    if (variant == 2)
                    {
                        readOnly = readOnly == 0 ? 1u : 0u;
                    }
                    if (variant == 3)
                    {
                        owner = (owner + 1) % 9;
                        hiddenPlayers[owner] = (byte)(hidden == 0 ? 1 : 0);
                    }
                    Capture();
                },
                // 둘째 이름 조회가 이미지 길이를 바꿔도 이미 보관한 쓰기 인자는 바뀌지 않는다
                FileName = source =>
                {
                    names++;
                    Event("N:" + ReadName(source));
                    if (variant == 6 && names == 2)
                    {
                        Put(active.Span, 0, SaveAlternate);
                        Put(active.Span, 8, 55);
                    }
                    Capture();
                    return ReadName(source);
                },
                // 권한 효과 후 이름/길이만 다시 읽고 저장/숨김 관문과 이미지 포인터는 재조회하지 않는다
                Permissions = (name, mode) =>
                {
                    Event("C:" + name + ':' + mode);
                    if (variant == 4)
                    {
                        WriteName(manager, "changed.fort");
                        readOnly = 1;
                        owner = 4;
                        Array.Fill(hiddenPlayers, (byte)0xff);
                    }
                    if (variant == 5)
                    {
                        Put(active.Span, 0, SaveAlternate);
                        Put(active.Span, 8, 33);
                    }
                    Capture();
                },
                Write = (name, pointer, length) =>
                {
                    Event("W:" + name + ':' + pointer + ':' + length);
                    Capture();
                },
                Release = pointer =>
                {
                    Event("F:" + pointer);
                    Capture();
                },
            };
            DataManagerFileSaveState state = new() { ReadOnlyForts = () => readOnly, LocalOwner = () => owner, HiddenPlayers = () => hiddenPlayers };
            RawDataManagerFileSave.Save(manager, state, hooks);
            if (Hex(manager) != row[1] || Hex(snapshot) != row[2] || readOnly != Number(row[3]) || owner != Number(row[4]) || Hex(hiddenPlayers) != row[5] || trace.ToString() != row[6])
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "DataManager 파일 저장 144행");
    }

    /// <summary>억제 경로는 이미지 생성/반납을 생략하지 않으며 호스트 실패에서도 중복 반납하지 않는다.</summary>
    [Fact]
    public void SaveSelectedBoundariesAndExceptions()
    {
        byte[] manager = new byte[64];
        byte[] hidden = new byte[9];
        uint readOnly = 1;
        uint owner = 999;
        int releases = 0;
        DataManagerFileSaveHooks hooks = new();
        DataManagerFileSaveState state = new() { ReadOnlyForts = () => readOnly, LocalOwner = () => owner, HiddenPlayers = () => hidden };
        Assert.True(Throws(() => RawDataManagerFileSave.Save(manager, state, hooks)));
        hooks.Image = (_, stream) => Put(stream.Span, 0, SaveImage);
        hooks.Release = pointer =>
        {
            Assert.Equal(SaveImage, pointer);
            releases++;
        };
        RawDataManagerFileSave.Save(manager, state, hooks);
        Assert.Equal(1, releases);
        readOnly = 0;
        Assert.True(Throws(() => RawDataManagerFileSave.Save(manager, state, hooks)));
        Assert.Equal(2, releases);
        owner = 1;
        hooks.FileName = _ => "working.fort";
        hooks.Permissions = (_, mode) => Assert.Equal(0x180u, mode);
        hooks.Write = (_, _, _) => throw new InvalidOperationException("호스트 파일 쓰기 실패 입력");
        Assert.True(Throws(() => RawDataManagerFileSave.Save(manager, state, hooks)));
        Assert.Equal(3, releases);
        hooks.Image = (_, stream) =>
        {
            Put(stream.Span, 0, SaveImage);
            throw new InvalidOperationException("호스트 이미지 생성 중 실패 입력");
        };
        Assert.True(Throws(() => RawDataManagerFileSave.Save(manager, state, hooks)));
        Assert.Equal(4, releases);
        hooks.Release = _ =>
        {
            releases++;
            throw new InvalidOperationException("호스트 반납 실패 입력");
        };
        Assert.True(Throws(() => RawDataManagerFileSave.Save(manager, state, hooks)));
        Assert.Equal(5, releases);
    }

    /// <summary>초기화 검사에서 가상 입력 주소와 같은 불투명 번호다.</summary>
    private const uint InitManager = 0x10001000, InitTable = 0x10002000, InitAlternate = 0x10003000;

    /// <summary>Native에서 실제 RET만 실행하는 004437f0은 C++/C# 효과가 없으므로 trace의 그 한 항목만 제외한다.</summary>
    /// <param name="text">fixture trace 칸.</param>
    private static string WithoutNoop(string text) => string.Join(';', text.Split(';').Where(entry => entry != "baseNoop"));

    /// <summary>명시 formatter 출력과 table/manager 변이를 동결 Native와 동일한 순서로 제공하는 재생 장치다(cpppj <c>InitializeReplay</c>).</summary>
    private sealed class InitializeReplay
    {
        /// <summary>Native의 고정 this와 root/current 전체 12바이트다.</summary>
        public byte[] Manager { get; } = new byte[12];

        /// <summary>미정 영역까지 A5로 명시한 검사 입력의 이름표이며 제품 기본 초기값이 아니다.</summary>
        public byte[] Table { get; } = new byte[512];

        /// <summary>교체 후보 이름표다.</summary>
        public byte[] Alternate { get; } = new byte[512];

        /// <summary>서식 장치가 반환하는 정의된 Terr00/NUL 범위다.</summary>
        private readonly byte[] _formatted = new byte[7];

        /// <summary>실제 전역(Client 이름표 token)이다. ref로 넘기므로 필드다.</summary>
        public uint Current;

        /// <summary>외부 장치 입력(변이 종류)이다.</summary>
        private readonly uint _mutation;

        /// <summary>외부 장치 입력(할당 결과 토큰 여부)이다.</summary>
        private readonly uint _allocation;

        /// <summary>실제 Create 호출 횟수다.</summary>
        private uint _creates;

        /// <summary>마지막으로 고정한 표 token이다.</summary>
        private uint _selected;

        /// <summary>현재 표의 이름 번호다.</summary>
        private uint _index;

        /// <summary>하위 호출의 고정 포인터와 현재 문자열을 관측한 순서다.</summary>
        public StringBuilder Trace { get; } = new();

        /// <summary>Native와 같은 kind/root/외부 장치 입력을 준비한다.</summary>
        /// <param name="kind">표 종류(0 없음, 1 슬래시 문자열, 2 변환된 표, 3 빈 표).</param>
        /// <param name="root">manager +4 root 값.</param>
        /// <param name="change">변이 종류.</param>
        /// <param name="allocated">신규 할당 결과 여부.</param>
        public InitializeReplay(uint kind, uint root, uint change, uint allocated = 1)
        {
            _mutation = change;
            _allocation = allocated;
            Table.AsSpan().Fill(0xa5);
            Alternate.AsSpan().Fill(0xa5);
            Put(Manager, 0, 0x514690);
            Put(Manager, 4, root);
            Put(Manager, 8, 0xa5a5a5a5);
            byte[] alt = Latin1("/Alt/Last//");
            alt.CopyTo(Alternate, 0);
            Alternate[alt.Length] = 0;
            if (kind == 1)
            {
                byte[] raw = Latin1("/One/Two//");
                raw.CopyTo(Table, 0);
                Table[raw.Length] = 0;
            }
            if (kind == 2)
            {
                byte[] raw = [0, (byte)'O', (byte)'n', (byte)'e', 0, (byte)'T', (byte)'w', (byte)'o', 0, 0, 0xff];
                raw.CopyTo(Table, 0);
            }
            if (kind == 3)
            {
                Table[0] = (byte)'/';
                Table[1] = (byte)'/';
                Table[2] = 0;
            }
            Current = kind != 0 ? InitTable : 0;
        }

        /// <summary>trace에 이미 캡처한 인자만 붙인다.</summary>
        /// <param name="entry">관측 항목.</param>
        private void Event(string entry)
        {
            if (Trace.Length != 0)
            {
                Trace.Append(';');
            }
            Trace.Append(entry);
        }

        /// <summary>읽기 조회 자체는 전역을 변경하지 않고 같은 물리 표를 반환한다.</summary>
        /// <param name="token">표 token.</param>
        private Span<byte> Buffer(uint token)
        {
            if (token == InitTable)
            {
                return Table;
            }
            if (token == InitAlternate)
            {
                return Alternate;
            }
            throw new ArgumentOutOfRangeException(null, "Native이름표NULL접근");
        }

        /// <summary>실제 getter 진입과 선택된 할당/서식을 기록하며 부모 모듈을 그대로 실행한다.</summary>
        public Span<byte> Names()
        {
            Event("names:" + Current);
            TemplateSectionNamesHooks hooks = new()
            {
                Allocate = size =>
                {
                    Event("allocate:" + size);
                    return _allocation != 0 ? InitTable : 0u;
                },
                Buffer = Buffer,
                Format = number =>
                {
                    Event("format:" + number);
                    _formatted[0] = (byte)'T';
                    _formatted[1] = (byte)'e';
                    _formatted[2] = (byte)'r';
                    _formatted[3] = (byte)'r';
                    _formatted[4] = (byte)('0' + number / 10);
                    _formatted[5] = (byte)('0' + number % 10);
                    _formatted[6] = 0;
                    return _formatted;
                },
            };
            return Buffer(RawTemplateSectionNames.Get(ref Current, hooks));
        }

        /// <summary>한 initialize 호출의 표 포인터/인덱스를 고정하되 전체 creates는 반복 사이에 보존한다.</summary>
        public void Initialize()
        {
            DataManagerInitializeHooks hooks = new()
            {
                Assertion = kind =>
                {
                    Event("assert:" + (uint)kind);
                    if (_mutation == 1 && kind == DataManagerInitializeAssertion.RootNotEmpty)
                    {
                        Current = InitAlternate;
                    }
                },
                Names = () =>
                {
                    Span<byte> bytes = Names();
                    _selected = Current;
                    _index = 0;
                    return bytes;
                },
                Create = name =>
                {
                    Event("at:" + _selected + ':' + _index++);
                    int baseLength = Buffer(_selected).Length;
                    uint address = _selected + (uint)(baseLength - name.Length);
                    string text = TerminatedText(name);
                    Event("create:" + InitManager + ':' + address + ':' + Hex(Latin1(text)));
                    _creates++;
                    Put(Manager, 4, 0x10004000);
                    Put(Manager, 8, 0x10004000 + _creates * 60);
                    if (_creates == 1 && _mutation == 2)
                    {
                        Current = InitAlternate;
                    }
                    if (_creates == 1 && _mutation == 3)
                    {
                        // 같은 표의 미래 항목만 명시 외부 입력으로 바꾼다
                        string[] before = ["State\0", "Two\0"];
                        string[] after = ["Other\0", "Six\0"];
                        // Native와 같은 기지 문자열만 바이트 검색하며 길이를 바꾸지 않는다
                        for (int at = 0; at < before.Length; at++)
                        {
                            int found = Table.AsSpan().IndexOf(Latin1(before[at]));
                            if (found >= 0)
                            {
                                Latin1(after[at]).CopyTo(Table, found);
                            }
                        }
                    }
                },
            };
            RawDataManagerInitialize.Initialize(Manager, hooks);
            // 마지막 빈 항목은 Create가 없으므로 반환 위치에서 같은 무효과 관측을 기록한다
            Event("at:" + _selected + ':' + _index);
        }

        /// <summary>같은 물리 이름 포인터의 현재 NUL까지 읽는다. 부모에서 선행 길이를 계산하지 않는다.</summary>
        /// <param name="name">이름 시작부터 표 끝까지의 뷰.</param>
        private static string TerminatedText(ReadOnlySpan<byte> name)
        {
            int end = name.IndexOf((byte)0);
            Assert.True(end >= 0, "초기화 검사 실제 NUL 범위");
            return Encoding.Latin1.GetString(name[..end]);
        }
    }

    /// <summary>초기화 64행의 전체 manager/두 표/순서를 같은 외부 입력으로 재생한다.</summary>
    [Fact]
    public void InitializeMatchesNative64Rows()
    {
        string[][] rows = X86Fixture.Read("data-manager-initialize");
        Assert.Equal(64, rows.Length);
        List<string> problems = [];
        // 각 행의 repeat도 원래 같은 Client/manager에서 계속한다
        foreach (string[] row in rows)
        {
            Assert.Equal(11, row.Length);
            InitializeReplay scene = new(Number(row[0]), Number(row[1]), Number(row[3]));
            for (uint call = 0; call < Number(row[2]); call++)
            {
                scene.Initialize();
            }
            if (row[4] != "1" || row[5] != "0x0" || scene.Current != Number(row[6]) || Hex(scene.Manager) != row[7] || Hex(scene.Table) != row[8]
                || Hex(scene.Alternate) != row[9] || scene.Trace.ToString() != WithoutNoop(row[10]))
            {
                problems.Add(string.Join('\t', row.Take(4)));
            }
        }
        NoProblems(problems, "DataManager 초기화 64행");
    }

    /// <summary>getter 12행의 최초 512 할당/재사용/NULL 공개 후 실패를 전체 바이트에 대조한다.</summary>
    [Fact]
    public void SectionNamesMatchNative12Rows()
    {
        string[][] rows = X86Fixture.Read("template-section-names");
        Assert.Equal(12, rows.Length);
        List<string> problems = [];
        // 기존 표이면 할당 입력 0에서도 실제 할당을 선택하지 않는다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            InitializeReplay scene = new(Number(row[0]), 0, 0, Number(row[1]));
            bool completed = true;
            try
            {
                for (uint call = 0; call < Number(row[2]); call++)
                {
                    scene.Names();
                }
            }
            catch (ArgumentOutOfRangeException)
            {
                completed = false;
            }
            if (completed != (row[3] == "1") || row[4] != (completed ? "0x0" : "0x4bcced") || scene.Current != Number(row[5]) || Hex(scene.Manager) != row[6]
                || Hex(scene.Table) != row[7] || Hex(scene.Alternate) != row[8] || scene.Trace.ToString() != WithoutNoop(row[9]))
            {
                problems.Add(string.Join('\t', row.Take(4)));
            }
        }
        NoProblems(problems, "Template 섹션 이름표 12행");
    }

    /// <summary>덤프 검사 장면: 실제 276바이트와 독립 노드/이름표/trace 관측의 수명을 같이 소유한다(cpppj <c>DumpScene</c>).</summary>
    private sealed class DumpScene
    {
        /// <summary>manager 양쪽 4바이트를 포함하며 current 밖의 쓰기를 탐지한다.</summary>
        public byte[] Bytes { get; } = new byte[284];

        /// <summary>원본 data/pos/end/alloc/name/next의 같은 물리다.</summary>
        public byte[][] Nodes { get; } = [new byte[60], new byte[60], new byte[60]];

        /// <summary>원본 SNART 제자리 변환과 미래 이름 변경의 현재 버퍼다.</summary>
        public byte[] NameTable { get; private set; } = [];

        /// <summary>가상 호출/비교/trace 순서다.</summary>
        public List<string> Events { get; } = [];

        /// <summary>완성 출력이다.</summary>
        public List<string> Lines { get; } = [];

        /// <summary>가상 호출과 출력 경계다.</summary>
        public DataManagerDumpHooks Hooks { get; } = new();

        /// <summary>검사 입력의 이름/끝 길이를 설치한다.</summary>
        public DumpScene()
        {
            Bytes.AsSpan().Fill(0xa5);
            Put(Manager(), 4, 1);
            Put(Manager(), 8, 99);
            // 세 독립 노드는 입력 순서대로 다음 링크를 가지며 pos/alloc은 끝 합계에 영향을 주지 않아야 한다
            for (uint index = 0; index < 3; index++)
            {
                Put(Nodes[index], 0, 0xdead0000u + index);
                Put(Nodes[index], 4, 17 + index);
                Put(Nodes[index], 8, index + 1);
                Put(Nodes[index], 12, 100 + index);
                Put(Nodes[index], 56, index < 2 ? index + 2 : 0);
            }
            SetNodeName(1, "One");
            SetNodeName(2, "Two");
            SetNodeName(3, "Three");
            SetNames("/One/Two/Three//");
            Hooks.FileName = manager =>
            {
                Assert.Equal(276, manager.Length);
                Events.Add("file");
                return "NONAME";
            };
            Hooks.Names = manager =>
            {
                Assert.Equal(276, manager.Length);
                Events.Add("names");
                return NameTable;
            };
            Hooks.Selection.Stream = token =>
            {
                if (token == 0 || token > Nodes.Length)
                {
                    throw new ArgumentOutOfRangeException(null, "덤프검사의실제노드번호");
                }
                return Nodes[token - 1];
            };
            Hooks.Selection.Compare = (first, second) =>
            {
                Events.Add("compare:" + first + ":" + second);
                return RawDataManagerSelection.CompareSectionNames(first, second);
            };
            Hooks.Trace = (level, text) =>
            {
                Assert.Equal(29, level);
                Events.Add("trace");
                Lines.Add(text);
            };
            Hooks.SeparatorAssertion = () => throw new InvalidOperationException("정상덤프이름표에없는separator진단");
        }

        /// <summary>같은 manager의 보호 구간을 제외한 276바이트를 동기 호출 동안 빌린다.</summary>
        public Span<byte> Manager() => Bytes.AsSpan(4, 276);

        /// <summary>실제 40바이트 이름 공간의 첫 NUL까지 복사하는 검사 입력 작성기다.</summary>
        /// <param name="token">노드 번호(1부터).</param>
        /// <param name="name">쓸 이름.</param>
        public void SetNodeName(int token, string name)
        {
            Nodes[token - 1].AsSpan(16, 40).Clear();
            Latin1(name).CopyTo(Nodes[token - 1], 16);
        }

        /// <summary>mutable SNART의 최초 슬래시 문자열과 마지막 NUL을 함께 소유한다.</summary>
        /// <param name="text">슬래시로 구분한 이름 목록.</param>
        public void SetNames(string text) => NameTable = [.. Latin1(text), 0];
    }

    /// <summary>선택된 명시 예외 메시지만 문자열로 반환하며 제품의 부분 current/이름표 상태는 되돌리지 않는다.</summary>
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

    /// <summary>가상 파일명/첫 로그/가상 이름표 후 실제 검색하며 pos와 양쪽 보호 바이트를 보존한다.</summary>
    [Fact]
    public void DumpNormalOrderAndPhysicalCurrent()
    {
        DumpScene scene = new();
        byte[] before = [.. scene.Bytes];
        byte[][] nodes = scene.Nodes.Select(node => node.ToArray()).ToArray();
        RawDataManagerDump.Dump(scene.Manager(), "Stripped Image:", scene.Hooks);
        Assert.Equal(new[] { "File NONAME: Stripped Image:\n", "0006 bytes total.\n" }, scene.Lines);
        Assert.True(scene.Events[0] == "file" && scene.Events[1] == "trace" && scene.Events[2] == "names" && scene.Events[^1] == "trace");
        Assert.Equal(3u, Get(scene.Manager(), 8));
        Assert.True(scene.Nodes.Zip(nodes).All(pair => pair.First.AsSpan().SequenceEqual(pair.Second)));
        Assert.True(scene.NameTable[0] == 0 && scene.NameTable[4] == 0 && scene.NameTable[^1] == 0xff);
        // 검색이 쓰는 current 4바이트 이외의 실제 manager와 양쪽 보호 바이트를 비교한다
        for (int index = 0; index < scene.Bytes.Length; index++)
        {
            if (index < 12 || index >= 16)
            {
                Assert.Equal(before[index], scene.Bytes[index]);
            }
        }
    }

    /// <summary>비교 뒤 current와 누락 로그 뒤 미래 이름표/길이의 같은 현재 값을 읽는다.</summary>
    [Fact]
    public void DumpCurrentAndFutureNamesFollowCallbackChanges()
    {
        DumpScene scene = new();
        scene.SetNames("/One/Missing/Three//");
        int comparisons = 0;
        int namesCalls = 0;
        scene.Hooks.FileName = manager =>
        {
            Put(manager, 8, 88);
            scene.Events.Add("file");
            return "changed.fort\0ignored";
        };
        scene.Hooks.Names = _ =>
        {
            namesCalls++;
            Assert.Single(scene.Lines);
            return scene.NameTable;
        };
        scene.Hooks.Selection.Compare = (first, second) =>
        {
            if (comparisons++ == 0)
            {
                Assert.True(first == "One" && second == "One");
                Put(scene.Manager(), 8, 3);
                return 0;
            }
            return RawDataManagerSelection.CompareSectionNames(first, second);
        };
        scene.Hooks.Trace = (level, text) =>
        {
            Assert.Equal(29, level);
            scene.Lines.Add(text);
            if (scene.Lines.Count == 1)
            {
                Assert.Equal(88u, Get(scene.Manager(), 8));
            }
            if (text == "ERROR: Missing section Missing\n")
            {
                Assert.Equal(0u, Get(scene.Manager(), 8));
                Span<byte> future = RawSnart.SelectName(scene.NameTable, 2, null);
                Latin1("Other").AsSpan().CopyTo(future);
                scene.SetNodeName(2, "Other");
                Put(scene.Nodes[1], 8, 27);
            }
        };
        RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks);
        Assert.Equal(1, namesCalls);
        Assert.Equal(new[] { "File changed.fort: \n", "ERROR: Missing section Missing\n", "0030 bytes total.\n" }, scene.Lines);
        Assert.Equal(2u, Get(scene.Manager(), 8));
    }

    /// <summary>end 합계는 signed 연산 오버플로 없이 원문 DWORD로 감기고 signed %04d로 표시한다.</summary>
    [Fact]
    public void DumpDwordWrapAndSignedMinimumWidth()
    {
        DumpScene scene = new();
        scene.SetNames("/One/Two//");
        Put(scene.Nodes[0], 8, 0xffffffffu);
        Put(scene.Nodes[1], 8, 2);
        RawDataManagerDump.Dump(scene.Manager(), "\0ignored", scene.Hooks);
        Assert.True(scene.Lines[^1] == "0001 bytes total.\n" && Get(scene.Manager(), 8) == 2);
        scene.Lines.Clear();
        scene.SetNames("/One//");
        Put(scene.Nodes[0], 8, 0x80000000u);
        RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks);
        Assert.Equal("-2147483648 bytes total.\n", scene.Lines[^1]);
        scene.Lines.Clear();
        Put(scene.Nodes[0], 8, 0xffffffffu);
        RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks);
        Assert.Equal("-001 bytes total.\n", scene.Lines[^1]);
    }

    /// <summary>separator/비교/로그 예외는 이미 쓴 current와 SNART 변환을 보존한다.</summary>
    [Fact]
    public void DumpSelectedFailurePreservesPartialState()
    {
        DumpScene scene = new();
        scene.SetNames("XOne//");
        scene.Hooks.SeparatorAssertion = () => throw new InvalidOperationException("명시separator중단");
        Assert.Equal("명시separator중단", Failure(() => RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks)));
        Assert.True(Get(scene.Manager(), 8) == 99 && scene.NameTable[0] == (byte)'X' && scene.Lines.Count == 1);
        scene.SetNames("/One//");
        scene.Hooks.Selection.Compare = (_, _) =>
        {
            Put(scene.Manager(), 8, 2);
            throw new InvalidOperationException("명시비교중단");
        };
        Assert.Equal("명시비교중단", Failure(() => RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks)));
        Assert.True(Get(scene.Manager(), 8) == 2 && scene.NameTable[0] == 0);
        scene.Hooks.Selection.Compare = null;
        scene.SetNames("/Missing//");
        scene.Hooks.Trace = (_, text) =>
        {
            if (text.StartsWith("ERROR:", StringComparison.Ordinal))
            {
                Put(scene.Manager(), 8, 3);
                throw new InvalidOperationException("명시trace중단");
            }
        };
        Assert.Equal("명시trace중단", Failure(() => RawDataManagerDump.Dump(scene.Manager(), null, scene.Hooks)));
        Assert.Equal(3u, Get(scene.Manager(), 8));
    }
}
