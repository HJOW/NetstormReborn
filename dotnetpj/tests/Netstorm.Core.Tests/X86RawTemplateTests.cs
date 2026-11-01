using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// Template 9칸 기록의 수명·적재(<c>RawTemplateLifetime</c>, <c>RawTemplateLoad</c>), 현재 Fort 이름 부모(<c>RawFortCurrentName</c>),
/// Fort 이미지 직렬화(<c>RawFortImage</c>), Player State 확장·초기화(<c>RawPlayerTemplateState</c>)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/TemplateLifetimeTests.cpp(240행), TemplateLoadTests.cpp, FortCurrentNameTests.cpp(876관측), FortImagePacketsTests.cpp,
/// PlayerStateExpandTests.cpp(99행), PlayerStateInitializationTests.cpp(2560행)와 같다 (0.128, 2026-11-01).
/// Fort 이미지의 송신(S)·예약(F) 행과 실제 Storage가 필요한 검사는 해당 계층을 옮길 때 잇는다.
/// </summary>
public sealed class X86RawTemplateTests
{
    /// <summary>전역/단일/초기화와 각 소멸/free 직전 전체 헤더를 독립 240행에 대조한다.</summary>
    [Fact]
    public void LifetimeMatchesNative240Rows()
    {
        string[][] rows = X86Fixture.Read("template-lifetime-v2");
        Assert.Equal(240, rows.Length);
        List<string> problems = [];
        // 각 행은 같은 전체 196바이트 입력과 현재 효과 변이를 사용한다
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string[] input = row[0][1..^1].Split(',');
            Assert.Equal(6, input.Length);
            byte[] bytes = Decode(row[1]);
            uint selected = Number(input[4]);
            uint variant = Number(input[5]);
            StringBuilder trace = new();

            // 헤더 180바이트의 현재 뷰다
            Span<byte> Headers() => bytes.AsSpan(8, 180);
            // 실제 경계 직전 상태를 관측한다
            void Record(char kind, uint handle)
            {
                if (trace.Length != 0)
                {
                    trace.Append(';');
                }
                trace.Append(kind).Append(',').Append(handle).Append(',').Append(Hex(bytes));
            }
            TemplateLifetimeHooks effects = new()
            {
                // 고정 힙 입력 주소는 원문의 외부 불투명 handle이다
                Destroy = handle =>
                {
                    Record('D', handle);
                    uint slot = (handle - 0x12100000u) / 0x1000u;
                    if (variant == 1)
                    {
                        Put(Headers(), (int)slot * 20 + 4, 0x87654321);
                        if (slot < 8)
                        {
                            Put(Headers(), (int)(slot + 1) * 20 + 4, 1);
                        }
                    }
                },
                Release = handle =>
                {
                    Record('F', handle);
                    uint slot = (handle - 0x12100000u) / 0x1000u;
                    if (variant == 2)
                    {
                        Put(Headers(), (int)slot * 20, 0xdead0000);
                        Put(Headers(), (int)slot * 20 + 16, 0x76543210);
                        if (slot < 8)
                        {
                            Put(Headers(), (int)(slot + 1) * 20, 0);
                        }
                    }
                },
            };
            if (input[0] == "\"A\"")
            {
                RawTemplateLifetime.ClearHeaders(Headers(), effects);
            }
            else if (input[0] == "\"S\"")
            {
                RawTemplateLifetime.ReleaseHeader(Headers().Slice((int)selected * 20, 20), effects);
            }
            else
            {
                RawTemplateLifetime.InitializeHeader(Headers().Slice((int)selected * 20, 20));
            }
            if (Hex(bytes) != row[2] || (trace.Length == 0 ? "-" : trace.ToString()) != row[3])
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "Template 수명 240행");
    }

    /// <summary>생성자가 쓰지 않는 이름 뒤 공간과 양쪽 경계는 별도 입력 값 그대로 남아야 한다.</summary>
    [Fact]
    public void LoadConstructorAndBorrowedFileName()
    {
        byte[] bytes = new byte[284];
        Array.Fill(bytes, (byte)0xa7);
        Span<byte> manager = bytes.AsSpan(4, 276);
        byte[] global = [1, 2, 3, 4];
        RawTemplateLoad.ConstructManager(manager, "MiXeD.fort", global);
        Assert.Equal(0x514690u, Get(manager, 0));
        Assert.Equal(0u, Get(manager, 4));
        Assert.Equal(0u, Get(manager, 8));
        Assert.Equal(0u, Get(manager, 0x10c));
        Assert.Equal(0xffffffffu, Get(manager, 0x110));
        Assert.All(global, value => Assert.Equal(0, value));
        Assert.Equal("MiXeD.fort", RawTemplateLoad.FileName(manager));
        Assert.Equal(0, manager[22]);
        Assert.Equal(0xa7, manager[23]);
        Assert.Equal(0xa7, manager[267]);
        Assert.Equal(0xa7a7a7a7u, Get(bytes, 0));
        Assert.Equal(0xa7a7a7a7u, Get(bytes, 280));
        // 이름이 바뀌면 같은 바이트를 다시 읽은 결과도 바뀐다(C++ string_view 별칭 대신 매번 manager 배열에서 읽는다)
        manager[12] = (byte)'m';
        Assert.Equal("miXeD.fort", RawTemplateLoad.FileName(manager));
        // 원본 C 문자열처럼 첫 NUL까지만 쓴다
        RawTemplateLoad.ConstructManager(manager, "x\0ignored", global);
        Assert.Equal("x", RawTemplateLoad.FileName(manager));
        Assert.Equal((byte)'X', manager[14]);
        string maximum = new('q', 255);
        RawTemplateLoad.ConstructManager(manager, maximum, global);
        Assert.Equal(maximum, RawTemplateLoad.FileName(manager));
        Assert.Equal(0, manager[267]);
        byte[] copy = [.. bytes];
        Assert.True(Throws(() => RawTemplateLoad.ConstructManager(copy.AsSpan(4, 276), new string('q', 256), global)));
        Assert.True(Throws(() => RawTemplateLoad.FileName(copy.AsSpan(4, 275))));
        manager[267] = (byte)'q';
        byte[] unterminated = [.. bytes];
        Assert.True(Throws(() => RawTemplateLoad.FileName(unterminated.AsSpan(4, 276))));
    }

    /// <summary>이미 파일을 가진 헤더는 같음/다름 어느 경우에도 할당/읽기/해제를 하지 않는다.</summary>
    [Fact]
    public void LoadExistingFileKeepsAllHeaderFields()
    {
        byte[] header = new byte[20];
        Array.Fill(header, (byte)0xa7);
        Put(header, 0, 0x1234);
        byte[] before = [.. header];
        int names = 0, compares = 0, diagnostics = 0;
        TemplateLoadHooks hooks = new()
        {
            FileName = manager =>
            {
                Assert.Equal(0x1234u, manager);
                names++;
                return "Keep.fort";
            },
            // CRT 비교의 0/비영 결과만 외부 효과 입력으로 제공한다
            SameName = (first, second) =>
            {
                Assert.True(first == "Keep.fort" && second == "KEEP.FORT");
                return ++compares == 1;
            },
            WrongName = () => diagnostics++,
        };
        RawTemplateLoad.Load(header, "KEEP.FORT", hooks);
        Assert.Equal(before, header);
        Assert.True(names == 1 && compares == 1 && diagnostics == 0);
        RawTemplateLoad.Load(header, "KEEP.FORT", hooks);
        Assert.Equal(before, header);
        Assert.True(names == 2 && compares == 2 && diagnostics == 1);
    }

    /// <summary>readonly 사전 설정은 파일 판정을 건너뛰되 1로 정규화하고 성공 시 현재 슬롯만 제목을 갱신한다.</summary>
    [Fact]
    public void LoadNewReadOnlyAndCurrentTitleBranches()
    {
        // 네 입력은 preset 비영/영과 판정 true/false 및 현재 슬롯 true/false를 독립해서 고른다
        for (uint variant = 0; variant < 4; variant++)
        {
            uint current = variant;
            byte[] header = new byte[20];
            Put(header, 4, 0xaabbccdd);
            Put(header, 8, current == 3 ? 0x87654321u : 0u);
            Put(header, 12, 0x7391);
            Put(header, 16, 0x12345678);
            StringBuilder trace = new();
            TemplateLoadHooks hooks = new()
            {
                Allocate = size =>
                {
                    Assert.True(size == 276 && Get(header, 4) == 0 && Get(header, 0) == 0);
                    trace.Append('A');
                    return 0x101;
                },
                // 생성자 EAX가 헤더에 설치되는 것을 구별한다
                Construct = (manager, name) =>
                {
                    Assert.True(manager == 0x101 && name == "new.fort" && Get(header, 0) == 0);
                    trace.Append('C');
                    return 0x202;
                },
                FileName = manager =>
                {
                    Assert.True(manager == 0x202 && Get(header, 0) == 0x202);
                    trace.Append('N');
                    return "new.fort";
                },
                ReadOnly = name =>
                {
                    Assert.True(name == "new.fort" && Get(header, 16) == 0x12345678);
                    trace.Append('R');
                    return current == 1;
                },
                ReadFile = manager =>
                {
                    Assert.True(manager == 0x202 && Get(header, 16) == 0 && Get(header, 12) == 0x7391 && Get(header, 8) == (current == 1 || current == 3 ? 1u : 0u));
                    trace.Append('F');
                    return true;
                },
                IsCurrent = () =>
                {
                    trace.Append('I');
                    return current != 2;
                },
                RefreshTitle = () => trace.Append('T'),
            };
            RawTemplateLoad.Load(header, "new.fort", hooks);
            Assert.Equal(current == 3 ? "ACFIT" : current == 2 ? "ACNRFI" : "ACNRFIT", trace.ToString());
            Assert.True(Get(header, 0) == 0x202 && Get(header, 4) == 0 && Get(header, 12) == 0x7391);
        }
    }

    /// <summary>실제 read/소멸 효과가 ptr를 바꾸어도 실패의 free는 소멸 직전 저장 포인터를 쓴다.</summary>
    [Fact]
    public void LoadFailureRereadsPointerAndPreservesOtherFields()
    {
        byte[] header = new byte[20];
        Put(header, 8, 4);
        Put(header, 12, 0x7391);
        StringBuilder trace = new();
        // readonly preset은 미선택 이름/판정 경계를 요구하지 않는다
        TemplateLoadHooks hooks = new()
        {
            Allocate = _ => 11,
            Construct = (pointer, _) => pointer,
            ReadFile = pointer =>
            {
                Assert.Equal(11u, pointer);
                trace.Append('F');
                Put(header, 0, 22);
                Put(header, 4, 1);
                Put(header, 8, 7);
                Put(header, 16, 9);
                return false;
            },
        };
        hooks.Lifetime.Destroy = pointer =>
        {
            Assert.Equal(22u, pointer);
            trace.Append('D');
            Put(header, 0, 33);
            Put(header, 12, 0x8273);
        };
        hooks.Lifetime.Release = pointer =>
        {
            Assert.True(pointer == 22 && Get(header, 0) == 33);
            trace.Append('R');
            Put(header, 16, 10);
        };
        RawTemplateLoad.Load(header, "failed.fort", hooks);
        Assert.Equal("FDR", trace.ToString());
        Assert.Equal(0u, Get(header, 0));
        Assert.Equal(1u, Get(header, 4));
        Assert.Equal(7u, Get(header, 8));
        Assert.Equal(0x8273u, Get(header, 12));
        Assert.Equal(10u, Get(header, 16));
        hooks.ReadFile = _ =>
        {
            Put(header, 0, 0);
            return false;
        };
        trace.Clear();
        RawTemplateLoad.Load(header, "failed.fort", hooks);
        // 실패 뒤 이미 NULL이면 소멸/free를 선택하지 않는다
        Assert.Equal(0, trace.Length);
        Assert.Equal(0u, Get(header, 0));
    }

    /// <summary>read 성공 뒤 ptr가 NULL이면 current/title을 조회하지 않고 NULL 할당도 임의 성공으로 바꾸지 않는다.</summary>
    [Fact]
    public void LoadNullAndUnselectedEffectBoundaries()
    {
        byte[] header = new byte[20];
        Put(header, 8, 1);
        TemplateLoadHooks hooks = new()
        {
            Allocate = _ => 1,
            Construct = (pointer, _) => pointer,
            ReadFile = pointer =>
            {
                Assert.Equal(1u, pointer);
                Put(header, 0, 0);
                return true;
            },
        };
        RawTemplateLoad.Load(header, "input.fort", hooks);
        Assert.Equal(0u, Get(header, 0));
        Array.Clear(header);
        Put(header, 4, 7);
        hooks.Allocate = _ => 0;
        hooks.Construct = null;
        hooks.ReadOnly = _ => false;
        hooks.FileName = pointer =>
        {
            Assert.Equal(0u, pointer);
            throw new InvalidOperationException("명시 NULL manager 가상 호출 거부");
        };
        Assert.True(Throws(() => RawTemplateLoad.Load(header, "input.fort", hooks)));
        Assert.Equal(0u, Get(header, 0));
        Assert.Equal(0u, Get(header, 4));
        Assert.True(Throws(() => RawTemplateLoad.Load(header.AsSpan(0, 19), "input.fort", hooks)));
    }

    /// <summary>원문 가상 manager 주소를 직접 보존하는 명시 토큰이다.</summary>
    private static readonly uint[] ManagerTokens = [0, 0x12001000, 0x12002000];

    /// <summary>원본 하위 callback에서의 명시 중단 표식이다. 제품의 알 수 없는 예외는 이 표식으로 삼키지 않는다.</summary>
    private sealed class NameStop(string cause) : Exception
    {
        /// <summary>중단 경계 이름이다.</summary>
        public string Cause { get; } = cause;
    }

    /// <summary>NUL 뒤도 버리지 않고 모든 바이트를 소문자 hex로 관측한다.</summary>
    /// <param name="bytes">관측할 바이트.</param>
    private static string NameHex(ReadOnlySpan<byte> bytes) => Hex(bytes);

    /// <summary>하위 장치의 NUL 종료 입력만 읽고 미종료를 임의 빈 문자열로 바꾸지 않는다.</summary>
    /// <param name="bytes">NUL로 끝나는 바이트.</param>
    private static ReadOnlySpan<byte> NameText(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(null, "FortName장치NUL범위");
        }
        return bytes[..end];
    }

    /// <summary>하위 PathSpec/이름 초기 입력의 NUL 뒤 바이트까지 복사한다.</summary>
    /// <param name="target">쓸 대상.</param>
    /// <param name="value">쓸 문자열.</param>
    private static void NameInput(Span<byte> target, string value)
    {
        if (value.Length >= target.Length)
        {
            throw new ArgumentOutOfRangeException(null, "FortName장치입력크기");
        }
        Latin1(value).CopyTo(target);
        target[value.Length] = 0;
    }

    /// <summary>기대 열을 읽지 않고 원문과 동일 초기 입력/명시 하위 응답만 공급하는 현재 Fort 이름 검사 장면이다(cpppj <c>NameDevices</c>).</summary>
    private sealed class NameDevices
    {
        /// <summary>제어 모드(current 또는 manager)다.</summary>
        private readonly string _mode;

        /// <summary>명시 변이 종류다.</summary>
        private readonly string _mutation;

        /// <summary>독립 입력: 현재 owner/다른 owner/현재 이름 번호/Fort/Editor/Mission 모드/종류다.</summary>
        private uint _owner, _other, _current, _dirty = 0xfffffffe, _fort, _editor, _mission, _pathMode, _kind, _newKind, _lastHeader;

        /// <summary>실제 하위 경계 횟수다.</summary>
        private uint _snapshots, _notifies, _paths, _deletes, _configs;

        /// <summary>005327e0이 FileSpec 버퍼를 선택했는지다.</summary>
        private bool _specPointer;

        /// <summary>고정 9×20바이트 헤더의 실제 물리 저장소다.</summary>
        private readonly byte[] _headers = new byte[180];

        /// <summary>두 manager의 이름 외 주변 바이트까지 관측한다.</summary>
        private readonly byte[][] _managers = [new byte[276], new byte[276]];

        /// <summary>현재 포인터가 선택하는 서로 다른 실제 이름 쓰기 버퍼다.</summary>
        private readonly byte[][] _names = [new byte[256], new byte[256]];

        /// <summary>외부 새 이름 입력이며 callback의 실제 쓰기 대상이다.</summary>
        private readonly byte[] _argument = new byte[512];

        /// <summary>선택 포인터와 달리 내용은 callback이 바꿀 수 있는 FileSpec이다.</summary>
        private readonly byte[] _spec = new byte[9];

        /// <summary>원문 미초기화 oldpath는 명시 바이트 입력으로 공급한다.</summary>
        private readonly byte[] _scratch = new byte[256];

        /// <summary>State getter 반환 후 +17을 읽는 실제 저장소다.</summary>
        private readonly byte[][] _states = Enumerable.Range(0, 9).Select(_ => new byte[28]).ToArray();

        /// <summary>다음 PathSpec 전까지 유효한 실제 반환 버퍼다.</summary>
        private byte[] _path = [];

        /// <summary>실제 경계 순서다.</summary>
        public StringBuilder Trace { get; } = new();

        /// <summary>마지막으로 도달한 물리 대상 이름이다.</summary>
        private string _physical = "";

        /// <summary>원문 고정 입력만 설치하고 이후 본문 효과를 선계산하지 않는다.</summary>
        /// <param name="input">fixture 입력 13칸.</param>
        public NameDevices(string input)
        {
            string[] fields = input.Split(',');
            Assert.Equal(13, fields.Length);
            _mode = fields[0];
            _owner = Number(fields[1]);
            _other = (_owner + 1) % 9;
            uint manager = Number(fields[2]);
            uint readOnly = Number(fields[3]);
            uint duplicate = Number(fields[4]);
            uint old = Number(fields[5]);
            _newKind = Number(fields[6]);
            _pathMode = Number(fields[7]);
            _fort = Number(fields[8]);
            _editor = Number(fields[9]);
            _mission = Number(fields[10]);
            _kind = Number(fields[11]);
            _mutation = fields[12];
            // 주변 헤더 입력은 원문과 동일하게 유지한다
            for (int index = 0; index < _headers.Length; index++)
            {
                _headers[index] = (byte)(0x70 + index);
            }
            // 사용하지 않는 슬롯도 manager 0과 독립 State 입력을 갖는다
            for (int slot = 0; slot < 9; slot++)
            {
                Put(_headers, slot * 20, 0);
                for (int index = 0; index < 28; index++)
                {
                    _states[slot][index] = (byte)(0x80 + index);
                }
                _states[slot][17] = (byte)_kind;
            }
            Put(_headers, (int)_other * 20, ManagerTokens[2]);
            Put(_headers, (int)_other * 20 + 4, 0);
            Put(_headers, (int)_other * 20 + 8, 0);
            Put(_headers, (int)_owner * 20, ManagerTokens[manager / 11]);
            Put(_headers, (int)_owner * 20 + 4, duplicate);
            Put(_headers, (int)_owner * 20 + 8, readOnly);
            // 각 manager의 +c만 NUL 이름으로 덮고 그 밖 276바이트 입력을 보존한다
            for (int index = 0; index < _managers.Length; index++)
            {
                uint key = (uint)((index + 1) * 11);
                for (int byteIndex = 0; byteIndex < 276; byteIndex++)
                {
                    _managers[index][byteIndex] = (byte)(0x91 + byteIndex + key);
                }
                NameInput(_managers[index].AsSpan(12), "C:/prior" + key + ".fort");
            }
            string[] oldNames = ["Old", "", "oLd"];
            // 현재 포인터를 바꿔도 이전 버퍼의 주소와 NUL 뒤 바이트는 유지한다
            for (int index = 0; index < _names.Length; index++)
            {
                Array.Fill(_names[index], (byte)0xa7);
                NameInput(_names[index], index != 0 ? "Other" : oldNames[old]);
            }
            Array.Fill(_argument, (byte)0xb6);
            string[] newNames = ["New", "Old", "oLd", "Bad.Name", "", new string('M', 255), new string('M', 256), "New\0.after"];
            if (_newKind == 7)
            {
                Latin1(newNames[7]).CopyTo(_argument, 0);
            }
            else if (_newKind < 8)
            {
                NameInput(_argument, newNames[_newKind]);
            }
            NameInput(_spec, "fortSpec");
            Array.Fill(_scratch, (byte)0xcd);
            NameInput(_scratch, "Prior");
        }

        /// <summary>동일 argument 주소를 callback과 본문에 공급한다. 8번은 실제 현재 이름 0의 자기참조다.</summary>
        private byte[] Argument() => _newKind == 8 ? _names[0] : _argument;

        /// <summary>헤더의 원문 manager 토큰을 관측용 명시 번호로 해석한다.</summary>
        /// <param name="slot">Template 슬롯.</param>
        private uint Manager(uint slot)
        {
            uint token = Get(_headers, (int)slot * 20);
            int found = Array.IndexOf(ManagerTokens, token);
            if (found < 0)
            {
                throw new InvalidOperationException("FortName장치manager토큰");
            }
            return (uint)found * 11;
        }

        /// <summary>실제 진입 순서를 연결한다.</summary>
        /// <param name="value">관측 항목.</param>
        private void Record(string value)
        {
            if (Trace.Length != 0)
            {
                Trace.Append(';');
            }
            Trace.Append(value);
        }

        /// <summary>명시 callback 효과만 적용하고 부모의 관문/호출 순서를 대신 판정하지 않는다.</summary>
        /// <param name="boundary">도달한 경계 이름.</param>
        /// <param name="fixedSlot">고정 setter this 슬롯(9면 현재 owner).</param>
        private void Mutate(string boundary, uint fixedSlot = 9)
        {
            uint selected = _owner;
            // 고정 setter this와 현재 owner를 구별한다
            uint target = fixedSlot < 9 ? fixedSlot : selected;
            string prefix = _mutation.Contains('-') ? _mutation[.._mutation.IndexOf('-')] : _mutation;
            if (_mutation == boundary + "-owner")
            {
                _owner = _other;
            }
            else if (_mutation == boundary + "-current1")
            {
                _current = 1;
            }
            else if (_mutation == "snapshot1-manager0" && boundary == "snapshot1")
            {
                Put(_headers, (int)selected * 20, 0);
            }
            else if ((_mutation == "snapshot1-readonly1" || _mutation == "snapshot2-readonly1" || _mutation == "notify-readonly1") && boundary == prefix)
            {
                Put(_headers, (int)target * 20 + 8, 1);
            }
            else if (_mutation == "snapshot1-name" && boundary == "snapshot1")
            {
                NameInput(Argument(), "Changed");
            }
            else if ((_mutation == "notify-manager22" || _mutation == "assert429-manager22" || _mutation == "assert430-manager22" || _mutation == "assert88-manager22") && boundary == prefix)
            {
                Put(_headers, (int)target * 20, ManagerTokens[2]);
            }
            else if ((_mutation == "title-editor1" || _mutation == "state-editor1" || _mutation == "config1-editor1") && boundary == prefix)
            {
                _editor = 1;
            }
            else if (_mutation == "title-dirty" && boundary == "title")
            {
                _dirty = 0xffffffff;
            }
            else if (_mutation == "state-mission1" && boundary == "state")
            {
                _mission = 1;
            }
            else if (_mutation == "path1-specsolo" && boundary == "path1")
            {
                NameInput(_spec, "SOLOSpec");
            }
            else if (_mutation == "path1-pointerother" && boundary == "path1")
            {
                _specPointer = false;
            }
            else if (_mutation == boundary + "-stop")
            {
                throw new NameStop(boundary);
            }
        }

        /// <summary>조회 함수는 순수 현재 값을 반환하며 dirty는 실제 동일 DWORD를 읽고 쓴다.</summary>
        public FortCurrentNameGlobals Globals() => new()
        {
            Owner = () => _owner,
            Fort = () => _fort,
            Editor = () => _editor,
            Mission = () => _mission,
            Dirty = () => _dirty,
            SetDirty = value => _dirty = value,
        };

        /// <summary>실제 하위 장치만 구성하고 이름 복사/헤더 +8 쓰기/두 snapshot 선택은 제품 본문이 수행한다.</summary>
        public FortCurrentNameHooks Hooks() => new()
        {
            Header = slot =>
            {
                _lastHeader = slot;
                if (slot >= 9)
                {
                    throw new ArgumentOutOfRangeException(null, "FortName장치header");
                }
                return _headers.AsSpan((int)slot * 20, 20);
            },
            CurrentName = () =>
            {
                _physical = "current-size";
                return _names[_current];
            },
            ManagerName = token =>
            {
                if (token == 0)
                {
                    throw new NameStop("manager0");
                }
                int found = Array.IndexOf(ManagerTokens, token, 1);
                if (found < 0)
                {
                    throw new InvalidOperationException("FortName장치현재manager주소");
                }
                _physical = "manager-name-size";
                return _managers[found - 1].AsSpan(12, 256);
            },
            SelectFileSpec = () => _specPointer = true,
            PathSpec = source =>
            {
                ReadOnlySpan<byte> text = NameText(source);
                ReadOnlySpan<byte> key = NameText(_spec);
                string name = Encoding.Latin1.GetString(text);
                string result;
                // 선택한 장치 입력으로 반환 경로만 만든다
                if (_pathMode == 1)
                {
                    result = "C:/fixed.fort";
                }
                else if (_pathMode == 3 || _pathMode == 4)
                {
                    int length = _pathMode == 3 ? 255 : 256;
                    result = new string('P', length > name.Length + 5 ? length - name.Length - 5 : 0) + name + ".fort";
                }
                else
                {
                    result = (NameHex(key) == "666f727453706563" ? "C:/work/d/" : "C:/solo/d/") + name + ".fort";
                }
                if (_pathMode == 2)
                {
                    result += "\0TAIL";
                }
                _paths++;
                _path = [.. Latin1(result), 0];
                Record("P:" + NameHex(key) + ':' + NameHex(text) + ':' + NameHex(_path.AsSpan(0, _path.Length - 1)));
                _physical = "old-path-size";
                Mutate("path" + _paths);
                return _path;
            },
            Assertion = line =>
            {
                Record("A:" + line);
                Mutate("assert" + line, line == 429 || line == 430 ? _lastHeader : 9);
            },
            Snapshot = () =>
            {
                _snapshots++;
                Record("S:" + _owner + ':' + Manager(_owner));
                Mutate("snapshot" + _snapshots);
            },
            Notify = slot =>
            {
                _notifies++;
                uint manager = Manager(slot);
                ReadOnlySpan<byte> text = NameText(_managers[manager / 11 - 1].AsSpan(12));
                Record("N:" + slot + ':' + manager + ':' + NameHex(text));
                Mutate("notify", slot);
            },
            SamePath = (old, next) =>
            {
                old = NameText(old);
                next = NameText(next);
                Record("E:" + NameHex(old) + ':' + NameHex(next));
                bool same = old.Length == next.Length;
                // A..Z만 소문자로 바꾸며 원래 버퍼는 수정하지 않는다
                for (int index = 0; same && index < old.Length; index++)
                {
                    byte a = old[index];
                    byte b = next[index];
                    same = (a >= 'A' && a <= 'Z' ? a + 32 : a) == (b >= 'A' && b <= 'Z' ? b + 32 : b);
                }
                Record("Q:" + (same ? '1' : '0'));
                return same;
            },
            // 실제 파일 조작은 절대 수행하지 않는다
            DeleteRequest = old =>
            {
                _deletes++;
                Record("D:" + NameHex(NameText(old)));
                Mutate("delete");
            },
            RefreshTitle = () =>
            {
                Record("T:" + NameHex(NameText(_names[_current])));
                Mutate("title");
            },
            State = slot =>
            {
                Record("G:" + slot);
                Mutate("state");
                return _states[slot];
            },
            Config = (key, value) =>
            {
                _configs++;
                Record("C:" + key + ':' + NameHex(NameText(value)));
                Mutate("config" + _configs);
            },
        };

        /// <summary>현재 전역/하위 횟수만 관측열로 변환한다.</summary>
        public string State()
        {
            uint[] values = [_owner, _current, _dirty, _fort, _editor, _mission, _specPointer ? 1u : 0u, _snapshots, _notifies, _paths, _deletes, _configs];
            return string.Join(',', values);
        }

        /// <summary>헤더/manager 주변/current/NUL 뒤 입력/FileSpec/지역 oldpath를 모두 직접 대조한다.</summary>
        public string Raw()
        {
            return NameHex(_headers) + ':' + NameHex(_managers[0]) + ':' + NameHex(_managers[1]) + ':' + NameHex(_names[0]) + ':' + NameHex(_names[1])
                + ':' + NameHex(_argument) + ':' + NameHex(_spec) + ':' + (_mode == "current" ? NameHex(_scratch) : "-");
        }

        /// <summary>실제 본문만 실행하고 도달한 물리 접근/callback 중단만 분리한다.</summary>
        public string Run()
        {
            try
            {
                FortCurrentNameHooks hooks = Hooks();
                if (_mode == "current")
                {
                    RawFortCurrentName.SetCurrentName(Argument(), _scratch, Globals(), hooks);
                }
                else
                {
                    RawFortCurrentName.SetFortManagerFileName(_owner, Argument(), hooks);
                }
                return "1";
            }
            catch (NameStop stop)
            {
                return stop.Cause;
            }
            catch (ArgumentOutOfRangeException)
            {
                if (_physical.Length == 0)
                {
                    throw;
                }
                return _physical;
            }
        }

        /// <summary>도달하지 않는 경계 미연결/중단 전 부분 효과/실제 입력 물리 범위 검사용 접근자다.</summary>
        public byte[] ArgumentBytes => Argument();

        /// <summary>검사가 직접 읽는 scratch다.</summary>
        public byte[] Scratch => _scratch;

        /// <summary>검사가 직접 읽는 첫 manager 바이트다.</summary>
        public byte[] FirstManager => _managers[0];

        /// <summary>검사가 직접 읽는 dirty 값이다.</summary>
        public uint Dirty => _dirty;

        /// <summary>검사가 직접 읽는 snapshot 횟수다.</summary>
        public uint Snapshots => _snapshots;

        /// <summary>검사가 직접 읽는 notify 횟수다.</summary>
        public uint Notifies => _notifies;

        /// <summary>검사가 직접 읽는 PathSpec 횟수다.</summary>
        public uint Paths => _paths;

        /// <summary>검사가 직접 읽는 config 횟수다.</summary>
        public uint Configs => _configs;
    }

    /// <summary>최초 438입력/두 정밀도 876관측과 명시 중단 156건까지 모든 변경을 대조한다.</summary>
    [Fact]
    public void CurrentNameMatchesNative876Observations()
    {
        string[][] rows = X86Fixture.Read("fort-current-name");
        Assert.Equal(876, rows.Length);
        HashSet<(string, string)> unique = [];
        List<string> problems = [];
        int stopped = 0;
        // 입력열만으로 본문을 실행한 뒤 고정 원문 출력과 대조한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Assert.True(unique.Add((row[0], row[1])), "중복 관측");
            NameDevices devices = new(row[1]);
            string completion = devices.Run();
            if (completion != "1")
            {
                stopped++;
            }
            if (completion != row[2] || devices.State() != row[3] || devices.Raw() != row[4] || devices.Trace.ToString() != row[5])
            {
                problems.Add($"{row[1]} completion={completion}/{row[2]} state={devices.State()}/{row[3]} trace={devices.Trace}/{row[5]}");
            }
        }
        Assert.Equal(156, stopped);
        NoProblems(problems, "현재 Fort 이름 876관측");
    }

    /// <summary>도달하지 않는 경계 미연결/중단 전 부분 효과/실제 입력 물리 범위를 원문 fixture와 별도로 검사한다.</summary>
    [Fact]
    public void CurrentNameSelectedBoundariesAndPhysicalStop()
    {
        NameDevices minimal = new("current,1,0,0,0,0,0,0,0,0,0,0,none");
        FortCurrentNameHooks hooks = minimal.Hooks();
        hooks.PathSpec = null;
        hooks.Snapshot = null;
        hooks.Notify = null;
        hooks.DeleteRequest = null;
        hooks.State = null;
        hooks.Config = null;
        RawFortCurrentName.SetCurrentName(minimal.ArgumentBytes, minimal.Scratch, minimal.Globals(), hooks);
        Assert.True(minimal.Dirty == 0xffffffff && minimal.Paths == 0 && minimal.Snapshots == 0 && minimal.Configs == 0);

        NameDevices missing = new("current,1,11,0,0,0,0,0,1,0,0,0,none");
        hooks = missing.Hooks();
        hooks.Notify = null;
        Assert.Throws<InvalidOperationException>(() => RawFortCurrentName.SetCurrentName(missing.ArgumentBytes, missing.Scratch, missing.Globals(), hooks));
        Assert.True(missing.Snapshots == 1 && missing.Notifies == 0 && missing.Dirty == 0xfffffffe);
        // Notify 미연결도 이미 복사한 파일명을 되돌리지 않는다
        Assert.Equal("433a2f776f726b2f642f4e65772e666f7274", NameHex(NameText(missing.FirstManager.AsSpan(12))));

        NameDevices physical = new("current,1,0,0,0,0,0,0,0,0,0,0,none");
        hooks = physical.Hooks();
        byte[] shortName = new byte[2];
        hooks.CurrentName = () => shortName;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFortCurrentName.SetCurrentName(physical.ArgumentBytes, physical.Scratch, physical.Globals(), hooks));
        Assert.True(shortName[0] == (byte)'N' && shortName[1] == (byte)'e' && physical.Dirty == 0xfffffffe);

        NameDevices unterminated = new("current,1,0,0,0,0,0,0,0,0,0,0,none");
        hooks = unterminated.Hooks();
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFortCurrentName.SetCurrentName(new byte[] { (byte)'A', (byte)'B' }, unterminated.Scratch, unterminated.Globals(), hooks));
        Assert.True(unterminated.Snapshots == 0 && unterminated.Dirty == 0xfffffffe);
    }

    /// <summary>Fort 이미지 검사에서 Early Missions 여섯 개와 캠페인 1-1/1-2의 실제 loadFort 파일 이름이다.</summary>
    private static readonly string[] FortNames = ["BridgeTheGap", "SecretWorkshop", "CaptureThePriest", "TacticalCombat", "SubtleDefense", "RawPower", "TheWarBegins", "MasterOfWhirligigs"];

    /// <summary>원본 데이터 폴더의 파일 시스템이다(없으면 null).</summary>
    private static readonly Lazy<GameFileSystem?> Files = new(() =>
        GameDataLocator.FindDataDirectory() is { } directory ? GameFileSystem.Open(directory) : null);

    /// <summary>원본에 준 현재 섹션 입력만 만들며 이미지의 기대 헤더·길이는 계산하지 않는다.</summary>
    /// <param name="profile">입력 종류(100 이상은 실제 보호 fort 파일).</param>
    /// <param name="seed">패턴 시드.</param>
    /// <param name="count">합성 섹션 수.</param>
    /// <param name="flag">서명 flag.</param>
    private static FortTemplate FortInput(uint profile, uint seed, uint count, uint flag)
    {
        FortTemplate fort;
        if (profile >= 100)
        {
            Assert.True(Files.Value is not null, "클론 데이터 폴더가 없습니다");
            byte[] bytes = Files.Value!.ReadAllBytes("d/" + FortNames[profile - 100] + ".fort");
            fort = FortTemplate.ReadSections(bytes);
            if ((seed & 1) != 0)
            {
                // 합성 섹션 12를 부족하면 빈 섹션으로 채워 13개까지 늘린다
                while (fort.Sections.Count < 13)
                {
                    fort.Sections.Add(new RawSectionBuffer());
                }
                fort.Sections[12] = RawSectionBuffer.FromBytes([0xa0, (byte)profile, (byte)seed, 0, 0xfe]);
            }
        }
        else
        {
            fort = new FortTemplate();
            // 합성 payload는 원본 입력과 같은 패턴이며 저장 순서·길이 기대값을 만들지 않는다
            for (uint index = 0; index < count; index++)
            {
                uint size = profile == 0 ? 0 : profile == 1 ? (index * 7 + seed) % 17 : profile == 2 && index == 34 ? 32696u : profile == 3 && index == 34 ? 32695u : 0u;
                byte[] section = new byte[size];
                // 바이트 패턴은 각 현재 stream의 독립 입력이다
                for (int position = 0; position < section.Length; position++)
                {
                    section[position] = (byte)(position * 29 + index * 13 + seed);
                }
                fort.Sections.Add(RawSectionBuffer.FromBytes(section));
            }
        }
        fort.Flag = (byte)flag;
        return fort;
    }

    /// <summary>원본의 논리 이미지 뒤 전체 고정 할당이 0으로 남았는지 검사한다.</summary>
    /// <param name="image">검사할 이미지.</param>
    private static void CheckFortAllocation(FortImageData image)
    {
        Assert.Equal(RawFortImage.Capacity, image.Storage.Length);
        Assert.True(image.End <= image.Storage.Length);
        Assert.All(image.Storage.Skip(image.End), value => Assert.Equal(0, value));
    }

    /// <summary>모든 독립 입력(저장 116행)의 현재 이미지/효과 읽기 순서를 대조한다. 송신(S)·예약(F) 행은 미션 로그인 패킷 계층을 옮길 때 잇는다.</summary>
    [Fact]
    public void FortImageSaveMatchesNative116Rows()
    {
        string[][] rows = X86Fixture.Read("fort-image-packets");
        List<string> problems = [];
        int compared = 0;
        // 각 행의 현재 Template·효과 변경은 다음 행에 누적하지 않는다
        foreach (string[] row in rows)
        {
            if (row[0] != "I")
            {
                continue;
            }
            Assert.Equal(7, row.Length);
            string[] fields = row[1].Split(',');
            Assert.Equal(13, fields.Length);
            uint[] input = fields.Select(Number).ToArray();
            FortTemplate first = FortInput(input[0], input[1], input[2], input[3]);
            StringBuilder trace = new();
            // 실제 외부 읽기/효과만 기록한다
            void Event(string text)
            {
                if (trace.Length != 0)
                {
                    trace.Append(';');
                }
                trace.Append(text);
            }
            FortImageHooks hooks = new()
            {
                TypeCount = () =>
                {
                    Event("G," + input[4]);
                    return input[4];
                },
                Assertion = kind => Event(kind == FortImageAssertion.TypeCount ? "A,types" : kind == FortImageAssertion.SectionCapacity ? "A,section" : "A,stream"),
            };
            FortImageData image = new RawFortImage(OriginalEdition.Patch1078, first, hooks).Save();
            CheckFortAllocation(image);
            if (DashHex(image.Bytes()) != row[2] || row[3] != "-" || row[4] != "-" || row[5] != "-" || (trace.Length == 0 ? "-" : trace.ToString()) != row[6])
            {
                problems.Add(row[1]);
            }
            compared++;
        }
        Assert.Equal(116, compared);
        NoProblems(problems, "Fort 이미지 저장 116행");
    }

    /// <summary>실제 파일의 현재 raw 섹션 변경이 반영되며 파일 캐시/해석 Money 필드로 되돌리지 않는다.</summary>
    [Fact]
    public void FortImageReflectsCurrentProtectedSections()
    {
        FortImageHooks hooks = new() { TypeCount = () => 188 };
        // 목표 8개 보호 fort의 실제 현재 섹션을 같은 writer로 저장한다
        for (uint profile = 100; profile < 108; profile++)
        {
            FortTemplate fort = FortInput(profile, 0, 35, 0);
            FortImageData original = new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save();
            FortImageData repeated = new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save();
            Assert.True(original.Storage.AsSpan().SequenceEqual(repeated.Storage) && original.End == repeated.End);
            fort.Sections[12] = RawSectionBuffer.FromBytes([0xa0, (byte)profile, 1, 0, 0xfe]);
            FortImageData changed = new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save();
            Assert.False(changed.Storage.AsSpan().SequenceEqual(original.Storage));
            CheckFortAllocation(changed);
            FortTemplate parsed = FortTemplate.ReadSections(changed.Bytes());
            Assert.Equal(fort.Sections.Count, parsed.Sections.Count);
            // 저장한 이미지를 다시 읽어도 섹션 바이트가 같다
            for (int index = 0; index < fort.Sections.Count; index++)
            {
                Assert.Equal(fort.Sections[index].ToArray(), parsed.Sections[index].ToArray());
            }
        }
    }

    /// <summary>원본의 엄격한 '&lt;' 진단은 정확히 32768인 정상 물리 쓰기를 막지 않고 초과만 명시 거부한다.</summary>
    [Fact]
    public void FortImagePhysicalCapacityAndRequiredConsumers()
    {
        FortTemplate fort = FortInput(2, 0, 35, 0);
        List<FortImageAssertion> assertions = [];
        FortImageHooks hooks = new() { TypeCount = () => 188, Assertion = assertions.Add };
        FortImageData exact = new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save();
        Assert.Equal(32768, exact.End);
        Assert.Equal(new[] { FortImageAssertion.SectionCapacity }, assertions);
        fort.Sections[34] = RawSectionBuffer.FromBytes([.. fort.Sections[34].ToArray(), 1]);
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save()));
        fort.Sections.Clear();
        hooks.TypeCount = () => 256;
        hooks.Assertion = null;
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save()));
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Cd1072, fort, hooks)));
        hooks.TypeCount = null;
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Patch1078, fort, hooks)));
        hooks.TypeCount = () => 188;
        for (int index = 0; index < 36; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Patch1078, fort, hooks)));
        Assert.True(Throws(() => new FortImageData { End = 1 }.Bytes()));
    }

    /// <summary>
    /// 마지막 길이 필드가 이미지 끝 직전(32766)에 놓이는 경계다. 길이 필드는 정상적으로 쓰고 이후 섹션 용량 진단과 길이 필드 용량 진단 뒤에 물리 범위에서 거부한다.
    /// 합성 입력의 크기 조합이 닿지 않는 경계를 직접 확인한다.
    /// </summary>
    [Fact]
    public void FortImageStreamCapacityBoundaryAtLastLengthWord()
    {
        FortTemplate fort = new();
        fort.Sections.Add(RawSectionBuffer.FromBytes(new byte[32762]));
        // 나머지 섹션은 빈 stream이다
        for (int index = 1; index < FortTemplate.SectionCount; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        List<FortImageAssertion> assertions = [];
        FortImageHooks hooks = new() { TypeCount = () => 188, Assertion = assertions.Add };
        Assert.True(Throws(() => new RawFortImage(OriginalEdition.Patch1078, fort, hooks).Save()));
        Assert.Equal(new[] { FortImageAssertion.SectionCapacity, FortImageAssertion.StreamCapacity }, assertions);
    }

    /// <summary>양쪽 정밀도 99개 독립 입력에서 원문 보존/0 확장/현재 pos·end를 검사한다.</summary>
    [Fact]
    public void PlayerStateExpandMatchesNative99Rows()
    {
        string[][] rows = X86Fixture.Read("player-state-expand");
        Assert.Equal(99, rows.Length);
        List<string> problems = [];
        // C#로 기대값을 계산하지 않고 원본 행 전체와 현재 메모리를 대조한다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            uint length = Number(row[0]);
            uint pattern = Number(row[1]);
            // 명시 원문과 새 할당의 미초기화 입력이다
            byte[] old = new byte[64];
            byte[] fresh = new byte[64];
            Array.Fill(fresh, (byte)0xa5);
            // 원본 관측기와 같은 독립 물리 바이트만 입력한다
            for (int index = 0; index < old.Length; index++)
            {
                old[index] = (byte)(index * 17 + pattern);
            }
            byte[] before = [.. old];
            byte[] headers = new byte[180];
            headers[20] = 1;
            PlayerTemplateStateStream[] streams = Enumerable.Range(0, 9).Select(_ => new PlayerTemplateStateStream()).ToArray();
            // 원본 lookup이 pos 13을 0으로 바꾸는 현재 입력이다
            streams[1] = new PlayerTemplateStateStream { Bytes = old, Position = 13, End = length, Present = true };
            string[] names = new string[9];
            Array.Fill(names, "");
            names[1] = "tutorial1";
            uint depth = 0;
            uint allocations = 0;
            RawPlayerTemplateState body = new(OriginalEdition.Patch1078, headers, slot => streams[slot], slot => names[slot], () => depth, value => depth = value, () => true,
                _ => throw new InvalidOperationException("State확장 정상입력의예상밖진단"),
                _ => throw new InvalidOperationException("PATCH State확장에서CD depth효과금지"),
                (owner, size) =>
                {
                    Assert.True(owner == 1 && size == 28);
                    allocations++;
                    return fresh.AsMemory(0, (int)size);
                });
            string state = Hex(body.Get(1)[..28]);
            if (allocations != Number(row[2]) || streams[1].Position != Number(row[3]) || streams[1].End != Number(row[4]) || state != row[5] || !old.AsSpan().SequenceEqual(before) || depth != 0)
            {
                problems.Add(row[0] + "," + row[1]);
            }
            // 두 정밀도와 다음 Get의 같은 현재 버퍼/pos 0도 확인한다
            for (int index = 2; index < 6; index++)
            {
                if (row[index] != row[index + 4])
                {
                    problems.Add(row[0] + "," + row[1] + " 정밀도");
                }
            }
            Span<byte> second = body.Get(1);
            Span<byte> third = body.Get(1);
            if (!Unsafe.AreSame(ref MemoryMarshal.GetReference(second), ref MemoryMarshal.GetReference(third)) || streams[1].Position != 0 || allocations != Number(row[2]))
            {
                problems.Add(row[0] + "," + row[1] + " 재호출");
            }
        }
        NoProblems(problems, "Player State 확장 99행");
    }

    /// <summary>부모 4bd550의 생성 조건·보존 스택/flags·28바이트 쓰기와 기존 State 자연 반환을 2560행에 대조한다.</summary>
    [Fact]
    public void PlayerStateInitializationMatchesNative2560Rows()
    {
        string[][] rows = X86Fixture.Read("player-state-initialization");
        Assert.Equal(2560, rows.Length);
        List<string> problems = [];
        // 각 행의 스택 미정 바이트는 원본 보장 초기값이 아닌 독립 입력이다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(4, input.Length);
            uint seed = Number(input[0]);
            uint present = Number(input[2]);
            uint end = Number(input[3]);
            byte[] stack = Decode(row[1]);
            byte[] expectedStack = Decode(row[2]);
            byte[] expectedNode = Decode(row[3]);
            byte[] buffer = new byte[64];
            // 같은 원문 버퍼의 생성 경계 입력을 준비하며 첫 28바이트 밖의 보존도 대조한다
            for (int offset = 0; offset < buffer.Length; offset++)
            {
                buffer[offset] = (byte)(offset * 13 + seed);
            }
            byte[] headers = new byte[180];
            headers[20] = 1;
            PlayerTemplateStateStream[] streams = Enumerable.Range(0, 9).Select(_ => new PlayerTemplateStateStream()).ToArray();
            // 같은 선택 경계의 pos 0과 실제 데이터 입력이다
            streams[1] = new PlayerTemplateStateStream { Bytes = buffer, Position = 17, End = end, Present = present != 0 };
            string[] names = new string[9];
            Array.Fill(names, "");
            string events = "S";
            RawPlayerTemplateState body = new(OriginalEdition.Patch1078, headers, slot => streams[slot], slot => names[slot], () => 0u, _ => { }, () => true,
                _ => throw new InvalidOperationException("State 초기화 예상 밖 진단"),
                _ => throw new InvalidOperationException("PATCH 빈 State의 CD 깊이 효과 금지"));
            body.BindInitialization(new PlayerTemplateStateInitialization
            {
                Bytes = (slot, output) =>
                {
                    Assert.Equal(1u, slot);
                    stack.AsSpan().CopyTo(output);
                },
                Replace = (slot, bytes) =>
                {
                    Assert.Equal(1u, slot);
                    events += 'C';
                    stack = bytes.ToArray();
                    bytes.CopyTo(buffer);
                    streams[1] = new PlayerTemplateStateStream { Bytes = buffer, Position = 28, End = 28, Present = true };
                    return streams[1];
                },
            });
            Span<byte> result = body.Get(1);
            events += 'E';
            if (!Unsafe.AreSame(ref MemoryMarshal.GetReference(result), ref buffer[0]) || Hex(stack) != Hex(expectedStack) || Hex(buffer) != row[4] || events != row[5]
                || streams[1].Position != Get(expectedNode, 4) || streams[1].End != Get(expectedNode, 8) || !streams[1].Present)
            {
                problems.Add(row[0]);
            }
        }
        NoProblems(problems, "Player State 초기화 2560행");
    }

    /// <summary>미연결 생성은 앞선 pos 0을 보존한 채 거부하고 생성 결과나 CD 지원을 합성하지 않는다.</summary>
    [Fact]
    public void PlayerStateInitializationRequiredEffectsAndEdition()
    {
        byte[] headers = new byte[180];
        headers[20] = 1;
        byte[] bytes = new byte[1];
        PlayerTemplateStateStream[] streams = Enumerable.Range(0, 9).Select(_ => new PlayerTemplateStateStream()).ToArray();
        streams[1] = new PlayerTemplateStateStream { Bytes = bytes, Position = 17, End = 0, Present = true };
        RawPlayerTemplateState state = new(OriginalEdition.Patch1078, headers, slot => streams[slot], _ => "", () => 0u, _ => { }, () => true, _ => { }, _ => { });
        Assert.True(Throws(() => state.Get(1)));
        Assert.Equal(0u, streams[1].Position);
        Assert.True(Throws(() => state.BindInitialization(new PlayerTemplateStateInitialization())));
        RawPlayerTemplateState cd = new(OriginalEdition.Cd1072, headers, slot => streams[slot], _ => "", () => 0u, _ => { }, () => true, _ => { }, _ => { });
        Assert.True(Throws(() => cd.BindInitialization(new PlayerTemplateStateInitialization { Bytes = (_, _) => { }, Replace = (_, _) => streams[1] })));
    }
}
