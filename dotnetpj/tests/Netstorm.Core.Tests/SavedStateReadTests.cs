// 저장 상태 파일 복원의 538행 전수 재생과 선택 장치·물리 계약을 검증한다.
using System.Text;
using System.Text.RegularExpressions;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawSavedState 판독 본체를 검증하는 테스트 클래스다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateReadTests
{
    /// <summary>최초 도구의 부분 중단 표식이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    internal sealed class SavedStopException : Exception
    {
    }

    /// <summary>원문 도구의 입력/하위 응답만 제공하는 장면이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    internal sealed class SavedDevices
    {
        /// <summary>파일 존재 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int Present;

        /// <summary>이름·참가자 판본이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint Profile;

        /// <summary>참가자 입력 수다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint ParticipantInput;

        /// <summary>꼬리 DWORD 입력이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint TailInput;

        /// <summary>저장 시각 입력이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint TimeInput;

        /// <summary>파일 길이 입력이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint Length;

        /// <summary>stack 초기 판본이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint Initial;

        /// <summary>입력 외부 변경이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public string Mutation = "none";

        /// <summary>복구 owner다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public UIntBox OwnerBox = new() { Value = 7u };

        /// <summary>복구 subscriber다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public UIntBox IdentityBox = new() { Value = 0x11223344u };

        /// <summary>꼬리 전역 세 개다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public UIntBox[] TailBoxes = [new() { Value = 0xababababu }, new() { Value = 0xababababu }, new() { Value = 0xababababu }];

        /// <summary>두 Player 물리다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[][] Players = [new byte[1620], new byte[1620]];

        /// <summary>9×16 장부 캐시다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Cache = new byte[144];

        /// <summary>이전 이름이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Handle = new byte[256];

        /// <summary>명시 초기 stack이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Scratch = new byte[SavedStateFormat.ScratchBytes];

        /// <summary>고정 경로다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] PathBytes = new byte[256];

        /// <summary>파일 바이트다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Data = [];

        /// <summary>파일 커서다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int Position;

        /// <summary>선택된 외부 호출 순번이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public Dictionary<string, uint> Calls = [];

        /// <summary>실제 pending 이름이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public string Pending = "BeforeSaved";

        /// <summary>실제 발화 순서다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public string Trace = string.Empty;

        /// <summary>부분 중단 여부다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public bool Stopped;

        /// <summary>해제 상태다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int Cleanup;

        /// <summary>현재 Player base다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int ActiveBase = 1;

        /// <summary>fixture 입력열만 해석해 원시 상태를 준비한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="input">쉼표 8열 입력이다.</param>
        public SavedDevices(string input)
        {
            string[] fields = input.Split(',');
            if (fields.Length != 8)
            {
                throw new InvalidOperationException("SavedState 입력열 수 오류");
            }

            Present = int.Parse(fields[0]);
            Profile = uint.Parse(fields[1]);
            ParticipantInput = uint.Parse(fields[2]);
            TailInput = uint.Parse(fields[3]);
            TimeInput = uint.Parse(fields[4]);
            Length = uint.Parse(fields[5]);
            Initial = uint.Parse(fields[6]);
            Mutation = fields[7];
            Scratch = SavedScratch(Initial);
            Data = SavedImage(Profile, ParticipantInput, TailInput, TimeInput);
            if (Length < Data.Length)
            {
                Array.Resize(ref Data, (int)Length);
            }

            Array.Fill(Cache, (byte)0x91);
            Encoding.Latin1.GetBytes("BeforeHandle").CopyTo(Handle, 0);
            Encoding.Latin1.GetBytes("!state.dat").CopyTo(PathBytes, 0);
            // 원문에서 복사할 이전 Player 이름을 미리 입력하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int block = 0; block < Players.Length; ++block)
            {
                Array.Fill(Players[block], (byte)(0x31 + block));
                // 아홉 기록 이름을 순서대로 입력하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                for (uint player = 0; player < 9; ++player)
                {
                    string name = $"Old{block}{player}";
                    Encoding.Latin1.GetBytes(name).CopyTo(Players[block], (int)(player * 180 + 12));
                    Players[block][player * 180 + 12 + name.Length] = 0;
                }
            }
        }

        /// <summary>DWORD를 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="bytes">대상이다.</param>
        /// <param name="at">위치다.</param>
        /// <param name="value">값이다.</param>
        private static void Put(byte[] bytes, int at, uint value)
        {
            // 낮은 바이트부터 쓰는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                bytes[at + index] = (byte)(value >> (index * 8));
            }
        }

        /// <summary>명시 초기 stack 392바이트 입력이다. 원문 결과와 같은 동일 초기 물리만 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="profile">초기 판본 겸 서명 판본이다.</param>
        /// <returns>초기 물리다.</returns>
        private static byte[] SavedScratch(uint profile)
        {
            var bytes = new byte[SavedStateFormat.ScratchBytes];
            Array.Fill(bytes, profile != 0u ? (byte)0xa5 : (byte)0);
            Put(bytes, 4, 1u);
            Put(bytes, 8, 0x22334455u);
            Put(bytes, 0, 0x201u);
            Put(bytes, 0x3c, 0x80000000u);
            Put(bytes, 0x40, profile != 0u ? SavedStateFormat.Signature : 0u);
            (int At, string Name, int Width)[] regions = [(0x48, "StackA", 32), (0x68, "StackB", 32), (0x88, "StackFort", 256)];
            // 세 명시 영역을 같은 입력으로 두는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            foreach ((int at, string name, int width) in regions)
            {
                Array.Fill(bytes, (byte)0, at, width);
                Encoding.Latin1.GetBytes(name).CopyTo(bytes, at);
            }

            return bytes;
        }

        /// <summary>저장 파일의 고정 레이아웃 입력만 작성한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="profile">이름 판본이다.</param>
        /// <param name="participants">참가자 수다.</param>
        /// <param name="tail">꼬리 DWORD다.</param>
        /// <param name="time">저장 시각이다.</param>
        /// <returns>파일 바이트다.</returns>
        private static byte[] SavedImage(uint profile, uint participants, uint tail, uint time)
        {
            var bytes = new byte[976];
            Put(bytes, 0, profile == 5u ? 0x12345678u : SavedStateFormat.Signature);
            string[] names = ["SavedBattle", ".Hidden", "Has.Dot", string.Empty, new string('A', 255), "Short"];
            Encoding.Latin1.GetBytes(names[profile]).CopyTo(bytes, 4);
            Put(bytes, 260, 1u);
            Put(bytes, 264, time);
            Put(bytes, 268, 0x8899aabbu);
            // 여덟 86바이트 레코드를 서로 다른 명시 입력으로 준비하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (uint index = 0; index < 8; ++index)
            {
                int at = 272 + (int)(index * 86);
                string first = $"New{index + 1}";
                string second = $"Address{index + 1}";
                Put(bytes, at, index + 1);
                Put(bytes, at + 4, 100u + index);
                Encoding.Latin1.GetBytes(first).CopyTo(bytes, at + 8);
                Encoding.Latin1.GetBytes(second).CopyTo(bytes, at + 38);
                Put(bytes, at + 68, index < participants ? 0x41u : 0x201u);
                Put(bytes, at + 72, 200u + index);
                // 마지막 Core 10바이트는 별도 값으로 두는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                for (uint octet = 0; octet < 10; ++octet)
                {
                    bytes[at + 76 + octet] = (byte)(index * 17 + octet * 13);
                }
            }

            Put(bytes, 960, 0x11223344u);
            Put(bytes, 964, 0x55667788u);
            Put(bytes, 968, 0x99aabbccu);
            Put(bytes, 972, tail);
            return bytes;
        }

        /// <summary>실제 발화 순서만 trace에 기록한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="kind">호출 종류다.</param>
        /// <param name="text">기록 문구다.</param>
        /// <returns>호출 순번이다.</returns>
        public uint Event(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }

            Trace += text;
            Calls[kind] = Calls.TryGetValue(kind, out uint count) ? count + 1u : 1u;
            return Calls[kind];
        }

        /// <summary>원문 도구의 외부 효과 입력만 동일하게 공급한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="boundary">호출 경계다.</param>
        public void Mutate(string boundary)
        {
            int split = Mutation.IndexOf('-');
            if (split < 0 || Mutation.Substring(0, split) != boundary)
            {
                return;
            }

            string action = Mutation.Substring(split + 1);
            if (action == "stop")
            {
                Stopped = true;
                throw new SavedStopException();
            }
            else if (action == "cursor976")
            {
                Position = 976;
            }
            else if (action == "owner8")
            {
                OwnerBox.Value = 8u;
            }
            else if (action == "baseB")
            {
                ActiveBase = 2;
            }
            else if (action == "baseA")
            {
                ActiveBase = 1;
            }
            else if (action == "flags0")
            {
                Put(Scratch, 0, 0u);
            }
            else if (action == "time0")
            {
                Put(Scratch, 0x3c, 0u);
            }
            else if (action == "tail1")
            {
                Put(Scratch, 0x34, 1u);
            }
            else if (action == "tail0")
            {
                Put(Scratch, 0x34, 0u);
            }
            else if (!action.StartsWith("short", StringComparison.Ordinal))
            {
                throw new InvalidOperationException("SavedState 입력 변경 미해석");
            }
        }

        /// <summary>같은 전역 DWORD와 현재 Player 물리를 빌린다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>판독 전역이다.</returns>
        public SavedStateReadGlobals Globals()
        {
            var scene = this;
            var globals = new SavedStateReadGlobals
            {
                Owner = () => scene.OwnerBox,
                Subscriber = () => scene.IdentityBox,
                PreviousHandle = () => scene.Handle,
                Players = () =>
                {
                    scene.Event("base", $"B:{scene.ActiveBase}");
                    return scene.Players[scene.ActiveBase - 1];
                },
            };
            // 각 고정 전역의 직접 대상을 연결하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < scene.TailBoxes.Length; ++index)
            {
                int owned = index;
                globals.Tail[owned] = () => scene.TailBoxes[owned];
            }

            return globals;
        }

        /// <summary>실제 일반 파일 읽기는 부분 폭만 복사하고 나머지를 비워주지 않는다. 실제 저장소에 직접 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="buffer">쓸 실제 저장소다.</param>
        /// <param name="offset">쓸 위치다.</param>
        /// <param name="length">요청 폭이다.</param>
        /// <returns>복사한 바이트 수다.</returns>
        private uint Read(byte[] buffer, int offset, int length)
        {
            string boundary = $"read{(Calls.TryGetValue("read", out uint count) ? count + 1u : 1u)}";
            int rest = Position < Data.Length ? Data.Length - Position : 0;
            int width = Math.Min(length, rest);
            if (Mutation == boundary + "-short0")
            {
                width = 0;
            }
            else if (Mutation == boundary + "-short2")
            {
                width = Math.Min(width, 2);
            }

            Array.Copy(Data, Math.Min(Position, Data.Length), buffer, offset, width);
            Position += width;
            string tag = ReferenceEquals(buffer, Scratch) ? $"S{offset}" : "G";
            uint number = Event("read", $"R:{tag}:{length}:{width}");
            Mutate($"read{number}");
            return (uint)width;
        }

        /// <summary>최초 도구의 외부 경계 응답이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>판독 훅이다.</returns>
        public SavedStateReadHooks Hooks()
        {
            var scene = this;
            return new SavedStateReadHooks
            {
                Path = () =>
                {
                    uint number = scene.Event("path", "P");
                    scene.Mutate($"path{number}");
                    return scene.PathBytes;
                },
                Exists = name =>
                {
                    Assert.Equal("!state.dat", name);
                    uint number = scene.Event("exists", "E:!state.dat");
                    scene.Mutate($"exists{number}");
                    return scene.Present;
                },
                Open = (name, fileObject, first, second) =>
                {
                    Assert.Equal("!state.dat", name);
                    Assert.Equal(24, fileObject.Length);
                    Assert.Equal(0u, first);
                    Assert.Equal(0u, second);
                    uint number = scene.Event("open", "O:!state.dat:0:0");
                    scene.Mutate($"open{number}");
                    return 101u;
                },
                Read = (file, buffer, offset, length, limit) =>
                {
                    Assert.Equal(101u, file);
                    Assert.Equal(0xffffffffu, limit);
                    return scene.Read(buffer, offset, length);
                },
                Close = (file, fileObject) =>
                {
                    Assert.Equal(101u, file);
                    Assert.Equal(24, fileObject.Length);
                    if (scene.Stopped)
                    {
                        scene.Cleanup = 1;
                    }

                    uint number = scene.Event("close", "D");
                    scene.Mutate($"close{number}");
                },
                Pending = name =>
                {
                    scene.Pending = name;
                    uint number = scene.Event("pending", $"N:{Hex(Encoding.Latin1.GetBytes(name))}");
                    scene.Mutate($"pending{number}");
                },
                InitializeCore = core =>
                {
                    Assert.Equal(10, core.Length);
                    Array.Fill(core, (byte)0);
                    uint number = scene.Event("init", "I");
                    scene.Mutate($"init{number}");
                },
                Metadata = (number, subscriber, core) =>
                {
                    if (number >= 9u)
                    {
                        throw new ArgumentOutOfRangeException(nameof(number), "SavedState 명시 9기록 캐시");
                    }

                    Put(scene.Cache, (int)(number * 16), subscriber);
                    Array.Copy(core, 0, scene.Cache, (int)(number * 16 + 4), 10);
                    uint current = scene.Event("metadata", $"M:{number}:{subscriber}:{Hex(core)}");
                    scene.Mutate($"metadata{current}");
                },
            };
        }

        /// <summary>실제 현재 전역/파일 커서/예외 해제 상태를 그대로 표현한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>상태 열이다.</returns>
        public string State() => $"{OwnerBox.Value},{IdentityBox.Value},{TailBoxes[0].Value},{TailBoxes[1].Value},{TailBoxes[2].Value},{ActiveBase},{Position},{Cleanup}";

        /// <summary>명시 두 Player/캐시/이름 전체의 모든 바이트를 hex로 표현한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>원시 열이다.</returns>
        public string Raw() => Hex(Handle) + Hex(Cache) + Hex(Players[0]) + Hex(Players[1]);

        /// <summary>pending 열을 표현한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>pending 열이다.</returns>
        public string PendingColumn() => Pending.Length == 0 ? "-" : Hex(Encoding.Latin1.GetBytes(Pending));

        /// <summary>바이트를 소문자 hex로 표현한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="bytes">바꿀 바이트다.</param>
        /// <returns>hex 문자열이다.</returns>
        public static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    }

    /// <summary>fixture 읽기 trace의 주소 토큰을 마스킹한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="trace">원문 trace다.</param>
    /// <returns>정규화 trace다.</returns>
    private static string NormalizeTrace(string trace) => Regex.Replace(trace, @"R:[^:]*:", "R:*:");

    /// <summary>지원 물리의 모든 538 관측을 전역/Player/stack/파일 해제와 실제 전체 trace에 대조한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ReplaySavedStateReadFixture()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "saved-state-read-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int compared = 0;
        int stops = 0;
        int physical = 0;
        int cleanups = 0;
        var seen = new HashSet<string>();
        // 주석 외 모든 행을 한 번씩 독립 물리에서 재현하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        foreach (string raw in lines)
        {
            string line = raw.EndsWith('\r') ? raw.Substring(0, raw.Length - 1) : raw;
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row.Length != 9)
            {
                throw new InvalidOperationException("SavedState fixture 열 수 오류");
            }

            Assert.True(seen.Add(row[0] + "\n" + row[1]));
            if (row[2] == "fault")
            {
                ++physical;
                continue;
            }

            var devices = new SavedDevices(row[1]);
            bool completed = true;
            int result = 0;
            try
            {
                result = RawSavedState.ReadSavedState(devices.Scratch, devices.Globals(), devices.Hooks());
            }
            catch (SavedStopException)
            {
                completed = false;
                ++stops;
            }

            Assert.Equal(row[2] == "normal", completed);
            if (completed)
            {
                Assert.Equal(uint.Parse(row[3]), unchecked((uint)result));
            }

            Assert.Equal(row[4], devices.State());
            Assert.Equal(row[5], devices.PendingColumn());
            Assert.Equal(row[6], devices.Raw());
            Assert.Equal(row[7], SavedDevices.Hex(devices.Scratch));
            Assert.Equal(NormalizeTrace(row[8]), NormalizeTrace(devices.Trace));
            ++compared;
            cleanups += devices.Cleanup;
        }

        Assert.Equal(538, compared);
        Assert.Equal(32, stops);
        Assert.Equal(4, physical);
        Assert.Equal(24, cleanups);
    }

    /// <summary>없음은 미선택 장치를 요구하지 않고 선택된 누락 하위는 해제 뒤 거부된다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingDevicesAndCleanup()
    {
        var absent = new SavedDevices("0,0,2,0,1,976,0,none");
        var none = absent.Hooks();
        none.Open = null;
        none.Read = null;
        none.Close = null;
        none.Pending = null;
        none.InitializeCore = null;
        none.Metadata = null;
        Assert.Equal(0, RawSavedState.ReadSavedState([], absent.Globals(), none));
        Assert.False(absent.Calls.ContainsKey("open"));
        // 서로 다른 실제 하위의 부분 prefix/파일 해제를 확인하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (uint stage = 0; stage < 4; ++stage)
        {
            var devices = new SavedDevices("1,0,2,0,1,976,0,none");
            var hooks = devices.Hooks();
            if (stage == 0)
            {
                hooks.Read = null;
            }
            else if (stage == 1)
            {
                hooks.Pending = null;
            }
            else if (stage == 2)
            {
                hooks.InitializeCore = null;
            }
            else
            {
                hooks.Metadata = null;
            }

            bool rejected = false;
            try
            {
                RawSavedState.ReadSavedState(devices.Scratch, devices.Globals(), hooks);
            }
            catch (InvalidOperationException)
            {
                rejected = true;
            }

            Assert.True(rejected);
            Assert.Equal(1u, devices.Calls["open"]);
            Assert.Equal(1u, devices.Calls["close"]);
            uint expectedReads = stage == 0 ? 0u : stage == 1 ? 2u : stage == 2 ? 5u : 12u;
            Assert.Equal(expectedReads, devices.Calls.TryGetValue("read", out uint reads) ? reads : 0u);
        }
    }

    /// <summary>원문 안전 계약 밖의 Player 번호와 NUL 없는 물리 끝은 명시 실패이고 앞선 prefix는 남는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void PhysicalContractsPreservePrefix()
    {
        var devices = new SavedDevices("1,0,2,0,1,976,1,none");
        Put(devices.Data, 272, 9u);
        bool rejected = false;
        try
        {
            RawSavedState.ReadSavedState(devices.Scratch, devices.Globals(), devices.Hooks());
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        Assert.Equal("SavedBattle", devices.Pending);
        Assert.Equal(0u, devices.Calls.TryGetValue("metadata", out uint metadata) ? metadata : 0u);
        Assert.Equal(1u, devices.Calls["close"]);
        Assert.Equal(358, devices.Position);
        var missingNul = new SavedDevices("1,0,2,0,1,976,0,none");
        Array.Fill(missingNul.Data, (byte)'A', 4, 256);
        rejected = false;
        try
        {
            RawSavedState.ReadSavedState(missingNul.Scratch, missingNul.Globals(), missingNul.Hooks());
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        Assert.Equal("BeforeSaved", missingNul.Pending);
        Assert.Equal(1u, missingNul.Calls["close"]);
        Assert.Equal(260, missingNul.Position);
    }

    /// <summary>끝 검증 DWORD가 아예 없어도 원문은 초기 0을 사용한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ShortTailKeepsInitialZero()
    {
        // 전체·짧은 꼬리 입력을 검증하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        foreach (int length in new[] { 976, 972 })
        {
            var devices = new SavedDevices("1,0,2,0,2147483648,976,0,none");
            Array.Resize(ref devices.Data, length);
            int value = RawSavedState.ReadSavedState(devices.Scratch, devices.Globals(), devices.Hooks());
            Assert.Equal(0x80000000u, unchecked((uint)value));
            Assert.Equal("SavedBattle", devices.Pending);
            Assert.Equal(8u, devices.Calls["metadata"]);
            Assert.Equal(1u, devices.Calls["close"]);
            Assert.Equal(length, devices.Position);
        }
    }

    /// <summary>DWORD를 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">대상이다.</param>
    /// <param name="at">위치다.</param>
    /// <param name="value">값이다.</param>
    private static void Put(byte[] bytes, int at, uint value)
    {
        // 낮은 바이트부터 쓰는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }
}
