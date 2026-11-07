// Player 전체 초기화의 선택 장치 요구와 앞선 부분 효과를 검증한다.
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerInitialization의 장치 계약을 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerInitializationTests
{
    /// <summary>명시 Player 물리 식별자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private static readonly uint[] Bases = [0x12200000u, 0x12201000u, 0x12202000u];

    /// <summary>실제 Player 기록 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int PlayerBytes = 1620;

    /// <summary>주소→물리 저장소다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Dictionary<uint, byte[]> _blocks = new();

    /// <summary>호출 순서 기록이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly uint[] _calls = new uint[5];

    /// <summary>현재 base 슬롯이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private uint _base;

    /// <summary>현재 base 슬롯에 쓰는 갈고리다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerInitializationGlobals _globals = new();

    /// <summary>세 블록을 미리 확보한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="old">이전 base 번호다. 3이면 0이다.</param>
    private void Prepare(uint old)
    {
        _blocks.Clear();
        // 세 주소를 각각 독립 저장소에 두는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (uint address in Bases)
        {
            _blocks[address] = new byte[PlayerBytes];
        }

        _base = old == 3u ? 0u : Bases[(int)old];
        _globals.FortCount = 0x12345678u;
        _globals.LastLoginPlayer = 0x89abcdefu;
        _globals.PlayerBase = () => _base;
        _globals.WritePlayerBase = address => _base = address;
        Array.Clear(_calls);
    }

    /// <summary>현재 실제 할당에서 요청한 영역의 복사본을 빌린다. 이 검사의 쓰기는 관찰하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="address">읽을 주소다.</param>
    /// <param name="size">읽을 폭이다.</param>
    /// <returns>물리 복사본이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">미등록 실제 물리다.</exception>
    private byte[] Memory(uint address, uint size)
    {
        if (address < PlayerBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(address), "초기화 입력의 NULL 물리");
        }

        // 세 블록 안에서 포함하는 저장소를 찾는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach ((uint bottom, byte[] block) in _blocks)
        {
            if (address >= bottom && (ulong)address + size <= (ulong)bottom + (uint)block.Length)
            {
                byte[] view = new byte[size];
                Array.Copy(block, (int)(address - bottom), view, 0, (int)size);
                return view;
            }
        }

        throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 미등록 실제 물리");
    }

    /// <summary>실제 외부 장치를 묶는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>훅 묶음이다.</returns>
    private PlayerInitializationHooks Hooks()
    {
        return new PlayerInitializationHooks
        {
            Memory = (address, size) => Memory(address, size),
            FreeFactory = token => ++_calls[0],
            FreePlayers = token => ++_calls[1],
            MallocPlayers = size => { ++_calls[2]; return Bases[2]; },
            NewFactory = size => { ++_calls[3]; return 0x12310000u + (_calls[3] - 1u) * 0x100u; },
            Alliance = (owner, mask) => ++_calls[4],
        };
    }

    /// <summary>현재 입력 장치의 선택 중단이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class InitStopException : Exception
    {
    }

    /// <summary>현재 NULL 물리 실패다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class InitNullException : ArgumentOutOfRangeException
    {
        /// <summary>NULL 물리 실패를 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        public InitNullException()
            : base("address", "초기화 입력의 NULL 물리")
        {
        }
    }

    /// <summary>두 현재 owner·모드를 함께 읽는 작은 뷰다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class InitPlayerView
    {
        /// <summary>현재 known 값이다.</summary>
        public uint PlayerKnown;

        /// <summary>현재 전투 소유자다.</summary>
        public uint BattleOwner = 1u;
    }

    /// <summary>할당 주소·외부 입력·실제 동맹·Chat 하위만 제공하는 장면이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class InitScene
    {
        /// <summary>이전·새 주소의 stale 바이트를 각각 독립 보관한다.</summary>
        public readonly byte[][] Blocks = [new byte[PlayerBytes], new byte[PlayerBytes], new byte[PlayerBytes]];

        /// <summary>실제 방향별 9×9 DWORD 표다.</summary>
        public readonly uint[] Allies = new uint[81];

        /// <summary>실제 숨김 9바이트 입력이다.</summary>
        public readonly byte[] Hidden = new byte[9];

        /// <summary>실제 Chat 채널이 직접 읽고 쓰는 저장소다.</summary>
        public readonly byte[] Chat = new byte[4800];

        /// <summary>기존 동맹이 같은 known·owner를 빌린다.</summary>
        public readonly InitPlayerView Player = new();

        /// <summary>원본 세 초기화 전역이다.</summary>
        public uint Base;

        /// <summary>첫 적재 뒤 0을 쓰는 로그인 요새 수다.</summary>
        public uint FortCount = 0x12345678u;

        /// <summary>그다음 0을 쓰는 마지막 로그인 Player다.</summary>
        public uint LastPlayer = 0x89abcdefu;

        /// <summary>명시 할당 반환 번호다.</summary>
        public uint Allocated;

        /// <summary>공장 신규 NULL 판정 번호다.</summary>
        public uint NullNew;

        /// <summary>두 현재 Core 입력 중 주소 쪽이다.</summary>
        public uint My;

        /// <summary>두 현재 Core 입력 중 값 쪽이다.</summary>
        public uint Current = Bases[0] + 180u + 0x6cu;

        /// <summary>두 현재 Core 입력 중 활성 쪽이다.</summary>
        public uint Active = 0xa5a5a5a5u;

        /// <summary>호출 순서 기록이다.</summary>
        public readonly uint[] Calls = new uint[5];

        /// <summary>명시 callee 진입 중단이다.</summary>
        public string Stop = "none";

        /// <summary>현재 입력 변경이다.</summary>
        public string Mutation = "none";

        /// <summary>부모 주소 재조회와 실제 자식 진입 기록이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>실제 Chat 진입 기록이다.</summary>
        public readonly List<string> ChatEvents = [];

        /// <summary>Core 수명 검사만 실제 반납 위치를 관찰한다.</summary>
        public Action<uint>? OnFreePlayers { get; set; }

        /// <summary>Core 수명 검사만 실제 슬롯 쓰기 위치를 관찰한다.</summary>
        public Action<uint>? OnAssignedBase { get; set; }

        /// <summary>현재 전역이다. 제품 쓰기가 같은 인스턴스에 닿는다.</summary>
        public readonly PlayerInitializationGlobals GlobalsValue = new();

        /// <summary>기대 결과 없이 Native와 같은 초기 물리를 구성한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="input">원문 입력 10칸이다.</param>
        public InitScene(string input)
        {
            string[] fields = input.Split(',');
            if (fields.Length != 10)
            {
                throw new InvalidOperationException("Player 초기화 입력 칸 수");
            }

            uint seed = uint.Parse(fields[0], CultureInfo.InvariantCulture);
            uint old = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            Allocated = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            Player.PlayerKnown = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            uint mask = uint.Parse(fields[4], CultureInfo.InvariantCulture);
            NullNew = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            Stop = fields[6];
            Mutation = fields[7];
            uint hiddenMask = uint.Parse(fields[8], CultureInfo.InvariantCulture);
            My = uint.Parse(fields[9], CultureInfo.InvariantCulture);
            if (old > 3u || Allocated > 3u)
            {
                throw new ArgumentOutOfRangeException(nameof(input), "Player 초기화 입력 base 번호");
            }

            Base = old == 3u ? 0u : Bases[(int)old];
            // 이전·새 주소의 stale 바이트를 각각 독립 준비하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int block = 0; block < Blocks.Length; ++block)
            {
                byte[] bytes = Blocks[block];
                // Native 도구와 같은 독립 패턴인 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int index = 0; index < bytes.Length; ++index)
                {
                    bytes[index] = unchecked((byte)(seed + (uint)block * 53u + (uint)index * 17u));
                }

                // 명시 공장 포인터·이름·subscriber만 입력으로 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int owner = 0; owner < 9; ++owner)
                {
                    int offset = owner * 180;
                    PutDword(bytes, offset, ((mask >> owner) & 1u) != 0u ? unchecked(0x12408000u + (uint)block * 0x2000u + (uint)owner * 0x100u) : 0u);
                    Array.Fill(bytes, unchecked((byte)(65 + owner)), offset + 12, 32);
                    byte[] name = Encoding.Latin1.GetBytes("OldName\0");
                    Array.Copy(name, 0, bytes, offset + 12, name.Length);
                    PutDword(bytes, offset + 44, unchecked(700u + (uint)owner + (uint)block * 100u));
                }
            }

            // 기존 81 DWORD의 바이트 패턴을 조립하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Allies.Length; ++index)
            {
                uint value = 0u;
                // Native의 같은 초기 바이트를 모으는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int index2 = 0; index2 < 4; ++index2)
                {
                    value |= (uint)unchecked((byte)(seed + (uint)(index * 4 + index2) * 7u)) << (index2 * 8);
                }

                Allies[index] = value;
            }

            // 명시 숨김 입력을 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Hidden.Length; ++index)
            {
                Hidden[index] = (byte)((hiddenMask >> index) & 1u);
            }

            // 기존 Chat 입력 장치의 stale 바이트를 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Chat.Length; ++index)
            {
                Chat[index] = unchecked((byte)(index * 37));
            }

            // 실제 빈 구독자와 NUL 뒤 stale을 준비하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < 100; ++index)
            {
                int offset = index * 48;
                Array.Fill(Chat, unchecked((byte)(65 + index % 26)), offset, 40);
                byte[] name = Encoding.Latin1.GetBytes("OldName\0");
                Array.Copy(name, 0, Chat, offset, name.Length);
                Chat[offset + 40] = unchecked((byte)(index * 13 + 0x95));
                PutDword(Chat, offset + 44, 0u);
            }

            GlobalsValue.PlayerBase = () =>
            {
                Trace.Add("R:" + Base);
                return Base;
            };
            GlobalsValue.WritePlayerBase = address =>
            {
                Trace.Add("W:" + address);
                Base = address;
                OnAssignedBase?.Invoke(address);
            };
        }

        /// <summary>같은 현재 슬롯의 원문 적재·저장만 trace에 표시한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>현재 전역이다.</returns>
        public PlayerInitializationGlobals Globals()
        {
            GlobalsValue.FortCount = FortCount;
            GlobalsValue.LastLoginPlayer = LastPlayer;
            return GlobalsValue;
        }

        /// <summary>호출 뒤 바뀐 카운터를 장면에 되돌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        public void SyncGlobals()
        {
            FortCount = GlobalsValue.FortCount;
            LastPlayer = GlobalsValue.LastLoginPlayer;
        }

        /// <summary>현재 같은 물리 주소를 해석한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="address">읽을 주소다.</param>
        /// <param name="size">읽을 폭이다.</param>
        /// <returns>살아 있는 구간이다.</returns>
        /// <exception cref="InitNullException">NULL 물리다.</exception>
        /// <exception cref="ArgumentOutOfRangeException">미등록 실제 물리다.</exception>
        public Memory<byte> Memory(uint address, uint size)
        {
            if (address < PlayerBytes)
            {
                throw new InitNullException();
            }

            // 현재 실제 세 할당에서 요청한 영역만 고르는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Blocks.Length; ++index)
            {
                if (address >= Bases[index] && (ulong)address + size <= (ulong)Bases[index] + (uint)Blocks[index].Length)
                {
                    return Blocks[index].AsMemory((int)(address - Bases[index]), (int)size);
                }
            }

            throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 미등록 실제 물리");
        }

        /// <summary>callee 진입 관측이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="kind">호출 번호다.</param>
        /// <param name="name">호출 이름이다.</param>
        /// <param name="argument">호출 인자다.</param>
        /// <param name="mask">동맹 마스크다.</param>
        /// <returns>호출 순서다.</returns>
        /// <exception cref="InitStopException">선택 중단이다.</exception>
        public uint Boundary(int kind, string name, uint argument, uint mask = 0u)
        {
            uint count = ++Calls[kind];
            Trace.Add(kind == 4 ? "A:" + argument + ":" + mask : name + ":" + argument);
            if (Stop == name + count)
            {
                throw new InitStopException();
            }

            return count;
        }

        /// <summary>명시 외부 장치의 현재 base·필드 변경과 실제 부모 재진입이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="boundary">고를 경계다.</param>
        public void Mutate(string boundary)
        {
            int dash = Mutation.IndexOf('-');
            if (dash < 0 || Mutation.Substring(0, dash) != boundary)
            {
                return;
            }

            string action = Mutation.Substring(dash + 1);
            Trace.Add("M:" + action);
            if (action == "baseB")
            {
                Base = Bases[1];
            }
            else if (action == "baseNull")
            {
                Base = 0u;
            }
            else if (action == "row64")
            {
                WriteLive(Base + 0x64u, 0x12345678u);
            }
            else if (action == "known1")
            {
                Player.PlayerKnown = 1u;
            }
            else if (action == "reenter")
            {
                Trace.Add("RE:begin");
                RawPlayerInitialization.InitializePlayerRecords(Globals(), Hooks());
                Trace.Add("RE:end");
            }
            else
            {
                throw new InvalidOperationException("Player 초기화의 미해석 명시 변경");
            }
        }

        /// <summary>살아 있는 저장소에 직접 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="address">쓸 주소다.</param>
        /// <param name="value">쓸 값이다.</param>
        /// <exception cref="ArgumentOutOfRangeException">미등록 실제 물리다.</exception>
        private void WriteLive(uint address, uint value)
        {
            // 세 블록 안에서 포함하는 저장소를 찾는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Blocks.Length; ++index)
            {
                if (address >= Bases[index] && (ulong)address + 4u <= (ulong)Bases[index] + (uint)Blocks[index].Length)
                {
                    PutDword(Blocks[index], (int)(address - Bases[index]), value);
                    return;
                }
            }

            throw new ArgumentOutOfRangeException(nameof(address), "Player 초기화의 미등록 실제 물리");
        }

        /// <summary>실제 기존 방향별 동맹·Chat 몸체를 연결한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="owner">소유자 번호다.</param>
        /// <param name="mask">쓸 마스크다.</param>
        public void Alliance(uint owner, uint mask)
        {
            uint count = Boundary(4, "alliance", owner, mask);
            Mutate("alliance" + count);
            RawChatChannels channels = new(OriginalEdition.Patch1078, Chat, () => My, (call, subscriber) => ChatEvents.Add(call == ChatChannelCall.NameChanged ? "N," + subscriber : "R"));
            PlayerParticipationState state = new() { LocalOwner = Player.BattleOwner, PlayerKnown = Player.PlayerKnown };
            PlayerParticipationHooks hooks = new()
            {
                ClearChat = channel => channels.Clear(channel),
                AddChat = (subscriber, name, channel) => channels.AddToChannel(subscriber, name, channel),
                Assertion = _ => throw new InvalidOperationException("Player 초기화 입력의 예상 밖 동맹 진단"),
            };
            new RawPlayerParticipation(OriginalEdition.Patch1078, Memory(Base, 1620u), Allies, Hidden, state, hooks).SetAlliance(owner, mask);
        }

        /// <summary>실제 명시 allocator·해제와 검증한 동맹 자식을 반환한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>훅 묶음이다.</returns>
        public PlayerInitializationHooks Hooks()
        {
            return new PlayerInitializationHooks
            {
                Memory = (address, size) => Memory(address, size),
                FreeFactory = token =>
                {
                    uint count = Boundary(0, "freeFactory", token);
                    Mutate("freeFactory" + count);
                },
                FreePlayers = token =>
                {
                    uint count = Boundary(1, "freePlayers", token);
                    OnFreePlayers?.Invoke(token);
                    Mutate("freePlayers" + count);
                },
                MallocPlayers = size =>
                {
                    if (size != 1620u)
                    {
                        throw new InvalidOperationException("Player 초기화 malloc 폭 오류");
                    }

                    uint count = Boundary(2, "malloc", size);
                    Mutate("malloc" + count);
                    return Allocated == 3u ? 0u : Bases[(int)Allocated];
                },
                NewFactory = size =>
                {
                    if (size != 120u)
                    {
                        throw new InvalidOperationException("Player 초기화 공장 폭 오류");
                    }

                    uint count = Boundary(3, "new", size);
                    Mutate("new" + count);
                    return count == NullNew ? 0u : unchecked(0x12310000u + (count - 1u) * 0x100u);
                },
                Alliance = (owner, mask) => Alliance(owner, mask),
            };
        }

        /// <summary>정상·외부 중단·NULL 물리 실패만 원문과 같은 이름으로 분류한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>상태 이름이다.</returns>
        public string Run()
        {
            try
            {
                RawPlayerInitialization.InitializePlayerRecords(Globals(), Hooks());
                SyncGlobals();
                return "normal";
            }
            catch (InitStopException)
            {
                SyncGlobals();
                return "stop";
            }
            catch (InitNullException)
            {
                SyncGlobals();
                return "fault-null";
            }
        }

        /// <summary>원문 출력과 같은 부호 없는 DWORD 순서다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>전역 문자열이다.</returns>
        public string GlobalsText() => $"{Base},{FortCount},{LastPlayer},{Player.PlayerKnown},{Player.BattleOwner},{My},{Current},{Active}";

        /// <summary>같은 세 Player 물리 전체를 출력한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>16진수 문자열이다.</returns>
        public string PlayersHex() => Hex(Blocks[0]) + Hex(Blocks[1]) + Hex(Blocks[2]);

        /// <summary>실제 81 DWORD를 원문 little endian 바이트로 표시한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>16진수 문자열이다.</returns>
        public string AlliesHex()
        {
            byte[] bytes = new byte[324];
            // 표시만 수행하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Allies.Length; ++index)
            {
                PutDword(bytes, index * 4, Allies[index]);
            }

            return Hex(bytes);
        }

        /// <summary>같은 바이트를 원문 hex 형식으로 출력한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="bytes">바꿀 바이트다.</param>
        /// <returns>소문자 16진수다.</returns>
        public static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

        /// <summary>같은 사건 기록을 원문 구분자로 표시한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="values">적을 기록이다.</param>
        /// <returns>합친 문자열이다.</returns>
        public static string Join(List<string> values) => values.Count == 0 ? "-" : string.Join(';', values);

        /// <summary>입력과 출력 표시용으로만 DWORD를 little endian으로 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="bytes">쓸 저장소다.</param>
        /// <param name="offset">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        private static void PutDword(byte[] bytes, int offset, uint value)
        {
            // 지정한 4바이트만 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                bytes[offset + index] = (byte)(value >> (index * 8));
            }
        }
    }

    /// <summary>제품 전에 고정한 전체 정상·중단·NULL·재진입과 실제 동맹·Chat 결과를 각각 대조한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void Replays202IndependentNativeObservations()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-initialization-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        int normal = 0;
        int fault = 0;
        int stopped = 0;
        HashSet<string> inputs = [];
        // 헤더를 제외한 모든 최초 행을 비교하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (line.StartsWith("control\t", StringComparison.Ordinal))
            {
                continue;
            }

            if (string.IsNullOrEmpty(line))
            {
                continue;
            }

            string rowText = line.EndsWith('\r') ? line.Substring(0, line.Length - 1) : line;
            string[] fields = rowText.Split('\t');
            Assert.Equal(10, fields.Length);
            InitScene scene = new(fields[1]);
            string status = scene.Run();
            Compare(rows, "status", status, fields[2]);
            Compare(rows, "globals", scene.GlobalsText(), fields[3]);
            Compare(rows, "players", scene.PlayersHex(), fields[4]);
            Compare(rows, "allies", scene.AlliesHex(), fields[5]);
            Compare(rows, "chat", Convert.ToHexString(scene.Chat).ToLowerInvariant(), fields[6]);
            Compare(rows, "chatEvents", InitScene.Join(scene.ChatEvents), fields[7]);
            Compare(rows, "trace", InitScene.Join(scene.Trace), fields[8]);
            Assert.True(fields[0] == "027f" || fields[0] == "037f");
            Assert.True(uint.Parse(fields[9], CultureInfo.InvariantCulture) > 0u);
            if (status == "normal")
            {
                ++normal;
            }
            else if (status == "fault-null")
            {
                ++fault;
            }
            else
            {
                Assert.Equal("stop", status);
                ++stopped;
            }

            inputs.Add(fields[1]);
            ++rows;
        }

        Assert.Equal(202, rows);
        Assert.Equal(101, inputs.Count);
        Assert.Equal(174, normal);
        Assert.Equal(8, fault);
        Assert.Equal(20, stopped);
    }

    /// <summary>원문과 다른 부분만 짧게 표시한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="row">행 번호다.</param>
    /// <param name="field">칸 이름이다.</param>
    /// <param name="actual">실제 값이다.</param>
    /// <param name="expected">기대 값이다.</param>
    private static void Compare(int row, string field, string actual, string expected)
    {
        if (actual != expected)
        {
            int at = 0;
            while (at < actual.Length && at < expected.Length && actual[at] == expected[at])
            {
                ++at;
            }

            Assert.Fail($"PlayerInit row{row} {field} at{at} actual={actual.Substring(at, Math.Min(128, actual.Length - at))} expected={expected.Substring(at, Math.Min(128, expected.Length - at))}");
        }
    }
    /// <summary>선택하지 않은 해제는 요구하지 않으며 선택 누락·짧은 DWORD는 앞선 실제 효과를 남긴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void RequiredDevicesKeepEarlierPartialEffects()
    {
        Prepare(3u);
        PlayerInitializationHooks hooks = Hooks();
        hooks.FreeFactory = null;
        hooks.FreePlayers = null;
        hooks.MallocPlayers = null;
        bool rejected = false;
        try
        {
            RawPlayerInitialization.InitializePlayerRecords(_globals, hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }

        Assert.True(rejected && _globals.FortCount == 0u && _globals.LastLoginPlayer == 0u && _base == 0u && _calls[0] == 0u && _calls[1] == 0u && _calls[3] == 0u);
        Prepare(3u);
        byte[] before = (byte[])_blocks[Bases[2]].Clone();
        PlayerInitializationHooks shortHooks = Hooks();
        shortHooks.Memory = (address, size) =>
        {
            byte[] view = Memory(address, size);
            byte[] shortView = new byte[3];
            Array.Copy(view, 0, shortView, 0, 3);
            return shortView;
        };
        rejected = false;
        try
        {
            RawPlayerInitialization.InitializePlayerRecords(_globals, shortHooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected && _base == Bases[2] && _globals.FortCount == 0u && _globals.LastLoginPlayer == 0u && _calls[2] == 1u && _calls[3] == 0u && _blocks[Bases[2]].AsSpan().SequenceEqual(before));
    }
}
