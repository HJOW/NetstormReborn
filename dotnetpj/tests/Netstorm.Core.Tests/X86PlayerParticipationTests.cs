using System.Globalization;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 10.78 의 로그인·방향별 동맹·AI 부착/해제 실제 기계어 관찰 608개를 <see cref="PlayerParticipation"/>의
/// Player 9명 1,620바이트·방향별 표 324바이트·외부 효과 인자와 순서·반환값으로 대조한다.
/// </summary>
/// <remarks>
/// 기대값은 cpppj/tests/fixtures/player-participation-x86.tsv(원본 PE 독립 실행, 링크 공유)이며 C# 에서 계산하지 않는다.
/// 초기 바이트 패턴과 입력 열의 의미는 cpppj/tests/PlayerParticipationTests.cpp 의 장면 구성을 따른다. 2026-10-15 감사 D03 에서 추가.
/// </remarks>
public sealed class X86PlayerParticipationTests
{
    /// <summary>fixture 의 고유 입력 개수. 같은 입력을 다른 검사에서 다시 더하지 않는다.</summary>
    private const int ExpectedRows = 608;

    /// <summary>fixture 열 수: 연산·입력·반환·Player9·표81·효과.</summary>
    private const int ColumnCount = 6;

    /// <summary>한 Player 기록의 폭(바이트).</summary>
    private const int Stride = PlayerParticipation.PlayerStride;

    /// <summary>TSV 의 부호 있는 십진수 칸을 읽는다(DWORD 감김은 호출자가 한다).</summary>
    private static long Number(string text) => long.Parse(text, CultureInfo.InvariantCulture);

    /// <summary>입력 칸을 원본 DWORD 로 감아 읽는다.</summary>
    private static uint Dword(string text) => unchecked((uint)Number(text));

    /// <summary>바이트열을 낮은 바이트부터 소문자 16진수로 만든다(fixture 형식).</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);

    /// <summary>DWORD 에 원본 정수 폭 네 바이트를 낮은 바이트부터 쓴다.</summary>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 원본 정수 폭 네 바이트만 바꾼다.
        for (int index = 0; index < 4; index++) bytes[offset + index] = (byte)(value >> (8 * index));
    }

    /// <summary>
    /// 원본 상태와 효과를 준비하는 장면. 기대하는 로그인/동맹/AI 판단을 계산하지 않고 입력 바이트와 실제 호출 기록만 만든다.
    /// </summary>
    private sealed class Scene
    {
        /// <summary>실제 폭 9×180바이트의 입력/출력 저장소.</summary>
        public byte[] Players { get; } = new byte[PlayerParticipation.PlayerCount * Stride];

        /// <summary>서로 다른 방향을 보존하는 방향별 9×9 표.</summary>
        public uint[] Allies { get; } = new uint[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerCount];

        /// <summary>Chat 숨김 표시 9개.</summary>
        public byte[] Hidden { get; } = new byte[PlayerParticipation.PlayerCount];

        /// <summary>호출마다 참조할 현재 전역 상태.</summary>
        public PlayerParticipationState State { get; } = new();

        /// <summary>실제로 발생한 효과의 순서. ';' 로 이어 붙인다.</summary>
        public List<string> Trace { get; } = [];

        /// <summary>원본 외부 효과를 호출 기록으로만 받는 훅. 가짜 연결이나 brain 은 만들지 않는다.</summary>
        public PlayerParticipationHooks Hooks { get; }

        /// <summary>
        /// 독립 입력 13~14개를 현재 저장소에만 적용한다. 열: 0 owner, 1 pending/target, 2 마스크, 3 flags, 4 brain,
        /// 5 구독자, 6 PlayerKnown, 7 로컬 번호, 8 metamap, 9 battle, 10 로컬 전송, 11 숨김 비트, 12 부착 토큰, 13 이름 길이(선택).
        /// </summary>
        /// <param name="fields">입력 칸.</param>
        public Scene(string[] fields)
        {
            Assert.True(fields.Length is 13 or 14);
            int owner = (int)Number(fields[0]);
            State.PlayerKnown = Dword(fields[6]);
            State.LocalOwner = Dword(fields[7]);
            State.Metamap = Dword(fields[8]) != 0;
            State.Battle = Dword(fields[9]) != 0;
            State.LocalTransport = Dword(fields[10]) != 0;
            // 실제 Player 이름/구독자/IP 와 독립 방향별 표를 준비한다.
            for (int player = 0; player < PlayerParticipation.PlayerCount; player++)
            {
                byte[] name = System.Text.Encoding.ASCII.GetBytes("P" + player.ToString(CultureInfo.InvariantCulture));
                name.CopyTo(Players, player * Stride + PlayerParticipation.NameOffset);
                Put(Players, player * Stride + 44, (uint)(player + 100));
                Array.Fill(Players, (byte)'Z', player * Stride + 48, 30);
                Players[player * Stride + 78] = (byte)'~';
                Players[player * Stride + 79] = 127;
                Hidden[player] = (byte)((Dword(fields[11]) >> player) & 1);
                // 초기 자료는 대칭으로 완성한 동맹이 아닌 입력 패턴이며 원본과 C# 에 똑같이 공급한다.
                for (int target = 0; target < PlayerParticipation.PlayerCount; target++)
                    Allies[player * PlayerParticipation.PlayerCount + target] = player == target ? 1U : (player + target) % 3 == 0 ? 7U : 0U;
            }
            int selected = owner >= 0 && owner < PlayerParticipation.PlayerCount ? owner : 0;
            Put(Players, selected * Stride + 80, Dword(fields[3]));
            Put(Players, selected * Stride + 84, Dword(fields[2]));
            Put(Players, selected * Stride + 176, Dword(fields[4]));
            Put(Players, selected * Stride + 44, Dword(fields[5]));
            if (fields.Length == 14)
            {
                int length = (int)Dword(fields[13]);
                Array.Fill(Players, (byte)'N', selected * Stride + 12, length);
                Players[selected * Stride + 12 + length] = 0;
            }
            Hooks = new PlayerParticipationHooks
            {
                Assertion = kind => Trace.Add("A," + (kind == PlayerParticipationAssertion.BrainAlreadyAssigned ? 'B' : 'M')),
                AssignConnection = (pending, target) => Trace.Add($"M,{unchecked((uint)pending)},{unchecked((uint)target)}"),
                RejectLogin = pending => Trace.Add($"R,{unchecked((uint)pending)}"),
                Disconnect = pending => Trace.Add($"D,{unchecked((uint)pending)}"),
                ClearChat = channel => Trace.Add($"X,{channel}"),
                AddChat = (subscriber, name, channel) => Trace.Add($"C,{subscriber},{Hex(name)},{channel}"),
                SendAlliance = (server, player, mask) => Trace.Add($"S,{server},{player},{mask}"),
                DestructBrain = token => Trace.Add($"K,{token}"),
                FreeBrain = token => Trace.Add($"F,{token}"),
            };
        }

        /// <summary>현재 저장소를 빌리는 처리기를 만든다. 게임 객체나 brain 을 생성하지 않는다.</summary>
        public PlayerParticipation Runtime() => new(Players, Allies, Hidden, State, Hooks);

        /// <summary>방향별 표 전체를 원본 DWORD 바이트 순서로 직렬화한다. 각 칸을 0/1로 정규화하지 않는다.</summary>
        public string AllyBytes()
        {
            var bytes = new byte[Allies.Length * 4];
            // 각 칸의 전체 DWORD 를 그대로 직렬화한다.
            for (int index = 0; index < Allies.Length; index++) Put(bytes, index * 4, Allies[index]);
            return Hex(bytes);
        }

        /// <summary>효과 기록을 fixture 형식(비어 있으면 "-")으로 돌려준다.</summary>
        public string TraceText() => Trace.Count == 0 ? "-" : string.Join(';', Trace);
    }

    /// <summary>원본 608개 고유 입력의 전체 Player 기록·방향별 표·효과 순서·반환값을 대조한다.</summary>
    [Fact]
    public void NativeParticipation_MatchesAllRecordsTablesAndEffects()
    {
        string[][] rows = X86Fixture.Read("player-participation").Where(row => row[0] != "kind").ToArray();
        Assert.Equal(ExpectedRows, rows.Length);
        var kinds = new Dictionary<string, int>();
        // 각 행은 독립 native 와 같은 새 저장소에서 시작한다.
        bool Matches(string[] row)
        {
            Assert.Equal(ColumnCount, row.Length);
            kinds[row[0]] = kinds.GetValueOrDefault(row[0]) + 1;
            string[] fields = row[1].Split(',');
            Scene scene = new(fields);
            PlayerParticipation runtime = scene.Runtime();
            uint owner = Dword(fields[0]);
            bool loginResult = true;
            switch (row[0])
            {
                case "Login": loginResult = runtime.CheckLogin((int)Number(fields[1]), (int)Number(fields[0])); break;
                case "Alliance": runtime.SetAlliance(owner, Dword(fields[2])); break;
                case "Add": runtime.AddAlly(owner, Dword(fields[1])); break;
                case "Attach": runtime.AttachBrain(owner, Dword(fields[12])); break;
                case "Remove": runtime.RemoveBrain(owner); break;
                case "RemoveAll": runtime.RemoveAllBrains(); break;
                default: throw new InvalidDataException("알 수 없는 Player 참여 fixture 연산입니다.");
            }
            bool returnOk = row[0] != "Login" || loginResult == (Number(row[2]) != 0);
            return returnOk && Hex(scene.Players) == row[3] && scene.AllyBytes() == row[4] && scene.TraceText() == row[5];
        }
        X86Fixture.CheckRows(rows, Matches);
        // 관찰 분포를 고정해 한 연산만 재생되거나 행이 조용히 줄어드는 일을 막는다.
        Assert.Equal(210, kinds["Login"]);
        Assert.Equal(122, kinds["Alliance"]);
        Assert.Equal(72, kinds["Add"]);
        Assert.Equal(192, kinds["Attach"]);
        Assert.Equal(6, kinds["Remove"]);
        Assert.Equal(6, kinds["RemoveAll"]);
    }

    /// <summary>
    /// 같은 brain 토큰은 파괴/반납 경계가 모두 끝날 때까지 조회되며, flags 0x20 은 해제 뒤에도 원본대로 남는다.
    /// 서로 다른 방향의 행은 독립이라 한쪽 설정이 반대 방향을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void BrainLifetimeAndDirectionalRows_FollowOriginalOrder()
    {
        var scene = new Scene("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        scene.Runtime().AttachBrain(1, 7002);
        Assert.Equal(7002u, scene.Runtime().Brain(1));
        PlayerParticipation? actual = null;
        var hooks = new PlayerParticipationHooks
        {
            DestructBrain = token => { Assert.Equal(7002u, token); Assert.Equal(7002u, actual!.Brain(1)); },
            FreeBrain = token => { Assert.Equal(7002u, token); Assert.Equal(7002u, actual!.Brain(1)); },
            ClearChat = _ => { },
            AddChat = (_, _, _) => { },
            Assertion = _ => { },
        };
        actual = new PlayerParticipation(scene.Players, scene.Allies, scene.Hidden, scene.State, hooks);
        actual.RemoveBrain(1);
        Assert.Equal(0u, actual.Brain(1));
        Assert.NotEqual(0u, actual.Flags(1) & PlayerParticipation.BrainFlag);

        actual.SetAlliance(1, 1U << 2);
        Assert.Equal(1u, scene.Allies[1 * 9 + 2]);
        Assert.Equal(7u, scene.Allies[2 * 9 + 1]);
        actual.SetAlliance(2, 0);
        Assert.Equal(1u, scene.Allies[1 * 9 + 2]);
        Assert.Equal(0u, scene.Allies[2 * 9 + 1]);
    }

    /// <summary>방향 조회·양방향 조회·참여 조건은 flags 0x22 와 표 한 칸만 읽는다.</summary>
    [Fact]
    public void Queries_ReadDirectionAndParticipationBitsOnly()
    {
        var scene = new Scene("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        PlayerParticipation runtime = scene.Runtime();
        Array.Clear(scene.Allies);
        scene.Allies[2 * 9 + 3] = 1;
        Assert.True(runtime.Allies(2, 3));
        Assert.False(runtime.Allies(3, 2));
        Assert.False(runtime.MutualAllies(2, 3));
        scene.Allies[3 * 9 + 2] = 1;
        Assert.True(runtime.MutualAllies(2, 3));
        // 범위 밖 번호는 동맹이 아니다.
        Assert.False(runtime.Allies(-1, 2));
        Assert.False(runtime.Allies(2, 9));
        // 연결 비트 0x01 만으로는 참여가 아니다. 0x02 또는 0x20 이 있어야 한다.
        Put(scene.Players, 4 * Stride + PlayerParticipation.FlagsOffset, 0x01);
        Assert.False(runtime.Participates(4));
        Put(scene.Players, 4 * Stride + PlayerParticipation.FlagsOffset, 0x02);
        Assert.True(runtime.Participates(4));
        Put(scene.Players, 5 * Stride + PlayerParticipation.FlagsOffset, 0x20);
        Assert.True(runtime.Participates(5));
        Assert.False(runtime.Participates(6));
        Assert.False(runtime.Participates(-1));
        Assert.False(runtime.Participates(9));
    }

    /// <summary>저장소 크기가 원본과 다르면 변경 전에 거부하고, 도달한 외부 효과가 비어 있으면 성공으로 숨기지 않는다.</summary>
    [Fact]
    public void Construction_RejectsWrongSizesAndMissingEffectsWhenReached()
    {
        var scene = new Scene("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        Assert.Throws<ArgumentException>(() => new PlayerParticipation(new byte[10], scene.Allies, scene.Hidden, scene.State, scene.Hooks));
        Assert.Throws<ArgumentException>(() => new PlayerParticipation(scene.Players, new uint[3], scene.Hidden, scene.State, scene.Hooks));
        Assert.Throws<ArgumentException>(() => new PlayerParticipation(scene.Players, scene.Allies, new byte[2], scene.State, scene.Hooks));
        var bare = new PlayerParticipation(scene.Players, scene.Allies, scene.Hidden, scene.State, new PlayerParticipationHooks());
        // 효과에 닿지 않는 경로(로그인 수락 + 로컬 전송)는 효과가 없어도 동작한다.
        scene.State.LocalTransport = true;
        Assert.True(bare.CheckLogin(2, 2));
        // 거부 경로는 거부 응답/연결 해제가 필요하다.
        Assert.Throws<InvalidOperationException>(() => bare.CheckLogin(2, 2));
        // 알림을 받을 Chat 효과가 필요한 동맹 설정(PlayerKnown 이 있을 때)도 같다.
        scene.State.PlayerKnown = 1;
        Assert.Throws<InvalidOperationException>(() => bare.SetAlliance(3, 0));
    }
}
