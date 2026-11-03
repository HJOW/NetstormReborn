using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// Fort CoreData 저장 검사를 같은 테스트 네임스페이스에 둔다. (F2c-6, 2026-11-03)
namespace Netstorm.Core.Tests;

/// <summary>
/// Fort CoreData 저장(<see cref="RawFortCoreWrite"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortCoreWriteTests</c>의 전수 338행·중단 16, 추가 22행·중단 2,
/// 콜백 36행·중단 6과 선택 장치 경계 직접 2종을 옮겼다. (F2c-6, 2026-11-03)
/// </summary>
public sealed class FortCoreWriteTests
{
    /// <summary>
    /// 원래 빈 칸과 숫자열의 순서를 보존해 독립 입력과 실측 열을 나눈다.
    /// </summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] CoreWriteFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 마지막 빈 칸도 하나의 열로 남긴다
        for (;;)
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>
    /// 현재 물리 바이트를 원문과 같은 소문자 hex로 표현한다.
    /// </summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string CoreWriteHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 바이트당 두 글자의 고정표로 이어 붙인다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>
    /// seed 직접 읽기는 장치 없이 빌리므로 혹시 있을 seed 항목만 걷어낸다.
    /// </summary>
    /// <param name="trace">걷어낼 trace 원문이다.</param>
    private static string CoreWriteWithoutSeed(string trace)
    {
        StringBuilder result = new();
        // 직접 참조 읽기에 인위 장치를 끼우지 않고 순서만 보존한다
        foreach (string item in CoreWriteFields(trace, ';'))
        {
            if (item.StartsWith("seed:", StringComparison.Ordinal))
            {
                continue;
            }
            if (result.Length != 0)
            {
                result.Append(';');
            }
            result.Append(item);
        }
        return result.ToString();
    }

    /// <summary>원문의 명시 외부 중단이며 예상 밖 표준 예외를 삼키지 않는다.</summary>
    internal sealed class CoreWriteStop : Exception;

    /// <summary>최초 관측의 입력과 명시 callback 효과만 구성한다. 기대 열은 실행 뒤 별도 비교한다.</summary>
    private sealed class CoreWriteDevices
    {
        /// <summary>고정 this 슬롯이다.</summary>
        public int Slot;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>현재 metamap(00594fc8)이다.</summary>
        public uint Metamap;

        /// <summary>현재 mission post(00594fa4)다.</summary>
        public uint Post;

        /// <summary>현재 editor(005c85a4)다.</summary>
        public uint Editor;

        /// <summary>현재 player owner(00540c70)다.</summary>
        public uint Owner;

        /// <summary>현재 seed DWORD(005c89b4)다.</summary>
        public uint SeedValue;

        /// <summary>명시 난수 입력 번호다.</summary>
        public uint Sequence;

        /// <summary>현재 Player 기준 번호(1 또는 2)다.</summary>
        public uint Active = 1;

        /// <summary>Create 뒤 옛 물리가 살아 있는 두 9×180바이트 물리다.</summary>
        public byte[][] Players = [new byte[1620], new byte[1620]];

        /// <summary>경계별 실제 호출 횟수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>명시 변경 입력이다.</summary>
        public string Mutation = "";

        /// <summary>전체 현재 trace다.</summary>
        public string Trace = "";

        /// <summary>일반 write 부분 출력이다.</summary>
        public List<byte> Output = [];

        /// <summary>
        /// 독립 입력의 입력 열만 읽고 제품의 분기나 기대를 계산하지 않는다.
        /// </summary>
        /// <param name="input">쉼표로 나눈 독립 입력 9열이다.</param>
        public CoreWriteDevices(string input)
        {
            string[] fields = CoreWriteFields(input, ',');
            if (fields.Length != 9)
            {
                throw new InvalidOperationException("CoreWrite입력열수오류");
            }
            Slot = int.Parse(fields[0], CultureInfo.InvariantCulture);
            Manager = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            Metamap = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            Post = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            Editor = uint.Parse(fields[4], CultureInfo.InvariantCulture);
            Owner = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            SeedValue = uint.Parse(fields[6], CultureInfo.InvariantCulture);
            Sequence = uint.Parse(fields[7], CultureInfo.InvariantCulture);
            Mutation = fields[8];
            // 최초 실행에 준비한 동일 물리 입력만 재구성한다
            for (int block = 0; block < Players.Length; block++)
            {
                // 같은 고정 CoreData 10바이트만 명시한다
                for (int index = 0; index < 9; index++)
                {
                    // 나머지 Player 바이트는 부모가 접근하지 않는다
                    for (int b = 0; b < 10; b++)
                    {
                        Players[block][index * 180 + 108 + b] = unchecked((byte)(block * 79 + index * 17 + b * 13));
                    }
                }
            }
        }

        /// <summary>
        /// 직접 현재 getter와 callback의 실제 발생 순서만 기록한다.
        /// </summary>
        /// <param name="kind">셀 경계 이름이다.</param>
        /// <param name="text">기록 본문이다.</param>
        public int Event(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            int count = Calls.GetValueOrDefault(kind) + 1;
            Calls[kind] = count;
            return count;
        }

        /// <summary>
        /// 최초 독립 실행에 준비한 외부 변경 입력만 동일하게 공급한다.
        /// </summary>
        /// <param name="boundary">변경이 걸린 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            int dash = Mutation.IndexOf('-');
            if (dash < 0)
            {
                return;
            }
            if (!string.Equals(Mutation.Substring(0, dash), boundary, StringComparison.Ordinal))
            {
                return;
            }
            string action = Mutation.Substring(dash + 1);
            if (action == "stop")
            {
                throw new CoreWriteStop();
            }
            else if (action.StartsWith("manager", StringComparison.Ordinal))
            {
                Manager = uint.Parse(action.Substring(7), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("post", StringComparison.Ordinal))
            {
                Post = uint.Parse(action.Substring(4), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("meta", StringComparison.Ordinal))
            {
                Metamap = uint.Parse(action.Substring(4), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("editor", StringComparison.Ordinal))
            {
                Editor = uint.Parse(action.Substring(6), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("owner", StringComparison.Ordinal))
            {
                Owner = uint.Parse(action.Substring(5), CultureInfo.InvariantCulture);
            }
            else if (action == "playerB")
            {
                Active = 2;
            }
            else if (action == "core")
            {
                string kind = boundary;
                // 경계 이름 뒤의 호출 번호를 걷어낸다
                while (kind.Length > 0 && char.IsDigit(kind[^1]))
                {
                    kind = kind[..^1];
                }
                int count = Calls.GetValueOrDefault(kind);
                // 옛 물리와 현재 물리를 모두 원문 입력과 같은 값으로 명시 변경한다
                foreach (byte[] block in Players)
                {
                    // 각 기록은 서로 다른 10바이트 입력이다
                    for (int index = 0; index < 9; index++)
                    {
                        // 뒤 필드는 각 child 뒤 실제로 다시 읽어야 한다
                        for (int b = 0; b < 10; b++)
                        {
                            block[index * 180 + 108 + b] = unchecked((byte)(count * 23 + index * 17 + b * 11));
                        }
                    }
                }
            }
            else if (action.StartsWith("seed", StringComparison.Ordinal))
            {
                string digits = action.Substring(4);
                SeedValue = action == "seed12340000" || action == "seed80000000"
                    ? Convert.ToUInt32(digits, 16)
                    : uint.Parse(digits, CultureInfo.InvariantCulture);
            }
            else
            {
                throw new InvalidOperationException("CoreWrite명시변경미해석");
            }
        }

        /// <summary>
        /// 같은 현재 Player 전역과 seed DWORD를 빌리는 실제 getter다.
        /// </summary>
        public FortCoreWriteGlobals Globals()
        {
            return new FortCoreWriteGlobals
            {
                Metamap = () =>
                {
                    Event("metamap", "metamap:4:" + Metamap.ToString(CultureInfo.InvariantCulture));
                    return Metamap;
                },
                MissionPost = () =>
                {
                    Event("post", "post:4:" + Post.ToString(CultureInfo.InvariantCulture));
                    return Post;
                },
                Editor = () =>
                {
                    Event("editor", "editor:4:" + Editor.ToString(CultureInfo.InvariantCulture));
                    return Editor;
                },
                Players = () =>
                {
                    Event("players", "players:4:" + Active.ToString(CultureInfo.InvariantCulture));
                    return Players[Active - 1].AsSpan();
                },
                PlayerOwner = () =>
                {
                    Event("owner", "owner:4:" + Owner.ToString(CultureInfo.InvariantCulture));
                    return Owner;
                },
                // seed는 직접 참조 읽기와 같아 Event를 기록하지 않는다
                Seed = () => SeedValue,
                SetSeed = value => SeedValue = value,
            };
        }

        /// <summary>
        /// 관측한 원문 외부 하위의 응답만 제공한다. 실제 writer와 난수 복원을 대신하는 기본값이 아니다.
        /// </summary>
        public FortCoreWriteHooks Hooks()
        {
            return new FortCoreWriteHooks
            {
                Manager = selected =>
                {
                    Assert.Equal(Slot, selected);
                    uint captured = Manager;
                    Event("manager", "manager:" + captured.ToString(CultureInfo.InvariantCulture));
                    return captured;
                },
                Assertion = reason =>
                {
                    int number = Event("assert", "A:" + ((uint)reason).ToString(CultureInfo.InvariantCulture));
                    Mutate("assert" + number.ToString(CultureInfo.InvariantCulture));
                },
                Create = (current, name) =>
                {
                    int number = Event("create", "C:" + current.ToString(CultureInfo.InvariantCulture) + ":" + name);
                    Mutate("create" + number.ToString(CultureInfo.InvariantCulture));
                    return 101u;
                },
                Refresh = () =>
                {
                    int number = Event("refresh", "F");
                    Mutate("refresh" + number.ToString(CultureInfo.InvariantCulture));
                },
                Write = (stream, bytes) =>
                {
                    // 부분 출력을 바이트대로 보존한다
                    foreach (byte value in bytes)
                    {
                        Output.Add(value);
                    }
                    int number = Event("write", "W:" + stream.ToString(CultureInfo.InvariantCulture) + ":" + bytes.Length.ToString(CultureInfo.InvariantCulture) + ":" + CoreWriteHex(bytes));
                    Mutate("write" + number.ToString(CultureInfo.InvariantCulture));
                },
                PrepareRandom = () =>
                {
                    int number = Event("prepare", "P");
                    Mutate("prepare" + number.ToString(CultureInfo.InvariantCulture));
                },
                NextRandom = maximum =>
                {
                    uint[] zeros = [0, 65536, 0x12348001u];
                    // 각 원문 반환의 low16 변환은 제품이 수행한다
                    int prior = Calls.GetValueOrDefault("random");
                    uint result = Sequence == 0 ? zeros[Math.Min(prior, 2)] : Sequence == 1 ? 0xffffffffu : 1u;
                    int number = Event("random", "R:" + maximum.ToString(CultureInfo.InvariantCulture) + ":" + result.ToString(CultureInfo.InvariantCulture));
                    Mutate("random" + number.ToString(CultureInfo.InvariantCulture));
                    return result;
                },
                Sp = (stream, value) =>
                {
                    int number = Event("sp", "S:" + stream.ToString(CultureInfo.InvariantCulture) + ":" + BitConverter.SingleToUInt32Bits(value).ToString(CultureInfo.InvariantCulture));
                    Mutate("sp" + number.ToString(CultureInfo.InvariantCulture));
                },
            };
        }

        /// <summary>
        /// 전체 현재 전역의 원시 DWORD와 현재 Player 기준 번호를 표현한다.
        /// </summary>
        public string State()
        {
            return string.Join(',', new string[]
            {
                Manager.ToString(CultureInfo.InvariantCulture),
                Metamap.ToString(CultureInfo.InvariantCulture),
                Post.ToString(CultureInfo.InvariantCulture),
                Editor.ToString(CultureInfo.InvariantCulture),
                Owner.ToString(CultureInfo.InvariantCulture),
                SeedValue.ToString(CultureInfo.InvariantCulture),
                Active.ToString(CultureInfo.InvariantCulture),
            });
        }

        /// <summary>
        /// 사용하지 않은 Player와 이전 할당의 CoreData도 모두 대조한다.
        /// </summary>
        public string Cores()
        {
            StringBuilder result = new();
            // 두 살아 있는 할당의 원시 순서를 보존한다
            foreach (byte[] block in Players)
            {
                // 같은 기록의 10바이트 전체다
                for (int index = 0; index < 9; index++)
                {
                    result.Append(CoreWriteHex(new ReadOnlySpan<byte>(block, index * 180 + 108, 10)));
                }
            }
            return result.ToString();
        }
    }

    /// <summary>
    /// 원문 fixture 한 개를 제품 실행 뒤에만 기대 열과 비교한다.
    /// </summary>
    /// <param name="name">읽을 fixture 이름이다.</param>
    /// <param name="expected">기대하는 전체 행 수다.</param>
    /// <param name="expectedStops">기대하는 명시 중단 수다.</param>
    private static void CompareCoreWriteFixture(string name, int expected, int expectedStops)
    {
        string[][] rows = X86Fixture.Read(name);
        int compared = 0;
        int stops = 0;
        HashSet<(string, string)> seen = [];
        // 실제 관측의 두 정밀도와 모든 부분 중단을 빠짐없이 대조한다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            Assert.True(seen.Add((row[0], row[1])));
            CoreWriteDevices devices = new(row[1]);
            bool completed = true;
            try
            {
                RawFortCoreWrite.WriteFortCoreData(devices.Slot, devices.Globals(), devices.Hooks());
            }
            catch (CoreWriteStop)
            {
                completed = false;
                stops++;
            }
            Assert.Equal(row[2] == "normal", completed);
            Assert.Equal(row[3], devices.State());
            Assert.Equal(row[4], devices.Cores());
            Assert.Equal(row[5], devices.Output.Count == 0 ? "-" : CoreWriteHex(devices.Output.ToArray()));
            Assert.Equal(CoreWriteWithoutSeed(row[6]), devices.Trace);
            compared++;
        }
        Assert.Equal(expected, compared);
        Assert.Equal(expectedStops, stops);
    }

    /// <summary>최초 169입력의 두 정밀도 338관측을 전체 원시 상태와 대조한다.</summary>
    [Fact]
    public void IndependentNative338()
    {
        CompareCoreWriteFixture("fort-core-write", 338, 16);
    }

    /// <summary>별도 고정한 세 번째 진단과 상위 비영 DWORD 22관측을 대조한다.</summary>
    [Fact]
    public void IndependentNativeExtra22()
    {
        CompareCoreWriteFixture("fort-core-write-extra", 22, 2);
    }

    /// <summary>실제 1기준으로 선택한 Create와 refresh, prepare 콜백의 별도 36관측을 대조한다.</summary>
    [Fact]
    public void IndependentNativeCallbacks36()
    {
        CompareCoreWriteFixture("fort-core-write-callbacks", 36, 6);
    }

    /// <summary>자연 관문은 미선택 장치를 요구하지 않고 실제 선택 시에는 그 위치에서 부분 효과와 함께 거부한다.</summary>
    [Fact]
    public void SelectedMissingDeviceBoundaries()
    {
        CoreWriteDevices natural = new("1,0,1,1,0,1,0,0,none");
        FortCoreWriteHooks skip = natural.Hooks();
        skip.Create = null;
        skip.Refresh = null;
        skip.Write = null;
        skip.PrepareRandom = null;
        skip.NextRandom = null;
        skip.Sp = null;
        FortCoreWriteGlobals globals = natural.Globals();
        globals.Editor = null;
        globals.Players = null;
        globals.PlayerOwner = null;
        globals.Seed = null;
        globals.SetSeed = null;
        RawFortCoreWrite.WriteFortCoreData(1, globals, skip);
        Assert.Equal(2, natural.Calls.GetValueOrDefault("assert"));
        Assert.Equal(0, natural.Calls.GetValueOrDefault("create"));
        Assert.Equal(0, natural.Calls.GetValueOrDefault("editor"));
        Assert.Equal(0u, natural.SeedValue);
        // 선택 하위만 순서별 제거하여 앞선 효과를 확인한다
        for (int stage = 0; stage < 7; stage++)
        {
            CoreWriteDevices selected = new("1,11,0,0,0,1,0,0,none");
            FortCoreWriteHooks hooks = selected.Hooks();
            FortCoreWriteGlobals current = selected.Globals();
            if (stage == 0)
            {
                hooks.Create = null;
            }
            else if (stage == 1)
            {
                hooks.Refresh = null;
            }
            else if (stage == 2)
            {
                hooks.Write = null;
            }
            else if (stage == 3)
            {
                current.Seed = null;
                current.SetSeed = null;
            }
            else if (stage == 4)
            {
                hooks.PrepareRandom = null;
            }
            else if (stage == 5)
            {
                hooks.NextRandom = null;
            }
            else
            {
                hooks.Sp = null;
            }
            bool rejected = false;
            try
            {
                RawFortCoreWrite.WriteFortCoreData(1, current, hooks);
            }
            catch (InvalidOperationException)
            {
                rejected = true;
            }
            Assert.True(rejected);
            Assert.Equal(stage == 0 ? 0 : 1, selected.Calls.GetValueOrDefault("create"));
            Assert.Equal(stage >= 2 ? 1 : 0, selected.Calls.GetValueOrDefault("refresh"));
            Assert.Equal(stage <= 2 ? 0 : stage < 6 ? 10 : 12, selected.Output.Count);
            Assert.Equal(stage == 6 ? 32769u : 0u, selected.SeedValue);
        }
    }

    /// <summary>고정 Player 주소의 물리 폭 검사는 자연 반환보다 앞당기지 않고 선행 refresh 부분 효과도 보존한다.</summary>
    [Fact]
    public void PhysicalReadAfterSelectedRefresh()
    {
        CoreWriteDevices devices = new("1,11,0,0,1,1,0,0,none");
        FortCoreWriteGlobals globals = devices.Globals();
        globals.Players = () => Span<byte>.Empty;
        RawFortCoreWrite.WriteFortCoreData(1, globals, devices.Hooks());
        Assert.Equal(0, devices.Calls.GetValueOrDefault("create"));
        devices.Editor = 0;
        bool rejected = false;
        try
        {
            RawFortCoreWrite.WriteFortCoreData(1, globals, devices.Hooks());
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(1, devices.Calls.GetValueOrDefault("create"));
        Assert.Equal(1, devices.Calls.GetValueOrDefault("refresh"));
        Assert.Empty(devices.Output);
        Assert.Equal(0u, devices.SeedValue);
    }
}
