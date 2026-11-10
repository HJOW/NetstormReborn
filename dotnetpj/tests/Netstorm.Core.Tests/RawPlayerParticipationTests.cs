// PATCH Player 기록·방향별 표 전체 출력과 로그인·AI 수명 효과를 검증한다.
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerParticipation의 로그인·동맹·brain을 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class RawPlayerParticipationTests
{
    /// <summary>실제 Player 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Stride = 0xb4;

    /// <summary>원본 상태·효과를 준비하는 장면이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class ParticipationScene
    {
        /// <summary>실제 9×180바이트 저장소다.</summary>
        public byte[] Players = new byte[1620];

        /// <summary>실제 81 DWORD 행렬이다.</summary>
        public uint[] Allies = new uint[81];

        /// <summary>실제 숨김 9바이트다.</summary>
        public byte[] Hidden = new byte[9];

        /// <summary>호출마다 참조할 현재 모드다.</summary>
        public PlayerParticipationState State = new();

        /// <summary>원본 외부 효과의 인자·순서 관찰이다.</summary>
        public PlayerParticipationHooks Hooks = new();

        /// <summary>실제 발생한 효과의 순서다.</summary>
        public string Trace = "";

        /// <summary>독립 입력 13개를 현재 저장소에 적용한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="fields">입력 칸이다.</param>
        public ParticipationScene(string[] fields)
        {
            Assert.True(fields.Length == 13 || fields.Length == 14);
            static uint Number(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
            int owner = int.Parse(fields[0], CultureInfo.InvariantCulture);
            State.PlayerKnown = Number(fields[6]);
            State.LocalOwner = Number(fields[7]);
            State.Metamap = Number(fields[8]) != 0u;
            State.Battle = Number(fields[9]) != 0u;
            State.LocalTransport = Number(fields[10]) != 0u;
            // 실제 이름·식별자·독립 방향별 표를 준비하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int player = 0; player < 9; ++player)
            {
                byte[] name = Encoding.Latin1.GetBytes("P" + player);
                Array.Copy(name, 0, Players, player * 180 + 12, name.Length);
                Put(Players, player * 180 + 44, unchecked((uint)(player + 100)));
                Array.Fill(Players, (byte)'Z', player * 180 + 48, 30);
                Players[player * 180 + 78] = (byte)'~';
                Players[player * 180 + 79] = 127;
                Hidden[player] = (byte)((Number(fields[11]) >> player) & 1u);
                // 초기자료는 대칭 완성 동맹이 아닌 입력 패턴인 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int target = 0; target < 9; ++target)
                {
                    Allies[player * 9 + target] = player == target ? 1u : (player + target) % 3 == 0 ? 7u : 0u;
                }
            }

            int selected = owner >= 0 && owner < 9 ? owner : 0;
            Put(Players, selected * 180 + 80, Number(fields[3]));
            Put(Players, selected * 180 + 84, Number(fields[2]));
            Put(Players, selected * 180 + 176, Number(fields[4]));
            Put(Players, selected * 180 + 44, Number(fields[5]));
            if (fields.Length == 14)
            {
                int length = (int)Number(fields[13]);
                Array.Fill(Players, (byte)'N', selected * 180 + 12, length);
                Players[selected * 180 + 12 + length] = 0;
            }

            Hooks.Assertion = kind => Add("A," + (kind == PlayerParticipationAssertion.BrainAlreadyAssigned ? "B" : "M"));
            Hooks.AssignConnection = (pending, target) => Add("M," + unchecked((uint)pending) + "," + unchecked((uint)target));
            Hooks.RejectLogin = pending => Add("R," + unchecked((uint)pending));
            Hooks.Disconnect = pending => Add("D," + unchecked((uint)pending));
            Hooks.ClearChat = channel => Add("X," + channel);
            Hooks.AddChat = (subscriber, name, channel) => Add("C," + subscriber + "," + Convert.ToHexString(Encoding.Latin1.GetBytes(name)).ToLowerInvariant() + "," + channel);
            Hooks.SendAlliance = (server, player, mask) => Add("S," + server + "," + player + "," + mask);
            Hooks.DestructBrain = token => Add("K," + token);
            Hooks.FreeBrain = token => Add("F," + token);
        }

        /// <summary>원본 효과의 호출만 기록한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="value">적을 사건이다.</param>
        public void Add(string value)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }

            Trace += value;
        }

        /// <summary>현재 저장소를 빌리는 처리기를 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>실행 몸체다.</returns>
        public RawPlayerParticipation Runtime() => new(OriginalEdition.Patch1078, Players, Allies, Hidden, State, Hooks);

        /// <summary>방향별 표 전체를 바이트 순서로 직렬화한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>소문자 16진수다.</returns>
        public string AllyBytes()
        {
            byte[] bytes = new byte[324];
            // 각 칸의 전체 DWORD를 정규화하지 않고 검사하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Allies.Length; ++index)
            {
                Put(bytes, index * 4, Allies[index]);
            }

            return Hex(bytes);
        }
    }

    /// <summary>전체 바이트를 낮은 순서 hex로 기록한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>소문자 16진수다.</returns>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>명시 입력의 DWORD를 little endian으로 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 저장소다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 원본 정수 폭 네 바이트만 변경하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본의 전체 기록·행렬과 모든 외부 효과를 고유 입력 608개에서 비교한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void Native608()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-participation-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int count = 0;
        // 원본이 반환한 모든 출력을 입력 순서대로 대조하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row[0] == "kind")
            {
                continue;
            }

            Assert.Equal(6, row.Length);
            string[] fields = row[1].Split(',');
            ParticipationScene scene = new(fields);
            RawPlayerParticipation runtime = scene.Runtime();
            uint owner = unchecked((uint)long.Parse(fields[0], CultureInfo.InvariantCulture));
            if (row[0] == "Login")
            {
                Assert.Equal(row[2] != "0", runtime.CheckLogin(int.Parse(fields[1], CultureInfo.InvariantCulture), int.Parse(fields[0], CultureInfo.InvariantCulture)));
            }
            else if (row[0] == "Alliance")
            {
                runtime.SetAlliance(owner, unchecked((uint)long.Parse(fields[2], CultureInfo.InvariantCulture)));
            }
            else if (row[0] == "Add")
            {
                runtime.AddAlly(owner, unchecked((uint)long.Parse(fields[1], CultureInfo.InvariantCulture)));
            }
            else if (row[0] == "Attach")
            {
                runtime.AttachBrain(owner, unchecked((uint)long.Parse(fields[12], CultureInfo.InvariantCulture)));
            }
            else if (row[0] == "Remove")
            {
                runtime.RemoveBrain(owner);
            }
            else if (row[0] == "RemoveAll")
            {
                runtime.RemoveAllBrains();
            }
            else
            {
                throw new InvalidOperationException("Player 참여 입력 종류 오류");
            }

            Assert.Equal(row[3], Hex(scene.Players));
            Assert.Equal(row[4], scene.AllyBytes());
            Assert.Equal(row[5], scene.Trace.Length == 0 ? "-" : scene.Trace);
            ++count;
        }

        Assert.Equal(608, count);
    }

    /// <summary>원본 수신 288개 입력의 전체 Player·방향표·현재 packet과 모든 효과를 대조한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void AlliancePacketNative288()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-alliance-packet-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int count = 0;
        // 기대 출력은 계산하지 않고 동결한 원본 전체 함수 반환만 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row[0] == "input")
            {
                continue;
            }

            string[] values = row[0].Split(',');
            Assert.Equal(5, values.Length);
            Assert.Equal(9, row.Length);
            string[] fields = [values[0], "1", "1515870810", "0", "0", "0", values[2], values[3], values[4], "0", "0", "341", "7002"];
            ParticipationScene scene = new(fields);
            RawPlayerParticipation runtime = scene.Runtime();
            byte[] storage = new byte[41];
            Array.Fill(storage, (byte)0xa7);
            Span<byte> packet = storage.AsSpan(16, 9);
            packet[0] = 1;
            packet[1] = 0x3d;
            packet[2] = 9;
            packet[3] = 0;
            packet[4] = (byte)uint.Parse(values[0], CultureInfo.InvariantCulture);
            Put(storage, 21, uint.Parse(values[1], CultureInfo.InvariantCulture));
            runtime.ReceiveAlliance(packet, broadcast =>
            {
                scene.Add("B," + Convert.ToHexString(broadcast).ToLowerInvariant());
            });
            string[] actual = [Hex(scene.Players), scene.AllyBytes(), scene.Trace.Length == 0 ? "-" : scene.Trace, Convert.ToHexString(packet).ToLowerInvariant()];
            Assert.Equal(row[1..5], actual);
            Assert.Equal(row[5..9], actual);
            Assert.True(storage.AsSpan(0, 16).ToArray().All(b => b == 0xa7) && storage.AsSpan(25).ToArray().All(b => b == 0xa7));
            ++count;
        }

        Assert.Equal(288, count);
    }

    /// <summary>같은 brain 토큰은 파괴·반납 경계가 모두 끝날 때까지 조회된다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void BrainLifetimeAndDirectionalRows()
    {
        ParticipationScene scene = new("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        RawPlayerParticipation runtime = scene.Runtime();
        runtime.AttachBrain(1u, 7002u);
        Assert.Equal(7002u, runtime.Brain(1u));
        scene.Hooks.DestructBrain = token => Assert.True(token == 7002u && runtime.Brain(1u) == 7002u);
        scene.Hooks.FreeBrain = token => Assert.True(token == 7002u && runtime.Brain(1u) == 7002u);
        RawPlayerParticipation actual = scene.Runtime();
        actual.RemoveBrain(1u);
        Assert.Equal(0u, actual.Brain(1u));
        Assert.True((scene.Players[180 + 80] & 0x20) != 0);
        actual.SetAlliance(1u, 1u << 2);
        Assert.True(scene.Allies[1 * 9 + 2] == 1u && scene.Allies[2 * 9 + 1] == 7u);
        actual.SetAlliance(2u, 0u);
        Assert.True(scene.Allies[1 * 9 + 2] == 1u && scene.Allies[2 * 9 + 1] == 0u);
    }

    /// <summary>실제 owner1..8 자식만 정리하며 증빙 소비 뒤 설치 효과를 닫는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void StateResetProofCompletesActualParentsAndClosesOwners()
    {
        ParticipationScene scene = new("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        RawPlayerParticipation runtime = scene.Runtime();
        Put(scene.Players, 0xb0, 9000u);
        // 원문 cleanup의 8회 reset 위치를 그대로 고르는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint owner = 1u; owner < 9u; ++owner)
        {
            Put(scene.Players, (int)(owner * 180 + 0xb0), 7000u + owner);
            Put(scene.Players, (int)(owner * 180 + 0x50), 0x12340020u);
            RawPlayerParticipation.BrainResetPass pass = runtime.RemoveBrainForState(owner);
            Assert.Equal(owner, pass.Owner);
            Assert.False(runtime.BrainReturnedForState(owner));
            Assert.Equal(0u, runtime.Brain(owner));
            runtime.ReturnBrainForState(pass);
            Assert.True(runtime.BrainReturnedForState(owner));
            Assert.Throws<InvalidOperationException>(() => runtime.AttachBrain(owner, 8000u + owner));
            Assert.Throws<InvalidOperationException>(() => runtime.RemoveBrain(owner));
            Assert.True((scene.Players[(int)(owner * 180 + 0x50)] & 0x20) != 0);
        }

        Assert.Equal(9000u, runtime.Brain(0u));
        Assert.False(runtime.BrainReturnedForState(0u));
        Assert.Equal("K,7001;F,7001;K,7002;F,7002;K,7003;F,7003;K,7004;F,7004;K,7005;F,7005;K,7006;F,7006;K,7007;F,7007;K,7008;F,7008", scene.Trace);
        Assert.Throws<ArgumentOutOfRangeException>(() => runtime.RemoveBrainForState(0u));
        Assert.Throws<ArgumentOutOfRangeException>(() => runtime.RemoveBrainForState(9u));
    }

    /// <summary>실제 반납 실패에서는 증빙이 없고 선행 효과와 현재 brain을 그대로 남긴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void StateResetFailureDoesNotPublishProofOrHidePrefix()
    {
        ParticipationScene scene = new("1,1,0,0,7002,0,0,1,0,0,0,0,7002".Split(','));
        bool failed = false;
        scene.Hooks.FreeBrain = token =>
        {
            scene.Add("F," + token);
            throw new InvalidOperationException("명시 반납 실패");
        };
        RawPlayerParticipation runtime = scene.Runtime();
        RawPlayerParticipation.BrainResetPass other = runtime.RemoveBrainForState(2u);
        try
        {
            runtime.RemoveBrainForState(1u);
        }
        catch (InvalidOperationException)
        {
            failed = true;
        }

        Assert.True(failed && scene.Trace == "K,7002;F,7002" && runtime.Brain(1u) == 7002u && !runtime.BrainReturnedForState(1u));
        runtime.ReturnBrainForState(other);
        Assert.True(runtime.BrainReturnedForState(2u));
    }

    /// <summary>다른 모듈·이동한 증빙·소멸 뒤 새 모듈은 이전 정상 복귀를 재사용할 수 없다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void StateResetForeignMovedAndExpiredProofsAreRejected()
    {
        ParticipationScene scene = new("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        RawPlayerParticipation first = scene.Runtime();
        RawPlayerParticipation second = scene.Runtime();
        RawPlayerParticipation.BrainResetPass pass = first.RemoveBrainForState(1u);
        Assert.Throws<ArgumentException>(() => second.ReturnBrainForState(pass));
        first.ReturnBrainForState(pass);
        Assert.True(first.BrainReturnedForState(1u) && !second.BrainReturnedForState(1u));
        Assert.Throws<InvalidOperationException>(() => first.ReturnBrainForState(pass));
        RawPlayerParticipation.BrainResetPass? old = null;
        RawPlayerParticipation? reused = null;
        reused = scene.Runtime();
        old = reused.RemoveBrainForState(3u);
        reused = scene.Runtime();
        Assert.Throws<ArgumentException>(() => reused.ReturnBrainForState(old));
        Assert.False(reused.BrainReturnedForState(3u));
    }

    /// <summary>가상 파괴 callback이 아직 실행 중이면 이전 증빙도 소비할 수 없다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void StateResetActualEffectMustReturnBeforeConsumption()
    {
        ParticipationScene scene = new("1,1,0,0,7002,0,0,1,0,0,0,0,7002".Split(','));
        RawPlayerParticipation? current = null;
        RawPlayerParticipation.BrainResetPass? previous = null;
        bool rejected = false;
        scene.Hooks.DestructBrain = token =>
        {
            Assert.Equal(7002u, token);
            Assert.Equal(7002u, current!.Brain(1u));
            try
            {
                current.ReturnBrainForState(previous!);
            }
            catch (InvalidOperationException error)
            {
                rejected = error.Message.Contains("진행 중", StringComparison.Ordinal);
            }
        };
        RawPlayerParticipation runtime = scene.Runtime();
        current = runtime;
        previous = runtime.RemoveBrainForState(2u);
        RawPlayerParticipation.BrainResetPass pass = runtime.RemoveBrainForState(1u);
        Assert.True(rejected && !runtime.BrainReturnedForState(1u) && !runtime.BrainReturnedForState(2u));
        runtime.ReturnBrainForState(pass);
        runtime.ReturnBrainForState(previous);
        Assert.True(runtime.BrainReturnedForState(1u) && runtime.BrainReturnedForState(2u));
    }

    /// <summary>전체의 반납은 이미 반납한 NULL을 허용하되 뒤 소유자를 생략하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void GuideRemoveAllKeepsRetiredNullsAndVisitsFollowingOwners()
    {
        ParticipationScene scene = new("1,1,0,0,0,0,0,1,0,0,0,0,7002".Split(','));
        RawPlayerParticipation runtime = scene.Runtime();
        Put(scene.Players, 0xb0, 9000u);
        RawPlayerParticipation.BrainResetPass first = runtime.RemoveBrainForState(1u);
        runtime.ReturnBrainForState(first);
        Put(scene.Players, 2 * 180 + 0xb0, 7002u);
        Put(scene.Players, 2 * 180 + 0x50, 0x12340020u);
        runtime.RemoveAllBrains();
        Assert.True(runtime.BrainReturnedForState(1u) && !runtime.BrainReturnedForState(2u) && runtime.Brain(2u) == 0u && runtime.Brain(0u) == 9000u);
        Assert.Equal("K,7002;F,7002", scene.Trace);
        runtime.RemoveAllBrains();
        Assert.Equal("K,7002;F,7002", scene.Trace);
        Put(scene.Players, 180 + 0xb0, 0x12345678u);
        Put(scene.Players, 2 * 180 + 0xb0, 7003u);
        Assert.Throws<InvalidOperationException>(() => runtime.RemoveAllBrains());
        Assert.Equal(7003u, runtime.Brain(2u));
        Assert.Equal("K,7002;F,7002", scene.Trace);
        Put(scene.Players, 180 + 0xb0, 0u);
        Put(scene.Players, 2 * 180 + 0xb0, 0u);
    }

    /// <summary>동맹 해제가 battle에서 bit·칸을 지우고 현재 마스크를 서버에 요청하는지 검증한다. 2026-11-11 F3 동맹 해제 본체에서 추가됐다.</summary>
    [Fact]
    public void RemoveAllyClearsBitAndSendsMaskInBattle()
    {
        // 원본 48f710의 battle 송신 경로만 둔다. PlayerKnown 0이라 Chat은 닿지 않는다.
        string trace = "";
        var runtime = new RawPlayerParticipation(
            OriginalEdition.Patch1078, new byte[1620], new uint[81], new byte[9],
            new PlayerParticipationState { Battle = true },
            new PlayerParticipationHooks { SendAlliance = (server, owner, mask) => trace += $"S,{server},{owner},{mask}" });
        runtime.SetAlliance(1u, 0xfu);
        Assert.Equal("", trace);
        runtime.RemoveAlly(1u, 2u);
        Assert.Equal("S,9,1,11", trace);
    }

    /// <summary>동맹 해제가 battle이 아닐 때는 송신 없이 bit·칸만 지우는지 검증한다. 2026-11-11 F3 동맹 해제 본체에서 추가됐다.</summary>
    [Fact]
    public void RemoveAllyStaysSilentWhenNotBattle()
    {
        // 같은 마스크에서 battle 끔으로 지운 뒤 battle 켜고 다시 지워 지속을 확인한다.
        string trace = "";
        var state = new PlayerParticipationState();
        var runtime = new RawPlayerParticipation(
            OriginalEdition.Patch1078, new byte[1620], new uint[81], new byte[9],
            state,
            new PlayerParticipationHooks { SendAlliance = (server, owner, mask) => trace += $"S,{server},{owner},{mask}" });
        runtime.SetAlliance(1u, 0xfu);
        runtime.RemoveAlly(1u, 2u);
        Assert.Equal("", trace);
        state.Battle = true;
        runtime.RemoveAlly(1u, 3u);
        Assert.Equal("S,9,1,3", trace);
    }

    /// <summary>방향성 표 범위를 벗어난 동맹 해제를 거부하는지 검증한다. 2026-11-11 F3 동맹 해제 본체에서 추가됐다.</summary>
    [Fact]
    public void RemoveAllyRejectsCellOutOfRange()
    {
        // 원본의 방향성 표 범위 검사를 그대로 둔다. 소유자 9는 Player 경계에서 먼저 멈춘다.
        var runtime = new RawPlayerParticipation(
            OriginalEdition.Patch1078, new byte[1620], new uint[81], new byte[9],
            new PlayerParticipationState(),
            new PlayerParticipationHooks());
        Assert.Throws<ArgumentOutOfRangeException>(() => runtime.RemoveAlly(8u, 9u));
        Assert.Throws<ArgumentOutOfRangeException>(() => runtime.RemoveAlly(9u, 0u));
    }
}
