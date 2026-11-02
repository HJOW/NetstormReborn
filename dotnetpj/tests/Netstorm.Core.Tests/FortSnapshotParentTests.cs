using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 전체 Fort·Battle 스냅샷 부모(<see cref="RawFortSnapshotParent"/>)를 독립 원본 입력과 대조한다.
/// cpppj <c>FortSnapshotParentTests</c>의 전수 2728행과 선택 직접 1종을 옮겼다. (F2c-4, 2026-11-03)
/// </summary>
public sealed class FortSnapshotParentTests
{
    /// <summary>최초 manager 11·22의 격리 포인터다.</summary>
    private const uint SnapshotManager11 = 0x12000100u;

    /// <summary>최초 manager 22의 격리 포인터다.</summary>
    private const uint SnapshotManager22 = 0x12000200u;

    /// <summary>부모에 호출되는 ID를 실제 가변 토큰으로 변환한다.</summary>
    /// <param name="manager">ID다.</param>
    private static uint SnapshotPointer(uint manager)
    {
        if (manager == 0)
        {
            return 0;
        }
        if (manager == 11)
        {
            return SnapshotManager11;
        }
        if (manager == 22)
        {
            return SnapshotManager22;
        }
        throw new ArgumentOutOfRangeException(nameof(manager), manager, "Snapshot manager ID 범위");
    }

    /// <summary>원본 포인터를 실제 가변 ID만 명시 장치로 바꾼다.</summary>
    /// <param name="pointer">포인터 값이다.</param>
    private static uint SnapshotManager(uint pointer)
    {
        if (pointer == 0)
        {
            return 0;
        }
        if (pointer == SnapshotManager11)
        {
            return 11;
        }
        if (pointer == SnapshotManager22)
        {
            return 22;
        }
        throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Snapshot manager 포인터 범위");
    }

    /// <summary>마지막 빈 칸도 보존하는 최초 fixture 입력·출력 행위 분리다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] SnapshotFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 각 구분자의 다음 위치에서 원문 그대로 복사한다
        for (; ; )
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

    /// <summary>Template의 현재 DWORD를 네 바이트부터 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint SnapshotGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 지정 네 바이트만 조합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>Template의 manager DWORD를 쓰고 부가 필드를 보존한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void SnapshotPut(Span<byte> bytes, int at, uint value)
    {
        // 원본 네 바이트만 사용한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 CPU와 명시 callback이 다른 전체 Template 메모리 읽기를 빠짐없이 현시한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string SnapshotHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 최초 관측의 원문 바이트 현시다
        for (int index = 0; index < bytes.Length; index++)
        {
            byte value = bytes[index];
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>독립 입력을 명시 답안으로 공급한다. 부분 순서·관문은 제품 함수가 결정한다.</summary>
    internal sealed class SnapshotStop : Exception;

    /// <summary>최초 관측의 입력을 명시 답안으로 공급한다. 부분 분기·getter는 제품 함수가 결정한다.</summary>
    private sealed class SnapshotDevices
    {
        /// <summary>입력 kind다.</summary>
        public uint Kind;

        /// <summary>고정 this와 현재 부모의 실제 this다.</summary>
        public int Slot;

        /// <summary>활성 slot이다.</summary>
        public int ActiveSlot;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>현재 owner다.</summary>
        public uint Owner;

        /// <summary>현재 fort 모드다.</summary>
        public uint Fort;

        /// <summary>현재 battle 모드다.</summary>
        public uint Battle;

        /// <summary>부모 ID다.</summary>
        public uint Parent = 111;

        /// <summary>만든 수다.</summary>
        public uint Created;

        /// <summary>-2..9의 현재 12기록×20바이트다. 부가 필드도 검증한다.</summary>
        public byte[] Headers = new byte[240];

        /// <summary>Create·current 변경용 명시 DataManager 답안이다.</summary>
        public Dictionary<uint, uint> Currents = new() { [0] = 90, [11] = 91, [22] = 92 };

        /// <summary>호출된 실제 child 수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>입력 효과 이름이다.</summary>
        public string Mutation = "";

        /// <summary>입력 trace다.</summary>
        public string Trace = "";

        /// <summary>최초 fixture 입력만 파싱하고 관측 기대값은 쓰지 않는다.</summary>
        /// <param name="input">쉼표 7칸 입력이다.</param>
        public SnapshotDevices(string input)
        {
            string[] fields = SnapshotFields(input, ',');
            if (fields.Length != 7)
            {
                throw new InvalidOperationException("Snapshot 부모 입력 칸 수 오류");
            }
            Kind = uint.Parse(fields[0], CultureInfo.InvariantCulture);
            Slot = int.Parse(fields[1], CultureInfo.InvariantCulture);
            Manager = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            Owner = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            Fort = uint.Parse(fields[4], CultureInfo.InvariantCulture);
            Battle = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            Mutation = fields[6];
            ActiveSlot = Slot;
            for (int selected = -2; selected <= 9; selected++)
            {
                // 모든 관련 Template을 원문 입력과 같은 패턴으로 초기화한다
                int at = (selected + 2) * 20;
                for (int offset = 0; offset < 20; offset++)
                {
                    Headers[at + offset] = (byte)((selected + 2) * 13 + offset);
                }
                SnapshotPut(Headers, at, SnapshotPointer((uint)selected % 2 == 0 ? 22u : 11u));
            }
            SnapshotPut(Headers, (Slot + 2) * 20, SnapshotPointer(Manager));
        }

        /// <summary>현재 호출을 같은 순서대로 trace로 보관한다.</summary>
        /// <param name="text">기록할 본문이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>현재 호출을 같은 순서의 child 진입 trace로 보관한다.</summary>
        /// <param name="kind">기록 종류다.</param>
        /// <param name="text">기록 본문이다.</param>
        public string Event(string kind, string text)
        {
            Record(text);
            int count = Calls.GetValueOrDefault(kind) + 1;
            Calls[kind] = count;
            return kind + count.ToString(CultureInfo.InvariantCulture);
        }

        /// <summary>지정된 외부 경계를 입력 효과로 적용한다. 부모의 다음 분기·getter는 건드리지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            int dash = Mutation.LastIndexOf('-');
            if (dash < 0 || Mutation.Substring(0, dash) != boundary)
            {
                return;
            }
            string action = Mutation.Substring(dash + 1);
            if (action.StartsWith("manager", StringComparison.Ordinal))
            {
                SnapshotPut(Headers, (ActiveSlot + 2) * 20, SnapshotPointer(uint.Parse(action.Substring(7), CultureInfo.InvariantCulture)));
            }
            else if (action.StartsWith("owner", StringComparison.Ordinal))
            {
                Owner = uint.Parse(action.Substring(5), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("fort", StringComparison.Ordinal))
            {
                Fort = uint.Parse(action.Substring(4), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("battle", StringComparison.Ordinal))
            {
                Battle = uint.Parse(action.Substring(6), CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("parent", StringComparison.Ordinal))
            {
                Parent = uint.Parse(action.Substring(6), CultureInfo.InvariantCulture);
            }
            else if (action == "current999")
            {
                foreach (uint key in new List<uint>(Currents.Keys))
                {
                    Currents[key] = 999;
                }
            }
            else if (action == "stop")
            {
                throw new SnapshotStop();
            }
        }

        /// <summary>원본의 현재 영역 읽기를 같은 위치 getter trace로 남긴다.</summary>
        public FortSnapshotParentGlobals Globals()
        {
            return new FortSnapshotParentGlobals
            {
                TemplateOwner = () =>
                {
                    Record("owner:" + Owner);
                    return Owner;
                },
                Fort = () =>
                {
                    Record("fort:" + Fort);
                    return Fort;
                },
                Battle = () =>
                {
                    Record("battle:" + Battle);
                    return Battle;
                },
            };
        }

        /// <summary>같은 this child를 명시 답안으로 받고 슬롯으로 기록한다.</summary>
        /// <param name="name">child 이름이다.</param>
        /// <param name="selected">고를 슬롯 번호다.</param>
        public void SlotChild(string name, int selected)
        {
            Assert.Equal(ActiveSlot, selected);
            string boundary = Event(name, name + ':' + selected);
            Mutate(boundary);
        }

        /// <summary>현재 native CALL 규격에 맞는 실제 child를 구성한다.</summary>
        public FortSnapshotParentHooks Hooks()
        {
            return new FortSnapshotParentHooks
            {
                Manager = selected =>
                {
                    if (selected < -2 || selected > 9)
                    {
                        throw new ArgumentOutOfRangeException(nameof(selected), selected, "Snapshot 명시 Template 물리 범위");
                    }
                    ActiveSlot = selected;
                    uint result = SnapshotManager(SnapshotGet(Headers, (selected + 2) * 20));
                    Record("manager:" + selected + ':' + result);
                    return result;
                },
                Assertion = reason =>
                {
                    string boundary = Event("assert", "assert:" + (uint)reason);
                    Mutate(boundary);
                },
                TechnologyParent = () =>
                {
                    uint result = Parent;
                    string boundary = Event("parent", "parent:" + result);
                    Mutate(boundary);
                    return Mutation == boundary + "-return0" ? 0u : result;
                },
                Create = (selected, name) =>
                {
                    Created++;
                    string boundary = Event("create", "create:" + selected + ':' + name + ':' + Created);
                    Currents[selected] = Created;
                    Mutate(boundary);
                    return Mutation == boundary + "-return0" ? 0u : Created;
                },
                Technology = (selected, stream) =>
                {
                    string boundary = Event("technology", "technology:" + selected + ':' + stream);
                    Mutate(boundary);
                },
                Money = stream =>
                {
                    string boundary = Event("money", "money:" + stream);
                    Mutate(boundary);
                },
                Mission = stream =>
                {
                    string boundary = Event("mission", "mission:" + stream);
                    Mutate(boundary);
                },
                TypeNames = selected => SlotChild("names", selected),
                Deck = selected => SlotChild("deck", selected),
                Objects = selected => SlotChild("objects", selected),
                Territory = selected => SlotChild("territory", selected),
                Badges = selected => SlotChild("badges", selected),
                CoreData = selected => SlotChild("core", selected),
                Notify = selected => SlotChild("notify", selected),
            };
        }

        /// <summary>입력 kind에 따라 제품 진입 하나만 실행한다.</summary>
        /// <param name="hooks">실제 조회·생성 경계다.</param>
        public void Run(FortSnapshotParentHooks hooks)
        {
            FortSnapshotParentGlobals globals = Globals();
            switch (Kind)
            {
                case 0:
                    RawFortSnapshotParent.SnapshotFortTemplate(Slot, globals, hooks);
                    break;
                case 1:
                    RawFortSnapshotParent.SnapshotBattleTemplate(Slot, globals, hooks);
                    break;
                case 2:
                    RawFortSnapshotParent.SnapshotCurrentTemplate(globals, hooks);
                    break;
                case 3:
                    RawFortSnapshotParent.SnapshotCurrentTemplateIfPresent(globals, hooks);
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(Kind), Kind, "Snapshot 부모 kind 범위");
            }
        }

        /// <summary>전체 현재 영역 8 DWORD를 최초 원문과 같은 순서로 현시한다.</summary>
        public string State()
        {
            return Owner + "," + Fort + ',' + Battle + ',' + Parent + ',' + Created + ',' + Currents[0] + ',' + Currents[11] + ',' + Currents[22];
        }
    }

    /// <summary>최초 1364입력×정밀도의 정상·부분 중단과 getter 순서·전체 메모리를 비교한다.</summary>
    [Fact]
    public void IndependentNative2728()
    {
        string[][] rows = X86Fixture.Read("fort-snapshot-parent");
        int compared = 0;
        int stopped = 0;
        HashSet<string> unique = [];
        // 관측열은 제품 실행을 마친 뒤에 비교에만 쓴다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Assert.True(unique.Add(row[0] + "\t" + row[1]));
            SnapshotDevices devices = new(row[1]);
            bool completed = true;
            try
            {
                devices.Run(devices.Hooks());
            }
            catch (SnapshotStop)
            {
                completed = false;
                stopped++;
            }
            Assert.Equal(completed, row[2] == "1");
            Assert.Equal(devices.State(), row[3]);
            Assert.Equal(SnapshotHex(devices.Headers), row[4]);
            Assert.Equal(devices.Trace, row[5]);
            compared++;
        }
        Assert.Equal(2728, compared);
        Assert.Equal(378, stopped);
    }

    /// <summary>두 부모의 미선택 child는 요구하지 않고 전달된 단절 child는 선행 실제 효과 뒤에 실패한다.</summary>
    [Fact]
    public void SelectedAndMissingDevices()
    {
        SnapshotDevices fort = new("0,1,11,1,1,1,none");
        FortSnapshotParentHooks hooks = fort.Hooks();
        hooks.CoreData = null;
        fort.Run(hooks);
        Assert.Equal(1, fort.Calls["badges"]);
        Assert.Equal(0, fort.Calls.GetValueOrDefault("core"));
        SnapshotDevices battle = new("1,1,11,1,1,1,none");
        hooks = battle.Hooks();
        hooks.TypeNames = null;
        hooks.Badges = null;
        battle.Run(hooks);
        Assert.Equal(0, battle.Calls.GetValueOrDefault("names"));
        Assert.Equal(0, battle.Calls.GetValueOrDefault("badges"));
        Assert.Equal(1, battle.Calls["core"]);
        SnapshotDevices missing = new("0,1,11,1,1,1,none");
        hooks = missing.Hooks();
        hooks.Create = null;
        bool rejected = false;
        try
        {
            missing.Run(hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected && missing.Calls["names"] == 1 && missing.Calls["parent"] == 1 && missing.Created == 0);
        SnapshotDevices absent = new("3,1,0,1,1,1,none");
        hooks = new FortSnapshotParentHooks
        {
            Manager = absent.Hooks().Manager,
        };
        absent.Run(hooks);
        Assert.Equal("owner:1;manager:1:0", absent.Trace);
    }
}
