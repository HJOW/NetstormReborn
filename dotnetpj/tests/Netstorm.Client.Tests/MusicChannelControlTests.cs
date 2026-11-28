using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원문 음악 제어5292행을 실제96BYTE 채널·부분 읽기·공유 음소거로 전수 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>명시 파일/COM 입력만 공급하고 실제 채널의 상태 전이·출력·잠금·원문 사건을 검증한다.</summary>
public sealed class MusicChannelControlTests
{
    /// <summary>원본 관측의16BYTE 간격 장치 버퍼 기준이다.</summary>
    private const uint BufferBase = 0x16001000;
    /// <summary>원본 관측에 공급한 실제 대체 파일 토큰이다.</summary>
    private const uint FileToken = 0x17001000;
    /// <summary>signed 인자도 원문DWORD 비트로 읽는다.</summary>
    private static uint Number(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원문DWORD 인자를 signed 비트 그대로 읽는다.</summary>
    private static int Whole(string text) => unchecked((int)Number(text));

    /// <summary>원문 실행기의 명시 파일·버퍼 반환 입력과 사건만 보유한다. 채널 상태를 대신 갱신하지 않는다.</summary>
    private sealed class Device
    {
        /// <summary>실제 제어와 효과음 재생기가 공유할 현재 전역이다.</summary>
        public SoundState Sound = null!;
        /// <summary>읽기 입력으로 공급하는 실제 메모리 파일 바이트다.</summary>
        public byte[] File = [];
        /// <summary>실제 대체 파일 위치다. DWORD 절대 위치도 손실 없이 보관한다.</summary>
        public long Position = 5;
        /// <summary>실제 대체 파일의 현재 개방 여부다.</summary>
        public bool Opened = true;
        /// <summary>읽기 실패-1·무제한-2·양수 부분 읽기 상한이다.</summary>
        public int Cap = -2;
        /// <summary>되감기 실패를 공급할 원문 signed 입력이다.</summary>
        public int SeekResult;
        /// <summary>현재 버퍼 참조 해제 HRESULT 입력이다.</summary>
        public int ReleaseResult;
        /// <summary>현재 음량 COM HRESULT 입력이다.</summary>
        public int GainResult;
        /// <summary>원문 재귀 잠금 관측의 현재 깊이다.</summary>
        public int LockDepth;
        /// <summary>다음 해당 음량 호출에서 실제 공유 전역을 바꾸는 명시 재진입 입력이다.</summary>
        public (bool Music, int Depth, int Pending)? Callback;
        /// <summary>IO·COM·보고·잠금 순서를 보존하는 이번 명령 사건이다.</summary>
        private readonly List<string> _events = [];
        /// <summary>IO 예외 때 실제 잠금과 부분 출력이 보존되는지 검사할 선택적 입력이다.</summary>
        public bool ThrowRead;

        /// <summary>원문처럼 장치 토큰을16BYTE 간격 순번으로 기록한다.</summary>
        private static string Ordinal(uint buffer) => "#" + (unchecked(buffer - BufferBase) / 16u).ToString(CultureInfo.InvariantCulture);
        /// <summary>실제 음량 호출과 명시 재진입 입력을 해당 버퍼에서 한 번 기록한다.</summary>
        private int Gain(uint buffer, int volume)
        {
            _events.Add("V:" + Ordinal(buffer) + ':' + volume);
            if (Callback is { } callback && buffer == BufferBase + (callback.Music ? 16u : 0u))
            {
                Callback = null;
                Sound.VolumeHoldDepth = callback.Depth;
                Sound.PendingMusicVolume = callback.Pending;
                _events.Add($"J:{callback.Depth}:{callback.Pending}");
            }
            return GainResult;
        }
        /// <summary>동일 실제 효과음 본체의 음량 경계만 공급하고 범위 밖 호출은 실패한다.</summary>
        public SoundDeviceHooks Effects() => new()
        {
            Status = _ => throw new InvalidOperationException("미선택 효과음 상태"),
            Play = (_, _) => throw new InvalidOperationException("미선택 효과음 재생"),
            SetPosition = (_, _) => throw new InvalidOperationException("미선택 효과음 위치"),
            SetVolume = Gain,
            SetPan = (_, _) => throw new InvalidOperationException("미선택 효과음 좌우"),
            Stop = _ => throw new InvalidOperationException("미선택 효과음 정지"),
            Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("미선택 효과음 복제"),
            Load = _ => throw new InvalidOperationException("미선택 효과음 적재"),
        };
        /// <summary>실제 채널이 요청한 IO/COM만 명시 반환 입력으로 처리한다.</summary>
        public MusicChannelHooks Hooks() => new()
        {
            SetVolume = Gain,
            Stop = buffer => _events.Add("X:" + Ordinal(buffer)),
            Release = buffer => { _events.Add("B:" + Ordinal(buffer)); return ReleaseResult; },
            Seek = (token, offset) =>
            {
                int result = Opened && token == FileToken && SeekResult != -1 ? unchecked((int)offset) : -1;
                _events.Add($"S:{token}:{offset}:{result}");
                if (result != -1) Position = offset;
                return result;
            },
            Read = (token, target) =>
            {
                if (ThrowRead) throw new IOException("명시 파일 읽기 예외");
                long before = Position;
                int result = -1;
                if (Opened && token == FileToken && Cap != -1)
                {
                    int size = (int)Math.Min(target.Length, Position > File.Length ? 0 : File.Length - Position);
                    if (Cap >= 0) size = Math.Min(size, Cap);
                    if (size != 0) File.AsSpan((int)Position, size).CopyTo(target);
                    Position += size;
                    result = size;
                }
                _events.Add($"R:{token}:{before}:{target.Length}:{result}");
                return result;
            },
            Close = token => { _events.Add("C:" + token); Opened = false; },
            Log = (channel, text) => _events.Add($"G:{channel}:" + text.TrimEnd('\n')),
            Report = (_, line) => _events.Add("!" + line),
            LockEvent = entering => { LockDepth += entering ? 1 : -1; Assert.True(LockDepth >= 0); _events.Add(entering ? "E" : "L"); },
        };
        /// <summary>원문 관측의 반복 가능한 실제 파일 입력을 작성한다. 열린 여부를 임의로 복구하지 않는다.</summary>
        public void SetFile(uint length, uint offset, uint tail = 0)
        {
            File = new byte[checked(5 + (int)length + (int)tail)];
            Position = 5L + offset;
            // 원문 실행기와 같은BYTE 입력만 공급하고 기대 출력은 계산하지 않는다.
            for (int index = 0; index < File.Length; index++) File[index] = unchecked((byte)(index * 7 + 11));
        }
        /// <summary>현재 실제 사건을 읽고 다음 명령용 기록만 비운다.</summary>
        public string Take()
        {
            string result = _events.Count == 0 ? "-" : string.Join(';', _events);
            _events.Clear();
            return result;
        }
    }

    /// <summary>같은 전역·이름표·실제96BYTE 채널·음소거 부모를 연결한 검사 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>효과음과 공개 음악 제어가 공유하는 동일 현재 전역이다.</summary>
        public readonly SoundState Sound = new() { Initialized = true, Device = true, Enabled = true, MusicInitialized = true,
            MasterVolume = -400, PendingMasterVolume = 123, MusicVolume = -600, PendingMusicVolume = 222 };
        /// <summary>원문 명시 IO/COM 장치다.</summary>
        public readonly Device Device = new();
        /// <summary>실제 효과음 이름·항목 물리다.</summary>
        public readonly SoundList List;
        /// <summary>같은 이름표·전역의 실제 효과음 계층이다.</summary>
        public readonly SoundPlayer Effects;
        /// <summary>실제 원문96BYTE 소유자다.</summary>
        public readonly MusicChannelState State = new();
        /// <summary>명시 IO 장치와 같은 상태의 실제 채널 본체다.</summary>
        public readonly MusicChannel Channel;
        /// <summary>같은 효과음/채널 음량에 연결한 실제 공개 음악 제어다.</summary>
        public readonly SoundMusicControl Music;
        /// <summary>같은 실제 활성/시작/정지 장치이며 Open/버퍼 확보는 미이식null이다.</summary>
        public readonly MusicPlaybackDevices Playback;
        /// <summary>어느 단계에서도 하드웨어나 실제 파일을 열지 않는다.</summary>
        public Scene(OriginalEdition edition)
        {
            Device.Sound = Sound;
            List = new SoundList(edition);
            Effects = new SoundPlayer(List, Sound, Device.Effects());
            Channel = new MusicChannel(edition, State, Device.Hooks());
            Music = new SoundMusicControl(edition, Sound, Effects, Channel.SetVolume);
            Playback = Channel.BindPlaybackDevices();
        }
        /// <summary>원문 seed의 실제96BYTE 패턴과 버퍼/파일/효과음 입력을 설치한다.</summary>
        public void Seed()
        {
            List.Initialize();
            uint effect = List.Lookup("effect.wav");
            List.SetField(effect, SoundField.Buffer, BufferBase);
            List.SetField(effect, SoundField.Attenuation, 300);
            List.SetField(effect, SoundField.Volume, unchecked((uint)-200));
            Span<byte> raw = State.Raw().Span;
            // 패딩·원문 잠금 공간 등 미작성BYTE도 전수 대조할 수 있게 원문 입력 패턴을 설치한다.
            for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 17 + 3));
            State.SetField(MusicField.Buffer, BufferBase + 16);
            State.SetField(MusicField.BufferBytes, 16);
            State.SetField(MusicField.Flags, 3);
            State.SetField(MusicField.File, FileToken);
            State.SetField(MusicField.Length, 16);
            State.SetField(MusicField.ReadOffset, 0);
            State.SetField(MusicField.WriteOffset, 9);
            State.SetField(MusicField.DataOffset, 5);
            State.SetField(MusicField.StopDepth, 0);
            Device.SetFile(16, 0);
        }
    }

    /// <summary>세 원문 판본의 모든5292행을 동일 실제 몸체에 재생하고 전체 원시상태·출력·사건을 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 1780)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1756)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1756)]
    public void EveryOriginalMusicControlSequenceMatchesWholeState(string name, OriginalEdition edition, int expected)
    {
        string[][] all = X86Fixture.Read("musiccontrol");
        Assert.Equal(5292, all.Length);
        string[][] rows = all.Where(row => row[0] == name).ToArray();
        Assert.Equal(expected, rows.Length);
        // 기대값을 새로 만들지 않고 원문 명령을 같은 실제 제어·IO 계층에 전달한다.
        foreach (string[] row in rows) Replay(row, edition);
    }

    /// <summary>한 원문 행의 모든 입력·공개 호출을 재생하고 원시96BYTE/출력128BYTE/전체 이름표·파일·음량 전역을 확인한다.</summary>
    private static void Replay(string[] row, OriginalEdition edition)
    {
        Assert.Equal(13, row.Length);
        var scene = new Scene(edition);
        byte[] output = new byte[128];
        output.AsSpan().Fill(0xa7);
        var tokens = new List<string>();
        // 원문 전역/파일 입력과 실제 호출의 순서를 그대로 보존한다.
        foreach (string operation in row[1].Split('|'))
        {
            string[] op = operation.Split(':');
            string shown = "-";
            switch (op[0])
            {
                case "seed": scene.Seed(); break;
                case "g":
                    switch (op[1])
                    {
                        case "depth": scene.Sound.VolumeHoldDepth = Whole(op[2]); break;
                        case "musicinit": scene.Sound.MusicInitialized = Number(op[2]) != 0; break;
                        case "initialized": scene.Sound.Initialized = Number(op[2]) != 0; break;
                        default: throw new InvalidDataException("음악 제어 관측의 알 수 없는 전역");
                    }
                    break;
                case "f": scene.State.SetField((MusicField)Number(op[1]), Number(op[2])); break;
                case "mb": scene.State.SetField(MusicField.Buffer, Number(op[1]) != 0 ? BufferBase + 16 : 0); break;
                case "file":
                    scene.State.SetField(MusicField.Length, Number(op[1]));
                    scene.State.SetField(MusicField.ReadOffset, Number(op[2]));
                    scene.Device.SetFile(Number(op[1]), Number(op[2]), Number(op[3]));
                    break;
                case "cap": scene.Device.Cap = Whole(op[1]); break;
                case "seek": scene.Device.SeekResult = Whole(op[1]); break;
                case "hr": if (op[1] == "release") scene.Device.ReleaseResult = Whole(op[2]); else scene.Device.GainResult = Whole(op[2]); break;
                case "cb": scene.Device.Callback = (op[1] == "music", Whole(op[2]), Whole(op[3])); break;
                case "read": shown = scene.Channel.Read(output.AsSpan(0, (int)Number(op[1]))).ToString(CultureInfo.InvariantCulture); break;
                case "start": shown = scene.Channel.Start(Number(op[1])) ? "1" : "0"; break;
                case "rewind": shown = scene.Channel.Rewind() ? "1" : "0"; break;
                case "close": scene.Channel.Close(); break;
                case "stop": scene.Channel.Stop(); break;
                case "allstop": scene.Music.Stop(scene.Playback); break;
                case "mv": scene.Music.SetVolume(Whole(op[1])); break;
                case "master": scene.Effects.SetMasterVolume(Whole(op[1])); break;
                case "push": scene.Music.PushMute(); break;
                case "pop": scene.Music.PopMute(); break;
                default: throw new InvalidDataException("음악 제어 관측의 알 수 없는 명령");
            }
            Assert.Equal(0, scene.Device.LockDepth);
            tokens.Add(scene.Device.Take() + '=' + shown);
        }
        Assert.Equal(row[2], string.Join('|', tokens));
        Assert.Equal(Convert.FromHexString(row[3]), scene.State.Raw().ToArray());
        Assert.Equal(Convert.FromHexString(row[4]), output);
        Assert.Equal(Number(row[5]), X86Fixture.Adler32(scene.List.Raw().Span));
        Assert.Equal(long.Parse(row[6], CultureInfo.InvariantCulture), scene.Device.Position);
        Assert.Equal(row[7], scene.Device.Opened ? "1" : "0");
        Assert.Equal(row[8..], new[] { scene.Sound.MasterVolume, scene.Sound.PendingMasterVolume, scene.Sound.VolumeHoldDepth,
            scene.Sound.MusicVolume, scene.Sound.PendingMusicVolume }.Select(value => value.ToString(CultureInfo.InvariantCulture)).ToArray());
    }

    /// <summary>실제96BYTE·읽기·반복 시작·짧은IO·정지가 같은 파일/상태를 사용하고 미이식 장치는null로 남는다.</summary>
    [Fact]
    public void ActualChannelConnectsStartReadStopAndPreservesUnimplementedBoundaries()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Seed();
        scene.State.SetField(MusicField.Length, 8);
        scene.Device.SetFile(8, 0);
        Assert.Same(scene.State, scene.Channel.State);
        Assert.True(scene.Channel.Start(1));
        Assert.True(scene.Channel.Active());
        _ = scene.Device.Take();
        byte[] target = new byte[6];
        Assert.Equal(6, scene.Channel.Read(target));
        Assert.Equal(6u, scene.State.Field(MusicField.ReadOffset));
        Assert.Equal(6, scene.Channel.Read(target));
        Assert.Equal(4u, scene.State.Field(MusicField.ReadOffset));
        Assert.Equal(9, scene.Device.Position);
        Assert.Equal(scene.Device.File.AsSpan(11, 2).ToArray(), target[..2]);
        scene.Device.Cap = 2;
        Assert.Equal(0, scene.Channel.Read(target.AsSpan(0, 4)));
        Assert.False(scene.Device.Opened);
        Assert.Equal(FileToken, scene.State.Field(MusicField.File));
        Assert.Equal(4u, scene.State.Field(MusicField.ReadOffset));
        scene.State.Raw().Span[0x3a] = 0x5a;
        scene.Music.Stop(scene.Playback);
        Assert.False(scene.Channel.Active());
        Assert.Equal(0u, scene.State.Field(MusicField.Buffer));
        Assert.Equal(0u, scene.State.Field(MusicField.File));
        Assert.Equal((byte)0x5a, scene.State.Raw().Span[0x3a]);
        Assert.Null(scene.Playback.Open);
        Assert.Null(scene.Playback.EnsureBuffer);
        Assert.Throws<InvalidOperationException>(() => scene.Music.Play("track.mus", 1, new(), _ => null, scene.Playback));
        Assert.Equal(0, scene.Device.LockDepth);
    }

    /// <summary>비정렬DWORD와duration 원시비트의 실제96BYTE 범위를 확인하고 잘못된 접근은 쓰기 전에 거부한다.</summary>
    [Fact]
    public void StateUsesExactRawStorageAndDurationBits()
    {
        var state = new MusicChannelState();
        Assert.Equal(96, state.Raw().Length);
        state.Raw().Span.Fill(0x5a);
        state.SetField((MusicField)1, 0x12345678);
        Assert.Equal(0x12345678u, state.Field((MusicField)1));
        Assert.Equal((byte)0x5a, state.Raw().Span[0]);
        Assert.Equal((byte)0x5a, state.Raw().Span[5]);
        // NaN payload를 정규화하지 않는지 확인할 원문8BYTE 입력이다.
        const ulong nanBits = 0x7ff8000012345678;
        BinaryPrimitives.WriteUInt64LittleEndian(state.Raw().Span.Slice(0x18, 8), nanBits);
        byte[] before = state.Raw().ToArray();
        Assert.Equal(nanBits, BitConverter.DoubleToUInt64Bits(state.Duration()));
        Assert.Equal(before, state.Raw().ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => state.Field((MusicField)96));
        Assert.Throws<ArgumentOutOfRangeException>(() => state.SetField((MusicField)93, 0));
        Assert.Equal(before, state.Raw().ToArray());
    }

    /// <summary>IO 예외 뒤 실제 잠금은 다른 스레드가 다시 획득할 수 있고 선행 상태와 출력은 보존된다.</summary>
    [Fact]
    public async Task IoExceptionReleasesActualCommandLockWithoutClearingState()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Seed();
        byte[] before = scene.State.Raw().ToArray();
        byte[] target = [0xa7, 0xa7];
        scene.Device.ThrowRead = true;
        Assert.Throws<IOException>(() => scene.Channel.Read(target));
        Assert.Equal(before, scene.State.Raw().ToArray());
        Assert.Equal(new byte[] { 0xa7, 0xa7 }, target);
        Assert.Equal(0, scene.Device.LockDepth);
        Assert.True(await Task.Run(scene.Channel.Active, TestContext.Current.CancellationToken).WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken));
    }
}
