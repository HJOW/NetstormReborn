using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Core.Tests.RawStorageTestSupport;

namespace Netstorm.Core.Tests;

/// <summary>
/// 새 맵(NewFort)의 세 원본 부모(<c>RawNewFort</c>)를 원본 x86 관측에 대조한다: manager 48행, 신규 자료 1,248행, 파일 부재 26행, 부재 State bit 78행.
/// 장면은 cpppj/tests/NewFortTests.cpp와 같고, 외부 재진입으로 전역이 바뀌는 13종 변이표를 그대로 재생한다 (2026-11-02 TS6).
/// </summary>
public sealed class RawNewFortTests
{
    /// <summary>원본 관측에서 명시한 불투명 allocation 입력이다(호스트 포인터가 아니다).</summary>
    private static readonly uint[] Allocations = [0x10006000, 0x10006200];

    /// <summary>모든 manager/missing 관측이 쓴 명시 파일 이름이다.</summary>
    private const string FortPath = "d/NewFort.fort";

    /// <summary>장치에 전달한 문자열을 관측 fixture의 바이트 표기(소문자 16진수)로 바꾼다.</summary>
    /// <param name="text">원본 바이트 문자열.</param>
    private static string TextHex(string text) => Hex(Latin1(text));

    /// <summary>입력 버퍼의 첫 NUL까지를 읽는다. 공백 정리나 문자 변환을 하지 않는다.</summary>
    /// <param name="bytes">NUL 종료 버퍼.</param>
    private static string Text(ReadOnlySpan<byte> bytes) => TextAt(bytes);

    /// <summary>
    /// 입력 주소와 전역을 같은 C# 필드에 대응시키는 검사 장면이다. 실제 게임 소유자를 대신하는 제품 장치가 아니라 원본이 명시한
    /// 파일·할당·등록·난수·저장 장치만 재생한다.
    /// </summary>
    private sealed class Replay
    {
        /// <summary>난수 변이가 되돌리는 초기 Editor 입력이다.</summary>
        public uint InitialEditor = 1;

        /// <summary>각 호출이 다시 읽는 현재 Editor 모드다.</summary>
        public uint Editor = 1;

        /// <summary>현재 Player 확인 값이다.</summary>
        public uint Known = 1;

        /// <summary>현재 Template 소유자다.</summary>
        public uint Owner = 1;

        /// <summary>현재 로컬 소유자다.</summary>
        public uint LocalOwner = 1;

        /// <summary>현재 Subscriber ID(00540c74)다.</summary>
        public uint Subscriber = 123;

        /// <summary>CoreData 활성 표시(00595348)다.</summary>
        public uint CoreActive = 0x12345678;

        /// <summary>시드 전체 DWORD(005c89b4)다.</summary>
        public uint Seed = 0xa55a6789;

        /// <summary>파일 부재 부모가 확정하는 대체 경로 표시다.</summary>
        public uint Alternate = 0xabcdef01;

        /// <summary>외부 재진입 변이 번호다.</summary>
        public uint Mutation;

        /// <summary>SP 조회가 돌려줄 float32 비트다.</summary>
        public uint SpBits;

        /// <summary>254 경계 난수 입력이다.</summary>
        public uint Random254;

        /// <summary>다음에 소모할 65536 경계 난수의 위치다.</summary>
        public int RandomIndex;

        /// <summary>PTR00540ca8의 두 입력 버퍼 중 현재 포인터가 다른 쪽인지 나타낸다.</summary>
        public bool OtherSelected;

        /// <summary>원문이 복사하는 첫째 NUL 버퍼와 A5 주변 영역이다.</summary>
        public readonly byte[] Handle = new byte[256];

        /// <summary>둘째 NUL 버퍼다.</summary>
        public readonly byte[] Other = new byte[256];

        /// <summary>10바이트 지움 뒤 나머지 6바이트가 보존되는지 함께 읽는 CoreData다.</summary>
        public readonly byte[] Core = new byte[16];

        /// <summary>9개 소유자의 State 32바이트를 모두 읽어 잘못된 소유자 쓰기를 검출한다.</summary>
        public readonly byte[] States = new byte[288];

        /// <summary>쓰기 장치로 전달된 20개 6바이트 레코드의 전체 복사다.</summary>
        public readonly byte[] Territory = new byte[120];

        /// <summary>0 재시도와 최종 비영값을 포함한 원본 난수 입력 순서다.</summary>
        public readonly List<uint> Randoms = [];

        /// <summary>변이 장치가 지정한 호출 횟수다. 제품 부모는 쓰지 않는다.</summary>
        private readonly Dictionary<string, uint> _counts = [];

        /// <summary>실제 하위 호출 순서와 캡처한 인자를 한 줄로 기록한다.</summary>
        public string Trace = "";

        /// <summary>원본 reset과 같은 전체 A5 입력과 두 문자열을 준비한다.</summary>
        public Replay()
        {
            Array.Fill(Handle, (byte)0xa5);
            Array.Fill(Other, (byte)0xa5);
            Array.Fill(Core, (byte)0xa5);
            Array.Fill(States, (byte)0xa5);
            Latin1("Before").CopyTo(Handle, 0);
            Handle[6] = 0;
            Latin1("Other").CopyTo(Other, 0);
            Other[5] = 0;
        }

        /// <summary>현재 handle 포인터의 같은 전체 저장소를 빌린다.</summary>
        private Span<byte> Current() => OtherSelected ? Other : Handle;

        /// <summary>호출 직전 인자를 기록한 뒤 원본에서 명시한 외부 재진입 효과만 적용한다.</summary>
        /// <param name="name">기록할 이벤트 이름(콜론 앞이 변이 키).</param>
        public void Event(string name)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += name;
            int colon = name.IndexOf(':');
            string key = colon < 0 ? name : name[..colon];
            _counts[key] = _counts.GetValueOrDefault(key) + 1;
            uint count = _counts[key];
            if (Mutation == 1 && key == "translate")
            {
                OtherSelected = true;
            }
            if (Mutation == 2 && key == "title" && count == 1)
            {
                Editor = 1 - Editor;
            }
            if (Mutation == 3 && key == "sp")
            {
                Owner = 2;
            }
            if (Mutation == 4 && key == "core")
            {
                Owner = 3;
            }
            if (Mutation == 5 && key == "prepare" && count == 1)
            {
                Seed = 0xbbbb0000u | (Seed & 0xffffu);
            }
            if (Mutation == 6 && key == "next" && name.StartsWith("next:65536", StringComparison.Ordinal) && count == (InitialEditor == 0 ? 2u : 1u))
            {
                Seed = 0x77770000u | (Seed & 0xffffu);
            }
            if (Mutation == 7 && key == "subscriber")
            {
                Owner = 4;
            }
            if (Mutation == 8 && key == "money")
            {
                Editor = 1;
            }
            if (Mutation == 9 && key == "state")
            {
                Owner = 5;
                Editor = 1 - Editor;
            }
            if (Mutation == 10 && key == "config" && count == 2)
            {
                OtherSelected = true;
            }
            if (Mutation == 11 && key == "register")
            {
                LocalOwner = 3;
            }
            if (Mutation == 12 && key == "coreWrite")
            {
                Seed = 0xdeadffff;
            }
        }

        /// <summary>제품 부모가 각 위치에서 다시 읽을 전역을 같은 장면에 연결한다.</summary>
        public NewFortDataGlobals Globals() => new()
        {
            Subscriber = () => Subscriber,
            SetSubscriber = value => Subscriber = value,
            Handle = () => Current(),
            Editor = () => Editor,
            PlayerKnown = () => Known,
            LocalOwner = () => LocalOwner,
            TemplateOwner = () => Owner,
            SetCoreActive = value => CoreActive = value,
            CoreSeed = () => Seed,
            SetCoreSeed = value => Seed = value
        };

        /// <summary>원본과 같은 큰 외부 하위 경계를 재생한다. CoreData 10바이트 지움은 제품 부모의 실제 본문이 실행한다.</summary>
        public NewFortDataHooks DataHooks() => new()
        {
            Translate = name =>
            {
                Assert.True(name == "Unnamed" && Subscriber == 0xffffffffu);
                Event("translate");
                return "Unnamed";
            },
            Configure = (key, value) => Event("config:" + key + ':' + TextHex(value)),
            Title = () => Event("title"),
            RegisterSubscriber = () =>
            {
                Event("register");
                Subscriber = 0xfedcba98;
            },
            Sp = selected =>
            {
                Event("sp:" + selected);
                return BitConverter.UInt32BitsToSingle(SpBits);
            },
            AddSp = amount => Event("addSp:" + BitConverter.SingleToUInt32Bits(amount) + ':' + LocalOwner),
            CoreData = () =>
            {
                Event("core");
                return Core;
            },
            CoreWrite = selected => Event("coreWrite:" + selected),
            PrepareRandom = () => Event("prepare"),
            NextRandom = bound =>
            {
                Event("next:" + bound);
                return bound == 254 ? Random254 : Randoms[RandomIndex++];
            },
            Subscriber = (selected, id, value) => Event("subscriber:" + selected + ':' + id + ':' + TextHex(Text(value))),
            Territory = (selected, records) =>
            {
                records.CopyTo(Territory);
                Event("territory:" + selected + ':' + Hex(Territory));
            },
            Money = selected => Event("money:" + selected),
            State = selected =>
            {
                Span<byte> result = States.AsSpan((int)selected * 32, 32);
                Event("state:" + selected);
                return result;
            },
            Notify = selected => Event("notify:" + selected)
        };

        /// <summary>파일 부재 부모에서는 두 큰 부모를 별도 fixture가 보장하는 명시 하위로 분리한다.</summary>
        public NewFortMissingFileHooks MissingHooks() => new()
        {
            Path = () =>
            {
                Event("path");
                return FortPath;
            },
            Exists = name =>
            {
                Event("exists:" + TextHex(name));
                return 0;
            },
            Owner = () => Owner,
            Manager = (selected, name, argument) => Event("manager:" + selected + ':' + TextHex(name) + ':' + argument),
            State = selected =>
            {
                Span<byte> result = States.AsSpan((int)selected * 32, 32);
                Event("state:" + selected);
                return result;
            },
            Notify = selected => Event("notify:" + selected),
            Data = () =>
            {
                Assert.Equal(0xabcdef01u, Alternate);
                Event("data");
            },
            Subscriber = selected =>
            {
                Assert.Equal(1u, Alternate);
                Event("subscriberLoad:" + selected + ":5508212:" + TextHex(Text(Current())));
            },
            Handle = () => Text(Current()),
            Configure = (key, value) => Event("config:" + key + ':' + TextHex(value)),
            Title = () => Event("title")
        };
    }

    /// <summary>manager 부분 쓰기·존재 삭제·두 할당·NULL의 실제 중단 위치를 48개 독립 관측에 대조한다.</summary>
    [Fact]
    public void ManagerMatchesNative48Rows()
    {
        string[][] rows = X86Fixture.Read("new-fort-manager");
        Assert.Equal(48, rows.Length);
        List<string> problems = [];
        // 각 행은 기존 ptr/exists/argument와 두 할당의 성공 비트를 따로 제공한다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            byte[] header = new byte[20];
            Array.Fill(header, (byte)0xa5);
            Put(header, 0, Number(row[0]));
            byte[] version = new byte[4];
            Put(version, 0, 0xdeadbeef);
            byte[][] managers = [new byte[276], new byte[276]];
            // 두 manager의 미사용 영역도 변경 없이 전부 대조한다
            foreach (byte[] manager in managers)
            {
                Array.Fill(manager, (byte)0xa5);
            }
            int allocationIndex = 0;
            string trace = "";
            void Record(string value) => trace += (trace.Length != 0 ? ";" : "") + value;
            byte[] Manager(uint token)
            {
                int found = Array.IndexOf(Allocations, token);
                return found >= 0 ? managers[found] : throw new InvalidOperationException("NewFort 검사 manager token");
            }
            NewFortManagerHooks hooks = new()
            {
                Assertion = line => Record("assert:" + line),
                Exists = name =>
                {
                    Record("exists:" + TextHex(name));
                    return (int)Number(row[1]);
                },
                Allocate = size =>
                {
                    Record("allocate:" + size);
                    int selected = allocationIndex++;
                    return row[3][selected] == '1' ? Allocations[selected] : 0u;
                },
                Construct = (token, name) =>
                {
                    Record("construct:" + token + ':' + TextHex(name));
                    RawTemplateLoad.ConstructManager(Manager(token), name, version);
                    return token;
                },
                FileName = token =>
                {
                    Record("fileName:" + token);
                    return RawTemplateLoad.FileName(Manager(token));
                },
                Erase = name => Record("erase:" + TextHex(name)),
                Destroy = token => Record("destroy:" + token),
                Release = token => Record("release:" + token),
                Initialize = token =>
                {
                    Assert.Equal(token, Get(header, 0));
                    Record("initialize:" + token);
                    if (token == 0)
                    {
                        throw new ArgumentOutOfRangeException(null, "원문0044d875 NULL read");
                    }
                }
            };
            bool completed = true;
            try
            {
                RawNewFort.CreateNewFortManager(header, FortPath, Number(row[2]), hooks);
            }
            catch (ArgumentOutOfRangeException)
            {
                completed = false;
            }
            string fault = completed ? "0x0" : trace.EndsWith("initialize:0", StringComparison.Ordinal) ? "0x44d875" : "0x4bfbff";
            if (completed != (Number(row[4]) != 0) || row[5] != fault || Hex(header) != row[6] || Get(version, 0) != Number(row[7])
                || Hex(managers[0]) + ',' + Hex(managers[1]) != row[8] || trace != row[9])
            {
                problems.Add("manager " + string.Join('/', row[0], row[1], row[2], row[3]) + " -> " + trace);
            }
        }
        NoProblems(problems, "NewFort manager");
    }

    /// <summary>Editor/known/SP 비트/seed·0 재시도/외부 재진입 13종의 1,248행 전체 상태와 호출 순서를 대조한다.</summary>
    [Fact]
    public void DataMatchesNative1248Rows()
    {
        string[][] rows = X86Fixture.Read("new-fort-data");
        Assert.Equal(1248, rows.Length);
        List<string> problems = [];
        // 기대값은 fixture만 사용하며 제품 부모의 판단을 검사에서 다시 계산하지 않는다
        foreach (string[] row in rows)
        {
            Assert.Equal(16, row.Length);
            Replay scene = new() { Known = Number(row[1]), SpBits = Number(row[2]), Seed = Number(row[3]), Random254 = Number(row[4]), Mutation = Number(row[6]) };
            scene.InitialEditor = scene.Editor = Number(row[0]);
            // 원본의 명시 난수 시퀀스를 소모하고 남은 개수도 확인한다
            foreach (string random in row[5].Split(','))
            {
                scene.Randoms.Add(Number(random));
            }
            RawNewFort.CreateNewFortData(scene.Globals(), scene.DataHooks());
            if (scene.RandomIndex != scene.Randoms.Count || scene.Subscriber != Number(row[7]) || Hex(scene.Handle.AsSpan(0, 16)) != row[8]
                || Hex(scene.Other.AsSpan(0, 16)) != row[9] || scene.CoreActive != Number(row[10]) || Hex(scene.Core) != row[11]
                || scene.Seed != Number(row[12]) || Hex(scene.States) != row[13] || Hex(scene.Territory) != row[14] || scene.Trace != row[15])
            {
                problems.Add("data " + string.Join('/', row[0], row[1], row[2], row[3], row[4], row[5], row[6]) + " -> " + scene.Trace);
            }
        }
        NoProblems(problems, "NewFort data");
    }

    /// <summary>파일 부재 부모의 두 번 path 조회·State 캡처·Notify 재조회·alternate 확정 순서를 26행에 대조한다.</summary>
    [Fact]
    public void MissingFileMatchesNative26Rows()
    {
        string[][] rows = X86Fixture.Read("new-fort-missing");
        Assert.Equal(26, rows.Length);
        List<string> problems = [];
        // 두 소유자와 같은 외부 변이표를 쓰되 실제 선택된 경계만 효과를 받는다
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            Replay scene = new() { Owner = Number(row[0]), Mutation = Number(row[1]) };
            bool installed = RawNewFort.InstallMissingNewFort(ref scene.Alternate, scene.MissingHooks());
            if (!installed || scene.Alternate != Number(row[2]) || Hex(scene.States) != row[3] || scene.Trace != row[4])
            {
                problems.Add("missing " + row[0] + '/' + row[1] + " -> " + scene.Trace);
            }
        }
        NoProblems(problems, "NewFort missing");
    }

    /// <summary>A5의 bit0은 이미 1이므로 별도 0/A4/FE 입력 78행에서 실제 선택 State만 bit0이 켜지는지 보강한다.</summary>
    [Fact]
    public void MissingStateBitMatchesNative78Rows()
    {
        string[][] rows = X86Fixture.Read("new-fort-missing-bits");
        Assert.Equal(78, rows.Length);
        List<string> problems = [];
        // bit0이 0인 세 전체 패턴과 소유자 재조회 효과를 같은 부모에 전달한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Replay scene = new() { Owner = Number(row[0]), Mutation = Number(row[1]) };
            Array.Fill(scene.States, (byte)Number(row[2]));
            bool installed = RawNewFort.InstallMissingNewFort(ref scene.Alternate, scene.MissingHooks());
            if (!installed || scene.Alternate != Number(row[3]) || Hex(scene.States) != row[4] || scene.Trace != row[5])
            {
                problems.Add("bits " + row[0] + '/' + row[1] + '/' + row[2] + " -> " + scene.Trace);
            }
        }
        NoProblems(problems, "NewFort missing bits");
    }

    /// <summary>
    /// 원문이 쓰는 물리 범위(handle 8바이트, CoreData 10바이트, State 18바이트, 부재 State 1바이트)의 정확한 경계를 확인한다:
    /// 한 바이트 모자라면 범위 예외로 멈추고 딱 맞으면 성공한다. fixture는 항상 넉넉한 버퍼를 쓰므로 경계는 직접 검사로 닫는다.
    /// </summary>
    [Fact]
    public void PhysicalRangesAreCheckedAtExactBoundaries()
    {
        // "Unnamed"와 NUL이 들어갈 8바이트가 필요하다
        Replay narrow = new();
        NewFortDataGlobals narrowGlobals = narrow.Globals();
        byte[] seven = new byte[7];
        narrowGlobals.Handle = () => seven;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNewFort.CreateNewFortData(narrowGlobals, narrow.DataHooks()));
        Replay exact = new() { Random254 = 3 };
        exact.Randoms.Add(0x1234);
        NewFortDataGlobals exactGlobals = exact.Globals();
        byte[] eight = new byte[8];
        exactGlobals.Handle = () => eight;
        RawNewFort.CreateNewFortData(exactGlobals, exact.DataHooks());
        Assert.Equal("556e6e616d656400", Hex(eight));
        // CoreData는 10바이트를 지운다
        Replay shortCore = new();
        shortCore.Randoms.Add(0x1234);
        NewFortDataHooks shortCoreHooks = shortCore.DataHooks();
        shortCoreHooks.CoreData = () => new byte[9];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNewFort.CreateNewFortData(shortCore.Globals(), shortCoreHooks));
        Replay fullCore = new();
        fullCore.Randoms.Add(0x1234);
        NewFortDataHooks fullCoreHooks = fullCore.DataHooks();
        fullCoreHooks.CoreData = () => new byte[10];
        RawNewFort.CreateNewFortData(fullCore.Globals(), fullCoreHooks);
        // State는 +0x11을 쓰므로 18바이트가 필요하다
        Replay shortState = new();
        shortState.Randoms.Add(0x1234);
        NewFortDataHooks shortStateHooks = shortState.DataHooks();
        shortStateHooks.State = _ => new byte[17];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNewFort.CreateNewFortData(shortState.Globals(), shortStateHooks));
        Replay fullState = new();
        fullState.Randoms.Add(0x1234);
        byte[] eighteen = new byte[18];
        NewFortDataHooks fullStateHooks = fullState.DataHooks();
        fullStateHooks.State = _ => eighteen;
        RawNewFort.CreateNewFortData(fullState.Globals(), fullStateHooks);
        Assert.Equal(2, eighteen[17]);
        // 파일 부재 부모는 State 첫 바이트가 필요하다
        Replay emptyState = new();
        NewFortMissingFileHooks emptyHooks = emptyState.MissingHooks();
        emptyHooks.State = _ => Array.Empty<byte>();
        uint alternate = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNewFort.InstallMissingNewFort(ref alternate, emptyHooks));
        Assert.Equal(0u, alternate);
        Replay oneByte = new();
        NewFortMissingFileHooks oneByteHooks = oneByte.MissingHooks();
        byte[] single = [0];
        oneByteHooks.State = _ => single;
        Assert.True(RawNewFort.InstallMissingNewFort(ref oneByte.Alternate, oneByteHooks));
        Assert.Equal(1, single[0]);
    }

    /// <summary>기존 파일이면 아무 신규 효과도 없이 false로 끝나고, 미연결 경계는 빈 성공이 아니라 명시 오류로 멈춘다.</summary>
    [Fact]
    public void ExistingFileAndMissingDevicesAreExplicit()
    {
        Replay scene = new();
        NewFortMissingFileHooks hooks = scene.MissingHooks();
        hooks.Exists = name =>
        {
            scene.Event("exists:" + TextHex(name));
            return 5;
        };
        uint alternate = 7;
        Assert.False(RawNewFort.InstallMissingNewFort(ref alternate, hooks));
        Assert.Equal(7u, alternate);
        Assert.Equal("path;exists:" + TextHex(FortPath), scene.Trace);
        Assert.Throws<InvalidOperationException>(() => RawNewFort.CreateNewFortData(scene.Globals(), new NewFortDataHooks()));
        Assert.Throws<ArgumentException>(() => RawNewFort.CreateNewFortManager(new byte[19], FortPath, 0, new NewFortManagerHooks()));
    }
}
